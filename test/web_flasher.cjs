const {chromium} = require('playwright');
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname,'..');
const site = path.join(root,'_site');
const expectedVersion = 'v' + JSON.parse(fs.readFileSync(path.join(site,'manifest.json'),'utf8')).version;

(async()=>{
  const browser = await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});
  try {
    const page = await browser.newPage({viewport:{width:1280,height:1100}});
    const errors=[];page.on('pageerror',error=>errors.push(error.message));
    let firmwareMissing=false;
    await page.addInitScript(()=>{
      window.copiedValue='';Object.defineProperty(navigator,'clipboard',{value:undefined,configurable:true});document.execCommand=()=>{window.copiedValue=document.activeElement.value;return true;};
      window.serialRequests=0;
      window.usbMode=false;
      const info={protocol:1,version:'0.2.0',api_key:'1234567890abcdef12345678',setup_ssid:'Lighthouse-Setup-EXAMPLE',wifi_connected:false,url:null};
      window.fakePort={
        open:async function(){
          this.readable=new ReadableStream({start(c){window.usbController=c;}});
          this.writable=new WritableStream({write(bytes){
            if(new TextDecoder().decode(bytes)==='LH_INFO\n')window.usbController.enqueue(new TextEncoder().encode('LH_INFO '+JSON.stringify(info)+'\n'));
          }});
        },close:async()=>{}
      };
      window.sendWifiAddress=()=>window.usbController.enqueue(new TextEncoder().encode('LH_INFO '+JSON.stringify({...info,wifi_connected:true,url:'http://192.168.1.42/'})+'\n'));
      Object.defineProperty(navigator,'serial',{configurable:true,value:{
        requestPort:async()=>{window.serialRequests++;if(window.usbMode)return window.fakePort;throw new DOMException('Test: no hardware selected','NotFoundError');},
        getPorts:async()=>[],addEventListener:()=>{},removeEventListener:()=>{}
      }});
    });
    await page.route('https://flasher.test/Lighthouse-Bridge/**', async route=>{
      const url=new URL(route.request().url());
      let relative=decodeURIComponent(url.pathname.slice('/Lighthouse-Bridge/'.length))||'index.html';
      const file=path.join(site,relative);
      if(!file.startsWith(site+path.sep)||!fs.existsSync(file)||(firmwareMissing&&relative.endsWith('.bin')))
        return route.fulfill({status:404,body:'missing'});
      const type=/\.m?js$/.test(relative)?'application/javascript':relative.endsWith('.css')?'text/css':relative.endsWith('.json')?'application/json':relative.endsWith('.html')?'text/html':'application/octet-stream';
      return route.fulfill({status:200,contentType:type,body:fs.readFileSync(file)});
    });
    await page.goto('https://flasher.test/Lighthouse-Bridge/');
    await page.waitForFunction(version=>document.querySelector('#version').textContent===version,expectedVersion);
    await page.waitForFunction(()=>customElements.get('esp-web-install-button'));
    assert.equal(await page.locator('html').getAttribute('lang'),'en');
    assert.match(await page.locator('h1').innerText(),/Your VR room/);
    assert.match(await page.locator('meta[name="description"]').getAttribute('content'),/Install Lighthouse Bridge/);
    assert.equal(await page.locator('#install').isDisabled(),true);
    assert.equal(await page.evaluate(()=>window.serialRequests),0);
    await page.locator('#board-confirm').check();
    assert.equal(await page.locator('#install').isEnabled(),true);
    await page.locator('#language').selectOption('de');
    assert.equal(await page.locator('html').getAttribute('lang'),'de');
    assert.match(await page.locator('h1').innerText(),/Dein VR-Raum/);
    assert.match(await page.locator('#load-status').innerText(),/Bereit/);
    assert.match(await page.locator('meta[name="description"]').getAttribute('content'),/Bis zu 16 Valve/);
    assert.equal(new URL(page.url()).searchParams.get('lang'),'de');
    assert.equal(await page.locator('#board-confirm').isChecked(),true);
    assert.equal(await page.locator('#install').isEnabled(),true);
    await page.goto('https://flasher.test/Lighthouse-Bridge/');
    await page.waitForFunction(version=>document.querySelector('#version').textContent===version,expectedVersion);
    assert.equal(await page.locator('html').getAttribute('lang'),'de','saved preference survives navigation');
    await page.goto('https://flasher.test/Lighthouse-Bridge/?lang=en');
    await page.waitForFunction(version=>document.querySelector('#version').textContent===version,expectedVersion);
    assert.equal(await page.locator('html').getAttribute('lang'),'en','URL overrides saved preference');
    await page.locator('#board-confirm').check();
    const download=await page.locator('#download').getAttribute('href');
    assert.equal(download,'firmware/lighthouse-bridge-esp32-merged.bin');
    assert.equal(await page.locator('#ota-download').getAttribute('href'),'firmware/lighthouse-bridge-'+expectedVersion+'.lhota');
    await page.screenshot({path:path.join(root,'docs/webflasher-desktop.png'),fullPage:true});
    await page.setViewportSize({width:390,height:844});
    assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
    await page.locator('#language').selectOption('de');
    assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false,'German mobile layout');
    await page.screenshot({path:path.join(root,'docs/webflasher-mobile.png'),fullPage:true});
    await page.evaluate(()=>{window.usbMode=true;});
    await page.locator('#usb-read').click();
    await page.waitForFunction(()=>document.getElementById('usb-key').textContent==='1234567890abcdef12345678');
    assert.equal(await page.locator('#install').isDisabled(),true,'USB reader owns the port');
    assert.equal(await page.locator('#usb-url').isHidden(),true);
    await page.locator('#copy-key').click();assert.equal(await page.evaluate(()=>window.copiedValue),'1234567890abcdef12345678');
    await page.evaluate(()=>window.sendWifiAddress());
    await page.waitForFunction(()=>document.getElementById('usb-url').getAttribute('href')==='http://192.168.1.42/');
    assert.equal(await page.evaluate(()=>JSON.stringify(localStorage).includes('1234567890abcdef12345678')),false);
    await page.locator('#usb-stop').click();
    await page.waitForFunction(()=>!document.getElementById('install').disabled);
    // Closing the upstream installer reuses its granted port without another picker.
    await page.evaluate(()=>{const dialog=document.createElement('ewt-install-dialog');dialog.port=window.fakePort;document.body.appendChild(dialog);dialog.dispatchEvent(new CustomEvent('closed',{bubbles:true}));dialog.remove();});
    await page.waitForFunction(()=>!document.getElementById('usb-stop').hidden);
    assert.equal(await page.evaluate(()=>window.serialRequests),1);
    await page.locator('#usb-stop').click();
    await page.waitForFunction(()=>!document.getElementById('install').disabled);
    await page.evaluate(()=>{window.usbMode=false;});
    await page.locator('#install').click();
    await page.waitForFunction(()=>window.serialRequests===2);
    // Actual upstream lazy imports run; requestPort is safely stubbed above.
    await page.waitForTimeout(1200);
    assert.deepEqual(errors,[]);
    firmwareMissing=true;
    await page.goto('https://flasher.test/Lighthouse-Bridge/');
    await page.waitForFunction(()=>document.querySelector('#version').textContent==='Nicht verfügbar');
    assert.match(await page.locator('#load-status').innerText(),/Firmware nicht verfügbar/);
    await page.locator('#language').selectOption('en');
    assert.equal(await page.locator('#version').innerText(),'Unavailable');
    assert.match(await page.locator('#load-status').innerText(),/Firmware unavailable/);
    await page.locator('#board-confirm').check();
    assert.equal(await page.locator('#install').isDisabled(),true);
    assert.equal(await page.locator('#download').isHidden(),true);
    assert.equal(await page.evaluate(()=>window.serialRequests),0);
    console.log('WebFlasher passed: English default, German switch, saved language, URL override, translated errors, real local modules, consent gate, USB picker invocation, both mobile layouts, USB key/address display and port handoff. No real serial port accessed.');
  } finally {await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
