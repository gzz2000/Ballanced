import {chromium} from 'playwright';
import {mkdir,writeFile} from 'node:fs/promises';
const [url,out]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false,checks:[]};
try{
 const page=await browser.newPage({viewport:{width:1600,height:827},serviceWorkers:'block'});
 await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 const advance=async()=>{const t=await page.evaluate(()=>ballance.gpu.frames+15);await page.waitForFunction(t=>ballance.gpu.frames>=t,t)};
 const inspect=async()=>page.evaluate(()=>{
  const c=document.querySelector('canvas').getBoundingClientRect(),s=document.querySelector('#stage').getBoundingClientRect();
  return {mode:document.querySelector('#screen-mode').value,width:ballance.gpu.logicalWidth,height:ballance.gpu.logicalHeight,
   gapX:s.width-c.width,gapY:s.height-c.height,offsetX:c.x+c.width/2-s.x-s.width/2,offsetY:c.y+c.height/2-s.y-s.height/2};
 });
 for(const mode of ['fit','fill','fit']){
  await page.locator('#screen-mode').selectOption(mode,{force:true});
  for(const [width,height] of [[1600,827],[600,1059],[1024,827]]){
   await page.setViewportSize({width,height});await advance();const check=await inspect();result.checks.push(check);
   if(Math.abs(check.offsetX)>.6||Math.abs(check.offsetY)>.6)throw Error('Canvas not centered');
   if(mode==='fit'&&(check.width*3!==check.height*4||check.gapX<0||check.gapY<0))throw Error('4:3 cropped or stretched');
   if(mode==='fill'&&(Math.abs(check.gapX)>1||Math.abs(check.gapY)>1))throw Error('Fill leaves unused space');
   await page.locator('#stage').screenshot({path:`${out}/${mode}-${width}.png`});
  }
 }
 // Pointer coordinates must use the centered canvas, including portrait top bars.
 await page.setViewportSize({width:600,height:1059});await advance();
 const box=await page.locator('canvas').boundingBox();
 await page.mouse.move(box.x+2,box.y+2);await advance();await page.mouse.move(box.x+box.width*.5,box.y+box.height*.2,{steps:8});await advance();
 await page.mouse.down();await advance();await page.mouse.up();await advance();
 await page.locator('#stage').screenshot({path:out+'/portrait-start-click.png'});
 // Start Level1 from the selected menu; level scripts reset the old resolution.
 await page.focus('#canvas');await page.keyboard.down('Enter');await advance();await page.keyboard.up('Enter');
 await page.waitForFunction(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('On Message StartLevel'),null,{timeout:30000});
 await advance();result.inLevel=await inspect();
 if(result.inLevel.width!==600||result.inLevel.height!==450)throw Error('Level load escaped 4:3 host dimensions');
 // Reproduce a render-only size reset while SDL still caches the host size.
 await page.evaluate(()=>ballance.gpu.resize(640,480));await advance();
 result.afterRenderReset=await inspect();
 if(result.afterRenderReset.width!==600||result.afterRenderReset.height!==450)throw Error('Render-only reset not repaired');

 const errors=await page.evaluate(()=>ballance.gpu.errors);if(errors.length)throw Error(errors.join('\n'));
 await page.locator('#screen-mode').selectOption('fill',{force:true});await page.reload();
 await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});await advance();
 result.persisted=await inspect();if(result.persisted.mode!=='fill'||Math.abs(result.persisted.gapY)>1)throw Error('Mode not persisted');
 result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
