// Net live allocator bytes at the same simulation boundary. Wasm reservation is
// reported separately: heap growth/high-water size is NOT snapshot payload size.
import {chromium} from 'playwright';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
const [url,manifestPath,out,...ids]=process.argv.slice(2);await mkdir(out,{recursive:true});
const manifest=JSON.parse(await readFile(manifestPath));
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const results=[],batch=Number(process.env.BALLANCE_PROFILE_BATCH||0);
try{
 await Promise.all(ids.map(async id=>{
  const tape=manifest.cases.find(c=>c.id===id);if(!tape)throw Error('Unknown tape '+id);
  const bytes=[...await readFile(tape.tape)],level=Number(tape.level.slice(-2));
  for(const mode of (batch?['strict']:['strict','fast'])){
   const result={id,level,mode,passed:false,actions:[]};results.push(result);
   const context=await browser.newContext({viewport:{width:800,height:650},...(batch?{serviceWorkers:'block'}:{})}),page=await context.newPage();
   if(batch)await context.route('**/app.mjs',async route=>{const response=await route.fetch();const body=(await response.text()).replace('player=await createPlayer({canvas:',`player=await createPlayer({preRun:[m=>Object.assign(m.ENV,{BALLANCE_FAST_FORWARD_BATCH:'${batch}'})],canvas:`);await route.fulfill({response,body})});
   try{
    await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
    await page.evaluate(({bytes,level})=>{const p=ballance.player;p.FS.mkdirTree('/user/ModLoader/TAS');p.FS.writeFile('/user/ModLoader/TAS/profile.tas',new Uint8Array(bytes));if(!p.ccall('BML_WebTASPlay','number',['string','number'],['/user/ModLoader/TAS/profile.tas',level]))throw Error(p.ccall('BML_WebTASError','string',[],[]))},{bytes,level});
    await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).frame>=1800,null,{timeout:120000});
    if(mode==='fast'){await page.evaluate(()=>ballance.player._BML_WebTASStop());await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).solver===0)}
    result.state=await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])));
    for(const action of [1,2,2,1,2,1,2,1]){
     if(!await page.evaluate(a=>ballance.player._BML_WebRequestCheckpoint(a),action))throw Error('Checkpoint request rejected');
     await page.waitForFunction(()=>ballance.player._BML_WebCheckpointResult()!==0,null,{timeout:30000});
     const stats=await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebCheckpointStats','string',[],[])));
     if(stats.result!==action)throw Error('Checkpoint failed '+JSON.stringify(stats));
     stats.netUsed=stats.afterUsed-stats.beforeUsed;result.actions.push(stats);
     const target=await page.evaluate(()=>ballance.gpu.frames+15);await page.waitForFunction(t=>ballance.gpu.frames>=t,target);
    }
    if(batch){
     const begin=await page.evaluate(()=>({time:performance.now(),state:JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[]))}));
     await page.waitForFunction(target=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).frame>=target,begin.state.frame+2048,{timeout:30000});
     const end=await page.evaluate(()=>({time:performance.now(),state:JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[]))}));
     result.catchup={batch,frames:end.state.frame-begin.state.frame,milliseconds:end.time-begin.time};
    }
    result.firstSnapshotNet=result.actions[0].netUsed;
    result.captures=result.actions.filter(a=>a.action===1).map(a=>a.milliseconds);
    result.restores=result.actions.filter(a=>a.action===2).map(a=>a.milliseconds);
    result.passed=true;
   }catch(e){result.error=String(e.stack||e);process.exitCode=1}
   finally{await writeFile(out+'/'+id+'-'+mode+'.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await context.close()}
  }
 }));
}finally{await writeFile(out+'/summary.json',JSON.stringify({browser:browser.version(),results},null,2));await browser.close()}
