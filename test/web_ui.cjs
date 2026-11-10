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
  let signedIn=false,passwordSet=false,saveFailure=false,offline=false,refreshes=0;
  const commands=[];
  const stations=Array.from({length:4},(_,i)=>({id:i+1,name:'Base station '+(i+1),mac:'',address_type:0,configured:false,requested:null,pending:false,busy:false,reading:false,reachable:false,command_sent:false,observed:'unknown',observed_raw:-1,observed_ms:0,error:''}));
  const data={ip:'192.168.1.42',hostname:'lighthouse-example',wifi_connected:true,alexa_started:true,uptime_ms:90000,scanning:false,scan_finished_ms:0,scan_truncated:false,stations,discovered:[]};
  await page.route('http://bridge.test/**',async route=>{
   const req=route.request(),uri=new URL(req.url()).pathname;
   if(uri==='/')return route.fulfill({contentType:'text/html',body:html});
   if(offline)return route.abort('connectionfailed');
   const body=req.method()==='POST'?req.postDataJSON():null;
   let status=200,result={};
   if(uri==='/api/login'){
    if(body.password===(passwordSet?password:key)){signedIn=true;result={password_required:!passwordSet};}
    else{status=401;result={error:'LOGIN_FAILED'};}
   }else if(!signedIn){status=401;result={error:'LOGIN_REQUIRED'};}
   else if(uri==='/api/account')result={password_required:!passwordSet,setup_ssid:'Lighthouse-Setup-EXAMPLE',...(passwordSet?{api_key:key}:{})};
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
    else{body.stations.forEach((s,i)=>Object.assign(stations[i],s,{configured:Boolean(s.mac)}));result={saved:true};}
   }else if(uri==='/api/power'){
    commands.push(body);status=202;result={accepted:true};
    // A command must not fabricate an observed state.
    for(const s of stations)if(body.station==='all'||s.id===body.station)Object.assign(s,{requested:body.state,pending:true});
   }else if(uri==='/api/wifi'){status=202;result={accepted:true,setup_ssid:'Lighthouse-Setup-EXAMPLE'};}
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
  for(let i=1;i<=4;i++)await page.locator('#pick-'+i).selectOption('AA:BB:CC:DD:EE:0'+i);
  assert.equal(await page.locator('#edit-type-2').inputValue(),'1');
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
  await page.waitForFunction(()=>document.getElementById('state-2').textContent==='ON · letzte Messung');
  assert.match(await page.locator('#status-2').textContent(),/Wiederholung/);
  assert.doesNotMatch(await page.locator('#status-2').textContent(),/veraltet/);
  Object.assign(stations[1],{reachable:false,read_failures:2});
  await page.waitForFunction(()=>document.getElementById('state-2').textContent==='Unbekannt');
  await page.waitForFunction(()=>document.getElementById('status-2').textContent.includes('nicht gelesen'));
  assert.equal(await page.locator('#state-2').textContent(),'Unbekannt');
  stations[2].reading=true;
  await page.waitForFunction(()=>document.getElementById('status-3').textContent.includes('Lese über Bluetooth'));
  assert.equal(await page.locator('#save').isEnabled(),true,'background reads must not trap the assignment form');
  stations[2].reading=false;
  await page.locator('#language').selectOption('en');
  assert.match(await page.locator('#status-2').textContent(),/Could not read/);
  assert.match(await page.locator('#message').textContent(),/State reads queued/);
  await page.locator('#configuration summary').click();
  await page.screenshot({path:path.join(root,'docs/webinterface-desktop.png'),fullPage:true});
  await page.setViewportSize({width:390,height:844});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
  await page.locator('#language').selectOption('de');assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
  await page.screenshot({path:path.join(root,'docs/webinterface-mobile.png'),fullPage:true});
  await page.locator('#network-settings summary').click();assert.equal(await page.locator('#api-key').isVisible(),true);
  await page.locator('#logout').click();await page.waitForFunction(()=>!document.getElementById('login-panel').hidden);
  assert.equal(await page.locator('#api-key').textContent(),'');
  await page.locator('#login-password').fill(key);await page.locator('#login-form button').click();
  await page.waitForFunction(()=>document.getElementById('message').textContent.includes('Passwort falsch'));
  await page.locator('#login-password').fill(password);await page.locator('#login-form button').click();
  await page.waitForFunction(()=>!document.getElementById('dashboard').hidden);
  assert.equal(await page.locator('#api-key').textContent(),key);
  assert.deepEqual(errors,[]);
  console.log('Embedded UI passed: bootstrap-to-own-password, key separation, EN/DE, discovered assignments, real-vs-requested states, refresh without writes, stale readings, logout and relogin.');
 }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
