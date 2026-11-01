// Usage: node qualify.mjs URL OUTPUT MissingImportProbe.bwmod MissingImportProbe.wasm
import {chromium} from '../../../node_modules/playwright/index.mjs';
import {mkdir,readFile,writeFile} from 'node:fs/promises';
const [url,out,pkg,wasm]=process.argv.slice(2);await mkdir(out,{recursive:true});
const absent='BallanceWeb_IntentionallyAbsentHostFunction_E3970',r={passed:false},logs=[];
const compiled=await WebAssembly.compile(await readFile(wasm));r.import=WebAssembly.Module.imports(compiled).find(x=>x.name===absent);if(r.import?.module!=='env'||r.import.kind!=='function')throw Error('Fixture lacks required direct/lazy function import');
const browser=await chromium.launch({executablePath:process.env.CHROME_PATH||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});let page;
try{
 page=await browser.newPage({viewport:{width:1024,height:827},serviceWorkers:'block'});page.on('console',m=>logs.push(m.text()));page.on('pageerror',e=>logs.push(e.stack));
 const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 await page.goto(url);await ready();await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#import-mods').setInputFiles(pkg);await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);
 await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);
 await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('IntentionallyAbsentHostFunction'),null,{timeout:120000});r.rejected=await page.locator('#status').textContent();
 if(!r.rejected.includes('MissingImportProbe')||!r.rejected.includes('缺少宿主或依赖接口'))throw Error('Rejection did not identify plugin and missing host interface');
 if(logs.some(x=>x.includes('MISSING_IMPORT_CONSTRUCTOR_EXECUTED')||x.includes('MISSING_IMPORT_ONLOAD_EXECUTED')))throw Error('Plugin code ran before preflight rejection');
 await page.screenshot({path:out+'/rejected.png'});
 await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#mod-list li').filter({hasText:'MissingImportProbe'}).getByRole('button',{name:'移除'}).click();await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);
 await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();
 r.recovered=await page.evaluate(()=>({loaded:ballance.player.webMods.map(m=>m.id),marker:ballance.player.FS.analyzePath('/user/missing-import-constructor-ran.txt').exists,installed:ballance.player.FS.readdir('/user/ModLoader/WebMods')}));
 if(r.recovered.marker)throw Error('Constructor filesystem side effect escaped preflight');if(r.recovered.loaded.includes('MissingImportProbe')||r.recovered.installed.includes('MissingImportProbe.bwmod'))throw Error('UI did not remove bad plugin');
 await page.screenshot({path:out+'/recovered.png'});await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));r.passed=true;
}catch(e){r.error=String(e.stack||e);if(page){r.status=await page.locator('#status').textContent().catch(()=>null);await page.screenshot({path:out+'/failure.png'}).catch(()=>{})}process.exitCode=1}
finally{await writeFile(out+'/result.json',JSON.stringify(r,null,2));await writeFile(out+'/browser.log',logs.join('\n'));console.log(JSON.stringify(r));await browser.close()}
