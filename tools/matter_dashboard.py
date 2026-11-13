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
        matterShow='Show active code / pairing status',
        matterWindowClosed='No active setup code. Use New code & open pairing window before adding the bridge in Alexa.',
        MATTER_CODE_UNAVAILABLE='No code is available for this window. Generate a new code and open the pairing window.')
    translations['de'].update(matterOpen='Neuer Code & Kopplungsfenster öffnen (15 Minuten)',
        matterHelp='Erzeuge vor dem Hinzufügen in Alexa einen neuen Code und öffne das Kopplungsfenster. Jede manuelle Aktivierung ersetzt den vorherigen Fenstercode. Bestehende Kopplungen bleiben verbunden. Codes gelten nur bis zum Schließen des Fensters oder Neustart.',
        matterWindowPending='Neuer Einrichtungscode wird erzeugt und das Kopplungsfenster geöffnet…',
        matterShow='Aktiven Code / Kopplungsstatus anzeigen',
        matterWindowClosed='Kein aktiver Einrichtungscode. Nutze vor dem Hinzufügen in Alexa Neuer Code & Kopplungsfenster öffnen.',
        MATTER_CODE_UNAVAILABLE='Für dieses Fenster ist kein Code verfügbar. Erzeuge einen neuen Code und öffne das Kopplungsfenster.')
    html=html[:match.start(1)]+json.dumps(translations,ensure_ascii=False)+html[match.end(1):]
    html=replace_once(html,"!matterInfo.pairing_code?'MATTER_CODE_UNAVAILABLE'", "matterInfo.commissioning_open&&!matterInfo.pairing_code?'MATTER_CODE_UNAVAILABLE'")
    html=replace_once(html,'async function loadMatter(open=false){','async function loadMatter(open=false,background=false){')
    html=replace_once(html,'if(matterBusy)return;const epoch=matterEpoch;matterBusy=true;renderMatter();',
        'if(matterBusy){if(!background&&loadMatter.background){loadMatter.queued=Boolean(open||loadMatter.queued);renderMatter();}return;}'
        'clearTimeout(loadMatter.timer);const epoch=matterEpoch;matterBusy=true;loadMatter.background=background;renderMatter();')
    html=replace_once(html,"  if(open)await api('/api/matter/commissioning',{});",
        "  if(open){if(matterInfo)matterInfo={...matterInfo,pairing_code:null,commissioning_pending:true};renderMatter();await api('/api/matter/commissioning',{});}")
    html=replace_once(html,'  if(info.commissioning_pending)setTimeout(()=>{if(epoch===matterEpoch&&loggedIn)loadMatter();},750);',
        '  if(info.commissioning_pending||info.commissioning_open)loadMatter.timer=setTimeout(()=>{if(epoch===matterEpoch&&loggedIn)loadMatter(false,true);},info.commissioning_pending?750:2000);')
    html=replace_once(html," $('matter-show').disabled=matterBusy;$('matter-open').disabled=matterBusy||",
        " const controlsBusy=(matterBusy&&!loadMatter.background)||loadMatter.queued!==undefined;"
        "$('matter-show').disabled=controlsBusy;$('matter-open').disabled=controlsBusy||!!matterInfo?.commissioning_pending||")
    html=replace_once(html,'matterBusy=false;renderMatter();',
        'matterBusy=false;loadMatter.background=false;renderMatter();'
        'if(loadMatter.queued!==undefined){const next=loadMatter.queued;delete loadMatter.queued;loadMatter(next);}')
    html=replace_once(html,"}catch(e){if(epoch===matterEpoch)message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}",
        "}catch(e){if(epoch===matterEpoch){matterInfo=null;$('matter-code').textContent='';$('matter-code').hidden=true;$('matter-status').textContent='';message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}}")
    html=replace_once(html,'function showLogin(){matterEpoch++;','function showLogin(){clearTimeout(loadMatter.timer);delete loadMatter.queued;loadMatter.background=false;matterEpoch++;')
    html=replace_once(html,'function renderMatter(){',
        "function setMatterText(id,value){const node=$(id);if(node.textContent!==value)node.textContent=value;}\nfunction renderMatter(){")
    html=replace_once(html," $('matter-status').textContent=[t(matterInfo.paired?'matterPaired':'matterUnpaired'),t(matterInfo.commissioning_pending?'matterWindowPending':matterInfo.commissioning_open?'matterWindowOpen':'matterWindowClosed'),problem?t(problem):''].filter(Boolean).join(' ');",
        " setMatterText('matter-status',[t(matterInfo.paired?'matterPaired':'matterUnpaired'),t(matterInfo.commissioning_pending?'matterWindowPending':matterInfo.commissioning_open?'matterWindowOpen':'matterWindowClosed'),problem?t(problem):''].filter(Boolean).join(' '));")
    html=replace_once(html,"$('matter-name').textContent=matterInfo.bridge_name||'';", "setMatterText('matter-name',matterInfo.bridge_name||'');")
    html=replace_once(html,"$('matter-code').textContent=matterInfo.pairing_code||'';", "setMatterText('matter-code',matterInfo.pairing_code||'');")
    return html
