const {chromium}=require('playwright'),fs=require('node:fs'),zlib=require('node:zlib'),assert=require('node:assert/strict');
const source=fs.readFileSync('.pio/rs-matter-sketch/RsMatter/web_ui_gzip.h','utf8');
const html=zlib.gunzipSync(Buffer.from(source.split('{')[1].split('}')[0].split(',').map(Number))).toString();
(async()=>{const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});try{
for(const lang of ['en','de'])for(const width of [1280,390,320]){
 const page=await browser.newPage({viewport:{width,height:1100}}),calls=[],errors=[];let signed=false,room='',rejectRoom=false,rejectMaintenance=false;
 const account={password_required:false,alexa_protocol:'matter',network_mode:'home',matter_ready:true,api_key:'111111111111111111111111',setup_ssid:'Example-Bridge',hostname:'example'};
 const status={version:'0.1.21-rs-matter',network_mode:'home',wifi_connected:true,alexa_protocol:'matter',matter_ready:true,matter_paired:true,stations:[],discovered:[]};
 page.on('pageerror',e=>errors.push(e.message));await page.route('http://bridge.test/**',route=>{
  const r=route.request(),p=new URL(r.url()).pathname;calls.push({p,method:r.method(),body:r.postDataJSON()});
  if(p==='/')return route.fulfill({contentType:'text/html',body:html});
  if(p==='/api/login'&&r.method()==='GET')return route.fulfill({json:{username_enabled:false}});
  if(p==='/api/login'){signed=true;return route.fulfill({json:{password_required:false}});}
  if(!signed)return route.fulfill({status:401,json:{error:'LOGIN_REQUIRED'}});
  if(p==='/api/account')return route.fulfill({json:{...account,room_name:room}});
  if(p==='/api/status')return route.fulfill({json:status});
  if(p==='/api/refresh')return route.fulfill({status:202,json:{accepted:true}});
  if(p==='/api/room'){if(rejectRoom)return route.fulfill({status:500,json:{error:'SAVE_FAILED'}});room=r.postDataJSON().name;return route.fulfill({json:{saved:true,name:room}});}
  if(p==='/api/maintenance')return route.fulfill({status:rejectMaintenance?409:202,json:rejectMaintenance?{error:'BLE_BUSY'}:{accepted:true,restarting:true}});
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
 await page.locator('#danger-settings summary').click();assert.equal(await page.locator('#danger-ssid').textContent(),account.setup_ssid);assert.equal(await page.locator('#danger-key').textContent(),account.api_key);
 assert.deepEqual(await page.locator('#danger-copy-key').evaluate(e=>{const s=getComputedStyle(e);return [s.backgroundColor,s.borderTopWidth,s.paddingTop,s.paddingLeft];}),['rgba(0, 0, 0, 0)','0px','0px','0px']);
 await page.evaluate(()=>{window.copied=[];window.copyText=async text=>window.copied.push(text);});await page.locator('#danger-copy-key').click();assert.deepEqual(await page.evaluate(()=>window.copied),[account.api_key]);
 page.once('dialog',d=>d.dismiss());await page.locator('#factory-reset').click();assert.equal(calls.filter(x=>x.p==='/api/maintenance').length,0);
 rejectMaintenance=true;page.once('dialog',d=>d.accept());await page.locator('#bridge-restart').click();await page.waitForFunction(()=>!ownerBusy);assert.equal(await page.evaluate(()=>networkChanging),false);
 rejectMaintenance=false;page.once('dialog',d=>d.accept());await page.locator('#bridge-restart').click();await page.waitForFunction(()=>networkChanging);assert.deepEqual(calls.filter(x=>x.p==='/api/maintenance').at(-1).body,{action:'restart'});assert.equal(await page.locator('#danger-copy-key').isEnabled(),true);
 await page.reload();await page.waitForFunction(()=>current&&!polling);await page.locator('#danger-settings summary').click();page.once('dialog',d=>{assert.match(d.message(),/USB/);d.accept();});await page.locator('#factory-reset').click();await page.waitForFunction(()=>networkChanging);
 assert.deepEqual(calls.filter(x=>x.p==='/api/maintenance').at(-1).body,{action:'factory_reset',confirm:true});assert.equal(calls.filter(x=>x.p==='/api/maintenance').length,3);
 assert.equal(await page.locator('#danger-key').textContent(),'');assert.equal(await page.locator('#api-key').textContent(),'');assert.equal(await page.locator('#danger-copy-key').isEnabled(),false);
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
 assert.equal(calls.some(x=>['/api/power','/api/network','/api/config','/api/matter/commissioning','/api/wifi'].includes(x.p)),false);assert.deepEqual(errors,[]);
 await page.locator('#logout').click();assert.equal(await page.locator('#room-label').textContent(),'');assert.equal(await page.locator('#danger-ssid').textContent(),'');await page.evaluate(()=>apiTail.catch(()=>{}));await page.close();
}
console.log('Owner/maintenance UI passed EN/DE 1280/390/320: configured intro, editable persisted text, cancel/failure, explicit confirmations, private copy, reset clearing and no unrelated mutations.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1);});
