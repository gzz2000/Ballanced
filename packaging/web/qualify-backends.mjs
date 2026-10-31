// Replay identical game packets/textures in WebGPU and WebGL2. Never compare
// different simulation frames, which would conflate motion with rendering.
import {chromium} from 'playwright';import {mkdir,writeFile} from 'node:fs/promises';
const [url,out]=process.argv.slice(2);await mkdir(out,{recursive:true});const result={passed:false,frames:[]};
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
try{
 const context=await browser.newContext({serviceWorkers:'block',viewport:{width:960,height:760}}),page=await context.newPage();
 await context.route('**/app.mjs',async route=>{const r=await route.fetch();let body=await r.text();body=body.replace("$('solver-status').title=",`globalThis.textureTrace=new Map();
 for(const name of ['texture','upload','deleteTexture']){const original=gpu[name];gpu[name]=function(...args){const id=args[0];if(name==='texture')textureTrace.set(id,[[name,args]]);else if(name==='deleteTexture')textureTrace.delete(id);else textureTrace.get(id)?.push([name,args.map(v=>ArrayBuffer.isView(v)?v.slice():v)]);return original.apply(this,args)}}
 $('solver-status').title=`);await route.fulfill({response:r,body})});
 await page.goto(url+'?renderer=webgpu');await page.waitForFunction(()=>window.ballance?.gpu.frames>45,null,{timeout:120000});
 await page.evaluate(async()=>{const {createBallanceGl}=await import('./webgl2.mjs');const c=document.createElement('canvas');globalThis.comparisonGl=await createBallanceGl(c,await(await fetch('./shaders-gl.json')).json())});
 const advance=async n=>{const t=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,t,{timeout:120000})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(20)};
 const compare=async()=>page.evaluate(()=>new Promise((resolve,reject)=>{
  const g=ballance.gpu,h=comparisonGl,original={},commands=[];let recording=false,depth=0;
  g.present();g.context.configure({device:g.device,format:g.format,alphaMode:'opaque',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});
  for(const name of ['clear','clearRect','beginDraw','uniform','binding','endDraw']){original[name]=g[name];g[name]=function(...args){if(recording&&!depth)commands.push([name,args.map(v=>ArrayBuffer.isView(v)||Array.isArray(v)?v.slice():v)]);++depth;try{return original[name].apply(g,args)}finally{--depth}}}
  const present=g.present;let started=false;
  g.present=function(){
   if(!started){started=true;recording=true;return present.call(g)}
   recording=false;g.present=present;for(const name in original)g[name]=original[name];
   try{
    const w=g.canvas.width,height=g.canvas.height,stride=Math.ceil(w*4/256)*256;
    g.flush();const buffer=g.device.createBuffer({size:stride*height,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ}),enc=g.device.createCommandEncoder();enc.copyTextureToBuffer({texture:g.surface},{buffer,bytesPerRow:stride},[w,height]);g.device.queue.submit([enc.finish()]);
    h.setSamples(g.samples);h.resize(g.logicalWidth,g.logicalHeight,w,height);
    for(const list of textureTrace.values())for(const [name,args] of list)h[name](...args);
    for(const [name,args] of commands)h[name](...args);h.present();
    const gl=h.gl;gl.bindFramebuffer(gl.FRAMEBUFFER,null);const bytes=new Uint8Array(w*height*4);gl.readPixels(0,0,w,height,gl.RGBA,gl.UNSIGNED_BYTE,bytes);
    const png=h.canvas.toDataURL();present.call(g);
    buffer.mapAsync(GPUMapMode.READ).then(()=>{
     const a=new Uint8Array(buffer.getMappedRange());let total=0,max=0,bad=0;const bgra=g.format.startsWith('bgra');
     const rgb=new Uint8ClampedArray(w*height*4);
     for(let y=0;y<height;y++)for(let x=0;x<w;x++){let delta=0;for(let c=0;c<3;c++){const v=a[y*stride+x*4+(bgra?2-c:c)];rgb[(y*w+x)*4+c]=v;const d=Math.abs(v-bytes[((height-y-1)*w+x)*4+c]);total+=d;max=Math.max(max,d);delta=Math.max(delta,d)}rgb[(y*w+x)*4+3]=255;if(delta>12)bad++;}
     const canvas=document.createElement('canvas');canvas.width=w;canvas.height=height;canvas.getContext('2d').putImageData(new ImageData(rgb,w,height),0,0);
     buffer.unmap();buffer.destroy();resolve({w,height,draws:commands.filter(c=>c[0]==='beginDraw').length,mean:total/(w*height*3),badFraction:bad/(w*height),max,errors:[...g.errors,...h.errors],glPng:png,gpuPng:canvas.toDataURL()});
    },reject);
   }catch(e){present.call(g);reject(e)}
  }
 }));
 for(const scene of ['menu','level']){
  if(scene==='level'){await page.focus('#canvas');await key('Enter');await key('Enter');await advance(180);await key('q');await advance(120)}
  for(const samples of [1,4]){await page.evaluate(n=>ballance.gpu.setSamples(n),samples);await advance(10);const frame=await compare();await writeFile(`${out}/${scene}-${samples}-gl.png`,Buffer.from(frame.glPng.split(',')[1],'base64'));await writeFile(`${out}/${scene}-${samples}-gpu.png`,Buffer.from(frame.gpuPng.split(',')[1],'base64'));delete frame.glPng;delete frame.gpuPng;result.frames.push({scene,samples,...frame});console.log(JSON.stringify(result.frames.at(-1)))}
 }
 if(result.frames.some(f=>f.errors.length||f.mean>3||f.badFraction>.03||f.draws<10))throw Error('Backend pixel comparison failed');result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}finally{result.browser=browser.version();await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
