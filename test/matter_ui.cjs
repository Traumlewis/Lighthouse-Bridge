const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const site=path.resolve(__dirname,'../_site');
const version=JSON.parse(fs.readFileSync(path.join(site,'matter-manifest.json'),'utf8')).version;
(async()=>{
 const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});
 try{
  const page=await browser.newPage({viewport:{width:390,height:844}}),errors=[],requests=[];
  let unsafe=false;
  page.on('pageerror',e=>errors.push(e.message));page.on('request',r=>requests.push(r.url()));
  await page.addInitScript(version=>{
   window.serialRequests=0;window.queries=[];window.copied='';
   Object.defineProperty(navigator,'clipboard',{value:undefined,configurable:true});
   document.execCommand=()=>{window.copied=document.activeElement.value;return true;};
   const info={protocol:1,version,bridge_name:'Lighthouse-Bridge Matter Test',station_id:1,station_name:'Owner name',wifi_connected:true,
    ready:true,paired:false,reachable:true,observed_raw:0,pairing_code:'34970112332',error:'',
    stations:[{id:1,name:'Owner name',reachable:true,observed_raw:0},{id:4,name:'Another station',reachable:true,observed_raw:11}]};
   const port={open:async function(){
    this.readable=new ReadableStream({start(c){window.usbController=c;}});
    this.writable=new WritableStream({write(bytes){const q=new TextDecoder().decode(bytes);window.queries.push(q);
     if(q==='LH_MATTER_INFO\n')window.usbController.enqueue(new TextEncoder().encode('LH_MATTER_INFO '+JSON.stringify(info)+'\n'));}});
   },close:async()=>{}};
   window.fakePort=port;
   window.sendPaired=()=>window.usbController.enqueue(new TextEncoder().encode('LH_MATTER_INFO '+JSON.stringify({...info,paired:true,pairing_code:null})+'\n'));
   window.sendSleep=()=>window.usbController.enqueue(new TextEncoder().encode('LH_MATTER_INFO '+JSON.stringify({...info,
    stations:[info.stations[0],{...info.stations[1],observed_raw:0}]})+'\n'));
   Object.defineProperty(navigator,'serial',{configurable:true,value:{requestPort:async()=>{window.serialRequests++;return port;},getPorts:async()=>[],addEventListener:()=>{},removeEventListener:()=>{}}});
  },version);
  await page.route('https://flasher.test/Lighthouse-Bridge/**',async route=>{
   const relative=new URL(route.request().url()).pathname.slice('/Lighthouse-Bridge/'.length)||'index.html';
   const file=path.join(site,relative);
   if(!file.startsWith(site+path.sep)||!fs.existsSync(file))return route.fulfill({status:404,body:'missing'});
   if(unsafe&&relative==='matter-manifest.json'){
    const manifest=JSON.parse(fs.readFileSync(file));manifest.builds[0].parts[3].offset=0;
    return route.fulfill({json:manifest});
   }
   const type=/\.m?js$/.test(relative)?'application/javascript':relative.endsWith('.css')?'text/css':relative.endsWith('.json')?'application/json':relative.endsWith('.html')?'text/html':'application/octet-stream';
   return route.fulfill({contentType:type,body:fs.readFileSync(file)});
  });
  await page.goto('https://flasher.test/Lighthouse-Bridge/matter.html');
  await page.waitForFunction(version=>document.querySelector('#matter-load').textContent.includes(version),version);
  await page.waitForFunction(()=>customElements.get('esp-web-install-button'));
  assert.equal(await page.locator('html').getAttribute('lang'),'en');
  assert.equal(await page.evaluate(()=>window.serialRequests),0);
  assert.equal(await page.locator('#matter-install').isDisabled(),true);
  await page.locator('#matter-confirm').check();assert.equal(await page.locator('#matter-install').isEnabled(),true);
  await page.locator('#language').selectOption('de');
  assert.equal(await page.locator('#matter-confirm').isChecked(),true);
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
  await page.locator('#matter-read').click();
  await page.waitForFunction(()=>document.querySelector('#matter-code').textContent==='34970112332');
  assert.equal(await page.locator('#matter-install').isDisabled(),true,'reader owns the USB port');
  assert.equal(await page.locator('#matter-read').isDisabled(),true);
  assert.equal(await page.locator('#matter-station').textContent(),'2');
  assert.equal(await page.locator('#matter-bridge').textContent(),'Lighthouse-Bridge Matter Test');
  assert.equal(await page.locator('#matter-stations li').count(),2);
  assert.match(await page.locator('#matter-stations').textContent(),/1 · Owner name.*SLEEP/);
  assert.match(await page.locator('#matter-stations').textContent(),/4 · Another station.*AN/);
  await page.locator('#matter-code').click();assert.equal(await page.evaluate(()=>window.copied),'34970112332');
  await page.locator('#language').selectOption('en');
  assert.equal(await page.locator('#matter-code').textContent(),'34970112332');
  assert.equal(await page.evaluate(()=>window.serialRequests),1,'language change keeps the connection');
  assert.equal(await page.evaluate(()=>JSON.stringify(localStorage).includes('34970112332')),false);
  assert.equal(requests.some(url=>url.includes('34970112332')),false);
  await page.evaluate(()=>window.sendSleep());
  await page.waitForFunction(()=>document.querySelector('#matter-stations li:last-child').textContent.includes('SLEEP'));
  await page.evaluate(()=>window.sendPaired());await page.waitForFunction(()=>document.querySelector('#matter-code').disabled);
  await page.locator('#matter-stop').click();await page.waitForFunction(()=>!document.querySelector('#matter-read').disabled);
  assert.equal(await page.locator('#matter-install').isEnabled(),true);
  assert.ok((await page.evaluate(()=>window.queries)).every(q=>q==='LH_MATTER_INFO\n'),'no reset or power command sent');
  await page.evaluate(()=>{
   const dialog=document.createElement('ewt-install-dialog');dialog.port=window.fakePort;document.body.append(dialog);
   dialog.dispatchEvent(new CustomEvent('closed',{bubbles:true}));dialog.remove();
  });
  await page.waitForFunction(()=>document.querySelector('#matter-read').disabled);
  await page.waitForFunction(()=>document.querySelector('#matter-code').textContent==='34970112332');
  assert.equal(await page.evaluate(()=>window.serialRequests),1,'installer handoff reuses its port without another picker');
  await page.locator('#matter-stop').click();await page.waitForFunction(()=>!document.querySelector('#matter-read').disabled);
  unsafe=true;await page.reload();await page.waitForFunction(()=>document.querySelector('#matter-load').classList.contains('error'));
  await page.locator('#matter-confirm').check();assert.equal(await page.locator('#matter-install').isDisabled(),true);
  assert.deepEqual(errors,[]);
  console.log('Matter WebFlasher passed: consent, separate manifest, EN/DE mobile, USB ownership, pairing-code privacy and unsafe-layout rejection. No device flashed.');
 }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
