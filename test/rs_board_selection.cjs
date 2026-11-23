const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
(async()=>{const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});try{
 for(const lang of ['en','de']) {
 const page=await browser.newPage({viewport:{width:390,height:1000}}),errors=[];let unsafe=false;page.on('pageerror',e=>errors.push(e.message));
 const root=path.resolve('_site');await page.route('https://flasher.test/Lighthouse-Bridge/**',route=>{
 const relative=new URL(route.request().url()).pathname.slice('/Lighthouse-Bridge/'.length),file=path.join(root,relative||'index.html');
 if(!file.startsWith(root+path.sep)||!fs.existsSync(file))return route.fulfill({status:404});
 if(unsafe&&relative==='rs-matter-c3-supermini-manifest.json'){const m=JSON.parse(fs.readFileSync(file));m.builds[0].chipFamily='ESP32';return route.fulfill({json:m});}
 const type=/\.m?js$/.test(file)?'application/javascript':file.endsWith('.json')?'application/json':file.endsWith('.html')?'text/html':file.endsWith('.css')?'text/css':'application/octet-stream';return route.fulfill({contentType:type,body:fs.readFileSync(file)});
 });
 const initialPath=lang==='de'?'rs-matter.html':'';
 await page.goto('https://flasher.test/Lighthouse-Bridge/'+initialPath+'?lang='+lang);
 await page.waitForURL('https://flasher.test/Lighthouse-Bridge/?lang='+lang);
 await page.waitForFunction(()=>!document.querySelector('#matter-download').hidden);
 for(const board of ['esp32','weact-c3','c3-supermini','s3-supermini']){
 await page.locator('#rs-board').selectOption(board);await page.waitForFunction(()=>!document.querySelector('#matter-download').hidden);
 const manifest=board==='esp32'?'manifest.json':`rs-matter-${board}-manifest.json`;
 assert.equal(await page.locator('#matter-installer').getAttribute('manifest'),manifest);
 assert.match(await page.locator('#matter-download').getAttribute('href'),new RegExp('firmware/rs-matter/'+(board==='esp32'?'':board+'/')));
 assert.equal(await page.locator('#matter-confirm').isChecked(),false);assert.equal(await page.locator('#matter-install').isDisabled(),true);
 await page.locator('#matter-confirm').check();await page.waitForFunction(()=>!document.querySelector('#matter-install').disabled);
 }
 unsafe=true;await page.locator('#rs-board').selectOption('c3-supermini');await page.waitForFunction(()=>document.querySelector('#matter-load').classList.contains('error'));assert.equal(await page.locator('#matter-install').isDisabled(),true);assert.equal(await page.locator('#matter-ota').isHidden(),true);
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);assert.deepEqual(errors,[]);await page.close();
 }console.log('WebFlasher board selection passed: four exact manifests/downloads, consent reset, wrong-chip manifest rejected, EN/DE mobile.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1);});
