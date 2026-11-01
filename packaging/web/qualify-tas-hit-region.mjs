// Legacy TAS entry must share the centered 4:3 coordinate system used to draw it.
import {chromium} from 'playwright';
import {mkdir, writeFile} from 'node:fs/promises';
const [url, out] = process.argv.slice(2);
await mkdir(out, {recursive:true});
const browser = await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome', headless:true});
const results = [];
try {
 for (const [name, width, height] of [['wide',2200,827], ['portrait',600,1059]]) {
  const context = await browser.newContext({serviceWorkers:'block', viewport:{width,height}});
  const page = await context.newPage();
  const advance = async n => {
   const target = await page.evaluate(n => ballance.gpu.frames+n, n);
   await page.waitForFunction(t => ballance.gpu.frames >= t, target, {timeout:60000});
  };
  const opened = () => page.evaluate(() => ballance.player.FS.readFile('/user/ModLoader/ModLoader.log', {encoding:'utf8'}).includes('TAS entry: opened project menu.'));
  const click = async (x,y) => {
   const box = await page.locator('canvas').boundingBox();
   await page.mouse.move(box.x+x, box.y+y);await advance(5);
   await page.mouse.down();await advance(5);await page.mouse.up();await advance(20);
  };
  try {
   await page.addInitScript(()=>localStorage.setItem('ballance.screenMode','fill'));
   await page.goto(url);
   await page.waitForFunction(() => window.ballance?.gpu.frames > 30, null, {timeout:120000});
   await page.focus('#canvas');await page.keyboard.down('Enter');await advance(6);await page.keyboard.up('Enter');await advance(20);
   const size = await page.evaluate(() => ({w:ballance.gpu.logicalWidth,h:ballance.gpu.logicalHeight}));
   // The old full-window location must no longer activate the entry.
   await click(size.w*.645,size.h*.8977);
   if (await opened()) throw Error('Old full-window hit region is still active: '+JSON.stringify(size));
   const uiw = Math.min(size.w,size.h*4/3), uih = uiw*3/4;
   await click((size.w-uiw)/2+uiw*.645,(size.h-uih)/2+uih*.8977);
   if (!await opened()) throw Error('Visible TAS entry did not open its project menu');
   await page.screenshot({path:`${out}/${name}.png`});
   const errors = await page.evaluate(() => ballance.gpu.errors);
   if (errors.length) throw Error(errors.join('\n'));
   results.push({name,passed:true,size});
  } finally {await context.close();}
 }
} catch (e) {results.push({passed:false,error:String(e.stack||e)});process.exitCode=1;}
finally {await writeFile(`${out}/result.json`,JSON.stringify(results,null,2));console.log(JSON.stringify(results));await browser.close();}
