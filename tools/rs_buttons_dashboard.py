"""Explicit private GPIO button settings for rs-matter."""
import json
import re


def buttons_dashboard(html):
    from matter_dashboard import replace_once
    html=replace_once(html,'<details id="network-settings">','''<details id="gpio-settings"><summary data-t="gpioTitle"></summary><p data-t="gpioHelp"></p><form id="gpio-form"><div id="gpio-bindings"></div><button type="submit" data-t="gpioSave"></button></form></details><details id="network-settings">''')
    match=re.search(r'const T=(.*?);\n',html)
    translations=json.loads(match[1])
    translations['en'].update(gpioTitle='GPIO buttons',gpioHelp='Connect momentary buttons between the selected GPIO and GND. Use no external voltage. Disabled by default. Each press queues one action; holding or booting with a button pressed sends no repeated commands. Off uses each station’s SLEEP/STANDBY preference.',gpioSave='Save buttons',gpioButton='Button',gpioDisabled='Disabled',gpioOn='All on',gpioOff='All off',gpioSaved='Button assignments saved.',BUTTON_CONFIG_INVALID='Choose separate supported GPIOs, or disable the button.')
    translations['de'].update(gpioTitle='GPIO-Taster',gpioHelp='Taster zwischen dem gewählten GPIO und GND anschließen. Keine externe Spannung anlegen. Standardmäßig deaktiviert. Jeder Tastendruck startet einen Auftrag; Gedrückthalten oder ein gedrückter Taster beim Start sendet keine wiederholten Befehle. Aus verwendet die SLEEP/STANDBY-Auswahl jeder Station.',gpioSave='Taster speichern',gpioButton='Taster',gpioDisabled='Deaktiviert',gpioOn='Alle an',gpioOff='Alle aus',gpioSaved='Tasterzuweisungen gespeichert.',BUTTON_CONFIG_INVALID='Verschiedene unterstützte GPIOs wählen oder den Taster deaktivieren.')
    html=html[:match.start(1)]+json.dumps(translations,ensure_ascii=False)+html[match.end(1):]
    script='''
let gpioEpoch=0;
$('gpio-settings').addEventListener('toggle',async()=>{
 const epoch=++gpioEpoch;
 if(!$('gpio-settings').open||!loggedIn)return;
 try{const data=await api('/api/buttons');if(epoch!==gpioEpoch||!loggedIn||!$('gpio-settings').open)return;
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
 try{if(button)button.disabled=true;const buttons=[0,1].map(i=>{const action=$('gpio-action-'+i).value;return {pin:action==='disabled'?-1:Number($('gpio-pin-'+i).value),action};});await api('/api/buttons',{buttons});message(()=>t('gpioSaved'));}
 catch(e){message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}finally{if(button)button.disabled=false;}
});
'''
    html=replace_once(html,"account=null;current=null;", "account=null;current=null;gpioEpoch++;$('gpio-settings').open=false;$('gpio-bindings').replaceChildren();")
    html=replace_once(html,'</style>','#gpio-bindings{display:grid;gap:14px}#gpio-bindings fieldset{min-width:0;border:1px solid var(--line);border-radius:10px;padding:14px;display:grid;gap:10px}#gpio-bindings select{width:100%;max-width:100%;min-width:0}</style>')
    return replace_once(html,'</script>',script+'</script>')
