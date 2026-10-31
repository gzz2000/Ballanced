import {chromium} from 'playwright';
import {mkdir,writeFile,readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
import {parseShareLink,installSharedFile} from './host/share-links.mjs';
const [url,out,resources]=process.argv.slice(2);await mkdir(out,{recursive:true});
const base='https://github.com/misiroqwq/ballance_misc/raw/refs/heads/main/TAS_Resources/TAS文件合集/';
const mapName='210522图.nmo',recordName='210522_2.20.095.tas',builtinName='HS01_4740_1.30.344_264_口语大赛_onlyforyou.tas';
const folder='其他/HS 210522 2.20.095 2025-10-12 by fluoresce/';
const map=encodeURI(base+folder+mapName),record=encodeURI(base+folder+recordName),builtin=encodeURI(base+'原版关常规/'+builtinName);
const hashes={map:'#!action=map&map='+map,builtin:'#!action=tas&level=1&record='+builtin,custom:'#!action=tas&map='+map+'&record='+record};
assert.equal(parseShareLink(hashes.map).map.name,mapName);
assert.equal(new URL(parseShareLink(hashes.map).map.url).hostname,'raw.githubusercontent.com');
assert.equal(parseShareLink('#!'+new URLSearchParams({action:'tas',map,record})).record.name,recordName);
for(const hash of ['#!action=no','#!action=map&map=javascript:alert(1)','# !', '#!action=tas&level=14&record='+builtin,'#!action=tas&level=1&map='+map+'&record='+record,'#!action=map&map='+map+'&action=map'])if(hash!=='# !')assert.throws(()=>parseShareLink(hash));
const files=new Map(),FS={mkdirTree(){},readdir(){return [...files.keys()].map(x=>x.split('/').pop())},isFile:()=>true,stat:()=>({mode:1}),readFile:p=>files.get(p),writeFile:(p,b)=>files.set(p,b)};
files.set('/x/a.nmo',new Uint8Array([1]));files.set('/x/a3.nmo',new Uint8Array([3]));
assert.deepEqual(installSharedFile(FS,'/x','a.nmo',new Uint8Array([3])),{path:'/x/a3.nmo',reused:true});
assert.equal(installSharedFile(FS,'/x','a.nmo',new Uint8Array([2])).path,'/x/a2.nmo');assert.equal(files.get('/x/a.nmo')[0],1);
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const results=[];const sha=b=>createHash('sha256').update(b).digest('hex');
try{await Promise.all(['map','builtin','custom'].map(async kind=>{
 const result={kind,passed:false,requests:[]};results.push(result);
 const context=await browser.newContext({viewport:{width:1100,height:800}}),page=await context.newPage();
 const errors=[];page.on('pageerror',e=>errors.push(String(e)));page.on('request',req=>{if(new URL(req.url()).hostname==='raw.githubusercontent.com')result.requests.push(req.url())});
 const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 const modal=()=>page.waitForFunction(()=>document.querySelector('#share-dialog').open,null,{timeout:120000});
 const yes=()=>page.locator('#share-dialog button[value=yes]').click();
 const loaded=()=>page.waitForFunction(()=>{const text=document.querySelector('#status').textContent;if(text.startsWith('分享链接：'))throw Error(text);return text.startsWith('已进入地图 ')||text.startsWith('已开始播放 ')},null,{timeout:100000});
 try{
  await page.goto(url+hashes[kind]);await modal();result.prompt=await page.textContent('#share-message');assert.equal(result.prompt,parseShareLink(hashes[kind]).prompt);assert.equal(result.requests.length,0);
  if(kind==='map'){
   await page.keyboard.press('Escape');await ready();assert.equal(result.requests.length,0);assert.equal(await page.locator('#share-dialog').isVisible(),false);
   await page.reload();await modal();await ready();
   await page.evaluate(name=>{const f=ballance.player.FS;f.mkdirTree('/user/ModLoader/Maps');f.writeFile('/user/ModLoader/Maps/'+name,new Uint8Array([1]));f.writeFile('/user/ModLoader/Maps/'+name.replace('.nmo','2.nmo'),new Uint8Array([2]))},mapName);
  }
  if(kind==='builtin'){
   await ready();const bytes=[...await readFile(resources+'/原版关常规/'+builtinName)];
   await page.evaluate(({name,bytes})=>{const f=ballance.player.FS;f.mkdirTree('/user/ModLoader/TAS');f.writeFile('/user/ModLoader/TAS/'+name,new Uint8Array([1]));f.writeFile('/user/ModLoader/TAS/'+name.replace('.tas','2.tas'),new Uint8Array(bytes))},{name:builtinName,bytes});
  }
  await yes();await loaded();
  if(kind!=='map')await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).frame>500,null,{timeout:30000});
  result.state=await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])));
  assert.equal(result.state.solver,kind==='map'?0:1);
  result.files=await page.evaluate(()=>({maps:ballance.player.FS.readdir('/user/ModLoader/Maps'),tapes:ballance.player.FS.readdir('/user/ModLoader/TAS')}));
  if(kind==='map'){
   assert(result.files.maps.includes('210522图3.nmo'));assert(!result.files.maps.includes('210522图4.nmo'));
   assert.deepEqual(await page.evaluate(()=>Array.from(ballance.player.FS.readFile('/user/ModLoader/Maps/210522图.nmo'))),[1]);
   const actual=Buffer.from(await page.evaluate(()=>Array.from(ballance.player.FS.readFile('/user/ModLoader/Maps/210522图3.nmo'))));assert.equal(sha(actual),sha(await readFile(resources+'/'+folder+mapName)));
   await page.reload();await modal();await yes();await loaded();assert(!await page.evaluate(()=>ballance.player.FS.analyzePath('/user/ModLoader/Maps/210522图4.nmo').exists));result.reuse=true;
   // A new link in a running level persists, reloads once and resumes consent.
   await page.evaluate(hash=>location.hash=hash,hashes.builtin);await modal();await yes();await loaded();
   await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).frame>100,null,{timeout:30000});assert(!await page.locator('#share-dialog').isVisible());result.inLevelLink=true;
  }
  if(kind==='builtin'){assert(result.files.tapes.includes(builtinName.replace('.tas','2.tas')));assert(!result.files.tapes.includes(builtinName.replace('.tas','3.tas')))}
  await page.screenshot({path:out+'/'+kind+'.png'});assert.equal(errors.length,0);result.passed=true;
 }catch(e){result.error=String(e.stack||e);result.status=await page.locator('#status').textContent().catch(()=>null);result.log=await page.evaluate(()=>window.ballance?.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).slice(-9000)).catch(()=>null);await page.screenshot({path:out+'/'+kind+'-failed.png'}).catch(()=>{});process.exitCode=1}
 finally{await writeFile(out+'/'+kind+'.json',JSON.stringify(result,null,2));console.log(JSON.stringify({...result,log:undefined}));await context.close()}
}));}finally{await writeFile(out+'/summary.json',JSON.stringify({browser:browser.version(),results},null,2));await browser.close()}
