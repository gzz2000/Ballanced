// Real Graphics menu choices, persistence, fixed sizes, Auto and extended-view mask.
import {chromium} from 'playwright';
import {mkdir,writeFile} from 'node:fs/promises';
const [url,out]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false,choices:[]};
try{
 const page=await browser.newPage({viewport:{width:1600,height:1259},serviceWorkers:'block'});
 const start=async()=>{await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});await page.focus('#canvas')};
 const advance=async(n=20)=>{const t=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,t)};
 const key=async k=>{await page.keyboard.down(k);await advance(5);await page.keyboard.up(k);await advance()};
 const open=async()=>{await key('ArrowDown');await key('ArrowDown');await key('Enter');await key('Enter')};
 const apply=async()=>{for(let i=0;i<3;i++)await key('ArrowDown');await key('Enter');await advance()};
 const inspect=()=>page.evaluate(()=>{const c=document.querySelector('canvas').getBoundingClientRect(),s=document.querySelector('#stage').getBoundingClientRect();return {selected:ballance.player.browserResolution,size:[ballance.gpu.logicalWidth,ballance.gpu.logicalHeight],css:[c.width,c.height],center:[c.x+c.width/2-s.x-s.width/2,c.y+c.height/2-s.y-s.height/2]}});
 const check=async expected=>{const v=await inspect();result.choices.push(v);if(JSON.stringify(v.selected)!==JSON.stringify(expected))throw Error('Wrong selected mode '+JSON.stringify(v));const scale=expected[0]?Math.min(1,1600/expected[0],1200/expected[1]):1;const w=expected[0]?Math.floor(expected[0]*scale/4)*4:1600;if(v.size[0]!==w||v.size[1]!==w*3/4||v.css.some((x,i)=>x!==v.size[i])||v.center.some(x=>Math.abs(x)>.5))throw Error('Wrong dimensions '+JSON.stringify(v))};
 await start();await open();await page.screenshot({path:out+'/auto-label.png'});await apply();await check([0,0]);
 for(const size of [[640,480],[800,600],[1024,768],[1280,960],[1600,1200],[1920,1440],[2560,1920]]){
  await key('Enter');await key('ArrowRight');await apply();await check(size);
 }
 await page.reload();await page.waitForFunction(()=>window.ballance?.gpu.frames>30);await check([2560,1920]);
 await page.focus('#canvas');await open();await page.screenshot({path:out+'/persisted-label.png'});
 // Back to Auto through the real menu, then to an exact 800x600 window.
 for(let i=0;i<7;i++)await key('ArrowLeft');await apply();await check([0,0]);
 await key('Enter');await key('ArrowRight');await key('ArrowRight');await apply();await check([800,600]);
 // A click in the centered canvas must hit the Graphics row after the resize.
 const b=await page.locator('canvas').boundingBox();await page.mouse.move(b.x+4,b.y+4);await advance();await page.mouse.move(b.x+b.width*.52,b.y+b.height*.35,{steps:12});await advance();await page.mouse.down();await advance(12);await page.mouse.up();await advance();await page.mouse.move(2,2);
 await key('ArrowLeft');await key('ArrowLeft');await apply();await check([0,0]);
 await page.mouse.move(2,2);
 for(const [width,height] of [[1800,827],[600,1059]]){
  await page.setViewportSize({width,height});await page.locator('#screen-mode').selectOption('fill',{force:true});await advance();
  const mask=await page.evaluate(()=>{const m=document.querySelector('#extended-view-mask'),c=m.firstElementChild,r=c.getBoundingClientRect(),canvas=document.querySelector('canvas').getBoundingClientRect();return {hidden:m.hidden,pointer:getComputedStyle(m).pointerEvents,core:[r.width,r.height],canvas:[canvas.width,canvas.height],center:[r.x+r.width/2-canvas.x-canvas.width/2,r.y+r.height/2-canvas.y-canvas.height/2],shadow:getComputedStyle(c).boxShadow}});
  if(mask.hidden||mask.pointer!=='none'||Math.abs(mask.core[0]*3-mask.core[1]*4)>.1||mask.center.some(x=>Math.abs(x)>.1)||mask.shadow==='none')throw Error('Bad extension mask '+JSON.stringify(mask));
  result['mask'+width]=mask;await page.screenshot({path:`${out}/mask-${width}.png`});
  await page.locator('#screen-mode').selectOption('fit',{force:true});await advance();if(!await page.locator('#extended-view-mask').isHidden())throw Error('Mask remains in fit mode');
 }
 const errors=await page.evaluate(()=>ballance.gpu.errors);if(errors.length)throw Error(errors.join('\n'));
 result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
