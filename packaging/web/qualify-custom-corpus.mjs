// Compare each map/tape pair in isolated browser user storage. A terminal tape
// is recorded separately from LevelFinish; partial author demos are not passes.
import {chromium} from 'playwright';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import path from 'node:path';
const [url,manifest,out,root=process.cwd()]=process.argv.slice(2);
await mkdir(out,{recursive:true});
const cases=JSON.parse(await readFile(manifest,'utf8')).map((c,i)=>({...c,id:i})).filter(c=>c.maps.length===1&&c.scope!=='resource_override');
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const results=[];let next=0;
async function run(c){
 const context=await browser.newContext({serviceWorkers:'block',viewport:{width:800,height:650}});const page=await context.newPage();const r={id:c.id,name:c.name,scope:c.scope};const errors=[];page.on('pageerror',e=>errors.push(String(e)));
 await context.route('https://corpus.invalid/**',async route=>{const file=route.request().url().endsWith('.nmo')?c.maps[0]:c.tape;await route.fulfill({status:200,headers:{'Access-Control-Allow-Origin':'*'},body:await readFile(path.resolve(root,file))})});
 await context.route('**/app.mjs',async route=>{const response=await route.fetch();const body=(await response.text()).replace('player=await createPlayer({canvas:',"player=await createPlayer({preRun:[m=>Object.assign(m.ENV,{BALLANCE_FAST_FORWARD_BATCH:'64'})],canvas:");await route.fulfill({response,body})});
 try{
  await page.goto(url+'#!'+new URLSearchParams({action:'tas',map:'https://corpus.invalid/candidate.nmo',record:'https://corpus.invalid/candidate.tas'}));await page.locator('#share-dialog button[value=yes]').click();
  await page.waitForFunction(()=>window.ballance&&JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).frame>100,null,{timeout:120000});
  await page.waitForFunction(()=>{const s=JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[]));return s.state==='idle'},null,{timeout:240000});
  r.terminal=true;
 }catch(e){r.error=String(e.stack||e)}
 try{const log=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));await writeFile(out+'/'+c.id+'.log',log);r.finish=(log.match(/On Message LevelFinish/g)||[]).length;r.falls=(log.match(/On Message BallOff/g)||[]).length;r.checkpoints=(log.match(/On Message PostCheckpoint/g)||[]).length;r.completed=log.includes('Record playback completed naturally');r.gpuErrors=await page.evaluate(()=>ballance.gpu.errors);r.state=await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])));}catch(e){r.captureError=String(e)}
 r.errors=errors;results.push(r);await writeFile(out+'/'+c.id+'.json',JSON.stringify(r,null,2));console.log(JSON.stringify(r));await context.close();
}
try{await Promise.all(Array.from({length:3},async()=>{for(;;){const c=cases[next++];if(!c)break;await run(c)}}))}finally{await writeFile(out+'/summary.json',JSON.stringify(results.sort((a,b)=>a.id-b.id),null,2));await browser.close()}
