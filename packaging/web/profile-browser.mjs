// Disposable real-level CPU/audio probe; no changes to the user's browser profile.
import {chromium} from 'playwright';
import {writeFile} from 'node:fs/promises';
const [url,out]=process.argv.slice(2);
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={browser:browser.version(),phases:[]};
const browserCdp=await browser.newBrowserCDPSession();
try{
 const page=await browser.newPage({viewport:{width:1280,height:800},deviceScaleFactor:2});
 await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:0});
 const advance=async n=>{const t=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(t=>ballance.gpu.frames>=t,t,{timeout:0})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(20)};
 await page.focus('#canvas');await key('Enter');await key('Enter');await advance(180);await key('q');await advance(180);await key('a');
 result.solver=await page.evaluate(()=>({precise:ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('precise x87 (TAS)')}));
 result.checkpoint=await page.evaluate(()=>ballance.player._BML_WebCheckpointResult());if(result.checkpoint!==1)throw new Error('Probe not in active level');
 await page.evaluate(()=>{
  const g=ballance.gpu;window.probe={};
  for(const name of ['createBuffer','createBindGroup']){const old=g.device[name].bind(g.device);g.device[name]=(...a)=>{probe[name]=(probe[name]||0)+1;return old(...a)}}
  const write=g.device.queue.writeBuffer.bind(g.device.queue);g.device.queue.writeBuffer=(buffer,offset,data,start,size)=>{probe.uploadBytes=(probe.uploadBytes||0)+(size??data.byteLength);return write(buffer,offset,data,start,size)};
  const node=ballance.player.SDL3.audio_playback.scriptProcessorNode,old=node.onaudioprocess;
  node.onaudioprocess=e=>{const now=performance.now();if(probe.audioLast)probe.audioGaps.push(now-probe.audioLast);probe.audioLast=now;old(e);let peak=0;const v=e.outputBuffer.getChannelData(0);for(let i=0;i<v.length;i+=32)peak=Math.max(peak,Math.abs(v[i]));probe.audioPeaks.push(peak);probe.audioExpected=1000*v.length/e.outputBuffer.sampleRate};
  window.resetProbe=()=>{window.probe={audioGaps:[],audioPeaks:[],frames:g.frames,started:performance.now()}};resetProbe();
 });
 const cdp=await page.context().newCDPSession(page);await cdp.send('Profiler.enable');await cdp.send('Performance.enable');
 for(const mode of ['idle','hold-repeat','rapid-taps']){
  await key('z');await advance(30);await page.evaluate(()=>resetProbe());
  const processBefore=(await browserCdp.send('SystemInfo.getProcessInfo')).processInfo;
  const before=(await cdp.send('Performance.getMetrics')).metrics;await cdp.send('Profiler.start');
  for(let i=0;i<150;++i){
   if(mode==='hold-repeat')await page.keyboard.down('ArrowUp');
   else if(mode==='rapid-taps')await page.keyboard.press('ArrowUp');
   await new Promise(r=>setTimeout(r,33));
  }
  await page.keyboard.up('ArrowUp');const {profile}=await cdp.send('Profiler.stop');
  const after=(await cdp.send('Performance.getMetrics')).metrics;
  const processCpu={};for(const p of (await browserCdp.send('SystemInfo.getProcessInfo')).processInfo){const old=processBefore.find(x=>x.id===p.id);if(old)processCpu[p.type]=(processCpu[p.type]||0)+p.cpuTime-old.cpuTime}
  const metrics={};for(const m of after)metrics[m.name]=m.value-(before.find(b=>b.name===m.name)?.value??0);
  const summary=await page.evaluate(()=>{const p=probe;return {elapsed:performance.now()-p.started,frames:ballance.gpu.frames-p.frames,buffers:p.createBuffer||0,uploadBytes:p.uploadBytes||0,bindGroups:p.createBindGroup||0,audioCallbacks:p.audioGaps.length,audioExpected:p.audioExpected,audioMaxGap:Math.max(...p.audioGaps),audioLate:p.audioGaps.filter(x=>x>p.audioExpected*1.5).length,silent:p.audioPeaks.filter(x=>x<1e-7).length,errors:ballance.gpu.errors}});
  const nodes=new Map(profile.nodes.map(n=>[n.id,n])),times=new Map();for(let i=0;i<profile.samples.length;++i){const n=nodes.get(profile.samples[i]),name=n.callFrame.functionName||n.callFrame.url;times.set(name,(times.get(name)||0)+profile.timeDeltas[i])}
  result.phases.push({mode,...summary,processCpu,taskSeconds:metrics.TaskDuration,scriptSeconds:metrics.ScriptDuration,top:[...times].sort((a,b)=>b[1]-a[1]).slice(0,16)});
 }
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>!ballance.running,null,{timeout:0});result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{await writeFile(out,JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
