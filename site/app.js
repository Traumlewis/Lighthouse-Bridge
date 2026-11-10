import { translations } from './translations.js?v=0.2.3';
import { validUpdateParts } from './flash-layout.mjs?v=0.2.3';
import { watchDeviceInfo } from './serial-info.mjs?v=0.2.3';

const installer = document.getElementById('installer');
const install = document.getElementById('install');
const confirmation = document.getElementById('board-confirm');
const status = document.getElementById('load-status');
let firmwareReady = false;
let firmwareState = 'checking';
let firmwareVersion = '';
let buildCommit = '';
let language = 'en';
const languagePicker = document.getElementById('language');
const languageKey = 'lighthouse-bridge.language';
let usbConnection = null, usbStarting = false, usbInfo = null, usbState = 'usb_idle', usbTimeout;
const supportedLanguage = value => value === 'en' || value === 'de';

function text(key) { return translations[language][key]; }
function renderStatus() {
  status.textContent = text(firmwareState);
  status.classList.toggle('error', firmwareState === 'unavailable');
  document.getElementById('version').textContent = firmwareVersion || text(
    firmwareState === 'unavailable' ? 'unavailable_version' : 'loading_version');
  if (buildCommit) document.getElementById('build-info').textContent = text('build') + ' ' + buildCommit;
}
function setLanguage(value, remember = false) {
  language = supportedLanguage(value) ? value : 'en';
  document.documentElement.lang = language;
  languagePicker.value = language;
  // All translated markup is authored locally in translations.js, never taken
  // from URL parameters, the manifest or device responses.
  document.querySelectorAll('[data-i18n]').forEach(node => {
    node.innerHTML = text(node.dataset.i18n);
  });
  document.querySelectorAll('[data-i18n-content]').forEach(node => {
    node.setAttribute('content', text(node.dataset.i18nContent));
  });
  document.querySelectorAll('[data-i18n-aria-label]').forEach(node => {
    node.setAttribute('aria-label', text(node.dataset.i18nAriaLabel));
  });
  renderStatus();
  renderUsbInfo();
  if (remember) {
    try { localStorage.setItem(languageKey, language); } catch (_) { /* Storage may be disabled. */ }
    const url = new URL(location.href);
    url.searchParams.set('lang', language);
    history.replaceState(null, '', url);
  }
}
let savedLanguage;
try { savedLanguage = localStorage.getItem(languageKey); } catch (_) { /* English is the default. */ }
const requestedLanguage = new URL(location.href).searchParams.get('lang');
setLanguage(supportedLanguage(requestedLanguage) ? requestedLanguage : savedLanguage);
languagePicker.addEventListener('change', () => setLanguage(languagePicker.value, true));

function updateButton() { install.disabled = !firmwareReady || !confirmation.checked || Boolean(usbConnection) || usbStarting; }
confirmation.addEventListener('change', updateButton);

async function loadFlasher() {
  try {
    await import('./vendor/install-button.js');
    await customElements.whenDefined('esp-web-install-button');
    installer.hidden = false;
  } catch (error) {
    document.getElementById('module-error').hidden = false;
  }
}

async function checkFirmware() {
  try {
    const response = await fetch('manifest.json', {cache:'no-store'});
    if (!response.ok) throw new Error('Manifest fehlt');
    const manifest = await response.json();
    const parts = manifest.builds?.find(build => build.chipFamily === 'ESP32')?.parts;
    if (!validUpdateParts(parts) || !manifest.version) throw new Error('Unsafe update manifest');
    await Promise.all(parts.map(async part => {
      const firmware = await fetch(new URL(part.path, response.url), {method:'HEAD',cache:'no-store'});
      if (!firmware.ok) throw new Error('Firmware segment missing');
    }));
    firmwareVersion = 'v' + manifest.version;
    document.getElementById('download').hidden = false;
    document.getElementById('checksum-link').hidden = false;
    firmwareReady = true;
    firmwareState = 'ready';
    updateButton();
    renderStatus();
  } catch (error) {
    firmwareState = 'unavailable';
    renderStatus();
  }
}

loadFlasher();
checkFirmware();
fetch('build-info.json', {cache:'no-store'}).then(response => response.ok ? response.json() : null).then(info => {
  if (info?.commit) { buildCommit = info.commit.slice(0,7); renderStatus(); }
}).catch(() => {});

function renderUsbInfo() {
  document.getElementById('usb-status').textContent = text(usbState);
  document.getElementById('usb-stop').hidden = !usbConnection;
  document.getElementById('usb-read').disabled = Boolean(usbConnection) || usbStarting;
  document.getElementById('usb-info').hidden = !usbInfo;
  if (usbInfo) {
    document.getElementById('usb-key').textContent = usbInfo.api_key;
    document.getElementById('usb-ssid').textContent = usbInfo.setup_ssid;
    const link = document.getElementById('usb-url');
    link.hidden = !usbInfo.url;
    document.getElementById('usb-wifi-wait').hidden = Boolean(usbInfo.url);
    if (usbInfo.url) { link.href = usbInfo.url; link.textContent = usbInfo.url; }
    else { link.removeAttribute('href'); link.textContent = ''; }
  }
}

async function startUsbInfo(knownPort) {
  if (usbConnection || usbStarting) return;
  if (!navigator.serial) { usbState = 'usb_unsupported'; renderUsbInfo(); return; }
  usbStarting = true; usbInfo = null; usbState = 'usb_wait'; updateButton(); renderUsbInfo();
  let ended = false;
  try {
    const port = knownPort || await navigator.serial.requestPort();
    const connection = await watchDeviceInfo(port, info => {
      clearTimeout(usbTimeout); usbInfo = info; usbState = 'usb_ready'; renderUsbInfo();
    }, () => {
      ended = true; clearTimeout(usbTimeout); usbConnection = null; usbStarting = false;
      if (usbState !== 'usb_timeout') usbState = usbInfo ? 'usb_disconnected' : 'usb_failed';
      updateButton(); renderUsbInfo();
    });
    if (!ended) usbConnection = connection;
    if (!usbInfo && !ended) usbTimeout = setTimeout(() => {
      usbState = 'usb_timeout'; renderUsbInfo(); usbConnection?.stop();
    },45000);
  } catch (error) { usbState = error.name === 'NotFoundError' ? 'usb_idle' : 'usb_failed'; }
  finally { usbStarting = false; updateButton(); renderUsbInfo(); }
}
document.getElementById('usb-read').addEventListener('click', () => startUsbInfo());
document.getElementById('usb-stop').addEventListener('click', () => usbConnection?.stop());
// ESP Web Tools owns its serial port until its dialog closes. Read only after
// that event and its asynchronous close(), without asking for another port.
document.addEventListener('closed', event => {
  const dialog = event.target;
  if (dialog.localName !== 'ewt-install-dialog' || !dialog.port) return;
  setTimeout(() => { if (!dialog.isConnected) startUsbInfo(dialog.port); },700);
});
