"""Private room label and explicit maintenance actions for rs-matter."""
import json
import re


def owner_dashboard(html):
    from matter_dashboard import replace_once
    def change(old, new):
        nonlocal html
        html = replace_once(html, old, new)
    change('<div class="eyebrow" data-t="hero"></div>', '''<div class="eyebrow"><button type="button" id="room-edit"><span id="room-label"></span></button></div><form id="room-form" hidden><label><span data-t="roomName"></span><input id="room-input" maxlength="64" required autocomplete="off"></label><div class="actions"><button type="submit" data-t="saveRoom"></button><button type="button" id="room-cancel" data-t="cancelRoom"></button></div></form>''')
    change('<details id="network-settings">', '''<details id="danger-settings"><summary>Dangerzone</summary><p><strong data-t="setupName"></strong><br><span id="danger-ssid"></span></p><p><strong data-t="setupPassword"></strong><br><button type="button" id="danger-copy-key"><code id="danger-key"></code></button></p><p class="hint" data-t="copyKey"></p><p data-t="restartHelp"></p><button type="button" id="bridge-restart" data-t="restartBridge"></button><p data-t="resetHelp"></p><button type="button" id="factory-reset" data-t="factoryReset"></button></details><details id="network-settings">''')
    change('</style>', '''
#room-edit{padding:0;border:0;background:transparent;color:inherit;font:inherit;text-align:left;max-width:100%;overflow-wrap:anywhere}#room-edit:hover:enabled{text-decoration:underline;text-underline-offset:4px}
#room-form{max-width:520px;margin:16px 0}#room-form .actions{display:flex;flex-wrap:wrap;gap:10px;margin-top:12px}
#danger-settings{border-color:var(--danger,#e87676)}#danger-ssid{overflow-wrap:anywhere}
#danger-copy-key{max-width:100%;padding:0;border:0;border-radius:0;background:transparent;color:var(--accent);font-weight:inherit;text-align:left;overflow-wrap:anywhere;cursor:copy}
#danger-copy-key code{color:inherit;white-space:normal;overflow-wrap:anywhere}#danger-copy-key:hover:enabled{filter:none;text-decoration:underline;text-underline-offset:3px}
</style>''')
    match = re.search(r'const T=(.*?);\n', html)
    translations = json.loads(match[1])
    translations['en'].update(roomName='Your room / owner',editRoom='Edit room or owner name',saveRoom='Save name',cancelRoom='Cancel',roomSaved='Room name saved.',ROOM_NAME_INVALID='Enter a name with 1–64 UTF-8 bytes and no control characters.',
        setupName='Setup Wi-Fi name',setupPassword='Setup Wi-Fi password',restartBridge='Restart bridge',restartHelp='Restart keeps your settings and Matter pairings. Control pauses briefly.',restartConfirm='Restart the bridge now?',restartAccepted='The bridge is restarting. Reload this page in a moment.',
        factoryReset='Factory reset',resetHelp='Removes Wi-Fi credentials, API key, web login, room name, stations, GPIO settings and Matter pairings. Afterwards, read the new setup Wi-Fi password over USB and set up the bridge again.',resetConfirm='Are you sure you want to factory-reset this bridge? All saved settings and Matter pairings will be deleted. You will need USB to read the new setup Wi-Fi password.',resetAccepted='Factory reset accepted. The bridge is restarting into first setup. Read its new setup Wi-Fi password over USB. The previous password is no longer valid.',RESET_CONFIRM_REQUIRED='Confirm the factory reset.',MAINTENANCE_INVALID='Invalid maintenance action.')
    translations['de'].update(roomName='Dein Raum / Besitzer',editRoom='Raum- oder Besitzernamen bearbeiten',saveRoom='Namen speichern',cancelRoom='Abbrechen',roomSaved='Raumname gespeichert.',ROOM_NAME_INVALID='Gib einen Namen mit 1–64 UTF-8-Bytes ohne Steuerzeichen ein.',
        setupName='Name des Setup-WLANs',setupPassword='Passwort des Setup-WLANs',restartBridge='Bridge neu starten',restartHelp='Beim Neustart bleiben Einstellungen und Matter-Kopplungen erhalten. Die Steuerung pausiert kurz.',restartConfirm='Bridge jetzt neu starten?',restartAccepted='Die Bridge startet neu. Lade diese Seite gleich erneut.',
        factoryReset='Factory-Reset',resetHelp='Löscht WLAN-Zugangsdaten, API-Schlüssel, Web-Anmeldung, Raumnamen, Basisstationen, GPIO-Einstellungen und Matter-Kopplungen. Lies danach das neue Setup-WLAN-Passwort per USB aus und richte die Bridge erneut ein.',resetConfirm='Bist du sicher, dass du diese Bridge auf Werkseinstellungen zurücksetzen möchtest? Alle gespeicherten Einstellungen und Matter-Kopplungen werden gelöscht. Du benötigst USB, um das neue Setup-WLAN-Passwort auszulesen.',resetAccepted='Factory-Reset angenommen. Die Bridge startet zur Ersteinrichtung neu. Lies das neue Setup-WLAN-Passwort per USB aus. Das bisherige Passwort ist nicht mehr gültig.',RESET_CONFIRM_REQUIRED='Bestätige den Factory-Reset.',MAINTENANCE_INVALID='Ungültige Wartungsaktion.')
    html = html[:match.start(1)] + json.dumps(translations, ensure_ascii=False) + html[match.end(1):]
    change('current=data;renderMatter();', 'current=data;renderOwner();renderMatter();')
    change('renderDebugMode();renderMatter();', 'renderDebugMode();renderMatter();renderOwner();')
    change('renderMatter();await refresh();', 'renderOwner();renderMatter();await refresh();')
    change("account.api_key=result.api_key;$('api-key').textContent=result.api_key;", "account.api_key=result.api_key;$('api-key').textContent=result.api_key;renderOwner();")
    change("$('setup-ssid').textContent=result.setup_ssid;", "$('setup-ssid').textContent=result.setup_ssid;renderOwner();")
    change("['copy-key','logout']", "['copy-key','danger-copy-key','logout']")
    change('account=null;current=null;', "account=null;current=null;ownerEpoch++;$('room-form').hidden=true;$('room-input').value='';$('room-edit').hidden=false;$('room-label').textContent='';$('danger-ssid').textContent='';$('danger-key').textContent='';$('danger-settings').open=false;")
    # Register before the initial setLanguage/loadAccount calls.
    change("loadAccount();const pollTimer=", '''let ownerEpoch=0,ownerBusy=false;
function renderOwner(){
 const privateView=loggedIn&&account&&!account.password_required;
 $('room-label').textContent=privateView?(account.room_name||t('hero')):'';$('room-edit').title=t('editRoom');$('room-edit').setAttribute('aria-label',t('editRoom'));
 const configured=(current?.stations||[]).some(s=>s.configured??Boolean(s.mac));
 document.querySelector('.intro h1').hidden=configured;document.querySelector('.intro p[data-t="intro"]').hidden=configured;
 $('danger-ssid').textContent=privateView?(account.setup_ssid||''):'';$('danger-key').textContent=privateView?(account.api_key||''):'';$('danger-copy-key').disabled=!privateView||!account.api_key;
}
$('room-edit').addEventListener('click',()=>{if(!loggedIn||!account||account.password_required||networkChanging)return;$('room-input').value=account.room_name||t('hero');$('room-form').hidden=false;$('room-edit').hidden=true;$('room-input').focus();$('room-input').select();});
function cancelRoom(){if(ownerBusy)return;$('room-form').hidden=true;$('room-edit').hidden=false;$('room-input').value='';$('room-edit').focus();}
$('room-cancel').addEventListener('click',cancelRoom);
$('room-input').addEventListener('keydown',event=>{if(event.key==='Escape'){event.preventDefault();cancelRoom();}});
$('room-form').addEventListener('submit',async event=>{event.preventDefault();if(ownerBusy||networkChanging)return;const name=$('room-input').value.trim();if(!name||new TextEncoder().encode(name).length>64){message(()=>t('ROOM_NAME_INVALID'),true);return;}const epoch=ownerEpoch;ownerBusy=true;for(const node of $('room-form').elements)node.disabled=true;try{const result=await api('/api/room',{name});if(epoch!==ownerEpoch||!loggedIn)return;account.room_name=result.name;$('room-form').hidden=true;$('room-edit').hidden=false;renderOwner();message(()=>t('roomSaved'));}catch(e){if(epoch===ownerEpoch)message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}finally{ownerBusy=false;for(const node of $('room-form').elements)node.disabled=networkChanging;}});
$('danger-copy-key').addEventListener('click',async()=>{try{await copyText($('danger-key').textContent);message(()=>t('copied'));}catch(e){message(()=>t('copyFailed'),true);}});
async function maintainBridge(action){
 if(ownerBusy||networkChanging||!confirm(t(action==='factory_reset'?'resetConfirm':'restartConfirm')))return;
 const epoch=ownerEpoch;ownerBusy=true;$('bridge-restart').disabled=$('factory-reset').disabled=true;
 try{await api('/api/maintenance',{action,...(action==='factory_reset'?{confirm:true}:{})});if(epoch!==ownerEpoch||!loggedIn)return;
 if(action==='factory_reset'){account.api_key='';$('api-key').textContent='';$('danger-key').textContent='';$('copy-key').disabled=$('danger-copy-key').disabled=true;}
 holdNetworkRestart(action==='factory_reset'?'resetAccepted':'restartAccepted');
 }catch(e){if(epoch===ownerEpoch)message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}finally{ownerBusy=false;if(!networkChanging)$('bridge-restart').disabled=$('factory-reset').disabled=false;}
}
$('bridge-restart').addEventListener('click',()=>maintainBridge('restart'));
$('factory-reset').addEventListener('click',()=>maintainBridge('factory_reset'));
loadAccount();const pollTimer=''')
    # setLanguage runs before the new declarations; renderOwner only uses existing
    # account/current state, while the event callbacks run after initialization.
    return html
