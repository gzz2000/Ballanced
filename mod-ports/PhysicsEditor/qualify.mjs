import {chromium} from '../../packaging/web/node_modules/playwright/index.mjs';
import {mkdir,readFile,writeFile} from 'node:fs/promises';
const [url,out,mod]=process.argv.slice(2);await mkdir(out,{recursive:true});
const bytes=[...await readFile(mod)],browser=await chromium.launch({executablePath:process.env.CHROME_PATH||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const cases=[{name:'default',y:-20,time:2,speed:1},{name:'gravity',y:-10,time:2,speed:1},{name:'physics-time',y:-20,time:1,speed:1},{name:'game-speed',y:-20,time:2,speed:.5}];
async function run(c){const r={...c,passed:false},log=[];const context=await browser.newContext({serviceWorkers:'block',viewport:{width:1024,height:827}}),page=await context.newPage();page.on('console',m=>log.push(m.text()));page.on('pageerror',e=>log.push(e.stack));try{
 await context.route('**/app.mjs',async route=>{const response=await route.fetch();let body=await response.text();const cfg=`Gravity {\nF X 0\nF Y ${c.y}\nF Z 0\n}\nTime {\nF PhysicsTimeFactor ${c.time}\nF GlobalGameSpeed ${c.speed}\n}\n`;
 body=body.replace('const loadedMods=await prepareMods',"player.FS.mkdirTree('/user/ModLoader/WebMods');player.FS.mkdirTree('/user/ModLoader/Configs');player.FS.writeFile('/user/ModLoader/WebMods/PhysicsEditor.bwmod',new Uint8Array("+JSON.stringify(bytes)+"));player.FS.writeFile('/user/ModLoader/Configs/PhysicsEditor.cfg',"+JSON.stringify(cfg)+");const loadedMods=await prepareMods");await route.fulfill({response,body});});
 const advance=async n=>{const target=await page.evaluate(n=>ballance.gpu.frames+n,n);await page.waitForFunction(n=>ballance.gpu.frames>=n,target,{timeout:60000})};
 const key=async k=>{await page.keyboard.down(k);await advance(6);await page.keyboard.up(k);await advance(18)};
 const status=()=>page.evaluate(()=>({state:JSON.parse(ballance.player.ccall('BML_WebPhysicsEditorStatus','string',[],[])),now:performance.now()}));
 await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});await page.focus('#canvas');
 await key('Enter');await key('Enter');await advance(400);await key('q');await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebPhysicsEditorStatus','string',[],[])).navActivations>0,null,{timeout:60000});await advance(60);
 r.start=await status();await advance(90);r.end=await status();r.config=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/Configs/PhysicsEditor.cfg',{encoding:'utf8'}));
 if(!r.start.state.init||r.start.state.blocks.length!==6)throw Error('Six physics graphs not initialized');
 for(const b of r.start.state.blocks){if(b.gravity[1]!==c.y||b.timeFactor!==c.time)throw Error('Config did not reach graph '+JSON.stringify(b));}
 if(r.start.state.scale!==c.speed)throw Error('Time scale ignored');
 await page.keyboard.down('ArrowUp');const t=await page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebPhysicsEditorStatus','string',[],[])).tick+264);await page.waitForFunction(t=>JSON.parse(ballance.player.ccall('BML_WebPhysicsEditorStatus','string',[],[])).tick>=t,t);await page.keyboard.up('ArrowUp');r.moved=await status();
 if(Math.hypot(...r.moved.state.ball.map((v,i)=>v-r.end.state.ball[i]))<.05)throw Error('Held direction did not move ball');
 r.virtualPerTick=(r.end.state.clock-r.start.state.clock)/(r.end.state.tick-r.start.state.tick);r.wallRatio=(r.end.state.clock-r.start.state.clock)/(r.end.now-r.start.now);
 await page.screenshot({path:out+'/'+c.name+'.png'});
 await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('游戏已退出'));
 if(!await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('PhysicsEditor unloaded')))throw Error('Unload callback absent');if(Math.abs(r.virtualPerTick-1000/264*c.speed)>.02)throw Error('Game-speed clock did not scale');r.passed=true;
}catch(e){r.error=String(e.stack||e);r.modlog=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'})).catch(()=>null);r.status=await page.locator('#status').textContent().catch(()=>null);await page.screenshot({path:out+'/'+c.name+'-failure.png'}).catch(()=>{})}finally{await writeFile(out+'/'+c.name+'.log',log.join('\n'));await context.close();}return r;}
const results=await Promise.all(cases.map(run));await writeFile(out+'/result.json',JSON.stringify(results,null,2));console.log(JSON.stringify(results));await browser.close();if(results.some(r=>!r.passed))process.exitCode=1;
