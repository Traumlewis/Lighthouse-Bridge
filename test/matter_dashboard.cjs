// Exercise the actual generated gzip dashboard with a mocked HTTP backend.
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),zlib=require('node:zlib');
const root=path.resolve(__dirname,'..');
const prototypeName=process.env.LH_PROTOTYPE==='tasmota'?'tasmota':'matter';
const version=JSON.parse(fs.readFileSync(path.join(root,'_site/'+prototypeName+'-manifest.json'),'utf8')).version;
const generatedPath=prototypeName==='tasmota'?'.pio/tasmota-src/tasmota/web_ui_gzip.h':'.pio/matter-sketch/MatterBridge/web_ui_gzip.h';
const generated=fs.readFileSync(path.join(root,generatedPath),'utf8');
const compressed=Buffer.from(generated.split('{')[1].split('}')[0].split(',').map(Number));
const html=zlib.gunzipSync(compressed).toString();
assert.ok(html.includes('Bridge '+version));
(async()=>{
 const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});
 try{
  // Use a real HTTP response for browser gzip decoding. route.fulfill(html)
  // below tests UI logic but bypasses the firmware's encoded transport entirely.
  const transport=require('node:http').createServer((req,res)=>{
   if(req.url==='/'){
    res.writeHead(200,{'Content-Type':'text/html; charset=utf-8','Content-Encoding':'gzip','Content-Length':compressed.length});
    for(let i=0;i<compressed.length;i+=512)res.write(compressed.subarray(i,i+512));
    res.end();
   }else{res.writeHead(401,{'Content-Type':'application/json'});res.end('{"error":"LOGIN_REQUIRED"}');}
  });
  await new Promise(resolve=>transport.listen(0,'127.0.0.1',resolve));
  const transportPage=await browser.newPage(),transportErrors=[];
  transportPage.on('pageerror',e=>transportErrors.push(e.message));
  try{
   const response=await transportPage.goto('http://127.0.0.1:'+transport.address().port+'/');
   assert.equal(response.headers()['content-encoding'],'gzip');
   assert.equal(await response.text(),html);
   await transportPage.locator('#login-password').waitFor({state:'visible'});
   assert.deepEqual(transportErrors,[]);
   const aborted=await transportPage.evaluate(async()=>{
    const saved=window.fetch;
    window.fetch=()=>Promise.reject(new DOMException('aborted','AbortError'));
    try{await api('/api/status');}catch(error){return error.code;}
    finally{window.fetch=saved;}
   });
   assert.equal(aborted,'NETWORK_ERROR','numeric browser abort code 20 needs a readable error');
   const peakRequests=await transportPage.evaluate(async()=>{
    const saved=window.fetch;let active=0,peak=0;
    window.fetch=async()=>{peak=Math.max(peak,++active);await new Promise(r=>setTimeout(r,40));--active;
     return new Response('{}',{status:200,headers:{'Content-Type':'application/json'}});};
    try{await Promise.all([api('/api/status'),api('/api/account')]);return peak;}
    finally{window.fetch=saved;}
   });
   assert.equal(peakRequests,1,'Matter JSON requests must not hold simultaneous TCP connections');
  }finally{await transportPage.close();await new Promise(resolve=>transport.close(resolve));}
  for(const lang of ['en','de']){
   const page=await browser.newPage({viewport:{width:390,height:844}}),errors=[],calls=[];
   page.on('pageerror',e=>errors.push(e.message));
   await page.addInitScript(()=>{document.execCommand=()=>{window.copied=document.activeElement.value;return true;};});
   await page.addInitScript(()=>{
    const timer=window.setTimeout;
    window.setTimeout=(fn,ms,...args)=>ms===2000?0:timer(fn,ms,...args);
   });
   let signedIn=false,windowOpen=false,statusFails=true,bridgeFails=true,windowPending=0,rotations=0,rejectWindow=false,rejectInfo=false;
   const key='1234567890abcdef12345678';let code='34970112332';
   const data={alexa_protocol:'matter',matter_ready:true,matter_paired:true,ip:'192.168.1.42',hostname:'lighthouse-example',wifi_connected:true,alexa_started:true,uptime_ms:90000,stations:[{id:1,name:'Owner station',mac:'AA:BB:CC:DD:EE:01',configured:true,alexa_compat:true,reachable:true,observed:'sleep',observed_raw:0,observed_ms:89000}],discovered:[]};
   await page.route('http://bridge.test/**',async route=>{
    const req=route.request(),uri=new URL(req.url()).pathname;calls.push({uri,method:req.method(),headers:req.headers()});
    if(uri==='/')return route.fulfill({contentType:'text/html',body:html});
    let result={},status=200;
    if(signedIn&&uri==='/api/matter'&&rejectInfo)return route.fulfill({status:503,json:{error:'NETWORK_ERROR'}});
    if(uri==='/api/login'){signedIn=true;result={password_required:false};}
    else if(!signedIn){status=401;result={error:'LOGIN_REQUIRED'};}
    else if(uri==='/api/account')result={password_required:false,api_key:key,setup_ssid:'Lighthouse-Setup-EXAMPLE',hostname:data.hostname,alexa_protocol:'matter',matter_ready:true};
    else if(uri==='/api/status'){if(statusFails){status=503;result={error:'OUT_OF_MEMORY'};}else result=data;}
    else if(uri==='/api/refresh'){status=202;result={accepted:true};}
    else if(uri==='/api/matter/commissioning'){assert.equal(req.method(),'POST');assert.deepEqual(req.postDataJSON(),{});if(rejectWindow){status=503;result={error:'MATTER_WINDOW_FAILED'};}else{windowOpen=true;if(prototypeName==='tasmota'){rotations++;code=['34970112332','34970112349','34970112363'][rotations-1];windowPending=1;}status=202;result={accepted:true};}}
    else if(uri==='/api/matter')result=bridgeFails||data.ble_worker_available===false?{protocol:1,version,bridge_name:'Lighthouse-Bridge Matter Test',ready:false,paired:false,pairing_code:null,error:data.ble_worker_available===false?(data.ble_startup_error||'BLE_WORKER_FAILED'):'MATTER_BRIDGE_FAILED',commissioning_open:false,commissioning_pending:false}:{ready:true,paired:true,bridge_name:'Lighthouse-Bridge Matter Test',pairing_code:prototypeName==='tasmota'&&(!windowOpen||windowPending)?null:code,commissioning_open:windowOpen&&!windowPending,commissioning_pending:!!windowPending,error:''};
    else if(uri==='/api/scan'){status=202;result={accepted:true};data.discovered=[{name:'LHB-test',mac:'AA:BB:CC:DD:EE:02',address_type:1,rssi:-60}];data.scan_finished_ms=90001;}
    else if(uri==='/api/config'){
     assert.equal(req.method(),'POST');
     result={saved:true,restarting:true,stations:req.postDataJSON().stations.map((s,i)=>({...s,id:s.id||i+1}))};
    }
    else if(uri==='/api/logout'){signedIn=false;result={logged_out:true};}
    else {status=404;result={error:'NOT_FOUND'};}
    if(uri==='/api/matter'&&windowPending)windowPending--;
    await route.fulfill({status,json:result});
   });
   await page.goto('http://bridge.test/?lang='+lang);
   assert.equal(await page.locator('#matter-settings').isHidden(),true);
   assert.equal(await page.locator('#matter-code').textContent(),'');
   await page.locator('#login-password').fill('personal password');await page.locator('#login-form button').click();
   await page.waitForFunction(()=>!document.getElementById('matter-settings').hidden);
   assert.equal(await page.locator('#stations article').count(),0,'failed status must not hide the independent Matter controls');
   assert.equal(calls.filter(c=>c.uri==='/api/matter').length,0,'code retrieved only by explicit action');
   await page.locator('#matter-settings summary').click();
   // A real owner response must show its startup error, not only the bridge name.
   await page.locator('#matter-show').click();
   await page.waitForFunction(()=>document.getElementById('matter-status').textContent.includes('could not be created')||document.getElementById('matter-status').textContent.includes('nicht erstellt'));
   assert.equal(await page.locator('#matter-code').isHidden(),true);
   assert.equal(await page.locator('#matter-open').isDisabled(),true);
   bridgeFails=false;
   // Browser translation may replace a button's descendants. Rendering and copy
   // must not depend on the old nested <code> element or translated DOM text.
   await page.locator('#matter-code').evaluate(node=>node.replaceChildren());
   await page.locator('#matter-show').click();
   if(prototypeName==='tasmota'){
    assert.equal(await page.locator('#matter-code').isHidden(),true,'closed window must not show a usable code');
    await page.locator('#matter-open').click();
   }
   await page.waitForFunction(code=>document.getElementById('matter-code').textContent===code,code);
   assert.equal(await page.locator('#matter-name').textContent(),'Lighthouse-Bridge Matter Test');
   assert.equal(await page.locator('.compat-label').isHidden(),true);
   await page.locator('#matter-code').evaluate(node=>node.textContent='translated text');await page.locator('#matter-code').click();assert.equal(await page.evaluate(()=>window.copied),code);
   await page.locator('#language').selectOption(lang==='en'?'de':'en');assert.equal(await page.locator('#matter-code').textContent(),code);
   await page.locator('#matter-open').click();
   if(prototypeName==='tasmota'){
    await page.waitForFunction(()=>matterInfo?.commissioning_pending);
    assert.equal(await page.locator('#matter-code').isHidden(),true,'pending renewal hides the previous code');
    assert.equal(await page.locator('#matter-open').isDisabled(),true);
   }
   await page.waitForFunction(()=>document.getElementById('matter-status').textContent.includes('open')||document.getElementById('matter-status').textContent.includes('offen'));
   if(prototypeName==='tasmota'){
    const previous='34970112332';
    await page.waitForFunction(()=>document.getElementById('matter-code').textContent==='34970112349');
    assert.notEqual(code,previous,'manual activation replaces the active code');
    await page.locator('#matter-code').click();assert.equal(await page.evaluate(()=>window.copied),code);
    rejectWindow=true;await page.locator('#matter-open').click();
    await page.waitForFunction(()=>!matterBusy&&document.getElementById('message').textContent.includes('fenster')||!matterBusy&&document.getElementById('message').textContent.includes('window'));
    assert.equal(await page.locator('#matter-code').isHidden(),true,'failed renewal must not show an old code');
    assert.equal(await page.locator('#matter-open').isDisabled(),false,'failed renewal allows retry');
    rejectWindow=false;await page.locator('#matter-open').click();
    await page.waitForFunction(()=>document.getElementById('matter-code').textContent==='34970112363');
    rejectInfo=true;await page.evaluate(()=>loadMatter());
    assert.equal(await page.locator('#matter-code').isHidden(),true,'failed private status must hide cached code');
    rejectInfo=false;await page.evaluate(()=>loadMatter());
    assert.equal(await page.locator('#matter-code').textContent(),code);
    windowOpen=false;await page.evaluate(()=>loadMatter());
    assert.equal(await page.locator('#matter-code').isHidden(),true,'expired/closed window clears code');
    assert.equal(await page.evaluate(code=>JSON.stringify(localStorage).includes(code),code),false);
   }
   statusFails=false;await page.evaluate(()=>refresh());
   await page.waitForFunction(()=>document.getElementById('card-1'));
   await page.locator('#configuration summary').click();await page.locator('#scan').click();
   await page.waitForFunction(()=>document.getElementById('scan-results').textContent.includes('LHB-test'));
   data.ble_worker_available=false;data.ble_startup_error='BLE_WORKER_FAILED';data.matter_ready=false;
   await page.evaluate(()=>refresh());
   await page.waitForFunction(()=>document.getElementById('on-1').disabled&&document.getElementById('scan').disabled);
   assert.equal(await page.locator('#on-1').isDisabled(),true);
   assert.equal(await page.locator('#scan').isDisabled(),true);
   assert.equal(await page.locator('#refresh').isDisabled(),true);
   assert.equal(await page.locator('#ota-update').isDisabled(),false,'BLE failure must not disable firmware recovery');
   await page.locator('#matter-show').click();
   await page.waitForFunction(()=>document.getElementById('matter-status').textContent.includes('worker could not start')||document.getElementById('matter-status').textContent.includes('Worker konnte nicht starten'));
   assert.equal(await page.locator('#matter-code').isHidden(),true);
   data.ble_startup_error='MATTER_START_FAILED';
   await page.evaluate(()=>refresh());await page.locator('#matter-show').click();
   await page.waitForFunction(()=>document.getElementById('matter-status').textContent.includes('could not start')||document.getElementById('matter-status').textContent.includes('konnte nicht gestartet'));
   assert.equal(await page.locator('#scan').isDisabled(),true);
   assert.equal(await page.locator('#ota-update').isDisabled(),false);
   assert.equal(await page.locator('#matter-code').isHidden(),true);
   assert.equal(calls.filter(c=>c.uri==='/api/power').length,0);
   assert.equal(calls.filter(c=>c.uri.startsWith('/api/matter')).some(c=>c.headers['x-api-key']),false);
   assert.equal(await page.evaluate(code=>JSON.stringify(localStorage).includes(code),code),false);
   assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
   data.ble_worker_available=true;data.ble_startup_error='';data.matter_ready=true;
   await page.evaluate(()=>refresh());
   await page.locator('#add-station').click();
   await page.locator('#edit-mac-new-1').fill('AA:BB:CC:DD:EE:02');
   const statusBeforeSave=calls.filter(c=>c.uri==='/api/status').length;
   await page.locator('#save').click();
   await page.waitForFunction(()=>document.getElementById('message').textContent===t('matterSaved'));
   await page.waitForTimeout(300);
   assert.equal(calls.filter(c=>c.uri==='/api/status').length,statusBeforeSave,
    'an acknowledged restart must not immediately fetch status from the rebooting board');
   assert.equal(await page.locator('#on-1').isDisabled(),true);
   assert.equal(await page.locator('#save').isDisabled(),true);
   await page.locator('#logout').click();await page.waitForFunction(()=>!document.getElementById('login-panel').hidden);
   assert.equal(await page.locator('#matter-code').textContent(),'');
   assert.equal(await page.locator('#matter-code').isHidden(),true);
   assert.deepEqual(errors,[]);await page.close();
  }
  console.log(prototypeName+' gzip dashboard: real localhost gzip/login rendering; private code, pairing window, copy, language and logout passed (mock API).');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
