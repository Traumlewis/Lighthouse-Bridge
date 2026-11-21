"""Explicit private GPIO button settings for rs-matter."""
import json
import re


def buttons_dashboard(html):
    from matter_dashboard import replace_once
    html=replace_once(html,'<details id="network-settings">','''<details id="gpio-settings"><summary data-t="gpioTitle"></summary><p data-t="gpioHelp"></p><form id="gpio-form"><div id="gpio-bindings"></div><button type="submit" data-t="gpioSave"></button></form><p data-t="remoteHelp"></p><p id="remote-pins" class="hint"></p><button type="button" id="remote-start" disabled data-t="remoteStart"></button></details><details id="network-settings">''')
    match=re.search(r'const T=(.*?);\n',html)
    translations=json.loads(match[1])
    translations['en'].update(gpioTitle='GPIO buttons & Remote Mode',gpioHelp='Connect momentary buttons between the selected GPIO and GND. Use no external voltage. Disabled by default. Each press queues one action; holding or booting with a button pressed sends no repeated commands. Off uses each station’s SLEEP/STANDBY preference.',gpioSave='Save buttons',gpioButton='Button',gpioDisabled='Disabled',gpioOn='All on',gpioOff='All off',gpioSaved='Button assignments saved.',BUTTON_CONFIG_INVALID='Choose separate supported GPIOs, or disable the button.')
    translations['de'].update(gpioTitle='GPIO-Taster & Remote Mode',gpioHelp='Taster zwischen dem gewählten GPIO und GND anschließen. Keine externe Spannung anlegen. Standardmäßig deaktiviert. Jeder Tastendruck startet einen Auftrag; Gedrückthalten oder ein gedrückter Taster beim Start sendet keine wiederholten Befehle. Aus verwendet die SLEEP/STANDBY-Auswahl jeder Station.',gpioSave='Taster speichern',gpioButton='Taster',gpioDisabled='Deaktiviert',gpioOn='Alle an',gpioOff='Alle aus',gpioSaved='Tasterzuweisungen gespeichert.',BUTTON_CONFIG_INVALID='Verschiedene unterstützte GPIOs wählen oder den Taster deaktivieren.')
    translations['en'].update(remoteHelp='Save one All on and one All off button on the wake pins below. In Remote Mode the bridge wakes on a press, uses Bluetooth to switch the stations, then sleeps again. Hold both buttons for five seconds to return to the previous Wi-Fi mode.', remotePins='Wake pins', remoteStart='Activate Remote Mode', remoteConfirm='Activate Remote Mode? Wi-Fi, the dashboard and Alexa will be unavailable until you hold both GPIO buttons for five seconds.', remoteAccepted='Remote Mode activated. Hold both buttons for five seconds to restore the previous mode.', REMOTE_PINS_INVALID='Save one All on and one All off button on supported wake pins.', REMOTE_CONFIRM_REQUIRED='Confirm entry into Remote Mode.', identify='Identify', identified='Identify sent to the station.', IDENTIFY_UNSUPPORTED='This station does not expose the V2 Identify function.', IDENTIFY_FAILED='Could not identify the station.')
    translations['de'].update(remoteHelp='Speichere je einen Taster für Alle an und Alle aus auf den unten genannten Weck-Pins. Im Remote Mode wacht die Bridge per Tastendruck auf, schaltet die Stationen per Bluetooth und schläft danach wieder. Halte beide Taster fünf Sekunden, um zum vorherigen WLAN-Modus zurückzukehren.', remotePins='Weck-Pins', remoteStart='Remote Mode aktivieren', remoteConfirm='Remote Mode aktivieren? WLAN, Webinterface und Alexa sind erst wieder verfügbar, wenn du beide GPIO-Taster fünf Sekunden hältst.', remoteAccepted='Remote Mode aktiviert. Halte beide Taster fünf Sekunden, um zum vorherigen Modus zurückzukehren.', REMOTE_PINS_INVALID='Speichere je einen Taster für Alle an und Alle aus auf unterstützten Weck-Pins.', REMOTE_CONFIRM_REQUIRED='Bestätige das Aktivieren des Remote Mode.', identify='Identifizieren', identified='Identify an die Basisstation gesendet.', IDENTIFY_UNSUPPORTED='Diese Basisstation bietet die V2-Identify-Funktion nicht an.', IDENTIFY_FAILED='Die Basisstation konnte nicht identifiziert werden.')
    html=replace_once(html, 'heading.append(remove);section.append(heading);', """const actions=element('div','config-actions');
  const identify=trans(element('button'),'identify');identify.type='button';identify.dataset.identify=d.key;
  identify.addEventListener('click',()=>identifyStation(d,identify));
  actions.append(identify,remove);heading.append(actions);section.append(heading);""")
    html=replace_once(html, "mac.autocomplete='off';", """mac.autocomplete='off';
  const refreshIdentify=()=>{identify.disabled=!savedIdentifyAddress(d.id,mac.value);};
  mac.addEventListener('input',refreshIdentify);refreshIdentify();""")
    html=replace_once(html, "mac.value=choices.value;d.mac=choices.value;", "mac.value=choices.value;d.mac=choices.value;refreshIdentify();")
    html=html[:match.start(1)]+json.dumps(translations,ensure_ascii=False)+html[match.end(1):]
    script='''
let gpioEpoch=0, wakePins=[], savedGpio=[], identifyBusy=false;
function savedIdentifyAddress(id,address){
 const saved=current?.stations.find(station=>station.id===id)?.mac;
 return saved&&address.trim().toUpperCase()===saved.toUpperCase()?saved:null;
}
async function identifyStation(d,button){
 if(identifyBusy||networkChanging)return;
 const mac=savedIdentifyAddress(d.id,$('edit-mac-'+d.key).value);
 if(!mac)return;
 identifyBusy=true;button.disabled=true;
 try{
  const {job}=await api('/api/identify',{id:d.id,mac});
  for(let attempt=0;attempt<30;attempt++){
   await new Promise(resolve=>setTimeout(resolve,500));
   if(networkChanging||!loggedIn)return;
   const result=await api('/api/identify');
   if(result.job!==job)break;
   if(result.pending)continue;
   message(()=>t(result.error||'identified'),Boolean(result.error));
   return;
  }
  message(()=>t('IDENTIFY_FAILED'),true);
 }catch(e){message(()=>t(e.code||'NETWORK_ERROR'),true);}
 finally{identifyBusy=false;if(!networkChanging)button.disabled=!savedIdentifyAddress(d.id,$('edit-mac-'+d.key)?.value||'');}
}
function renderRemote(){
 const valid=savedGpio.length===2&&savedGpio.every(b=>wakePins.includes(b.pin))&&
   savedGpio[0].pin!==savedGpio[1].pin&&['all_on','all_off'].every(action=>savedGpio.some(b=>b.action===action));
 $('remote-start').disabled=!valid||networkChanging;
 $('remote-pins').textContent=wakePins.length?t('remotePins')+': '+wakePins.join(', '):'';
}
$('remote-start').addEventListener('click',async()=>{
 if(networkChanging||!confirm(t('remoteConfirm')))return;
 $('remote-start').disabled=true;
 try{await api('/api/remote',{confirm:true});holdNetworkRestart('remoteAccepted');}
 catch(e){message(()=>t(e.code||'NETWORK_ERROR'),true);renderRemote();}
});
$('gpio-settings').addEventListener('toggle',async()=>{
 const epoch=++gpioEpoch;
 if(!$('gpio-settings').open||!loggedIn)return;
 try{const data=await api('/api/buttons');if(epoch!==gpioEpoch||!loggedIn||!$('gpio-settings').open)return;
 wakePins=data.wake_pins||[];savedGpio=data.buttons;renderRemote();
 const container=$('gpio-bindings');container.replaceChildren();
 data.buttons.forEach((binding,index)=>{
  const group=document.createElement('fieldset'),legend=document.createElement('legend');const label=document.createElement('span');label.dataset.t='gpioButton';label.textContent=t('gpioButton');legend.append(label,document.createTextNode(' '+(index+1)));group.append(legend);
  const pin=document.createElement('select');pin.id='gpio-pin-'+index;pin.setAttribute('aria-label',t('gpioButton')+' '+(index+1)+' GPIO');
  for(const number of [-1,...data.pins]){const option=document.createElement('option');option.value=number;option.textContent=number<0?t('gpioDisabled'):'GPIO '+number;if(number<0)option.dataset.t='gpioDisabled';pin.append(option);}pin.value=binding.pin;
  const action=document.createElement('select');action.id='gpio-action-'+index;action.setAttribute('aria-label',t('gpioButton')+' '+(index+1));
  for(const [value,key]of [['disabled','gpioDisabled'],['all_on','gpioOn'],['all_off','gpioOff']]){const option=document.createElement('option');option.value=value;option.dataset.t=key;option.textContent=t(key);action.append(option);}action.value=binding.action;
  group.append(pin,action);container.append(group);
 });
 }catch(e){message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}
});
$('gpio-form').addEventListener('submit',async event=>{
 event.preventDefault();const button=event.submitter;
 try{if(button)button.disabled=true;const buttons=[0,1].map(i=>{const action=$('gpio-action-'+i).value;return {pin:action==='disabled'?-1:Number($('gpio-pin-'+i).value),action};});await api('/api/buttons',{buttons});savedGpio=buttons;renderRemote();message(()=>t('gpioSaved'));}
 catch(e){message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}finally{if(button)button.disabled=false;}
});
'''
    html=replace_once(html,"account=null;current=null;", "account=null;current=null;gpioEpoch++;wakePins=[];savedGpio=[];renderRemote();$('gpio-settings').open=false;$('gpio-bindings').replaceChildren();")
    html=replace_once(html,'</style>','.config-actions{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:6px}.config-actions button{padding-inline:10px}#gpio-bindings{display:grid;gap:14px}#gpio-bindings fieldset{min-width:0;border:1px solid var(--line);border-radius:10px;padding:14px;display:grid;gap:10px}#gpio-bindings select{width:100%;max-width:100%;min-width:0}</style>')
    html=replace_once(html,'renderDebugMode();renderMatter();','renderDebugMode();renderMatter();renderRemote();')
    return replace_once(html,"setLanguage(requested===",script+"setLanguage(requested===")
