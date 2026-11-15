//! Host-only encrypted IM regression over the production runtime/UDP adapter.
//! Session seeding follows the pinned SDK's own e2e fixture; no PASE/CASE handshake,
//! radio, actual attestation or physical controller is simulated by that setup.
use super::*;
use core::net::{Ipv6Addr, SocketAddr, SocketAddrV6};
use core::num::NonZeroU8;
use rs_matter::acl::{AclEntry, AuthMode};
use rs_matter::dm::Privilege;
use rs_matter::im::{AttrPath, AttrResp, IMStatusCode, OpCode, ReportDataResp, StatusResp};
use rs_matter::sc::ExchangeInitiator;
use rs_matter::tlv::{get_root_node_struct, FromTLV, TLVTag, TLVWrite, ToTLV};
use rs_matter::transport::exchange::Exchange;
use rs_matter::transport::network::{Address, NetworkReceive, NetworkSend, NoNetwork};
use rs_matter::transport::session::{NocCatIds, ReservedSession, SessionMode};

fn address(port: u16) -> Address {
    Address::Udp(SocketAddr::V6(SocketAddrV6::new(
        Ipv6Addr::LOCALHOST,
        port,
        0,
        1,
    )))
}
#[derive(Clone, Copy)]
struct ClientWire;
impl NetworkSend for ClientWire {
    async fn send_to(&mut self, data: &[u8], _: Address) -> Result<(), Error> {
        MATTER_INBOX.lock().unwrap().push_back(data.to_vec());
        Ok(())
    }
}
impl NetworkReceive for ClientWire {
    async fn wait_available(&mut self) -> Result<(), Error> {
        while CLIENT_INBOX.lock().unwrap().is_empty() {
            embassy_time::Timer::after_millis(5).await;
        }
        Ok(())
    }
    async fn recv_from(&mut self, data: &mut [u8]) -> Result<(usize, Address), Error> {
        loop {
            let packet = CLIENT_INBOX.lock().unwrap().pop_front();
            if let Some(packet) = packet {
                assert!(packet.len() <= data.len());
                data[..packet.len()].copy_from_slice(&packet);
                return Ok((packet.len(), address(5540)));
            }
            embassy_time::Timer::after_millis(5).await;
        }
    }
}
fn seed(matter: &Matter<'_>, local: u64, remote: u64, peer: Address, crypto: &impl Crypto) {
    matter.with_state(|state| {
        let fabric = state.fabrics.add_with_post_init(|_| Ok(())).unwrap();
        let mut acl = AclEntry::new(None, Privilege::ADMIN, AuthMode::Case);
        acl.add_subject(remote).unwrap();
        fabric.acl_add(acl).unwrap();
    });
    let mut session = ReservedSession::reserve_now(matter.transport(), crypto).unwrap();
    session
        .update(
            local,
            remote,
            1,
            1,
            peer,
            SessionMode::Case {
                fab_idx: NonZeroU8::new(1).unwrap(),
                cat_ids: NocCatIds::default(),
            },
            None,
            None,
            None,
            None,
        )
        .unwrap();
    session.complete();
}
pub(super) fn check() {
    let paths_before = PATHS.lock().unwrap().len();
    let mut root_parts = config().stations[..config().count as usize]
        .iter()
        .map(|station| station.endpoint)
        .collect::<std::vec::Vec<_>>();
    let legacy_one = root_parts.contains(&1);
    let primary_aggregator = if legacy_one { 2 } else { 1 };
    root_parts.push(2);
    if !legacy_one {
        root_parts.push(1);
    }
    root_parts.sort_unstable();
    MATTER_INBOX.lock().unwrap().clear();
    CLIENT_INBOX.lock().unwrap().clear();
    send_plan(&[], i32::MAX);
    let client = Box::new(Matter::new(
        &TEST_DEV_DET,
        DevCommData::Passcode(BasicCommData {
            password: Spake2pVerifierPassword::from(20202021u32.to_le_bytes()),
            discriminator: 3840,
        }),
        &TEST_DEV_ATT,
        5540,
    ));
    let crypto = default_crypto(platform::Random, DAC_PRIVKEY);
    seed(matter(), 12345678, 445566, address(4242), &crypto);
    seed(&client, 445566, 12345678, address(5540), &crypto);
    let mut transport = Box::pin(client.run(&crypto, ClientWire, ClientWire, NoNetwork));
    let mut flow = Box::pin(async {
        // An earlier unsupported path must not poison later valid paths or
        // exchanges. Include reversed endpoints, repeats, and both optional
        // cluster and nonexistent endpoint probes before the owner's exact path.
        // Scalar ClusterRevision forces packet chunking without list-item
        // chunking (which legitimately adds several reports per array path).
        let chunked = [(1, 29, 65533, true); 80];
        let cases: &[&[(u16, u32, u32, bool)]] = &[
            &[(1, 29, 0, true)],
            &[
                (0, 29, 3, true),
                (2, 29, 3, true),
                (1, 29, 3, true),
                (0, 29, 0, true),
                (2, 29, 0, true),
                (1, 29, 1, true),
            ],
            &[(6, 29, 0, true), (2, 29, 0, true), (1, 29, 0, true)],
            &[(65534, 29, 0, false), (1, 29, 0, true)],
            &[(0, 70, 65532, false), (1, 29, 0, true)],
            &[(1, 29, 0, true), (1, 29, 0, true)],
            &chunked,
        ];
        for paths in cases {
            let mut exchange =
                Exchange::initiate(&*client, &crypto, NonZeroU8::new(1).unwrap(), 12345678)
                    .await
                    .unwrap();
            exchange
                .send_with(|_, writer| {
                    writer.start_struct(&TLVTag::Anonymous)?;
                    writer.start_array(&TLVTag::Context(0))?;
                    for &(endpoint, cluster, attr, _) in *paths {
                        AttrPath {
                            endpoint: Some(endpoint),
                            cluster: Some(cluster),
                            attr: Some(attr),
                            ..Default::default()
                        }
                        .to_tlv(&TLVTag::Anonymous, &mut *writer)?;
                    }
                    writer.end_container()?;
                    writer.bool(&TLVTag::Context(3), false)?;
                    writer.end_container()?;
                    Ok(Some(OpCode::ReadRequest.into()))
                })
                .await
                .unwrap();
            let mut received = 0;
            let mut chunks = 0;
            loop {
                exchange.recv_fetch().await.unwrap();
                let more = {
                    let rx = exchange.rx().unwrap();
                    let response =
                        ReportDataResp::from_tlv(&get_root_node_struct(rx.payload()).unwrap())
                            .unwrap();
                    let reports = response
                        .attr_reports
                        .unwrap()
                        .into_iter()
                        .map(|report| report.unwrap())
                        .collect::<std::vec::Vec<_>>();
                    assert!(!reports.is_empty());
                    assert!(
                        received + reports.len() <= paths.len(),
                        "paths={} received={} chunk={} reports={reports:?}",
                        paths.len(),
                        received,
                        chunks
                    );
                    for (report, &(endpoint, cluster, attr, success)) in reports
                        .iter()
                        .zip(&paths[received..received + reports.len()])
                    {
                        match report {
                            AttrResp::Data(data) => {
                                assert!(success, "unsupported probe unexpectedly succeeded");
                                assert_eq!(data.path.endpoint, Some(endpoint));
                                assert_eq!(data.path.cluster, Some(cluster));
                                assert_eq!(data.path.attr, Some(attr));
                                // Check serialized topology and types, not just
                                // successful path expansion or response status.
                                if cluster == 29 && attr == 0 {
                                    let types = data
                                        .data
                                        .array()
                                        .unwrap()
                                        .iter()
                                        .map(|item| {
                                            item.unwrap()
                                                .structure()
                                                .unwrap()
                                                .ctx(0)
                                                .unwrap()
                                                .u32()
                                                .unwrap()
                                        })
                                        .collect::<std::vec::Vec<_>>();
                                    assert_eq!(
                                        types,
                                        match endpoint {
                                            0 => std::vec![0x16],
                                            2 => std::vec![0x0e],
                                            1 if !legacy_one => std::vec![0x0e],
                                            _ => std::vec![0x100, 0x13],
                                        }
                                    );
                                } else if cluster == 29 && attr == 3 {
                                    let parts = data
                                        .data
                                        .array()
                                        .unwrap()
                                        .iter()
                                        .map(|item| item.unwrap().u16().unwrap())
                                        .collect::<std::vec::Vec<_>>();
                                    assert_eq!(
                                        parts,
                                        match endpoint {
                                            0 => root_parts.clone(),
                                            ep if ep == primary_aggregator => root_parts
                                                .iter()
                                                .copied()
                                                .filter(|id| *id != 1 || legacy_one)
                                                .filter(|id| *id != 2)
                                                .collect(),
                                            _ => std::vec![],
                                        }
                                    );
                                } else if cluster == 29 && attr == 1 && endpoint == 1 {
                                    let clusters = data
                                        .data
                                        .array()
                                        .unwrap()
                                        .iter()
                                        .map(|item| item.unwrap().u32().unwrap())
                                        .collect::<std::vec::Vec<_>>();
                                    assert_eq!(
                                        clusters,
                                        if legacy_one {
                                            std::vec![29, 4, 6, 57]
                                        } else {
                                            std::vec![29]
                                        }
                                    );
                                }
                            }
                            AttrResp::Status(status) => {
                                assert!(
                                    !success,
                                    "encrypted CASE descriptor read failed: {status:?}"
                                );
                                assert_eq!(status.path.endpoint, Some(endpoint));
                                assert_eq!(status.path.cluster, Some(cluster));
                                assert_eq!(status.path.attr, Some(attr));
                                assert_eq!(
                                    status.status.status,
                                    if endpoint == 65534 {
                                        IMStatusCode::UnsupportedEndpoint
                                    } else {
                                        IMStatusCode::UnsupportedCluster
                                    }
                                );
                            }
                        }
                    }
                    received += reports.len();
                    chunks += 1;
                    response.more_chunks.unwrap_or(false)
                };
                if !more {
                    break;
                }
                exchange
                    .send_with(|_, writer| {
                        StatusResp::write(writer, IMStatusCode::Success)?;
                        Ok(Some(OpCode::StatusResponse.into()))
                    })
                    .await
                    .unwrap();
            }
            assert_eq!(received, paths.len());
            if paths.len() == chunked.len() {
                assert!(chunks > 1, "test must exercise actual ReportData chunking");
            }
            exchange.acknowledge().await.unwrap();
        }
    });
    let mut cx = Context::from_waker(Waker::noop());
    let mut done = false;
    for _ in 0..2000 {
        CLOCK_US.fetch_add(5000, Ordering::SeqCst);
        assert!(unsafe { lh_rs_pump() } >= 0);
        assert!(transport.as_mut().poll(&mut cx).is_pending());
        if flow.as_mut().poll(&mut cx).is_ready() {
            done = true;
            break;
        }
    }
    assert!(
        done,
        "encrypted read did not complete within simulated 10 s"
    );
    assert_eq!(
        &PATHS.lock().unwrap()[paths_before..],
        &[[1, 65534, 29, 0], [1, 0, 70, 65532]],
        "only deliberately unsupported probes may produce rejection paths"
    );
    assert_eq!(
        WRITES.load(Ordering::SeqCst),
        0,
        "descriptor/metadata reads must never send power"
    );
}
