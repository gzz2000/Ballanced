// End-to-end qualification for the three upstream MyBMLMods HUD ports.
// Run from the workspace after building their packages; uses a fresh isolated profile.
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const {chromium}=await import(pathToFileURL(path.join(repo,'packaging/web/node_modules/playwright/index.mjs')));
const build=path.resolve(process.env.BALLANCE_HUD_BUILD_ROOT||'build/web/mod-ports-hud');
import {createWriteStream} from 'node:fs';
import {mkdir,writeFile} from 'node:fs/promises';
const out=path.join(build,process.env.BALLANCE_TEST_OUTPUT||'qualification');await mkdir(out,{recursive:true});
const trace=createWriteStream(out+'/live.log');
const browser=await chromium.launch({executablePath:process.env.BALLANCE_TEST_BROWSER||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
let page;const logs=[],r={passed:false,scope:'HUD data, input, pickup/restore, config and unload; review screenshots separately for rendered text'};
try{
 page=await browser.newPage({viewport:{width:1024,height:827},serviceWorkers:'block'});page.on('console',m=>(logs.push(m.text()),trace.write(m.text()+'\n')));page.on('pageerror',e=>(logs.push(e.stack),trace.write(e.stack+'\n')));
 await page.goto(process.env.BALLANCE_TEST_URL||'http://localhost:18766');const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});await ready();
 await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#import-mods').setInputFiles(['PositionViewer','EnergyItemCounter','KeystrokeViewer'].map(n=>path.join(build,n,n+'.bwmod')));
 await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();
 r.mods=await page.evaluate(()=>ballance.player.webMods.map(x=>x.id));
 const advance=async n=>{let target=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(target=>ballance.gpu.frames>=target,target,{timeout:60000})};
 const key=async k=>{await page.keyboard.down(k);await advance(8);await page.keyboard.up(k);await advance(20)};
 const texts=()=>page.evaluate(()=>Object.fromEntries(['PositionViewer','EnergyItemCounter','KeystrokeViewer'].map(n=>[n,ballance.player.ccall('BML_Web'+n+'Text','string',[],[])])));
 await page.focus('#canvas');await advance(200);
 r.mapFiles=await page.evaluate(()=>ballance.player.FS.readdir('/game/3D Entities/Level'));
 await page.evaluate(()=>ballance.player._BML_WebTASSharePrepare(1));await page.waitForFunction(()=>ballance.player._BML_WebTASSharePrepare(0)===1,null,{timeout:60000});
 r.mapLoad=await page.evaluate(()=>{const p=ballance.player; p.FS.writeFile('/user/ModLoader/Maps/HUD-Level10.nmo',p.FS.readFile('/game/3D Entities/Level/Level_10.NMO'));return p.ccall('BML_WebTASShareLaunch','number',['string','string','number'],['','/user/ModLoader/Maps/HUD-Level10.nmo',0])});
 if(!r.mapLoad)throw Error(await page.evaluate(()=>ballance.player.ccall('BML_WebMapError','string',[],[])));
 await page.waitForFunction(()=>ballance.player._BML_WebMapStatus()===2,null,{timeout:60000});await advance(300);
 r.initial=await texts();await key('a');await advance(30);r.saved=await texts();
 await page.keyboard.down('ArrowUp');await advance(240);r.held=await texts();await page.screenshot({path:out+'/held.png'});await page.keyboard.up('ArrowUp');await advance(200);r.released=await texts();await page.screenshot({path:out+'/hud.png'});
 await page.keyboard.down('ArrowUp');await advance(8);await key('z');r.restoredWhileHeld=await texts();
 if(!r.restoredWhileHeld.KeystrokeViewer.includes('↑'))throw Error('Held key HUD stale after restore');
 await page.keyboard.up('ArrowUp');await advance(20);r.restored=await texts();await page.screenshot({path:out+'/restored.png'});
 await page.keyboard.down('ArrowUp');await advance(240);await page.keyboard.up('ArrowUp');await advance(200);r.recollected=await texts();
 await key('z');await advance(20);r.restoredAgain=await texts();
 if(r.initial.EnergyItemCounter===r.released.EnergyItemCounter)throw Error('No energy pickup observed');
 if(r.released.EnergyItemCounter!==r.recollected.EnergyItemCounter)throw Error('Energy HUD failed recollection');
 if(r.initial.EnergyItemCounter!==r.restored.EnergyItemCounter||r.initial.EnergyItemCounter!==r.restoredAgain.EnergyItemCounter)throw Error('Energy HUD failed restore');
 if(r.initial.PositionViewer===r.held.PositionViewer)throw Error('Position text did not update');
 if(!r.held.KeystrokeViewer.includes('↑')||r.released.KeystrokeViewer.includes('↑'))throw Error('Keystroke state not updated');
 if(!r.initial.EnergyItemCounter.includes('remaining'))throw Error('Energy counter absent');
 r.configs=await page.evaluate(()=>Object.fromEntries(['PositionViewer','EnergyItemCounter','KeystrokeViewer'].map(n=>[n,ballance.player.FS.readFile('/user/ModLoader/Configs/'+n+'.cfg',{encoding:'utf8'})])));
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));
 r.log=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));
 for(const n of ['PositionViewer','EnergyItemCounter','KeystrokeViewer'])if(!r.log.includes(n+' unloaded'))throw Error(n+' missing unload');
 r.quitLog=r.log;
 await page.evaluate(async()=>{const p=ballance.player,path='/user/ModLoader/Configs/KeystrokeViewer.cfg';p.FS.writeFile(path,p.FS.readFile(path,{encoding:'utf8'}).replace('B All_Keys_Mode 0','B All_Keys_Mode 1'));await ballance.persist()});
 await page.reload();await ready();await advance(200);
 await page.evaluate(()=>ballance.player._BML_WebTASSharePrepare(1));await page.waitForFunction(()=>ballance.player._BML_WebTASSharePrepare(0)===1);
 await page.evaluate(()=>ballance.player.ccall('BML_WebTASShareLaunch','number',['string','string','number'],['','/user/ModLoader/Maps/HUD-Level10.nmo',0]));await page.waitForFunction(()=>ballance.player._BML_WebMapStatus()===2);await advance(300);
 await page.focus('#canvas');await page.keyboard.down('b');await advance(8);r.configAllKeys=await texts();await page.screenshot({path:out+'/config-all-keys.png'});await page.keyboard.up('b');
 if(!r.configAllKeys.KeystrokeViewer.toLowerCase().includes('b'))throw Error('Persisted all-keys config did not apply');
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));
 await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();
 for(const n of ['PositionViewer','EnergyItemCounter','KeystrokeViewer']){await page.locator('#mod-list li').filter({hasText:n+' · 已安装'}).getByRole('button',{name:'移除'}).click();await page.waitForFunction(n=>![...document.querySelectorAll('#mod-list li')].some(x=>x.textContent.includes(n+' · 已安装')),n)}
 await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();r.afterRemoval=await page.evaluate(()=>ballance.player.webMods.map(x=>x.id));
 if(r.afterRemoval.some(n=>['PositionViewer','EnergyItemCounter','KeystrokeViewer'].includes(n)))throw Error('Removed mod remains loaded');
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));
 r.passed=true;
}catch(e){r.error=String(e.stack||e);if(page){r.status=await page.locator('#status').textContent().catch(()=>null);await page.screenshot({path:out+'/failure.png'}).catch(()=>{});r.log=await page.evaluate(()=>window.ballance?.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'})).catch(()=>null)}process.exitCode=1}
finally{await writeFile(out+'/result.json',JSON.stringify(r,null,2));await writeFile(out+'/browser.log',logs.join('\n'));console.log(JSON.stringify({...r,log:undefined,quitLog:undefined,configs:undefined}));await browser.close()}
