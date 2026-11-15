"""Local access-point presentation and explicit return to the saved home network."""
import json
import re


def local_dashboard(html):
    from matter_dashboard import replace_once
    def change(old,new):
        nonlocal html
        html=replace_once(html,old,new)
    change('let loginOptionsEpoch=0;', 'let networkChanging=false;\nlet loginOptionsEpoch=0;')
    change("if(response.status===401&&path!=='/api/login')", "if(response.status===401&&path!=='/api/login'&&!networkChanging)")
    change('||polling||configSaving||otaBusy)return;', '||polling||configSaving||otaBusy||networkChanging)return;')
    change("$('connection').textContent=t(data.wifi_connected?'online':'wifiLost');", "$('connection').textContent=t(data.network_mode==='local'?'localOnline':data.wifi_connected?'online':'wifiLost');$('home-network').hidden=data.network_mode!=='local';document.querySelector('[data-t=wifiHelp]').textContent=t(data.network_mode==='local'?'localWifiHelp':'wifiHelp');document.querySelector('[data-t=apiKey]').textContent=t(data.network_mode==='local'?'localApiKey':'apiKey');")
    change("$('device-address').textContent=data.ip+' · '+data.hostname+'.local';", "$('device-address').textContent=data.network_mode==='local'?data.ip+' · '+t('localOnline'):data.ip+' · '+data.hostname+'.local';")
    change('<button id="wifi" data-t="wifiButton"></button>', '<button id="wifi" data-t="wifiButton"></button><button id="home-network" hidden data-t="returnHome"></button>')
    change("if(!matterInfo){setMatterText", "if((current?.network_mode||account?.network_mode)==='local'){setMatterText('matter-status',t('localMatter'));setMatterText('matter-name','');setMatterText('matter-code','');$('matter-code').hidden=true;$('matter-open').disabled=true;return;}\n if(!matterInfo){setMatterText")
    change("t(data.alexa_protocol==='matter'?'matterAlexaHelp':'alexaHelp')", "t(data.network_mode==='local'?'localMatter':data.alexa_protocol==='matter'?'matterAlexaHelp':'alexaHelp')")
    change("if(!confirm(t('keyConfirm')))return;", "if(!confirm(t('keyConfirm')+(current?.network_mode==='local'?' '+t('localKeyConfirm'):'')))return;")
    change("message(()=>t('keySaved'));", "if(result.restarting)holdNetworkRestart('localKeySaved');else message(()=>t('keySaved'));")
    change("message(()=>t('deviceSaved')+result.url);await refresh();", "if(result.restarting)holdNetworkRestart('localNameSaved');else {message(()=>t('deviceSaved')+result.url);await refresh();}")
    change("async()=>{try{await api('/api/logout',{});", "async()=>{if(networkChanging){showLogin();return;}try{await api('/api/logout',{});")
    change("message(()=>t('wifiStarted'));clearInterval(pollTimer);", "holdNetworkRestart('wifiStarted');")
    change("$('wifi').addEventListener('click'", '''function holdNetworkRestart(key){networkChanging=true;clearInterval(pollTimer);clearTimeout(loadMatter.timer);message(()=>t(key));for(const node of document.querySelectorAll('button,input,select'))if(!['copy-key','logout'].includes(node.id))node.disabled=true;}
$('home-network').addEventListener('click',async()=>{if(!confirm(t('returnHomeConfirm')))return;try{const result=await api('/api/network',{mode:'home'});if(result.restarting)holdNetworkRestart('returnHomeSaved');}catch(e){message(()=>e.code?t(e.code):t('NETWORK_ERROR'),true);}});
$('wifi').addEventListener('click' ''')
    match=re.search(r'const T=(.*?);\n',html);translations=json.loads(match[1])
    translations['en'].update(wifiConfirm='Restart the bridge to open protected Wi-Fi setup? Control pauses during setup.',wifiStarted='The bridge is restarting for Wi-Fi setup. Connect to its setup Wi-Fi using the API key, then open http://192.168.4.1/.',localOnline='Local Wi-Fi',localMatter='Matter is paused in Local Mode. Return to your home Wi-Fi to use Alexa and existing pairings.',
        localWifiHelp='Connected directly to the bridge, without a router. The dashboard is at http://192.168.4.1/. Stay connected if your phone reports no internet. Wi-Fi setup lets you join a home network or select Local Mode again.',
        localApiKey='API key / bridge Wi-Fi password',returnHome='Return to home Wi-Fi',returnHomeConfirm='Restart in home-network mode? Connect your phone back to your home Wi-Fi afterwards. Saved Wi-Fi credentials and Matter pairings stay intact. If no network connects, the Wi-Fi setup portal opens.',
        returnHomeSaved='The bridge is restarting in home-network mode. Reconnect to your home Wi-Fi, then open the bridge at its home address. If needed, connect to its setup Wi-Fi to configure the home network.',
        wifiHelp='Open the protected Wi-Fi setup to change your home network or choose Local Mode (own Wi-Fi without a router). Control pauses during setup. Alternatively hold BOOT for five seconds.',
        localKeyConfirm='In Local Mode the bridge also restarts to apply its new Wi-Fi password. Copy the new key before reconnecting.',
        localKeySaved='New key saved. Copy it now: it is the new bridge Wi-Fi password. Reconnect to the bridge with it, then reload this page. Your web password stays unchanged.',
        localNameSaved='Wi-Fi name saved. The bridge is restarting. Reconnect to the renamed bridge Wi-Fi, then open http://192.168.4.1/.',
        LOCAL_MODE='Matter is paused in Local Mode.',NETWORK_MODE_INVALID='Choose home-network mode.')
    translations['de'].update(wifiConfirm='Bridge neu starten und geschütztes WLAN-Setup öffnen? Die Steuerung pausiert während des Setups.',wifiStarted='Die Bridge startet für das WLAN-Setup neu. Verbinde dich mit ihrem Setup-WLAN und dem API-Schlüssel als Passwort. Öffne anschließend http://192.168.4.1/.',localOnline='Lokales WLAN',localMatter='Matter ist im Local Mode pausiert. Kehre ins Heim-WLAN zurück, um Alexa und bestehende Kopplungen zu verwenden.',
        localWifiHelp='Direkt mit der Bridge verbunden, ohne Router. Das Webinterface ist unter http://192.168.4.1/ erreichbar. Bleibe verbunden, wenn dein Handy kein Internet meldet. Im WLAN-Setup kannst du ein Heimnetz einrichten oder Local Mode erneut wählen.',
        localApiKey='API-Schlüssel / Passwort des Bridge-WLANs',returnHome='Zurück ins Heim-WLAN',returnHomeConfirm='Im Heimnetz-Modus neu starten? Verbinde dein Handy danach wieder mit dem Heim-WLAN. Gespeicherte WLAN-Daten und Matter-Kopplungen bleiben erhalten. Ohne erreichbares Heimnetz öffnet sich das WLAN-Setup.',
        returnHomeSaved='Die Bridge startet im Heimnetz-Modus neu. Verbinde dich wieder mit dem Heim-WLAN und öffne die Heimnetz-Adresse der Bridge. Verbinde dich bei Bedarf mit ihrem Setup-WLAN, um das Heimnetz einzurichten.',
        wifiHelp='Öffne das geschützte WLAN-Setup, um dein Heimnetz zu ändern oder Local Mode zu wählen (eigenes WLAN ohne Router). Die Steuerung pausiert während des Setups. Alternativ BOOT fünf Sekunden halten.',
        localKeyConfirm='Im Local Mode startet die Bridge zusätzlich neu, um das neue WLAN-Passwort zu übernehmen. Kopiere den neuen Schlüssel vor dem erneuten Verbinden.',
        localKeySaved='Neuer Schlüssel gespeichert. Jetzt kopieren: Er ist das neue Passwort des Bridge-WLANs. Verbinde dich damit erneut mit der Bridge und lade die Seite neu. Dein Web-Passwort bleibt unverändert.',
        localNameSaved='WLAN-Name gespeichert. Die Bridge startet neu. Verbinde dich mit dem umbenannten Bridge-WLAN und öffne http://192.168.4.1/.',
        LOCAL_MODE='Matter ist im Local Mode pausiert.',NETWORK_MODE_INVALID='Wähle den Heimnetz-Modus.')
    return html[:match.start(1)]+json.dumps(translations,ensure_ascii=False)+html[match.end(1):]
