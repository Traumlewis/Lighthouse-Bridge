use core::{
    alloc::{GlobalAlloc, Layout},
    net::{IpAddr, Ipv4Addr, Ipv6Addr, SocketAddr, SocketAddrV6},
    task::Waker,
};
use rs_matter::dm::{
    clusters::{
        gen_diag::{InterfaceTypeEnum, NetifDiag, NetifInfo},
        net_comm::{NetCtl, NetCtlError, NetworkScanInfo, NetworkType, WirelessCreds},
        wifi_diag::{WifiDiag, WirelessDiag},
    },
    networks::NetChangeNotif,
};
use rs_matter::stack::{
    net::{NetSocket, NetStack},
    wireless::{WifiDriver, WifiTask},
};
use rs_matter::transport::network::{Address, NetworkMulticast, NetworkReceive, NetworkSend};
use rs_matter::utils::timers::Timer;
use rs_matter::{
    error::{Error, ErrorCode},
    persist::KvBlobStore,
};
extern "C" {
    fn lh_rs_alloc(size: usize, align: usize) -> *mut u8;
    fn free(p: *mut u8);
    fn lh_rs_time() -> u64;
    fn lh_rs_random(data: *mut u8, size: usize);
    fn lh_rs_lock() -> u32;
    fn lh_rs_unlock(state: u32);
    pub fn lh_rs_fault();
    fn lh_rs_store(op: u32, key: u16, data: *mut u8, size: usize) -> i32;
    fn lh_rs_net(info: *mut NetInfo) -> bool;
    fn lh_rs_socket(port: u16, ip4: *const u8, interface: u32) -> i32;
    fn lh_rs_close(fd: i32);
    fn lh_rs_recv(fd: i32, buf: *mut u8, size: usize, addr: *mut WireAddr) -> i32;
    fn lh_rs_send(fd: i32, buf: *const u8, size: usize, addr: *const WireAddr) -> i32;
    fn lh_rs_multicast(fd: i32, join: bool, group: *const u8, interface: u32) -> bool;
}
#[repr(C)]
#[derive(Default)]
struct NetInfo {
    mac: [u8; 8],
    ip4: [u8; 4],
    ip6: [[u8; 16]; 4],
    ipv6_count: u32,
    interface: u32,
    connected: bool,
}
#[repr(C)]
#[derive(Default)]
struct WireAddr {
    ip: [u8; 16],
    port: u16,
    scope: u32,
}
impl WireAddr {
    fn from(addr: SocketAddr) -> Self {
        match addr {
            SocketAddr::V6(a) => Self {
                ip: a.ip().octets(),
                port: a.port(),
                scope: a.scope_id(),
            },
            SocketAddr::V4(a) => Self {
                ip: a.ip().to_ipv6_mapped().octets(),
                port: a.port(),
                scope: 0,
            },
        }
    }
    fn into_addr(self) -> SocketAddr {
        SocketAddr::V6(SocketAddrV6::new(
            Ipv6Addr::from(self.ip),
            self.port,
            0,
            self.scope,
        ))
    }
}
struct Alloc;
unsafe impl GlobalAlloc for Alloc {
    unsafe fn alloc(&self, l: Layout) -> *mut u8 {
        lh_rs_alloc(l.size(), l.align())
    }
    unsafe fn dealloc(&self, p: *mut u8, _: Layout) {
        free(p)
    }
    unsafe fn realloc(&self, p: *mut u8, l: Layout, n: usize) -> *mut u8 {
        let q = lh_rs_alloc(n, l.align());
        if !q.is_null() {
            core::ptr::copy_nonoverlapping(p, q, l.size().min(n));
            free(p);
        }
        q
    }
}
#[cfg(not(test))]
#[global_allocator]
static ALLOC: Alloc = Alloc;
struct Critical;
critical_section::set_impl!(Critical);
unsafe impl critical_section::Impl for Critical {
    unsafe fn acquire() -> u32 {
        lh_rs_lock()
    }
    unsafe fn release(s: u32) {
        lh_rs_unlock(s)
    }
}
struct Clock;
embassy_time_driver::time_driver_impl!(static CLOCK:Clock=Clock);
impl embassy_time_driver::Driver for Clock {
    fn now(&self) -> u64 {
        unsafe { lh_rs_time() }
    }
    fn schedule_wake(&self, _: u64, _: &Waker) {} // The single foreground executor polls every loop.
}
#[derive(Clone, Copy)]
pub struct Random;
impl rand::TryRng for Random {
    type Error = core::convert::Infallible;
    fn try_next_u32(&mut self) -> Result<u32, Self::Error> {
        let mut b = [0; 4];
        self.try_fill_bytes(&mut b)?;
        Ok(u32::from_le_bytes(b))
    }
    fn try_next_u64(&mut self) -> Result<u64, Self::Error> {
        let mut b = [0; 8];
        self.try_fill_bytes(&mut b)?;
        Ok(u64::from_le_bytes(b))
    }
    fn try_fill_bytes(&mut self, b: &mut [u8]) -> Result<(), Self::Error> {
        unsafe { lh_rs_random(b.as_mut_ptr(), b.len()) };
        Ok(())
    }
}
impl rand::TryCryptoRng for Random {}
pub fn text(data: &[u8]) -> &str {
    core::str::from_utf8(data.split(|c| *c == 0).next().unwrap_or(&[])).unwrap_or("")
}
pub struct Store;
impl KvBlobStore for Store {
    fn load<'a>(&mut self, key: u16, buf: &'a mut [u8]) -> Result<Option<&'a [u8]>, Error> {
        let n = unsafe { lh_rs_store(0, key, buf.as_mut_ptr(), buf.len()) };
        if n == -1 {
            Ok(None)
        } else if n < 0 || n as usize > buf.len() {
            Err(ErrorCode::StdIoError.into())
        } else {
            Ok(Some(&buf[..n as usize]))
        }
    }
    fn store(&mut self, key: u16, data: &[u8], _: &mut [u8]) -> Result<(), Error> {
        if unsafe { lh_rs_store(1, key, data.as_ptr() as *mut _, data.len()) } >= 0 {
            Ok(())
        } else {
            Err(ErrorCode::StdIoError.into())
        }
    }
    fn remove(&mut self, key: u16, _: &mut [u8]) -> Result<(), Error> {
        if unsafe { lh_rs_store(2, key, core::ptr::null_mut(), 0) } >= 0 {
            Ok(())
        } else {
            Err(ErrorCode::StdIoError.into())
        }
    }
}
#[derive(Clone, Copy)]
pub struct Net;
impl NetChangeNotif for Net {
    async fn wait_changed(&self) {
        Timer::after_secs(2).await
    }
}
impl NetifDiag for Net {
    fn netifs(&self, f: &mut dyn FnMut(&NetifInfo) -> Result<(), Error>) -> Result<(), Error> {
        let mut n = NetInfo::default();
        if !unsafe { lh_rs_net(&mut n) } {
            return Ok(());
        }
        let v4 = [Ipv4Addr::from(n.ip4)];
        let v6 = n.ip6.map(Ipv6Addr::from);
        f(&NetifInfo {
            name: "WiFi",
            operational: n.connected,
            offprem_svc_reachable_ipv4: None,
            offprem_svc_reachable_ipv6: None,
            hw_addr: &n.mac,
            ipv4_addrs: &v4,
            ipv6_addrs: &v6[..(n.ipv6_count as usize).min(4)],
            netif_type: InterfaceTypeEnum::WiFi,
            netif_index: n.interface,
        })
    }
}
impl WirelessDiag for Net {
    fn connected(&self) -> Result<bool, Error> {
        let mut n = NetInfo::default();
        unsafe { lh_rs_net(&mut n) };
        Ok(n.connected)
    }
}
impl WifiDiag for Net {}
impl NetCtl for Net {
    fn net_type(&self) -> NetworkType {
        NetworkType::Wifi
    }
    async fn scan<F>(&self, _: Option<&[u8]>, _: F) -> Result<(), NetCtlError>
    where
        F: FnMut(&NetworkScanInfo) -> Result<(), Error>,
    {
        Err(Error::from(ErrorCode::InvalidAction).into())
    }
    async fn connect(&self, creds: &WirelessCreds<'_>) -> Result<(), NetCtlError> {
        let saved = saved_wifi()?;
        match creds {
            WirelessCreds::Wifi {
                ssid,
                pass: password,
            } if *ssid == &*saved.ssid && *password == &*saved.password => {
                if self.connected()? {
                    Ok(())
                } else {
                    Err(NetCtlError::OtherConnectionFailure)
                }
            }
            _ => Err(NetCtlError::NetworkNotFound),
        }
    }
}
pub struct Wifi;
impl WifiDriver for Wifi {
    type NetCtl<'a> = Net;
    async fn run<T: WifiTask>(&mut self, mut task: T) -> Result<(), Error> {
        task.run(Net, Net, Net, LocalMdns).await
    }
}
pub struct Socket(i32, u32);
impl Drop for Socket {
    fn drop(&mut self) {
        unsafe { lh_rs_close(self.0) }
    }
}
impl NetStack for Net {
    type Socket<'a> = Socket;
    async fn bind(
        &self,
        addr: SocketAddr,
        ipv4: Ipv4Addr,
        interface: u32,
    ) -> Result<Socket, Error> {
        let fd = unsafe { lh_rs_socket(addr.port(), ipv4.octets().as_ptr(), interface) };
        if fd < 0 {
            Err(ErrorCode::StdIoError.into())
        } else {
            Ok(Socket(fd, interface))
        }
    }
}
#[derive(Clone, Copy)]
pub struct Half(i32, u32);
impl NetSocket for Socket {
    type Send<'a> = Half;
    type Receive<'a> = Half;
    type Multicast<'a> = Half;
    fn split(&mut self) -> (Half, Half, Half) {
        (
            Half(self.0, self.1),
            Half(self.0, self.1),
            Half(self.0, self.1),
        )
    }
}
impl NetworkSend for Half {
    async fn send_to(&mut self, data: &[u8], addr: Address) -> Result<(), Error> {
        let Address::Udp(addr) = addr else {
            return Err(ErrorCode::InvalidAction.into());
        };
        let a = WireAddr::from(addr);
        let n = unsafe { lh_rs_send(self.0, data.as_ptr(), data.len(), &a) };
        if n == data.len() as i32 {
            Ok(())
        } else {
            Err(ErrorCode::StdIoError.into())
        }
    }
}
impl NetworkReceive for Half {
    async fn wait_available(&mut self) -> Result<(), Error> {
        Timer::after_millis(5).await;
        Ok(())
    }
    async fn recv_from(&mut self, data: &mut [u8]) -> Result<(usize, Address), Error> {
        loop {
            let mut a = WireAddr::default();
            let n = unsafe { lh_rs_recv(self.0, data.as_mut_ptr(), data.len(), &mut a) };
            if n >= 0 {
                return Ok((n as usize, Address::Udp(a.into_addr())));
            }
            if n != -1 {
                return Err(ErrorCode::StdIoError.into());
            }
            Timer::after_millis(5).await;
        }
    }
}
impl NetworkMulticast for Half {
    async fn join(&mut self, addr: IpAddr) -> Result<(), Error> {
        let group = match addr {
            IpAddr::V6(a) => a,
            IpAddr::V4(a) => a.to_ipv6_mapped(),
        };
        if unsafe { lh_rs_multicast(self.0, true, group.octets().as_ptr(), self.1) } {
            Ok(())
        } else {
            Err(ErrorCode::StdIoError.into())
        }
    }
    async fn leave(&mut self, addr: IpAddr) -> Result<(), Error> {
        let group = match addr {
            IpAddr::V6(a) => a,
            IpAddr::V4(a) => a.to_ipv6_mapped(),
        };
        if unsafe { lh_rs_multicast(self.0, false, group.octets().as_ptr(), self.1) } {
            Ok(())
        } else {
            Err(ErrorCode::StdIoError.into())
        }
    }
}

/// One DNS-SD owner for Matter; retain the dashboard's saved hostname.
pub struct LocalMdns;
extern "C" {
    fn lh_rs_hostname(out: *mut u8, size: usize);
}
impl rs_matter::stack::mdns::Mdns for LocalMdns {
    async fn run<C, S>(
        &mut self,
        matter: &rs_matter::Matter<'_>,
        crypto: &C,
        net: &S,
        _: &[u8],
        ipv4: Ipv4Addr,
        ipv6: &[Ipv6Addr],
        interface: u32,
    ) -> Result<(), Error>
    where
        C: rs_matter::crypto::Crypto,
        S: NetStack,
    {
        use rs_matter::transport::network::mdns::{
            builtin::{BuiltinMdns, Host},
            MDNS_IPV4_BROADCAST_ADDR, MDNS_IPV6_BROADCAST_ADDR, MDNS_PORT,
        };
        loop {
            let mut socket = net
                .bind(
                    SocketAddr::V6(SocketAddrV6::new(Ipv6Addr::UNSPECIFIED, MDNS_PORT, 0, 0)),
                    ipv4,
                    interface,
                )
                .await?;
            let (send, recv, mut multicast) = socket.split();
            multicast.join(IpAddr::V4(MDNS_IPV4_BROADCAST_ADDR)).await?;
            multicast.join(IpAddr::V6(MDNS_IPV6_BROADCAST_ADDR)).await?;
            let mut hostname = [0; 33];
            unsafe { lh_rs_hostname(hostname.as_mut_ptr(), hostname.len()) };
            let host = Host {
                hostname: text(&hostname),
                ip: ipv4,
                ipv6,
            };
            let mut mdns = BuiltinMdns::new();
            let changed = async {
                loop {
                    Timer::after_secs(2).await;
                    let mut next = [0; 33];
                    unsafe { lh_rs_hostname(next.as_mut_ptr(), next.len()) };
                    if next != hostname {
                        break;
                    }
                }
            };
            match rs_matter::utils::futures::select(
                mdns.run(
                    send,
                    recv,
                    &host,
                    Some(ipv4),
                    Some(interface),
                    matter,
                    crypto,
                ),
                changed,
            )
            .await
            {
                rs_matter::utils::futures::Either::First(result) => return result,
                rs_matter::utils::futures::Either::Second(()) => {}
            }
        }
    }
}

extern "C" {
    fn lh_rs_wifi(ssid: *mut u8, password: *mut u8) -> u32;
}
pub fn saved_wifi() -> Result<rs_matter::dm::networks::wireless::Wifi, Error> {
    let mut ssid = [0; 32];
    let mut password = [0; 64];
    let lengths = unsafe { lh_rs_wifi(ssid.as_mut_ptr(), password.as_mut_ptr()) };
    let sl = (lengths & 0xffff) as usize;
    let pl = (lengths >> 16) as usize;
    if sl == 0 || sl > 32 || pl > 64 {
        return Err(ErrorCode::InvalidData.into());
    }
    let mut wifi = rs_matter::dm::networks::wireless::Wifi::new();
    wifi.ssid
        .vec
        .extend_from_slice(&ssid[..sl])
        .map_err(|_| ErrorCode::NoSpace)?;
    wifi.password
        .vec
        .extend_from_slice(&password[..pl])
        .map_err(|_| ErrorCode::NoSpace)?;
    Ok(wifi)
}
pub async fn seed_wifi(stack: &super::Stack, store: &mut Store) -> Result<(), Error> {
    let wifi = saved_wifi()?;
    let mut bytes = [0; 256];
    let n = stack
        .networks(|networks| {
            networks.reset();
            let ssid = wifi.ssid.clone();
            networks
                .add_or_update(
                    &ssid,
                    unsafe {
                        rs_matter::utils::init::init_from_closure(
                            |slot: *mut rs_matter::dm::networks::wireless::Wifi| {
                                slot.write(wifi);
                                Ok(())
                            },
                        )
                    },
                    |_| Ok(()),
                )
                .map_err(|_| ErrorCode::InvalidData)?;
            networks.set_managed(true);
            networks.store(&mut bytes)
        })
        .await?;
    store.store(rs_matter::persist::NETWORKS_KEY, &bytes[..n], &mut [])
}
