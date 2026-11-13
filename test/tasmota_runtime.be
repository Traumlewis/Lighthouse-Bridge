var diagnostic_events = []
global.lh_diag = def (event, first, second) diagnostic_events.push([event, first, second]) end
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
  var cluster, command, attribute, status
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
  var mdns_pase_wifi, hostname_wifi
  def init() self.mdns_pase_wifi = true end
  def is_commissioning_open() return true end
  def compute_manual_pairing_code() return "synthetic-code" end
end
class LighthouseTestSessions
  def count_active_fabrics() return 2 end
end
class LighthouseTestInfo
  var commissioning, sessions
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
