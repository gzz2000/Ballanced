// Real browser keyboard events: console submit/completion/history and input handoff.
import {chromium} from 'playwright';
import {mkdir,writeFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
const [url,out]=process.argv.slice(2);
if(!url||!out)throw Error('Usage: qualify-console.mjs URL OUTPUT');
await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:process.env.CHROME||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const results=[];
try{for(const remap of (process.env.BALLANCE_TEST_BINDING==='tab'?[false]:process.env.BALLANCE_TEST_BINDING==='f6'?[true]:[false,true])){
 const result={name:remap?'fullspeed-f6':'fullspeed-tab',passed:false};results.push(result);console.log('Starting '+result.name);
 const folder=out+'/'+result.name;await mkdir(folder,{recursive:true});
 const context=await browser.newContext({serviceWorkers:'block',viewport:{width:1024,height:827}}),page=await context.newPage();
 const errors=[];page.on('pageerror',e=>errors.push(String(e)));
 if(remap)await context.route('**/app.mjs',async route=>{
  const response=await route.fetch(),source=await response.text();
  const config='Keys {\n K FullSpeed 64\n K SaveState 30\n K RestoreState 44\n}\n';
  const body=source.replace('const code=player.callMain(',`player.FS.mkdirTree('/user/ModLoader/Configs');player.FS.writeFile('/user/ModLoader/Configs/PracticeTools.cfg',${JSON.stringify(config)});const code=player.callMain(`);
  assert.notEqual(body,source);await route.fulfill({response,body});
 });
 const advance=async n=>{const target=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,target,{timeout:30000})};
 const key=async k=>{await page.keyboard.down(k);await advance(4);await page.keyboard.up(k);await advance(12)};
 const type=async text=>{await page.keyboard.type(text,{delay:70});await advance(12)};
 const log=()=>page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));
 const commands=text=>(text.match(/Execute Command:/g)||[]).length;
 const capture=name=>page.screenshot({path:folder+'/'+name+'.png'});
 try{
  await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});await page.focus('#canvas');
  assert((await log()).includes(`full-speed=${remap?64:15} `),'PracticeTools binding was not applied');
  await key('/');await type('cheat');await key('Enter');
  assert((await log()).includes('Cheat Mode On'),'Slash/cheat/Enter did not execute');
  assert.equal(commands(await log()),1);await capture('submit');
  await key('/');await type('che');await key('Tab');await capture('single-completion');
  assert.equal(commands(await log()),1,'Tab must not execute');await key('Enter');
  assert((await log()).includes('Cheat Mode Off'),'Tab completion did not execute cheat');assert.equal(commands(await log()),2);
  await key('/');await type('c');await key('Tab');await capture('candidates');
  await key('Tab');await capture('next');await page.keyboard.down('Shift');await key('Tab');await page.keyboard.up('Shift');await advance(12);await capture('previous');
  assert.equal(commands(await log()),2,'Candidate navigation executed a command');
  await key('Escape');await key('Home');await page.keyboard.down('Shift');await key('End');await page.keyboard.up('Shift');await key('Backspace');
  await type('che');await key('Tab');await key('Enter');assert.equal(commands(await log()),3,'Dismiss/edit/completion lost the command field');
  await key('/');await key('ArrowUp');await key('Enter');assert.equal(commands(await log()),4,'History did not restore the previous command');
  assert((await log()).trim().endsWith('Cheat Mode Off'),'History did not toggle cheat off');
  assert(!(await log()).includes('Full-speed override enabled'),'Typing Tab in console reached PracticeTools');
  // The first call returns 0 only from the main menu (and requests Start).
  // An Enter release leaked to the game would already have opened Start (1).
  assert.equal(await page.evaluate(()=>ballance.player._BML_WebTASSharePrepare(0)),0,'Console Enter activated the underlying Start button');
  await page.waitForFunction(()=>ballance.player._BML_WebTASSharePrepare(0)===1);
  await page.mouse.move(1,1);await key('Enter');
  await page.waitForFunction(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('On Message CounterActive'),null,{timeout:60000});
  const before=(await log()).length;
  await key('/');await type('cheat');await key('Enter');await capture('in-level-submit');
  assert((await log()).slice(before).includes('Cheat Mode On'));
  await key('/');await key('Escape');await advance(20);
  assert(!/On Message (PauseLevel|PreExitLevel)/.test((await log()).slice(before)),'Console Escape leaked into gameplay');
  await key(remap?'F6':'Tab');
  const after=(await log()).slice(before);assert(after.includes('Full-speed override enabled')&&after.includes('Full-speed override released'),'Practice binding stopped working after console closed');
  if(remap){const n=(await log()).length;await key('Tab');assert(!(await log()).slice(n).includes('Full-speed override enabled'),'Old Tab binding remained active')}
  assert.equal(await page.evaluate(()=>document.activeElement.id),'canvas','Tab moved browser focus');
  result.commands=commands(await log());result.gpuErrors=await page.evaluate(()=>ballance.gpu.errors);result.pageErrors=errors;
  assert.equal(result.gpuErrors.length,0);assert.equal(errors.length,0);result.passed=true;
 }catch(e){result.error=String(e.stack||e);await capture('failure').catch(()=>{});process.exitCode=1}
 finally{await writeFile(folder+'/loader.log',await log().catch(()=>''));await writeFile(folder+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await context.close()}
}}finally{await browser.close();await writeFile(out+'/result.json',JSON.stringify({passed:results.every(x=>x.passed),results},null,2)+'\n')}
