const {chromium}=await import(process.env.PLAYWRIGHT_MODULE||new URL('../../../node_modules/playwright/index.mjs',import.meta.url).href);
import {mkdir,writeFile} from 'node:fs/promises';
const [url,out,mod]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:process.env.CHROME_EXECUTABLE||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false,draws:[]},errors=[];let page;
try{
 page=await browser.newPage({viewport:{width:1024,height:827},serviceWorkers:'block'});page.on('pageerror',e=>errors.push(e.stack));
 const ready=()=>page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 await page.goto(url);await ready();await page.locator('#toggle-sidebar').click();await page.locator('#tab-data').click();await page.locator('#import-mods').setInputFiles(mod);await page.waitForFunction(()=>!document.querySelector('#restart-mods').disabled);
 await Promise.all([page.waitForEvent('load'),page.locator('#restart-mods').click()]);await ready();
 for(let mode=0;mode<14;mode++){
  const item=await page.evaluate(mode=>{
   const p=ballance.player,s=JSON.parse(p.ccall('BML_WebFontDraw','string',['number'],[mode]));
   const hex=p.ccall('BML_WebFontPixelsHex','string',[],[]),raw=Uint8Array.from(hex.match(/../g),x=>parseInt(x,16)),rgba=new Uint8ClampedArray(raw.length);
   for(let i=0;i<raw.length;i+=4){rgba[i]=raw[i+2];rgba[i+1]=raw[i+1];rgba[i+2]=raw[i];rgba[i+3]=raw[i+3];}
   const c=document.createElement('canvas');c.width=256;c.height=96;c.getContext('2d').putImageData(new ImageData(rgba,256,96),0,0);
   const colored=[];for(let i=0;i<raw.length;i+=4)if(raw[i+2]>220&&raw[i+1]>150&&raw[i+1]<180&&raw[i]<60)colored.push(i/4);
   const rows=new Set(colored.map(i=>Math.floor(i/256))),bands=[];
   for(let y=0;y<96;y++)if(rows.has(y)){if(!rows.has(y-1))bands.push([y,y]);else bands[bands.length-1][1]=y;}
   let hash=2166136261;for(const value of raw)hash=Math.imul(hash^value,16777619)>>>0;
   return {...s,colorPixels:colored.length,colorBands:bands,pixelHash:hash,image:c.toDataURL()};
  },mode);
  await writeFile(`${out}/case-${mode}.png`,Buffer.from(item.image.split(',')[1],'base64'));delete item.image;result.draws.push(item);
  if(!item.copied)throw Error('Bitmap copy failed');
  if(mode===0&&item.mismatch!==0)throw Error('Asymmetric ARGB roundtrip mismatch');
  if(mode&&(!item.drawn||!item.fontInfo||item.height!==(mode>=11?16:24)||item.glyphPixels<15||item.colorPixels<5))throw Error('Glyph/color/font metrics failed: '+JSON.stringify(item));
 }
 const b=result.draws.map(x=>x.bbox);
 if(!(b[1][0]<b[2][0]&&b[2][0]<b[3][0]&&b[1][1]<b[4][1]&&b[4][1]<b[5][1]))throw Error('Alignment did not move glyphs correctly');
 for(let i=1;i<8;i++)if(b[i][0]<16||b[i][1]<8||b[i][2]>=240||b[i][3]>=88)throw Error('Rect clipping failed');
 if(![8,9,10].every(i=>result.draws[i].colorBands.length===2))throw Error('LF did not produce two glyph rows');
 if(!(b[8][1]<b[9][1]&&b[9][1]<b[10][1]))throw Error('Multiline vertical alignment failed');
 if(![11,12].every(i=>result.draws[i].colorBands.length===3))throw Error('Three-row LF/CRLF rendering failed');
 if(result.draws[11].pixelHash!==result.draws[12].pixelHash)throw Error('CRLF differs from LF');
 if(b[13][0]<16||b[13][1]<8||b[13][2]>=64||b[13][3]>=48||result.draws[13].colorBands.length<2)throw Error('Multiline clipping failed');
 result.saveCycles=await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebFontSaveCycles','string',[],[])));
 if(result.saveCycles.errors||!result.saveCycles.sizes.every(x=>x===18)||!result.saveCycles.weights.every(x=>x===700))throw Error('Repeated Save/Load changed requested font');
 result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1;if(page)await page.screenshot({path:out+'/failure.png'}).catch(()=>{});}
finally{result.browserErrors=errors;await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close();}
