import {chromium} from '../../packaging/web/node_modules/playwright/index.mjs';
import {mkdir,writeFile} from 'node:fs/promises';
const [url,out,mod]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:process.env.CHROME_PATH||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false},logs=[];let page;
try{
 page=await browser.newPage({viewport:{width:1024,height:827},serviceWorkers:'block'});
 page.on('console',m=>logs.push(m.text()));page.on('pageerror',e=>logs.push(e.stack));
 const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 const advance=async n=>{const target=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(n=>ballance.gpu.frames>=n,target,{timeout:60000})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(18)};
 const status=()=>page.evaluate(()=>({state:JSON.parse(ballance.player.ccall('BML_WebRealTimeMeterStatus','string',[],[])),text:ballance.player.ccall('BML_WebRealTimeMeterText','string',[],[]),now:performance.now()}));
 await page.goto(url);await ready();
 await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#import-mods').setInputFiles(mod);await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);
 await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();
 result.menu=await status();if(result.menu.state.label||!result.menu.state.enabled)throw Error('Wrong initial state');
 await page.focus('#canvas');await key('Enter');await key('Enter');await advance(180);await key('q');
 await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebRealTimeMeterStatus','string',[],[])).counting,null,{timeout:60000});
 result.start=await status();await advance(120);result.running=await status();
 if(!result.running.state.label||!result.running.text.includes('Ingame')||!result.running.text.includes('World'))throw Error('HUD text not present');
 const wall=result.running.now-result.start.now,clock=result.running.state.worldMs-result.start.state.worldMs;
 if(Math.abs(clock-wall)>60||result.running.state.ingameMs<=result.start.state.ingameMs)throw Error('Clock not advancing at real elapsed time');
 await page.screenshot({path:out+'/running.png'});
 await key('Escape');result.paused=await status();await advance(90);result.pauseWait=await status();
 if(result.pauseWait.state.worldMs-result.paused.state.worldMs<500)throw Error('World clock incorrectly freezes with pause');
 await key('Escape');await advance(30);
 // Real menu exit: Esc -> last (Exit Level), Enter. Do not synthesize callbacks.
 await key('Escape');await key('ArrowUp');await key('Enter');await advance(30);await key('ArrowLeft');await key('Enter');await advance(100);
 result.exited=await status();
 await writeFile(out+'/before-exit.png',await page.screenshot());
 if(result.exited.state.label||!result.exited.state.levelExited)throw Error('Level exit did not release label');
 const cfg=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/Configs/RealTimeMeter.cfg',{encoding:'utf8'}));result.config=cfg;
 await page.evaluate(async()=>{const p='/user/ModLoader/Configs/RealTimeMeter.cfg',s=ballance.player.FS.readFile(p,{encoding:'utf8'});if(!s.includes('B Enabled 1'))throw Error('Unknown config encoding');ballance.player.FS.writeFile(p,s.replace('B Enabled 1','B Enabled 0'));await ballance.persist()});
 await page.reload();await ready();result.disabled=await status();if(result.disabled.state.enabled)throw Error('Disabled config not persisted');
 await page.focus('#canvas');await key('Enter');await key('Enter');await advance(180);await key('q');await advance(60);result.disabledLevel=await status();if(result.disabledLevel.state.label||result.disabledLevel.state.counting)throw Error('Disabled plugin still draws/counts');
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));
 const log=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));if(!log.includes('RealTimeMeter unloaded'))throw Error('Unload absent');
 result.passed=true;
}catch(e){result.error=String(e.stack||e);if(page){result.status=await page.locator('#status').textContent().catch(()=>null);await page.screenshot({path:out+'/failure.png'}).catch(()=>{})}process.exitCode=1}
finally{await writeFile(out+'/result.json',JSON.stringify(result,null,2));await writeFile(out+'/browser.log',logs.join('\n'));console.log(JSON.stringify(result));await browser.close()}
