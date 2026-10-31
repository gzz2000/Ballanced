// Pixel tests use a disposable browser and the real WebGPU device.
import {chromium} from 'playwright';
import {createServer} from 'node:http';
import {readFile,writeFile} from 'node:fs/promises';
import {resolve,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
const [shaderPath,output]=process.argv.slice(2);
if(!output)throw new Error('Usage: qualify-renderer.mjs shaders.json result.json');
const renderer=await readFile(resolve(dirname(fileURLToPath(import.meta.url)),'../../Source/RenderEngine/src/CKRasterizer/CKWebGpuRasterizer/webgpu.mjs'));
const shaders=await readFile(shaderPath);
const html=`<!doctype html><canvas id="canvas"></canvas><script type="module">
import {createBallanceGpu} from '/renderer.mjs';
try {
 const canvas=document.querySelector('canvas');
 const gpu=await createBallanceGpu(canvas,await(await fetch('/shaders.json')).json());
 gpu.context.configure({device:gpu.device,format:gpu.format,alphaMode:'opaque',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});
 const results=[];
 for(const samples of [1,4])for(const [logicalWidth,logicalHeight,width,height] of [[64,64,128,128],[96,64,192,128],[64,64,128,128]]) {
  // SDL changes the backing canvas before dispatching its resize event.
  canvas.width=width;canvas.height=height;
  gpu.setSamples(samples);gpu.resize(logicalWidth,logicalHeight,width,height);
  gpu.clearRect(112,0xffff0000,1,0,[0,0,logicalWidth,logicalHeight]);
  gpu.clearRect(112,0xff00ff00,0.5,7,[16,16,48,48]);
  gpu.flush();
  const stride=Math.ceil(width*4/256)*256;
  const buffer=gpu.device.createBuffer({size:stride*height,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
  const encoder=gpu.device.createCommandEncoder();
  encoder.copyTextureToBuffer({texture:gpu.surface},{buffer,bytesPerRow:stride},[width,height]);
  gpu.device.queue.submit([encoder.finish()]);await buffer.mapAsync(GPUMapMode.READ);
  const bytes=new Uint8Array(buffer.getMappedRange()),bgra=gpu.format.startsWith('bgra');
  const pixel=(x,y)=>{const i=y*stride+x*4;return [bytes[i+(bgra?2:0)],bytes[i+1],bytes[i+(bgra?0:2)],bytes[i+3]]};
  const outside=pixel(4,4),inside=pixel(64,64),edge=pixel(31,64);
  if(String(outside)!=='255,0,0,255'||String(inside)!=='0,255,0,255'||String(edge)!=='255,0,0,255')throw new Error('Partial clear changed unrelated pixels: '+JSON.stringify({samples,width,height,outside,inside,edge}));
  buffer.unmap();buffer.destroy();gpu.present();results.push({samples,width,height,outside,inside,edge});
 }
 await gpu.device.queue.onSubmittedWorkDone();
 if(gpu.errors.length)throw new Error(gpu.errors.join('\\n'));
 gpu.shutdown();window.result={passed:true,results};
}catch(error){window.result={passed:false,error:String(error.stack||error)}}
</script>`;
const server=createServer((req,res)=>{if(req.url==='/renderer.mjs'){res.setHeader('Content-Type','text/javascript');res.end(renderer)}else if(req.url==='/shaders.json'){res.setHeader('Content-Type','application/json');res.end(shaders)}else {res.setHeader('Content-Type','text/html');res.end(html)}});
await new Promise(r=>server.listen(0,'127.0.0.1',r));
const browser=await chromium.launch({executablePath:process.env.CHROME_PATH||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
try {
 const page=await browser.newPage();await page.goto('http://127.0.0.1:'+server.address().port);
 await page.waitForFunction(()=>window.result,{},{timeout:0});
 const result=await page.evaluate(()=>window.result);result.browser=browser.version();
 await writeFile(output,JSON.stringify(result,null,2));console.log(JSON.stringify(result));if(!result.passed)process.exitCode=1;
}finally{await browser.close();await new Promise(r=>server.close(r))}
