import {chromium} from 'playwright';
import {createServer} from 'node:http';
import {readFile,writeFile,stat} from 'node:fs/promises';
import {createReadStream} from 'node:fs';
import {resolve,sep,join} from 'node:path';
const [siteArg,output,mapPath,tapePath]=process.argv.slice(2);
if(!tapePath)throw new Error('Usage: qualify-offline.mjs site result.json map.nmo tape.tas');
const site=resolve(siteArg);
const server=createServer(async(req,res)=>{try{
 const name=decodeURIComponent(new URL(req.url,'http://localhost').pathname);
 const path=resolve(site,'.'+(name.endsWith('/')?name+'index.html':name));
 if(!path.startsWith(site+sep))throw new Error('Invalid path');
 await stat(path);res.setHeader('Content-Type',path.endsWith('.mjs')?'text/javascript':path.endsWith('.js')?'text/javascript':path.endsWith('.wasm')?'application/wasm':path.endsWith('.html')?'text/html':path.endsWith('.css')?'text/css':path.endsWith('.json')?'application/json':'application/octet-stream');
 createReadStream(path).pipe(res);
}catch{res.writeHead(404).end()}});
await new Promise(r=>server.listen(0,'127.0.0.1',r));
const browser=await chromium.launch({executablePath:process.env.CHROME_PATH||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true,args:['--autoplay-policy=document-user-activation-required']});
const result={passed:false},logs=[];
try{
 const context=await browser.newContext(),page=await context.newPage();

 if(process.env.BALLANCE_TEST_WEBGL==='1')await page.addInitScript(()=>Object.defineProperty(navigator,'gpu',{value:{requestAdapter:async()=>null}}));
 await page.addInitScript(()=>{
  window.cacheTransfers=[];
  navigator.serviceWorker?.addEventListener('message',event=>{
   if(event.data?.cacheProgress!==undefined)cacheTransfers.push({...event.data,gameStarted:!!window.ballance?.running});
  });
 });
 page.on('console',message=>{logs.push(message.text());if(message.type()==='error')console.error(message.text())});page.on('pageerror',error=>logs.push('PAGE ERROR '+error));
 const destination=new URL(process.env.BALLANCE_TEST_URL||'http://127.0.0.1:'+server.address().port);
 if(process.env.BALLANCE_TEST_WEBGL==='1')destination.searchParams.set('renderer','auto');
 await page.goto(destination.href);
 // Playwright evaluate grants a CDP user gesture. Observe startup without that
 // synthetic activation so the autoplay-policy test is meaningful.
 const cdp=await context.newCDPSession(page);
 for(;;){
  const {result:probe}=await cdp.send('Runtime.evaluate',{userGesture:false,returnByValue:true,expression:`({
   ready:window.ballance?.gpu.frames>=30&&document.querySelector('#offline').textContent.includes('离线缓存已就绪'),
   activated:navigator.userActivation.hasBeenActive,audio:window.ballance?.player.SDL3?.audioContext?.state,
   startButtons:document.querySelectorAll('#start').length,restartHidden:document.querySelector('#restart-screen').hidden})`});
  if(probe.value.ready){result.autostart=probe.value;break}
  await new Promise(r=>setTimeout(r,100));
 }
 if(result.autostart.activated||result.autostart.startButtons||!result.autostart.restartHidden)throw new Error('Game did not start without user activation');
 await page.keyboard.press('Shift');
 await page.waitForFunction(()=>ballance.player.SDL3?.audioContext?.state==='running',{},{timeout:10000});
 result.audioAfterGesture='running';

 result.cacheTransfer=await page.evaluate(()=>({first:cacheTransfers[0],last:cacheTransfers.at(-1),
  monotonic:cacheTransfers.every((v,i)=>!i||v.cacheBytes>=cacheTransfers[i-1].cacheBytes)}));
 if(result.cacheTransfer.first.gameStarted)throw new Error('Progress started after game');
 if(!result.cacheTransfer.monotonic||result.cacheTransfer.last?.cacheBytes!==result.cacheTransfer.last?.totalBytes)throw new Error('Cache byte progress incomplete');
 await page.click('#toggle-sidebar');await page.click('#tab-data');
 await page.locator('#maps').setInputFiles(mapPath);await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('导入完成'),{},{timeout:0});
 await page.evaluate(()=>document.querySelector('#status').textContent='Testing next import');
 await page.locator('#tapes').setInputFiles(tapePath);await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('导入完成'),{},{timeout:0});
 if(process.env.BALLANCE_TEST_EDITOR==='1') {
  await page.click('#tab-editor');
  await page.selectOption('#records',{label:tapePath.split('/').pop()});await page.click('#load-record');
  await page.waitForFunction(()=>document.querySelector('#record-info').textContent.includes('次采样'),{},{timeout:0});
  result.edit=await page.evaluate(()=>({first:ballance.player._BML_WebTASKeys(0),delta:ballance.player._BML_WebTASDelta(0)}));
  await page.locator('#range-tools').evaluate(e=>e.open=true);await page.fill('#first','10');await page.fill('#last','12');await page.click('#inspect');
  await page.locator('#keys input').first().check();await page.fill('#delta','4.25');await page.click('#apply');
  result.edit.changed=await page.evaluate(()=>({keys:ballance.player._BML_WebTASKeys(10),delta:ballance.player._BML_WebTASDelta(10)}));
  if(!(result.edit.changed.keys&1)||result.edit.changed.delta!==4.25)throw new Error('Editor range did not apply');
  await page.click('#undo');await page.fill('#copy-name','browser-roundtrip.tas');await page.click('#save-record');
  await page.waitForFunction(()=>document.querySelector('#editor-status').textContent.includes('副本已保存'),{},{timeout:0});
  // Compare decompressed legacy bytes using independent Node zlib below.
  result.edit.copy=await page.evaluate(()=>Array.from(ballance.player.FS.readFile('/user/ModLoader/TAS/browser-roundtrip.tas')));
  const {inflateSync}=await import('node:zlib');
  const original=await readFile(tapePath),copy=Buffer.from(result.edit.copy);delete result.edit.copy;
  if(!original.subarray(0,4).equals(copy.subarray(0,4))||!inflateSync(original.subarray(4)).equals(inflateSync(copy.subarray(4))))throw new Error('Editor undo/save changed original record data');
  result.edit.roundtrip=true;
 }
 if(process.env.BALLANCE_TEST_PLAY==='1') {
  const advance=async count=>{
   const target=await page.evaluate(n=>ballance.gpu.frames+n,count);
   await page.waitForFunction(t=>ballance.gpu.frames>=t,target,{timeout:0});
  };
  await page.selectOption('#level','1');await page.click('#play-record');
  await page.waitForFunction(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('precise x87 (TAS)'),{},{timeout:0});
  await advance(300);
  for(const action of ['capture','restore','restore','capture','restore','restore']) {
   await page.focus('#canvas');await page.keyboard.press(action==='capture'?'a':'z');
   await page.waitForFunction(()=>ballance.player._BML_WebCheckpointResult()>0,{},{timeout:0});
   const state=await page.evaluate(()=>({result:ballance.player._BML_WebCheckpointResult(),status:document.querySelector('#status').textContent}));
   if(state.result!==(action==='capture'?1:2))throw new Error('UI checkpoint failed: '+JSON.stringify(state));
   await advance(30);
  }
  await page.waitForFunction(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('On Message LevelFinish'),{},{timeout:0});
  const log=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));
  result.play={finished:log.includes('On Message LevelFinish'),checkpoints:log.split('Browser checkpoint action=').length-1,death:log.includes('On Message BallOff')||log.includes('On Message Dead')};
  if(result.play.death||result.play.checkpoints!==6)throw new Error('UI playback/checkpoint outcome failed');
 }
 await page.evaluate(async()=>{
  ballance.player.FS.writeFile('/user/browser-persistence-check.txt','offline-data-v1');await ballance.persist();
 });
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'),{},{timeout:0});
 result.first=await page.evaluate(()=>({backend:ballance.gpu.backend,frames:ballance.gpu.frames,errors:ballance.gpu.errors,
  files:ballance.player.FS.readdir('/user'),log:ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'})}));
 if(result.first.errors.length||!result.first.log.includes('Goodbye!'))throw new Error('First session failed orderly shutdown');
 result.restart=await page.evaluate(()=>{
  const a=document.querySelector('#stage').getBoundingClientRect(),b=document.querySelector('#restart').getBoundingClientRect();
  return {offset:[b.x+b.width/2-a.x-a.width/2,b.y+b.height/2-a.y-a.height/2],hidden:document.querySelector('#restart-screen').hidden};
 });
 if(result.restart.hidden||result.restart.offset.some(n=>Math.abs(n)>1))throw new Error('Restart is not centered in game stage');
 await page.screenshot({path:output.replace(/\.json$/, '-restart.png')});
 await context.setOffline(true);
 await Promise.all([page.waitForEvent('load'),page.click('#restart')]);
 await page.waitForFunction(()=>document.querySelector('#offline').textContent.includes('离线缓存已就绪'),{},{timeout:0});
 await page.waitForFunction(()=>window.ballance?.gpu.frames>=30,{},{timeout:0});
 result.second=await page.evaluate(()=>({backend:ballance.gpu.backend,frames:ballance.gpu.frames,errors:ballance.gpu.errors,
  sentinel:ballance.player.FS.readFile('/user/browser-persistence-check.txt',{encoding:'utf8'}),
  maps:ballance.player.FS.readdir('/user/ModLoader/Maps'),tapes:ballance.player.FS.readdir('/user/ModLoader/TAS')}));
 if(process.env.BALLANCE_TEST_FAST_CHECKPOINTS==='1') {
  const advance=async n=>{const t=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,t,{timeout:0})};
  const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(20)};
  await page.focus('#canvas');await key('Enter');await key('Enter');await advance(180);await key('q');await advance(180);
  for(const action of [1,2,2,1,2,2]) {
   await key(action===1?'a':'z');
   const state=await page.evaluate(()=>({result:ballance.player._BML_WebCheckpointResult(),status:document.querySelector('#status').textContent}));
   if(state.result!==action)throw new Error('Fast hotkey checkpoint failed: '+JSON.stringify(state));
   await page.keyboard.down('ArrowUp');await advance(30);await page.keyboard.up('ArrowUp');
  }
  const log=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));
  result.fast={checkpoints:log.split(/Browser checkpoint action=[12] record=0/).length-1,precise:log.includes('precise x87 (TAS)'),errors:await page.evaluate(()=>ballance.gpu.errors)};
  if(result.fast.checkpoints!==6||result.fast.precise||result.fast.errors.length)throw new Error('Fast mode checkpoint coverage failed');
  await page.screenshot({path:output.replace(/\.json$/,'.png')});
 }
 if(result.second.sentinel!=='offline-data-v1'||result.second.errors.length)throw new Error('Offline/persistence failed');
 if(!result.second.maps.includes(mapPath.split('/').pop())||!result.second.tapes.includes(tapePath.split('/').pop()))throw new Error('Imported data lost');
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'),{},{timeout:0});
 await Promise.all([page.waitForEvent('load'),page.click('#restart')]);
 await page.waitForFunction(()=>window.ballance?.gpu.frames>=30,{},{timeout:0});
 result.third=await page.evaluate(()=>({running:ballance.running,errors:ballance.gpu.errors,
  sentinel:ballance.player.FS.readFile('/user/browser-persistence-check.txt',{encoding:'utf8'})}));
 if(!result.third.running||result.third.errors.length||result.third.sentinel!=='offline-data-v1')throw new Error('Repeated restart failed');
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'),{},{timeout:0});
 if(process.env.BALLANCE_TEST_WEBGL==='1'&&(result.first.backend!=='webgl2'||result.second.backend!=='webgl2'))throw Error('Expected WebGL2 fallback online and offline');
 result.passed=true;delete result.first.log;
}catch(error){result.error=String(error.stack||error);process.exitCode=1}
finally{result.browser=browser.version();result.logs=logs;await writeFile(output,JSON.stringify(result,null,2));console.log(JSON.stringify({...result,logs:undefined}));await browser.close();await new Promise(r=>server.close(r))}
