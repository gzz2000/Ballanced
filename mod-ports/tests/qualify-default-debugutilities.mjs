// Default installation, offline startup, and upgrade from the optional package.
import {chromium} from '../../packaging/web/node_modules/playwright/index.mjs';
import {mkdir,writeFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
const [url,out,extra]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const context=await browser.newContext(),page=await context.newPage(),errors=[];
page.on('pageerror',e=>errors.push(String(e)));
const ready=async()=>{await page.waitForFunction(()=>window.ballance?.gpu.frames>30||document.querySelector('#status').textContent.startsWith('无法启动'),null,{timeout:120000});assert(await page.evaluate(()=>!!window.ballance),await page.locator('#status').textContent());assert.equal(await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).split('Loading Mod DebugUtilities[').length-1),1)};
const save=()=>page.evaluate(async()=>{await new Promise((resolve,reject)=>ballance.player.FS.syncfs(false,e=>e?reject(e):resolve()))});
const results={passed:false};
try{
 await page.goto(url);await ready();
 assert(await page.evaluate(()=>ballance.player.webDefaultMods.some(m=>m.id==='DebugUtilities')));
 assert.equal(await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebDebugUtilitiesStatus','string',[],[])).cheat),false);
 results.fresh=true;
 await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();assert(await page.locator('#mod-list').textContent().then(t=>t.includes('DebugUtilities · 默认')));
 // Reproduce storage left by the previous optional release, with a user setting.
 await page.evaluate(async()=>{const p=ballance.player,bytes=new Uint8Array(await(await fetch('mods/DebugUtilities.bwmod')).arrayBuffer());p.FS.writeFile('/user/ModLoader/WebMods/DebugUtilities.bwmod',bytes);let cfg=p.FS.readFile('/user/ModLoader/Configs/DebugUtilities.cfg',{encoding:'utf8'});if(!/EnableSuicideKey\s+1/.test(cfg))throw Error('Missing config fixture');p.FS.writeFile('/user/ModLoader/Configs/DebugUtilities.cfg',cfg.replace(/EnableSuicideKey\s+1/,'EnableSuicideKey 0'));});
 await save();await page.reload();await ready();results.upgrade=true;
 assert(await page.evaluate(()=>/EnableSuicideKey\s+0/.test(ballance.player.FS.readFile('/user/ModLoader/Configs/DebugUtilities.cfg',{encoding:'utf8'}))));
 await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();assert((await page.locator('#mod-list').textContent()).includes('已由默认版本取代'));
 // A retained old default copy must not prevent installing another optional mod.
 await page.locator('#import-mods').setInputFiles(extra);await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled||document.querySelector('#mods-status').textContent.startsWith('安装失败'));
 assert(!(await page.locator('#mods-status').textContent()).startsWith('安装失败'));
 await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();results.otherModInstall=true;
 await page.waitForFunction(()=>navigator.serviceWorker.controller&&document.querySelector('#offline').textContent.includes('已就绪'));
 await context.setOffline(true);await page.reload();await ready();results.offline=true;
 results.gpuErrors=await page.evaluate(()=>ballance.gpu.errors);assert.equal(results.gpuErrors.length,0);assert.equal(errors.length,0);results.passed=true;
}catch(e){results.error=String(e.stack||e);await page.screenshot({path:out+'/failure.png'}).catch(()=>{});process.exitCode=1}
finally{results.errors=errors;await writeFile(out+'/result.json',JSON.stringify(results,null,2));console.log(results);await browser.close()}
