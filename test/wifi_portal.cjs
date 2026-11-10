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
  console.log('Wi-Fi portal CSS passed dashboard palette, button colors and mobile width using pinned vendor styles.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
