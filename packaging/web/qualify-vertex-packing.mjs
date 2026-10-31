// Replay a captured game frame with dense and compact attributes. Pixel equality
// tests defaults, integer inputs, the used texture stages and both 2D/3D shaders.
import {chromium} from 'playwright';
import {writeFile} from 'node:fs/promises';
const [url,out]=process.argv.slice(2),result={passed:false,frames:[]};
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
try{
 const page=await browser.newPage({viewport:{width:1280,height:800},deviceScaleFactor:2});
 const target=new URL(url);target.searchParams.set('renderer','webgpu');await page.goto(target.href);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:0});
 const advance=async n=>{const t=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,t,{timeout:0})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(20)};
 const compare=async()=>page.evaluate(()=>new Promise((resolve,reject)=>{
  const g=ballance.gpu,original={},commands=[];let recording=false,depth=0;
  g.present();g.context.configure({device:g.device,format:g.format,alphaMode:'opaque',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});
  for(const name of ['clear','clearRect','beginDraw','uniform','binding','endDraw']){
   original[name]=g[name];g[name]=function(...args){
    if(recording&&!depth)commands.push([name,args.map(v=>ArrayBuffer.isView(v)||Array.isArray(v)?v.slice():v)]);
    ++depth;try{return original[name].apply(g,args)}finally{--depth}
   };
  }
  const present=g.present;let started=false;
  g.present=function(){
   if(!started){started=true;recording=true;return present.call(g)}
   recording=false;g.present=present;for(const name in original)g[name]=original[name];
   const width=g.canvas.width,height=g.canvas.height,stride=Math.ceil(width*4/256)*256;
   const read=()=>{g.flush();const b=g.device.createBuffer({size:stride*height,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ}),e=g.device.createCommandEncoder();e.copyTextureToBuffer({texture:g.surface},{buffer:b,bytesPerRow:stride},[width,height]);g.device.queue.submit([e.finish()]);return b};
   try{
    const a=read();let compactBytes=0,denseBytes=0,draws=0;
    for(const [name,args] of commands){
     if(name==='beginDraw'){
      const [state,pt,layout,vertices,count,indices,indexCount,index32,mask,vertexStride,defaults]=args;
      const dense=new Uint8Array(count*256);compactBytes+=vertices.length+(mask===65535?0:256);denseBytes+=dense.length;++draws;
      for(let v=0;v<count;++v){dense.set(defaults,v*256);let slot=0;for(let i=0;i<16;++i)if(mask&(1<<i)){dense.set(vertices.subarray(v*vertexStride+slot*16,v*vertexStride+(slot+1)*16),v*256+i*16);++slot}}
      g.beginDraw(state,pt,layout,dense,count,indices,indexCount,index32);
     }else g[name](...args);
    }
    const b=read();present.call(g);
    Promise.all([a.mapAsync(GPUMapMode.READ),b.mapAsync(GPUMapMode.READ)]).then(()=>{
     const x=new Uint8Array(a.getMappedRange()),y=new Uint8Array(b.getMappedRange());let differences=0;for(let i=0;i<x.length;++i)differences+=x[i]!==y[i];
     a.unmap();b.unmap();a.destroy();b.destroy();resolve({draws,compactBytes,denseBytes,differences,errors:[...g.errors]});
    },reject);
   }catch(e){present.call(g);reject(e)}
  };
 }));
 for(const samples of [1,4]){await page.evaluate(n=>ballance.gpu.setSamples(n),samples);await advance(15);result.frames.push({scene:'menu',samples,...await compare()})}
 await page.focus('#canvas');await key('Enter');await key('Enter');await advance(180);await key('q');await advance(180);
 for(const samples of [1,4]){await page.evaluate(n=>ballance.gpu.setSamples(n),samples);await advance(15);result.frames.push({scene:'level',samples,...await compare()})}
 result.defaults=await page.evaluate(()=>Object.fromEntries(['Segment','BallanceTAS'].map(name=>[name,ballance.player.FS.readFile('/user/ModLoader/Configs/'+name+'.cfg',{encoding:'utf8'})])));
 if(!/B Enabled\? 0/.test(result.defaults.Segment)||!/B ShowOSD 0/.test(result.defaults.BallanceTAS))throw new Error('Overlay defaults not disabled');
 if(result.frames.some(f=>f.differences||f.errors.length||f.draws<10||f.compactBytes>=f.denseBytes))throw new Error('Compact stream differs from dense layout');
 result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{result.browser=browser.version();await writeFile(out,JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
