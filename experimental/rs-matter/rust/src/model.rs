use super::platform;
use alloc::vec::Vec;
use bridged::ClusterHandler as _;
use core::cell::Cell;
use light::ClusterHandler as _;
use rs_matter::dm::clusters::decl::on_off as light;
use rs_matter::dm::clusters::groups::{self, ClusterHandler as _};
use rs_matter::dm::clusters::{
    decl::bridged_device_basic_information as bridged,
    desc::{self, ClusterHandler as _},
};
use rs_matter::dm::devices::{DEV_TYPE_AGGREGATOR, DEV_TYPE_BRIDGED_NODE, DEV_TYPE_ON_OFF_LIGHT};
use rs_matter::dm::{
    Async, Cluster, DataModel, Dataver, EmptyHandler, Endpoint, Handler, HandlerContext,
    InvokeContext, InvokeReply, MatchContext, Metadata, Node, NonBlockingHandler, ReadContext,
    ReadReply, WriteContext,
};
use rs_matter::error::{Error, ErrorCode};
use rs_matter::tlv::{Nullable, TLVBuilderParent, Utf8StrBuilder};
use rs_matter::{clusters, devices, root_endpoint, with, Matter};
extern "C" {
    fn lh_queue_power(endpoint: u16, command: u32) -> bool;
    fn lh_rs_observed(endpoint: u16) -> u32;
    fn lh_rs_generation(endpoint: u16) -> u32;
}

const ROOT: Endpoint<'static> = root_endpoint!(wifi);
#[derive(Clone, Copy, Default)]
struct Timed {
    on: u16,
    off: u16,
    active: bool,
    generation: u32,
}
pub struct Model {
    endpoints: Vec<Endpoint<'static>>,
    timers: [Cell<Timed>; 16],
    versions: [Cell<u32>; 16],
    previous: [Cell<u32>; 16],
}
impl Model {
    pub fn new(random: &mut impl rand::Rng) -> Self {
        let mut endpoints = Vec::with_capacity(18);
        endpoints.push(ROOT);
        endpoints.push(Endpoint::new(
            2,
            devices!(DEV_TYPE_AGGREGATOR),
            clusters!(desc::DescHandler::CLUSTER),
        ));
        for station in &super::config().stations[..super::config().count as usize] {
            endpoints.push(Endpoint::new(
                station.endpoint,
                devices!(DEV_TYPE_ON_OFF_LIGHT, DEV_TYPE_BRIDGED_NODE),
                clusters!(
                    desc::DescHandler::CLUSTER,
                    groups::GroupsHandler::CLUSTER,
                    Light::CLUSTER,
                    Bridged::CLUSTER
                ),
            ));
        }
        Self {
            endpoints,
            timers: core::array::from_fn(|_| Cell::new(Timed::default())),
            versions: core::array::from_fn(|_| Cell::new(random.next_u32())),
            previous: core::array::from_fn(|_| Cell::new(0)),
        }
    }
    fn index(&self, ep: u16) -> Result<usize, Error> {
        super::config().stations[..super::config().count as usize]
            .iter()
            .position(|s| s.endpoint == ep)
            .ok_or(ErrorCode::EndpointNotFound.into())
    }
}
impl Metadata for Model {
    fn access<F, R>(&self, f: F) -> R
    where
        F: FnOnce(&Node<'_>) -> R,
    {
        f(&Node {
            endpoints: &self.endpoints,
        })
    }
}
struct Light<'a> {
    model: &'a Model,
    index: usize,
}
impl Light<'_> {
    fn endpoint(&self) -> u16 {
        super::config().stations[self.index].endpoint
    }
    fn state(&self) -> u32 {
        unsafe { lh_rs_observed(self.endpoint()) }
    }
    fn command(&self, command: u32) -> Result<(), Error> {
        self.model.timers[self.index].set(Timed::default());
        if unsafe { lh_queue_power(self.endpoint(), command) } {
            Ok(())
        } else {
            Err(ErrorCode::Busy.into())
        }
    }
}
impl light::ClusterHandler for Light<'_> {
    const CLUSTER: Cluster<'static> = light::FULL_CLUSTER
        .with_features(light::Feature::LIGHTING.bits())
        .with_attrs(with!(required))
        .with_cmds(with!(
            light::CommandId::Off
                | light::CommandId::On
                | light::CommandId::Toggle
                | light::CommandId::OffWithEffect
                | light::CommandId::OnWithRecallGlobalScene
                | light::CommandId::OnWithTimedOff
        ));
    fn dataver(&self) -> u32 {
        self.model.versions[self.index].get()
    }
    fn dataver_changed(&self) {
        self.model.versions[self.index].set(self.dataver().wrapping_add(1));
    }
    fn on_off(&self, _: impl ReadContext) -> Result<bool, Error> {
        Ok(self.state() & 1 != 0)
    }
    fn global_scene_control(&self, _: impl ReadContext) -> Result<bool, Error> {
        Ok(true)
    }
    fn on_time(&self, _: impl ReadContext) -> Result<u16, Error> {
        Ok(self.model.timers[self.index].get().on)
    }
    fn off_wait_time(&self, _: impl ReadContext) -> Result<u16, Error> {
        Ok(self.model.timers[self.index].get().off)
    }
    fn start_up_on_off(
        &self,
        _: impl ReadContext,
    ) -> Result<Nullable<light::StartUpOnOffEnum>, Error> {
        Ok(Nullable::none())
    }
    fn handle_off(&self, _: impl InvokeContext) -> Result<(), Error> {
        self.command(0)
    }
    fn handle_on(&self, _: impl InvokeContext) -> Result<(), Error> {
        self.command(1)
    }
    fn handle_toggle(&self, _: impl InvokeContext) -> Result<(), Error> {
        self.command(2)
    }
    fn handle_off_with_effect(
        &self,
        _: impl InvokeContext,
        _: light::OffWithEffectRequest<'_>,
    ) -> Result<(), Error> {
        self.command(0)
    }
    fn handle_on_with_recall_global_scene(&self, _: impl InvokeContext) -> Result<(), Error> {
        self.command(1)
    }
    fn handle_on_with_timed_off(
        &self,
        ctx: impl InvokeContext,
        request: light::OnWithTimedOffRequest<'_>,
    ) -> Result<(), Error> {
        let control = request.on_off_control()?;
        let on = request.on_time()?;
        let off = request.off_wait_time()?;
        if control.contains(light::OnOffControlBitmap::ACCEPT_ONLY_WHEN_ON) && self.state() & 1 == 0
        {
            return Ok(());
        }
        let mut timer = self.model.timers[self.index].get();
        if timer.off > 0 && self.state() & 1 == 0 {
            timer.off = timer.off.min(off);
        } else {
            if !unsafe { lh_queue_power(self.endpoint(), 1) } {
                return Err(ErrorCode::Busy.into());
            }
            timer.on = timer.on.max(on);
            timer.off = off;
            timer.active = true;
            timer.generation = unsafe { lh_rs_generation(self.endpoint()) };
        }
        self.model.timers[self.index].set(timer);
        ctx.notify_own_attr_changed(0x4001);
        ctx.notify_own_attr_changed(0x4002);
        Ok(())
    }
    fn set_on_time(&self, ctx: impl WriteContext, value: u16) -> Result<(), Error> {
        let mut t = self.model.timers[self.index].get();
        t.on = value;
        self.model.timers[self.index].set(t);
        ctx.notify_changed();
        Ok(())
    }
    fn set_off_wait_time(&self, ctx: impl WriteContext, value: u16) -> Result<(), Error> {
        let mut t = self.model.timers[self.index].get();
        t.off = value;
        self.model.timers[self.index].set(t);
        ctx.notify_changed();
        Ok(())
    }
}
struct Bridged<'a> {
    model: &'a Model,
    index: usize,
}
impl bridged::ClusterHandler for Bridged<'_> {
    const CLUSTER: Cluster<'static> = bridged::FULL_CLUSTER
        .with_features(0)
        .with_attrs(
            with!(required; bridged::AttributeId::NodeLabel | bridged::AttributeId::UniqueID),
        )
        .with_cmds(with!());
    fn dataver(&self) -> u32 {
        self.model.versions[self.index].get()
    }
    fn dataver_changed(&self) {
        self.model.versions[self.index].set(self.dataver().wrapping_add(1));
    }
    fn reachable(&self, _: impl ReadContext) -> Result<bool, Error> {
        Ok(unsafe { lh_rs_observed(super::config().stations[self.index].endpoint) } & 2 != 0)
    }
    fn unique_id<P: TLVBuilderParent>(
        &self,
        _: impl ReadContext,
        b: Utf8StrBuilder<P>,
    ) -> Result<P, Error> {
        b.set(platform::text(&super::config().stations[self.index].unique))
    }
    fn node_label<P: TLVBuilderParent>(
        &self,
        _: impl ReadContext,
        b: Utf8StrBuilder<P>,
    ) -> Result<P, Error> {
        b.set(platform::text(&super::config().stations[self.index].name))
    }
    fn handle_keep_active(
        &self,
        _: impl InvokeContext,
        _: bridged::KeepActiveRequest<'_>,
    ) -> Result<(), Error> {
        Err(ErrorCode::CommandNotFound.into())
    }
}
struct Dispatch<'a>(&'a Model);
impl Handler for Dispatch<'_> {
    fn read(&self, ctx: impl ReadContext, reply: impl ReadReply) -> Result<(), Error> {
        let ep = ctx.attr().endpoint_id;
        let cluster = ctx.attr().cluster_id;
        if cluster == 0x1d {
            let h = if ep == 2 {
                desc::DescHandler::new_aggregator(Dataver::new(1))
            } else {
                desc::DescHandler::new(Dataver::new(1))
            };
            return h.adapt().read(ctx, reply);
        }
        let index = self.0.index(ep)?;
        match cluster {
            4 => groups::GroupsHandler::new(
                Dataver::new(self.0.versions[index].get()),
                super::matter(),
            )
            .adapt()
            .read(ctx, reply),
            6 => light::HandlerAdaptor(Light {
                model: self.0,
                index,
            })
            .read(ctx, reply),
            0x39 => bridged::HandlerAdaptor(Bridged {
                model: self.0,
                index,
            })
            .read(ctx, reply),
            _ => Err(ErrorCode::ClusterNotFound.into()),
        }
    }
    fn write(&self, ctx: impl WriteContext) -> Result<(), Error> {
        let index = self.0.index(ctx.attr().endpoint_id)?;
        if ctx.attr().cluster_id == 6 {
            light::HandlerAdaptor(Light {
                model: self.0,
                index,
            })
            .write(ctx)
        } else {
            Err(ErrorCode::InvalidAction.into())
        }
    }
    fn invoke(&self, ctx: impl InvokeContext, reply: impl InvokeReply) -> Result<(), Error> {
        let index = self.0.index(ctx.cmd().endpoint_id)?;
        if ctx.cmd().cluster_id == 4 {
            groups::GroupsHandler::new(Dataver::new(self.0.versions[index].get()), super::matter())
                .adapt()
                .invoke(ctx, reply)
        } else if ctx.cmd().cluster_id == 6 {
            light::HandlerAdaptor(Light {
                model: self.0,
                index,
            })
            .invoke(ctx, reply)
        } else {
            Err(ErrorCode::CommandNotFound.into())
        }
    }
    fn bump_dataver(&self, ctx: impl MatchContext) {
        if let Some(ep) = ctx.endpt() {
            if let Ok(i) = self.0.index(ep) {
                self.0.versions[i].set(self.0.versions[i].get().wrapping_add(1));
            }
        }
    }
    async fn run(&self, ctx: impl HandlerContext) -> Result<(), Error> {
        loop {
            super::window_control(&ctx)?;
            for (i, station) in super::config().stations[..super::config().count as usize]
                .iter()
                .enumerate()
            {
                let state = unsafe { lh_rs_observed(station.endpoint) };
                let before = self.0.timers[i].get();
                let (timer, sleep) = tick(before, unsafe { lh_rs_generation(station.endpoint) });
                if sleep {
                    unsafe {
                        lh_queue_power(station.endpoint, 0);
                    }
                }
                self.0.timers[i].set(timer);
                if timer.on != before.on || timer.off != before.off {
                    self.0.versions[i].set(self.0.versions[i].get().wrapping_add(1));
                    if timer.on != before.on {
                        ctx.notify_attr_changed(station.endpoint, 6, 0x4001);
                    }
                    if timer.off != before.off {
                        ctx.notify_attr_changed(station.endpoint, 6, 0x4002);
                    }
                }
                let old = self.0.previous[i].replace(state);
                if old != state {
                    self.0.versions[i].set(self.0.versions[i].get().wrapping_add(1));
                    if (state ^ old) & 1 != 0 {
                        ctx.notify_attr_changed(station.endpoint, 6, 0);
                    }
                    if (state ^ old) & 2 != 0 {
                        ctx.notify_attr_changed(station.endpoint, 0x39, 0x11);
                    }
                }
            }
            embassy_time::Timer::after_millis(100).await;
        }
    }
}
impl NonBlockingHandler for Dispatch<'_> {}
pub fn data_model<'a>(
    matter: &'a Matter<'a>,
    random: impl rand::Rng + Copy + 'a,
    model: &'a Model,
) -> impl DataModel + 'a {
    (
        model,
        EmptyHandler
            .chain(
                |e, _| e == 0,
                Async(super::Stack::root_handler(matter, &(), random)),
            )
            .chain(|e, _| e != 0, Async(Dispatch(model))),
    )
}

fn tick(mut timer: Timed, generation: u32) -> (Timed, bool) {
    if timer.active {
        if generation != timer.generation {
            return (Timed::default(), false);
        }
        if timer.on == 0xffff {
            return (timer, false);
        }
        timer.on = timer.on.saturating_sub(1);
        if timer.on == 0 {
            timer.active = false;
            return (timer, true);
        }
    } else if timer.off > 0 && timer.off != 0xffff {
        timer.off -= 1;
    }
    (timer, false)
}
#[cfg(test)]
mod timer_tests {
    use super::*;
    #[test]
    fn explicit_timer_expires_once_and_never_overrides_newer_work() {
        let initial = Timed {
            on: 1,
            off: 5,
            active: true,
            generation: 7,
        };
        let (expired, sleep) = tick(initial, 7);
        assert!(sleep);
        assert!(!tick(expired, 7).1);
        let (cancelled, sleep) = tick(initial, 8);
        assert!(!sleep);
        assert!(!cancelled.active);
        let forever = Timed {
            on: 0xffff,
            ..initial
        };
        assert!(!tick(forever, 7).1);
        assert_eq!(tick(forever, 7).0.on, 0xffff);
    }
}
