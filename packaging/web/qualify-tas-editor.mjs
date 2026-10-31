// Exercise real DOM gestures and the native record codec/solver lifecycle.
import {chromium} from 'playwright';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {inflateSync,deflateSync} from 'node:zlib';
const [url,out,tape]=process.argv.slice(2);await mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});
const result={passed:false},errors=[];
try{
 const page=await browser.newPage({viewport:{width:1500,height:1000}});page.on('pageerror',e=>errors.push(String(e)));
 await page.goto(url);await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 const state=()=>page.evaluate(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])));
 if(await page.textContent('.brand')!=='BallanceWebby gzz2000'||await page.locator('#tas-status').isVisible())throw Error('Brand or idle TAS indicator');
 result.initial=await state();if(result.initial.solver!==0||result.initial.state!=='idle')throw Error('Initial solver/state');
 await page.click('#toggle-sidebar');await page.click('#tab-data');await page.locator('#tapes').setInputFiles(tape);
 await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('导入完成'));
 await page.click('#tab-editor');await page.selectOption('#records',{label:tape.split('/').pop()});await page.click('#load-record');
 await page.waitForFunction(()=>document.querySelector('#record-info').textContent.includes('次采样'));
 const frames=Number((await page.textContent('#record-info')).match(/\d+/)[0]);
 const samples=()=>page.evaluate(()=>Array.from({length:20},(_,i)=>[ballance.player._BML_WebTASKeys(i),ballance.player._BML_WebTASDelta(i)]));
 const original=await samples();
 const cell=(row,bit)=>page.locator(`#record-grid button[data-row="${row}"][data-bit="${bit}"]`);
 await cell(2,2).click();let after=await samples();
 if(after[2][0]!== (original[2][0]^4)||after.some((x,i)=>x[1]!==original[i][1]||(i!==2&&x[0]!==original[i][0])))throw Error('Cell toggle changed other input/timing');
 await page.click('#undo');if(JSON.stringify(await samples())!==JSON.stringify(original))throw Error('Click undo');
 // A whole painted stroke is a single undo unit, including skipped pointer rows.
 await page.locator('#record-grid').scrollIntoViewIfNeeded();
 const a=await cell(2,3).boundingBox(),b=await cell(8,3).boundingBox();
 await page.mouse.move(a.x+a.width/2,a.y+a.height/2);await page.mouse.down();await page.mouse.move(b.x+b.width/2,b.y+b.height/2,{steps:6});await page.mouse.up();
 after=await samples();const down=!(original[2][0]&8);
 for(let i=0;i<20;++i){const expected=i>=2&&i<=8?((original[i][0]&~8)|(down?8:0)):original[i][0];if(after[i][0]!==expected||after[i][1]!==original[i][1])throw Error('Paint range '+i+' actual='+JSON.stringify(after[i])+' expected='+expected)}
 await page.click('#undo');if(JSON.stringify(await samples())!==JSON.stringify(original))throw Error('Stroke undo');
 // Selection by row headers, then column fill preserves all other columns.
 await page.locator('#record-grid button.sample-number[data-row="3"]').click();await page.locator('#record-grid button.sample-number[data-row="7"]').click({modifiers:['Shift']});
 await page.click('#mode-fill');await page.locator('#record-grid button[data-column="4"]').click();after=await samples();
 const fillDown=!original.slice(3,8).every(x=>x[0]&16);
 for(let i=0;i<20;++i){const expected=i>=3&&i<=7?((original[i][0]&~16)|(fillDown?16:0)):original[i][0];if(after[i][0]!==expected||after[i][1]!==original[i][1])throw Error('Column fill '+i)}
 await page.click('#undo');
 await page.locator('#record-grid input[data-delta="5"]').fill('4.25');await page.locator('#record-grid input[data-delta="5"]').press('Tab');
 after=await samples();if(after[5][1]!==4.25||after[5][0]!==original[5][0])throw Error('Interval edit');await page.click('#undo');
 await page.locator('#record-grid').evaluate(e=>e.scrollTop=e.scrollHeight);
 await page.waitForTimeout(300);
 result.scroll=await page.locator('#record-grid').evaluate(e=>({top:e.scrollTop,height:e.scrollHeight,client:e.clientHeight,rows:[...e.querySelectorAll('tr[data-sample]')].map(r=>[r.dataset.sample,r.getBoundingClientRect().height])}));
 if(!await page.locator(`#record-grid button.sample-number[data-row="${frames-1}"]`).count())throw Error('Last row missing: '+JSON.stringify(result.scroll));
 result.grid={frames,rendered:await page.locator('#record-grid tbody tr').count()};if(result.grid.rendered>45)throw Error('Grid not virtualized');
 await page.locator('#record-grid').evaluate(e=>e.scrollTop=0);await page.waitForTimeout(100);
 await page.focus('#record-grid');await page.keyboard.press('Home');await page.keyboard.press('Shift+ArrowDown');if(await page.inputValue('#first')!=='0'||await page.inputValue('#last')!=='1')throw Error('Keyboard range selection');
 await page.locator('#sidebar').evaluate(e=>e.scrollTop=0);
 result.headerTop=await page.locator('header').evaluate(e=>e.getBoundingClientRect().top);if(result.headerTop!==0)throw Error('Editor focus scrolled the page/header');await page.screenshot({path:out+'/editor.png'});
 await page.fill('#copy-name','visual-roundtrip.tas');await page.click('#save-record');await page.waitForFunction(()=>document.querySelector('#editor-status').textContent.includes('副本已保存'));
 const bytes=await readFile(tape),raw=inflateSync(bytes.subarray(4)),copy=Buffer.from(await page.evaluate(()=>Array.from(ballance.player.FS.readFile('/user/ModLoader/TAS/visual-roundtrip.tas'))));
 if(!inflateSync(copy.subarray(4)).equals(raw))throw Error('Undo/save failed bit-exact record roundtrip');result.roundtrip=true;
 // A prefix-only fixture deliberately reaches EOF before the level ends.
 const prefix=raw.subarray(0,(raw.length/frames)*360),header=Buffer.alloc(4);header.writeUInt32LE(prefix.length);const short=Buffer.concat([header,deflateSync(prefix)]);
 await page.evaluate(bytes=>ballance.player.FS.writeFile('/user/ModLoader/TAS/short-eof.tas',new Uint8Array(bytes)),[...short]);
 await page.evaluate(()=>ballance.persist());await page.selectOption('#records',{label:'short-eof.tas'});
 await page.click('#play-record');await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).state==='playing',null,{timeout:120000});
 result.playing=await state();if(result.playing.solver!==1||!result.playing.canStop||result.playing.total!==360)throw Error('Playback status');
 await page.screenshot({path:out+'/playing.png'});
 await page.waitForFunction(()=>{const s=JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[]));return s.state==='idle'&&s.solver===0},null,{timeout:30000});
 await page.waitForTimeout(1000);result.eof=await state();if(await page.locator('#tas-status').isVisible())throw Error('Idle TAS indicator remained visible');if(result.eof.state!=='idle'||result.eof.canStop||result.eof.solver!==0)throw Error('EOF rearmed replay');
 // Reload returns to menu while preserving files, then exercise click-to-stop.
 await page.reload();await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 await page.click('#toggle-sidebar');await page.click('#tab-editor');await page.selectOption('#records',{label:tape.split('/').pop()});await page.click('#play-record');
 await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).state==='playing',null,{timeout:120000});
 await page.waitForFunction(()=>JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[])).frame>800,null,{timeout:30000});
 await page.click('#tas-status');await page.waitForFunction(()=>{const s=JSON.parse(ballance.player.ccall('BML_WebTASStatus','string',[],[]));return s.state==='idle'&&s.solver===0});
 result.stopped=await state();if(result.stopped.editable)throw Error('Stop test did not reach active level');if(!await page.locator('#tas-status').isDisabled())throw Error('Stop button still active');
 await page.reload();await page.waitForFunction(()=>window.ballance?.gpu.frames>30,null,{timeout:120000});
 result.pending=await page.evaluate(path=>{const p=ballance.player;const ok=p.ccall('BML_WebTASPlay','number',['string','number'],[path,1]);const before=JSON.parse(p.ccall('BML_WebTASStatus','string',[],[]));p._BML_WebTASStop();return {ok,before}},'/user/ModLoader/TAS/'+tape.split('/').pop());
 if(!result.pending.ok||!result.pending.before.canStop)throw Error('Pending replay not armed');
 await page.waitForTimeout(1500);result.cancelled=await state();if(result.cancelled.state!=='idle'||result.cancelled.solver!==0)throw Error('Pending cancellation rearmed');
 result.errors=errors;if(errors.length)throw Error('Browser errors');result.passed=true;
}catch(e){result.error=String(e.stack||e);process.exitCode=1}
finally{result.browser=browser.version();await writeFile(out+'/result.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close()}
