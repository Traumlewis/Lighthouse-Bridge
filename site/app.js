const installer = document.getElementById('installer');
const install = document.getElementById('install');
const confirmation = document.getElementById('board-confirm');
const status = document.getElementById('load-status');
let firmwareReady = false;

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
    document.getElementById('version').textContent = 'v' + manifest.version;
    document.getElementById('download').hidden = false;
    document.getElementById('checksum-link').hidden = false;
    firmwareReady = true;
    updateButton();
    status.textContent = 'Bereit. Bestätige dein Board und wähle anschließend den USB-Port. Erst im Installationsdialog wird geflasht.';
  } catch (error) {
    status.textContent = 'Firmware nicht verfügbar. Bitte später neu laden oder den Build-Status auf GitHub prüfen.';
    status.classList.add('error');
    document.getElementById('version').textContent = 'Nicht verfügbar';
  }
}

loadFlasher();
checkFirmware();
fetch('build-info.json', {cache:'no-store'}).then(response => response.ok ? response.json() : null).then(info => {
  if (info?.commit) document.getElementById('build-info').textContent = 'Build ' + info.commit.slice(0,7);
}).catch(() => {});
