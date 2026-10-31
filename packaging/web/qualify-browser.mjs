// Local browser qualification only: no proprietary assets are copied into the
// source tree or served outside loopback. A fresh browser profile is disposable.
import {chromium} from 'playwright';
import {createServer} from 'node:http';
import {createReadStream} from 'node:fs';
import {readFile, writeFile, mkdir, readdir} from 'node:fs/promises';
import {resolve, dirname, join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';

const [playerArg, gameArg, templateArg, manifestArg, caseId, outputArg, shaderBundle, rendererArg, snapshotSequence, snapshotControl] = process.argv.slice(2);
if (!outputArg) throw new Error('Usage: qualify-browser.mjs PlayerWeb.mjs game template-user corpus.json case-id new-output [shaders.json [webgpu.mjs]]');
const player = resolve(playerArg), game = resolve(gameArg), output = resolve(outputArg);
const cases = JSON.parse(await readFile(manifestArg)).cases;
const selected = cases.find(c => c.id === caseId);
if (!selected) throw new Error('Unknown corpus case');
const hash = bytes => createHash('sha256').update(bytes).digest('hex');
if (hash(await readFile(selected.tape)) !== selected.tape_sha256) throw new Error('Tape hash mismatch');
await mkdir(output); // A live/finished run must not be silently overwritten.
const source = dirname(fileURLToPath(import.meta.url));
const candidateBuffers=new Map([
  ['/PlayerWeb.mjs',await readFile(player)],
  ['/PlayerWeb.wasm',await readFile(player.replace(/\.mjs$/,'.wasm'))],
]);
if (shaderBundle) candidateBuffers.set('/PlayerBrowser.wasm',candidateBuffers.get('/PlayerWeb.wasm'));
const resources = new Map([
  ['/worker.mjs', join(source, 'qualification-worker.mjs')],
  ['/graphics.mjs', join(source, 'qualification-graphics.mjs')],
  ['/webgpu.mjs', join(source, '../../Source/RenderEngine/src/CKRasterizer/CKWebGpuRasterizer/webgpu.mjs')],
]);
if (shaderBundle) resources.set('/shaders.json',resolve(shaderBundle));
if (shaderBundle) {
  candidateBuffers.set('/webgpu.mjs',await readFile(rendererArg ? resolve(rendererArg) : resources.get('/webgpu.mjs')));
  candidateBuffers.set('/shaders.json',await readFile(resolve(shaderBundle)));
}
const assets = [];
async function addFile(path, vfs) {
  const url = '/asset/' + assets.length;
  resources.set(url, path); assets.push({url, vfs});
}
async function addTree(path, vfs) {
  for (const entry of await readdir(path, {withFileTypes:true})) {
    if (entry.isDirectory()) await addTree(join(path, entry.name), `${vfs}/${entry.name}`);
    else if (entry.isFile()) await addFile(join(path, entry.name), `${vfs}/${entry.name}`);
  }
}
for (const name of ['3D Entities', 'Sounds', 'Text', 'Textures'])
  await addTree(join(game, name), '/game/' + name);
await addFile(join(game, 'base.cmo'), '/game/base.cmo');
for (const name of ['Player.ini', 'Database.tdb'])
  await addFile(resolve(templateArg, name), '/user/' + name);
await addFile(selected.tape, '/user/ModLoader/TAS/qualification.tas');
const config = `TAS {\n B Enable 1\n B AutoRestart 0\n B StopOnFinish 1\n}\nStartup {\n B Enabled 1\n S Project qualification\n S Level ${selected.level}\n B AutoLoad 1\n}\nOSD {\n B ShowOSD 0\n}\n`;
const destinations = new Map([
  ['stdout.log','stdout.log'], ['player.trace','player.trace'], ['retail.trace','retail.trace'],
  ['score.csv','score.csv'], ['ModLoader.log','user/ModLoader/ModLoader.log'], ['result.json','browser-result.json'],
]);
const pageHTML = shaderBundle ? `<!doctype html><meta charset="utf-8"><title>Ballanced WebGPU qualification</title>
<button id="start">Start qualification</button><canvas id="canvas" tabindex="0"></canvas>
<script type="module" src="/graphics.mjs"></script>` : `<!doctype html><meta charset="utf-8"><title>Ballanced WASM qualification</title>
<h1>Ballanced browser qualification</h1><pre id="status">Loading...</pre>
<script type="module">
const worker = new Worker('/worker.mjs', {type:'module'});
worker.onmessage = event => {
 document.querySelector('#status').textContent = JSON.stringify(event.data,null,2);
 if(event.data.done) window.qualificationResult=event.data;
};
worker.onerror = event => {
 window.qualificationResult={done:true,error:event.message};
 document.querySelector('#status').textContent=event.message;
};
</script>`;
const server = createServer(async (request, response) => {
  try {
    const url = new URL(request.url, 'http://localhost');
    response.setHeader('Cache-Control','no-store');
    if (request.method === 'POST' && url.pathname.startsWith('/result/')) {
      const name = decodeURIComponent(url.pathname.slice(8));
      if (!destinations.has(name)) {response.writeHead(404).end();return;}
      const chunks=[];let size=0;
      for await (const chunk of request) {
        size+=chunk.length;if(size>64*1024*1024) throw new Error('Result too large');
        chunks.push(chunk);
      }
      const path=join(output,destinations.get(name));
      await mkdir(dirname(path),{recursive:true});await writeFile(path,Buffer.concat(chunks));
      response.end('ok');return;
    }
    if (url.pathname === '/') {response.setHeader('Content-Type','text/html');response.end(pageHTML);return;}
    if (url.pathname === '/favicon.ico') {response.writeHead(204).end();return;}
    if (candidateBuffers.has(url.pathname)) {
      response.setHeader('Content-Type',url.pathname.endsWith('.mjs')?'text/javascript':url.pathname.endsWith('.json')?'application/json':'application/wasm');
      response.end(candidateBuffers.get(url.pathname));return;
    }
    if (url.pathname === '/manifest.json') {
      response.setHeader('Content-Type','application/json');response.end(JSON.stringify({assets,config,snapshotSequence,webCheckpoints:process.env.BALLANCE_TEST_WEB_CHECKPOINTS==='1',resizeStress:process.env.BALLANCE_TEST_RESIZE==='1'}));return;
    }
    const path=resources.get(url.pathname);
    if(!path){response.writeHead(404).end();return;}
    response.setHeader('Content-Type',path.endsWith('.mjs')?'text/javascript':path.endsWith('.wasm')?'application/wasm':'application/octet-stream');
    const stream=createReadStream(path);stream.on('error',()=>response.destroy());stream.pipe(response);
  } catch(error) {response.writeHead(500).end(String(error));}
});
await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
const browser=await chromium.launch({
  executablePath:process.env.CHROME_PATH || '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  headless:true,
});
try {
  const page=await browser.newPage();
  page.on('console',message=>console.log(message.type(),message.text()));
  page.on('pageerror',error=>console.error(error));
  await page.goto(`http://127.0.0.1:${server.address().port}/`);
  if (shaderBundle) await page.click('#start');
  await page.waitForFunction(()=>window.qualificationResult,{},{timeout:0});
  const result=await page.evaluate(()=>window.qualificationResult);
  result.case=selected.id;result.tape=selected.tape_sha256;
  result.candidate={js:hash(candidateBuffers.get('/PlayerWeb.mjs')),wasm:hash(candidateBuffers.get('/PlayerWeb.wasm'))};
  if(shaderBundle)Object.assign(result.candidate,{renderer:hash(candidateBuffers.get('/webgpu.mjs')),shaders:hash(candidateBuffers.get('/shaders.json'))});
  result.browser=browser.version();
  await writeFile(join(output,'browser-result.json'),JSON.stringify(result,null,2));
  console.log(JSON.stringify(result));
  if(result.error || result.exit!==0) process.exitCode=1;
} finally {await browser.close();await new Promise(resolve=>server.close(resolve));}
const reviewArgs=snapshotSequence ? [join(source,'review-snapshots.py'),resolve(snapshotControl),output,snapshotSequence]
  : [join(source,'review-browser.py'),resolve(manifestArg),caseId,output];
const review=await promisify(execFile)('python3',reviewArgs);
console.log(review.stdout.trim());
