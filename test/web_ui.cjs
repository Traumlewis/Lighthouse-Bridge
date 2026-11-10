// Exercises the HTML actually compiled into the firmware. API/radio are mocked.
const { chromium } = require('playwright');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(path.join(root,'include/web_ui.h'),'utf8').split('R"LHHTML(')[1].split(')LHHTML"')[0];

(async () => {
  const browser = await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});
  try {
    const page = await browser.newPage({viewport:{width:1280,height:1100}});
    const pageErrors=[]; page.on('pageerror', error=>pageErrors.push(error.message));
    const commands=[];
    const stations=Array.from({length:4},(_,i)=>({id:i+1,name:'Basisstation '+(i+1),mac:'',address_type:0,configured:false,requested:null,pending:false,busy:false,command_sent:false,last_sent_ms:0,observed:'unknown',observed_raw:-1,observed_ms:0,error:''}));
    const data={ip:'192.168.1.42',hostname:'lighthouse-a1b2c3',wifi_connected:true,alexa_started:true,uptime_ms:90000,scanning:false,scan_finished_ms:0,scan_truncated:false,stations,discovered:[]};
    let saveFailure=false, offline=false;
    await page.route('http://bridge.test/**',async route=>{
      const request=route.request(), url=new URL(request.url());
      if(url.pathname==='/') return route.fulfill({contentType:'text/html',body:html});
      if(offline) return route.abort('connectionfailed');
      const body=request.method()==='POST'?request.postDataJSON():null;
      let status=200, response={};
      if(url.pathname==='/api/status') response=data;
      else if(url.pathname==='/api/scan') {
        status=202; response={accepted:true}; data.scan_finished_ms=88000;
        data.discovered=stations.map((s,i)=>({name:i===0?'LHB-<img src=x onerror=alert(1)>':'LHB-'+(i+1),mac:'AA:BB:CC:DD:EE:0'+(i+1),address_type:i%2,rssi:-40-i*5}));
      } else if(url.pathname==='/api/config') {
        if(saveFailure) {status=409;response={error:'BLE beschaeftigt; nach Abschluss erneut speichern'};}
        else {body.stations.forEach((s,i)=>Object.assign(stations[i],s,{configured:Boolean(s.mac)}));response={saved:true};}
      } else if(url.pathname==='/api/power') {
        commands.push(body); status=202;response={accepted:true};
        for(const s of stations) if(body.station==='all'||s.id===body.station) Object.assign(s,{requested:body.state,command_sent:true,observed:body.state,observed_raw:body.state==='on'?11:0,observed_ms:89000});
      } else if(url.pathname==='/api/wifi') {status=202;response={accepted:true,setup_ssid:'Lighthouse-Setup-A1B2C3'};}
      else {status=404;response={error:'unknown'};}
      await route.fulfill({status,contentType:'application/json',body:JSON.stringify(response)});
    });
    await page.goto('http://bridge.test/');
    await page.waitForFunction(()=>document.getElementById('connection').textContent==='Im Heimnetz');
    assert.equal(await page.locator('.card').count(),4);
    assert.equal(await page.locator('#all-on').isDisabled(),true);
    assert.equal(await page.locator('#on-1').isDisabled(),true);
    await page.locator('#configuration summary').click();
    await page.locator('#scan').click();
    await page.waitForFunction(()=>document.querySelectorAll('#scan-results li').length===4);
    assert.equal(await page.locator('#scan-results img').count(),0,'scan names must be text, never HTML');
    for(let i=1;i<=4;i++) await page.locator('#pick-'+i).selectOption('AA:BB:CC:DD:EE:0'+i);
    assert.equal(await page.locator('#edit-type-2').inputValue(),'1','preserve random BLE address type');
    await page.locator('#edit-name-1').fill('Raumecke links');
    await page.waitForTimeout(2800); // Polling must not overwrite unsaved inputs.
    assert.equal(await page.locator('#edit-name-1').inputValue(),'Raumecke links');
    saveFailure=true; await page.locator('#save').click();
    await page.waitForFunction(()=>document.getElementById('message').textContent.includes('BLE beschaeftigt'));
    assert.equal(await page.locator('#edit-name-1').inputValue(),'Raumecke links');
    saveFailure=false; await page.locator('#save').click();
    await page.waitForFunction(()=>document.getElementById('name-1').textContent==='Raumecke links');
    await page.locator('#all-on').click();
    await page.waitForFunction(()=>document.getElementById('state-4').textContent==='ON');
    assert.deepEqual(commands.at(-1),{station:'all',state:'on'});
    await page.locator('#sleep-2').click();
    await page.waitForFunction(()=>document.getElementById('state-2').textContent==='SLEEP');
    assert.deepEqual(commands.at(-1),{station:2,state:'sleep'});
    await page.locator('#all-sleep').click();
    await page.waitForFunction(()=>document.getElementById('state-4').textContent==='SLEEP');
    assert.deepEqual(commands.at(-1),{station:'all',state:'sleep'});
    assert.ok(commands.every(c=>c.state==='on'||c.state==='sleep'));
    await page.locator('#configuration summary').click();
    await page.screenshot({path:path.join(root,'docs/webinterface-desktop.png'),fullPage:true});
    await page.setViewportSize({width:390,height:844});
    assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false,'mobile overflow');
    await page.screenshot({path:path.join(root,'docs/webinterface-mobile.png'),fullPage:true});
    stations[2].error='BLE-Verbindung fehlgeschlagen';stations[2].command_sent=false;
    await page.waitForFunction(()=>document.getElementById('status-3').textContent.includes('BLE-Verbindung fehlgeschlagen'));
    assert.equal(await page.locator('#status-3').evaluate(el=>el.classList.contains('warning')),true);
    offline=true;
    await page.waitForFunction(()=>document.getElementById('connection').textContent==='Nicht erreichbar');
    offline=false;
    await page.waitForFunction(()=>document.getElementById('connection').textContent==='Im Heimnetz');
    await page.getByText('WLAN & lokale API',{exact:true}).click();
    page.once('dialog',dialog=>dialog.accept());
    await page.locator('#wifi').click();
    await page.waitForFunction(()=>document.getElementById('message').textContent.includes('Lighthouse-Setup-A1B2C3'));
    assert.deepEqual(pageErrors,[]);
    console.log('Web UI passed: setup, 4 slots, MAC/type selection, form persistence, ON/SLEEP, errors, WiFi setup, mobile layout.');
  } finally { await browser.close(); }
})().catch(error=>{console.error(error);process.exitCode=1;});
