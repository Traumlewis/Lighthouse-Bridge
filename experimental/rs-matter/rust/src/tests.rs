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
extern "C" fn lh_rs_socket(_: u16, _: *const u8, _: u32) -> i32 {
    1
}
#[no_mangle]
extern "C" fn lh_rs_close(_: i32) {}
#[no_mangle]
extern "C" fn lh_rs_recv(_: i32, _: *mut u8, _: usize, _: *mut u8) -> i32 {
    -1
}
#[no_mangle]
extern "C" fn lh_rs_send(_: i32, _: *const u8, size: usize, _: *const u8) -> i32 {
    size as i32
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
