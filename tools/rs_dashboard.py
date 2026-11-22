"""Optional account controls and per-station OFF preferences for rs-matter only."""
import json
import re


def preferences_dashboard(html):
    from matter_dashboard import replace_once
    def change(old, new):
        nonlocal html
        html = replace_once(html, old, new)
    change('<header><div class="header-brand"><span class="pill" id="connection" role="status">Sign in</span><div class="brand">',
           '<header><div class="header-brand"><div class="brand">')
    change('Lighthouse Bridge</div></div><div class="header-actions">',
           'Lighthouse Bridge</div><span class="pill" id="connection" role="status">Sign in</span></div><div class="header-actions">')
    change('</style>', '''
.header-actions select,#logout{font:600 16px/1.5 system-ui,sans-serif;padding:11px 18px;border:1px solid var(--line);border-radius:10px;min-height:48px;background:transparent;color:var(--text)}
.header-actions select{padding-right:32px}.header-brand{flex-wrap:nowrap}.header-brand .brand{min-width:0}.header-brand .pill{white-space:nowrap}
@media(max-width:600px){.header-brand{width:100%;justify-content:space-between}.header-brand .brand{font-size:17px}.header-brand .pill{font-size:11px;padding:6px 8px;max-width:none}.mark{margin-right:6px}}
#stations .station-name{margin:0;font-size:20px;min-width:0}#stations .topline{align-items:flex-start}#stations .state{flex-shrink:0;max-width:48%}
#stations .slot{display:block;margin-top:12px}#stations .address{margin-top:4px;overflow-wrap:anywhere}
#stations .status{height:auto;min-height:0;margin:12px 0;scrollbar-gutter:auto}#stations .debug .status{height:6em;min-height:6em;overflow:auto;scrollbar-gutter:stable}
#stations .actions{margin-top:18px}.account-toggle{display:flex;align-items:center;gap:10px;margin:16px 0}.account-toggle input{width:auto;margin:0}
#station-info-dialog{width:min(34rem,calc(100% - 2rem));max-height:85vh;box-sizing:border-box;overflow:auto;color:var(--text);background:var(--bg);border:1px solid var(--line);border-radius:12px;padding:18px}
#station-info-dialog::backdrop{background:#0009}#station-info-data{white-space:pre-wrap;overflow-wrap:anywhere;font:inherit}
#stations .info-action{display:block;margin-top:8px}#username-form{max-width:460px}#username-form button{margin-top:12px}.discovered-choice{margin-top:10px}
</style>''')
    change('id="refresh"', 'id="refresh" hidden')
    change('function renderDebugMode(){', "function renderDebugMode(){$('refresh').hidden=!debugEnabled;")
    change('<p class="muted" data-t="loginHelp"></p>', '<p class="muted" id="login-help" data-t="loginHelp" hidden></p>')
    change('<p data-t="username">Username: admin</p>', '<label id="login-username-label" hidden><span data-t="usernameLabel"></span><input id="login-username" autocomplete="username" maxlength="32"></label>')
    change('<button id="change-password" data-t="changePassword"></button>', '''<button id="change-password" data-t="changePassword"></button><form id="username-form"><label class="account-toggle"><input type="checkbox" id="username-enabled"><span data-t="usernameEnabled"></span></label><label id="username-setting-label" hidden><span data-t="usernameLabel"></span><input id="web-username" autocomplete="username" maxlength="32" pattern="[A-Za-z0-9_.-]{1,32}"></label><p data-t="usernameHelp"></p><button type="submit" data-t="saveUsername"></button></form>''')
    # Place rotation beside the private key, never in public information.
    change('<code id="api-key"></code></button>', '<code id="api-key"></code></button><p><button type="button" id="rotate-key" data-t="rotateKey"></button></p>')
    match=re.search(r'const T=(.*?);\n',html)
    translations=json.loads(match[1])
    translations['en'].update(security='Change password and username',changePassword='Password',
        usernameLabel='Username',usernameEnabled='Require a username at sign-in',saveUsername='Save username settings',
        usernameHelp='Optional. Use 1–32 letters, numbers, dots, hyphens or underscores. First setup needs no username. Saving signs out other browser sessions.',
        usernameSaved='Username settings saved.',USERNAME_INVALID='Use a username with 1–32 letters, numbers, dots, hyphens or underscores.',
        rotateKey='Generate a new API key',keyConfirm='Replace the API key? Existing API integrations need the new key. The setup Wi-Fi password changes too; your router Wi-Fi credentials and web password stay unchanged.',keySaved='New API key saved. Update your integrations.',KEY_CONFIRM_REQUIRED='Confirm API key replacement.',
        station='Base station',stationId='STATION ID',allSleep='Turn all off',turnOff='Turn off',standby='STANDBY',
        offState='When switched off',offHelp='Used by Alexa and the dashboard. SLEEP stops the station; STANDBY keeps the motor running.',
        note='Each station uses its saved SLEEP or STANDBY preference when switched off. Displayed states are measured over Bluetooth.',
        STATE_INVALID='Only on, off, sleep and standby are allowed.',CONFIG_OFF_STATE='Choose SLEEP or STANDBY.',
        chooseDiscovered='Choose a discovered device…',discoveredLabel='Discovered Bluetooth devices',
        loginHelp='For the first login, use the API key shown over USB. Then choose your web password.',
        LOGIN_FAILED='Sign-in failed. Check your sign-in details.')
    translations['de'].update(security='Passwort und Benutzername ändern',changePassword='Passwort',
        usernameLabel='Benutzername',usernameEnabled='Benutzernamen bei der Anmeldung verlangen',saveUsername='Benutzernamen speichern',
        usernameHelp='Optional. 1–32 Buchstaben, Ziffern, Punkte, Bindestriche oder Unterstriche. Bei der Ersteinrichtung ist kein Benutzername nötig. Speichern meldet andere Browser-Sitzungen ab.',
        usernameSaved='Einstellungen zum Benutzernamen gespeichert.',USERNAME_INVALID='Verwende einen Benutzernamen mit 1–32 Buchstaben, Ziffern, Punkten, Bindestrichen oder Unterstrichen.',
        rotateKey='Neuen API-Schlüssel erzeugen',keyConfirm='API-Schlüssel ersetzen? Bestehende API-Integrationen benötigen danach den neuen Schlüssel. Auch das Passwort des Setup-WLANs ändert sich; Router-WLAN und Web-Passwort bleiben unverändert.',keySaved='Neuer API-Schlüssel gespeichert. Aktualisiere deine Integrationen.',KEY_CONFIRM_REQUIRED='Bestätige das Ersetzen des API-Schlüssels.',
        stationId='STATION ID',allSleep='Alle ausschalten',turnOff='Ausschalten',standby='STANDBY',
        offState='Beim Ausschalten',offHelp='Gilt für Alexa und das Dashboard. SLEEP stoppt die Station; bei STANDBY läuft der Motor weiter.',
        note='Beim Ausschalten verwendet jede Station ihre gespeicherte Auswahl SLEEP oder STANDBY. Die angezeigten Zustände werden über Bluetooth gemessen.',
        STATE_INVALID='Erlaubt sind on, off, sleep und standby.',CONFIG_OFF_STATE='Wähle SLEEP oder STANDBY.',
        chooseDiscovered='Gefundenes Gerät auswählen …',discoveredLabel='Gefundene Bluetooth-Geräte',
        loginHelp='Verwende bei der ersten Anmeldung den über USB angezeigten API-Schlüssel. Wähle danach dein Web-Passwort.',
        LOGIN_FAILED='Anmeldung fehlgeschlagen. Prüfe deine Anmeldedaten.')
    translations['en'].update(stationInfo='Device information', stationInfoRead='Reading Bluetooth device information…', STATION_INFO_FAILED='Device information could not be read.', infoType='Device type', infoName='Bluetooth name', infoFirmware='Firmware version', infoChannel='Channel', infoManufacturer='Manufacturer name', infoModel='Model number', infoSerial='Serial number', infoHardware='Hardware revision', infoMissing='Unavailable', infoClose='Close')
    translations['de'].update(stationInfo='Geräteinformationen', stationInfoRead='Bluetooth-Geräteinformationen werden gelesen …', STATION_INFO_FAILED='Die Geräteinformationen konnten nicht gelesen werden.', infoType='Gerätetyp', infoName='Bluetooth-Name', infoFirmware='Firmware-Version', infoChannel='Kanal', infoManufacturer='Hersteller', infoModel='Modellnummer', infoSerial='Seriennummer', infoHardware='Hardware-Revision', infoMissing='Nicht verfügbar', infoClose='Schließen')
    for language,values in translations.items():
        if language=='en':
            for k,v in values.items(): values[k]=v.replace('Basis station','Base station').replace('basis station','base station')
    html=html[:match.start(1)]+json.dumps(translations,ensure_ascii=False)+html[match.end(1):]
    start=html.index('function createCard(id){');end=html.index('function field(',start)
    html=html[:start]+'''function createCard(id){
 const card=element('article','card');card.id='card-'+id;
 const top=element('div','topline'),name=element('h2','station-name'),badge=element('span','state');name.id='name-'+id;badge.id='state-'+id;top.append(name,badge);card.append(top);
 for(const [cls,prefix]of[['slot','station-id-'],['address','mac-'],['status','status-']]){const node=element('div',cls);node.id=prefix+id;card.append(node);}
 const actions=element('div','actions');for(const[state,key]of[['on','turnOn'],['sleep','sleep']]){const button=trans(element('button',state==='on'?'primary':''),key);button.id=state+'-'+id;button.disabled=true;button.addEventListener('click',()=>power(id,state==='on'?'on':'off'));actions.append(button);}card.append(actions);$('stations').append(card);
}
'''+html[end:]
    change(" const actions=element('div','actions');for(const[state,key]", """ const details=card.querySelector('.status');
 const statusText=element('span');statusText.id='status-text-'+id;
 const info=trans(element('button','info-action'),'stationInfo');info.type='button';info.id='station-info-'+id;info.hidden=true;
 info.addEventListener('click',()=>openStationInfo(id));details.append(statusText,info);
 const actions=element('div','actions');for(const[state,key]""")
    change("$('status-'+s.id).textContent=value;", """$('status-text-'+s.id).textContent=value;
 $('station-info-'+s.id).hidden=!debugEnabled||!s.configured;
 $('station-info-'+s.id).disabled=!bleAvailable||Boolean(data.maintenance)||otaBusy||configSaving||statusNeedsReload;""")
    change("$('status-'+s.id).hidden=false;", "$('status-'+s.id).hidden=!debugEnabled&&!value;")
    change("$('name-'+s.id).textContent=s.name;", "$('card-'+s.id).classList.toggle('debug',debugEnabled);$('station-id-'+s.id).textContent=t('stationId')+' '+s.id;$('station-id-'+s.id).hidden=$('mac-'+s.id).hidden=!debugEnabled;trans($('sleep-'+s.id),s.off_state==='standby'?'standby':'sleep');$('name-'+s.id).textContent=s.name;")
    change("d.alexa_compat=$('edit-alexa-'+d.key).checked;", "d.alexa_compat=$('edit-alexa-'+d.key).checked;d.off_state=$('edit-off-'+d.key).value;")
    change("mac.setAttribute('list','discovered-addresses');section.append(fields);", "section.append(fields);const discovered=element('label','discovered-choice');discovered.append(trans(element('span'),'discoveredLabel'));const choices=element('select');choices.id='edit-discovered-'+d.key;choices.addEventListener('change',()=>{if(choices.value){mac.value=choices.value;d.mac=choices.value;}});discovered.append(choices);section.append(discovered);const offLabel=element('label');offLabel.append(trans(element('span'),'offState'));const off=element('select');off.id='edit-off-'+d.key;off.append(new Option('SLEEP','sleep'),new Option('STANDBY','standby'));off.value=d.off_state||'sleep';offLabel.append(off);section.append(offLabel,trans(element('p','hint'),'offHelp'));")
    change(" $('add-station').disabled=drafts.length>=16;", " updateDiscoveryChoices();$('add-station').disabled=drafts.length>=16;")
    change("  $('scan-results').replaceChildren();", "  updateDiscoveryChoices();$('scan-results').replaceChildren();")
    change('function initForm(stations){', '''function updateDiscoveryChoices(){for(const d of drafts){const select=$('edit-discovered-'+d.key);if(!select)continue;const value=select.value;select.replaceChildren(new Option(t('chooseDiscovered'),''));for(const f of current?.discovered||[])select.append(new Option(f.name+' · '+f.mac+' · '+f.rssi+' dBm',f.mac));if([...select.options].some(o=>o.value===value))select.value=value;select.disabled=select.options.length===1;}}
function initForm(stations){''')
    change('alexa_compat:Boolean(d.alexa_compat)}','alexa_compat:Boolean(d.alexa_compat),off_state:d.off_state||\'sleep\'}')
    change("power('all','sleep')", "power('all','off')")
    change("t('requested')+': '+(station==='all'?t(state):t('station')+' '+station+' · '+t(state))", "t('requested')+': '+(station==='all'?(state==='off'?t('turnOff'):t(state)):t('station')+' '+station+' · '+(state==='off'?t(current?.stations.find(s=>s.id===station)?.off_state||'sleep'):t(state)))")
    change("{password:$('login-password').value}", "{password:$('login-password').value,...(!$('login-username-label').hidden?{username:$('login-username').value}:{})}")
    change("$('login-password').value='';", "$('login-password').value='';$('login-username').value='';")
    change('function showLogin(){', "let loginOptionsEpoch=0;\nasync function loadLoginOptions(){const epoch=++loginOptionsEpoch;try{const info=await api('/api/login');if(epoch!==loginOptionsEpoch||loggedIn)return;$('login-help').hidden=info.password_set!==false;$('login-username-label').hidden=!info.username_enabled;$('login-username').required=!!info.username_enabled;}catch(_){}}\nfunction showLogin(){$('login-help').hidden=true;$('login-username').value='';$('login-username-label').hidden=true;$('login-username').required=false;loadLoginOptions();")
    change("loggedIn=true;$('login-panel')", "loggedIn=true;loginOptionsEpoch++;$('login-panel')")
    change("if(!account.password_required){$('api-key')", "if(!account.password_required){$('username-enabled').checked=!!account.username_enabled;$('web-username').value=account.username||'';renderUsernameSetting();$('api-key')")
    change("$('logout').addEventListener('click'", '''function renderUsernameSetting(){$('username-setting-label').hidden=!$('username-enabled').checked;$('web-username').required=$('username-enabled').checked;}
$('username-enabled').addEventListener('change',renderUsernameSetting);
$('username-form').addEventListener('submit',async event=>{event.preventDefault();const button=event.submitter;try{if(button)button.disabled=true;await api('/api/username',{username_enabled:$('username-enabled').checked,username:$('web-username').value});message(()=>t('usernameSaved'));await loadAccount();}catch(e){message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}finally{if(button)button.disabled=false;}});
$('rotate-key').addEventListener('click',async()=>{if(!confirm(t('keyConfirm')))return;const button=$('rotate-key');button.disabled=true;try{const result=await api('/api/key',{confirm:true});account.api_key=result.api_key;$('api-key').textContent=result.api_key;message(()=>t('keySaved'));}catch(e){message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}finally{button.disabled=false;}});
$('logout').addEventListener('click' ''')
    # The optional account name is cleared with other private page state.
    change("account=null;current=null;", "account=null;current=null;$('web-username').value='';$('username-enabled').checked=false;renderUsernameSetting();")
    change('</main><script>', '</main><dialog id="station-info-dialog" aria-labelledby="station-info-heading"><h2 id="station-info-heading" data-t="stationInfo"></h2><pre id="station-info-data" aria-live="polite"></pre><form method="dialog"><button data-t="infoClose"></button></form></dialog><script>')
    change('function renderDebugMode(){', """let stationInfoEpoch=0,stationInfoView=null;
function renderStationInfo(){
 if(!stationInfoView)return;
 const {info,error}=stationInfoView;
 const fields=[['type','infoType'],['name','infoName'],['firmware','infoFirmware'],['channel','infoChannel'],['manufacturer','infoManufacturer'],['model','infoModel'],['serial','infoSerial'],['hardware','infoHardware']];
 $('station-info-data').textContent=info?fields.map(([key,label])=>t(label)+': '+(info[key]??t('infoMissing'))).join('\\n'):t(error||'stationInfoRead');
}
function closeStationInfo(){
 stationInfoEpoch++;
 stationInfoView=null;
 $('station-info-data').textContent='';
 if($('station-info-dialog').open)$('station-info-dialog').close();
}
$('station-info-dialog').addEventListener('close',()=>{if(!$('station-info-dialog').open)closeStationInfo();});
async function openStationInfo(id){
 const saved=current?.stations.find(s=>s.id===id&&s.configured);
 if(!debugEnabled||!loggedIn||!saved||otaBusy||networkChanging)return;
 const epoch=++stationInfoEpoch;
 const active=()=>epoch===stationInfoEpoch&&$('station-info-dialog').open&&current?.stations.some(s=>s.id===id&&s.mac===saved.mac);
 stationInfoView={};renderStationInfo();$('station-info-dialog').showModal();
 try{
  const {job}=await api('/api/station-info',{id,mac:saved.mac});
  for(let attempt=0;attempt<60&&active();attempt++){
   await new Promise(resolve=>setTimeout(resolve,500));
   if(!active())return;
   const result=await api('/api/station-info');
   if(!active())return;
   if(result.job!==job)break;
   if(result.pending)continue;
   stationInfoView={info:result.info,error:result.error||(!result.info?'STATION_INFO_FAILED':null)};renderStationInfo();return;
  }
  if(active())throw new Error();
 }catch(e){if(active()){stationInfoView={error:e.code||'STATION_INFO_FAILED'};renderStationInfo();}}
}
function renderDebugMode(){if(!debugEnabled)closeStationInfo();else renderStationInfo();""")
    from rs_local_dashboard import local_dashboard
    from rs_buttons_dashboard import buttons_dashboard
    from rs_owner_dashboard import owner_dashboard
    return owner_dashboard(buttons_dashboard(local_dashboard(html)))
