import { translations } from './translations.js';

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

function updateButton() { install.disabled = !firmwareReady || !confirmation.checked; }
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
    const part = manifest.builds?.find(build => build.chipFamily === 'ESP32')?.parts?.[0];
    if (!part || part.offset !== 0 || !part.path || !manifest.version) throw new Error('Ungültiges Manifest');
    const firmware = await fetch(new URL(part.path, response.url), {method:'HEAD',cache:'no-store'});
    if (!firmware.ok) throw new Error('Firmware-Datei fehlt');
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
