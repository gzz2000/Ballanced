// Same-page replay regression: real TAS picker -> Level 8 -> finish -> save
// highscore -> Home -> repeat. Batching changes presentation only; the original
// tape deltas/inputs still drive every strict physics sample.
import {chromium} from 'playwright';
import {mkdir,readFile,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
const [url,out,tape,cyclesText='3',goldenPath]=process.argv.slice(2);
if(!url||!out||!tape)throw Error('Usage: qualify-tas-repeat.mjs URL OUTPUT HS08_TAPE [CYCLES=3] [OLD_FIRST_TRACE]');
const cycles=Number(cyclesText);assert(Number.isInteger(cycles)&&cycles>=2);
await mkdir(out,{recursive:true});
const result={passed:false,cycles:[],tapeSha256:createHash('sha256').update(await readFile(tape)).digest('hex')};
const browser=await chromium.launch({executablePath:process.env.CHROME||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const context=await browser.newContext({serviceWorkers:'block',viewport:{width:1024,height:827}});
// Unlock Level 8 in the disposable profile, as in the reporter's existing
// profile. Only the pre-game unlock array is seeded; scores use the real UI.
function unlockFixture(player) {
 const path='/user/Database.tdb',bytes=player.FS.readFile(path);
 const data=Uint8Array.from(bytes,x=>(-((((x<<3)|(x>>5))&255)^0xaf))&255);
 const view=new DataView(data.buffer);let at=0,found=false;
 while(at<data.length){
  const end=data.indexOf(0,at),name=new TextDecoder().decode(data.subarray(at,end));
  const size=view.getInt32(end+1,true),base=end+5;
  if(name==='DB_Levelfreischaltung'){
   if(view.getInt32(base,true)!==1||view.getInt32(base+4,true)!==12)throw Error('Unexpected unlock schema');
   const columnEnd=data.indexOf(0,base+12);
   if(view.getInt32(columnEnd+1,true)!==1)throw Error('Unlock column is not an integer');
   for(let row=0;row<12;row++)view.setInt32(columnEnd+5+row*4,1,true);
   found=true;break;
  }
  at=base+size;
 }
 if(!found)throw Error('Unlock array missing');
 player.FS.writeFile(path,Uint8Array.from(data,x=>{const v=((-x)&255)^0xaf;return ((v>>>3)|(v<<5))&255}));
}
const page=await context.newPage(),consoleLog=[];
page.on('console',m=>consoleLog.push(m.text()));page.on('pageerror',e=>consoleLog.push(String(e)));
await context.route('**/app.mjs',async route=>{
 const response=await route.fetch(),env={BALLANCE_FAST_FORWARD_BATCH:'64',BALLANCE_TAS_STATE_TRACE:'/user/repeat.trace',BALLANCE_TAS_LIFECYCLE_TRACE:'1'};
 const source=await response.text();let body=source.replace('player=await createPlayer({canvas:',`player=await createPlayer({preRun:[m=>Object.assign(m.ENV,${JSON.stringify(env)})],canvas:`);
 assert.notEqual(body,source,'Test environment injection failed');
 const seeded=body.replace('const code=player.callMain(',`(${unlockFixture.toString()})(player);const code=player.callMain(`);
 assert.notEqual(seeded,body,'Unlock fixture injection failed');body=seeded;await route.fulfill({response,body});
});
const advance=async n=>{const target=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,target,{timeout:60000})};
const key=async k=>{await page.keyboard.down(k);await advance(4);await page.keyboard.up(k);await advance(20)};
const log=()=>page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));
const capture=async name=>page.screenshot({path:`${out}/${name}.png`});
const click=async(x,y)=>{
 const box=await page.locator('#canvas').boundingBox();
 // Fixed 4:3 canvas, normalized coordinates shared by Bui and the level menu.
 await page.mouse.move(box.x+box.width*x,box.y+box.height*y);await advance(5);
 await page.mouse.down();await advance(5);await page.mouse.up();await advance(20);
};
const readBinary=async path=>Buffer.from(await page.evaluate(path=>{
 const bytes=ballance.player.FS.readFile(path);let str='';
 for(let i=0;i<bytes.length;i+=32768)str+=String.fromCharCode(...bytes.subarray(i,i+32768));
 return btoa(str);
},path),'base64');
function records(bytes){
 assert.equal(bytes.subarray(0,8).toString(),'BALTRCE\0');assert.equal(bytes.readUInt32LE(12),344);
 const runs=[];let current=[],previous=-1n;
 for(let at=16;at+344<=bytes.length;at+=344){const record=bytes.subarray(at,at+344),tick=record.readBigUInt64LE();if(tick<previous){runs.push(current);current=[]}current.push(record);previous=tick}
 if(current.length)runs.push(current);return runs;
}
function compare(reference,actual){
 // Startup visual orientation/score retain the last scene until ball creation.
 // The physics core itself is checked, including position, velocity, impulses,
 // movement state and both interpolation quaternions, plus the core-valid flag.
 const fields=[['dt',8,4],['keys',24,4],['position',28,12],['physicsCore',152,192]];
 const differences={};const count=Math.min(reference.length,actual.length);
 for(const [name,offset,size] of fields){const mismatches=[];for(let i=0;i<count;i++)if(!reference[i].subarray(offset,offset+size).equals(actual[i].subarray(offset,offset+size)))mismatches.push(i);differences[name]={count:mismatches.length,first:mismatches.slice(0,5)}}
 return {comparedSamples:count,differences};
}
try{
 await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 await page.evaluate(bytes=>ballance.player.FS.writeFile('/user/ModLoader/TAS/HS08.tas',new Uint8Array(bytes)),[...await readFile(tape)]);
 for(let run=0;run<cycles;run++){
  const before=(await log()).length;
  // Navigate the game's own Start, TAS, project and Play buttons. No direct
  // WebTASPlay call, synthesized LevelFinish or programmatic highscore writes.
  await page.mouse.move(1,1);await page.focus('#canvas');await key('Enter');await advance(60);
  await click(.645,.8977);await capture(`run${run+1}-projects`);
  assert((await log()).slice(before).includes('TAS entry: opened project menu.'),'TAS menu did not open');
  await click(.49,.325);await capture(`run${run+1}-details`);
  await click(.49,.645);await capture(`run${run+1}-armed`);
  assert((await log()).slice(before).includes('Playing TAS: HS08'),'TAS picker did not arm HS08');
  // Leave ImGui's release-to-unblock transition before selecting the level.
  // The game resets this menu selection to Level 1 on each entry.
  await page.mouse.move(1,1);await advance(60);
  for(let i=0;i<7;i++)await key('ArrowDown');
  await capture(`run${run+1}-level8`);await key('Enter');
  await capture(`run${run+1}-loading`);
  await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).frame>0,null,{timeout:30000});
  await page.waitForFunction(n=>/On Message (BallOff|Dead|LevelFinish)/.test(ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).slice(n)),before,{timeout:240000});
  const played=(await log()).slice(before);
  assert(played.includes('On Message LevelFinish'),'Replay failed before finishing');
  assert(!/On Message (BallOff|Dead)\b/.test(played),'Replay lost a ball');
  await advance(1000);await capture(`run${run+1}-finish`);
  await page.focus('#canvas');await key('Enter');await advance(120);await key('Enter');await advance(120);
  await page.keyboard.type('Repeat');await key('Enter');await key('Enter');
  await page.mouse.move(512,710);await advance(5);await page.mouse.down();await advance(5);await page.mouse.up();await advance(180);
  await capture(`run${run+1}-saved`);
  assert((await log()).slice(before).includes('On Message PostEndLevel'),'Highscore flow did not complete');
  const database=await readBinary('/user/Database.tdb');
  result.cycles.push({run:run+1,finished:true,death:false,databaseSha256:createHash('sha256').update(database).digest('hex'),databaseBytes:database.length});
  await page.mouse.move(1,1);await key('ArrowUp');await key('Enter');await advance(120);
  assert((await log()).slice(before).includes('On Message PostExitLevel'),'Did not return Home');
  await capture(`run${run+1}-home`);console.log(JSON.stringify(result.cycles.at(-1)));
 }
 const trace=await readBinary('/user/repeat.trace'),runs=records(trace);
 assert.equal(runs.length,cycles,'Missing replay trace');
 result.samples=runs.map(run=>run.length);
 assert(result.samples.every(n=>n>=52565),'Incomplete HS08 trace');
 result.repeatComparisons=runs.slice(1).map(run=>compare(runs[0],run));
 if(goldenPath)result.goldenComparison=compare(records(await readFile(goldenPath))[0],runs[0]);
 for(const check of [...result.repeatComparisons,...(result.goldenComparison?[result.goldenComparison]:[])])for(const [name,field] of Object.entries(check.differences))assert.equal(field.count,0,`Trajectory differs: ${name}`);
 result.gpuErrors=await page.evaluate(()=>ballance.gpu.errors);assert.equal(result.gpuErrors.length,0);
 if(process.env.BALLANCE_KEEP_TEST_TRACE==='1')await writeFile(out+'/repeat.trace',trace);
 result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1;await capture('failure').catch(()=>{})}
finally{
 await writeFile(out+'/loader.log',await log().catch(()=>''));await writeFile(out+'/console.log',consoleLog.join('\n'));
 await writeFile(out+'/result.json',JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result));await browser.close();
}
