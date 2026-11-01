// Workspace test dependency; override for a standalone checkout.
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || new URL('../../../packaging/web/node_modules/playwright/index.mjs',import.meta.url).href);
import {mkdir,writeFile} from 'node:fs/promises';
const [url,out,mod,probe,kind='static']=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:process.env.CHROME_EXECUTABLE || '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false,kind,states:[]},logs=[];let page;
try{
 page=await browser.newPage({viewport:{width:1024,height:827},serviceWorkers:'block'});page.on('console',x=>logs.push(x.text()));page.on('pageerror',e=>logs.push(e.stack));
 const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 const advance=async n=>{const f=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(f=>ballance.gpu.frames>=f,f,{timeout:60000})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(20)};
 const click=async(x,y)=>{const b=await page.locator('canvas').boundingBox();await page.mouse.move(b.x+x*b.width/1024,b.y+y*b.height/768);await advance(5);await page.mouse.down();await advance(5);await page.mouse.up();await advance(20)};
 const state=async label=>{const s=await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebCameraPortState','string',[],[])));result.states.push({label,...s});return s};
 await page.goto(url);await ready();await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#import-mods').setInputFiles([mod,probe]);await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);
 await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();
 result.mods=await page.evaluate(()=>ballance.player.webMods.map(x=>x.id));
 await page.focus('#canvas');await key('Enter');await key('Enter');await advance(240);await key('q');await advance(200);
 const first=await state('start');await page.screenshot({path:out+'/start.png'});
 if(!first.playing)throw Error('Did not reach playing state');
 if(kind==='static'){
  if(first.camera!=='StaticCamera')throw Error('Static camera not attached');
  await key('a');if(await page.evaluate(()=>ballance.player._BML_WebCheckpointResult())!==1)throw Error('Snapshot capture failed');
  const offset=s=>s.cam.map((v,i)=>v-s.ball[i]);const distance=(a,b)=>Math.hypot(...a.map((x,i)=>x-b[i]));
  await page.keyboard.down('ArrowUp');await advance(25);await page.keyboard.up('ArrowUp');await advance(3);const moved=await state('moved');
  if(distance(first.ball,moved.ball)<.1||distance(offset(first),offset(moved))>.025)throw Error('Camera follow offset drifted');
  await page.keyboard.down('ShiftLeft');await page.keyboard.down('ArrowRight');await advance(6);await page.keyboard.up('ArrowRight');await page.keyboard.up('ShiftLeft');await advance(15);const rotated=await state('rotated'),v=offset(moved);
  if(distance(offset(rotated),[-v[2],v[1],v[0]])>.025)throw Error('Quarter turn did not preserve exact camera offset');
  await key('z');const restored=await state('restored-after-rotation');result.snapshotResult=await page.evaluate(()=>ballance.player._BML_WebCheckpointResult());result.snapshotCompatible=distance(offset(restored),offset(first))<.025;
  await page.keyboard.down('ShiftRight');await advance(6);const synced=await state('synced');await page.keyboard.up('ShiftRight');
  if(distance(synced.cam,synced.normal)>.025)throw Error('Camera sync differs from normal camera');
 }
 await key('Escape');await key('ArrowUp');await key('ArrowUp');await key('Enter');for(let i=0;i<3;i++)await key('ArrowDown');await key('Enter');await advance(30);await page.screenshot({path:out+'/mods-menu.png'});
 await click(635,116);await click(510,435);await click(510,402);await page.screenshot({path:out+'/settings.png'});
 if(kind==='static'){
  await click(563,231);await click(433,667);const disabled=await state('disabled-in-mod-menu');if(disabled.camera==='StaticCamera')throw Error('Disable config callback did not detach camera');
  await click(476,231);await click(433,667);const enabled=await state('enabled-in-mod-menu');if(enabled.camera!=='StaticCamera')throw Error('Enable config callback did not attach camera');
 }else{
  await click(476,231);await click(480,338);await page.keyboard.press('Control+A');await page.keyboard.type('5');await key('Enter');
  await click(480,447);await page.keyboard.press('Control+A');await page.keyboard.type('20');await key('Enter');
  await click(480,556);await page.keyboard.press('Control+A');await page.keyboard.type('cyan');await key('Enter');await click(433,667);
  const fog=await state('fog-config-applied');if(fog.fog===0||fog.start!==5||fog.end!==20||fog.color!==0xff00ffff)throw Error('Fog configuration callback did not apply');await page.screenshot({path:out+'/fog-cyan.png'});
 }
 for(let i=0;i<3;i++)await click(504,688);await key('Escape');await key('Escape');await advance(30);const resumed=await state('resumed');
 if(!resumed.playing)throw Error('Did not resume level after mod settings');
 if(kind==='fog'&&resumed.back!==32)throw Error('Fog far-plane adjustment not applied');
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));
 const log=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));if(kind==='static'&&!log.includes('StaticCamera unloaded'))throw Error('OnUnload was not called');result.normalExit=true;
 await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#mod-list li').filter({hasText:kind==='static'?'StaticCamera':'FogMode'}).getByRole('button',{name:'移除'}).click();await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();
 const left=await page.evaluate(()=>ballance.player.webMods.map(x=>x.id));if(left.includes(kind==='static'?'StaticCamera':'FogMode'))throw Error('Removed mod still loaded');result.removed=true;
 result.ordinaryGameplayPassed=true;result.passed=true;
}catch(e){result.error=String(e.stack||e);if(page){result.status=await page.locator('#status').textContent().catch(()=>null);await page.screenshot({path:out+'/failure.png'}).catch(()=>{})}process.exitCode=1}
finally{if(page)await writeFile(out+'/loader.log',await page.evaluate(()=>window.ballance?.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'})||'').catch(()=>''));await writeFile(out+'/browser.log',logs.join('\n'));await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
