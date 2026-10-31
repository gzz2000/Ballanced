// Complete the user's shared examples with unchanged strict simulation, batching
// 64 samples per render only in the disposable test host.
import {chromium} from 'playwright';import {mkdir,writeFile} from 'node:fs/promises';
const [url,out,only]=process.argv.slice(2);await mkdir(out,{recursive:true});
const base='https://github.com/misiroqwq/ballance_misc/raw/refs/heads/main/TAS_Resources/TAS文件合集/';
const folder=base+'其他/HS 210522 2.20.095 2025-10-12 by fluoresce/';
const cases={builtin:{action:'tas',level:'1',record:base+'原版关常规/HS01_4740_1.30.344_264_口语大赛_onlyforyou.tas'},custom:{action:'tas',map:folder+'210522图.nmo',record:folder+'210522_2.20.095.tas'}};
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});const results=[];
try{await Promise.all(Object.entries(cases).filter(([kind])=>!only||kind===only).map(async([kind,params])=>{
 const result={kind,passed:false};results.push(result);const context=await browser.newContext({serviceWorkers:'block',viewport:{width:960,height:780}}),page=await context.newPage();
 if(process.env.BALLANCE_TEST_REALTIME!=='1')await context.route('**/app.mjs',async route=>{const response=await route.fetch();const body=(await response.text()).replace('player=await createPlayer({canvas:',"player=await createPlayer({preRun:[m=>Object.assign(m.ENV,{BALLANCE_FAST_FORWARD_BATCH:'64'})],canvas:");await route.fulfill({response,body})});
 try{
  await page.goto(url+'#!'+new URLSearchParams(params));await page.locator('#share-dialog button[value=yes]').click();
  await page.waitForFunction(()=>window.ballance&&JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).frame>100,null,{timeout:120000});
  await page.waitForFunction(()=>{const log=ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'});const s=JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[]));return log.includes('On Message LevelFinish')||s.state==='idle'},null,{timeout:240000});
  result.outcome=await page.evaluate(()=>{const log=ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'});return {finished:log.includes('On Message LevelFinish'),death:log.includes('On Message BallOff')||log.includes('On Message Dead'),state:JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])),errors:ballance.gpu.errors}});
  await page.screenshot({path:out+'/'+kind+'.png'});
  if(!result.outcome.finished||result.outcome.death||result.outcome.errors.length)throw Error('Shared tape did not finish cleanly');result.passed=true;
 }catch(e){result.error=String(e.stack||e);result.log=await page.evaluate(()=>window.ballance?.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).slice(-8000)).catch(()=>null);process.exitCode=1}
 finally{await writeFile(out+'/'+kind+'.json',JSON.stringify(result,null,2));console.log(JSON.stringify({...result,log:undefined}));await context.close()}
}));}finally{await writeFile(out+'/summary.json',JSON.stringify({browser:browser.version(),results},null,2));await browser.close()}
