import {chromium} from '../../packaging/web/node_modules/playwright/index.mjs';
import {mkdir,readFile,writeFile} from 'node:fs/promises';
const [url,out,mod,tape]=process.argv.slice(2);await mkdir(out,{recursive:true});
const bytes=[...await readFile(mod)];const browser=await chromium.launch({executablePath:process.env.CHROME_PATH||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});const result={passed:false};
try{
 const context=await browser.newContext({serviceWorkers:'block',viewport:{width:1024,height:827}}),page=await context.newPage();
 await context.route('https://example.invalid/HS01.tas',async route=>route.fulfill({status:200,headers:{'Access-Control-Allow-Origin':'*'},contentType:'application/octet-stream',body:await readFile(tape)}));
 await context.route('**/app.mjs',async route=>{const response=await route.fetch();let body=await response.text();body=body.replace('player=await createPlayer({canvas:',"player=await createPlayer({preRun:[m=>m.ENV.BALLANCE_FAST_FORWARD_BATCH='64'],canvas:");body=body.replace('const loadedMods=await prepareMods',"player.FS.mkdirTree('/user/ModLoader/WebMods');player.FS.writeFile('/user/ModLoader/WebMods/RealTimeMeter.bwmod',new Uint8Array("+JSON.stringify(bytes)+"));const loadedMods=await prepareMods");await route.fulfill({response,body})});
 await page.goto(url+'#!'+new URLSearchParams({action:'tas',level:'1',record:'https://example.invalid/HS01.tas'}));await page.locator('#share-dialog button[value=yes]').click();
 await page.waitForFunction(()=>window.ballance&&JSON.parse(ballance.player.ccall('BML_WebRealTimeMeterStatus','string',[],[])).counting,null,{timeout:120000});
 result.start=await page.evaluate(()=>({state:JSON.parse(ballance.player.ccall('BML_WebRealTimeMeterStatus','string',[],[])),now:performance.now()}));
 await page.waitForFunction(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('On Message LevelFinish'),null,{timeout:180000});
 await page.waitForFunction(()=>!JSON.parse(ballance.player.ccall('BML_WebRealTimeMeterStatus','string',[],[])).counting,null,{timeout:30000});
 result.finish=await page.evaluate(()=>({state:JSON.parse(ballance.player.ccall('BML_WebRealTimeMeterStatus','string',[],[])),text:ballance.player.ccall('BML_WebRealTimeMeterText','string',[],[]),now:performance.now(),log:ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'})}));
 if(/On Message (BallOff|Dead)/.test(result.finish.log))throw Error('TAS fell');delete result.finish.log;
 if(result.finish.state.ingameMs-result.start.state.ingameMs<(result.finish.now-result.start.now)*2)throw Error('Batched TAS did not distinguish virtual time and real time');
 const target=await page.evaluate(()=>ballance.gpu.frames+60);await page.waitForFunction(n=>ballance.gpu.frames>=n,target);
 result.afterFinishText=await page.evaluate(()=>ballance.player.ccall('BML_WebRealTimeMeterText','string',[],[]));if(result.afterFinishText!==result.finish.text)throw Error('Finished timer text keeps advancing');
 await page.screenshot({path:out+'/finish.png'});result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
