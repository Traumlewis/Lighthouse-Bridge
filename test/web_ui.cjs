// Embedded firmware HTML, with HTTP/serial/radio represented by a mock backend.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs');const path=require('node:path');
const root=path.resolve(__dirname,'..');
const html=fs.readFileSync(path.join(root,'include/web_ui.h'),'utf8').split('R"LHHTML(')[1].split(')LHHTML"')[0];
(async()=>{
 const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});
 try{
  const page=await browser.newPage({viewport:{width:1280,height:1100}}),errors=[];
  page.on('pageerror',error=>errors.push(error.message));
  const key='1234567890abcdef12345678',password='my own bridge password';
  let nextId=6,otaPrepared=false,otaInstalled=false,otaInvalid=false;
  let signedIn=false,passwordSet=false,saveFailure=false,offline=false,refreshes=0;
  const commands=[];
  const stations=Array.from({length:4},(_,i)=>({id:i+1,name:'Base station '+(i+1),mac:'',address_type:1,configured:false,requested:null,pending:false,busy:false,reading:false,reachable:false,command_sent:false,observed:'unknown',observed_raw:-1,observed_ms:0,error:''}));
  const data={ip:'192.168.1.42',hostname:'lighthouse-example',wifi_connected:true,alexa_started:true,uptime_ms:90000,scanning:false,scan_finished_ms:0,scan_truncated:false,stations,discovered:[]};
  await page.addInitScript(()=>{window.copiedValue='';document.execCommand=command=>{if(command==='copy'){window.copiedValue=document.activeElement.value;return true;}return false;};});
  await page.route('http://bridge.test/**',async route=>{
   const req=route.request(),uri=new URL(req.url()).pathname;
   if(uri==='/')return route.fulfill({contentType:'text/html',body:html});
   if(offline)return route.abort('connectionfailed');
   const body=req.method()==='POST'&&uri!=='/api/ota'?req.postDataJSON():null;
   let status=200,result={};
   if(uri==='/api/login'){
    if(body.password===(passwordSet?password:key)){signedIn=true;result={password_required:!passwordSet};}
    else{status=401;result={error:'LOGIN_FAILED'};}
   }else if(!signedIn){status=401;result={error:'LOGIN_REQUIRED'};}
   else if(uri==='/api/account')result={password_required:!passwordSet,hostname:data.hostname,device_name:'',setup_ssid:'Lighthouse-Setup-EXAMPLE',...(passwordSet?{api_key:key}:{})};
   else if(uri==='/api/password'){
    if(body.password===key){status=400;result={error:'PASSWORD_MUST_DIFFER'};}
    else{assert.equal(body.password,password);passwordSet=true;result={saved:true};}
   }else if(uri==='/api/logout'){signedIn=false;result={logged_out:true};}
   else if(!passwordSet){status=428;result={error:'PASSWORD_REQUIRED'};}
   else if(uri==='/api/status')result=data;
   else if(uri==='/api/refresh'){refreshes++;status=202;result={accepted:true};}
   else if(uri==='/api/scan'){
    status=202;result={accepted:true};data.scan_finished_ms=88000;
    data.discovered=stations.map((s,i)=>({name:i===0?'LHB-<img src=x onerror=alert(1)>':'LHB-'+i,mac:'AA:BB:CC:DD:EE:0'+(i+1),address_type:i%2,rssi:-40-i*5}));
   }else if(uri==='/api/config'){
    if(saveFailure){status=409;result={error:'BLE_BUSY'};}
    else{const saved=body.stations.map((s,i)=>({...stations.find(old=>old.id===s.id),...s,id:i+1,hue_id:stations.find(old=>old.id===s.id)?.hue_id||s.id||nextId++,configured:Boolean(s.mac)}));stations.splice(0,stations.length,...saved);result={saved:true,stations};await new Promise(resolve=>setTimeout(resolve,180));}
   }else if(uri==='/api/power'){
    commands.push(body);status=202;result={accepted:true};
    // A command must not fabricate an observed state.
    for(const s of stations)if(body.station==='all'||s.id===body.station)Object.assign(s,{requested:body.state,pending:true,alexa_target_pending:true,requested_ms:data.uptime_ms});
   }else if(uri==='/api/device'){assert.equal(body.name,'vr-room');data.hostname=body.name;result={saved:true,hostname:body.name,setup_ssid:body.name,url:'http://vr-room.local/'};}
   else if(uri==='/api/ota/prepare'){otaPrepared=true;result={token:'test-upload-token',ready:false,max_bytes:0x1e0000};}
   else if(uri==='/api/ota/cancel'){otaPrepared=false;result={cancelled:true};}
   else if(uri==='/api/ota'&&req.method()==='GET')result={prepared:otaPrepared,ready:true};
   else if(uri==='/api/ota'){assert.equal(req.headers()['x-ota-token'],'test-upload-token');assert.ok(req.postDataBuffer().includes(Buffer.from('LHBRIDGE-ESP32-OTA1')));if(otaInvalid){status=400;result={error:'OTA_INVALID'};}else{otaInstalled=true;result={installed:true,restarting:true};}}
   else if(uri==='/api/wifi'){status=202;result={accepted:true,setup_ssid:'Lighthouse-Setup-EXAMPLE'};}
   else{status=404;result={error:'unknown'};}
   await route.fulfill({status,contentType:'application/json',body:JSON.stringify(result)});
  });
  await page.goto('http://bridge.test/');
  assert.equal(await page.locator('html').getAttribute('lang'),'en');
  assert.equal(await page.locator('#dashboard').isHidden(),true);
  assert.equal(await page.locator('#api-key').innerText(),'');
  await page.locator('#login-password').fill(key);await page.locator('#login-form button').click();
  await page.waitForFunction(()=>!document.getElementById('password-panel').hidden);
  assert.equal(await page.locator('#dashboard').isHidden(),true);
  assert.equal(await page.locator('#api-key').innerText(),'');
  await page.locator('#new-password').fill(key);await page.locator('#confirm-password').fill(key);await page.locator('#password-form button[type=submit]').click();
  await page.waitForFunction(()=>document.getElementById('message').textContent.includes('different from your API key'));
  await page.locator('#new-password').fill(password);await page.locator('#confirm-password').fill(password);await page.locator('#password-form button[type=submit]').click();
  await page.waitForFunction(()=>!document.getElementById('dashboard').hidden);
  await page.waitForFunction(()=>document.getElementById('connection').textContent==='On home network');
  assert.equal(await page.locator('#api-key').textContent(),key);
  assert.ok(refreshes>=1,'read states immediately after sign-in');
  assert.equal(await page.locator('#all-on').isDisabled(),true);
  await page.locator('#configuration summary').click();await page.locator('#scan').click();
  await page.waitForFunction(()=>document.querySelectorAll('#scan-results li').length===4);
  assert.equal(await page.locator('#scan-results img').count(),0);
  for(let i=1;i<=4;i++)await page.locator('#edit-mac-'+i).fill('AA:BB:CC:DD:EE:0'+i);
  assert.equal(await page.locator('[id^=pick-],[id^=edit-type-]').count(),0);
  assert.equal(await page.locator('#edit-mac-2').getAttribute('list'),'discovered-addresses');
  assert.equal(await page.locator('#discovered-addresses option').count(),4);
  await page.locator('#edit-name-1').fill('Left corner');
  await page.locator('#language').selectOption('de');
  assert.equal(await page.locator('#edit-name-1').inputValue(),'Left corner');
  await page.waitForTimeout(2800);assert.equal(await page.locator('#edit-name-1').inputValue(),'Left corner');
  saveFailure=true;await page.locator('#save').click();
  await page.waitForFunction(()=>document.getElementById('message').textContent.includes('Bluetooth ist beschäftigt'));
  saveFailure=false;await page.locator('#save').click();
  await page.waitForFunction(()=>document.getElementById('name-1').textContent==='Left corner');
  // External state change becomes visible before any button is pressed.
  Object.assign(stations[0],{observed:'on',observed_raw:11,observed_ms:89000,reachable:true});
  await page.waitForFunction(()=>document.getElementById('state-1').textContent==='ON');
  assert.equal(commands.length,0);
  await page.locator('#sleep-1').click();
  await page.waitForFunction(()=>document.getElementById('status-1').textContent.includes('Vorgemerkt'));
  assert.equal(await page.locator('#state-1').textContent(),'ON','requested SLEEP is not a measurement');
  assert.deepEqual(commands.at(-1),{station:1,state:'sleep'});
  Object.assign(stations[0],{pending:false,observed:'sleep',observed_raw:0,observed_ms:89500,reachable:true});
  await page.waitForFunction(()=>document.getElementById('state-1').textContent==='SLEEP');
  const before=commands.length;await page.locator('#refresh').click();assert.equal(commands.length,before);assert.ok(refreshes>=2);
  Object.assign(stations[1],{observed:'on',observed_raw:11,observed_ms:88000,reachable:true,read_failures:1,error:'BLE_READ_FAILED'});
  await page.waitForFunction(()=>document.getElementById('state-2').textContent==='ON');
  assert.equal(await page.locator('#debug-indicator').isHidden(),true);
  assert.doesNotMatch(await page.locator('#status-2').textContent(),/Wiederholung|nicht gelesen/);
  assert.doesNotMatch(await page.locator('#status-2').textContent(),/Letzte erfolgreiche Messung/);
  assert.equal(await page.locator('#status-2').evaluate(node=>node.classList.contains('warning')),false);
  const layout=()=>page.evaluate(()=>[...document.querySelectorAll('#stations article,#stations .actions,#stations .status,footer,#version-toggle')].map(node=>{const r=node.getBoundingClientRect();return [node.id||node.className,...[r.left+scrollX,r.top+scrollY,r.width,r.height].map(n=>Math.round(n*100)/100)];}));
  const layoutBefore=await layout();
  const commandCount=commands.length,refreshCount=refreshes;
  await page.locator('#version-toggle').click();assert.equal(await page.locator('#version-toggle').getAttribute('aria-pressed'),'false');
  await page.waitForTimeout(1300); // Expired first click must not contribute.
  await page.locator('#version-toggle').click();await page.locator('#version-toggle').click();
  assert.equal(await page.locator('#version-toggle').getAttribute('aria-pressed'),'false','two fresh clicks cannot enable debug');
  await page.locator('#version-toggle').click();
  assert.equal(await page.locator('#debug-indicator').textContent(),'Debug aktiviert');
  assert.equal(await page.locator('#debug-indicator').isVisible(),true);
  assert.deepEqual(await layout(),layoutBefore,'desktop cards/actions/footer do not move when enabling debug');
  assert.equal(commands.length,commandCount);assert.equal(refreshes,refreshCount,'debug toggle does not send refresh writes');
  await page.waitForFunction(()=>document.getElementById('state-2').textContent==='ON · letzte Messung');
  assert.match(await page.locator('#status-2').textContent(),/Wiederholung/);
  assert.doesNotMatch(await page.locator('#status-2').textContent(),/veraltet/);
  Object.assign(stations[1],{reachable:false,read_failures:2});
  await page.waitForFunction(()=>document.getElementById('state-2').textContent==='Unbekannt');
  await page.waitForFunction(()=>document.getElementById('status-2').textContent.includes('nicht gelesen'));
  assert.equal(await page.locator('#state-2').textContent(),'Unbekannt');
  assert.doesNotMatch(await page.locator('#status-2').textContent(),/veraltet/,'recent failed readings are unavailable, not expired');
  await page.locator('#version-toggle').click({clickCount:3});
  assert.equal(await page.locator('#status-2').textContent(),'nicht erreichbar');
  assert.doesNotMatch(await page.locator('#status-2').textContent(),/nicht gelesen/);
  assert.equal(await page.locator('#status-2').evaluate(node=>node.classList.contains('warning')),false);
  assert.equal(await page.locator('#state-2').textContent(),'Unbekannt');
  assert.equal(await page.locator('#debug-indicator').isHidden(),true);
  await page.locator('#version-toggle').click({clickCount:3});
  stations[1].observed_ms=20000;
  await page.waitForFunction(()=>document.getElementById('status-2').textContent.includes('veraltet'));
  stations[1].observed_ms=88000;
  Object.assign(stations[3],{requested:'on',command_sent:true,command_confirmed:false,command_status:'verifying',observed:'sleep',observed_raw:0,observed_ms:89000,reachable:true});
  await page.waitForFunction(()=>document.getElementById('status-4').textContent.includes('prüfe Zustand'));
  assert.equal(await page.locator('#state-4').textContent(),'SLEEP','verifying an ON command cannot invent an ON measurement');
  await page.locator('#language').selectOption('en');
  assert.match(await page.locator('#status-4').textContent(),/Last command: ON.*checking state/);
  Object.assign(stations[3],{command_confirmed:true,command_status:'confirmed',observed:'on',observed_raw:11});
  await page.waitForFunction(()=>document.getElementById('status-4').textContent.includes('target state confirmed'));
  assert.equal(await page.locator('#state-4').textContent(),'ON');
  await page.locator('#version-toggle').click({clickCount:3});
  assert.doesNotMatch(await page.locator('#status-4').textContent(),/Last command|confirmed|reading/);
  assert.equal(await page.locator('#status-4').evaluate(node=>node.classList.contains('warning')),false);
  assert.equal(await page.locator('#state-4').textContent(),'ON');
  await page.locator('#version-toggle').focus();await page.keyboard.press('Enter');await page.keyboard.press('Enter');await page.keyboard.press('Enter');
  assert.match(await page.locator('#status-4').textContent(),/target state confirmed/);
  Object.assign(stations[3],{command_confirmed:false,command_status:'unconfirmed'});
  await page.waitForFunction(()=>document.getElementById('status-4').textContent.includes('not confirmed within 30 seconds'));
  assert.equal(await page.locator('#state-4').textContent(),'ON','a late measurement and the verification deadline are distinct');
  assert.equal(await page.locator('#status-4').evaluate(node=>node.classList.contains('warning')),true);
  await page.locator('#language').selectOption('de');
  assert.match(await page.locator('#status-4').textContent(),/nicht innerhalb von 30 Sekunden bestätigt/);
  Object.assign(stations[3],{command_sent:false,command_status:'failed'});
  await page.waitForFunction(()=>document.getElementById('status-4').textContent.includes('Befehl konnte nicht gesendet'));
  Object.assign(stations[3],{requested:null,command_status:'none'});
  Object.assign(stations[2],{reading:true,read_active:false,read_queued:true});
  await page.waitForFunction(()=>document.getElementById('status-3').textContent.includes('Abfrage vorgemerkt'));
  assert.doesNotMatch(await page.locator('#status-3').textContent(),/Lese über Bluetooth/);
  await page.locator('#language').selectOption('en');
  assert.match(await page.locator('#status-3').textContent(),/Bluetooth read queued/);
  assert.match(await page.locator('#status-2').textContent(),/Last successful reading/);
  await page.locator('#language').selectOption('de');
  Object.assign(stations[2],{read_active:true,read_queued:false});
  stations[2].reading=true;
  await page.waitForFunction(()=>document.getElementById('status-3').textContent.includes('Lese über Bluetooth'));
  assert.equal(await page.locator('#save').isEnabled(),true,'background reads must not trap the assignment form');
  assert.equal(await page.locator('#wifi').isEnabled(),true,'background reads must not flicker the Wi-Fi button');
  Object.assign(stations[2],{reading:false,read_active:false,read_queued:false});
  await page.locator('#language').selectOption('en');
  assert.match(await page.locator('#status-2').textContent(),/Could not read/);
  assert.match(await page.locator('#status-2').textContent(),/Last successful reading/);
  assert.match(await page.locator('#message').textContent(),/State reads queued/);
  await page.locator('#configuration summary').click();
  await page.screenshot({path:path.join(root,'docs/webinterface-desktop.png'),fullPage:true});
  await page.setViewportSize({width:390,height:844});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
  Object.assign(stations[1],{read_failures:1,reachable:true,requested:'sleep',command_status:'confirmed',error:'BLE diagnostic '+ 'x'.repeat(80)});
  await page.waitForFunction(()=>document.getElementById('status-2').textContent.includes('Last command: SLEEP'));
  const mobileLayout=await layout();
  await page.locator('#version-toggle').click({clickCount:3});
  assert.deepEqual(await layout(),mobileLayout,'mobile cards/actions/footer do not move when hiding details');
  await page.locator('#version-toggle').click({clickCount:3});
  assert.deepEqual(await layout(),mobileLayout,'mobile cards/actions/footer do not move when showing details');
  assert.equal(await page.locator('#status-2').evaluate(node=>node.scrollHeight>node.clientHeight),true,'long diagnostics scroll inside the reserved area');
  await page.locator('#language').selectOption('de');assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
  const germanMobileLayout=await layout();await page.locator('#version-toggle').click({clickCount:3});
  assert.deepEqual(await layout(),germanMobileLayout,'German mobile layout stays fixed');
  await page.locator('#version-toggle').click({clickCount:3});assert.deepEqual(await layout(),germanMobileLayout);
  await page.screenshot({path:path.join(root,'docs/webinterface-mobile.png'),fullPage:true});
  await page.locator('#network-settings summary').click();assert.equal(await page.locator('#api-key').isVisible(),true);
  await page.locator('#logout').click();await page.waitForFunction(()=>!document.getElementById('login-panel').hidden);
  assert.equal(await page.locator('#api-key').textContent(),'');
  assert.equal(await page.locator('#version-toggle').getAttribute('aria-pressed'),'false','sign-out resets debug');
  await page.locator('#login-password').fill(key);await page.locator('#login-form button').click();
  await page.waitForFunction(()=>document.getElementById('message').textContent.includes('Passwort falsch'));
  await page.locator('#login-password').fill(password);await page.locator('#login-form button').click();
  await page.waitForFunction(()=>!document.getElementById('dashboard').hidden);
  assert.equal(await page.locator('#api-key').textContent(),key);
  await page.locator('#version-toggle').click({clickCount:3});
  assert.equal(await page.locator('#version-toggle').getAttribute('aria-pressed'),'true');
  await page.reload();await page.waitForFunction(()=>!document.getElementById('dashboard').hidden);
  assert.equal(await page.locator('#version-toggle').getAttribute('aria-pressed'),'false','reload resets debug');
  await page.locator('#network-settings').evaluate(node=>node.open=true);
  assert.deepEqual(await page.locator('#copy-key').evaluate(node=>{const s=getComputedStyle(node);return [s.backgroundColor,s.borderWidth,s.padding];}),['rgba(0, 0, 0, 0)','0px','0px'],'API key is plain clickable text without a button box');
  const keyContrast=await page.locator('#api-key').evaluate(node=>{
   const luminance=color=>{const [r,g,b]=color.match(/[\d.]+/g).slice(0,3).map(Number).map(n=>{n/=255;return n<=.04045?n/12.92:((n+.055)/1.055)**2.4;});return .2126*r+.7152*g+.0722*b;};
   let backdrop=node;while(backdrop&&getComputedStyle(backdrop).backgroundColor==='rgba(0, 0, 0, 0)')backdrop=backdrop.parentElement;
   const fg=luminance(getComputedStyle(node).color),bg=luminance(getComputedStyle(backdrop).backgroundColor);return (Math.max(fg,bg)+.05)/(Math.min(fg,bg)+.05);
  });assert.ok(keyContrast>=4.5,'API key contrast must remain readable');
  await page.locator('#copy-key').click();assert.equal(await page.evaluate(()=>window.copiedValue),key);
  await page.evaluate(()=>{window.copiedValue='';});await page.locator('#copy-key').focus();await page.keyboard.press('Enter');assert.equal(await page.evaluate(()=>window.copiedValue),key);
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false,'plain key fits the mobile dashboard');
  await page.locator('#device-name').fill('vr-room');await page.locator('#device-form button').click();
  await page.waitForFunction(()=>document.getElementById('device-address').textContent.includes('vr-room.local'));
  assert.equal(await page.locator('#setup-ssid').textContent(),'vr-room');
  await page.locator('#configuration').evaluate(node=>node.open=true);
  for(let i=0;i<12;i++)await page.locator('#add-station').click();
  assert.equal(await page.locator('.config-slot').count(),16);assert.equal(await page.locator('#add-station').isDisabled(),true);
  await page.locator('#config-fields .config-slot').nth(4).locator('input[list]').fill('AA:BB:CC:DD:EE:05');
  await page.locator('#config-fields .config-slot').nth(5).locator('input[list]').fill('AA:BB:CC:DD:EE:06');
  await page.locator('#language').selectOption('en');assert.equal(await page.locator('.config-slot').count(),16);
  await page.locator('#save').click();assert.equal(await page.locator('#on-1').isDisabled(),true,'controls wait for the saved ID mapping');await page.waitForFunction(()=>document.querySelectorAll('#stations article').length===16&&!document.getElementById('on-1').disabled);
  assert.equal(stations[4].id,5);assert.equal(stations[4].hue_id,6);assert.equal(stations[15].id,16);
  assert.deepEqual(await page.locator('#stations .slot').allTextContents(),Array.from({length:16},(_,i)=>'STATION '+String(i+1).padStart(2,'0')));
  await page.locator('[data-remove="2"]').click();await page.locator('#save').click();
  await page.waitForFunction(()=>document.querySelectorAll('#stations article').length===15);assert.equal(stations[1].id,2);assert.equal(stations[1].name,'Base station 3');
  await page.locator('#add-station').click();await page.locator('#save').click();
  await page.waitForFunction(()=>document.getElementById('card-16'));assert.equal(stations.at(-1).id,16);assert.equal(stations.at(-1).hue_id,18);
  await page.locator('#on-5').click();assert.deepEqual(commands.at(-1),{station:5,state:'on'});
  assert.equal(stations[4].hue_id,7);stations[4].pending=false;
  // Removing all entries is supported; empty state is not represented by fixed slots.
  await page.locator('#configuration').evaluate(node=>node.open=true);
  while(await page.locator('[data-remove]').count())await page.locator('[data-remove]').first().click();
  await page.locator('#save').click();await page.waitForFunction(()=>document.querySelectorAll('#stations article').length===0);
  assert.equal(await page.locator('#all-on').isDisabled(),true);
  // Exercise the actual browser multipart upload, header token, waiting and errors.
  await page.locator('#ota-settings').evaluate(node=>node.open=true);
  const file=Buffer.alloc(1064);file.write('LHBRIDGE-ESP32-OTA1');file.writeUInt32LE(1000,24);
  await page.locator('#ota-file').setInputFiles({name:'test.lhota',mimeType:'application/octet-stream',buffer:file});
  otaInvalid=true;await page.locator('#ota-update').click();
  await page.waitForFunction(()=>document.getElementById('ota-status').textContent.includes('Invalid, damaged'));
  assert.equal(otaInstalled,false);assert.equal(otaPrepared,false);
  otaInvalid=false;await page.locator('#ota-update').click();
  await page.waitForFunction(()=>document.getElementById('ota-status').textContent.includes('Update installed'));
  assert.equal(otaInstalled,true);assert.deepEqual(errors,[]);
  console.log('Embedded UI passed: bootstrap-to-own-password, key separation, EN/DE, discovered assignments, real-vs-requested states, refresh without writes, stale readings, logout and relogin.');
 }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
