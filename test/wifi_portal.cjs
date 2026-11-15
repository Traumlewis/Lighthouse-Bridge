// Real pinned vendor CSS plus firmware customization; form content is simulated.
const {chromium}=require('playwright'),fs=require('fs'),assert=require('node:assert/strict');
function literals(source){return [...source.matchAll(/"(?:\\.|[^"\\])*"/g)].map(m=>JSON.parse(m[0])).join('');}
(async()=>{
 const vendor=fs.readFileSync('.pio/libdeps/esp32dev/WiFiManager/wm_strings_en.h','utf8');
 const source=fs.readFileSync('src/main.cpp','utf8');
 const style=literals(vendor.split('const char HTTP_STYLE[]')[1].split('"</style>";')[0])+ '</style>';
 const head=literals(source.split('wifiManager.setCustomHeadElement(')[1].split(');\n')[0]);
 const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});
 try{const page=await browser.newPage();await page.setContent('<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">'+style+head+'</head><body><div class="wrap"><h1>Lighthouse Bridge</h1><a href="#">Home network</a><form><label>Wi-Fi name<input value="Example network"></label><label>Password<input type="password"></label><button>Save</button></form><div class="msg S"><h4>Saved</h4>Connecting to Wi-Fi</div></div></body></html>');
  assert.equal(await page.evaluate(()=>getComputedStyle(document.body).backgroundColor),'rgb(16, 24, 23)');
  assert.equal(await page.locator('button').evaluate(n=>getComputedStyle(n).backgroundColor),'rgb(194, 247, 140)');
  for(const width of [1280,390]){await page.setViewportSize({width,height:844});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);}
  // The rs root uses its own local markup; vendor configuration forms keep their routes.
  const rsSource=fs.readFileSync('include/rs_wifi_portal.h','utf8');
  const rsHead=rsSource.match(/R"LHHEAD\(([\s\S]*?)\)LHHEAD"/)[1];
  const rsRoot=rsSource.match(/R"LHPAGE\(([\s\S]*?)\)LHPAGE"/)[1].replace('%PORTAL_HEAD%',rsHead).replace('%SSID%','Lighthouse-Setup-EXAMPLE');
  const localSource=fs.readFileSync('include/local_wifi_page.h','utf8');
  const confirm=localSource.match(/R"LHLOCAL\(([\s\S]*?)\)LHLOCAL"/)[1].replace('%PORTAL_HEAD%',rsHead).replace('%TOKEN%','synthetic-token');
  const saved=localSource.match(/R"LHSAVED\(([\s\S]*?)\)LHSAVED"/)[1].replace('%PORTAL_HEAD%',rsHead);
  const context=await browser.newContext();const portal=await context.newPage(),errors=[],requests=[];
  portal.on('pageerror',e=>errors.push(e.message));
  await portal.addInitScript(()=>{const original=setTimeout;window.setTimeout=(fn,ms,...args)=>ms===5000?0:original(fn,ms,...args);});
  await portal.route('http://portal.test/**',route=>{const r=route.request(),p=new URL(r.url()).pathname;requests.push({method:r.method(),p});return route.fulfill({contentType:'text/html',body:p==='/local'?confirm:p==='/saved'?saved:p==='/wifi'?'<!doctype html><html><head>'+style+rsHead+'</head><body><div class="wrap"><h1>Home Wi-Fi</h1><form><label>Wi-Fi name<input value="Example network"></label><label>Password<input type="password"></label><button>Save</button></form></div></body></html>':p==='/info'||p==='/exit'?'<p>Navigation target</p>':rsRoot});});
  for(const lang of ['en','de'])for(const width of [1280,390,320]){
   await portal.setViewportSize({width,height:1000});await portal.goto('http://portal.test/');await portal.locator('#portal-language').selectOption(lang);
   assert.equal(await portal.locator('html').getAttribute('lang'),lang);assert.equal(await portal.locator('#setup-ssid').textContent(),'Lighthouse-Setup-EXAMPLE');
   assert.equal(await portal.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
   const gaps=await portal.locator('.mode-card').evaluateAll(nodes=>nodes[1].getBoundingClientRect().top-nodes[0].getBoundingClientRect().bottom);
   assert.ok(gaps>=12,'mode cards must have a visible gap');
   const secondary=await portal.locator('.portal-secondary button').evaluateAll(nodes=>nodes.map(n=>({x:n.getBoundingClientRect().x,right:n.getBoundingClientRect().right,top:n.getBoundingClientRect().top,height:n.getBoundingClientRect().height})));
   assert.ok(secondary[1].x-secondary[0].right>=12);assert.equal(secondary[0].top,secondary[1].top);assert.ok(secondary.every(n=>n.height>=44));
   const labels=await portal.locator('.mode-description').allTextContents();assert.ok(labels.every(s=>s.length>15));assert.equal((await portal.locator('body').textContent()).includes('Not connected'),false);
   if(width===390||width===1280)await portal.screenshot({path:'.pio/rs-0117-portal-'+lang+'-'+(width===390?'mobile':'desktop')+'.png',fullPage:true});
   await portal.locator('form[action="/local"] button').click();assert.equal(new URL(portal.url()).pathname,'/local');assert.equal(await portal.locator('html').getAttribute('lang'),lang,'confirmation preserves selected language');assert.equal(await portal.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
   await portal.goto('http://portal.test/saved');assert.equal(await portal.locator('html').getAttribute('lang'),lang);assert.equal(await portal.locator('#saved-open').getAttribute('href'),'http://192.168.4.1/');assert.equal(await portal.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
  }
  for(const route of ['/wifi','/info','/exit']){await portal.goto('http://portal.test/');await portal.locator('form[action="'+route+'"] button').click();assert.equal(new URL(portal.url()).pathname,route);}
  assert.equal(requests.some(r=>r.method!=='GET'),false,'presentation and navigation do not change mode');assert.deepEqual(errors,[]);await context.close();
  console.log('Wi-Fi portal passed: stable vendor styling, rs EN/DE layouts at 1280/390/320, separated actions, language continuity, local navigation and synthetic previews.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
