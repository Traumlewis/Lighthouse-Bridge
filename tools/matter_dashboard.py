"""Dashboard behavior shared by the independent Matter implementation."""
import json
import re

def replace_once(source,old,new):
    if source.count(old)!=1: raise ValueError("Dashboard boundary changed")
    return source.replace(old,new,1)

def rotation_dashboard(html):
    import re
    match=re.search(r'const T=(.*?);\n',html)
    translations=json.loads(match[1])
    translations['en'].update(matterOpen='New code & open pairing window (15 minutes)',
        matterHelp='Generate a new code and open a pairing window before adding this bridge in Alexa. Each manual activation replaces the previous window code. Existing pairings stay connected. Codes expire with the window and after restart.',
        matterWindowPending='Generating a new setup code and opening the pairing window…',
        matterLoading='Loading pairing status…',
        matterWindowClosed='No active setup code. Use New code & open pairing window before adding the bridge in Alexa.',
        MATTER_CODE_UNAVAILABLE='No code is available for this window. Generate a new code and open the pairing window.')
    translations['de'].update(matterOpen='Neuer Code & Kopplungsfenster öffnen (15 Minuten)',
        matterHelp='Erzeuge vor dem Hinzufügen in Alexa einen neuen Code und öffne das Kopplungsfenster. Jede manuelle Aktivierung ersetzt den vorherigen Fenstercode. Bestehende Kopplungen bleiben verbunden. Codes gelten nur bis zum Schließen des Fensters oder Neustart.',
        matterWindowPending='Neuer Einrichtungscode wird erzeugt und das Kopplungsfenster geöffnet…',
        matterLoading='Kopplungsstatus wird geladen …',
        matterWindowClosed='Kein aktiver Einrichtungscode. Nutze vor dem Hinzufügen in Alexa Neuer Code & Kopplungsfenster öffnen.',
        MATTER_CODE_UNAVAILABLE='Für dieses Fenster ist kein Code verfügbar. Erzeuge einen neuen Code und öffne das Kopplungsfenster.')
    for language in ['en','de']:
        translations[language].pop('matterShow',None)
    html=html[:match.start(1)]+json.dumps(translations,ensure_ascii=False)+html[match.end(1):]
    html=replace_once(html,'<button id="matter-show" data-t="matterShow">Show setup code</button>','')
    html=replace_once(html,"!matterInfo.pairing_code?'MATTER_CODE_UNAVAILABLE'", "matterInfo.commissioning_open&&!matterInfo.pairing_code?'MATTER_CODE_UNAVAILABLE'")
    html=replace_once(html,'async function loadMatter(open=false){','async function loadMatter(open=false,background=false){')
    html=replace_once(html,'if(matterBusy)return;const epoch=matterEpoch;matterBusy=true;renderMatter();',
        "if(!open&&!$('matter-settings').open)return;"
        'if(matterBusy){if(!background){loadMatter.queued=Boolean(open||loadMatter.queued);renderMatter();}return;}'
        'clearTimeout(loadMatter.timer);const epoch=matterEpoch;'
        'if(!background&&!open)matterInfo=null;'
        'matterBusy=true;loadMatter.background=background;renderMatter();')
    html=replace_once(html,"  if(open)await api('/api/matter/commissioning',{});",
        "  if(open){if(matterInfo)matterInfo={...matterInfo,pairing_code:null,commissioning_pending:true};renderMatter();await api('/api/matter/commissioning',{});}")
    html=replace_once(html,'  if(info.commissioning_pending)setTimeout(()=>{if(epoch===matterEpoch&&loggedIn)loadMatter();},750);',
        "  if($('matter-settings').open&&(info.commissioning_pending||info.commissioning_open))loadMatter.timer=setTimeout(()=>{if(epoch===matterEpoch&&loggedIn)loadMatter(false,true);},info.commissioning_pending?750:2000);")
    html=replace_once(html,'matterInfo=info;renderMatter();',
        "matterInfo=$('matter-settings').open?info:null;renderMatter();")
    html=replace_once(html," $('matter-show').disabled=matterBusy;$('matter-open').disabled=matterBusy||",
        " const controlsBusy=(matterBusy&&!loadMatter.background)||loadMatter.queued!==undefined;"
        "$('matter-open').disabled=controlsBusy||!!matterInfo?.commissioning_pending||")
    html=replace_once(html,'matterBusy=false;renderMatter();',
        'matterBusy=false;loadMatter.background=false;renderMatter();'
        'if(loadMatter.queued!==undefined){const next=loadMatter.queued;delete loadMatter.queued;loadMatter(next);}')
    html=replace_once(html,"}catch(e){if(epoch===matterEpoch)message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}",
        "}catch(e){if(epoch===matterEpoch){matterInfo=null;$('matter-code').textContent='';$('matter-code').hidden=true;$('matter-status').textContent='';message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}}")
    html=replace_once(html,'function showLogin(){matterEpoch++;',"function showLogin(){$('matter-settings').open=false;clearTimeout(loadMatter.timer);delete loadMatter.queued;loadMatter.background=false;matterEpoch++;")
    html=replace_once(html,'function renderMatter(){',
        "function setMatterText(id,value){const node=$(id);if(node.textContent!==value)node.textContent=value;}\nfunction renderMatter(){")
    html=replace_once(html," $('matter-status').textContent=[t(matterInfo.paired?'matterPaired':'matterUnpaired'),t(matterInfo.commissioning_pending?'matterWindowPending':matterInfo.commissioning_open?'matterWindowOpen':'matterWindowClosed'),problem?t(problem):''].filter(Boolean).join(' ');",
        " setMatterText('matter-status',[t(matterInfo.paired?'matterPaired':'matterUnpaired'),t(matterInfo.commissioning_pending?'matterWindowPending':matterInfo.commissioning_open?'matterWindowOpen':'matterWindowClosed'),problem?t(problem):''].filter(Boolean).join(' '));")
    html=replace_once(html,"$('matter-name').textContent=matterInfo.bridge_name||'';", "setMatterText('matter-name',matterInfo.bridge_name||'');")
    html=replace_once(html,"$('matter-code').textContent=matterInfo.pairing_code||'';", "setMatterText('matter-code',matterInfo.pairing_code||'');")
    html=replace_once(html,'if(!matterInfo)return;',
        "if(!matterInfo){setMatterText('matter-code','');$('matter-code').hidden=true;"
        "setMatterText('matter-name','');setMatterText('matter-status',matterBusy&&$('matter-settings').open?t('matterLoading'):'');return;}")
    # Opening the section is an explicit private read, never a commissioning POST.
    # Closing it stops polling and clears the code; accepted renewals still finish.
    html=replace_once(html,"$('matter-show').addEventListener('click',()=>loadMatter());",
        "$('matter-settings').addEventListener('toggle',()=>{"
        "if(!$('matter-settings').open){clearTimeout(loadMatter.timer);matterInfo=null;renderMatter();return;}"
        "if(loggedIn&&!account?.password_required)loadMatter();});")
    return presentation_dashboard(html)


def presentation_dashboard(html):
    """Apply the owner's rs-matter layout, measured badge colors and OTA reload."""
    version=re.search(r'id="version-toggle"[^>]*>Bridge ([^<]+)</button>',html)[1]
    html=replace_once(html,'async function api(path,data){',
        'const pageFirmwareVersion='+json.dumps(version)+';\n'
        "async function checkFirmwarePage(){const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),8000);"
        "try{const response=await fetch('/',{cache:'no-store',credentials:'same-origin',signal:controller.signal});"
        "if(!response.ok)return;const doc=new DOMParser().parseFromString(await response.text(),'text/html');"
        "const version=doc.getElementById('version-toggle')?.textContent;"
        "if(version&&version!=='Bridge '+pageFirmwareVersion)location.reload();}catch(_){}finally{clearTimeout(timer);}}\n"
        'async function api(path,data){')
    html=replace_once(html,"if(response.status===401&&path!=='/api/login')showLogin();",
        "if(response.status===401&&path!=='/api/login'){const hadSession=loggedIn;showLogin();"
        "if(hadSession&&!otaBusy)checkFirmwarePage();}")
    html=replace_once(html,'function render(data){',
        "function render(data){if(data.version&&data.version!==pageFirmwareVersion){location.reload();return;}")
    pill='<span class="pill" id="connection" role="status">Sign in</span>'
    html=replace_once(html,pill,'')
    html=replace_once(html,'<header><div class="brand">',
        '<header><div class="header-brand">'+pill+'<div class="brand">')
    html=replace_once(html,'Lighthouse Bridge</div><div class="header-actions">',
        'Lighthouse Bridge</div></div><div class="header-actions">')
    html=replace_once(html,'</style>',
        '.header-brand{display:flex;align-items:center;gap:12px;max-width:100%;flex-wrap:wrap}'
        '.header-brand .brand{white-space:nowrap}.header-brand .pill{flex-shrink:0}'
        '@media(max-width:600px){.header-brand{gap:8px;flex-wrap:nowrap}'
        '.header-brand .brand{font-size:17px;white-space:normal;min-width:0}'
        '.header-brand .pill{padding:6px 10px}}'
        '.state[data-state="unknown"]{background:#343c43;color:#c6cdd4}'
        '.state[data-state="sleep"]{background:#163951;color:#8dd2ff}'
        '.state[data-state="standby"]{background:#49331e;color:#ffbe78}</style>')
    html=replace_once(html,"$('state-'+s.id).title=$('state-'+s.id).textContent;",
        "$('state-'+s.id).dataset.state=s.reachable?s.observed:'unknown';"
        "$('state-'+s.id).title=$('state-'+s.id).textContent;")
    match=re.search(r'const T=(.*?);\n',html)
    translations=json.loads(match[1])
    translations['en']['otaReconnectFailed']='Update installed. The bridge has not responded yet. Reload this page once it is reachable.'
    translations['de']['otaReconnectFailed']='Update installiert. Die Bridge antwortet noch nicht. Lade die Seite neu, sobald sie erreichbar ist.'
    html=html[:match.start(1)]+json.dumps(translations,ensure_ascii=False)+html[match.end(1):]
    # Sessions are lost on reboot. A still-authenticated old server is not yet
    # proof that the new image is ready; reload only on a reachable login gate.
    html=replace_once(html,"$('ota-form').addEventListener('submit',async event=>{",
        "async function waitForOtaRestart(){const deadline=Date.now()+90000;"
        "while(Date.now()<deadline){await new Promise(resolve=>setTimeout(resolve,2000));"
        "const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),3000);"
        "try{const response=await fetch('/api/account',{credentials:'same-origin',cache:'no-store',signal:controller.signal});"
        "if(response.status===401){location.reload();return;}await response.arrayBuffer();}catch(_){}finally{clearTimeout(timer);}}"
        "$('ota-status').textContent=t('otaReconnectFailed');message(()=>t('otaReconnectFailed'));}\n"
        "$('ota-form').addEventListener('submit',async event=>{")
    html=replace_once(html,"if($('matter-settings').open&&(info.commissioning_pending||info.commissioning_open))",
        "if(!otaBusy&&$('matter-settings').open&&(info.commissioning_pending||info.commissioning_open))")
    html=replace_once(html,'const controlsBusy=(matterBusy&&!loadMatter.background)||',
        'const controlsBusy=otaBusy||(matterBusy&&!loadMatter.background)||')
    html=replace_once(html,"message(()=>t('otaDone'));prepared=false;",
        "message(()=>t('otaDone'));prepared=false;clearTimeout(loadMatter.timer);waitForOtaRestart();")
    html=replace_once(html,'otaBusy=false;if(current)render(current);',
        "otaBusy=false;if(current)render(current);if($('matter-settings').open)loadMatter(false,true);")
    from rs_dashboard import preferences_dashboard
    return preferences_dashboard(html)
