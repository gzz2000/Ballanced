// Independent real-browser acceptance. Usage: node qualify-batch.mjs URL WORKSPACE OUTPUT
import {chromium} from '../../packaging/web/node_modules/playwright/index.mjs';
import {mkdir,readFile,writeFile} from 'node:fs/promises';
import path from 'node:path';
const [url,root,out,only]=process.argv.slice(2);await mkdir(out,{recursive:true});
const packages={PositionViewer:'mod-ports-hud/PositionViewer',EnergyItemCounter:'mod-ports-hud/EnergyItemCounter',KeystrokeViewer:'mod-ports-hud/KeystrokeViewer',StaticCamera:'mod-ports-camera/static',RealTimeMeter:'mod-ports-resources/RealTimeMeter',PhysicsEditor:'mod-ports-physics'};
const browser=await chromium.launch({executablePath:process.env.CHROME_PATH||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
async function run(mode){const r={mode,passed:false},logs=[],names=Object.keys(packages).filter(n=>mode!=='tas'||!['StaticCamera','PhysicsEditor'].includes(n));const context=await browser.newContext({viewport:{width:1024,height:827},serviceWorkers:mode==='tas'?'block':'allow'}),page=await context.newPage();page.on('console',m=>logs.push(m.text()));page.on('pageerror',e=>logs.push(e.stack));try{
 if(mode==='tas'){
  await context.route('**/app.mjs',async route=>{const response=await route.fetch(),body=(await response.text()).replace('player=await createPlayer({canvas:',"player=await createPlayer({preRun:[m=>m.ENV.BALLANCE_FAST_FORWARD_BATCH='64'],canvas:");await route.fulfill({response,body})});
  await context.route('https://example.invalid/HS01.tas',async route=>route.fulfill({status:200,headers:{'Access-Control-Allow-Origin':'*'},contentType:'application/octet-stream',body:await readFile(path.join(root,'build/e3064-native-user/ModLoader/TAS/HS01_4740_1.30.344_264_reference.tas'))}));
 }
 const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 const advance=async n=>{const t=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,t,{timeout:60000})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(18)};
 const text=()=>page.evaluate(()=>Object.fromEntries(['PositionViewer','EnergyItemCounter','KeystrokeViewer','RealTimeMeter'].map(n=>[n,ballance.player.ccall('BML_Web'+n+'Text','string',[],[])])));
 const loaderLog=()=>page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));
 const verifyLoaded=async()=>{const log=await loaderLog();for(const n of names){const count=log.split('Loading Mod '+n+'[').length-1;if(count!==1)throw Error(n+' loaded '+count+' times');}r.loaded=await page.evaluate(()=>ballance.player.webMods.map(x=>x.id))};
 const quit=async()=>{await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));const log=await loaderLog();for(const n of names)if(!log.includes(n+' unloaded'))throw Error(n+' did not unload');return log};
 await page.goto(url);await ready();await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#import-mods').setInputFiles(names.map(n=>path.join(root,'build/web',packages[n],n+'.bwmod')));await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();await verifyLoaded();
 r.configs=await page.evaluate(names=>Object.fromEntries(names.map(n=>[n,ballance.player.FS.readFile('/user/ModLoader/Configs/'+n+'.cfg',{encoding:'utf8'})])),names);
 if(mode==='tas'){
  await page.evaluate(()=>location.hash='!'+new URLSearchParams({action:'tas',level:'1',record:'https://example.invalid/HS01.tas'}));await page.locator('#share-dialog button[value=yes]').click();
  await page.waitForFunction(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('On Message LevelFinish'),null,{timeout:180000});
  await page.waitForFunction(()=>!JSON.parse(ballance.player.ccall('BML_WebRealTimeMeterStatus','string',[],[])).counting);
  r.finished=await text();const log=await loaderLog();if(/On Message (BallOff|Dead)/.test(log))throw Error('HS01 fell');await page.screenshot({path:out+'/tas-finish.png'});r.quitLog=await quit();
 }else{
  await page.focus('#canvas');await advance(120);await page.evaluate(()=>ballance.player._BML_WebTASSharePrepare(1));await page.waitForFunction(()=>ballance.player._BML_WebTASSharePrepare(0)===1);
  const load=await page.evaluate(()=>{const p=ballance.player;p.FS.writeFile('/user/ModLoader/Maps/Batch-Level10.nmo',p.FS.readFile('/game/3D Entities/Level/Level_10.NMO'));return p.ccall('BML_WebTASShareLaunch','number',['string','string','number'],['','/user/ModLoader/Maps/Batch-Level10.nmo',0])});if(!load)throw Error('Level10 launch failed');
  await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebPhysicsEditorStatus','string',[],[])).navActivations>0,null,{timeout:60000});await advance(30);r.initial=await text();
  await page.keyboard.down('ArrowUp');await advance(80);r.held=await text();await page.screenshot({path:out+'/held.png'});await page.keyboard.up('ArrowUp');await advance(12);r.released=await text();
  if(r.initial.PositionViewer===r.held.PositionViewer)throw Error('Position did not update');if(!r.held.KeystrokeViewer.includes('↑')||r.released.KeystrokeViewer.includes('↑'))throw Error('Held/released key HUD mismatch');if(!r.initial.EnergyItemCounter.includes('remaining'))throw Error('Counter absent');if(!r.held.RealTimeMeter.includes('Ingame'))throw Error('Clock absent');
  await key('Escape');r.paused=await text();r.pauseBall=await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebPhysicsEditorStatus','string',[],[])).ball);await advance(40);r.pauseBallAfter=await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebPhysicsEditorStatus','string',[],[])).ball);if(Math.hypot(...r.pauseBall.map((v,i)=>v-r.pauseBallAfter[i]))>0.0001)throw Error('Ball moved during pause');await key('Escape');await page.keyboard.down('ArrowUp');await advance(20);await page.keyboard.up('ArrowUp');r.resumed=await text();if(r.paused.PositionViewer===r.resumed.PositionViewer)throw Error('Resume input did not move ball');
  await page.screenshot({path:out+'/resumed.png'});r.quitLog=await quit();
  // An offline qualification is optional; report concrete result, never infer it.
  const cacheState=await page.evaluate(async()=>({controlled:!!navigator.serviceWorker.controller,keys:await caches.keys()}));r.cacheState=cacheState;
  if(cacheState.controlled){await context.setOffline(true);try{await page.reload({waitUntil:'load',timeout:30000});await ready();await verifyLoaded();r.offlineReload='passed';await quit()}catch(e){r.offlineReload='failed: '+e.message;throw e}finally{await context.setOffline(false)}}else r.offlineReload='not tested: worker not controlling page';
 }
 r.passed=true;
}catch(e){r.error=String(e.stack||e);r.status=await page.locator('#status').textContent().catch(()=>null);r.log=await page.evaluate(()=>window.ballance?.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'})).catch(()=>null);await page.screenshot({path:out+'/'+mode+'-failure.png'}).catch(()=>{})}
finally{await writeFile(out+'/'+mode+'.json',JSON.stringify(r,null,2));await writeFile(out+'/'+mode+'.log',logs.join('\n'));console.log(JSON.stringify({...r,configs:undefined,quitLog:undefined,log:undefined}));await context.close()}return r;}
const results=await Promise.all((only?[only]:['ordinary','tas']).map(run));await browser.close();if(results.some(r=>!r.passed))process.exitCode=1;
