import {prepareMods,unpackMod,validateModSet,modStore} from './mods.mjs';
import {startShareLinks} from './share-links.mjs';
import {createTASGrid} from './tas-grid.mjs';
import {createBallanceRenderer} from './renderer.mjs';
const $=id=>document.getElementById(id);
// Automatic startup may precede permission to play audio. Resume the existing
// SDL context on a real gesture without delaying the game behind a start button.
function resumeAudio(){
    const audio=player?.SDL3?.audioContext;
    if(audio?.state==='suspended')void audio.resume().catch(console.error);
}
for(const type of ['pointerdown','keydown'])window.addEventListener(type,resumeAudio,true);
// SDL listens on window. Form typing must not become game input, but key-up
// still reaches SDL so a held game key released over a form cannot get stuck.
for(const type of ['keydown','keypress'])document.addEventListener(type,event=>{
    // Navigation keys belong to the game even during OS key repeat. SDL may
    // decline a repeated edge; that must not re-enable browser scroll/defaults.
    // Keep delivering every event to SDL and never clear the held physical key.
    // Cancelling Space keydown suppresses the keypress SDL uses for text.
    // Cancel its keypress default instead, preserving console spaces without scrolling.
    if(event.target===$('canvas')&&(['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Tab'].includes(event.code)||(event.code==='Space'&&type==='keypress')))event.preventDefault();
    if(event.target!==$('canvas')){
        if(type==='keydown'&&event.target.getAttribute?.('role')==='tab'&&['ArrowLeft','ArrowRight','Home','End'].includes(event.key)){
            const index=tabs.indexOf(event.target.id.slice(4));
            const next=event.key==='Home'?0:event.key==='End'?2:(index+(event.key==='ArrowRight'?1:2))%3;
            event.preventDefault();selectTab(tabs[next],true);
        }
        if(type==='keydown'&&event.key==='Escape'&&!$('sidebar').hidden&&$('sidebar').contains(event.target)){
            event.preventDefault();setSidebar(false);
        }
        event.stopImmediatePropagation();
    }
});
let modCatalog;
let player,gpu,running=false,userReady=false,syncTail=Promise.resolve(),timer,restartNeedsSave=false,runtimeFailure=null;
const status=text=>{$('status').textContent=text};
const storage=text=>{$('storage').textContent=text};
const rendererKey='ballance.renderer',rendererChoices=['webgl2','webgpu','auto'];
function rendererPreference(){
    const override=new URLSearchParams(location.search).get('renderer');
    if(rendererChoices.includes(override))return override;
    try{const saved=localStorage.getItem(rendererKey);if(rendererChoices.includes(saved))return saved}catch{}
    return 'webgl2';
}
const selectedRenderer=rendererPreference();
$('renderer').value=selectedRenderer;
$('renderer').onchange=()=>{$('apply-renderer').disabled=$('renderer').value===selectedRenderer};
$('apply-renderer').onclick=async()=>{
    const backend=$('renderer').value;
    if(!rendererChoices.includes(backend))return;
    $('apply-renderer').disabled=true;$('renderer').disabled=true;
    try{
        if(userReady)await persist();
        localStorage.setItem(rendererKey,backend);
        // A fresh document releases SDL/WASM and the old canvas context. Remove
        // diagnostic overrides so the explicitly selected preference wins.
        const url=new URL(location.href);url.searchParams.delete('renderer');
        location.replace(url.href);
    }catch(error){
        status('未切换渲染器：'+error.message);
        $('renderer').disabled=false;$('apply-renderer').disabled=false;
    }
};

const sync=()=>{
    const operation=async()=>{
        player._BallanceWebFlushFiles();
        await new Promise((resolve,reject)=>player.FS.syncfs(false,error=>error?reject(error):resolve()));
    };
    const result=syncTail.then(operation);
    syncTail=result.catch(()=>{});
    return result;
};
function walk(path){
    const files=[];
    for(const name of player.FS.readdir(path)){
        if(name==='.'||name==='..')continue;
        const child=path+'/'+name,stat=player.FS.stat(child);
        if(player.FS.isDir(stat.mode))files.push(...walk(child));else files.push({path:child,size:stat.size});
    }
    return files;
}
let fileDirectory='/user';
const formatSize=n=>n>=1e6?(n/1e6).toFixed(1)+' MB':n>=1000?(n/1000).toFixed(1)+' KB':n+' B';
function listFiles(){
    $('files').replaceChildren();refreshRecords();
    if(!userReady)return;
    const query=$('file-search').value.trim().toLocaleLowerCase();
    let entries;
    if(query)entries=walk('/user').filter(f=>!f.path.split('/').some(p=>p.startsWith('.'))&&f.path.toLocaleLowerCase().includes(query));
    else entries=player.FS.readdir(fileDirectory).filter(n=>!n.startsWith('.')).map(name=>{
        const path=fileDirectory+'/'+name,stat=player.FS.stat(path);
        return {path,size:stat.size,directory:player.FS.isDir(stat.mode)};
    });
    entries.sort((a,b)=>Number(!!b.directory)-Number(!!a.directory)||a.path.localeCompare(b.path));
    $('file-path').textContent=query?'搜索结果':fileDirectory.slice(5)||'/';
    $('file-up').disabled=fileDirectory==='/user';$('file-home').disabled=false;
    for(const entry of entries){
        const row=document.createElement('tr'),name=document.createElement('td'),size=document.createElement('td'),actions=document.createElement('td');
        const label=query?entry.path.slice(6):entry.path.split('/').pop();
        if(entry.directory){
            const button=document.createElement('button');button.className='folder';button.textContent='▸ '+label;
            button.onclick=()=>{fileDirectory=entry.path;$('file-search').value='';listFiles()};name.append(button);size.textContent='—';
        }else{
            name.textContent=label;
            const type=document.createElement('span');type.className='file-type';type.textContent=label.split('.').pop().toUpperCase();name.append(type);
            size.textContent=formatSize(entry.size);
            const button=document.createElement('button');button.textContent='下载';button.setAttribute('aria-label','下载 '+label);
            button.onclick=()=>{
                player._BallanceWebFlushFiles();
                const url=URL.createObjectURL(new Blob([player.FS.readFile(entry.path)]));
                const a=document.createElement('a');a.href=url;a.download=entry.path.split('/').pop();a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
            };actions.append(button);
        }
        row.append(name,size,actions);$('files').append(row);
    }
    if(!entries.length){const row=document.createElement('tr'),cell=document.createElement('td');cell.colSpan=3;cell.textContent=query?'没有匹配的文件':'此文件夹为空';row.append(cell);$('files').append(row)}
    $('file-count').textContent=entries.length+' 项';
}
$('file-search').oninput=listFiles;
$('file-home').onclick=()=>{fileDirectory='/user';$('file-search').value='';listFiles()};
$('file-up').onclick=()=>{fileDirectory=fileDirectory.slice(0,fileDirectory.lastIndexOf('/'))||'/user';$('file-search').value='';listFiles()};
async function persist(){
    try{await sync();storage('已保存到此浏览器。');listFiles()}
    catch(error){storage('保存失败：'+error.message);throw error}
}
// Container dimensions are independent of the canvas, preventing resize loops.
// ResizeObserver covers header wrapping/layout changes; window/media events also
// catch browser zoom and movement between displays with a different pixel ratio.
let resizeFrame=0,lastSize='';
const screenModeKey='ballance.screenMode';
const resolutionKey='ballance.resolution';
let resolution=[0,0];
const resolutionSizes=[[0,0],[640,480],[800,600],[1024,768],[1280,960],[1600,1200],[1920,1440],[2560,1920]];
try{
    const saved=JSON.parse(localStorage.getItem(resolutionKey));
    if(resolutionSizes.some(size=>JSON.stringify(size)===JSON.stringify(saved)))resolution=saved;
}catch{}
function selectResolution(width,height){
    if(!resolutionSizes.some(size=>size[0]===width&&size[1]===height))return;
    resolution=[width,height];
    if(player)player.browserResolution=resolution;
    try{localStorage.setItem(resolutionKey,JSON.stringify(resolution))}catch{}
    lastSize='';scheduleResize();
}
try{if(localStorage.getItem(screenModeKey)==='fill')$('screen-mode').value='fill'}catch{}
$('screen-mode').onchange=()=>{
    try{localStorage.setItem(screenModeKey,$('screen-mode').value)}catch{}
    scheduleResize();
};
function resizeGame(){
    resizeFrame=0;if(!running)return;
    const stage=$('stage');
    let width=Math.floor(stage.clientWidth),height=Math.floor(stage.clientHeight);
    if(resolution[0]){
        // A fixed window has the requested CSS size where space permits. On a
        // smaller browser/phone, shrink proportionally instead of cropping it.
        const scale=Math.min(1,width/resolution[0],height/resolution[1]);
        const unit=Math.max(1,Math.floor(resolution[0]*scale/4));
        width=unit*4;height=unit*3;
    }else if($('screen-mode').value!=='fill'){
        const unit=Math.floor(Math.min(width/4,height/3));
        width=unit*4;height=unit*3;
    }
    if(width<=0||height<=0)return;
    const mask=$('extended-view-mask');
    mask.hidden=$('screen-mode').value!=='fill';
    mask.style.width=width+'px';mask.style.height=height+'px';
    const coreWidth=Math.min(width,height*4/3);
    mask.firstElementChild.style.width=coreWidth+'px';
    mask.firstElementChild.style.height=coreWidth*3/4+'px';
    const key=width+':'+height+':'+devicePixelRatio;
    const canvas=$('canvas');
    if(key!==lastSize||canvas.clientWidth!==width||canvas.clientHeight!==height){
        // SDL's browser size query reads the CSS rectangle. A level script can
        // reset it to 640x480 even when SDL's cached window size is unchanged.
        canvas.style.width=width+'px';canvas.style.height=height+'px';
        if(player._BallanceWebResize(width,height))lastSize=key;
    }
}
function scheduleResize(){if(!resizeFrame)resizeFrame=requestAnimationFrame(resizeGame)}
const layoutObserver=new ResizeObserver(()=>{
    document.documentElement.style.setProperty('--header-height',document.querySelector('header').offsetHeight+'px');
    scheduleResize();
});
layoutObserver.observe(document.querySelector('header'));layoutObserver.observe($('stage'));
layoutObserver.observe($('canvas')); // Game scripts may reset their authored initial resolution.
window.addEventListener('resize',scheduleResize);
let densityQuery;
function watchDensity(){
    densityQuery?.removeEventListener('change',watchDensity);
    densityQuery=matchMedia('(resolution: '+devicePixelRatio+'dppx)');
    densityQuery.addEventListener('change',watchDensity);
    // A density-only change need not fire window.resize. SDL refreshes its
    // cached drawable scale from that event before our normal resize request.
    if(running)window.dispatchEvent(new Event('resize'));
    scheduleResize();
}
watchDensity();
function setSidebar(open){
    $('sidebar').hidden=!open;document.body.classList.toggle('sidebar-open',open);
    $('toggle-sidebar').setAttribute('aria-expanded',String(open));
    if(open)$('close-sidebar').focus();else if(running)$('canvas').focus();else $('toggle-sidebar').focus();
}
$('toggle-sidebar').onclick=()=>setSidebar($('sidebar').hidden);
$('close-sidebar').onclick=()=>setSidebar(false);
const tabs=['display','data','editor'];
function selectTab(name,focus=false){
    document.body.classList.toggle('editor-open',name==='editor');
    for(const tab of tabs){const active=tab===name,button=$('tab-'+tab);button.setAttribute('aria-selected',String(active));button.tabIndex=active?0:-1;$(tab==='editor'?'editor':'panel-'+tab).hidden=!active}
    if(name==='data'&&userReady)listFiles();
    if(focus)$('tab-'+name).focus();
}
for(const name of tabs)$('tab-'+name).onclick=()=>selectTab(name);

async function importFiles(input,directory,extension){
    try{
        for(const file of input.files){
            if(!file.name.toLowerCase().endsWith(extension)||/[\\/\0]/.test(file.name))throw new Error('文件名或类型不正确');
            const path=directory+'/'+file.name;
            // Imports never silently replace an existing recording or map.
            if(player.FS.analyzePath(path).exists)throw new Error(file.name+' 已存在，请先重命名后导入');
            player.FS.mkdirTree(directory);player.FS.writeFile(path,new Uint8Array(await file.arrayBuffer()));
        }
        await persist();status('导入完成。请从游戏的地图或 TAS 菜单选择文件。');
    }catch(error){storage('导入失败：'+error.message)}finally{input.value=''}
}
function showRestart(message){
    running=false;clearInterval(timer);clearInterval(runtimeTimer);$('loading-screen').hidden=true;
    $('solver-status').textContent='解算器 —';$('tas-status').textContent='TAS 未回放';$('tas-status').disabled=true;$('tas-status').hidden=true;grid.setEditable(false);
    for(const id of ['load-record','inspect','apply','undo','save-record','play-record'])$(id).disabled=true;
    $('stage').classList.add('ended');$('restart-screen').hidden=false;status(message);
}
async function startGame(){
    status('正在准备资源…');
    try{
        await cachePreparation;
        status('正在启动游戏…');$('loading-status').textContent='正在启动游戏…';
        const manifest=await(await fetch('./assets.json')).json();
        gpu=await createBallanceRenderer($('canvas'),{backend:selectedRenderer});
        // Multiple script/host resizes can cancel out within one layout pass,
        // so ResizeObserver alone can miss a reset to the authored 640x480.
        const resizeRenderer=gpu.resize.bind(gpu);
        gpu.resize=(...args)=>{const result=resizeRenderer(...args);scheduleResize();return result;};
        $('solver-status').title='渲染器：'+(gpu.backend==='webgl2'?'WebGL2':'WebGPU');
        $('renderer-status').textContent='当前渲染器：'+(gpu.backend==='webgl2'?'WebGL2':'WebGPU');
        const {default:createPlayer}=await import('./PlayerBrowser.mjs');
        player=await createPlayer({canvas:$('canvas'),ballanceGpu:gpu,
            browserResolution:resolution,onResolutionChange:selectResolution,
            print:line=>console.log(line),printErr:line=>console.error(line),
            onAbort:reason=>{runtimeFailure=String(reason);showRestart('游戏运行失败：'+reason)},
            onBallanceExit:async()=>{
                running=false;clearInterval(timer);
                try{await persist();showRestart('游戏已退出，数据已保存。')}
                catch(error){restartNeedsSave=true;showRestart('游戏已退出，但保存失败：'+error.message+'。重新开始时将重试保存。')}
            }});
        player.FS.mkdirTree('/user');player.FS.mount(player.IDBFS,{},'/user');
        await new Promise((resolve,reject)=>player.FS.syncfs(true,error=>error?reject(error):resolve()));
        userReady=true;
        // One-time migration of the previous browser defaults; preserve other
        // configured keys, and allow F5/F8 again after the migration marker.
        const marker='/user/.web-practice-keys-v2',config='/user/ModLoader/Configs/PracticeTools.cfg';
        if(!player.FS.analyzePath(marker).exists){
            if(player.FS.analyzePath(config).exists){
                let text=player.FS.readFile(config,{encoding:'utf8'});
                if(/K SaveState 63\b/.test(text)&&/K RestoreState 66\b/.test(text)){
                    text=text.replace(/K SaveState 63\b/,'K SaveState 30').replace(/K RestoreState 66\b/,'K RestoreState 44');
                    player.FS.writeFile(config,text);
                }
            }
            player.FS.writeFile(marker,'A/Z defaults migrated');
        }
        // Apply the new browser overlay defaults once, including existing profiles.
        // Subsequent choices made in the plugin menu remain untouched.
        const overlayMarker='/user/.web-overlays-v1';
        if(!player.FS.analyzePath(overlayMarker).exists){
            for(const [name,key] of [['Segment','Enabled?'],['BallanceTAS','ShowOSD']]){
                const path='/user/ModLoader/Configs/'+name+'.cfg';
                if(player.FS.analyzePath(path).exists){
                    const lines=player.FS.readFile(path,{encoding:'utf8'}).split('\n');
                    player.FS.writeFile(path,lines.map(line=>{
                        const parts=line.trim().split(/\s+/);
                        return parts[0]==='B'&&parts[1]===key ? line.replace(/([01])\s*$/,'0') : line;
                    }).join('\n'));
                }
            }
            player.FS.writeFile(overlayMarker,'Browser overlays default off');
        }
        const once='/user/.web-tas-once-v1',tasConfig='/user/ModLoader/Configs/BallanceTAS.cfg';
        if(!player.FS.analyzePath(once).exists){
            if(player.FS.analyzePath(tasConfig).exists)player.FS.writeFile(tasConfig,player.FS.readFile(tasConfig,{encoding:'utf8'}).replace(/B AutoRestart 1\b/,'B AutoRestart 0'));
            player.FS.writeFile(once,'Browser replay stops at completion');
        }
        player.FS.mkdirTree('/game/Bin');let cursor=0;
        await Promise.all(Array.from({length:8},async()=>{
            while(cursor<manifest.assets.length){
                const asset=manifest.assets[cursor++];
                if(asset.default&&player.FS.analyzePath(asset.vfs).exists)continue;
                const response=await fetch(asset.url);if(!response.ok)throw new Error('无法加载 '+asset.vfs);
                const bytes=new Uint8Array(await response.arrayBuffer());
                const digest=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),x=>x.toString(16).padStart(2,'0')).join('');
                if(digest!==asset.sha256)throw new Error('资源校验失败 '+asset.vfs);
                player.FS.mkdirTree(asset.vfs.slice(0,asset.vfs.lastIndexOf('/')));player.FS.writeFile(asset.vfs,bytes);
            }
        }));
        modCatalog=await (await fetch('mods.json')).json();
        $('import-mods').disabled=false;refreshMods();
        const loadedMods=await prepareMods(player,modCatalog,status);
        player.webMods=loadedMods;player.webModCatalog=modCatalog;refreshMods();
        $('mods-status').textContent='已动态加载 '+loadedMods.length+' 个插件。';
        const code=player.callMain(['--root-path','/game','--cmo','/game/base.cmo','--user-data-path','/user','--rasterizer','webgpu']);
        if(runtimeFailure)throw new Error(runtimeFailure);
        if(code)throw new Error('Player 初始化失败 '+code);
        running=true;resizeGame();gpu.setSamples(Number($('samples').value));
        for(const id of ['maps','tapes','save','refresh'])$(id).disabled=false;
        timer=setInterval(()=>{void persist().catch(()=>{})},10000);
        await persist();if(runtimeFailure)return;status('');$('loading-screen').hidden=true;$('canvas').focus();
        // Keep introspection available for local acceptance tests and diagnostics.
        window.ballance={player,gpu,get running(){return running},persist};
        updateRuntimeStatus();runtimeTimer=setInterval(updateRuntimeStatus,250);
    }catch(error){showRestart('无法启动：'+error.message);console.error(error)}
}
$('restart').onclick=async()=>{
    $('restart').disabled=true;
    try{
        if(restartNeedsSave)await persist();
        // A fresh document also releases the old Wasm heap, GPU and SDL handlers.
        // The service worker serves the runtime offline; IDBFS retains user data.
        location.reload();
    }catch(error){status('保存失败，请重试：'+error.message);$('restart').disabled=false}
};
$('samples').onchange=()=>gpu?.setSamples(Number($('samples').value));
$('save').onclick=()=>{void persist().catch(()=>{})};$('refresh').onclick=listFiles;
function refreshMods(){
    $('mod-list').replaceChildren();if(!modCatalog||!player)return;
    player.FS.mkdirTree(modStore);
    for(const item of modCatalog.mods){const li=document.createElement('li');li.textContent=item.id+' · 默认';$('mod-list').append(li)}
    for(const file of player.FS.readdir(modStore).sort())if(file.endsWith('.bwmod')){
        const li=document.createElement('li');li.textContent=file.slice(0,-6)+(modCatalog.mods.some(x=>x.id===file.slice(0,-6))?' · 已由默认版本取代 ':' · 已安装 ');
        const button=document.createElement('button');button.textContent='移除';button.onclick=async()=>{
            button.disabled=true;try{player.FS.unlink(modStore+'/'+file);await persist();refreshMods();$('restart-mods').disabled=false;$('mods-status').textContent='已移除，重启游戏后生效。'}catch(e){$('mods-status').textContent=e.message;button.disabled=false}
        };li.append(button);$('mod-list').append(li);
    }
}
$('import-mods').onchange=async()=>{
    const input=$('import-mods');input.disabled=true;
    try{
        const proposed=new Map();
        for(const file of input.files){
            if(file.size>128*1024*1024)throw Error('插件包超过128MB');
            const bytes=new Uint8Array(await file.arrayBuffer()),mod=await unpackMod(bytes,modCatalog.sdk);
            if(modCatalog.mods.some(x=>x.id===mod.manifest.id))throw Error('默认插件随游戏更新，不能在这里替换：'+mod.manifest.id);
            if(proposed.has(mod.manifest.id))throw Error('重复插件：'+mod.manifest.id);
            proposed.set(mod.manifest.id,{mod,bytes});
        }
        const combined=(player.webDefaultMods||[]).map(manifest=>({manifest}));
        for(const file of player.FS.readdir(modStore))if(file.endsWith('.bwmod')&&!proposed.has(file.slice(0,-6))){
            const mod=await unpackMod(player.FS.readFile(modStore+'/'+file),modCatalog.sdk);
            if(!modCatalog.mods.some(x=>x.id===mod.manifest.id))combined.push(mod);
        }
        combined.push(...[...proposed.values()].map(x=>x.mod));validateModSet(combined);
        for(const [id,{bytes}] of proposed){const path=modStore+'/'+id+'.bwmod';player.FS.writeFile(path+'.tmp',bytes);player.FS.rename(path+'.tmp',path)}
        await persist();refreshMods();$('restart-mods').disabled=false;$('mods-status').textContent='插件已保存，重启游戏后加载。';
    }catch(e){$('mods-status').textContent='安装失败：'+e.message}
    finally{input.disabled=false;input.value=''}
};
$('restart-mods').onclick=async()=>{
    $('restart-mods').disabled=true;try{await persist();location.reload()}catch(e){$('mods-status').textContent=e.message;$('restart-mods').disabled=false}
};
$('maps').onchange=()=>importFiles($('maps'),'/user/ModLoader/Maps','.nmo');
$('tapes').onchange=()=>importFiles($('tapes'),'/user/ModLoader/TAS','.tas');
document.addEventListener('visibilitychange',()=>{if(document.hidden&&userReady)void persist().catch(()=>{})});

const documentVersion=document.querySelector('meta[name="ballance-build"]').content;
$('update-game').onclick=async()=>{
    $('update-game').disabled=true;
    try{if(userReady)await persist();location.reload()}
    catch(error){status('保存失败，尚未更新：'+error.message);$('update-game').disabled=false}
};
// An active worker means every versioned game/runtime asset was cached and
// checked successfully. A failed install never replaces the previous version.
const cachePreparation=(async()=>{
if('serviceWorker' in navigator){
    let cacheReady=false;
    const progress=$('cache-progress');
    navigator.serviceWorker.addEventListener('message',event=>{
        const data=event.data;
        if(data?.cacheActivated&&data.cacheActivated!==documentVersion){
            $('update-game').hidden=false;$('offline').textContent='新版本已就绪';
        }
        if(data?.cacheError){progress.hidden=true;$('offline').textContent='离线缓存失败：'+data.cacheError}
        else if(data?.cacheProgress!==undefined&&!cacheReady){
            progress.hidden=false;
            if(data.totalBytes!==null&&data.totalBytes>0){
                progress.max=data.totalBytes;progress.value=data.cacheBytes;
                $('loading-status').textContent=(data.cacheByteKind==='compressed-packs'?'已下载压缩资源：':'离线资源：')+(data.cacheBytes/1e6).toFixed(1)+' / '+(data.totalBytes/1e6).toFixed(1)+' MB'+(data.cacheBytes===data.totalBytes?' · 正在完成校验…':'');
            }else{
                progress.max=data.total;progress.value=data.cacheProgress;
                $('loading-status').textContent='离线资源：'+data.cacheProgress+' / '+data.total+' 个文件';
            }
        }
    });
    return navigator.serviceWorker.register('./sw.js',{updateViaCache:'none'}).then(async registration=>{
        // Check every document load rather than waiting for the browser's
        // periodic SW check. Offline/update failures leave the active cache usable.
        try{await registration.update()}catch{}
        const installing=registration.installing||registration.waiting;
        if(installing&&installing.state!=='activated'&&installing.state!=='redundant'){
            await new Promise(resolve=>{
                const changed=()=>{if(installing.state==='activated'||installing.state==='redundant'){installing.removeEventListener('statechange',changed);resolve()}};
                installing.addEventListener('statechange',changed);changed();
            });
        }
        if(!registration.active)throw new Error('离线资源未安装完成，尝试直接加载。');
        const ready=await navigator.serviceWorker.ready;
        const activeVersion=await new Promise(resolve=>{
            const channel=new MessageChannel(),timeout=setTimeout(()=>{channel.port1.close();resolve(null)},3000);
            channel.port1.onmessage=event=>{clearTimeout(timeout);channel.port1.close();resolve(event.data.version)};
            ready.active?.postMessage({type:'VERSION'},[channel.port2]);
        });
        if(activeVersion&&activeVersion!==documentVersion){
            location.reload();await new Promise(()=>{});
        }
        cacheReady=true;
        progress.hidden=true;$('offline').textContent='离线缓存已就绪';
        $('offline').title='下次可在没有网络时打开此页面。';
        const persistent=await navigator.storage?.persist?.();
        if(!persistent)$('offline').title+=' 浏览器仍可能在空间不足时清理网站数据，请备份存档。';
    }).catch(error=>{progress.hidden=true;$('offline').textContent='离线缓存失败：'+error.message});
}else {$('cache-progress').hidden=true;$('offline').textContent='当前环境不支持离线缓存，请通过 localhost 或 HTTPS 打开。'}
})();

const keyNames=['↑','↓','←','→','Shift','Space','Q','Esc','Enter（保留位）'];
for(const [i,name] of keyNames.entries()){
    const label=document.createElement('label'),input=document.createElement('input');input.type='checkbox';input.dataset.bit=i;label.append(input,document.createTextNode(name+' '));$('keys').append(label);
}
for(let level=1;level<=13;++level){const option=document.createElement('option');option.value=level;option.textContent=String(level);$('level').append(option)}
function refreshRecords(){
    const previous=$('records').value;$('records').replaceChildren();
    if(!player)return;
    for(const file of walk('/user').filter(x=>x.path.startsWith('/user/ModLoader/TAS/')&&x.path.endsWith('.tas')&&!x.path.slice(20).includes('/'))){
        const option=document.createElement('option');option.value=file.path;option.textContent=file.path.split('/').pop();$('records').append(option);
    }
    if([...$('records').options].some(o=>o.value===previous))$('records').value=previous;
    $('load-record').disabled=$('play-record').disabled=!running||lastRuntime?.editable===false||!$('records').value;
}
const call=(name,types=[],args=[],result='number')=>player.ccall('BML_WebTAS'+name,result,types,args);
const error=()=>call('Error',[],[],'string');
let runtimeTimer=null,lastRuntime=null;
const grid=createTASGrid($('record-grid'),{call,
    onSelection(first,last){$('first').value=first;$('last').value=last;$('grid-selection').textContent=first===last?'采样 '+first:first+'–'+last+' · '+(last-first+1)+' 次'},
    onError(e){$('editor-status').textContent=e.message},
    onChange(){$('editor-status').textContent='内存副本已修改；保存副本后可回放。'}
});
for(const mode of ['select','paint','fill'])$('mode-'+mode).onclick=()=>{
    grid.setMode(mode);for(const m of ['select','paint','fill'])$('mode-'+m).setAttribute('aria-pressed',String(mode===m));
    $('grid-help').textContent=mode==='select'?'选择：拖动行号或 Shift 点击，选择采样范围。':mode==='paint'?'画笔：点击或拖动按键格；一次拖动可以整体撤销。':'填充：先选采样范围，再点击按键格或列标题，统一按下或松开。';
};
function updateRuntimeStatus(){
    if(!running||document.hidden)return;
    try{
        const state=JSON.parse(call('Status',[],[],'string'));lastRuntime=state;
        $('solver-status').textContent=state.solver===1?'严格 x87':state.solver===0?'快速模式':'解算器 —';$('solver-status').dataset.mode=state.solver;
        const labels={idle:'TAS 未回放',pending:'TAS 准备中',playing:'TAS 回放',paused:'TAS 已暂停',recording:'TAS 录制中',translating:'TAS 转换中'};
        const text=(labels[state.state]||'TAS')+(state.canStop&&state.total?' '+state.frame+'/'+state.total:'')+(state.canStop?' · 停止':'');
        if($('tas-status').textContent!==text)$('tas-status').textContent=text;
        $('tas-status').disabled=!state.canStop;$('tas-status').hidden=state.state==='idle';grid.setEditable(state.editable);
        for(const id of ['load-record','play-record'])$(id).disabled=!state.editable||!$('records').value;
        for(const id of ['apply','undo','save-record'])$(id).disabled=!state.editable||!grid.count;
    }catch(e){console.error(e)}
}
$('tas-status').onclick=()=>{try{if(!call('Stop'))throw new Error(error());updateRuntimeStatus();$('canvas').focus()}catch(e){status(e.message)}};
function inspect(){
    const first=Number($('first').value),keys=call('Keys',['number'],[first]);
    if(keys<0)throw new Error('采样编号超出范围');
    for(const input of $('keys').querySelectorAll('input'))input.checked=!!(keys&(1<<Number(input.dataset.bit)));
    const delta=call('Delta',['number'],[first]);
    $('editor-status').textContent='采样 '+first+'：'+delta+' ms';
    grid.select(first,Number($('last').value));
    grid.refresh();
}
function editorAction(id,fn){$(id).onclick=async()=>{try{await fn()}catch(e){$('editor-status').textContent=e.message;console.error(e)}}}
editorAction('load-record',()=>{
    const count=call('Load',['string'],[$('records').value]);if(count<0)throw new Error(error());
    $('record-info').textContent=count+' 次采样';$('first').max=$('last').max=count-1;$('first').value=$('last').value=0;
    for(const id of ['inspect','apply','undo','save-record'])$(id).disabled=false;
    grid.load(count);
    $('copy-name').value=$('records').value.split('/').pop().replace(/\.tas$/, '-edited.tas');inspect();
});
editorAction('inspect',()=>{inspect();grid.select(Number($('first').value),Number($('last').value),true)});
editorAction('apply',()=>{
    const keys=[...$('keys').querySelectorAll('input')].reduce((mask,i)=>mask|(i.checked?1<<Number(i.dataset.bit):0),0);
    if(!call('SetRange',['number','number','number','number'],[Number($('first').value),Number($('last').value),keys,Number($('delta').value)]))throw new Error(error());
    grid.refresh();inspect();$('editor-status').textContent='修改已应用到内存副本，原文件未覆盖。';
});
editorAction('undo',()=>{if(!call('Undo'))throw new Error('没有可撤销的修改');grid.refresh();inspect()});
editorAction('save-record',async()=>{
    const name=$('copy-name').value;if(!name.endsWith('.tas')||/[\\/\0]/.test(name))throw new Error('请输入 .tas 副本文件名');
    const path='/user/ModLoader/TAS/'+name;
    if(!call('SaveCopy',['string'],[path]))throw new Error(error());
    await persist();$('records').value=path;$('editor-status').textContent='副本已保存，可下载或回放。';
});
editorAction('play-record',()=>{
    if(!call('Play',['string','number'],[$('records').value,Number($('level').value)]))throw new Error(error());
    $('editor-status').textContent='已启动严格模式回放。点击标题栏回放状态可停止；播完自动退出回放。';$('canvas').focus();updateRuntimeStatus();
});

const gameReady=startGame();
startShareLinks({ready:gameReady,getPlayer:()=>running?player:null,persist,status,focus:()=>{setSidebar(false);$('canvas').focus();updateRuntimeStatus()}});
