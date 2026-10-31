import {chromium} from 'playwright';import {createServer} from 'node:http';import {mkdir,writeFile} from 'node:fs/promises';import assert from 'node:assert/strict';
const [url,out]=process.argv.slice(2);await mkdir(out,{recursive:true});const requests=[];
const server=createServer((req,res)=>{requests.push(req.url);if(!req.url.includes('cors'))res.setHeader('Access-Control-Allow-Origin','*');
 if(req.url.includes('slow')){const timer=setTimeout(()=>res.end('not a map'),2000);res.on('close',()=>clearTimeout(timer));return}
 if(req.url.includes('missing')){res.writeHead(404).end('missing');return}
 if(req.url.includes('bad.tas')){res.end(Buffer.from([8,0,0,0,42,42,42,42]));return}
 if(req.url.includes('html')){res.setHeader('Content-Type','text/html');res.end('<!doctype html><title>Login</title>');return}
 res.end('not a map');
});await new Promise(r=>server.listen(0,'127.0.0.1',r));const remote='http://127.0.0.1:'+server.address().port;
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});const result={passed:false,checks:[]};
try{
 const page=await browser.newPage();await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 const link=async(path,action='map')=>{await page.evaluate(h=>location.hash=h,'#!'+new URLSearchParams(action==='map'?{action,map:remote+path}:{action,level:1,record:remote+path}));await page.waitForFunction(()=>document.querySelector('#share-dialog').open)};
 for(const [path,action,expected] of [['/missing.nmo','map','HTTP 404'],['/html.nmo','map','返回了网页'],['/bad.nmo','map','文件头无效'],['/cors.nmo','map','跨域'],['/bad.tas','tas','分享链接：']]){
  const before=requests.length;await link(path,action);assert.equal(requests.length,before);await page.locator('#share-dialog button[value=yes]').click();
  await page.waitForFunction(text=>document.querySelector('#status').textContent.includes(text),expected,{timeout:120000});
  result.checks.push({path,status:await page.textContent('#status')});
  assert.equal(await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).solver),0);
 }
 await link('/slow.nmo');await page.locator('#share-dialog button[value=yes]').click();
 for(let i=0;i<100&&!requests.includes('/slow.nmo');++i)await page.waitForTimeout(20);assert(requests.includes('/slow.nmo'));
 await link('/replacement.nmo');await page.keyboard.press('Escape');await page.waitForTimeout(2200);
 assert(!await page.evaluate(()=>ballance.player.FS.analyzePath('/user/ModLoader/Maps/slow.nmo').exists));assert(!requests.includes('/replacement.nmo'));result.checks.push({cancelledInFlight:true});
 await page.evaluate(remote=>{location.hash='#!'+new URLSearchParams({action:'map',map:remote+'/first.nmo'});setTimeout(()=>location.hash='#!'+new URLSearchParams({action:'map',map:remote+'/second.nmo'}),0)},remote);
 await page.waitForFunction(()=>document.querySelector('#share-dialog').open&&document.querySelector('#share-message').textContent.includes('second.nmo'));await page.keyboard.press('Escape');
 assert(!requests.includes('/first.nmo')&&!requests.includes('/second.nmo'));result.checks.push({supersededPrompt:true});
 result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{result.requests=requests;await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close();await new Promise(r=>server.close(r))}
