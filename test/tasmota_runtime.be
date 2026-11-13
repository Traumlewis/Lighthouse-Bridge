var diagnostic_events = []
global.lh_diag = def (*args) diagnostic_events.push(args) end
# Real pinned Berry plugin/data-model tests. Transport and clock are host fakes.
class LighthouseTestClock
  static var _debug_present = false
  def millis() return 0 end
  def wifi(*args) return global.lh_test_wifi end
  def publish_result(*args) end
  def version() return 0x01000000 end
  def loglevel(level) return false end
end
global.tasmota = LighthouseTestClock()
global.log = def (*args) end
class LighthouseTestEvents
  def publish_event(*args) end
end
class LighthouseTestDevice : global.Lighthouse_Device
  var updates
  def init()
    self.tick = 0
    self.disable_bridge_mode = false
    self.plugins = []
    self.events = LighthouseTestEvents()
    self.updates = []
    self.plugins_config = {}
    for i: 1..17
      if i == 2 continue end
      self.plugins_config[str(i)] = {"type":"lighthouse","name":format("Station %02i",i),"unique":format("test-e%i",i)}
    end
  end
  def attribute_updated(ep, cluster, attribute, fabric)
    self.updates.push([ep, cluster, attribute])
  end
end
class LighthouseTestContext
  var cluster, command, attribute, status, endpoint, msg, TimedRequest, command_fields, log
  def init(cluster, command)
    self.cluster = cluster
    self.command = command
  end
end
var calls = []
var accept = true
global.lh_power = def (endpoint, command)
  calls.push([endpoint, command])
  return accept
end
var device = LighthouseTestDevice()
device.autoconf_device()
assert(size(device.plugins) == 18, "full bridge+16 endpoint model")
assert(device.find_plugin_by_endpoint(0) != nil)
assert(device.find_plugin_by_endpoint(2) != nil)
var TLV = matter.TLV
for ep: 1..17
  if ep == 2 continue end
  var light = device.find_plugin_by_endpoint(ep)
  assert(light != nil && light.endpoint == ep)
  assert(!light.shadow_onoff && !light.measured_reachable, "no optimistic boot state")
  assert(size(calls) == 0, "no boot power command")
  light.observed(true,true)
  var count = size(device.updates)
  light.observed(true,true)
  assert(size(device.updates) == count, "no duplicate subscription changes")
  var ctx = LighthouseTestContext(0x39,nil)
  ctx.attribute = 0x11
  assert(light.read_attribute(nil,ctx,TLV.Matter_TLV_item()).val == true)
  ctx.attribute = 0x12
  assert(light.read_attribute(nil,ctx,TLV.Matter_TLV_item()).val == format("test-e%i",ep))
  ctx.cluster = 0x1D
  ctx.attribute = 0
  var types = light.read_attribute(nil,ctx,TLV.Matter_TLV_item())
  assert(types.val[0].val[0].val == 0x100, "On/Off Light classification")
  ctx.cluster = 6
  ctx.attribute = 0
  assert(light.read_attribute(nil,ctx,TLV.Matter_TLV_item()).val == true)
  for command: 0..2
    var command_context = LighthouseTestContext(6,command)
    assert(light.invoke_request(nil,nil,command_context) == true)
    assert(light.shadow_onoff && light.measured_reachable, "acceptance cannot alter measurements")
  end
  assert(size(calls) == 3)
  assert(calls[0][0] == ep && calls[0][1] == 0)
  assert(calls[1][0] == ep && calls[1][1] == 1)
  assert(calls[2][0] == ep && calls[2][1] == 2)
  calls.clear()
  var unsupported = LighthouseTestContext(6,0x40)
  assert(light.invoke_request(nil,nil,unsupported) == nil && unsupported.status == 0x81)
  assert(size(calls) == 0)
  accept = false
  var busy = LighthouseTestContext(6,0)
  assert(light.invoke_request(nil,nil,busy) == nil && busy.status == 1)
  assert(light.shadow_onoff && light.measured_reachable)
  calls.clear()
  accept = true
  light.update_virtual({"Power":0})
  assert(light.shadow_onoff && size(calls) == 0, "MtrUpdate cannot send power")
  var blocked = false
  try light.set_onoff(false) except .. blocked = true end
  assert(blocked && light.shadow_onoff && size(calls) == 0)
  light.observed(false,false)
  assert(!light.shadow_onoff && !light.measured_reachable)
end
# Real Descriptor serialization must contain the retained first light and all 16.
var context = LighthouseTestContext(0x1D,nil)
context.attribute = 3
var children = device.find_plugin_by_endpoint(2).read_attribute(nil,context,TLV.Matter_TLV_item())
assert(size(children.val) == 16, "aggregator PartsList must not truncate")
var found_first = false
for child: children.val
  assert(child.val != 0 && child.val != 2)
  if child.val == 1 found_first = true end
end
assert(found_first, "retained endpoint 1 must survive")
print("Tasmota Berry passed: actual 16-light model, Descriptor children, On/Off/Toggle dispatch, measured-only updates, identity, busy/unsupported rejection, no boot/virtual writes.")

var empty = LighthouseTestDevice()
empty.plugins_config = {}
empty.autoconf_device()
assert(size(empty.plugins) == 2, "zero station bridge")
var single = LighthouseTestDevice()
single.plugins_config = {"1":{"type":"lighthouse","name":"Retained first","unique":"test-e1"}}
single.autoconf_device()
assert(size(single.plugins) == 3 && single.find_plugin_by_endpoint(1) != nil)
assert(size(calls) == 0, "empty/single startup must not send power")

# Run the real numeric receive/root hooks without transport or packet contents.
var root = single.find_plugin_by_endpoint(0)
var invalid_command = LighthouseTestContext(0x30,0x7F)
root.invoke_request(nil,nil,invalid_command)
assert(diagnostic_events[-1] == [2,0x30,0x7F])
var handler = matter.MessageHandler(single)
handler.msg_received(bytes(), "private-peer-not-recorded", 5540)
var saw_receive = false
for event: diagnostic_events
  assert(size(event)==3 && type(event[0])=="int" && type(event[1])=="int" && type(event[2])=="int")
  if event[0]==0 saw_receive = true end
end
assert(saw_receive, "real receive hook runs without logging peer or payload")

class LighthouseTestCommissioning
  var mdns_pase_wifi, hostname_wifi, commissioning_salt
  def init() self.mdns_pase_wifi = true end
  def is_commissioning_open() return true end
  def is_root_commissioning_open() return true end
  def compute_manual_pairing_code() return "synthetic-code" end
end
class LighthouseTestSessions
  def count_active_fabrics() return 2 end
end
class LighthouseTestInfo
  var commissioning, sessions, setup_window_code, setup_window_salt, root_passcode, root_discriminator
  def init()
    self.commissioning = LighthouseTestCommissioning()
    self.sessions = LighthouseTestSessions()
  end
end
import json
global.matter_device = LighthouseTestInfo()
var info = json.load(global.lh_info())
assert(info["code"]=="synthetic-code" && info["paired"] && info["open"])
var metadata = diagnostic_events[-1]
assert(metadata[0]==8 && metadata[1]==3 && metadata[2]==2)

class LighthouseTestSocket
  var listening
  def init() self.listening = true end
end
class LighthouseTestAnnouncements
  var starts
  def init() self.starts = 0 end
  def start_mdns_announce_hostnames() self.starts += 1 end
end
class LighthouseTestStart : global.Lighthouse_Device
  var starts
  def init()
    self.starts = 0
    self.commissioning = LighthouseTestAnnouncements()
  end
  def autoconf_device() end
  def _start_udp(port)
    self.starts += 1
    self.udp_server = LighthouseTestSocket()
  end
end
var startup = LighthouseTestStart()
startup.start()
startup.start()
assert(startup.starts==1 && startup.commissioning.starts==1, "constructor plus explicit start must announce once")
startup.udp_server.listening = false
startup.start()
assert(startup.starts==2 && startup.commissioning.starts==2, "stopped transport can restart")
global.lh_test_wifi = {"ip":"192.0.2.1"}
global.matter_device.commissioning.hostname_wifi = "synthetic-host"
global.lh_network()
global.lh_test_wifi["ip6local"] = "fe80::1"
global.lh_network()
assert(size(global.lh_mdns_calls)==2)
assert(global.lh_mdns_calls[0][0]=="synthetic-host" && global.lh_mdns_calls[0][1]=="" && global.lh_mdns_calls[0][2]=="192.0.2.1")
assert(global.lh_mdns_calls[1][0]=="synthetic-host" && global.lh_mdns_calls[1][1]=="fe80::1")
assert(global.matter_device.sessions.count_active_fabrics()==2 && size(calls)==0)

# The active window captures a fresh verifier while bootstrap fields are restored.
import crypto
# Native checksum/crypto bindings are host fakes; production keeps pinned bindings.
var checksum = module("checksum")
checksum.checksum = def (value) return "0" end
matter.Verhoeff = checksum
var entropy = 100
crypto.random = def (length)
  entropy += 1
  var value = bytes()
  for i: 1..length value.add(entropy,1) end
  return value
end
class LighthouseTestRotation : matter.Commissioning
  var fail, fail_stop, captured_passcode, captured_discriminator, duration
  def init(device)
    self.device = device
    self.fail = false
    self.fail_stop = false
  end
  def stop_basic_commissioning()
    if self.fail_stop raise "test_error", "synthetic stop failure" end
    self.commissioning_open = nil
    self.commissioning_salt = nil
    self.mdns_pase_wifi = false
  end
  def start_root_basic_commissioning(duration)
    self.duration = duration
    self.captured_passcode = self.device.root_passcode
    self.captured_discriminator = self.device.root_discriminator
    self.commissioning_salt = crypto.random(16)
    self.commissioning_open = 900000
    self.mdns_pase_wifi = true
    if self.fail raise "test_error", "synthetic failure" end
  end
end
var pairing = global.matter_device
pairing.root_passcode = 20202021
pairing.root_discriminator = 1234
pairing.commissioning = LighthouseTestRotation(pairing)
var bootstrap_code = pairing.commissioning.compute_manual_pairing_code()
assert(json.load(global.lh_info())["code"]==nil, "closed windows expose no code")
global.lh_commission()
var first_code = json.load(global.lh_info())["code"]
assert(size(first_code)==11 && first_code!=bootstrap_code)
assert(pairing.root_passcode==20202021 && pairing.root_discriminator==1234)
assert(pairing.commissioning.captured_passcode!=20202021 && pairing.commissioning.duration==900)
global.lh_commission()
var second_code = json.load(global.lh_info())["code"]
assert(size(second_code)==11 && second_code!=first_code && second_code!=bootstrap_code)
pairing.commissioning.stop_basic_commissioning()
assert(json.load(global.lh_info())["code"]==nil, "closing invalidates the displayed window code")
global.lh_commission()
pairing.commissioning.commissioning_salt = crypto.random(16)
pairing.commissioning.commissioning_admin_fabric = true
assert(json.load(global.lh_info())["code"]==nil, "another administrator's window must not expose our old code")
pairing.commissioning.fail = true
var failed = false
try global.lh_commission() except .. failed = true end
assert(failed && pairing.root_passcode==20202021 && pairing.root_discriminator==1234)
assert(!pairing.commissioning.is_commissioning_open() && pairing.setup_window_code==nil)
assert(pairing.sessions.count_active_fabrics()==2 && size(calls)==0, "rotation must not remove fabrics or send power")

pairing.commissioning.fail = false
global.lh_commission()
pairing.commissioning.fail_stop = true
failed = false
try global.lh_commission() except .. failed = true end
assert(failed && pairing.root_passcode==20202021 && pairing.root_discriminator==1234)
assert(json.load(global.lh_info())["code"]==nil, "cleanup failure cannot expose an incorrect bootstrap code")
pairing.commissioning.fail_stop = false
pairing.commissioning.stop_basic_commissioning()

# Exercise actual IM read serialization, including explicit unsupported attributes.
var im = matter.IM(single)
var read_context = LighthouseTestContext(6,nil)
read_context.endpoint = 1
read_context.attribute = 0
read_context.status = 0x86
var read_bytes = im.read_single_attribute_to_bytes(single.find_plugin_by_endpoint(1),read_context,nil,false)
assert(size(read_bytes)>0)
var read_metadata = diagnostic_events[-1]
assert(size(read_metadata)==5 && read_metadata[0]==9 && read_metadata[1]==1 && read_metadata[2]==6 && read_metadata[3]==0 && read_metadata[4]==0)
read_context.attribute = 0x1234
read_context.status = 0x86
read_bytes = im.read_single_attribute_to_bytes(nil,read_context,nil,false)
read_metadata = diagnostic_events[-1]
assert(size(read_bytes)>0 && read_metadata[4]==0x86)
for event: diagnostic_events
  assert(size(event)==3 || size(event)==5)
  for value: event assert(type(value)=="int", "metadata cannot contain attribute values, peer addresses or credential strings") end
end

class LighthouseTestFrame
  var payload
  def encode_frame(raw, destination) self.payload=raw.copy() end
  def encrypt() end
end
class LighthouseTestWire
  var session, raw
  def init() self.raw=bytes() end
  def build_response(*args) return LighthouseTestFrame() end
end
class LighthouseTestSender
  var sent
  def send_response_frame(frame) self.sent=frame.payload end
end
class LighthouseTestInvokeDevice
  var message_handler
  def init() self.message_handler=LighthouseTestSender() end
  def invoke_request(session, fields, ctx)
    if ctx.command==4
      ctx.command=5
      return matter.TLV.Matter_TLV_struct()
    end
    ctx.status=0x81
    return nil
  end
end
matter.get_command_name=def (*args) return nil end
var invoking = LighthouseTestInvokeDevice()
var invoke_im = matter.IM(invoking)
var invoke_ctx = LighthouseTestContext(0x30,4)
invoke_ctx.endpoint=0
invoke_ctx.TimedRequest=false
assert(invoke_im.process_invoke_request_solo(LighthouseTestWire(),invoke_ctx))
var invoke_metadata=diagnostic_events[-1]
assert(invoke_metadata[0]==10 && invoke_metadata[3]==4 && invoke_metadata[4]==0, "log original request command, not mutated response ID")
invoke_ctx.command=6
assert(invoke_im.process_invoke_request_solo(LighthouseTestWire(),invoke_ctx))
invoke_metadata=diagnostic_events[-1]
assert(invoke_metadata[3]==6 && invoke_metadata[4]==0x81, "protocol rejection must be visible without a transport exception")

var noc_status = matter.TLV.Matter_TLV_struct()
noc_status.add_TLV(0,6,3)
assert(invoke_im.lh_response_status(0x3e,6,noc_status,0x81)==0x203)
assert(invoke_im.lh_response_status(0x30,4,noc_status,0x81)==0x203)
var certificate_response = matter.TLV.Matter_TLV_struct()
certificate_response.add_TLV(0,0x10,bytes("AABB"))
assert(invoke_im.lh_response_status(0x3e,2,certificate_response,0x81)==0, "never interpret certificate bytes as a status")
assert(invoke_im.lh_response_status(0x30,4,true,0)==0, "diagnostics must preserve boolean success results")

var write_result = matter.WriteResponseMessage()
write_result.write_responses = []
read_context.cluster = 0x3f
read_context.attribute = 0
read_context.status = 0x88
im.write_single_attribute_status_to_bytes(write_result,read_context,"synthetic-sensitive-value-not-recorded")
var write_metadata = diagnostic_events[-1]
assert(write_metadata[0]==12 && write_metadata[2]==0x3f && write_metadata[3]==0 && write_metadata[4]==0x88)
for event: diagnostic_events
  for value: event assert(type(value)=="int") end
end

# TC is optional and not advertised: a probe rejection is not a proven GS014 cause.
var tc_context=LighthouseTestContext(0x30,6)
tc_context.endpoint=0
tc_context.attribute=0xfffc
tc_context.status=0x81
var actual_root=single.find_plugin_by_endpoint(0)
assert(actual_root.read_attribute(nil,tc_context,matter.TLV.Matter_TLV_item()).val==0)
assert(actual_root.invoke_request(nil,nil,tc_context)==nil && tc_context.status==0x81)
