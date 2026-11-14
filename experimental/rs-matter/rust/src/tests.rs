extern crate std;
use super::*;
use core::sync::atomic::{AtomicU32, Ordering};
use rs_matter::dm::{
    clusters::desc::{ClusterHandler as _, DescHandler},
    Metadata,
};
static WRITES: AtomicU32 = AtomicU32::new(0);
#[no_mangle]
extern "C" fn lh_rs_lock() -> u32 {
    0
}
#[no_mangle]
extern "C" fn lh_rs_unlock(_: u32) {}
#[no_mangle]
extern "C" fn lh_rs_time() -> u64 {
    CLOCK_US.load(Ordering::SeqCst)
}
#[no_mangle]
unsafe extern "C" fn lh_rs_random(out: *mut u8, size: usize) {
    core::ptr::write_bytes(out, 42, size);
}
#[no_mangle]
extern "C" fn lh_rs_generation(_: u16) -> u32 {
    0
}
#[no_mangle]
extern "C" fn lh_rs_observed(_: u16) -> u32 {
    2
}
#[no_mangle]
extern "C" fn lh_queue_power(_: u16, _: u32) -> bool {
    WRITES.fetch_add(1, Ordering::SeqCst);
    true
}
#[no_mangle]
extern "C" fn lh_rs_fault() {
    panic!("unexpected runtime panic")
}
fn configuration() -> Config {
    let mut cfg = Config {
        passcode: 20202021,
        discriminator: 3840,
        count: 16,
        stations: [Station {
            endpoint: 0,
            name: [0; 41],
            unique: [0; 40],
        }; 16],
        serial: [0; 33],
    };
    cfg.serial[..9].copy_from_slice(b"synthetic");
    for (i, s) in cfg.stations.iter_mut().enumerate() {
        s.endpoint = if i == 0 { 1 } else { (i + 2) as u16 };
        s.name[..4].copy_from_slice(b"Lamp");
        s.unique[..4].copy_from_slice(b"test");
    }
    cfg
}
#[test]
fn actual_rs_model_preserves_sparse_ids_and_never_writes_on_startup() {
    unsafe {
        CONFIG = Some(configuration());
    }
    let model = model::Model::new(&mut platform::Random);
    model.access(|node| {
        assert_eq!(node.endpoints.len(), 18);
        assert_eq!(node.endpoints[0].id, 0);
        assert_eq!(node.endpoints[1].id, 2);
        assert_eq!(node.endpoints[2].id, 1);
        assert_eq!(node.endpoints[17].id, 17);
        assert!(node
            .endpoints
            .iter()
            .all(|e| e.clusters.iter().any(|c| c.id == DescHandler::CLUSTER.id)));
    });
    assert_eq!(WRITES.load(Ordering::SeqCst), 0);
    // Reproduce a registry whose former endpoint 1 has been retired.
    let mut cfg = configuration();
    cfg.count = 4;
    for i in 0..4 {
        cfg.stations[i].endpoint = 7 + i as u16;
    }
    unsafe {
        CONFIG = Some(cfg);
    }
    let sparse = model::Model::new(&mut platform::Random);
    sparse.access(|node| {
        assert_eq!(
            node.endpoints
                .iter()
                .map(|e| e.id)
                .collect::<std::vec::Vec<_>>(),
            [0, 2, 7, 8, 9, 10]
        )
    });
    unsafe {
        (&mut *core::ptr::addr_of_mut!(CONFIG))
            .as_mut()
            .unwrap()
            .count = 0;
    }
    let empty = model::Model::new(&mut platform::Random);
    empty.access(|node| assert_eq!(node.endpoints.len(), 2));
    assert_eq!(WRITES.load(Ordering::SeqCst), 0);
    drop((model, sparse, empty));
    let cfg = configuration();
    unsafe {
        assert!(lh_rs_begin(&cfg));
    }
    for _ in 0..20 {
        poll_runtime();
    }
    assert!(lh_rs_arena_used() > 0);
    assert!(lh_rs_arena_used() + 1024 <= lh_rs_arena_bytes());
    // Retain protocol minima while freeing reservations, rather than limiting peers.
    assert!(rs_matter::fabric::MAX_FABRICS >= 5);
    assert!(rs_matter::session::MAX_SESSIONS >= 3 * rs_matter::fabric::MAX_FABRICS);
    assert!(rs_matter::transport::session::MAX_EXCHANGES >= 5);
    let mut code = [0; 12];
    let mut paired = false;
    let mut open = false;
    unsafe {
        assert!(lh_rs_info(
            code.as_mut_ptr(),
            code.len(),
            &mut paired,
            &mut open
        ));
    }
    assert!(!paired && !open);
    assert_eq!(
        code[0], 0,
        "startup cannot advertise pairing before the BLE worker is released"
    );
    unsafe {
        assert!(lh_rs_open());
    }
    unsafe {
        assert!(lh_rs_info(
            code.as_mut_ptr(),
            code.len(),
            &mut paired,
            &mut open
        ));
    }
    assert_eq!(
        code[0], 0,
        "pending renewal must hide the old code immediately"
    );
    for _ in 0..20 {
        poll_runtime();
    }
    unsafe {
        assert!(lh_rs_info(
            code.as_mut_ptr(),
            code.len(),
            &mut paired,
            &mut open
        ));
    }
    assert!(!paired && open);
    mdns_receive_checks();
    assert_ne!(platform::text(&code), "34970112332");
    CLOCK_US.fetch_add(901000000, Ordering::SeqCst);
    for _ in 0..20 {
        poll_runtime();
    }
    unsafe {
        assert!(lh_rs_info(
            code.as_mut_ptr(),
            code.len(),
            &mut paired,
            &mut open
        ));
    }
    assert!(
        !open && code[0] == 0,
        "expired window must not expose its code"
    );
    assert!(!NVS.lock().unwrap().is_empty());
    assert_eq!(
        WRITES.load(Ordering::SeqCst),
        0,
        "real startup/window renewal cannot send power"
    );
    udp_backpressure_checks();
}
#[test]
fn invalid_registry_or_commissioning_input_is_rejected() {
    let mut cfg = configuration();
    assert!(valid_config(&cfg));
    cfg.count = 17;
    assert!(!valid_config(&cfg));
    cfg.count = 16;
    for ep in [0, 2, 0xffff, 3] {
        cfg.stations[0].endpoint = ep;
        assert!(!valid_config(&cfg));
    }
    cfg.stations[0].endpoint = 1;
    cfg.discriminator = 4096;
    assert!(!valid_config(&cfg));
    cfg.discriminator = 3840;
    for pin in [0, 11111111, 12345678, 87654321, 100000000] {
        cfg.passcode = pin;
        assert!(!valid_config(&cfg));
    }
}
#[test]
fn manual_code_uses_the_exact_verifier_password() {
    let comm = BasicCommData {
        password: Spake2pVerifierPassword::from(20202021u32.to_le_bytes()),
        discriminator: 3840,
    };
    assert_eq!(comm.compute_pairing_code().as_str(), "34970112332");
}

// Native host shims exercise startup/persistence/window logic; no radio or flash.
static CLOCK_US: core::sync::atomic::AtomicU64 = core::sync::atomic::AtomicU64::new(0);
static NVS: std::sync::Mutex<std::collections::BTreeMap<u16, std::vec::Vec<u8>>> =
    std::sync::Mutex::new(std::collections::BTreeMap::new());
#[no_mangle]
unsafe extern "C" fn lh_rs_store(op: u32, key: u16, data: *mut u8, size: usize) -> i32 {
    let mut map = NVS.lock().unwrap();
    match op {
        0 => match map.get(&key) {
            Some(value) if value.len() <= size => {
                core::ptr::copy_nonoverlapping(value.as_ptr(), data, value.len());
                value.len() as i32
            }
            Some(_) => -2,
            None => -1,
        },
        1 => {
            map.insert(key, core::slice::from_raw_parts(data, size).to_vec());
            0
        }
        2 => {
            map.remove(&key);
            0
        }
        _ => -2,
    }
}
#[repr(C)]
struct HostNet {
    mac: [u8; 8],
    ip4: [u8; 4],
    ip6: [[u8; 16]; 4],
    count: u32,
    index: u32,
    connected: bool,
}
#[no_mangle]
unsafe extern "C" fn lh_rs_net(out: *mut HostNet) -> bool {
    let mut ip = [0; 16];
    ip[0] = 0xfe;
    ip[1] = 0x80;
    ip[15] = 1;
    *out = HostNet {
        mac: [2, 0, 0, 0, 0, 1, 0, 0],
        ip4: [192, 0, 2, 1],
        ip6: [ip, [0; 16], [0; 16], [0; 16]],
        count: 1,
        index: 1,
        connected: true,
    };
    true
}
#[no_mangle]
unsafe extern "C" fn lh_rs_wifi(ssid: *mut u8, password: *mut u8) -> u32 {
    core::ptr::copy_nonoverlapping(b"test".as_ptr(), ssid, 4);
    core::ptr::copy_nonoverlapping(b"test-only".as_ptr(), password, 9);
    4 | (9 << 16)
}
#[no_mangle]
extern "C" fn lh_rs_socket(port: u16, _: *const u8, _: u32) -> i32 {
    if port == 5353 { 2 } else { 1 }
}
static UDP_TIMEOUTS: AtomicU32 = AtomicU32::new(0);
#[no_mangle]
extern "C" fn lh_rs_send_timeout() { UDP_TIMEOUTS.fetch_add(1, Ordering::SeqCst); }
static SOCKET_CLOSES: AtomicU32 = AtomicU32::new(0);
#[no_mangle]
extern "C" fn lh_rs_close(_: i32) { SOCKET_CLOSES.fetch_add(1, Ordering::SeqCst); }
static MDNS_INBOX: std::sync::Mutex<std::collections::VecDeque<std::vec::Vec<u8>>> = std::sync::Mutex::new(std::collections::VecDeque::new());
static MDNS_RECEIVED: AtomicU32 = AtomicU32::new(0);
static UDP_OUTPUT: std::sync::Mutex<std::vec::Vec<(i32, std::vec::Vec<u8>)>> = std::sync::Mutex::new(std::vec::Vec::new());
#[no_mangle]
extern "C" fn lh_rs_peek(fd: i32) -> i32 {
    if fd == 2 && !MDNS_INBOX.lock().unwrap().is_empty() { 1 } else { 0 }
}
#[repr(C)]
struct HostWireAddr { ip: [u8; 16], port: u16, scope: u32 }
#[no_mangle]
unsafe extern "C" fn lh_rs_recv(fd: i32, out: *mut u8, size: usize, addr: *mut u8) -> i32 {
    if fd != 2 { return -1; }
    let Some(packet) = MDNS_INBOX.lock().unwrap().pop_front() else { return -1; };
    assert!(packet.len() <= size);
    core::ptr::copy_nonoverlapping(packet.as_ptr(), out, packet.len());
    let mut ip = [0; 16]; ip[15] = 1;
    core::ptr::write(addr.cast::<HostWireAddr>(), HostWireAddr { ip, port: 5353, scope: 1 });
    MDNS_RECEIVED.fetch_add(1, Ordering::SeqCst);
    packet.len() as i32
}
struct SendPlan {
    values: [i32; 4], len: usize, pos: usize, fallback: i32, calls: u32,
}
static SEND_PLAN: std::sync::Mutex<SendPlan> = std::sync::Mutex::new(SendPlan {
    values: [0; 4], len: 0, pos: 0, fallback: i32::MAX, calls: 0,
});
fn send_plan(values: &[i32], fallback: i32) {
    let mut plan = SEND_PLAN.lock().unwrap();
    *plan = SendPlan { values: [0; 4], len: values.len(), pos: 0, fallback, calls: 0 };
    plan.values[..values.len()].copy_from_slice(values);
}
#[no_mangle]
extern "C" fn lh_rs_send(fd: i32, data: *const u8, size: usize, _: *const u8) -> i32 {
    if fd == 2 {
        UDP_OUTPUT.lock().unwrap().push((fd, unsafe { core::slice::from_raw_parts(data,size) }.to_vec()));
    }
    let mut plan = SEND_PLAN.lock().unwrap();
    plan.calls += 1;
    let result = if plan.pos < plan.len {
        let result = plan.values[plan.pos]; plan.pos += 1; result
    } else { plan.fallback };
    if result == i32::MAX { size as i32 } else { result }
}
#[no_mangle]
extern "C" fn lh_rs_multicast(_: i32, _: bool, _: *const u8, _: u32) -> bool {
    true
}
#[no_mangle]
unsafe extern "C" fn lh_rs_hostname(out: *mut u8, size: usize) {
    let text = b"lighthouse-test\0";
    assert!(size >= text.len());
    core::ptr::copy_nonoverlapping(text.as_ptr(), out, text.len());
}
fn poll_runtime() {
    CLOCK_US.fetch_add(100000, Ordering::SeqCst);
    assert!(unsafe { lh_rs_pump() } >= 0);
}


fn udp_backpressure_checks() {
    use core::net::{Ipv4Addr, SocketAddr};
    use rs_matter::stack::net::{NetSocket, NetStack};
    use rs_matter::transport::network::{Address, NetworkSend};
    let mut cx = Context::from_waker(Waker::noop());
    let mut bind = Box::pin(platform::Net.bind(SocketAddr::from((Ipv4Addr::UNSPECIFIED, 0)), Ipv4Addr::LOCALHOST, 1));
    let Poll::Ready(Ok(mut socket)) = bind.as_mut().poll(&mut cx) else { panic!("mock bind"); };
    let (mut sender, _, _) = socket.split();
    let address = Address::Udp(SocketAddr::from((Ipv4Addr::LOCALHOST, 5540)));
    let payload = [1, 2, 3];
    send_plan(&[-1, -1, 3], i32::MAX);
    let mut send = Box::pin(sender.send_to(&payload, address));
    assert!(send.as_mut().poll(&mut cx).is_pending());
    CLOCK_US.fetch_add(10000, Ordering::SeqCst);
    assert!(send.as_mut().poll(&mut cx).is_pending());
    CLOCK_US.fetch_add(10000, Ordering::SeqCst);
    assert!(matches!(send.as_mut().poll(&mut cx), Poll::Ready(Ok(()))));
    assert_eq!(SEND_PLAN.lock().unwrap().calls, 3);
    drop(send);
    for result in [-2, 1] { // Permanent errors and short datagrams never ACK.
        send_plan(&[result], i32::MAX);
        let mut send = Box::pin(sender.send_to(&payload, address));
        assert!(matches!(send.as_mut().poll(&mut cx), Poll::Ready(Err(_))));
        assert_eq!(SEND_PLAN.lock().unwrap().calls, 1);
    }
    send_plan(&[], -1);
    let mut send = Box::pin(sender.send_to(&payload, address));
    let mut timed_out = false;
    for _ in 0..20 {
        if let Poll::Ready(result) = send.as_mut().poll(&mut cx) {
            assert!(result.is_err()); timed_out = true; break;
        }
        CLOCK_US.fetch_add(100000, Ordering::SeqCst);
    }
    assert!(timed_out && SEND_PLAN.lock().unwrap().calls <= 17);
    assert_eq!(UDP_TIMEOUTS.load(Ordering::SeqCst), 1);
    drop(send);
    send_plan(&[-1], i32::MAX);
    let closes = SOCKET_CLOSES.load(Ordering::SeqCst);
    let mut send = Box::pin(sender.send_to(&payload, address));
    assert!(send.as_mut().poll(&mut cx).is_pending());
    drop(send); // Cancellation must leave the socket usable.
    assert_eq!(SOCKET_CLOSES.load(Ordering::SeqCst), closes);
    let mut send = Box::pin(sender.send_to(&payload, address));
    assert!(matches!(send.as_mut().poll(&mut cx), Poll::Ready(Ok(()))));
    drop(send);
    send_plan(&[], i32::MAX);
}


fn mdns_receive_checks() {
    // Leave the Matter UDP socket idle, then deliver a real DNS PTR query to
    // mDNS. Both consumers share the stack RX buffer; an idle socket must not
    // take it and block the ready mDNS socket indefinitely.
    let query = b"\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x08_matterc\x04_udp\x05local\x00\x00\x0c\x00\x01";
    let before = UDP_OUTPUT.lock().unwrap().len();
    MDNS_INBOX.lock().unwrap().push_back(query.to_vec());
    for _ in 0..100 { poll_runtime(); }
    assert_eq!(MDNS_RECEIVED.load(Ordering::SeqCst), 1,
               "idle Matter socket starved a ready mDNS socket of the shared RX buffer");
    let output = UDP_OUTPUT.lock().unwrap();
    assert!(output[before..].iter().any(|(_,p)| p.len()>12 && p[2]&0x80!=0 && p.windows(8).any(|s| s==b"_matterc") && p.windows(4).any(|s| s==b"CM=1")),
            "PTR query did not receive a Basic Commissioning DNS response");
}
