import {createBallanceGpu} from './webgpu.mjs';
const $=id=>document.getElementById(id);
// SDL listens on window. Form typing must not become game input, but key-up
// still reaches SDL so a held game key released over a form cannot get stuck.
for(const type of ['keydown','keypress'])window.addEventListener(type,event=>{
    if(event.target!==$('canvas'))event.stopImmediatePropagation();
},true);
let player,gpu,running=false,userReady=false,syncTail=Promise.resolve(),timer;
const status=text=>{$('status').textContent=text};
const storage=text=>{$('storage').textContent=text};
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
function listFiles(){
    $('files').replaceChildren();
    refreshRecords();
    for(const entry of walk('/user')){
        const item=document.createElement('li'),button=document.createElement('button');
        item.append(document.createTextNode(entry.path.slice(6)+' · '+entry.size+' B'));
        button.textContent='下载';button.onclick=()=>{
            player._BallanceWebFlushFiles();
            const url=URL.createObjectURL(new Blob([player.FS.readFile(entry.path)]));
            const a=document.createElement('a');a.href=url;a.download=entry.path.split('/').pop();a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
        };
        item.append(button);$('files').append(item);
    }
}
async function persist(){
    try{await sync();storage('已保存到此浏览器。');listFiles()}
    catch(error){storage('保存失败：'+error.message);throw error}
}
function fit(){
    if(!running)return;
    const width=Math.max(320,Math.min(1280,Math.floor($('stage').clientWidth)));
    const height=Math.max(240,Math.min(960,Math.floor(innerHeight-190)));
    player._BallanceWebResize(width,height);$('canvas').focus();
}
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
$('start').onclick=async()=>{
    $('start').disabled=true;status('正在加载游戏…');
    try{
        const manifest=await(await fetch('./assets.json')).json();
        gpu=await createBallanceGpu($('canvas'),await(await fetch('./shaders.json')).json());
        const {default:createPlayer}=await import('./PlayerBrowser.mjs');
        player=await createPlayer({canvas:$('canvas'),ballanceGpu:gpu,
            print:line=>console.log(line),printErr:line=>console.error(line),
            onAbort:reason=>status('游戏运行失败：'+reason),
            onBallanceExit:async()=>{
                running=false;clearInterval(timer);$('quit').disabled=$('fit').disabled=true;
                await persist();status('游戏已退出，数据已保存。重新加载页面可以再次启动。');
            }});
        player.FS.mkdirTree('/user');player.FS.mount(player.IDBFS,{},'/user');
        await new Promise((resolve,reject)=>player.FS.syncfs(true,error=>error?reject(error):resolve()));
        userReady=true;player.FS.mkdirTree('/game/Bin');let cursor=0;
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
        const code=player.callMain(['--root-path','/game','--cmo','/game/base.cmo','--user-data-path','/user','--rasterizer','webgpu']);
        if(code)throw new Error('Player 初始化失败 '+code);
        running=true;gpu.setSamples(Number($('samples').value));
        for(const id of ['quit','fit','maps','tapes','save','refresh','capture','restore'])$(id).disabled=false;
        timer=setInterval(()=>{void persist().catch(()=>{})},10000);
        await persist();status('方向键移动，Enter 确认，Esc 暂停。');$('canvas').focus();
        // Keep introspection available for local acceptance tests and diagnostics.
        window.ballance={player,gpu,get running(){return running},persist};
    }catch(error){status('无法启动：'+error.message);console.error(error)}
};
$('quit').onclick=()=>{if(running){$('quit').disabled=true;player._BallanceWebRequestQuit()}};
$('fit').onclick=fit;$('samples').onchange=()=>gpu?.setSamples(Number($('samples').value));
$('save').onclick=()=>{void persist().catch(()=>{})};$('refresh').onclick=listFiles;
$('maps').onchange=()=>importFiles($('maps'),'/user/ModLoader/Maps','.nmo');
$('tapes').onchange=()=>importFiles($('tapes'),'/user/ModLoader/TAS','.tas');
document.addEventListener('visibilitychange',()=>{if(document.hidden&&userReady)void persist().catch(()=>{})});

// An active worker means every versioned game/runtime asset was cached and
// checked successfully. A failed install never replaces the previous version.
if('serviceWorker' in navigator){
    navigator.serviceWorker.addEventListener('message',event=>{
        if(event.data?.cacheError)$('offline').textContent='离线缓存失败：'+event.data.cacheError;
        else if(event.data?.cacheProgress)$('offline').textContent='正在缓存离线资源：'+event.data.cacheProgress+'/'+event.data.total;
    });
    navigator.serviceWorker.register('./sw.js').then(async registration=>{
        const ready=await navigator.serviceWorker.ready;
        $('offline').textContent='离线缓存已就绪。下次可在没有网络时打开此页面。';
        if(registration.waiting)$('offline').textContent+=' 新版本将在关闭本网站所有页面后启用。';
        const persistent=await navigator.storage?.persist?.();
        if(!persistent)$('offline').textContent+=' 浏览器仍可能在空间不足时清理网站数据，请备份存档。';
    }).catch(error=>{$('offline').textContent='离线缓存失败：'+error.message});
}else $('offline').textContent='当前环境不支持离线缓存，请通过 localhost 或 HTTPS 打开。';

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
    $('load-record').disabled=$('play-record').disabled=!$('records').value;
}
const call=(name,types=[],args=[],result='number')=>player.ccall('BML_WebTAS'+name,result,types,args);
const error=()=>call('Error',[],[],'string');
function inspect(){
    const first=Number($('first').value),keys=call('Keys',['number'],[first]);
    if(keys<0)throw new Error('采样编号超出范围');
    for(const input of $('keys').querySelectorAll('input'))input.checked=!!(keys&(1<<Number(input.dataset.bit)));
    const delta=call('Delta',['number'],[first]);
    $('editor-status').textContent='采样 '+first+'：'+delta+' ms';
    const rows=[];for(let i=first;i<first+12;++i){const k=call('Keys',['number'],[i]);if(k<0)break;rows.push(i+'  '+call('Delta',['number'],[i])+' ms  '+keyNames.filter((_,b)=>k&(1<<b)).join(' '))}
    $('preview').textContent=rows.join('\n');
}
function editorAction(id,fn){$(id).onclick=async()=>{try{await fn()}catch(e){$('editor-status').textContent=e.message;console.error(e)}}}
editorAction('load-record',()=>{
    const count=call('Load',['string'],[$('records').value]);if(count<0)throw new Error(error());
    $('record-info').textContent=count+' 次采样';$('first').max=$('last').max=count-1;$('first').value=$('last').value=0;
    for(const id of ['inspect','apply','undo','save-record'])$(id).disabled=false;
    $('copy-name').value=$('records').value.split('/').pop().replace(/\.tas$/, '-edited.tas');inspect();
});
editorAction('inspect',inspect);
editorAction('apply',()=>{
    const keys=[...$('keys').querySelectorAll('input')].reduce((mask,i)=>mask|(i.checked?1<<Number(i.dataset.bit):0),0);
    if(!call('SetRange',['number','number','number','number'],[Number($('first').value),Number($('last').value),keys,Number($('delta').value)]))throw new Error(error());
    inspect();$('editor-status').textContent='修改已应用到内存副本，原文件未覆盖。';
});
editorAction('undo',()=>{if(!call('Undo'))throw new Error('没有可撤销的修改');inspect()});
editorAction('save-record',async()=>{
    const name=$('copy-name').value;if(!name.endsWith('.tas')||/[\\/\0]/.test(name))throw new Error('请输入 .tas 副本文件名');
    const path='/user/ModLoader/TAS/'+name;
    if(!call('SaveCopy',['string'],[path]))throw new Error(error());
    await persist();$('records').value=path;$('editor-status').textContent='副本已保存，可下载或回放。';
});
editorAction('play-record',()=>{
    if(!call('Play',['string','number'],[$('records').value,Number($('level').value)]))throw new Error(error());
    $('editor-status').textContent='已启动严格模式回放。F3 停止；Esc 是录像中的游戏暂停键。';$('canvas').focus();
});

async function checkpoint(action){
    if(!running||!player._BML_WebRequestCheckpoint(action)){status('请先进入关卡，且不要同时执行多个保存恢复操作。');return;}
    status(action===1?'正在保存关卡状态…':'正在恢复关卡状态…');
    const result=await new Promise(resolve=>{
        const poll=()=>{const result=player._BML_WebCheckpointResult();if(result||!running)resolve(result);else requestAnimationFrame(poll)};poll();
    });
    status(result>0?(action===1?'关卡状态已保存在内存中。':'关卡状态和 TAS 回放位置已恢复。'):error());$('canvas').focus();
}
$('capture').onclick=()=>checkpoint(1);$('restore').onclick=()=>checkpoint(2);
window.addEventListener('keydown',event=>{
    if(event.target===$('canvas')&&!event.repeat&&(event.code==='F5'||event.code==='F8')){
        event.preventDefault();event.stopImmediatePropagation();void checkpoint(event.code==='F5'?1:2);
    }
},true);
