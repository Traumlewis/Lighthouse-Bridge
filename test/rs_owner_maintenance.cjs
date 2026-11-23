const {chromium}=require('playwright'),fs=require('node:fs'),zlib=require('node:zlib'),assert=require('node:assert/strict');
const source=fs.readFileSync('.pio/rs-matter-sketch/RsMatter/web_ui_gzip.h','utf8');
const html=zlib.gunzipSync(Buffer.from(source.split('{')[1].split('}')[0].split(',').map(Number))).toString();
(async()=>{const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});try{
for(const lang of ['en','de'])for(const width of [1280,390,320]){
 const page=await browser.newPage({viewport:{width,height:1100}}),calls=[],errors=[];
 let signed=false,room='',rejectRoom=false,rejectMaintenance=false,restarting=false,restartProbes=0;
 const account={password_required:false,alexa_protocol:'matter',network_mode:'home',matter_ready:true,api_key:'111111111111111111111111',setup_ssid:'Example-Bridge',hostname:'example'};
 const status={version:'0.1.32',network_mode:'home',wifi_connected:true,alexa_protocol:'matter',matter_ready:true,matter_paired:true,stations:[],discovered:[]};
 page.on('pageerror',e=>errors.push(e.message));await page.route('http://bridge.test/**',route=>{
  const r=route.request(),p=new URL(r.url()).pathname;calls.push({p,method:r.method(),body:r.postDataJSON()});
  if(p==='/')return route.fulfill({contentType:'text/html',body:html});
  if(p==='/api/login'&&r.method()==='GET')return route.fulfill({json:{username_enabled:false}});
  if(p==='/api/login'){signed=true;return route.fulfill({json:{password_required:false}});}
  if(p==='/api/account'&&restarting){
   restartProbes++;
   if(restartProbes===2)return route.abort('connectionrefused');
   if(restartProbes===3){restarting=false;signed=false;}
  }
  if(!signed)return route.fulfill({status:401,json:{error:'LOGIN_REQUIRED'}});
  if(p==='/api/account')return route.fulfill({json:{...account,room_name:room}});
  if(p==='/api/status')return route.fulfill({json:status});
  if(p==='/api/refresh')return route.fulfill({status:202,json:{accepted:true}});
  if(p==='/api/room'){if(rejectRoom)return route.fulfill({status:500,json:{error:'SAVE_FAILED'}});room=r.postDataJSON().name;return route.fulfill({json:{saved:true,name:room}});}
  if(p==='/api/maintenance'&&r.postDataJSON().action==='factory_preview')return route.fulfill({json:{reset_key:'222222222222222222222222'}});
  if(p==='/api/maintenance'){
   if(rejectMaintenance)return route.fulfill({status:409,json:{error:'BLE_BUSY'}});
   restarting=r.postDataJSON().action==='restart';
   return route.fulfill({status:202,json:{accepted:true,restarting:true}});
  }
  if(p==='/api/logout'){signed=false;return route.fulfill({json:{ok:true}});}
  return route.fulfill({status:404,json:{error:'NOT_FOUND'}});
 });
 const login=async()=>{await page.locator('#login-password').fill('example password');await page.locator('#login-form button').click();await page.waitForFunction(()=>current&&!polling);};
 await page.goto('http://bridge.test/?lang='+lang);await login();
 assert.equal(await page.locator('.intro h1').isVisible(),true);
 status.stations=[{id:1,name:'Example',mac:'',configured:false,reachable:false,observed:'unknown'}];await page.evaluate(()=>refresh());assert.equal(await page.locator('.intro h1').isVisible(),true);
 status.stations[0]={...status.stations[0],mac:'02:00:00:00:00:01',configured:true};await page.evaluate(()=>refresh());
 assert.equal(await page.locator('.intro h1').isVisible(),false);assert.equal(await page.locator('.intro p[data-t="intro"]').isVisible(),false);
 await page.locator('#room-edit').click();await page.locator('#room-input').fill('Draft 🎮');await page.evaluate(()=>refresh());
 await page.locator('#language').selectOption(lang==='en'?'de':'en');assert.equal(await page.locator('#room-input').inputValue(),'Draft 🎮');
 await page.locator('#room-input').press('Escape');assert.equal(calls.filter(x=>x.p==='/api/room').length,0);
 await page.locator('#room-edit').focus();await page.keyboard.press('Enter');await page.locator('#room-input').fill('Leas <VR> 🎮');await page.locator('#room-form button[type=submit]').click();
 await page.waitForFunction(()=>account.room_name==='Leas <VR> 🎮');assert.equal(room,'Leas <VR> 🎮');assert.equal(await page.locator('#room-label').textContent(),room);assert.equal(await page.locator('#room-label').locator('*').count(),0);
 await page.locator('#language').selectOption(lang);assert.equal(await page.locator('#room-label').textContent(),room);
 await page.locator('#room-edit').click();rejectRoom=true;await page.locator('#room-input').fill('Rejected');await page.locator('#room-form button[type=submit]').click();await page.waitForFunction(()=>!ownerBusy);assert.equal(await page.locator('#room-label').textContent(),room);await page.locator('#room-cancel').click();rejectRoom=false;
 await page.reload();await page.waitForFunction(()=>current&&!polling);assert.equal(await page.locator('#room-label').textContent(),room);
 await page.locator('#danger-settings summary').click();
 const danger=page.locator('#danger-settings');
 for(const value of [account.setup_ssid,account.api_key])assert.equal((await danger.textContent()).includes(value),false);
 await page.locator('#network-settings summary').click();
 assert.equal(await page.locator('#setup-ssid').textContent(),account.setup_ssid);
 assert.equal(await page.locator('#api-key').textContent(),account.api_key);
 await page.evaluate(()=>{window.copied=[];window.copyText=async text=>window.copied.push(text);});
 await page.locator('#copy-key').click();
 assert.deepEqual(await page.evaluate(()=>window.copied),[account.api_key]);
 await page.locator('#network-settings summary').click();
 assert.equal(await page.locator('summary[data-t="alexa"]').count(),0);
 const smart=page.locator('#matter-settings');
 assert.deepEqual(await smart.locator('li').allTextContents(),['Google Home','Apple HomeKit','Alexa','Home Assistant','Samsung SmartThings','IKEA DIRIGERA Hub']);
 assert.match(await smart.locator('summary').textContent(),/Smart Home/i);
 await page.locator('#factory-reset').click();await page.locator('#reset-preview').waitFor();
 assert.equal(await page.locator('#reset-key').textContent(),'222222222222222222222222');
 assert.equal(await page.locator('#api-key').textContent(),account.api_key);
 await page.locator('#reset-copy-key').click();
 assert.deepEqual(await page.evaluate(()=>window.copied),[account.api_key,'222222222222222222222222']);
 page.once('dialog',d=>d.dismiss());await page.locator('#reset-confirm').click();
 assert.equal(calls.filter(x=>x.p==='/api/maintenance').length,1);
 await page.locator('#reset-cancel').click();assert.equal(await page.locator('#reset-key').textContent(),'');
 rejectMaintenance=true;page.once('dialog',d=>d.accept());await page.locator('#bridge-restart').click();await page.waitForFunction(()=>!ownerBusy);assert.equal(await page.evaluate(()=>networkChanging),false);
 const reload=page.waitForNavigation({waitUntil:'domcontentloaded'});
 const documentCount=()=>calls.filter(x=>x.p==='/').length;
 const beforeRestart=documentCount();
 rejectMaintenance=false;page.once('dialog',d=>d.accept());await page.locator('#bridge-restart').click();await page.waitForFunction(()=>networkChanging);assert.deepEqual(calls.filter(x=>x.p==='/api/maintenance').at(-1).body,{action:'restart'});assert.equal(await page.locator('#copy-key').isEnabled(),true);
 await reload;
 assert.equal(restartProbes,3,'wait for the new login gate after an old response and a dropped connection');
 assert.equal(documentCount(),beforeRestart+1);
 assert.equal(new URL(page.url()).searchParams.get('lang'),lang);
 await page.locator('#login-panel').waitFor();await login();await page.locator('#danger-settings summary').click();
 await page.locator('#factory-reset').click();await page.locator('#reset-preview').waitFor();
 page.once('dialog',d=>d.accept());await page.locator('#reset-confirm').click();await page.waitForFunction(()=>networkChanging);
 assert.deepEqual(calls.filter(x=>x.p==='/api/maintenance').at(-1).body,{action:'factory_reset',confirm:true,reset_key:'222222222222222222222222'});
 assert.equal(calls.filter(x=>x.p==='/api/maintenance').length,5);
 assert.equal(await page.locator('#reset-key').textContent(),'222222222222222222222222');
 assert.equal(await page.locator('#reset-copy-key').isEnabled(),true);
 assert.equal(await page.locator('#api-key').textContent(),'');assert.equal(await page.locator('#copy-key').isEnabled(),false);
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
 assert.equal(calls.some(x=>['/api/power','/api/network','/api/config','/api/matter/commissioning','/api/wifi'].includes(x.p)),false);assert.deepEqual(errors,[]);
 await page.locator('#logout').click();assert.equal(await page.locator('#room-label').textContent(),'');assert.equal(await page.locator('#reset-key').textContent(),'');await page.evaluate(()=>apiTail.catch(()=>{}));await page.close();
}
console.log('Owner/maintenance UI passed EN/DE 1280/390/320: owner label, restart reconnect and automatic reload, explicit reset, private copy, reset clearing and no unrelated mutations.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1);});
