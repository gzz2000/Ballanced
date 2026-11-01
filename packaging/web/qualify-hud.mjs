// Capture the actual paused-level HUD and menu at multiple aspect ratios.
import {chromium} from 'playwright';
import {mkdir,writeFile} from 'node:fs/promises';
const [url,out]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false,layouts:[]};
try{
 const page=await browser.newPage({viewport:{width:1024,height:828},deviceScaleFactor:1});
 await page.addInitScript(()=>localStorage.setItem('ballance.screenMode','fill'));
 await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:0});
 const advance=async n=>{const t=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,t,{timeout:0})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(20)};
 await page.focus('#canvas');await key('Enter');await key('Enter');await advance(180);await key('q');await advance(180);await key('Escape');
 result.paused=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('On Message PauseLevel'));
 if(!result.paused)throw new Error('Did not enter/pause level');
 await page.evaluate(()=>{
  const g=ballance.gpu,begin=g.beginDraw.bind(g),end=g.endDraw.bind(g),binding=g.binding.bind(g);let frame=-1,current=null;window.hudPackets=[];
  g.beginDraw=(...args)=>{if(frame!==g.frames){frame=g.frames;hudPackets=[]}current=null;
   if(args[1]){const vertices=args[3],stride=args[9]??256,v=new DataView(vertices.buffer,vertices.byteOffset,vertices.byteLength),points=[];
    const indices=args[5],count=args[6],bytes=args[7]?4:2,iv=count?new DataView(indices.buffer,indices.byteOffset,indices.byteLength):null;
    const used=count?[...new Set(Array.from({length:count},(_,i)=>bytes===4?iv.getUint32(i*bytes,true):iv.getUint16(i*bytes,true)))].sort((a,b)=>a-b):Array.from({length:args[4]},(_,i)=>i);
    for(const i of used)points.push([v.getFloat32(i*stride,true),v.getFloat32(i*stride+4,true)]);
    current={points,textures:[]};
   }return begin(...args)};
  g.binding=(...args)=>{if(current)current.textures.push(args[1]);return binding(...args)};
  g.endDraw=()=>{if(current)hudPackets.push(current);current=null;return end()};
 });
 const header=await page.locator('header').evaluate(e=>e.getBoundingClientRect().height);
 for(const [name,width,height] of [['standard',1024,768],['wide',1600,768],['ultrawide',2200,768],['portrait',768,1024],['standard-again',1024,768]]){
  await page.setViewportSize({width,height:height+Math.round(header)});await advance(25);
  const layout=await page.evaluate(()=>({width:ballance.gpu.logicalWidth,height:ballance.gpu.logicalHeight,packets:hudPackets,errors:ballance.gpu.errors}));layout.name=name;
  await page.locator('canvas').screenshot({path:out+'/'+name+'.png'});result.layouts.push(layout);
 }
 // At equal height, a wider window should only translate the authored UI.
 const first=result.layouts[0];
 for(const layout of result.layouts.slice(1,3)){
  const shift=(layout.width-first.width)/2;
  const a=first.packets,b=layout.packets;
  let compared=0,maxError=0;
  if(a.length!==b.length)throw new Error('HUD packet count changed on width-only resize');
  for(let i=0;i<a.length;++i){if(a[i].points.length!==b[i].points.length)throw new Error('HUD glyph/quad count changed');
   for(let j=0;j<a[i].points.length;++j){const p=a[i].points[j],q=b[i].points[j];maxError=Math.max(maxError,Math.abs(q[0]-p[0]-shift),Math.abs(q[1]-p[1]));++compared}
  }
  layout.comparison={compared,maxError};if(maxError>1.01)throw new Error('HUD/text stretched or drifted: '+nameOf(layout)+' '+maxError);
 }
 // Return-to-4:3 must not accumulate offsets; plugin logo stays viewport based.
 const returned=result.layouts.at(-1);let returnError=0;
 if(first.packets.length!==returned.packets.length)throw new Error('Packet count changed on return');
 for(let i=0;i<first.packets.length;++i){
  const a=first.packets[i].points,b=returned.packets[i].points;
  if(a.length!==b.length)throw new Error('HUD glyph count changed on return');
  for(let j=0;j<a.length;++j)if(a[j][1]>80)returnError=Math.max(returnError,Math.abs(a[j][0]-b[j][0]),Math.abs(a[j][1]-b[j][1]));
 }
 result.returnError=returnError;if(returnError>1.01)throw new Error('HUD accumulated resize drift');
 // Click Options using the resized menu, then navigate back with the keyboard.
 await page.setViewportSize({width:1600,height:768+Math.round(header)});await advance(20);
 const box=await page.locator('canvas').boundingBox();await page.mouse.move(box.x+5,box.y+5);await advance(5);await page.mouse.move(box.x+800,box.y+384,{steps:8});await advance(10);await page.mouse.down();await advance(5);await page.mouse.up();await advance(20);
 await page.locator('canvas').screenshot({path:out+'/options-click.png'});
 const clicked=await page.evaluate(()=>hudPackets);if(clicked.length===first.packets.length)throw new Error('Wide menu Options click did not change menu');result.optionsClick=true;
 await key('Escape');
 if(result.layouts.some(x=>x.errors.length))throw new Error('GPU errors');result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{result.browser=browser.version();await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify({passed:result.passed,error:result.error,layouts:result.layouts.map(({name,width,height,comparison})=>({name,width,height,comparison}))}));await browser.close()}
function nameOf(layout){return layout.name}
