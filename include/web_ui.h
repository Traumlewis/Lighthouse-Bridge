#pragma once
#include <pgmspace.h>

const char WEB_UI[] PROGMEM = R"LHHTML(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="dark"><title>Lighthouse Bridge</title>
<style>
:root{color-scheme:dark;--bg:#101817;--card:#1b2624;--line:#33433f;--text:#edf5ef;--muted:#a3b9af;--accent:#c2f78c;--warn:#ffce95}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:16px/1.5 system-ui,sans-serif}main{max-width:980px;margin:auto;padding:36px 24px 70px}header{display:flex;align-items:center;justify-content:space-between;gap:16px;margin-bottom:42px}.brand{font-weight:700;letter-spacing:-.6px;font-size:20px}.mark{display:inline-grid;place-items:center;background:var(--accent);color:var(--bg);height:34px;width:34px;border-radius:10px;margin-right:10px}.pill{font-size:12px;color:var(--muted);border:1px solid var(--line);padding:6px 12px;border-radius:99px}h1{font-size:clamp(30px,5vw,46px);line-height:1.15;letter-spacing:-1.6px;margin:8px 0 14px}h2{font-size:18px;margin:0 0 8px}.eyebrow{color:var(--accent);font-size:12px;letter-spacing:1.6px;text-transform:uppercase}.muted{color:var(--muted)}.intro{max-width:650px}.group{margin:30px 0;display:flex;flex-wrap:wrap;gap:10px}button{font:inherit;font-weight:600;border:1px solid var(--line);border-radius:10px;padding:11px 18px;background:transparent;color:var(--text);cursor:pointer}button.primary{background:var(--accent);color:#15210f;border-color:var(--accent)}button:hover{filter:brightness(1.12)}button:disabled{opacity:.4;cursor:not-allowed}button:focus-visible,input:focus-visible,select:focus-visible,summary:focus-visible{outline:2px solid var(--accent);outline-offset:3px}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px}.card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:22px;min-width:0}.topline{display:flex;justify-content:space-between;align-items:center;gap:12px}.slot{font:12px ui-monospace,monospace;color:var(--muted)}.state{font-size:12px;border-radius:99px;padding:3px 9px;background:#2c3b35;color:var(--accent)}.station-name{font-size:20px;margin:18px 0 3px;overflow-wrap:anywhere}.address{font:12px ui-monospace,monospace;color:var(--muted)}.status{font-size:13px;margin:18px 0 14px;min-height:40px;color:var(--muted);overflow-wrap:anywhere}.actions{display:flex;gap:8px}.actions button{flex:1}.note{font-size:13px;color:var(--muted);margin:24px 0 30px}.warning{color:var(--warn)}details{border:1px solid var(--line);border-radius:14px;margin-top:14px;padding:18px 22px}summary{font-weight:600;cursor:pointer}details p{font-size:14px;color:var(--muted)}.fields{display:grid;grid-template-columns:1fr 1fr;gap:14px;margin:18px 0}label{display:block;color:var(--muted);font-size:13px}input,select{width:100%;margin-top:6px;background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:11px;color:var(--text);font:14px system-ui}.config-slot{border-top:1px solid var(--line);margin:20px 0;padding-top:16px}.config-slot h3{font-size:15px;margin:0}.config-slot .fields{margin:10px 0}.hint{font-size:12px;color:var(--muted)}#message{border:1px solid var(--line);padding:12px 16px;border-radius:10px;margin:16px 0;white-space:pre-wrap}#message[hidden]{display:none}footer{font-size:12px;color:var(--muted);display:flex;justify-content:space-between;margin-top:36px;gap:12px;flex-wrap:wrap}code{font-size:13px;color:var(--accent)}.scan-list{padding:0;list-style:none;font-size:13px}.scan-list li{padding:8px 0;border-bottom:1px solid var(--line);overflow-wrap:anywhere}@media(max-width:600px){main{padding:24px 16px 45px}header{margin-bottom:32px}.grid,.fields{grid-template-columns:1fr}.card{padding:18px}header .pill{max-width:135px;text-align:center}.group button{flex:1}details{padding:16px}.station-name{margin-top:12px}}
</style>
</head>
<body><main>
<header><div class="brand"><span class="mark" aria-hidden="true">◈</span>Lighthouse Bridge</div><span class="pill" id="connection" role="status">Verbinde …</span></header>
<section class="intro"><div class="eyebrow">Dein VR-Raum</div><h1>Bereit, wenn du es bist.</h1><p class="muted">Vier Basisstationen. Eine Steuerung.<br>Schalte sie einzeln oder gemeinsam in ON und SLEEP.</p></section>
<div id="message" role="status" hidden></div>
<div class="group"><button class="primary" id="all-on" disabled>Alle einschalten</button><button id="all-sleep" disabled>Alle in SLEEP</button></div>
<section class="grid" id="stations" aria-label="Basisstationen"></section>
<p class="note">„Aus“ bedeutet immer <strong>SLEEP</strong>. Nach dem Senden wird ein verfügbarer BLE-Zustand einmal gelesen. Die Anzeige ist keine laufende Überwachung.</p>
<details id="configuration"><summary>Basisstationen einrichten</summary>
<p>Suche nach eingeschalteten oder schlafenden Lighthouse-V2-Stationen in Reichweite. Ordne jede Station genau einem Platz zu und speichere die Auswahl.</p>
<button id="scan">Bluetooth-Suche starten</button><p class="hint" id="scan-hint">Die Suche dauert etwa fünf Sekunden; laufende Schaltbefehle werden zuerst bearbeitet.</p>
<ul id="scan-results" class="scan-list"></ul>
<form id="config-form"><div id="config-fields"></div><button class="primary" id="save" type="submit">Zuordnung speichern</button></form>
<p class="hint">Alexa-Namen können nach einer Umbenennung eine erneute Gerätesuche benötigen. Der gemeinsame Schalter heißt „VR Basisstationen“.</p>
</details>
<details><summary>Alexa verbinden</summary><p>ESP32 und Echo Dot müssen im selben Heimnetz sein. Starte nach der Zuordnung eine Gerätesuche in Alexa oder sage: „Alexa, suche Geräte.“</p><p>Es erscheinen vier einzelne Lampen und „VR Basisstationen“. Beispiel: „Alexa, schalte VR Basisstationen aus.“</p><p class="warning">Die Hue-Emulation ist experimentell und muss mit deinem Echo Dot der 5. Generation getestet werden. Alexas Schalter zeigt den angeforderten Zustand; BLE-Fehler siehst du hier.</p></details>
<details><summary>WLAN &amp; lokale API</summary><p>Für einen WLAN-Wechsel startet der ESP32 einen geschützten Setup-Zugangspunkt. Währenddessen pausiert die normale Bedienung. Alternativ nach dem Start die BOOT-Taste fünf Sekunden halten.</p><button id="wifi">WLAN-Setup öffnen</button><p class="hint">Das Setup-Passwort wird über USB im seriellen Monitor ausgegeben. Es ist auch das Web-Passwort für „admin“ und der API-Key. Beim ersten Setup öffnest du 192.168.4.1.</p><p>Für die spätere Pebble-Anbindung: <code>POST /api/power</code> mit <code>{"station":"all","state":"sleep"}</code> und dem Header <code>X-API-Key</code>. Status: <code>GET /api/status</code>.</p></details>
<footer><span id="device-address">Lokale Steuerung · ESP32</span><span>Valve Lighthouse V2 · Bridge 0.1.0</span></footer>
</main>
<script>
'use strict';
const $ = id => document.getElementById(id);
let current = null, formInitialized = false, discoveredSignature = '', polling = false;
const labels = {on:'ON',sleep:'SLEEP',standby:'STANDBY (gelesen)',booting:'Startet',unknown:'Unbekannt'};
function message(text, error=false) { $('message').textContent=text; $('message').hidden=false; $('message').classList.toggle('warning',error); }
async function api(path, data) {
  const controller = new AbortController(), timer = setTimeout(()=>controller.abort(),8000);
  try {
    const response = await fetch(path,{method:data===undefined?'GET':'POST',credentials:'same-origin',cache:'no-store',headers:data===undefined?{}:{'Content-Type':'application/json'},body:data===undefined?undefined:JSON.stringify(data),signal:controller.signal});
    let result; try { result=await response.json(); } catch(e) { throw new Error(response.status===401?'Bitte erneut mit admin anmelden.':'Ungültige Antwort vom ESP32.'); }
    if(!response.ok) throw new Error(result.error || 'Anfrage fehlgeschlagen ('+response.status+').');
    return result;
  } finally { clearTimeout(timer); }
}
function element(tag, cls, text) { const node=document.createElement(tag); if(cls) node.className=cls; if(text!==undefined) node.textContent=text; return node; }
function createCard(id) {
  const card=element('article','card'); card.id='card-'+id;
  const top=element('div','topline'); top.append(element('span','slot','STATION 0'+id));
  const badge=element('span','state','Unbekannt'); badge.id='state-'+id; top.append(badge); card.append(top);
  const name=element('h2','station-name','Basisstation '+id); name.id='name-'+id; card.append(name);
  const mac=element('div','address','Noch nicht zugeordnet'); mac.id='mac-'+id; card.append(mac);
  const status=element('div','status','Warte auf Verbindung …'); status.id='status-'+id; card.append(status);
  const actions=element('div','actions');
  for(const [state,text] of [['on','Einschalten'],['sleep','SLEEP']]) {
    const button=element('button',state==='on'?'primary':'',text); button.id=state+'-'+id; button.disabled=true;
    button.addEventListener('click',()=>power(id,state)); actions.append(button);
  }
  card.append(actions); $('stations').append(card);
}
function inputField(parent, text, id, value, maxLength) {
  const label=element('label','',text), input=element('input'); input.id=id; input.value=value; input.maxLength=maxLength; label.append(input); parent.append(label); return input;
}
function initForm(stations) {
  $('config-fields').replaceChildren();
  for(const station of stations) {
    const id=station.id, section=element('section','config-slot'); section.append(element('h3','','Platz '+id));
    const pickLabel=element('label','','Gefundene Station'), picker=element('select'); picker.id='pick-'+id;
    picker.append(new Option('Manuell eingeben oder Suche starten','')); pickLabel.append(picker); section.append(pickLabel);
    const fields=element('div','fields');
    inputField(fields,'Name für Alexa','edit-name-'+id,station.name,40);
    const mac=inputField(fields,'Bluetooth-Adresse (leer = deaktiviert)','edit-mac-'+id,station.mac,17); mac.placeholder='AA:BB:CC:DD:EE:FF'; mac.autocomplete='off';
    section.append(fields);
    const typeLabel=element('label','','Bluetooth-Adresstyp'), type=element('select'); type.id='edit-type-'+id;
    type.append(new Option('Öffentlich (0)','0'),new Option('Zufällig / Random (1)','1')); type.value=String(station.address_type); typeLabel.append(type); section.append(typeLabel);
    picker.addEventListener('change',()=>{ const found=current?.discovered.find(item=>item.mac===picker.value); if(found){mac.value=found.mac;type.value=String(found.address_type);} });
    $('config-fields').append(section);
  }
  formInitialized=true; discoveredSignature='';
}
function render(data) {
  current=data;
  $('connection').textContent=data.wifi_connected?'Im Heimnetz':'WLAN getrennt';
  $('device-address').textContent=data.ip+' · '+data.hostname+'.local';
  let configured=0, busy=data.scanning;
  for(const s of data.stations) {
    configured+=s.configured?1:0; busy ||= s.pending||s.busy;
    $('name-'+s.id).textContent=s.name; $('mac-'+s.id).textContent=s.mac||'Noch nicht zugeordnet';
    $('state-'+s.id).textContent=labels[s.observed]||'Unbekannt';
    let text='In den Einstellungen eine Station zuordnen.';
    if(s.configured) {
      if(s.pending) text='Wartet: '+labels[s.requested];
      else if(s.busy) text='Bluetooth-Verbindung · Ziel '+labels[s.requested];
      else if(s.error) text=s.error;
      else if(s.command_sent) text='Befehl '+labels[s.requested]+' gesendet. '+(s.observed_raw<0?'Keine Zustandsbestätigung.':'Zuletzt gelesen: '+labels[s.observed]+'.');
      else text='Noch kein Befehl gesendet.';
      if(s.observed_ms) text+=' Messung vor '+Math.floor(((data.uptime_ms-s.observed_ms)>>>0)/1000)+' s.';
    }
    $('status-'+s.id).textContent=text; $('status-'+s.id).classList.toggle('warning',Boolean(s.error));
    $('on-'+s.id).disabled=$('sleep-'+s.id).disabled=!s.configured;
  }
  $('all-on').disabled=$('all-sleep').disabled=!configured;
  $('save').disabled=Boolean(busy); $('wifi').disabled=Boolean(busy); $('scan').disabled=data.scanning;
  $('scan').textContent=data.scanning?'Suche vorgemerkt / läuft …':'Bluetooth-Suche starten';
  if(!formInitialized) initForm(data.stations);
  const signature=JSON.stringify(data.discovered);
  if(signature!==discoveredSignature) {
    discoveredSignature=signature;
    for(const s of data.stations) {
      const picker=$('pick-'+s.id), previous=picker.value;
      picker.replaceChildren(new Option('Manuell eingeben / Auswahl unverändert',''));
      for(const found of data.discovered) picker.append(new Option(found.name+' · '+found.mac,found.mac));
      if(data.discovered.some(item=>item.mac===previous)) picker.value=previous;
    }
    $('scan-results').replaceChildren();
    for(const found of data.discovered) $('scan-results').append(element('li','',found.name+' · '+found.mac+' · '+found.rssi+' dBm'));
  }
  if(data.scan_finished_ms&&!data.scanning) $('scan-hint').textContent=data.discovered.length+' Station(en) gefunden.'+(data.scan_truncated?' Ergebnis auf 16 Geräte begrenzt.':'');
}
async function refresh() {
  if(polling) return; polling=true;
  try { render(await api('/api/status')); }
  catch(e) { $('connection').textContent='Nicht erreichbar'; message(e.message,true); }
  finally { polling=false; }
}
async function power(station,state) {
  try { await api('/api/power',{station,state}); message((station==='all'?'Alle zugeordneten Stationen':'Station '+station)+': '+labels[state]+' angefordert.'); await refresh(); }
  catch(e) { message(e.message,true); }
}
for(let i=1;i<=4;i++) createCard(i);
$('all-on').addEventListener('click',()=>power('all','on'));
$('all-sleep').addEventListener('click',()=>power('all','sleep'));
$('scan').addEventListener('click',async()=>{try{await api('/api/scan',{});await refresh();}catch(e){message(e.message,true);}});
$('config-form').addEventListener('submit',async event=>{
  event.preventDefault();
  const stations=[];
  for(let i=1;i<=4;i++) stations.push({name:$('edit-name-'+i).value.trim(),mac:$('edit-mac-'+i).value.trim(),address_type:Number($('edit-type-'+i).value)});
  try{await api('/api/config',{stations});message('Zuordnung gespeichert. Du kannst jetzt schalten oder Alexa nach Geräten suchen lassen.');await refresh();}catch(e){message(e.message,true);}
});
$('wifi').addEventListener('click',async()=>{
  if(!confirm('WLAN-Setup starten? Die normale Bedienung pausiert für bis zu drei Minuten.'))return;
  try{const result=await api('/api/wifi',{});message('Verbinde dich mit '+result.setup_ssid+' und öffne http://192.168.4.1. Das Passwort steht im seriellen Monitor.');clearInterval(pollTimer);}catch(e){message(e.message,true);}
});
refresh(); const pollTimer=setInterval(refresh,2500);
</script></body></html>)LHHTML";
