// Force WebGL2 even on a WebGPU-capable machine; do not use a user's profile.
import {chromium} from 'playwright';
import {mkdir,writeFile} from 'node:fs/promises';
const [url,out]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false,browser:browser.version(),errors:[]};
try{
 const context=await browser.newContext({serviceWorkers:'block',viewport:{width:960,height:760}});const page=await context.newPage();
 page.on('pageerror',e=>result.errors.push(String(e)));
 page.on('console',m=>{if(m.type()==='error')console.log(m.text().slice(0,1800))});
 await page.goto(url+'?renderer=webgl2');
 await page.waitForFunction(()=>window.ballance?.gpu?.frames>10||document.querySelector('#status')?.textContent.includes('无法'),null,{timeout:120000});
 result.start=await page.evaluate(()=>({status:document.querySelector('#status')?.textContent,backend:window.ballance?.gpu?.backend,frames:window.ballance?.gpu?.frames,errors:window.ballance?.gpu?.errors}));
 if(result.start.backend!=='webgl2')throw Error(JSON.stringify(result.start));
 await page.waitForTimeout(5000);await page.screenshot({path:out+'/menu.png'});
 result.pixel=await page.evaluate(async()=>{
   const {createBallanceGl}=await import('./webgl2.mjs');const c=document.createElement('canvas');document.body.append(c);
   const gpu=await createBallanceGl(c,await(await fetch('./shaders-gl.json')).json()),gl=gpu.gl,results=[];
   for(const samples of [1,4])for(const [lw,lh,w,h] of [[64,64,128,128],[96,64,192,128],[64,96,128,192]]){
     gpu.setSamples(samples);gpu.resize(lw,lh,w,h);gpu.clearRect(112,0xffff0000,1,0,[0,0,lw,lh]);gpu.clearRect(112,0xff00ff00,.5,7,[16,16,48,48]);gpu.present();
     gl.bindFramebuffer(gl.FRAMEBUFFER,null);const bytes=new Uint8Array(w*h*4);gl.readPixels(0,0,w,h,gl.RGBA,gl.UNSIGNED_BYTE,bytes);
     const pixel=(x,y)=>Array.from(bytes.slice(((h-1-y)*w+x)*4,((h-1-y)*w+x)*4+4));
     const values={samples:gpu.samples,w,h,outside:pixel(4,4),inside:pixel(64,64),edge:pixel(31,64)};
     if(String(values.outside)!=='255,0,0,255'||String(values.inside)!=='0,255,0,255'||String(values.edge)!=='255,0,0,255')throw Error(JSON.stringify(values));results.push(values);
   }
   for(const hardware of [true,false])for(let pt=0;pt<2;pt++)for(let layout=0;layout<3;layout++)gpu.program(pt,layout,hardware);
   gpu.checkError('all FFP variants');
   const errors=gpu.errors;gpu.shutdown();c.remove();if(errors.length)throw Error(errors.join('\n'));return results;
 });
 result.fallback=await page.evaluate(async()=>{
   const {createBallanceRenderer}=await import('./renderer.mjs'),descriptor=Object.getOwnPropertyDescriptor(navigator,'gpu'),checks=[];
   try{for(const [name,gpu] of [['missing',undefined],['no-adapter',{requestAdapter:async()=>null}],['device-rejected',{requestAdapter:async()=>({requestDevice:async()=>{throw Error('test device rejected')}})}]]){
     Object.defineProperty(navigator,'gpu',{configurable:true,value:gpu});const canvas=document.createElement('canvas'),renderer=await createBallanceRenderer(canvas);
     if(renderer.backend!=='webgl2'||!renderer.fallbackReason)throw Error('Automatic fallback failed');checks.push({name,backend:renderer.backend,reason:renderer.fallbackReason});renderer.shutdown();
   }}finally{if(descriptor)Object.defineProperty(navigator,'gpu',descriptor);else delete navigator.gpu}return checks;
 });
 result.end=await page.evaluate(()=>({frames:ballance.gpu.frames,draws:ballance.gpu.draws,errors:ballance.gpu.errors,programs:[...ballance.gpu.programs.keys()],shaders:Object.keys(ballance.gpu.shaders)}));
 if(result.errors.length||result.end.errors.length)throw Error('Browser/GL errors');result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}finally{await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
