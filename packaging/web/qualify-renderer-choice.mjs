// Backend switching crosses document/GPU/IDBFS lifetimes; test the real UI.
import {chromium} from 'playwright';
import {writeFile} from 'node:fs/promises';
const [url,out]=process.argv.slice(2),result={passed:false,steps:[]};
const browser=await chromium.launch({executablePath:process.env.CHROME_PATH||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
try{
 const context=await browser.newContext(),page=await context.newPage();
 await page.addInitScript(()=>{
  window.adapterRequests=0;
  if(navigator.gpu){const original=navigator.gpu.requestAdapter.bind(navigator.gpu);navigator.gpu.requestAdapter=(...args)=>{adapterRequests++;return original(...args)}}
 });
 const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>15,null,{timeout:120000});
 const inspect=()=>page.evaluate(()=>({backend:ballance.gpu.backend,choice:document.querySelector('#renderer').value,saved:localStorage.getItem('ballance.renderer'),requests:adapterRequests,errors:ballance.gpu.errors}));
 const assert=(value,message)=>{if(!value)throw Error(message)};
 const open=async()=>{await page.click('#toggle-sidebar');await page.click('#tab-display')};
 await page.goto(url);await ready();let state=await inspect();result.steps.push({name:'fresh',...state});
 assert(state.backend==='webgl2'&&state.choice==='webgl2'&&state.requests===0,'Default must not probe WebGPU');
 await page.evaluate(()=>ballance.player.FS.writeFile('/user/renderer-switch.txt','persist-before-switch'));
 await open();await page.selectOption('#renderer','webgpu');
 // A failed flush must leave the running document, preference and data intact.
 await page.evaluate(()=>{window.originalFlush=ballance.player._BallanceWebFlushFiles;ballance.player._BallanceWebFlushFiles=()=>{throw Error('test storage failure')}});
 await page.click('#apply-renderer');await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('未切换渲染器'));
 state=await inspect();assert(state.backend==='webgl2'&&state.saved===null,'Failed save changed backend/preference');result.steps.push({name:'save-failure-keeps-session',...state});
 await page.evaluate(()=>{ballance.player._BallanceWebFlushFiles=originalFlush});
 await Promise.all([page.waitForEvent('load'),page.click('#apply-renderer')]);await ready();state=await inspect();
 assert(state.backend==='webgpu'&&state.saved==='webgpu'&&state.choice==='webgpu','WebGPU choice not persisted');
 assert(await page.evaluate(()=>ballance.player.FS.readFile('/user/renderer-switch.txt',{encoding:'utf8'}))==='persist-before-switch','User data lost switching');result.steps.push({name:'switch-webgpu',...state});
 await page.reload();await ready();state=await inspect();assert(state.backend==='webgpu','Reload forgot preference');result.steps.push({name:'reload-webgpu',...state});
 await page.goto(url+'?renderer=webgl2');await ready();assert((await inspect()).backend==='webgl2','Diagnostic override failed');
 await open();await page.selectOption('#renderer','auto');await Promise.all([page.waitForEvent('load'),page.click('#apply-renderer')]);await ready();state=await inspect();
 assert(state.backend==='webgpu'&&state.choice==='auto'&&state.saved==='auto'&&!new URL(page.url()).searchParams.has('renderer'),'Sidebar choice did not replace URL override');result.steps.push({name:'auto-removes-override',...state});
 await open();await page.selectOption('#renderer','webgl2');await Promise.all([page.waitForEvent('load'),page.click('#apply-renderer')]);await ready();
 await context.setOffline(true);await page.reload();await ready();state=await inspect();assert(state.backend==='webgl2'&&state.saved==='webgl2'&&state.requests===0,'Offline preference failed');result.steps.push({name:'offline-webgl2',...state});
 await open();await page.screenshot({path:out.replace(/\.json$/,'.png')});
 assert(result.steps.every(s=>!s.errors.length),'GPU errors');result.passed=true;
}catch(error){result.error=String(error.stack||error);process.exitCode=1}
finally{result.browser=browser.version();await writeFile(out,JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
