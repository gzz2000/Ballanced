// Exercise independent SDK output through the public importer and offline cache.
import {chromium} from 'playwright';import {readFile,mkdir,writeFile} from 'node:fs/promises';
const [url,out,packagePath]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false},logs=[];let page;
try{
 const context=await browser.newContext();page=await context.newPage({viewport:{width:1024,height:827}});
 page.on('console',m=>logs.push(m.text()));page.on('pageerror',e=>logs.push(e.stack));
 const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 await page.goto(url);await ready();
 result.defaults=await page.evaluate(()=>ballance.player.webMods.map(x=>({id:x.id,source:x.source})));
 result.jsServices=await page.evaluate(()=>{
  const m=ballance.player,result={};for(const name of ['emscripten_get_now','random_get','__syscall_uname']){const ptr=m.ccall('BallanceWebModSymbol','number',['string'],[name]);if(!ptr)throw Error('Missing retained JS service '+name);result[name]=true;if(name==='emscripten_get_now'&&!Number.isFinite(m.bwModFunction(ptr)()))throw Error('Invalid JS service signature')};return result;
 });
 if(result.defaults.length!==3||result.defaults.some(x=>x.source!=='default'))throw Error('Default mod set incorrect');
 // Validate parser failures without installing/executing any malformed module.
 result.rejections=await page.evaluate(async bytes=>{
  const {unpackMod,validateModSet}=await import('./mods.mjs'),sdk=ballance.player.webModCatalog.sdk;
  const raw=new Uint8Array(await new Response(new Blob([new Uint8Array(bytes)]).stream().pipeThrough(new DecompressionStream('gzip'))).arrayBuffer());
  const n=new DataView(raw.buffer).getUint32(8,true),base=JSON.parse(new TextDecoder().decode(raw.slice(12,12+n))),payload=raw.slice(12+n),results=[];
  for(const kind of ['sdk','abi-major','abi-revision','path','hash','truncated','duplicate']){
   const m=structuredClone(base);if(kind==='sdk'){delete m.abi;m.sdk='wrong-sdk'}if(kind==='abi-major')m.abi={id:'incompatible-major',minRevision:0};if(kind==='abi-revision')m.abi={id:sdk.abi.id,minRevision:sdk.abi.revision+1};if(kind==='path')m.files[0].path='../escape.wasm';if(kind==='hash')m.files[0].sha256='0'.repeat(64);if(kind==='duplicate')m.files.push(m.files[0]);
   const meta=new TextEncoder().encode(JSON.stringify(m)),data=new Uint8Array(12+meta.length+payload.length-(kind==='truncated'?1:0));data.set(raw.slice(0,8));new DataView(data.buffer).setUint32(8,meta.length,true);data.set(meta,12);data.set(payload.subarray(0,data.length-12-meta.length),12+meta.length);
   const zipped=new Uint8Array(await new Response(new Blob([data]).stream().pipeThrough(new CompressionStream('gzip'))).arrayBuffer());
   try{await unpackMod(zipped,sdk);throw Error('ACCEPTED '+kind)}catch(e){if(e.message.startsWith('ACCEPTED'))throw e;results.push({kind,error:e.message})}
  }
  for(const dep of [{id:'MissingMod',version:null},{id:'BallanceTAS',version:'incompatible'}]){
   const manifest={...base,requires:[dep]};try{validateModSet([...ballance.player.webMods.map(manifest=>({manifest})),{manifest}]);throw Error('ACCEPTED dependency')}catch(e){if(e.message.startsWith('ACCEPTED'))throw e;results.push({kind:'dependency',error:e.message})}
  }
  const provenance={...base,sdk:'a-different-build-fingerprint',abi:{id:sdk.abi.id,minRevision:0}};
  const metadata=new TextEncoder().encode(JSON.stringify(provenance)),data=new Uint8Array(12+metadata.length+payload.length);
  data.set(raw.slice(0,8));new DataView(data.buffer).setUint32(8,metadata.length,true);data.set(metadata,12);data.set(payload,12+metadata.length);
  const zipped=new Uint8Array(await new Response(new Blob([data]).stream().pipeThrough(new CompressionStream('gzip'))).arrayBuffer());
  await unpackMod(zipped,sdk);results.push({kind:'compatible-abi-different-sdk',accepted:true});
  return results;
 },[...await readFile(packagePath)]);
 await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#import-mods').setInputFiles(packagePath);
 await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);
 if(!(await page.locator('#mods-status').textContent()).includes('已保存'))throw Error('Import failed');
 await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();
 const inspect=()=>page.evaluate(()=>({mods:ballance.player.webMods.map(x=>x.id),log:ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'})}));
 let state=await inspect();if(!state.mods.includes('WebHello')||!state.log.includes('WEB_HELLO_LOADED resource=hello-from-package')||!state.log.includes('WEB_HELLO_MENU_CALLBACK'))throw Error('Third-party mod/resource/callback unavailable');
 result.imported=state.mods;
 await page.waitForFunction(()=>!!navigator.serviceWorker.controller,null,{timeout:120000});
 await context.setOffline(true);await page.reload();await ready();state=await inspect();
 if(!state.mods.includes('WebHello')||!state.log.includes('WEB_HELLO_LOADED resource=hello-from-package'))throw Error('Offline user mod unavailable');result.offline=true;
 await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#mod-list button').click();await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);
 await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();state=await inspect();
 if(state.mods.includes('WebHello')||state.log.includes('WEB_HELLO_LOADED'))throw Error('Removed mod remains active');result.removedOffline=true;
 // Native IMod shutdown must still run although code stays linked until teardown.
 await context.setOffline(false);await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#import-mods').setInputFiles(packagePath);await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));
 state=await inspect();if(!state.log.includes('WEB_HELLO_UNLOADED'))throw Error('OnUnload not called');result.unloaded=true;
 result.passed=true;
}catch(e){result.error=String(e.stack||e);if(page){result.status=await page.locator('#status').textContent().catch(()=>null);await page.screenshot({path:out+'/failure.png'}).catch(()=>{})}process.exitCode=1}
finally{await writeFile(out+'/result.json',JSON.stringify(result,null,2));await writeFile(out+'/browser.log',logs.join('\n'));console.log(JSON.stringify(result));await browser.close()}
