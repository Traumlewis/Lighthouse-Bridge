// Real generated rs-matter HTML: measured colors/layout and post-OTA reconnect.
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),zlib=require('node:zlib');
const root=path.resolve(__dirname,'..');
const header=fs.readFileSync(path.join(root,'.pio/rs-matter-sketch/RsMatter/web_ui_gzip.h'),'utf8');
const html=zlib.gunzipSync(Buffer.from(header.split('{')[1].split('}')[0].split(',').map(Number))).toString();
(async()=>{
 const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});
 try{
  const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
  let signedIn=false,installed=false,invalid=true,rootReads=0,reconnects=0,uploads=0,mutations=[];
  let servedVersion='0.1.28-rs-matter';
  const stations=['on','unknown','sleep','standby'].map((observed,i)=>({id:i+1,name:'Station '+(i+1),mac:'02:00:00:00:00:0'+(i+1),configured:true,
   observed,observed_raw:[11,-1,0,2][i],reachable:i!==1,observed_ms:89000,command_status:'none',requested:null,pending:false,busy:false}));
  const status={version:'0.1.28-rs-matter',alexa_protocol:'matter',matter_ready:true,matter_paired:true,wifi_connected:true,alexa_started:true,
   ip:'192.0.2.1',hostname:'lighthouse-bridge-a1b2c3',uptime_ms:90000,stations,discovered:[]};
  const account={setup_ssid:'Lighthouse-Bridge-Setup-A1B2C3',password_required:false,api_key:'1234567890abcdef12345678',hostname:'lighthouse-bridge-a1b2c3',alexa_protocol:'matter',matter_ready:true};
  await page.route('http://bridge.test/**',async route=>{
   const r=route.request(),uri=new URL(r.url()).pathname;
   if(uri==='/'){rootReads++;return route.fulfill({contentType:'text/html',body:html.replaceAll('0.1.28-rs-matter',servedVersion)});}
   if(r.method()==='POST')mutations.push(uri);
   if(uri==='/api/login'&&r.method()==='GET')return route.fulfill({json:{username_enabled:false}});
    if(uri==='/api/login'){signedIn=true;return route.fulfill({json:{password_required:false}});}
   if(uri==='/api/logout'){signedIn=false;return route.fulfill({json:{logged_out:true}});}
   if(installed&&uri==='/api/account'){
    reconnects++;
    if(reconnects===1)return route.fulfill({json:account}); // old process still alive
    if(reconnects===2)return route.abort('connectionfailed'); // board rebooting
    signedIn=false;
   }
   if(!signedIn)return route.fulfill({status:401,json:{error:'LOGIN_REQUIRED'}});
   if(uri==='/api/account')return route.fulfill({json:account});
   if(uri==='/api/status')return route.fulfill({json:status});
   if(uri==='/api/refresh')return route.fulfill({status:202,json:{accepted:true}});
   if(uri==='/api/matter')return route.fulfill({json:{ready:true,paired:true,bridge_name:'Lighthouse-Bridge',commissioning_open:false,pairing_code:null}});
   if(uri==='/api/station-info'){
    if(r.method()==='POST'){
     assert.deepEqual(r.postDataJSON(),{id:3,mac:stations[2].mac});
     return route.fulfill({status:202,json:{job:7}});
    }
    return route.fulfill({json:{job:7,pending:false,info:{type:'Lighthouse V2',name:'LHB-12345678',firmware:'R: 1.2.3\nM: 4.5.6\nB: 7.8.9',channel:1,manufacturer:'Example <b>Vendor</b>',model:112233,serial:'FB0012345678'}}});
   }
   if(uri==='/api/ota/prepare')return route.fulfill({status:202,json:{token:'synthetic-token',ready:true,max_bytes:0x1e0000}});
   if(uri==='/api/ota/cancel')return route.fulfill({json:{cancelled:true}});
   if(uri==='/api/ota'&&r.method()==='POST'){
    uploads++;assert.equal(r.headers()['x-ota-token'],'synthetic-token');
    if(invalid)return route.fulfill({status:400,json:{error:'OTA_INVALID'}});
    installed=true;return route.fulfill({json:{installed:true,restarting:true}});
   }
   return route.fulfill({status:404,json:{error:'NOT_FOUND'}});
  });
  await page.goto('http://bridge.test/');
  await page.locator('#login-password').fill('synthetic password');await page.locator('#login-form button').click();
  await page.waitForFunction(()=>document.getElementById('state-1')?.textContent==='ON');
  const expected={on:['rgb(194, 247, 140)','rgb(44, 59, 53)'],unknown:['rgb(198, 205, 212)','rgb(52, 60, 67)'],
   sleep:['rgb(141, 210, 255)','rgb(22, 57, 81)'],standby:['rgb(255, 190, 120)','rgb(73, 51, 30)']};
  for(const width of [1280,390,320])for(const language of ['en','de']){
   await page.setViewportSize({width,height:1100});await page.locator('#language').selectOption(language);
   const layout=await page.evaluate(()=>{
    const badge=document.getElementById('connection').getBoundingClientRect();
    const brand=document.querySelector('.brand').getBoundingClientRect();
    return {right:badge.right,left:badge.left,brandRight:brand.right,top:badge.top,brandTop:brand.top,brandBottom:brand.bottom,brandWidth:brand.width,groupWidth:document.querySelector('.header-brand').getBoundingClientRect().width,font:getComputedStyle(document.querySelector('.brand')).fontSize,overflow:document.documentElement.scrollWidth>innerWidth};
   });
   assert.ok(layout.left>=layout.brandRight,'connection status must be right of Lighthouse Bridge at '+width+' / '+language+' '+JSON.stringify(layout));
   assert.ok(layout.top<layout.brandBottom,'status and brand must share a header row');
   assert.equal(layout.overflow,false);
   for(let i=0;i<4;i++){
    const measured=await page.locator('#state-'+(i+1)).evaluate(n=>({state:n.dataset.state,fg:getComputedStyle(n).color,bg:getComputedStyle(n).backgroundColor,
     height:n.getBoundingClientRect().height}));
    assert.equal(measured.state,stations[i].observed);assert.deepEqual([measured.fg,measured.bg],expected[stations[i].observed]);
    const lum=c=>{const p=c.match(/[\d.]+/g).slice(0,3).map(Number).map(v=>{v/=255;return v<=.04045?v/12.92:((v+.055)/1.055)**2.4;});return .2126*p[0]+.7152*p[1]+.0722*p[2];};
    assert.ok((lum(measured.fg)+.05)/(lum(measured.bg)+.05)>=4.5,'badge text must remain readable');
   }
  }
  const oldHeight=await page.locator('#card-3').evaluate(n=>n.getBoundingClientRect().height);
  stations[2].requested='on';stations[2].pending=true;await page.evaluate(()=>refresh());
  assert.equal(await page.locator('#state-3').getAttribute('data-state'),'sleep','requested ON cannot color a measured SLEEP as ON');
  assert.ok(await page.locator('#card-3').evaluate(n=>n.getBoundingClientRect().height)>=oldHeight);
  stations[2].reachable=false;await page.evaluate(()=>refresh());
  await page.waitForFunction(()=>document.getElementById('state-3').dataset.state==='unknown');
  assert.equal(await page.locator('#state-3').getAttribute('data-state'),'unknown','unreachable cached SLEEP must use Unknown gray');
  stations[2].reachable=true;stations[2].pending=false;stations[2].requested=null;await page.evaluate(()=>refresh());
  await page.locator('#network-settings summary').click();
  assert.equal(await page.locator('#setup-ssid').textContent(),account.setup_ssid);
  assert.equal(await page.locator('#device-name').inputValue(),account.hostname);
  await page.locator('#network-settings summary').click();
  assert.equal(await page.locator('#station-info-3').isVisible(),false);
  await page.locator('#version-toggle').click({clickCount:3});
  await page.locator('#station-info-3').click();
  const info=page.locator('#station-info-data');
  await page.waitForFunction(()=>document.getElementById('station-info-data').textContent.includes('LHB-12345678'));
  for(const value of ['Lighthouse V2','Firmware-Version: R: 1.2.3\nM: 4.5.6\nB: 7.8.9','Kanal: 1','Example <b>Vendor</b>','Modellnummer: 112233','Seriennummer: FB0012345678','Hardware-Revision: Nicht verfügbar']){
   assert.ok((await info.textContent()).includes(value),value);
  }
  assert.equal(await info.locator('b').count(),0);
  const bounds=await page.locator('#station-info-dialog').boundingBox();
  assert.ok(bounds.x>=0&&bounds.x+bounds.width<=320);
  await page.keyboard.press('Escape');
  await page.waitForFunction(()=>!document.getElementById('station-info-dialog').open&&document.getElementById('station-info-data').textContent==='');
  await page.locator('#version-toggle').click({clickCount:3});
  assert.equal(await page.locator('#station-info-3').isVisible(),false);
  assert.equal(mutations.filter(p=>p==='/api/station-info').length,1);
  await page.locator('#matter-settings summary').click();
  await page.waitForFunction(()=>document.getElementById('matter-name').textContent==='Lighthouse-Bridge');
  assert.equal(await page.locator('#matter-name').textContent(),'Lighthouse-Bridge');
  assert.equal((await page.locator('body').innerText()).includes('Lighthouse-Bridge Matter Test'),false);
  await page.locator('#matter-settings summary').click();
  await page.waitForFunction(()=>matterInfo===null);
  await page.setViewportSize({width:390,height:1100});
  await page.screenshot({path:path.join(root,'.pio/rs-0111-mobile.png'),fullPage:true});
  await page.locator('#ota-settings').evaluate(n=>n.open=true);
  const file=Buffer.alloc(68);file.write('LHBRIDGE-ESP32-OTA1');file.writeUInt32LE(4,24);
  await page.locator('#ota-file').setInputFiles({name:'test.lhota',mimeType:'application/octet-stream',buffer:file});
  await page.locator('#ota-update').click();
  await page.waitForFunction(()=>!otaBusy&&document.getElementById('ota-status').textContent.includes('beschädigt'));
  await page.waitForTimeout(2200);assert.equal(rootReads,1,'a rejected image must not reload the page');assert.equal(uploads,1);
  invalid=false;
  const stillOld=page.waitForResponse(r=>r.url().endsWith('/api/account')&&r.status()===200);
  await page.locator('#ota-update').click();await stillOld;
  assert.equal(rootReads,1,'the old authenticated server must not trigger a premature reload');
  await page.locator('#login-panel').waitFor({state:'visible',timeout:20000});
  assert.equal(rootReads,2,'successful update reloads exactly once when the rebooted login endpoint is reachable');
  assert.ok(reconnects>=3);assert.equal(uploads,2,'reconnect polling must never repeat the upload');
  assert.equal(new URL(page.url()).searchParams.get('lang'),'de','reload retains selected language');
  // An update initiated outside this tab also invalidates its old session and
  // must replace the stale HTML; an ordinary expiry must only show login.
  installed=false;
  const signIn=async()=>{
   await page.locator('#login-password').fill('synthetic password');
   await page.locator('#login-form button').click();
   await page.locator('#dashboard').waitFor({state:'visible'});
   await page.waitForFunction(()=>loggedIn&&current?.version===pageFirmwareVersion&&!polling);
  };
  await signIn();servedVersion='99.0.1-rs-matter';status.version=servedVersion;signedIn=false;
  await page.evaluate(()=>{refresh();});
  await page.waitForFunction(()=>document.getElementById('version-toggle').textContent==='Bridge 99.0.1-rs-matter');
  assert.equal(rootReads,4,'external update probes the public version then reloads the new HTML once');
  await page.locator('#login-panel').waitFor({state:'visible'});
  await signIn();signedIn=false;
  const unchangedPage=page.waitForResponse(r=>new URL(r.url()).pathname==='/'&&r.request().resourceType()==='fetch');
  await page.evaluate(()=>refresh());await unchangedPage;
  await page.waitForTimeout(150);
  assert.equal(rootReads,5,'ordinary session expiry probes once without reloading unchanged firmware');
  await signIn();servedVersion='99.0.2-rs-matter';status.version=servedVersion;
  await page.evaluate(()=>{refresh();});
  await page.waitForFunction(()=>document.getElementById('version-toggle').textContent==='Bridge 99.0.2-rs-matter');
  assert.equal(rootReads,6,'a version mismatch on an authenticated status also reloads the page');
  assert.equal(uploads,2,'external firmware detection never writes another image');

  assert.equal(mutations.some(p=>['/api/power','/api/config','/api/matter/commissioning'].includes(p)),false);
  assert.deepEqual(errors,[]);await page.close();
  console.log('rs-matter preferences passed: EN/DE desktop/mobile status placement, measured badge colors/contrast, branding, failed-OTA no reload and successful-OTA reconnect/reload without re-upload.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
