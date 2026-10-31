// Exercise cold install with delayed, chunked pack responses (no Content-Length).
import {chromium} from 'playwright';
import {createServer} from 'node:http';
import {readFile,writeFile} from 'node:fs/promises';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const [siteArg,out]=process.argv.slice(2),site=resolve(siteArg),requests=[],finished=[];
const manifest=JSON.parse(await readFile(site+'/build.json','utf8'));
const total=manifest.packs.reduce((n,p)=>n+p.size,0),start=Date.now();
const server=createServer(async(req,res)=>{try{
 const name=decodeURIComponent(new URL(req.url,'http://localhost').pathname).slice(1)||'index.html';
 if(name.includes('..'))throw Error('path');requests.push({name,method:req.method,time:Date.now()-start});
 const bytes=await readFile(site+'/'+name);res.setHeader('Cache-Control','no-cache');
 res.setHeader('Content-Type',/\.(mjs|js)$/.test(name)?'text/javascript':name.endsWith('.wasm')?'application/wasm':name.endsWith('.html')?'text/html':name.endsWith('.css')?'text/css':'application/octet-stream');
 if(name.endsWith('.pack')){
  // Enough latency to observe partial transfer before any bundle finishes.
  for(let i=0;i<bytes.length&&!res.destroyed;i+=128*1024){res.write(bytes.subarray(i,i+128*1024));await new Promise(r=>setTimeout(r,12))}
  finished.push({name,time:Date.now()-start});res.end();
 }else res.end(bytes);
}catch{res.writeHead(404).end()}});
await new Promise(r=>server.listen(0,'127.0.0.1',r));
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false};
try{
 const context=await browser.newContext(),page=await context.newPage();
 await page.addInitScript(origin=>{window.transfers=[];navigator.serviceWorker.addEventListener('message',e=>{if(e.data?.cacheProgress!==undefined)window.transfers.push({...e.data,time:Date.now()-origin,visible:!document.querySelector('#cache-progress').hidden,game:!!window.ballance})})},start);
 await page.goto('http://127.0.0.1:'+server.address().port+'/');
 await page.waitForFunction(()=>window.transfers?.some(p=>p.cacheBytes>0),null,{timeout:60000});
 result.early=await page.evaluate(()=>({messages:transfers.slice(0,3),visible:!document.querySelector('#cache-progress').hidden,text:document.querySelector('#loading-status').textContent,game:!!window.ballance}));
 assert(result.early.visible&&!result.early.game);assert.match(result.early.text,/已下载压缩资源/);
 await page.waitForFunction(()=>window.ballance?.gpu.frames>=30,null,{timeout:120000});
 const transfers=await page.evaluate(()=>window.transfers);
 assert.equal(transfers[0].cacheBytes,0);assert.equal(transfers.at(-1).cacheBytes,total);assert.equal(transfers.at(-1).cacheProgress,manifest.files.length);
 assert(transfers.every((p,i)=>!i||p.cacheBytes>=transfers[i-1].cacheBytes));
 assert(transfers.find(p=>p.cacheBytes>0).time<Math.min(...finished.map(f=>f.time)));
 assert.equal(requests.filter(r=>r.method==='HEAD').length,0);
 assert.equal(requests.filter(r=>r.name.startsWith('assets/')).length,0);
 assert.equal(requests.filter(r=>r.name.endsWith('.pack')).length,manifest.packs.length);
 result.transfers={samples:transfers.length,first:transfers[0],last:transfers.at(-1),firstBundleComplete:Math.min(...finished.map(f=>f.time))};
 await context.setOffline(true);await page.reload();await page.waitForFunction(()=>window.ballance?.gpu.frames>=30,null,{timeout:60000});
 result.offline=true;result.passed=true;await context.close();
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{result.requests=requests;await writeFile(out,JSON.stringify(result,null,2));console.log(JSON.stringify({...result,requests:requests.length}));await browser.close();await new Promise(r=>server.close(r))}
