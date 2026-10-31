// Real settings tabs, user file download and previous-default migration.
import {chromium} from 'playwright';
import {readFile,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
const [url,out,mapPath]=process.argv.slice(2);
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false};
try{
 const page=await browser.newPage({viewport:{width:1400,height:850}});
 await page.addInitScript(()=>navigator.serviceWorker.addEventListener('message',e=>{if(e.data?.cacheBytes>0&&!window.ballance){window.sawEarlyProgress=true;requestAnimationFrame(()=>{const p=document.querySelector('#cache-progress'),s=document.querySelector('#stage');if(p&&!p.hidden){const a=p.getBoundingClientRect(),b=s.getBoundingClientRect();window.progressCenter={dx:Math.abs(a.x+a.width/2-b.x-b.width/2),dy:Math.abs(a.y+a.height/2-b.y-b.height/2),width:a.width};}})}}));
 await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:0});
 if(await page.locator('#quit,#capture,#restore').count())throw new Error('Removed toolbar buttons still present');
 result.earlyProgress=await page.evaluate(()=>!!window.sawEarlyProgress);if(!result.earlyProgress)throw new Error('No early transfer progress');
 result.progressCenter=await page.evaluate(()=>window.progressCenter);
 if(!result.progressCenter||result.progressCenter.dx>1||result.progressCenter.dy>60)throw new Error('Initial progress is not centered');
 if(await page.locator('.brand small').textContent()!=='by gzz2000')throw new Error('Missing author');
 if(await page.locator('#loading-screen').isVisible())throw new Error('Loading overlay covers game');
 await page.click('#toggle-sidebar');await page.selectOption('#samples','1');await page.selectOption('#samples','4');
 await page.click('#tab-data');await page.locator('#maps').setInputFiles(mapPath);
 await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('导入完成'),null,{timeout:0});
 await page.getByRole('button',{name:'▸ ModLoader',exact:true}).click();await page.getByRole('button',{name:'▸ Maps',exact:true}).click();
 await page.fill('#file-search','Level_01');
 const downloadPromise=page.waitForEvent('download');await page.getByRole('button',{name:'下载 ModLoader/Maps/Level_01.NMO',exact:true}).click();const download=await downloadPromise;
 const hash=b=>createHash('sha256').update(b).digest('hex');result.download=hash(await readFile(await download.path()))===hash(await readFile(mapPath));if(!result.download)throw new Error('Downloaded map changed');
 await page.screenshot({path:out.replace('.json','-files.png')});
 await page.focus('#tab-data');await page.keyboard.press('ArrowRight');if(await page.locator('#tab-editor').getAttribute('aria-selected')!=='true')throw new Error('Tab keyboard navigation failed');
 await page.keyboard.press('Escape');if(await page.locator('#sidebar').isVisible())throw new Error('Esc did not close settings');
 const stop=async()=>{await page.evaluate(()=>ballance.player._BallanceWebRequestQuit());await page.waitForFunction(()=>!ballance.running&&document.querySelector('#status').textContent.includes('游戏已退出'),null,{timeout:0})};
 const start=async()=>{await Promise.all([page.waitForEvent('load'),page.click('#restart')]);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:0})};
 await stop();
 await page.evaluate(async()=>{
  const fs=ballance.player.FS,path='/user/ModLoader/Configs/PracticeTools.cfg';
  let s=fs.readFile(path,{encoding:'utf8'}).replace(/K SaveState \d+/,'K SaveState 63').replace(/K RestoreState \d+/,'K RestoreState 66');
  fs.writeFile(path,s);fs.unlink('/user/.web-practice-keys-v2');
  for(const [name,key] of [['Segment','Enabled?'],['BallanceTAS','ShowOSD']]){const p='/user/ModLoader/Configs/'+name+'.cfg';fs.writeFile(p,fs.readFile(p,{encoding:'utf8'}).replace('B '+key+' 0','B '+key+' 1'))}
  fs.unlink('/user/.web-overlays-v1');await new Promise((r,j)=>fs.syncfs(false,e=>e?j(e):r()));
 });
 await start();
 result.migrated=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('save=30 restore=44'));
 result.overlayMigrated=await page.evaluate(()=>['Segment','BallanceTAS'].every((name,i)=>ballance.player.FS.readFile('/user/ModLoader/Configs/'+name+'.cfg',{encoding:'utf8'}).includes('B '+['Enabled?','ShowOSD'][i]+' 0')));
 if(!result.overlayMigrated)throw new Error('Overlay defaults did not migrate');
 if(!result.migrated)throw new Error('Previous defaults did not migrate');await stop();
 await page.evaluate(async()=>{
  const fs=ballance.player.FS,path='/user/ModLoader/Configs/PracticeTools.cfg';
  for(const [name,key] of [['Segment','Enabled?'],['BallanceTAS','ShowOSD']]){const p='/user/ModLoader/Configs/'+name+'.cfg';fs.writeFile(p,fs.readFile(p,{encoding:'utf8'}).replace('B '+key+' 0','B '+key+' 1'))}
  fs.writeFile(path,fs.readFile(path,{encoding:'utf8'}).replace(/K SaveState \d+/,'K SaveState 31').replace(/K RestoreState \d+/,'K RestoreState 32'));
  await new Promise((r,j)=>fs.syncfs(false,e=>e?j(e):r()));
 });
 await start();result.custom=await page.evaluate(()=>ballance.player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'}).includes('save=31 restore=32'));
 result.overlayCustom=await page.evaluate(()=>['Segment','BallanceTAS'].every((name,i)=>ballance.player.FS.readFile('/user/ModLoader/Configs/'+name+'.cfg',{encoding:'utf8'}).includes('B '+['Enabled?','ShowOSD'][i]+' 1')));
 if(!result.overlayCustom)throw new Error('Custom overlay choices overwritten');
 if(!result.custom)throw new Error('Custom shortcuts were overwritten');await stop();result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{result.browser=browser.version();await writeFile(out,JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
