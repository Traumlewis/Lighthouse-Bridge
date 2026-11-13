# Lighthouse adapter for Tasmota's Matter engine. GPL-3.0-or-later.
# The BLE worker alone writes power. Only C++ measurements update these shadows.
import matter
import global
import json

#@ solidify:Lighthouse_Light,weak
class Lighthouse_Light : Matter_Plugin_Virt_Light0
  static var TYPE = "lighthouse"
  var measured_reachable
  var unique_id
  def init(device, endpoint, config)
    self.measured_reachable = false
    self.unique_id = config["unique"]
    super(self).init(device, endpoint, config)
  end
  def invoke_request(session, val, ctx)
    if ctx.cluster == 6
      if ctx.command != 0 && ctx.command != 1 && ctx.command != 2
        ctx.status = 0x81
        return nil
      end
      if !global.lh_power(self.endpoint, ctx.command)
        ctx.status = 1
        return nil
      end
      return true
    end
    return super(self).invoke_request(session, val, ctx)
  end
  def set_onoff(pow)
    raise "value_error", "Power requires the Lighthouse queue"
  end
  def update_virtual(payload)
    # No generic virtual-device command can replace a measured reading.
  end
  def read_attribute(session, ctx, tlv)
    if ctx.cluster == 0x39
      if ctx.attribute == 0x11
        return tlv.set(8, self.measured_reachable)
      elif ctx.attribute == 0x12 || ctx.attribute == 0x0F
        return tlv.set(0x0C, self.unique_id)
      end
    end
    return super(self).read_attribute(session, ctx, tlv)
  end
  def observed(on, reachable)
    if on != self.shadow_onoff
      self.shadow_onoff = on
      self.attribute_updated(6, 0)
    end
    if reachable != self.measured_reachable
      self.measured_reachable = reachable
      self.attribute_updated(0x39, 0x11)
    end
  end
end

#@ solidify:Lighthouse_Device,weak
class Lighthouse_Device : matter.Device
  # Shared C++ registry reserves aggregator 2 and allocates lights from 3.
  var setup_window_code, setup_window_salt
  static var EP = 3
  static var plugins_classes = {"lighthouse":Lighthouse_Light}
  def load_param()
    # Only credentials/fabrics live in the engine's own files. C++ owns bindings.
    super(self).load_param()
    var previous = self.plugins_config
    var next = global.lh_configuration["stations"]
    var changed = previous == nil || size(previous) != size(next)
    if !changed
      for key: next.keys()
        var before = previous.find(key)
        if before == nil
          changed = true
        else
          for field: ["type","name","unique"]
            if before.find(field) != next[key].find(field) changed = true end
          end
        end
      end
    end
    if changed
      self.configuration_version += 1
      if self.configuration_version <= 0 self.configuration_version = 1 end
    end
    self.plugins_config = next
    self.plugins_config_remotes = {}
    self.plugins_persist = true
    self.next_ep = global.lh_configuration["next_endpoint"]
    self.root_passcode = global.lh_configuration["passcode"]
    self.root_discriminator = global.lh_configuration["discriminator"]
    self.disable_bridge_mode = false
    self.lights_delayed_on = false
  end
  def start()
    # The base constructor already starts immediately when Wi-Fi is connected.
    if self.udp_server != nil && self.udp_server.listening return end
    super(self).start()
  end
  def autoconf_device()
    if size(self.plugins) == 0
      matter.Autoconf(self).instantiate_plugins_from_config(self.plugins_config)
      if size(self.plugins) != size(self.plugins_config) + 2
        raise "value_error", "Incomplete Lighthouse endpoint model"
      end
    end
  end
end

#@ solidify:lh_begin,weak
def lh_begin(configuration)
  import json
  global.lh_configuration = json.load(configuration)
  global.matter_device = matter.LighthouseDevice()
  global.matter_device.start()
  global.matter_device.save_param()
  if !global.matter_device.udp_server.listening
    raise "network_error", "Matter UDP did not start"
  end
end

#@ solidify:lh_report,weak
def lh_report(endpoint, on, reachable)
  var plugin = global.matter_device.find_plugin_by_endpoint(endpoint)
  if plugin == nil  raise "value_error", "Unknown Lighthouse endpoint" end
  plugin.observed(on, reachable)
end

#@ solidify:lh_info,weak
def lh_info()
  var device = global.matter_device
  import json
  global.lh_diag(8, (device.commissioning.is_commissioning_open() ? 1 : 0) |
                   (device.commissioning.mdns_pase_wifi ? 2 : 0), device.sessions.count_active_fabrics())
  var commissioning = device.commissioning
  var code = nil
  if commissioning.is_commissioning_open()
    if device.setup_window_salt != nil && device.setup_window_salt == commissioning.commissioning_salt
      code = device.setup_window_code
    elif commissioning.is_root_commissioning_open()
      code = commissioning.compute_manual_pairing_code()
    end
  end
  return json.dump({"code":code,
                    "paired":device.sessions.count_active_fabrics() > 0,
                    "open":device.commissioning.is_commissioning_open()})
end

#@ solidify:lh_commission,weak
def lh_commission()
  import crypto
  var device = global.matter_device
  var commissioning = device.commissioning
  var saved_passcode = device.root_passcode
  var saved_discriminator = device.root_discriminator
  var old_code = device.setup_window_code
  var root_code = commissioning.compute_manual_pairing_code()
  device.setup_window_code = nil
  try
    commissioning.stop_basic_commissioning()
    var code
    while true
      device.root_passcode = commissioning.generate_random_passcode()
      device.root_discriminator = crypto.random(2).get(0, 2) & 0xFFF
      code = commissioning.compute_manual_pairing_code()
      if code != root_code && code != old_code break end
    end
    commissioning.start_root_basic_commissioning(900)
    if !commissioning.is_commissioning_open() || !commissioning.mdns_pase_wifi
      raise "network_error", "Commissioning window unavailable"
    end
    device.setup_window_code = code
    device.setup_window_salt = commissioning.commissioning_salt
  except .. as e, m
    device.root_passcode = saved_passcode
    device.root_discriminator = saved_discriminator
    device.setup_window_code = nil
    device.setup_window_salt = commissioning.commissioning_salt
    try commissioning.stop_basic_commissioning() except .. end
    raise e, m
  end
  # The active verifier uses the fresh PIN; persistent bootstrap credentials stay.
  device.root_passcode = saved_passcode
  device.root_discriminator = saved_discriminator
end

#@ solidify:lh_checkpoint,weak
def lh_checkpoint()
  if global.contains("matter_device") && global.matter_device != nil
    global.matter_device.sessions.save_fabrics()
    global.matter_device.save_before_restart()
  end
end

#@ solidify:lh_network,weak
def lh_network()
  import mdns
  var wifi = tasmota.wifi()
  mdns.add_hostname(global.matter_device.commissioning.hostname_wifi,
                    wifi.find("ip6local", ""), wifi.find("ip", ""))
end
