//! Bounded protocol diagnostics. Never forward SDK log text or packet contents.
use core::{
    cell::RefCell,
    fmt::{self, Write},
};
use critical_section::Mutex;
use log::{Level, LevelFilter, Log, Metadata, Record};
use rs_matter::transport::session::{SessionMode, SessionState};

extern "C" {
    fn lh_rs_trace(
        module: u32,
        event: *const u8,
        event_len: usize,
        error: *const u8,
        error_len: usize,
    );
    fn lh_rs_trace_state(fabrics: u32, pase: u32, case: u32, failsafe: bool, open: bool);
}

const EVENTS: &[(&str, &str)] = &[
    ("Got Attestation Request", "attestation_request"),
    ("Got Cert Chain Request", "certificate_request"),
    ("Got CSR Request", "csr_request"),
    ("Got Add Trusted Root Cert Request", "add_root_request"),
    ("Got Add NOC Request", "add_noc_request"),
    (
        "Got Commissioning Complete Request",
        "commissioning_complete_request",
    ),
    (
        "Commissioning complete, fabric and network settings persisted",
        "commissioning_complete_persisted",
    ),
    ("Got Remove Fabric Request", "remove_fabric_request"),
    ("Removed operational fabric", "fabric_removed"),
    ("Fail-Safe timeout expired", "failsafe_expired"),
    ("No fabric and node ID match", "case_destination_mismatch"),
    ("Sigma1 has mismatched", "case_sigma1_invalid"),
    ("Sigma3 outer TLV parse failed", "case_sigma3_tlv_error"),
    (
        "Sigma3 encrypted field parse failed",
        "case_sigma3_field_error",
    ),
    ("Sigma3 AEAD decrypt failed", "case_sigma3_decrypt_error"),
    (
        "Sigma3 decrypted TLV parse failed",
        "case_sigma3_plaintext_error",
    ),
    ("Certificate Chain doesn't match", "case_certificate_error"),
    ("Sigma3 Signature doesn't match", "case_signature_error"),
    ("Abandoned because of error", "exchange_abandoned"),
    ("Error reading attribute", "attribute_read_error"),
    ("Error processing attribute read", "attribute_read_status"),
    ("Error writing attribute", "attribute_write_error"),
    ("Error processing attribute write", "attribute_write_status"),
    ("Error invoking command", "command_error"),
    ("Error processing command", "command_status"),
    (
        "PASE Basic Commissioning Window opened",
        "basic_window_opened",
    ),
    ("PASE Commissioning Window closed", "window_closed"),
];
// Only these static tokens may cross FFI. Unknown content is never printed.
const ERRORS: &[&str] = &[
    "none",
    "AlreadyExists",
    "AttributeNotFound",
    "BufferTooSmall",
    "ClusterNotFound",
    "CommandNotFound",
    "EndpointNotFound",
    "InvalidAction",
    "InvalidCommand",
    "FailSafeRequired",
    "NeedsTimedInteraction",
    "ConstraintError",
    "InvalidDataType",
    "UnsupportedAccess",
    "ResourceExhausted",
    "Busy",
    "NoExchange",
    "NoFabricId",
    "NoHandler",
    "NoMemory",
    "NoSession",
    "NoSharedTrustRoots",
    "NoSpace",
    "NoSpaceExchanges",
    "NoSpaceSessions",
    "TxTimeout",
    "RxTimeout",
    "NotFound",
    "PacketPoolExhaust",
    "StdIoError",
    "Invalid",
    "InvalidAAD",
    "InvalidData",
    "InvalidTransportType",
    "InvalidOpcode",
    "InvalidProto",
    "InvalidPeerAddr",
    "InvalidAuthKey",
    "InvalidSignature",
    "InvalidState",
    "InvalidTime",
    "InvalidArgument",
    "RwLock",
    "TLVNotFound",
    "TLVTypeMismatch",
    "TruncatedPacket",
    "Utf8Fail",
    "GennCommInvalidAuthentication",
    "NocInvalidNoc",
    "NocInvalidPublicKey",
    "NocMissingCsr",
    "NocFabricTableFull",
    "NocFabricConflict",
    "NocLabelConflict",
    "NocInvalidFabricIndex",
    "NocInvalidAdminSubject",
    "Failure",
    "InvalidSubscription",
    "UnsupportedEndpoint",
    "UnsupportedCommand",
    "UnsupportedAttribute",
    "UnsupportedWrite",
    "UnreportableAttribute",
    "UnsupportedRead",
    "DataVersionMismatch",
    "Timeout",
    "UnsupportedNode",
    "AccessRestricted",
    "UnsupportedCluster",
    "UnsupportedEvent",
    "PathsExhausted",
    "TimedRequestMisMatch",
    "InvalidInState",
    "NoCommandResponse",
    "TermsAndConditionsChanged",
    "InvalidParameter",
    "CloseSession",
];

#[derive(Clone, Copy, Debug, PartialEq)]
struct Event {
    module: u8,
    event: u8,
    error: u8,
}
struct Queue {
    entries: [Event; 32],
    head: usize,
    count: usize,
    dropped: u32,
}
static QUEUE: Mutex<RefCell<Queue>> = Mutex::new(RefCell::new(Queue {
    entries: [Event {
        module: 0,
        event: 0,
        error: 0,
    }; 32],
    head: 0,
    count: 0,
    dropped: 0,
}));
struct Logger;
static LOGGER: Logger = Logger;

struct Line {
    data: [u8; 256],
    len: usize,
}
impl Write for Line {
    fn write_str(&mut self, text: &str) -> fmt::Result {
        let count = (self.data.len() - self.len).min(text.len());
        self.data[self.len..self.len + count].copy_from_slice(&text.as_bytes()[..count]);
        self.len += count;
        if count == text.len() {
            Ok(())
        } else {
            Err(fmt::Error)
        }
    }
}
fn module(target: &str) -> u8 {
    if target.starts_with("rs_matter_sc") {
        1
    } else if target.starts_with("rs_matter_im") {
        2
    } else if target.starts_with("rs_matter_respond") {
        3
    } else if target.starts_with("rs_matter_dm") {
        4
    } else if target.starts_with("rs_matter_node") {
        5
    } else if target.starts_with("rs_matter_transport") {
        6
    } else {
        0
    }
}
fn classify(target: &str, level: Level, text: &str) -> Option<Event> {
    let module = module(target);
    if module == 0 || level > Level::Info {
        return None;
    }
    let event = match EVENTS.iter().position(|(needle, _)| text.contains(needle)) {
        Some(index) => index as u8,
        None if level <= Level::Warn => EVENTS.len() as u8,
        _ => return None,
    };
    let error = text
        .split(|c: char| !c.is_ascii_alphanumeric() && c != '_')
        .find_map(|token| {
            ERRORS
                .iter()
                .skip(1)
                .position(|name| *name == token)
                .map(|i| i + 1)
        })
        .unwrap_or(0) as u8;
    if event as usize == EVENTS.len() && level == Level::Warn && error == 0 {
        return None;
    }
    Some(Event {
        module,
        event,
        error,
    })
}
impl Log for Logger {
    fn enabled(&self, meta: &Metadata) -> bool {
        meta.level() <= Level::Info && module(meta.target()) != 0
    }
    fn log(&self, record: &Record) {
        if !self.enabled(record.metadata()) {
            return;
        }
        let mut line = Line {
            data: [0; 256],
            len: 0,
        };
        let _ = line.write_fmt(*record.args());
        // A truncated UTF-8 character need not be inspected or logged.
        let text = core::str::from_utf8(&line.data[..line.len]).unwrap_or("");
        if let Some(event) = classify(record.target(), record.level(), text) {
            critical_section::with(|cs| {
                let mut q = QUEUE.borrow(cs).borrow_mut();
                if q.count == q.entries.len() {
                    q.dropped = q.dropped.saturating_add(1);
                } else {
                    let tail = (q.head + q.count) % q.entries.len();
                    q.entries[tail] = event;
                    q.count += 1;
                }
            });
        }
    }
    fn flush(&self) {}
}
pub fn install() {
    if log::set_logger(&LOGGER).is_ok() {
        log::set_max_level(LevelFilter::Info);
    }
}
// Drain only after the stack poll returns, outside all SDK/critical-section locks.
pub fn drain() {
    loop {
        let event = critical_section::with(|cs| {
            let mut q = QUEUE.borrow(cs).borrow_mut();
            if q.count == 0 {
                return None;
            }
            let event = q.entries[q.head];
            q.head = (q.head + 1) % q.entries.len();
            q.count -= 1;
            Some(event)
        });
        let Some(event) = event else {
            break;
        };
        let name = EVENTS
            .get(event.event as usize)
            .map(|(_, name)| *name)
            .unwrap_or("sdk_warning_or_error");
        let error = ERRORS[event.error as usize];
        unsafe {
            lh_rs_trace(
                event.module as u32,
                name.as_ptr(),
                name.len(),
                error.as_ptr(),
                error.len(),
            );
        }
    }
    let dropped = critical_section::with(|cs| {
        let mut q = QUEUE.borrow(cs).borrow_mut();
        core::mem::take(&mut q.dropped)
    });
    if dropped != 0 {
        let name = "diagnostic_queue_overflow";
        let error = "events_dropped";
        unsafe {
            lh_rs_trace(
                dropped,
                name.as_ptr(),
                name.len(),
                error.as_ptr(),
                error.len(),
            );
        }
    }
}
#[derive(Clone, Copy, PartialEq)]
struct State {
    fabrics: u32,
    pase: u32,
    case: u32,
    failsafe: bool,
    open: bool,
}
static mut PREVIOUS: Option<State> = None;
pub fn state_changed() {
    let next = super::matter().with_state(|s| {
        let mut next = State {
            fabrics: s.fabrics.iter().count() as u32,
            pase: 0,
            case: 0,
            failsafe: s.failsafe.is_armed(),
            open: s.pase.comm_window_state().is_open(),
        };
        for session in s.sessions.iter() {
            if session.state() != SessionState::Active {
                continue;
            }
            match session.get_session_mode() {
                SessionMode::Pase { .. } => next.pase += 1,
                SessionMode::Case { .. } => next.case += 1,
                _ => {}
            }
        }
        next
    });
    // Like the runtime FFI, this is used exclusively by the foreground executor.
    unsafe {
        if PREVIOUS != Some(next) {
            PREVIOUS = Some(next);
            lh_rs_trace_state(next.fabrics, next.pase, next.case, next.failsafe, next.open);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn only_fixed_events_and_error_tokens_leave_the_logger() {
        assert!(EVENTS.iter().all(|(_, label)| label.len() < 64));
        assert!(ERRORS.iter().all(|label| label.len() < 48));
        let secret = "private-key=synthetic-secret IP=192.0.2.99 certificate=[1,2,3]";
        assert!(classify("rs_matter_sc::case", Level::Info, secret).is_none());
        assert!(classify(
            "rs_matter_sc::case",
            Level::Debug,
            "Sigma3 AEAD decrypt failed"
        )
        .is_none());
        let e = classify(
            "rs_matter_sc::case",
            Level::Error,
            "Sigma3 AEAD decrypt failed: Error::InvalidSignature secret=synthetic",
        )
        .unwrap();
        assert_eq!(EVENTS[e.event as usize].1, "case_sigma3_decrypt_error");
        assert_eq!(ERRORS[e.error as usize], "InvalidSignature");
        let e = classify(
            "rs_matter_node_core::failsafe",
            Level::Warn,
            "Fail-Safe timeout expired for fabric 123456789, disarming",
        )
        .unwrap();
        assert_eq!(EVENTS[e.event as usize].1, "failsafe_expired");
        let e = classify(
            "rs_matter_dm::clusters::noc",
            Level::Info,
            "Got Remove Fabric Request",
        )
        .unwrap();
        assert_eq!(EVENTS[e.event as usize].1, "remove_fabric_request");
        assert!(classify("unrelated", Level::Error, secret).is_none());
        let e = classify("rs_matter_respond", Level::Error, secret).unwrap();
        assert_eq!(e.event as usize, EVENTS.len());
        assert_eq!(e.error, 0);
        let mut line = Line {
            data: [0; 256],
            len: 0,
        };
        assert!(write!(line, "{}", "x".repeat(400)).is_err());
        assert_eq!(line.len, 256);
    }
}
