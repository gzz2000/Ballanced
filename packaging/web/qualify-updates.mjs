// Upgrade real cached games in one disposable profile, retaining an old tab.
import {chromium} from 'playwright';
import {createServer} from 'node:http';
import {readFile,writeFile} from 'node:fs/promises';
import {resolve,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
const [legacyArg,candidateArg,out]=process.argv.slice(2),legacy=resolve(legacyArg),candidate=resolve(candidateArg);
const hash=bytes=>createHash('sha256').update(bytes).digest('hex');
const template=await readFile(resolve(dirname(fileURLToPath(import.meta.url)),'host/sw.template.js'),'utf8');
const metadata=JSON.parse(await readFile(candidate+'/build.json','utf8'));
const variant=async(label,bad=false)=>{
 const version='upgrade-test-'+label,overrides=new Map();
 const html=(await readFile(candidate+'/index.html','utf8')).replace(metadata.version,version).replace('by gzz2000','by gzz2000 · '+label);
 const app=(await readFile(candidate+'/app.mjs','utf8'))+'\n// Upgrade fixture '+label+'\n';
 overrides.set('index.html',Buffer.from(html));overrides.set('app.mjs',Buffer.from(app));
 const files=metadata.files.map(file=>overrides.has(file.url)?{...file,size:overrides.get(file.url).length,sha256:hash(overrides.get(file.url))}:file);
 overrides.set('sw.js',Buffer.from(template.replace('__VERSION__',JSON.stringify(version)).replace('__FILES__',JSON.stringify(files))));
 if(bad)overrides.set('shaders.json',Buffer.from('corrupt shader download'));
 return {root:candidate,version,overrides};
};
const a={root:legacy,version:JSON.parse(await readFile(legacy+'/build.json','utf8')).version,overrides:new Map()},b={root:candidate,version:metadata.version,overrides:new Map()},c=await variant('C'),d=await variant('D',true);
let active=a;const result={passed:false,steps:[]};
const server=createServer(async(req,res)=>{try{
 const name=decodeURIComponent(new URL(req.url,'http://localhost').pathname).slice(1)||'index.html';if(name.includes('..'))throw new Error('path');if(name==='sw.js')console.log('Serving SW',active.version);
 const bytes=active.overrides.get(name)??await readFile(active.root+'/'+name);
 res.setHeader('Cache-Control','no-cache');res.setHeader('Content-Length',bytes.length);
 res.setHeader('Content-Type',/\.(mjs|js)$/.test(name)?'text/javascript':name.endsWith('.wasm')?'application/wasm':name.endsWith('.html')?'text/html':name.endsWith('.css')?'text/css':'application/octet-stream');
 res.end(req.method==='HEAD'?undefined:bytes);
}catch{res.writeHead(404).end()}});
await new Promise(r=>server.listen(0,'127.0.0.1',r));
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
try{
 const context=await browser.newContext(),page=await context.newPage(),url='http://127.0.0.1:'+server.address().port+'/';
 await page.addInitScript(()=>navigator.serviceWorker.addEventListener('message',e=>{if(e.data?.cacheError||e.data?.cacheActivated||e.data?.cacheProgress===e.data?.total&&e.data?.total)console.log('SW-STATE '+JSON.stringify(e.data))}));
 page.on('pageerror',e=>console.log('PAGE ERROR',String(e)));page.on('console',m=>{if(m.type()==='error'||m.text().startsWith('SW-STATE'))console.log('CONSOLE ERROR',m.text())});
 const ready=async(p,version)=>{try{await p.waitForFunction(v=>window.ballance?.gpu.frames>30&&(!v||document.querySelector('meta[name="ballance-build"]')?.content===v),version,{timeout:30000})}catch(e){console.log('READY FAILED',await p.evaluate(async()=>({html:document.querySelector('meta[name="ballance-build"]')?.content,status:document.querySelector('#status')?.textContent,cache:document.querySelector('#offline')?.textContent,running:window.ballance?.running,caches:await caches.keys()})));throw e}};
 const step=(name,details={})=>{result.steps.push({name,...details});console.log(name,JSON.stringify(details))};
 await page.goto(url);await ready(page);
 await page.evaluate(async()=>{window.originalDocument=true;ballance.player.FS.writeFile('/user/upgrade-sentinel.txt','keep-user-data');await ballance.persist()});step('legacy installed');
 active=b;await page.evaluate(async()=>{const r=await navigator.serviceWorker.getRegistration();await r.update()});
 const deadline=Date.now()+120000;
 for(;;){
  const installed=await page.evaluate(async expected=>{
   const r=await navigator.serviceWorker.getRegistration();if(r.installing||r.waiting||r.active?.state!=='activated')return false;
   return new Promise(resolve=>{const c=new MessageChannel(),t=setTimeout(()=>{c.port1.close();resolve(false)},500);c.port1.onmessage=e=>{clearTimeout(t);c.port1.close();resolve(e.data.version===expected)};r.active.postMessage({type:'VERSION'},[c.port2])});
  },b.version);
  if(installed)break;if(Date.now()>deadline)throw new Error('Updated worker never activated');
  await new Promise(r=>setTimeout(r,100));
 }
 const oldHash=await page.evaluate(async()=>{const bytes=await(await fetch('app.mjs')).arrayBuffer();return [...new Uint8Array(await crypto.subtle.digest('SHA-256',bytes))].map(x=>x.toString(16).padStart(2,'0')).join('')});
 if(oldHash!==hash(await readFile(legacy+'/app.mjs'))||!await page.evaluate(()=>window.originalDocument))throw new Error('Legacy running tab mixed/reloaded on activation');step('legacy tab pinned across activation',await page.evaluate(async()=>{const r=await navigator.serviceWorker.getRegistration();return {active:r.active?.state,waiting:r.waiting?.state,installing:r.installing?.state,caches:await caches.keys(),controller:navigator.serviceWorker.controller?.scriptURL}}));
 await page.reload();await ready(page,b.version);
 if(await page.evaluate(()=>ballance.player.FS.readFile('/user/upgrade-sentinel.txt',{encoding:'utf8'}))!=='keep-user-data')throw new Error('Lost user data');step('legacy ordinary refresh upgraded');
 const oldTab=await context.newPage();await oldTab.goto(url);await ready(oldTab,b.version);await oldTab.evaluate(()=>window.originalDocument=true);
 active=c;await page.reload();await ready(page,c.version);step('single refresh upgrades next release');
 const cdp=await context.newCDPSession(page);await cdp.send('ServiceWorker.enable');await cdp.send('ServiceWorker.stopAllWorkers');
 const pinnedHash=await oldTab.evaluate(async()=>{const bytes=await(await fetch('app.mjs')).arrayBuffer();return [...new Uint8Array(await crypto.subtle.digest('SHA-256',bytes))].map(x=>x.toString(16).padStart(2,'0')).join('')});
 if(pinnedHash!==hash(await readFile(candidate+'/app.mjs'))||!await oldTab.evaluate(()=>window.originalDocument))throw new Error('Old tab lost pin after SW restart');step('old tab retained after worker restart');
 await context.setOffline(true);await page.reload();await ready(page,c.version);
 if(await page.evaluate(()=>ballance.player.FS.readFile('/user/upgrade-sentinel.txt',{encoding:'utf8'}))!=='keep-user-data')throw new Error('Offline user data lost');step('offline refresh retains current layout and user data');
 await context.setOffline(false);active=d;await page.reload();await ready(page,c.version);
 // The navigation's implicit update check can retry D after the host already
 // fell back to C. Wait for that installing worker to settle before asserting
 // that its temporary cache has been removed (a temporary cache is not active).
 let failed;const failureDeadline=Date.now()+30000;
 for(;;){
  failed=await page.evaluate(async()=>{const r=await navigator.serviceWorker.getRegistration();return {caches:await caches.keys(),errors:ballance.gpu.errors,installing:r.installing?.state,waiting:r.waiting?.state}});
  if(!failed.installing&&!failed.waiting&&!failed.caches.includes('ballanced-web-'+d.version))break;
  if(Date.now()>failureDeadline)throw new Error('Corrupt candidate did not settle: '+JSON.stringify(failed));
  await new Promise(r=>setTimeout(r,100));
 }
 const activeAfterFailure=await page.evaluate(async()=>{const r=await navigator.serviceWorker.getRegistration();return new Promise((resolve,reject)=>{const c=new MessageChannel(),t=setTimeout(()=>reject(Error('VERSION timeout')),3000);c.port1.onmessage=e=>{clearTimeout(t);c.port1.close();resolve(e.data.version)};r.active.postMessage({type:'VERSION'},[c.port2])})});
 if(activeAfterFailure!==c.version||failed.errors.length)throw new Error('Failed update replaced valid cache: '+JSON.stringify({failed,activeAfterFailure}));step('corrupt update rolls back',{...failed,activeAfterFailure});
 if(!await oldTab.locator('#update-game').isVisible())throw new Error('No update action in the old running tab');
 await oldTab.evaluate(()=>ballance.player.FS.writeFile('/user/update-button-sentinel.txt','saved-before-update'));
 await oldTab.click('#update-game');await ready(oldTab,c.version);
 if(await oldTab.evaluate(()=>ballance.player.FS.readFile('/user/update-button-sentinel.txt',{encoding:'utf8'}))!=='saved-before-update')throw new Error('Update button lost user data');step('update button persists then restarts');
 await oldTab.close();active=await variant('E');await page.reload();await ready(page,active.version);
 const retained=await page.evaluate(()=>caches.keys());if(retained.includes('ballanced-web-'+b.version))throw new Error('Closed old tab cache never collected');step('unused old cache collected', {retained});
 result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{result.browser=browser.version();await writeFile(out,JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close();await new Promise(r=>server.close(r))}
