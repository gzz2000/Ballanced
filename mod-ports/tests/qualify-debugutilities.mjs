// Dynamic DebugUtilities port: actual console/keyboard actions, no direct cheat calls.
import {chromium} from '../../packaging/web/node_modules/playwright/index.mjs';
import {mkdir,writeFile,readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
const [url,out,packagePath,tape,mode='ordinary']=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:process.env.CHROME||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const context=await browser.newContext({viewport:{width:1024,height:827},serviceWorkers:mode==='tas'?'block':'allow'}),page=await context.newPage();
const result={passed:false,mode,steps:[]},logs=[];page.on('console',m=>logs.push(m.text()));page.on('pageerror',e=>logs.push(String(e)));
if(mode==='tas')await context.route('**/app.mjs',async route=>{const response=await route.fetch();await route.fulfill({response,body:(await response.text()).replace('player=await createPlayer({canvas:',"player=await createPlayer({preRun:[m=>m.ENV.BALLANCE_FAST_FORWARD_BATCH='64'],canvas:")})});
const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>30||document.querySelector('#status').textContent.startsWith('无法启动'),null,{timeout:120000}).then(async()=>assert(await page.evaluate(()=>!!window.ballance),'Startup failed: '+await page.locator('#status').textContent()));
const advance=async n=>{const target=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,target,{timeout:60000})};
const key=async k=>{await page.keyboard.down(k);await advance(5);await page.keyboard.up(k);await advance(15)};
const state=()=>page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebDebugUtilitiesStatus','string',[],[])));
const log=()=>page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));
const mark=async name=>{const s=await state();result.steps.push({name,state:s});console.log(name,JSON.stringify(s))};
const command=async text=>{await key('/');await page.keyboard.type(text,{delay:45});await advance(8);await key('Enter');assert((await log()).includes('Execute Command: '+text),'Command not executed: '+text)};
const snapshot=async action=>{assert.equal(await page.evaluate(a=>ballance.player._BML_WebRequestCheckpoint(a),action),1);await page.waitForFunction(()=>ballance.player._BML_WebCheckpointResult()!==0,null,{timeout:60000});assert.equal(await page.evaluate(()=>ballance.player._BML_WebCheckpointResult()),action)};
const quit=async()=>{await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));assert((await log()).includes('DebugUtilities unloaded'))};
try{
 await page.goto(url);await ready();await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#import-mods').setInputFiles(packagePath);await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();
 assert.equal((await log()).split('Loading Mod DebugUtilities[').length-1,1);await mark('loaded');await page.focus('#canvas');
 await page.evaluate(()=>ballance.player._BML_WebTASSharePrepare(1));await page.waitForFunction(()=>ballance.player._BML_WebTASSharePrepare(0)===1);
 if(mode==='tas'){
  await page.evaluate(bytes=>ballance.player.FS.writeFile('/user/ModLoader/TAS/HS01.tas',new Uint8Array(bytes)),[...await readFile(tape)]);
  assert.equal(await page.evaluate(()=>ballance.player.ccall('BML_WebTASPlay','number',['string','number'],['/user/ModLoader/TAS/HS01.tas',1])),1);
  await page.waitForFunction(()=>/On Message (LevelFinish|BallOff|Dead)/.test(ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'})),null,{timeout:240000});
  assert((await log()).includes('On Message LevelFinish'));assert(!/On Message (BallOff|Dead)/.test(await log()));await mark('tas-finished');await quit();
 }else{
  await key('Enter');await page.waitForFunction(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('On Message CounterActive'),null,{timeout:60000});await key('q');await mark('level-ready');
  await command('cheat');assert((await state()).cheat);await key('Escape');
  await command('score set 12345');assert.equal((await state()).score,12345);await command('score add 100');assert.equal((await state()).score,12445);await command('score sub 50');assert.equal((await state()).score,12395);await mark('score');
  await command('speed 1');const force=(await state()).force;assert(force>0);await command('speed 2');assert.equal((await state()).force,force*2);await command('speed 1');await mark('speed');await key('Escape');
  await page.keyboard.down('ControlLeft');await advance(8);assert.equal((await state()).speed,3);await page.keyboard.up('ControlLeft');await advance(15);assert.equal((await state()).speed,1);await mark('control-speed');
  await page.keyboard.down('f');await page.waitForTimeout(150);assert((await state()).skipRender);await page.keyboard.up('f');await advance(15);assert(!(await state()).skipRender);await mark('skip-render');
  const lives=(await state()).lives;await key('l');await advance(100);assert((await state()).lives>lives);await mark('life');
  for(const [k,name] of [['i','Paper'],['o','Wood'],['p','Stone']]){await key(k);await advance(100);assert((await state()).ballType.includes(name));await mark('transform-'+name)}
  await snapshot(1);const saved=await state();await page.keyboard.down('F1');await advance(25);await page.keyboard.up('F1');await advance(10);assert((await state()).ball[1]>saved.ball[1]);await mark('force-up');await page.keyboard.down('F2');await advance(15);await page.keyboard.up('F2');await advance(5);await mark('force-down');await snapshot(2);await snapshot(2);
  const before=(await state()).phRows;
  for(const k of ['j','k','n','m']){await page.keyboard.down(k);await advance(10);await page.keyboard.up(k);await advance(20)}
  assert.equal((await state()).summoned,4);assert.equal((await state()).phRows,before+4);await mark('summon-four');await snapshot(2);assert.equal((await state()).summoned,0);await mark('restore-before-summon');
  await key('m');assert.equal((await state()).summoned,1);await snapshot(1);await key('j');assert.equal((await state()).summoned,2);await snapshot(2);assert.equal((await state()).summoned,1);await snapshot(2);await mark('overwrite-restore-summon');
  await command('spawn');assert((await log()).includes('Set Spawn Point to'));await key('Backspace');await advance(100);assert((await log()).includes('Sector Reset'));await mark('reset');
  await command('sector 2');await advance(120);assert.equal((await state()).sector,2);await mark('sector');await command('sector 999');await advance(120);assert.equal((await state()).sector,4);await mark('final-sector');await command('sector 2');await advance(120);
  await key('r');await advance(140);assert((await log()).includes('Killed Ball'));await mark('kill');
  await command('win');await page.waitForFunction(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('On Message LevelFinish'),null,{timeout:60000});await mark('win');await page.screenshot({path:out+'/win.png'});
  await quit();await page.reload();await ready();assert.equal((await log()).split('Loading Mod DebugUtilities[').length-1,1);await mark('restart');
  result.config=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/Configs/DebugUtilities.cfg',{encoding:'utf8'}));assert(result.config.includes('TurnPaper'));
  await page.waitForFunction(()=>navigator.serviceWorker.controller&&document.querySelector('#offline').textContent.includes('已就绪'),null,{timeout:120000});
  await quit();await context.setOffline(true);await page.reload();await ready();await mark('offline');await context.setOffline(false);
  await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#mod-list li').filter({hasText:'DebugUtilities'}).getByRole('button',{name:'移除'}).click();await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();assert(!(await log()).includes('Loading Mod DebugUtilities['));result.removed=true;
 }
 result.gpuErrors=await page.evaluate(()=>ballance.gpu.errors);assert.equal(result.gpuErrors.length,0);result.passed=true;
}catch(e){result.error=String(e.stack||e);result.status=await page.locator('#status').textContent().catch(()=>null);await page.screenshot({path:out+'/failure.png'}).catch(()=>{});process.exitCode=1}
finally{await writeFile(out+'/result.json',JSON.stringify(result,null,2));await writeFile(out+'/loader.log',await log().catch(()=>''));await writeFile(out+'/browser.log',logs.join('\n'));console.log(JSON.stringify({...result,steps:undefined,config:undefined}));await browser.close()}
