// Actual page layout, HiDPI, 3D coverage/projection and pointer regression.
import {chromium} from 'playwright';
import {createServer} from 'node:http';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {resolve,sep,join} from 'node:path';
const [siteArg,outArg]=process.argv.slice(2),site=resolve(siteArg),out=resolve(outArg);
await mkdir(out);
const server=createServer(async(req,res)=>{try{
 const name=decodeURIComponent(new URL(req.url,'http://localhost').pathname),path=resolve(site,'.'+(name==='/'?'/index.html':name));
 if(!path.startsWith(site+sep))throw new Error('Invalid path');
 res.setHeader('Content-Type',path.endsWith('.mjs')||path.endsWith('.js')?'text/javascript':path.endsWith('.wasm')?'application/wasm':path.endsWith('.html')?'text/html':path.endsWith('.css')?'text/css':'application/octet-stream');res.end(await readFile(path));
}catch{res.writeHead(404).end()}});
await new Promise(r=>server.listen(0,'127.0.0.1',r));
const browser=await chromium.launch({executablePath:process.env.CHROME_PATH||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false,sizes:[]};let page;
try{
 page=await browser.newPage({viewport:{width:1400,height:850},deviceScaleFactor:2});
 await page.addInitScript(()=>{
  const original=window.matchMedia.bind(window);window.densityQueries=[];
  window.matchMedia=query=>{const result=original(query);if(query.includes('resolution'))densityQueries.push(result);return result};
 });
 page.on('pageerror',e=>console.error(e));

 await page.goto('http://127.0.0.1:'+server.address().port);
 await page.waitForFunction(()=>window.ballance?.gpu.frames>30,{},{timeout:0});
 const advance=async n=>{const t=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,t,{timeout:0})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(20)};
 const inspect=async()=>{
  await advance(15);
  const size=await page.evaluate(()=>{const c=document.querySelector('canvas'),s=document.querySelector('#stage'),b=s.getBoundingClientRect(),h=document.querySelector('header').getBoundingClientRect();return {css:[c.clientWidth,c.clientHeight],pixels:[c.width,c.height],stage:[s.clientWidth,s.clientHeight],edges:[b.x,b.y-h.bottom,innerHeight-b.bottom],dpr:devicePixelRatio,focus:document.activeElement.id,errors:ballance.gpu.errors}});
  if(String(size.css)!==String(size.stage)||size.pixels.some((x,i)=>Math.abs(x-size.css[i]*size.dpr)>1)||size.errors.length)throw new Error('Responsive sizing failed '+JSON.stringify(size));
  if(size.edges.some(v=>Math.abs(v)>1))throw new Error('Gap around game viewport '+JSON.stringify(size.edges));
  result.sizes.push(size);return size;
 };
 for(const [width,height] of [[1400,850],[900,850],[650,750],[320,700],[1400,850]]){await page.setViewportSize({width,height});await inspect()}
 await page.evaluate(()=>{
  const g=ballance.gpu,original=g.beginDraw.bind(g);window.menuQuads=[];window.menuFrame=-1;
  g.beginDraw=(...args)=>{
   if(menuFrame!==g.frames){menuFrame=g.frames;menuQuads=[]}
   if(args[1])for(let base=0;base+4<=args[4];base+=4){const v=new DataView(args[3].buffer,args[3].byteOffset,args[3].byteLength),xs=[],ys=[];
    for(let i=0;i<4;++i){xs.push(v.getFloat32((base+i)*256,true));ys.push(v.getFloat32((base+i)*256+4,true))}
    const x=Math.min(...xs),y=Math.min(...ys),w=Math.max(...xs)-x,h=Math.max(...ys)-y;
    if(w>100&&h>30&&w/h>2&&w/h<10&&Math.abs(x+w/2-document.querySelector('canvas').clientWidth/2)<10)menuQuads.push({x,y,w,h});
   }return original(...args);
  };
 });
 await page.setViewportSize({width:1150,height:850});await inspect();const menuA=await page.evaluate(()=>menuQuads);
 await page.setViewportSize({width:1500,height:850});await inspect();const menuB=await page.evaluate(()=>menuQuads);
 result.menu={before:menuA,after:menuB};
 if(menuA.length<5||menuA.length!==menuB.length||menuA.some((a,i)=>Math.abs(a.w-menuB[i].w)>0.1||Math.abs(a.h-menuB[i].h)>0.1))throw new Error('Menu quads stretched with window');
 await page.locator('canvas').screenshot({path:join(out,'wide-menu.png')});
 // A scene restricted to 4:3 leaves both outer canvas bands black. Read actual
 // compositor pixels rather than merely trusting CSS/backbuffer dimensions.
 result.edgeCoverage=await page.evaluate(async base64=>{
  const image=await createImageBitmap(new Blob([Uint8Array.from(atob(base64),c=>c.charCodeAt(0))],{type:'image/png'}));
  const canvas=new OffscreenCanvas(image.width,image.height),ctx=canvas.getContext('2d');ctx.drawImage(image,0,0);
  const {width:w,height:h}=image,pixels=ctx.getImageData(0,0,w,h).data;
  return [[5,15],[w-15,w-5]].map(([a,b])=>{
   let light=0,total=0;for(let x=a;x<b;++x)for(let y=Math.floor(h/8);y<Math.floor(h*7/8);y+=8){const i=(y*w+x)*4;light+=Math.max(pixels[i],pixels[i+1],pixels[i+2])>24;++total}return light/total;
  });
 },(await readFile(join(out,'wide-menu.png'))).toString('base64'));
 if(result.edgeCoverage.some(n=>n<.8))throw new Error('3D scene remains letterboxed: '+JSON.stringify(result.edgeCoverage));
 // Mouse hover and click Start in resized canvas coordinates, then keyboard.
 const box=await page.locator('canvas').boundingBox();await page.mouse.move(box.x+5,box.y+5);await advance(3);
 await page.mouse.move(box.x+box.width*.5,box.y+box.height*.20,{steps:6});await advance(6);await page.mouse.down();await advance(6);await page.mouse.up();await advance(30);
 await key('Enter');await advance(180);await key('q');await advance(180);await key('Escape');
 await page.evaluate(()=>{
  const g=ballance.gpu,original=g.uniform.bind(g);window.projectionDraws=[];window.projectionFrame=-1;
  g.uniform=(group,binding,bytes)=>{
   if(group===1&&binding===0&&!g.pending.positionT){
    if(projectionFrame!==g.frames){projectionFrame=g.frames;projectionDraws=[]}
    projectionDraws.push(Array.from(new Float32Array(bytes.slice(0,256).buffer)));
   }
   return original(group,binding,bytes);
  };
 });
 await page.setViewportSize({width:1150,height:850});const narrow=await inspect();const before=await page.evaluate(()=>projectionDraws);
 await page.setViewportSize({width:1500,height:850});const wide=await inspect();const after=await page.evaluate(()=>projectionDraws);
 if(narrow.css[1]!==wide.css[1])throw new Error('Projection fixture needs equal heights');
 // Recover projection from the actual submitted MVP and model-view uniforms;
 // this remains valid if a camera animation advances between two frames.
 const rows=m=>Array.from({length:4},(_,r)=>Array.from({length:4},(_,c)=>m[c*4+r]));
 const inverse=m=>{
  const a=m.map((r,i)=>[...r,...[0,1,2,3].map(j=>Number(i===j))]);
  for(let c=0;c<4;++c){let p=c;for(let r=c+1;r<4;++r)if(Math.abs(a[r][c])>Math.abs(a[p][c]))p=r;
   [a[c],a[p]]=[a[p],a[c]];const v=a[c][c];if(Math.abs(v)<1e-12)throw new Error('Singular model view');a[c]=a[c].map(x=>x/v);
   for(let r=0;r<4;++r)if(r!==c){const v=a[r][c];a[r]=a[r].map((x,j)=>x-v*a[c][j])}
  }return a.map(r=>r.slice(4));
 };
 const projection=values=>{
  const m=rows(values.slice(0,16)),v=inverse(rows(values.slice(32,48)));
  return m.map(r=>[0,1,2,3].map(c=>r.reduce((sum,x,k)=>sum+x*v[k][c],0)));
 };
 if(!before.length||!after.length)throw new Error('No 3D matrices captured');
 // Level 1's sky callback deliberately interpolates FOV toward PI. Test the
 // modal projection of actual level geometry, not that separate sky effect.
 const geometryProjection=(draws,minDraws=20)=>{
  const groups=new Map();for(const values of draws){let p;try{p=projection(values)}catch{continue}
   const key=p[0][0].toFixed(3)+':'+p[1][1].toFixed(3),group=groups.get(key)??{matrix:p,count:0};++group.count;groups.set(key,group);
  }
  const modal=[...groups.values()].sort((a,b)=>b.count-a.count)[0];
  if(!modal||modal.count<minDraws||modal.count<=draws.length/2)throw new Error('No dominant level geometry projection');return modal;
 };
 const first=geometryProjection(before),second=geometryProjection(after),a=first.matrix,b=second.matrix;
 result.projection={before:a,after:b,geometryDraws:[first.count,second.count]};
 if(Math.abs(a[0][0]*narrow.css[0]-b[0][0]*wide.css[0])>Math.abs(a[0][0]*narrow.css[0])*1e-4)throw new Error('Horizontal FOV does not expand');
 if(Math.abs(a[1][1]-b[1][1])>Math.abs(a[1][1])*1e-4)throw new Error('Vertical FOV changed');
 await page.locator('canvas').screenshot({path:join(out,'wide-paused-level.png')});
 await page.setViewportSize({width:600,height:850});const portrait=await inspect();const portraitP=geometryProjection(await page.evaluate(()=>projectionDraws),5).matrix;
 await page.setViewportSize({width:600,height:1050});const taller=await inspect();const tallerP=geometryProjection(await page.evaluate(()=>projectionDraws),5).matrix;
 result.portrait={before:portraitP,after:tallerP};
 if(Math.abs(portraitP[0][0]-tallerP[0][0])>1e-4||tallerP[1][1]>=portraitP[1][1])throw new Error('Portrait view cropped instead of expanding vertically');
 await page.locator('canvas').screenshot({path:join(out,'portrait-paused-level.png')});
 await page.setViewportSize({width:1500,height:850});await inspect();

 // Resizing must not focus the game while someone edits a form.
 await page.click('#toggle-sidebar');
 const sidebarSize=await inspect();
 if(sidebarSize.css[0]>=wide.css[0]-300)throw new Error('Desktop sidebar did not reserve space');
 await page.click('#tab-editor');await page.focus('#copy-name');await page.setViewportSize({width:1200,height:800});
 if((await inspect()).focus!=='copy-name')throw new Error('Resize stole keyboard focus');
 const cdp=await page.context().newCDPSession(page);await cdp.send('Emulation.setDeviceMetricsOverride',{width:1200,height:800,deviceScaleFactor:1,mobile:false});
 // CDP changes resolution values but emits neither resize nor media change.
 // Deliver the real browser's density notification; no game resize API is called.
 await page.evaluate(()=>[...densityQueries].forEach(q=>q.dispatchEvent(new Event('change'))));await inspect();
 await cdp.send('Emulation.setDeviceMetricsOverride',{width:1200,height:800,deviceScaleFactor:2,mobile:false});
 // CDP changes resolution values but emits neither resize nor media change.
 // Deliver the real browser's density notification; no game resize API is called.
 await page.evaluate(()=>[...densityQueries].forEach(q=>q.dispatchEvent(new Event('change'))));await inspect();
 const log=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}));
 if(!log.includes('On Message PauseLevel'))throw new Error('Resized menu click did not lead into a playable/pausable level');
 result.paused=true;
 await page.evaluate(()=>{const original=ballance.player._BallanceWebResize;window.resizeCalls=0;ballance.player._BallanceWebResize=(...args)=>{++resizeCalls;return original(...args)}});
 await advance(60);result.idleResizeCalls=await page.evaluate(()=>resizeCalls);
 if(result.idleResizeCalls!==0)throw new Error('Resize feedback loop while layout is idle');
 await page.keyboard.press('Escape');await inspect();
 if(await page.locator('#sidebar').isVisible())throw new Error('Escape did not close sidebar');
 await page.setViewportSize({width:650,height:750});const mobileClosed=await inspect();
 await page.click('#toggle-sidebar');const mobileOpen=await inspect();
 if(String(mobileClosed.css)!==String(mobileOpen.css))throw new Error('Narrow sidebar squeezed game viewport');
 await page.screenshot({path:join(out,'sidebar-narrow.png')});
 await page.click('#close-sidebar');await inspect();
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'),{},{timeout:0});
 result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1;try{result.log=await page.evaluate(()=>ballance.player.FS.readFile('/user/Player.log',{encoding:'utf8'}));await page.screenshot({path:join(out,'failure.png')})}catch{}}
finally{result.browser=browser.version();await writeFile(join(out,'result.json'),JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close();await new Promise(r=>server.close(r))}
