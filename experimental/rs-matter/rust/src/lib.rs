//! rs-matter owns protocol state; the C++ BLE worker owns physical power.
#![no_std]
#![feature(allocator_api)]
extern crate alloc;
mod model;
mod platform;
use alloc::boxed::Box;
use core::{
    future::Future,
    pin::Pin,
    task::{Context, Poll, Waker},
};
use rs_matter::crypto::{default_crypto, Crypto};
use rs_matter::dm::devices::test::{DAC_PRIVKEY, TEST_DEV_ATT, TEST_DEV_DET};
use rs_matter::error::Error;
use rs_matter::pairing::{BasicCommData, DevCommData, Spake2pVerifierPassword};
use rs_matter::{
    dm::networks::wireless::WifiNetworks, stack::MatterStack, utils::init::InitMaybeUninit, Matter,
};

use rand::Rng;

// The 32-bit operational future is ~12 KiB. Host pointers need more space.
// platform::Wifi::run checks the exact allocated future at compile time.
const BUMP: usize = if core::mem::size_of::<usize>() == 4 {
    16384
} else {
    24576
};
type Stack = MatterStack<'static, BUMP, WifiNetworks<1>>;
static mut STACK: *const Stack = core::ptr::null();
static mut TASK: Option<Pin<Box<dyn Future<Output = Result<(), Error>>>>> = None;
static mut READY: bool = false;
static mut OPEN_REQUEST: bool = false;
static mut CODE_WINDOW: u64 = 0;
static mut CODE: [u8; 12] = [0; 12];
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Station {
    pub endpoint: u16,
    pub name: [u8; 41],
    pub unique: [u8; 40],
}
#[repr(C)]
pub struct Config {
    pub passcode: u32,
    pub discriminator: u16,
    pub count: u16,
    pub stations: [Station; 16],
    pub serial: [u8; 33],
}
static mut CONFIG: Option<Config> = None;
pub fn config() -> &'static Config {
    unsafe { (&*core::ptr::addr_of!(CONFIG)).as_ref().unwrap() }
}
fn stack() -> &'static Stack {
    unsafe { &*STACK }
}
pub fn matter() -> &'static Matter<'static> {
    stack().matter()
}

#[no_mangle]
pub unsafe extern "C" fn lh_rs_begin(cfg: *const Config) -> bool {
    if cfg.is_null() || !STACK.is_null() || !valid_config(&*cfg) {
        return false;
    }
    CONFIG = Some(core::ptr::read(cfg));
    let Ok(task) = Box::try_new(run()) else {
        return false;
    };
    TASK = Some(Box::into_pin(task));
    true
}
#[no_mangle]
pub unsafe extern "C" fn lh_rs_pump() -> i32 {
    let Some(task) = (&mut *core::ptr::addr_of_mut!(TASK)).as_mut() else {
        return -1;
    };
    let mut cx = Context::from_waker(Waker::noop());
    match task.as_mut().poll(&mut cx) {
        Poll::Pending => {
            if READY {
                1
            } else {
                0
            }
        }
        Poll::Ready(_) => {
            READY = false;
            TASK = None;
            -1
        }
    }
}
#[no_mangle]
pub unsafe extern "C" fn lh_rs_open() -> bool {
    if !READY {
        return false;
    }
    OPEN_REQUEST = true;
    true
}
#[no_mangle]
pub unsafe extern "C" fn lh_rs_info(
    code: *mut u8,
    capacity: usize,
    paired: *mut bool,
    open: *mut bool,
) -> bool {
    if !READY || capacity < 12 || code.is_null() || paired.is_null() || open.is_null() {
        return false;
    }
    *paired = matter().has_fabrics();
    *open = matter().comm_window_state().is_open();
    core::ptr::copy_nonoverlapping(core::ptr::addr_of!(CODE).cast::<u8>(), code, 12);
    if !*open || CODE_WINDOW == 0 || OPEN_REQUEST || current_window() != CODE_WINDOW {
        *code = 0;
    }
    true
}

#[no_mangle]
pub unsafe extern "C" fn lh_rs_pending() -> bool {
    OPEN_REQUEST
}
fn current_window() -> u64 {
    matter().with_state(
        |state| match state.pase.comm_window().map(|w| w.mdns_service()) {
            Some(rs_matter::transport::network::MatterLocalService::Commissionable {
                id,
                enhanced: false,
                ..
            }) => id,
            _ => 0,
        },
    )
}

async fn run() -> Result<(), Error> {
    let cfg = config();
    let details = Box::leak(
        Box::try_new(rs_matter::basic_info::BasicInfoConfig {
            product_name: "Lighthouse-Bridge Matter Test",
            device_name: "Lighthouse-Bridge Matter Test",
            vendor_name: "Lighthouse Bridge",
            serial_no: platform::text(&cfg.serial),
            unique_id: platform::text(&cfg.serial),
            sw_ver: 0x000102,
            sw_ver_str: "0.1.2-rs-matter",
            ..TEST_DEV_DET
        })
        .map_err(|_| Error::from(rs_matter::error::ErrorCode::NoSpace))?,
    );
    let comm = BasicCommData {
        password: Spake2pVerifierPassword::from(cfg.passcode.to_le_bytes()),
        discriminator: cfg.discriminator,
    };
    let initial_code = comm.compute_pairing_code();
    unsafe {
        CODE[..11].copy_from_slice(initial_code.as_bytes());
    }
    let memory = Box::<Stack>::try_new_uninit()
        .map_err(|_| Error::from(rs_matter::error::ErrorCode::NoSpace))?;
    let stack = Box::leak(memory).init_with(Stack::init(
        details,
        DevCommData::Passcode(comm),
        &TEST_DEV_ATT,
    ));
    unsafe {
        STACK = stack as *const _;
    }
    let crypto = default_crypto(platform::Random, DAC_PRIVKEY);
    let mut random = crypto.rand()?;
    let model = model::Model::new(&mut random);
    let mut store = platform::Store;
    stack.startup(&crypto, &mut store).await?;
    // Pairing is opened explicitly only after C++ has a usable BLE worker.
    stack.matter().close_comm_window(&())?;
    platform::seed_wifi(stack, &mut store).await?;
    let kv = stack.kv(store);
    unsafe {
        CODE_WINDOW = current_window();
        READY = true;
    }
    stack
        .run_wifi_oper(
            platform::Wifi,
            &crypto,
            model::data_model(stack.matter(), random, &model),
            kv,
            (),
        )
        .await
}
pub fn window_control(ctx: &impl rs_matter::dm::HandlerContext) -> Result<(), Error> {
    unsafe {
        if OPEN_REQUEST {
            OPEN_REQUEST = false;
            let crypto = ctx.crypto();
            let mut rng = crypto.rand()?;
            let (comm, code) = loop {
                let passcode = 1 + rng.next_u32() % 99999998;
                if passcode % 11111111 == 0
                    || passcode == 12345678
                    || passcode == 87654321
                    || passcode == config().passcode
                {
                    continue;
                }
                let comm = BasicCommData {
                    password: Spake2pVerifierPassword::from(passcode.to_le_bytes()),
                    discriminator: (rng.next_u32() & 0xfff) as u16,
                };
                let code = comm.compute_pairing_code();
                if code.as_bytes() != &CODE[..11] {
                    break (comm, code);
                }
            };
            matter().close_comm_window(ctx)?;
            let mut salt = rs_matter::pairing::SPAKE2P_VERIFIER_SALT_ZEROED;
            rng.fill_bytes(salt.access_mut());
            matter().with_state(|state| {
                state.pase.open_basic_comm_window(
                    rng.next_u64(),
                    &DevCommData::Passcode(comm),
                    salt.access(),
                    900,
                    None,
                    || matter().transport().notify_comm_window_changed(),
                    |e, c| ctx.notify_cluster_changed(e, c),
                )
            })?;
            CODE[..11].copy_from_slice(code.as_bytes());
            CODE_WINDOW = current_window();
        }
    }
    Ok(())
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_: &core::panic::PanicInfo<'_>) -> ! {
    unsafe { platform::lh_rs_fault() };
    loop {
        core::hint::spin_loop();
    }
}

#[no_mangle]
pub extern "C" fn lh_rs_stack_bytes() -> usize {
    core::mem::size_of::<Stack>()
}
#[no_mangle]
pub extern "C" fn lh_rs_arena_bytes() -> usize {
    BUMP
}
#[no_mangle]
pub extern "C" fn lh_rs_arena_used() -> usize {
    platform::WIFI_ARENA_USED.load(core::sync::atomic::Ordering::Relaxed)
}
#[no_mangle]
pub extern "C" fn lh_rs_task_bytes() -> usize {
    core::mem::size_of_val(&run())
}

fn valid_config(cfg: &Config) -> bool {
    if cfg.count > 16
        || cfg.discriminator > 4095
        || cfg.passcode == 0
        || cfg.passcode > 99999998
        || cfg.passcode % 11111111 == 0
        || cfg.passcode == 12345678
        || cfg.passcode == 87654321
    {
        return false;
    }
    let stations = &cfg.stations[..cfg.count as usize];
    for (i, station) in stations.iter().enumerate() {
        if station.endpoint == 0
            || station.endpoint == 2
            || station.endpoint == 0xffff
            || stations[..i].iter().any(|s| s.endpoint == station.endpoint)
        {
            return false;
        }
        if platform::text(&station.name).is_empty() || platform::text(&station.unique).is_empty() {
            return false;
        }
    }
    !platform::text(&cfg.serial).is_empty()
}

#[cfg(test)]
mod tests;
