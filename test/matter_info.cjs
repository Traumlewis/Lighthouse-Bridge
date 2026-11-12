const assert = require('node:assert/strict');
(async()=>{
  const {parseMatterInfo} = await import('../site/matter-info.mjs');
  const {validMatterParts,validUpdateParts} = await import('../site/flash-layout.mjs');
  const {watchDeviceInfo} = await import('../site/serial-info.mjs');
  const info={protocol:1,version:'0.1.0-matter',station_id:1,station_name:'Test station',wifi_connected:true,
    ready:true,paired:false,reachable:true,observed_raw:0,pairing_code:'34970112332',error:''};
  const frame=value=>'LH_MATTER_INFO '+JSON.stringify(value)+'\n';
  assert.deepEqual(parseMatterInfo(frame(info)),info);
  for(const bad of [{...info,station_id:17},{...info,pairing_code:'<script>'},
    {...info,paired:true},{...info,ready:false},{...info,reachable:'true'},{...info,version:'../../bad'}])
    assert.equal(parseMatterInfo(frame(bad)),null);
  assert.equal(parseMatterInfo('LH_INFO {}'),null);
  let stream, closed=false, commands=[];
  const port={open:async()=>{},close:async()=>{closed=true;},
    readable:new ReadableStream({start(c){stream=c;}}),
    writable:new WritableStream({write(bytes){commands.push(new TextDecoder().decode(bytes));}})};
  const got=[];
  const watcher=await watchDeviceInfo(port,v=>got.push(v),()=>{}, {parse:parseMatterInfo,query:'LH_MATTER_INFO\n'});
  const wire=frame(info); stream.enqueue(new TextEncoder().encode(wire.slice(0,30)));
  stream.enqueue(new TextEncoder().encode(wire.slice(30)));
  await new Promise(resolve=>setTimeout(resolve,20));
  assert.deepEqual(got,[info]); assert.deepEqual(commands,['LH_MATTER_INFO\n']);
  await watcher.stop(); assert.equal(closed,true);
  const manifest=require('../_site/matter-manifest.json');
  assert.equal(validMatterParts(manifest.builds[0].parts),true);
  assert.equal(validUpdateParts(manifest.builds[0].parts),false,'stable and experimental images cannot be mixed');
  const bad=structuredClone(manifest.builds[0].parts);bad[3].offset=0;
  assert.equal(validMatterParts(bad),false);
  console.log('Matter USB frames, stream handoff, malformed data and separate safe flash layout passed.');
})().catch(error=>{console.error(error);process.exitCode=1;});
