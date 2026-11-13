import { translations } from './translations.js?v=tasmota-0.1.3';
import { validMatterParts, validTasmotaParts } from './flash-layout.mjs?v=tasmota-0.1.3';
import { watchDeviceInfo, parseDeviceInfo } from './serial-info.mjs?v=tasmota-0.1.3';
import { parseMatterInfo } from './matter-info.mjs?v=tasmota-0.1.3';

const $ = id => document.getElementById(id);
const isTasmota = document.body.dataset.prototype === 'tasmota';
const prototypeName = isTasmota ? 'tasmota' : 'matter';
const validPrototypeParts = isTasmota ? validTasmotaParts : validMatterParts;
const supported = value => value === 'en' || value === 'de';
const languageKey = 'lighthouse-bridge.language';
let language = 'en', ready = false, version = '', loadState = 'checking';
let connection = null, starting = false, info = null, deviceInfo = null, usbState = 'usb_idle', timeout;
const text = key => translations[language][key];

function render() {
  $('matter-load').textContent = text(loadState) + (version ? ' · v' + version : '');
  $('matter-load').classList.toggle('error', loadState === 'unavailable');
  $('matter-install').disabled = !ready || !$('matter-confirm').checked || Boolean(connection) || starting;
  $('matter-read').disabled = Boolean(connection) || starting;
  $('matter-stop').hidden = !connection;
  const usbText = {usb_idle:'matter_usb_idle',usb_wait:'matter_usb_wait',usb_timeout:'matter_usb_timeout'};
  $('matter-usb-status').textContent = text(usbText[usbState] || usbState);
  $('matter-access').hidden = !deviceInfo;
  $('matter-key').textContent = deviceInfo?.api_key || '';
  $('matter-ssid').textContent = deviceInfo?.setup_ssid || '';
  $('matter-url').hidden = !deviceInfo?.url;
  $('matter-url').href = deviceInfo?.url || '#';
  $('matter-url').textContent = deviceInfo?.url || '';
  $('matter-device').hidden = !info;
  if (!info) return;
  const stations = info.stations || (info.station_id ? [{id:info.station_id,name:info.station_name,
    reachable:info.reachable,observed_raw:info.observed_raw}] : []);
  $('matter-station').textContent = String(stations.length);
  $('matter-bridge').textContent = info.bridge_name || 'Lighthouse Matter Test';
  const list = $('matter-stations'); list.replaceChildren();
  for (const station of stations) {
    const item = document.createElement('li');
    item.textContent = `${station.id} · ${station.name} — ` + measurement(station);
    list.append(item);
  }
  $('matter-code').textContent = info.pairing_code || '—';
  $('matter-code').disabled = !info.pairing_code;
  const errors = {CONFIGURE_STABLE_FIRST:'matter_need_config',WAITING_WIFI:'matter_wait_wifi'};
  $('matter-pair-state').textContent = info.error ? text(errors[info.error] || 'matter_device_error') + ' (' + info.error + ')' :
    text(info.paired ? 'matter_paired' : 'matter_enter_code');
  $('matter-measurement').textContent = stations.length ? '' : text('matter_no_station');
}
function measurement(station) {
  return text(!station.reachable ? 'matter_unavailable' : [1,8,9].includes(station.observed_raw) ? 'matter_starting' :
    station.observed_raw === 11 ? 'matter_on' : station.observed_raw === 0 ? 'matter_sleep' : 'matter_other');
}
function setLanguage(value, remember = false) {
  language = supported(value) ? value : 'en'; document.documentElement.lang = language; $('language').value = language;
  document.title = text(isTasmota ? 'tasmota_page_title' : 'matter_page_title');
  document.querySelector('meta[name="description"]').content = text(isTasmota ? 'tasmota_description' : 'matter_description');
  document.querySelectorAll('[data-i18n]').forEach(node => { node.innerHTML = text(node.dataset.i18n); });
  document.querySelectorAll('[data-i18n-aria-label]').forEach(node => node.setAttribute('aria-label',text(node.dataset.i18nAriaLabel)));
  if (remember) {
    try { localStorage.setItem(languageKey, language); } catch (_) {}
    const url = new URL(location.href); url.searchParams.set('lang',language); history.replaceState(null,'',url);
  }
  render();
}
let saved;
try { saved = localStorage.getItem(languageKey); } catch (_) {}
const requested = new URL(location.href).searchParams.get('lang');
setLanguage(supported(requested) ? requested : saved);
$('language').addEventListener('change',() => setLanguage($('language').value,true));
$('matter-confirm').addEventListener('change',render);

async function check() {
  try {
    const response = await fetch(`${prototypeName}-manifest.json`,{cache:'no-store'});
    if (!response.ok) throw new Error();
    const manifest = await response.json(), parts = manifest.builds?.[0]?.parts;
    if (manifest.builds?.length !== 1 || manifest.builds[0].chipFamily !== 'ESP32' ||
        !new RegExp('^\\d+\\.\\d+\\.\\d+-' + prototypeName + '$').test(manifest.version) || !validPrototypeParts(parts)) throw new Error();
    await Promise.all(parts.map(async part => {
      const result = await fetch(new URL(part.path,response.url),{method:'HEAD',cache:'no-store'});
      if (!result.ok) throw new Error();
    }));
    version = manifest.version; ready = true; loadState = 'ready';
    $('matter-download').href = `firmware/${prototypeName}/lighthouse-bridge-${version}.zip`;
    $('matter-download').hidden = false;
    $('matter-ota').href = `firmware/${prototypeName}/lighthouse-bridge-${version}.lhota`;
    $('matter-ota').hidden = false;
  } catch (_) { loadState = 'unavailable'; }
  render();
}
import('./vendor/install-button.js').then(() => customElements.whenDefined('esp-web-install-button')).then(() => {
  $('matter-installer').hidden = false;
}).catch(() => { $('matter-module-error').hidden = false; });
check();

async function readInfo(knownPort) {
  if (connection || starting) return;
  if (!navigator.serial) { usbState = 'usb_unsupported'; render(); return; }
  starting = true; info = null; deviceInfo = null; usbState = 'usb_wait'; render();
  let ended = false;
  try {
    const port = knownPort || await navigator.serial.requestPort();
    const watcher = await watchDeviceInfo(port, value => {
      clearTimeout(timeout); if(value.kind==='access')deviceInfo=value.data;else info=value.data; usbState = 'usb_ready'; render();
    }, () => {
      ended = true; clearTimeout(timeout); connection = null; starting = false;
      if (usbState !== 'usb_timeout') usbState = info ? 'usb_disconnected' : 'usb_failed'; render();
    }, {parse:line=>{const access=parseDeviceInfo(line);if(access)return {kind:'access',data:access};const matter=parseMatterInfo(line);return matter?{kind:'matter',data:matter}:null;},query:'LH_INFO\nLH_MATTER_INFO\n'});
    if (!ended) connection = watcher;
    if (!info && !ended) timeout = setTimeout(() => { usbState = 'usb_timeout'; render(); connection?.stop(); },45000);
  } catch (error) { usbState = error.name === 'NotFoundError' ? 'usb_idle' : 'usb_failed'; }
  finally { starting = false; render(); }
}
$('matter-read').addEventListener('click',() => readInfo());
$('matter-stop').addEventListener('click',() => connection?.stop());
document.addEventListener('closed',event => {
  const dialog = event.target;
  if (dialog.localName === 'ewt-install-dialog' && dialog.port) {
    setTimeout(() => { if (!dialog.isConnected) readInfo(dialog.port); },700);
  }
});
$('matter-code').addEventListener('click',async () => {
  if (!info?.pairing_code) return;
  try {
    let copied = false;
    try { if (navigator.clipboard && window.isSecureContext) { await navigator.clipboard.writeText(info.pairing_code); copied = true; } } catch (_) {}
    if (!copied) {
      const field = document.createElement('textarea'); field.value = info.pairing_code;
      field.style.position = 'fixed'; field.style.opacity = '0'; document.body.append(field); field.select();
      const ok = document.execCommand('copy'); field.remove(); if (!ok) throw new Error();
    }
    $('matter-copy-status').textContent = text('matter_copied');
  } catch (_) { $('matter-copy-status').textContent = text('matter_copy_failed'); }
});

$('matter-key').addEventListener('click',async()=>{
 if(!deviceInfo?.api_key)return;
 try {
  let copied=false;
  try{if(navigator.clipboard&&window.isSecureContext){await navigator.clipboard.writeText(deviceInfo.api_key);copied=true;}}catch(_){}
  if(!copied){const input=document.createElement('textarea');input.value=deviceInfo.api_key;input.style.position='fixed';input.style.opacity='0';document.body.append(input);input.select();const ok=document.execCommand('copy');input.remove();if(!ok)throw new Error();}
  $('matter-copy-status').textContent=text('copied');
 }catch(_){$('matter-copy-status').textContent=text('matter_copy_failed');}
});
