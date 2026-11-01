// Exercise real BML settings widgets, not just CKTimeManager setters.
import {chromium} from 'playwright';
import {mkdir,writeFile} from 'node:fs/promises';
const [url,out]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false,actions:[]};let page;
try{
 page=await browser.newPage({viewport:{width:1600,height:827},serviceWorkers:'block'});
 await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 const advance=async n=>{const t=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,t,{timeout:60000})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(20)};
 const click=async(x,y)=>{const b=await page.locator('canvas').boundingBox();await page.mouse.move(b.x+x*b.width/1024,b.y+y*b.height/768);await advance(5);await page.mouse.down();await advance(5);await page.mouse.up();await advance(20)};
 const open=async()=>{await key('Escape');await key('ArrowUp');await key('ArrowUp');await key('Enter');for(let i=0;i<3;i++)await key('ArrowDown');await key('Enter');await click(510,221);await click(510,451)};
 const close=async()=>{for(let i=0;i<3;i++)await click(504,688);await key('Escape');await key('Escape')};
 const limit=async n=>{
  await open();await click(480,338);await page.keyboard.press('ControlOrMeta+A');await page.keyboard.type(String(n),{delay:70});await key('Enter');await click(433,667);
  const cfg=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/Configs/BML.cfg',{encoding:'utf8'}));
  if(!new RegExp('SetMaxFrameRate '+n+'(?:\\s|$)').test(cfg))throw Error('BML setting did not commit '+n);
  await close();result.actions.push({limit:n});
 };
 const checkpoint=async action=>{await key(action===1?'a':'z');const value=await page.evaluate(()=>ballance.player._BML_WebCheckpointResult());result.actions.push({action,result:value});if(value!==action)throw Error('Checkpoint failed '+action)};
 await page.focus('#canvas');await key('Enter');await key('Enter');await advance(240);await key('q');await advance(240);
 await checkpoint(1);await limit(132);await checkpoint(2);await limit(264);await checkpoint(2);await checkpoint(2);
 await limit(132);await checkpoint(1);await limit(264);await checkpoint(2);await checkpoint(2);await checkpoint(1);await checkpoint(2);
 await page.screenshot({path:out+'/restored.png'});
 const log=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));
 if(log.includes('Typed reference restore failed'))throw Error('Typed references failed');
 result.checkpoints=log.split(/Browser checkpoint action=/).length-1;if(result.checkpoints!==9)throw Error('A checkpoint key was not handled');
 result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{if(page)await writeFile(out+'/loader.log',await page.evaluate(()=>window.ballance?.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'})||'').catch(()=>''));await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
