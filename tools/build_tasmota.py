#!/usr/bin/env python3
"""Build the isolated Lighthouse/Tasmota prototype with the original dual slots."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]
PIN = '6aadc3f171f5b501cc325dcd78daf3d625b01d32'
VERSION = '0.1.4-tasmota'
UPSTREAM = ROOT / '.tools/tasmota-src'
STAGE = ROOT / '.pio/tasmota-src'
PROTOTYPE = ROOT / 'experimental/tasmota'

def write(path, data):
    data = data.encode() if isinstance(data,str) else data
    path.parent.mkdir(parents=True,exist_ok=True)
    if not path.exists() or path.read_bytes() != data: path.write_bytes(data)

def original(relative):
    return subprocess.check_output(['git','-C',str(UPSTREAM),'show',f'{PIN}:{relative}']).decode()

def replace_once(source, old, new):
    if source.count(old) != 1: raise ValueError(f'Pinned upstream boundary changed: {old[:80]}')
    return source.replace(old,new,1)

def patch(relative, changes):
    source = original(relative)
    for old,new in changes: source = replace_once(source,old,new)
    write(STAGE / relative,source)

def rotation_dashboard(html):
    import re
    match=re.search(r'const T=(.*?);\n',html)
    translations=json.loads(match[1])
    translations['en'].update(matterOpen='New code & open pairing window (15 minutes)',
        matterHelp='Generate a new code and open a pairing window before adding this bridge in Alexa. Each manual activation replaces the previous window code. Existing pairings stay connected. Codes expire with the window and after restart.',
        matterWindowPending='Generating a new setup code and opening the pairing window…')
    translations['de'].update(matterOpen='Neuer Code & Kopplungsfenster öffnen (15 Minuten)',
        matterHelp='Erzeuge vor dem Hinzufügen in Alexa einen neuen Code und öffne das Kopplungsfenster. Jede manuelle Aktivierung ersetzt den vorherigen Fenstercode. Bestehende Kopplungen bleiben verbunden. Codes gelten nur bis zum Schließen des Fensters oder Neustart.',
        matterWindowPending='Neuer Einrichtungscode wird erzeugt und das Kopplungsfenster geöffnet…')
    html=html[:match.start(1)]+json.dumps(translations,ensure_ascii=False)+html[match.end(1):]
    html=replace_once(html,"!matterInfo.pairing_code?'MATTER_CODE_UNAVAILABLE'", "matterInfo.commissioning_open&&!matterInfo.pairing_code?'MATTER_CODE_UNAVAILABLE'")
    html=replace_once(html,'if(matterBusy)return;const epoch=matterEpoch;',
        'if(matterBusy)return;clearTimeout(loadMatter.timer);const epoch=matterEpoch;')
    html=replace_once(html,"  if(open)await api('/api/matter/commissioning',{});",
        "  if(open){if(matterInfo)matterInfo={...matterInfo,pairing_code:null,commissioning_pending:true};renderMatter();await api('/api/matter/commissioning',{});}")
    html=replace_once(html,'  if(info.commissioning_pending)setTimeout(()=>{if(epoch===matterEpoch&&loggedIn)loadMatter();},750);',
        '  if(info.commissioning_pending||info.commissioning_open)loadMatter.timer=setTimeout(()=>{if(epoch===matterEpoch&&loggedIn)loadMatter();},info.commissioning_pending?750:2000);')
    html=replace_once(html,"$('matter-open').disabled=matterBusy||", "$('matter-open').disabled=matterBusy||!!matterInfo?.commissioning_pending||")
    html=replace_once(html,"}catch(e){if(epoch===matterEpoch)message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}",
        "}catch(e){if(epoch===matterEpoch){matterInfo=null;$('matter-code').textContent='';$('matter-code').hidden=true;$('matter-status').textContent='';message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}}")
    html=replace_once(html,'function showLogin(){matterEpoch++;','function showLogin(){clearTimeout(loadMatter.timer);matterEpoch++;')
    return html

def prepare():
    if not UPSTREAM.exists():
        subprocess.run(['git','clone','--no-checkout','--depth','1','https://github.com/arendst/Tasmota.git',str(UPSTREAM)],check=True)
    if subprocess.run(['git','-C',str(UPSTREAM),'cat-file','-e',PIN],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode:
        subprocess.run(['git','-C',str(UPSTREAM),'fetch','--depth','1','origin',PIN],check=True)
    stamp = STAGE / '.lighthouse-upstream'
    if not stamp.exists() or stamp.read_text() != PIN:
        data = subprocess.check_output(['git','-C',str(UPSTREAM),'archive',PIN])
        STAGE.mkdir(parents=True,exist_ok=True)
        with tarfile.open(fileobj=io.BytesIO(data)) as archive: archive.extractall(STAGE,filter='data')
        write(stamp,PIN)
    write(STAGE / 'tasmota/user_config_override.h',(PROTOTYPE / 'user_config_override.h').read_bytes())
    write(STAGE / 'tasmota/lighthouse_features.h',(PROTOTYPE / 'lighthouse_features.h').read_bytes())
    patch('tasmota/include/tasmota_configurations.h',[])
    configuration = (STAGE / 'tasmota/include/tasmota_configurations.h').read_text()
    write(STAGE / 'tasmota/include/tasmota_configurations.h',configuration+'\n#include "../lighthouse_features.h"\n')
    patch('tasmota/tasmota.ino',[
        ('void setup(void) {','void setup(void) {'),
        ('  SettingsLoad();','  SettingsLoad();\n  Settings->flag.mqtt_enabled = 0;\n  Settings->webserver = 0;'),
        ('  TasmotaGlobal.rules_flag.system_init = 1;','  TasmotaGlobal.rules_flag.system_init = 1;\n  lighthouseSetup();'),
        ('  Scheduler();','  Scheduler();\n  lighthouseLoop();')])
    # These declarations precede Arduino's generated prototypes.
    main = (STAGE / 'tasmota/tasmota.ino').read_text()
    write(STAGE / 'tasmota/tasmota.ino','#include "tasmota_runtime.h"\nstruct bvm;\n'+main)
    patch('tasmota/tasmota_support/support_wifi.ino',[
        ('void WiFiSetSleepMode(void)\n{','void WiFiSetSleepMode(void)\n{\n  return; // Lighthouse retains WIFI_PS_MIN_MODEM for BLE coexistence.'),
        ('void WifiDisable(void) {','void WifiDisable(void) {\n  return;'),
        ('void WifiShutdown(bool option) {','void WifiShutdown(bool option) {\n  return;'),
        ('void WifiSetMode(WiFiMode_t wifi_mode) {','void WifiSetMode(WiFiMode_t wifi_mode) {\n  return;'),
        ('void WifiConfig(uint8_t type)\n{','void WifiConfig(uint8_t type)\n{\n  return;'),
        ('void WifiBegin(uint8_t flag, uint8_t channel) {','void WifiBegin(uint8_t flag, uint8_t channel) {\n  return;'),
        ('void WifiConnect(void)\n{','void WifiConnect(void)\n{\n  return; // Lighthouse owns saved Wi-Fi and its protected portal.'),
        ('void WifiCheck(uint8_t param)\n{','void WifiCheck(uint8_t param)\n{\n  WifiSetState(WiFi.status() == WL_CONNECTED); return;')])
    patch('tasmota/tasmota_support/support_tasmota.ino',[
        ('void SerialInput(void) {','void SerialInput(void) {\n  return; // Physical USB is handled only by Lighthouse.'),
        ('if (TasmotaGlobal.ota_state_flag && CommandsReady()) {','if (false) { // Only the authenticated Lighthouse OTA receiver is enabled.')])
    patch('tasmota/tasmota_xdrv_driver/xdrv_02_9_mqtt.ino',[
        ('void MqttReconnect(void) {','void MqttReconnect(void) {\n  return; // No generic MQTT management in this appliance.'),
        ('void MqttCheck(void) {','void MqttCheck(void) {\n  return;')])
    patch('tasmota/tasmota_support/settings.ino',[
        ('void SettingsErase(uint8_t type) {','void SettingsErase(uint8_t type) {\n  return; // Never erase shared NVS or either app slot.'),
        ('void SettingsSdkErase(void) {','void SettingsSdkErase(void) {\n  return;')])
    patch('tasmota/tasmota_support/support.ino',[
        ('void AddLog(uint32_t loglevel, PGM_P formatP, ...) {','void AddLog(uint32_t loglevel, PGM_P formatP, ...) {\n  return; // Matter logs can contain credentials; use sanitized Lighthouse diagnostics.'),
        ('void AddLogData(uint32_t loglevel, const char* log_data, const char* log_data_payload = nullptr, const char* log_data_retained = nullptr) {',
         'void AddLogData(uint32_t loglevel, const char* log_data, const char* log_data_payload = nullptr, const char* log_data_retained = nullptr) {\n  return;')])
    patch('tasmota/tasmota_support/support_esp32.ino',[
        ('void SettingsErase(uint8_t type) {','void SettingsErase(uint8_t type) {\n  return; // Preserve all shared NVS and filesystem data.'),
        ('void EspPrepRestartToSafeBoot(void) {','void EspPrepRestartToSafeBoot(void) {\n  return;'),
        ('bool EspPrepSwitchPartition(uint32_t state) {','bool EspPrepSwitchPartition(uint32_t state) {\n  return false;'),
        ('bool OtaFactoryRead(void) {','bool OtaFactoryRead(void) {\n  return false;'),
        ('void NvmSave(const char *sNvsName, const char *sName, const void *pSettings, unsigned nSettingsLen) {','void NvmSave(const char *sNvsName, const char *sName, const void *pSettings, unsigned nSettingsLen) {\n  return;'),
        ('int32_t NvmErase(const char *sNvsName) {','int32_t NvmErase(const char *sNvsName) {\n  return ESP_ERR_NOT_SUPPORTED;'),
        ('  NvmSave("main", "Settings", pSettings, nSettingsLen);','  // Settings are isolated in the existing filesystem, never shared NVS.'),
        ('  if (NvmLoad("main", "Settings", data, size)) {','  if (false) {'),
        ('  NvmSave("qpc", "pcreg", pSettings, nSettingsLen);','  // No quick-power-cycle NVS writes.'),
        ('  NvmLoad("qpc", "pcreg", pSettings, nSettingsLen);','  memset(pSettings,0,nSettingsLen);')])
    patch('tasmota/tasmota_xdrv_driver/xdrv_01_9_webserver.ino',[
        ('void StartWebserver(int type) {','void StartWebserver(int type) {\n  return; // Authenticated Lighthouse HTTP is the only web listener.')])
    patch('tasmota/tasmota_xdrv_driver/xdrv_52_7_berry_embedded.ino',[
        ('  "do import tapp end "','  // External Tasmota applications are disabled.'),
        ('  "global.matter_device = matter.Device() "','  // Lighthouse creates the complete measured model after BLE/HTTP startup.'),
        ('  "if tasmota.settings.bootcount == 0 "\n    "import partition_core "\n    "var p = partition_core.Partition() "\n    "p.resize_fs_to_max() "\n  "end "', '  // The exact dual-slot partition table must never be resized.')])
    fs = original('tasmota/tasmota_xdrv_driver/xdrv_50_filesystem.ino')
    start = fs.index(' if (!LittleFS.begin(true, "")')
    end = fs.index('#endif // ESP32',start)
    fs = fs[:start]+' if (!lh_tasmota_mount_fs()) { ffsp = nullptr; return; }\n'+fs[end:]
    write(STAGE / 'tasmota/tasmota_xdrv_driver/xdrv_50_filesystem.ino',fs)
    for relative in ['Matter_z_Autoconf.be','Matter_Plugin_1_Aggregator.be','Matter_zz_Device.be']:
        path = 'lib/libesp32/berry_matter/src/embedded/'+relative
        source = original(path).replace('0x0001 #-matter.AGGREGATOR_ENDPOINT-#','0x0002 #-matter.AGGREGATOR_ENDPOINT-#')
        write(STAGE/path,source)
    ui=original('lib/libesp32/berry_matter/src/embedded/Matter_UI.be')
    license_header=ui[:ui.index('import matter')]
    write(STAGE/'lib/libesp32/berry_matter/src/embedded/Matter_UI.be',license_header+        'import matter\n#@ solidify:Matter_UI,weak\nclass Matter_UI\n  def init(device, enabled) end\nend\nmatter.UI = Matter_UI\n')
    all_plugins=original('lib/libesp32/berry_matter/src/embedded/Matter_Plugin_z_All.be')
    all_plugins=replace_once(all_plugins,'plugins_classes = register_native_classes()',
        'plugins_classes = {"v_light0":Matter_Plugin_Virt_Light0}')
    write(STAGE/'lib/libesp32/berry_matter/src/embedded/Matter_Plugin_z_All.be',all_plugins)
    # Matter needs Berry's native mDNS module even with generic Tasmota discovery off.
    for relative in ['lib/libesp32/berry/default/be_modtab.c',
                     'lib/libesp32/berry_tasmota/src/be_mdns_module.c',
                     'tasmota/tasmota_xdrv_driver/xdrv_52_3_berry_mdns.ino']:
        source=original(relative)
        if relative.endswith('be_modtab.c'):
            source=replace_once(source,'#ifdef USE_DISCOVERY\n    &be_native_module(mdns),',
                '#if defined(USE_DISCOVERY) || defined(USE_BERRY_MDNS)\n    &be_native_module(mdns),')
        else:
            source=replace_once(source,'#ifdef USE_DISCOVERY',
                '#if defined(USE_DISCOVERY) || defined(USE_BERRY_MDNS)')
        write(STAGE/relative,source)
    # Fixed numeric diagnostics never forward addresses, payloads or exceptions.
    patch('lib/libesp32/berry_matter/src/embedded/Matter_MessageHandler.be',[
        ('    var ret = false\n','    var ret = false\n    global.lh_diag(0, 0, 0)\n'),
        ('        if !frame.decode_payload()    return false end',
         '        if !frame.decode_payload()    return false end\n        global.lh_diag(1, frame.protocol_id, frame.opcode)'),
        ('        frame.decode_payload()\n','        frame.decode_payload()\n        global.lh_diag(1, frame.protocol_id, frame.opcode)\n'),
        ('      log("MTR: MessageHandler::msg_received exception: "+str(e)+";"+str(m), 2)',
         '      global.lh_diag(7, 0, 0)\n      log("MTR: MessageHandler::msg_received exception: "+str(e)+";"+str(m), 2)')])
    root='lib/libesp32/berry_matter/src/embedded/Matter_Plugin_1_Root.be'
    source=original(root)
    source=replace_once(source,'    var command = ctx.command\n',
        '    var command = ctx.command\n    global.lh_diag(2, cluster, command)\n')
    source=replace_once(source,'          self.device.commissioning.start_commissioning_complete_deferred(session)',
        '          self.device.commissioning.start_commissioning_complete_deferred(session)\n          global.lh_diag(6, 0, 0)')
    write(STAGE/root,source)
    patch('lib/libesp32/berry_matter/src/embedded/Matter_UDPServer.be',[
        ('    if ok\n','    if !ok global.lh_diag(4, 0, 0) end\n    if ok\n'),
        ('          self.packets_sent.remove(idx)\n',
         '          global.lh_diag(5, 0, 0)\n          self.packets_sent.remove(idx)\n')])
    commissioning='lib/libesp32/berry_matter/src/embedded/Matter_z_Commissioning.be'
    source=original(commissioning)
    diagnostic_boundary='      log("MTR: Exception" + str(e) + "|" + str(m), 2)'
    if source.count(diagnostic_boundary) != 6:
        raise ValueError('Pinned commissioning exception boundaries changed')
    source=source.replace(diagnostic_boundary,'      global.lh_diag(3, 0, 0)\n'+diagnostic_boundary)
    write(STAGE/commissioning,source)
    for name in ['Matter_MessageHandler.be','Matter_Plugin_1_Root.be','Matter_UDPServer.be','Matter_z_Commissioning.be']:
        target=STAGE/'lib/libesp32/berry_matter/src/embedded'/name
        write(target,replace_once(target.read_text(),'import matter\n','import matter\nimport global\n'))
    mdns_binding='tasmota/tasmota_xdrv_driver/xdrv_52_3_berry_mdns.ino'
    source=(STAGE/mdns_binding).read_text()
    source=replace_once(source,'        err = mdns_delegate_hostname_add(hostname, head);',
        '        err = mdns_hostname_exists(hostname) ? mdns_delegate_hostname_set_address(hostname, head) : mdns_delegate_hostname_add(hostname, head);')
    write(STAGE/mdns_binding,source)
    # The existing pinned minimal Arduino SDK omits SHA512, while the new HTTPUpdate
    # accepts it. No HTTPUpdate listener or OTA state machine is reachable here.
    write(STAGE / 'lighthouse-partitions.csv',(ROOT/'partitions.csv').read_bytes())
    board = json.loads(original('boards/esp32.json'))
    board['build']['extra_flags'] = '-DARDUINO_TASMOTA -DESP32_4M'
    board['build']['f_cpu'] = '240000000L'
    board['build']['partitions'] = 'lighthouse-partitions.csv'
    board['upload'].pop('arduino',None); board['upload']['maximum_size'] = 0x1e0000
    write(STAGE/'boards/lighthouse-esp32.json',json.dumps(board,indent=2))
    write(STAGE/'variants/tasmota/esp32/pins_arduino.h',original('pins_arduino/esp32/pins_arduino.h'))
    for name in ['lighthouse.cpp','tasmota_runtime.h','tasmota_matter_adapter.h']:
        write(STAGE/'tasmota'/name,(PROTOTYPE/name).read_bytes())
    write(STAGE/'tasmota/tasmota_xdrv_driver/xdrv_99_lighthouse_runtime.ino',(PROTOTYPE/'tasmota_runtime.ino').read_bytes())
    write(STAGE/'lib/libesp32/berry_matter/src/embedded/Matter_zzz_Lighthouse.be',(PROTOTYPE/'lighthouse.be').read_bytes())
    patch('lib/libesp32/berry_matter/src/be_matter_module.c',[
        ('#include "be_fixed_matter.h"','#include "solidify/solidified_Matter_zzz_Lighthouse.h"\n#include "be_fixed_matter.h"'),
        ('  Device, class(be_class_Matter_Device)',
         '  Device, class(be_class_Matter_Device)\n  LighthouseDevice, class(be_class_Lighthouse_Device)\n'+
         '\n'.join('  '+fn+', closure('+fn+'_closure)' for fn in ['lh_begin','lh_report','lh_info','lh_commission','lh_checkpoint','lh_network']))])
    patch('tasmota/tasmota_xdrv_driver/xdrv_52_9_berry.ino',[
        ('    BrLoad("preinit.be");','    // External startup scripts are disabled in the appliance prototype.'),
        ('        BrLoad("autoexec.be");','        // The embedded Lighthouse module is the sole application script.')])
    # Pin the exact BLE library used by the existing sequential worker.
    nimble = ROOT/'.tools/tasmota-nimble'
    if not nimble.exists():
        subprocess.run(['git','clone','--depth','1','--branch','2.5.2','https://github.com/h2zero/NimBLE-Arduino.git',str(nimble)],check=True)
    libraries={'NimBLE-Arduino':nimble,'ArduinoJson':ROOT/'.pio/libdeps/esp32dev/ArduinoJson',
               'WiFiManager':ROOT/'.pio/libdeps/esp32dev/WiFiManager'}
    for name,directory in libraries.items():
        expected={'NimBLE-Arduino':'2.5.2','ArduinoJson':'6.21.5','WiFiManager':'2.0.17'}[name]
        properties=dict(line.split('=',1) for line in (directory/'library.properties').read_text().splitlines() if '=' in line)
        if properties.get('version')!=expected:
            raise ValueError(f'Unexpected {name} version')
        target=STAGE/'lib/lighthouse'/name
        if not target.exists():
            target.parent.mkdir(parents=True,exist_ok=True); shutil.copytree(directory,target,ignore=shutil.ignore_patterns(".git",".pio","__pycache__"))
    write(STAGE/'lib/lighthouse/NimBLE-Arduino/src/lighthouse_nimble_policy.h',(PROTOTYPE/'lighthouse_nimble_policy.h').read_bytes())
    nimconfig=(nimble/'src/nimconfig.h').read_text()
    write(STAGE/'lib/lighthouse/NimBLE-Arduino/src/nimconfig.h',nimconfig.replace('#include "nimconfig_rename.h"','#include "nimconfig_rename.h"\n#include "lighthouse_nimble_policy.h"'))
    gap_path='src/nimble/nimble/host/src/ble_gap.c'
    gap=(nimble/gap_path).read_text()
    gap=replace_once(gap,'extern int is_ble_hs_resolv_enabled(void);','extern uint8_t is_ble_hs_resolv_enabled(void);')
    write(STAGE/'lib/lighthouse/NimBLE-Arduino'/gap_path,gap)
    formatter=original('tasmota/tasmota_xdrv_driver/xdrv_52_3_berry_matter.ino')
    formatter=formatter.replace('WebColor(COL_TEXT)','0xffffff').replace('WebColor(COL_BACKGROUND)','0x07101e')
    write(STAGE/'tasmota/tasmota_xdrv_driver/xdrv_52_3_berry_matter.ino',formatter)
    configure_environment(STAGE)
    return STAGE

def configure_environment(stage):
    flags=' '.join('-I'+str(path) for path in [ROOT/'include',ROOT/'src',ROOT/'experimental/matter/MatterBridge',stage/'tasmota'])
    write(stage/'platformio_override.ini','''[platformio]
default_envs = lighthouse
[env:lighthouse]
extends = env:tasmota32_base
board = lighthouse-esp32
board_build.partitions = lighthouse-partitions.csv
board_upload.maximum_size = 1966080
build_flags = ${env:tasmota32_base.build_flags}
    -DUSE_CONFIG_OVERRIDE
    -Os
    '''+flags+'''
lib_extra_dirs = ${env:tasmota32_base.lib_extra_dirs}
    lib/lighthouse
lib_ignore = HTTPUpdateServer, USB, NetBIOS, ArduinoOTA, esp-nimble-cpp
extra_scripts = pre:pio-tools/pre_source_dir.py
    pre:pio-tools/compress-html.py
    post:pio-tools/strip-flags.py
    pre:pio-tools/add_c_flags.py
    post:pio-tools/picolibc_flags.py
    post:pio-tools/dump-defines.py
    post:pio-tools/gen-berry-defines.py
    post:pio-tools/gen-berry-structures.py
''')

def source_fingerprint():
    digest=hashlib.sha256(PIN.encode())
    for directory in ['include','src','experimental/tasmota','experimental/matter/MatterBridge']:
        for path in sorted((ROOT/directory).rglob('*')):
            if path.is_file() and path.suffix in ['.h','.cpp','.ino','.be']:
                digest.update(str(path.relative_to(ROOT)).encode()+b'\0'+path.read_bytes())
    for relative in ['tools/build_tasmota.py','tools/build_matter.py','partitions.csv','tools/requirements-tasmota-build.txt']:
        digest.update((ROOT/relative).read_bytes())
    return digest.hexdigest()

REQUIRED_MODULES = ('json','crypto','mdns','path','introspect','string','math','matter')
def validate_runtime_modules(elf, nm):
    listing=subprocess.check_output([str(nm),str(elf)],text=True)
    symbols={line.split()[-1] for line in listing.splitlines() if line.split()}
    missing=[name for name in REQUIRED_MODULES if 'be_native_module_'+name not in symbols]
    if missing: raise ValueError('Missing linked Berry modules: '+', '.join(missing))


def package_build():
    import zipfile
    from build_matter import validate_segments
    from ota_package import package
    from package_release import source_path_allowed
    build=STAGE/'.pio/build/lighthouse'
    framework=ROOT/'.pio/tasmota-pio-core/packages/framework-arduinoespressif32'
    segments=[(0x1000,(build/'bootloader.bin').read_bytes()),(0x8000,(build/'partitions.bin').read_bytes()),
              (0xe000,(framework/'tools/partitions/boot_app0.bin').read_bytes()),(0x10000,(build/'firmware.bin').read_bytes())]
    validate_segments(segments)
    reference=ROOT/'dist/lighthouse-bridge-partitions.bin'
    if not reference.exists(): reference=PROTOTYPE/'reference-partitions.bin'
    if segments[1][1] != reference.read_bytes():
        raise ValueError('Tasmota must retain the exact complete stable partition table')
    if VERSION.encode() not in segments[-1][1] or b'LHBRIDGE-RUNTIME-V1-ESP32' not in segments[-1][1]:
        raise ValueError('Not the complete Lighthouse Tasmota application')
    output=ROOT/'dist/tasmota'; output.mkdir(parents=True,exist_ok=True)
    parts=[]; checksums=[]
    for name,(offset,data) in zip(['bootloader','partitions','boot-app0','app'],segments):
        filename=f'lighthouse-tasmota-{name}.bin'; write(output/filename,data)
        sha=hashlib.sha256(data).hexdigest(); checksums.append(f'{sha}  {filename}\n')
        parts.append({'offset':offset,'path':f'firmware/tasmota/{filename}?sha={sha}'})
    ota_name=f'lighthouse-bridge-{VERSION}.lhota'; ota=package(segments[-1][1]);write(output/ota_name,ota)
    checksums.append(f'{hashlib.sha256(ota).hexdigest()}  {ota_name}\n')
    write(output/'SHA256SUMS',''.join(checksums))
    write(output/'manifest.json',json.dumps({'name':'Lighthouse Bridge — Tasmota Matter prototype','version':VERSION,
        'new_install_prompt_erase':True,'new_install_improv_wait_time':0,
        'builds':[{'chipFamily':'ESP32','parts':parts}]},indent=2)+'\n')
    size_tool=ROOT/'.pio/tasmota-pio-core/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp-elf-size'
    validate_runtime_modules(build/'firmware.elf',size_tool.with_name('xtensa-esp-elf-nm'))
    sections=subprocess.check_output([str(size_tool),'-A',str(build/'firmware.elf')],text=True)
    ram=sum(int(line.split()[1]) for line in sections.splitlines() if line.startswith(('.dram0.data ','.dram0.bss ')))
    info={'version':VERSION,'engine':'tasmota-berry','upstream_commit':PIN,'app_bytes':len(segments[-1][1]),
          'app_slot_bytes':0x1e0000,'static_ram_bytes':ram,'hardware_validated':False,'stations':16,
          'bridge_name':'Lighthouse-Bridge Matter Test','source_fingerprint':source_fingerprint(),
          'platform':'https://github.com/tasmota/platform-espressif32/releases/download/2026.11.50/platform-espressif32.zip',
          'pioarduino':'6.2.0.post1','nimble':'2.5.2','license':'GPL-3.0-or-later'}
    write(output/'build-info.json',json.dumps(info,indent=2)+'\n')
    archive=output/f'lighthouse-bridge-{VERSION}.zip'
    temporary=archive.with_suffix('.zip.tmp')
    with zipfile.ZipFile(temporary,'w',zipfile.ZIP_DEFLATED) as bundle:
        for file in sorted(output.iterdir()):
            if file.name in {entry.split()[1] for entry in checksums}|{'SHA256SUMS','build-info.json'}: bundle.write(file,file.name)
        bundle.write(PROTOTYPE/'README.md','READ-ME-FIRST.md')
        bundle.write(PROTOTYPE/'LICENSE.txt','LICENSE-TASMOTA.txt')
        bundle.write(ROOT/'VERSION.md','VERSION.md')
        # Include complete corresponding modified Tasmota source, including the
        # Berry VM, libraries, generated bytecode headers and application driver.
        for file in sorted(STAGE.rglob('*')):
            relative=file.relative_to(STAGE)
            if file.is_file() and source_path_allowed(relative) and file.suffix not in ['.o','.a','.elf','.map','.bin'] and file.name!='berry':
                bundle.write(file,'source/tasmota-upstream/'+str(relative))
        for directory in ['include','src','experimental/tasmota','experimental/matter/MatterBridge','tools','test','third_party']:
            for file in sorted((ROOT/directory).rglob('*')):
                relative=file.relative_to(ROOT)
                if file.is_file() and source_path_allowed(relative): bundle.write(file,'source/'+str(relative))
        if not (PROTOTYPE/'reference-partitions.bin').exists():
            bundle.write(reference,'source/experimental/tasmota/reference-partitions.bin')
        for name in ['partitions.csv','VERSION.md','LICENSE','AGENTS.md']:
            bundle.write(ROOT/name,'source/'+name)
    temporary.replace(archive)
    print(f'Tasmota {VERSION}: {len(segments[-1][1])}/1966080 app bytes, {ram} static RAM bytes; no device validation.')

def main():
    global STAGE
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pio',default=str(ROOT/'.tools/tasmota-venv/bin/pio'))
    parser.add_argument('--prepare-only',action='store_true')
    parser.add_argument('--skip-compile',action='store_true')
    parser.add_argument('--source-tree',type=Path)
    parser.add_argument('--prepared',action='store_true',help='Use the complete patched source tree from the source ZIP')
    args=parser.parse_args()
    if args.source_tree: STAGE=args.source_tree.resolve()
    if args.prepared:
        if (STAGE/'.lighthouse-upstream').read_text()!=PIN: raise ValueError('Unexpected prepared source pin')
        configure_environment(STAGE)
    else: prepare()
    from build_matter import render_dashboard
    write(STAGE/'tasmota/web_ui_gzip.h',render_dashboard(VERSION,STAGE/'tasmota/matter-ui.html',rotation_dashboard))
    stamp=STAGE/'.lighthouse-build-inputs'
    fingerprint=source_fingerprint()
    if not args.skip_compile:
        coc=STAGE/'lib/libesp32/berry/tools/coc/coc'
        coc.chmod(coc.stat().st_mode | 0o111)  # ZIP extractors may discard executable bits.
        subprocess.run(['make','-C',str(STAGE/'lib/libesp32/berry'),'-j2'],check=True,stdout=subprocess.DEVNULL)
        subprocess.run([str(STAGE/'lib/libesp32/berry/berry'),'-s','-g','solidify_all.be'],cwd=STAGE/'lib/libesp32/berry_matter',check=True,stdout=subprocess.DEVNULL)
        if not args.prepare_only:
            import os
            env=dict(os.environ,PLATFORMIO_CORE_DIR=str(ROOT/'.pio/tasmota-pio-core'))
            subprocess.run([args.pio,'run','-d',str(STAGE),'-e','lighthouse'],check=True,env=env)
            write(stamp,fingerprint)
    elif not stamp.exists() or stamp.read_text()!=fingerprint:
        raise ValueError('Sources changed: compile before packaging')
    if not args.prepare_only: package_build()

if __name__=='__main__': main()
