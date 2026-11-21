const {chromium}=require('playwright'),fs=require('node:fs'),zlib=require('node:zlib'),assert=require('node:assert/strict');
const source=fs.readFileSync('.pio/rs-matter-sketch/RsMatter/web_ui_gzip.h','utf8');const html=zlib.gunzipSync(Buffer.from(source.split('{')[1].split('}')[0].split(',').map(Number))).toString();
(async()=>{const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});try{for(const lang of ['en','de'])for(const width of [1280,390,320]){
 const page=await browser.newPage({viewport:{width,height:1100}}),calls=[],errors=[];page.on('pageerror',e=>errors.push(e.message));let signed=false,remoteRequests=0,bindings=[{pin:-1,action:'disabled'},{pin:-1,action:'disabled'}];
 await page.route('http://bridge.test/**',route=>{const r=route.request(),p=new URL(r.url()).pathname;calls.push({p,method:r.method()});if(p==='/')return route.fulfill({contentType:'text/html',body:html});if(p==='/api/login'&&r.method()==='GET')return route.fulfill({json:{username_enabled:false}});if(p==='/api/login'){signed=true;return route.fulfill({json:{password_required:false}});}if(!signed)return route.fulfill({status:401,json:{error:'LOGIN_REQUIRED'}});
 if(p==='/api/account')return route.fulfill({json:{password_required:false,alexa_protocol:'matter',network_mode:'home',matter_ready:true,api_key:'111111111111111111111111',hostname:'example'}});
 if(p==='/api/status')return route.fulfill({json:{version:'0.1.23-rs-matter',network_mode:'home',wifi_connected:true,alexa_protocol:'matter',matter_ready:true,matter_paired:true,stations:[{id:1,name:"Example",mac:"02:00:00:00:00:01",configured:true,reachable:true,observed:"sleep",observed_raw:0}],discovered:[]}});
 if(p==='/api/refresh')return route.fulfill({status:202,json:{accepted:true}});
 if(p==='/api/buttons'){if(r.method()==='GET')return route.fulfill({json:{pins:[25,26,27,32,33],wake_pins:[25,26,27,32,33],buttons:bindings}});bindings=r.postDataJSON().buttons;return route.fulfill({json:{saved:true}});}
 if(p==='/api/identify'){
  if(r.method()==='POST'){assert.deepEqual(r.postDataJSON(),{id:1,mac:'02:00:00:00:00:01'});return route.fulfill({status:202,json:{job:1}});}
  return route.fulfill({json:{job:1,pending:false,error:''}});
 }
 if(p==='/api/remote'){assert.deepEqual(r.postDataJSON(),{confirm:true});++remoteRequests;return route.fulfill({status:202,json:{accepted:true}});}
 if(p==='/api/logout'){signed=false;return route.fulfill({json:{ok:true}});}return route.fulfill({status:404,json:{error:'NOT_FOUND'}});});
 await page.goto('http://bridge.test/?lang='+lang);await page.locator('#login-password').fill('example password');await page.locator('#login-form button').click();await page.waitForFunction(()=>current&&!polling);
 assert.equal(await page.locator('#danger-settings').evaluate(node=>node.previousElementSibling.querySelector('summary').dataset.t),'security');
 await page.locator('#configuration summary').click();
 assert.deepEqual(await page.locator('.config-actions button').evaluateAll(nodes=>nodes.map(n=>n.dataset.t)),['identify','removeStation']);
 const input=page.locator('#edit-mac-1'), identify=page.locator('[data-identify="1"]');
 await input.fill('02:00:00:00:00:02');assert.equal(await identify.isDisabled(),true);
 await input.fill('02:00:00:00:00:01');await identify.click();
 await page.waitForFunction(()=>document.querySelector('#message').textContent.includes('Identify'));
 assert.equal(calls.filter(x=>x.p==='/api/identify'&&x.method==='POST').length,1);
 assert.equal(calls.filter(x=>x.p==='/api/buttons').length,0);await page.locator('#gpio-settings summary').click();await page.locator('#gpio-pin-0').waitFor();
 assert.equal(await page.locator('#remote-start').isDisabled(),true);
 await page.locator('#gpio-pin-0').selectOption('25');await page.locator('#gpio-action-0').selectOption('all_on');await page.locator('#gpio-pin-1').selectOption('26');await page.locator('#gpio-action-1').selectOption('all_off');
 await page.locator('#language').selectOption(lang==='en'?'de':'en');assert.equal(await page.locator('#gpio-pin-0').inputValue(),'25');assert.equal(await page.locator('#gpio-action-1').inputValue(),'all_off');
 await page.locator('#gpio-form button').click();await page.waitForFunction(()=>document.querySelector('#message').textContent.includes('saved')||document.querySelector('#message').textContent.includes('gespeichert'));
 assert.deepEqual(bindings,[{pin:25,action:'all_on'},{pin:26,action:'all_off'}]);assert.equal(calls.filter(x=>x.p==='/api/buttons'&&x.method==='POST').length,1);
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
 assert.equal(calls.some(x=>['/api/power','/api/network','/api/config','/api/matter/commissioning','/api/wifi'].includes(x.p)),false);assert.deepEqual(errors,[]);
 await page.locator('#remote-start').waitFor();
 page.once('dialog',d=>d.dismiss());await page.locator('#remote-start').click();assert.equal(remoteRequests,0);
 page.once('dialog',d=>d.accept());await page.locator('#remote-start').click();
 await page.waitForFunction(()=>networkChanging);assert.equal(remoteRequests,1);
 await page.locator('#logout').click();await page.waitForFunction(()=>!loggedIn);assert.equal(await page.locator('#gpio-bindings').textContent(),'');await page.evaluate(()=>apiTail.catch(()=>{}));await page.close();
}console.log('GPIO settings passed EN/DE desktop/mobile: explicit private GET, assignments, language preservation, one save, no power/network action, logout clearing.');}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1);});
