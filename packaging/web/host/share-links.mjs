// Hash links stay client-side; no remote asset is fetched before confirmation.
export function shareAsset(value,extension){
    let url;
    try{url=new URL(value.trim())}catch{throw Error('分享链接中的文件地址无效。')}
    if(!['http:','https:'].includes(url.protocol)||url.username||url.password)throw Error('文件地址必须是 HTTP 或 HTTPS 链接。');
    if(url.hostname==='github.com'){
        const match=url.pathname.match(/^\/([^/]+)\/([^/]+)\/raw\/(.+)$/);
        if(match){url.hostname='raw.githubusercontent.com';url.pathname='/'+match[1]+'/'+match[2]+'/'+match[3]}
    }
    let name;try{name=decodeURIComponent(url.pathname.split('/').pop())}catch{throw Error('文件名编码无效。')}
    if(!name||/[\\/\x00-\x1f\x7f]/.test(name)||!name.toLowerCase().endsWith(extension))throw Error('分享文件必须是 '+extension+' 文件。');
    url.hash='';return {url:url.href,name};
}
export function parseShareLink(hash){
    if(!hash.startsWith('#!'))return null;
    const query=new URLSearchParams(hash.slice(2));
    for(const key of ['action','map','record','level'])if(query.getAll(key).length>1)throw Error('分享参数重复：'+key);
    const action=query.get('action');if(!['map','tas'].includes(action))throw Error('不支持的分享操作。');
    const map=query.has('map')?shareAsset(query.get('map'),'.nmo'):null;
    const record=query.has('record')?shareAsset(query.get('record'),'.tas'):null;
    const level=query.has('level')?query.get('level'):null;
    if(action==='map'&&(!map||record||level!==null))throw Error('地图分享需要 map 地址。');
    if(action==='tas'&&(!record||(!map&&level===null)||(map&&level!==null)))throw Error('TAS 分享需要 record，以及 level 或 map 其中之一。');
    if(level!==null&&(!/^(?:[1-9]|1[0-3])$/.test(level)))throw Error('原版关卡编号应为 1–13。');
    const prompt=action==='map'?`是否下载并进入地图 ${map.name}？`:
        map?`是否下载地图 ${map.name} 并播放TAS录像 ${record.name}？`:
        `是否播放TAS录像 ${record.name} (Level ${level})？`;
    return {action,map,record,level:level===null?0:Number(level),prompt};
}
function sameBytes(a,b){if(a.length!==b.length)return false;for(let i=0;i<a.length;++i)if(a[i]!==b[i])return false;return true}
export function installSharedFile(FS,directory,name,bytes){
    if(!name||/[\\/\x00-\x1f\x7f]/.test(name)||name==='.'||name==='..')throw Error('文件名无效');
    FS.mkdirTree(directory);
    const dot=name.lastIndexOf('.'),stem=name.slice(0,dot),extension=name.slice(dot);
    const entries=new Set(FS.readdir(directory));
    const candidates=[name,...[...entries].filter(n=>{
        if(!n.startsWith(stem)||!n.endsWith(extension))return false;
        return /^[2-9]$|^[1-9][0-9]+$/.test(n.slice(stem.length,-extension.length));
    }).sort((a,b)=>Number(a.slice(stem.length,-extension.length))-Number(b.slice(stem.length,-extension.length)))];
    for(const candidate of candidates){
        const path=directory+'/'+candidate;
        if(entries.has(candidate)&&FS.isFile(FS.stat(path).mode)&&sameBytes(FS.readFile(path),bytes))return {path,reused:true};
    }
    let target=name;for(let n=2;entries.has(target);++n)target=stem+n+extension;
    const path=directory+'/'+target;FS.writeFile(path,bytes);return {path,reused:false};
}
async function download(asset,signal,status){
    status('正在下载 '+asset.name+'…');
    let response;
    try{response=await fetch(asset.url,{signal,credentials:'omit',referrerPolicy:'no-referrer'})}
    catch(e){if(signal.aborted)throw e;throw Error('无法下载 '+asset.name+'。请检查网络和链接是否允许跨域访问。')}
    if(!response.ok)throw Error('下载 '+asset.name+' 失败：HTTP '+response.status);
    const limit=128*1024*1024,chunks=[];let size=0;
    const reader=response.body.getReader();
    try{for(;;){const {value,done}=await reader.read();if(done)break;size+=value.length;if(size>limit)throw Error('文件超过 128 MiB 下载上限。');chunks.push(value)}}
    catch(e){await reader.cancel().catch(()=>{});throw e}
    if(!size)throw Error(asset.name+' 是空文件。');
    const bytes=new Uint8Array(size);let offset=0;for(const chunk of chunks){bytes.set(chunk,offset);offset+=chunk.length}
    const prefix=new TextDecoder().decode(bytes.subarray(0,80)).trimStart().toLowerCase();
    if(prefix.startsWith('<!doctype html')||prefix.startsWith('<html'))throw Error(asset.name+' 的链接返回了网页，请使用原始文件下载地址。');
    if(asset.name.toLowerCase().endsWith('.nmo')&&(bytes.length<32||new TextDecoder().decode(bytes.subarray(0,7))!=='Nemo Fi'))throw Error('地图文件头无效。');
    if(asset.name.toLowerCase().endsWith('.tas')){
        const unpacked=bytes.length>=5?new DataView(bytes.buffer).getUint32(0,true):0;
        if(!unpacked||unpacked%8||unpacked>64*1024*1024)throw Error('TAS 文件头无效或解压后超过 64 MiB。');
    }
    return bytes;
}
const wait=(signal,ms)=>new Promise((resolve,reject)=>{
    const onAbort=()=>{clearTimeout(timer);reject(signal.reason)};
    const timer=setTimeout(()=>{signal.removeEventListener('abort',onAbort);resolve()},ms);
    signal.addEventListener('abort',onAbort,{once:true});if(signal.aborted)onAbort();
});
export function startShareLinks({ready,getPlayer,persist,status,focus}){
    const dialog=document.getElementById('share-dialog'),message=document.getElementById('share-message');
    const key='ballance-share-request-v1';let active;
    const check=signal=>signal.throwIfAborted();
    function remember(ticket){try{sessionStorage.setItem(key,JSON.stringify(ticket))}catch{}}
    function forget(){try{sessionStorage.removeItem(key)}catch{}}
    function ticketFor(hash){try{const t=JSON.parse(sessionStorage.getItem(key));return t?.hash===hash&&Date.now()-t.time<600000?t:null}catch{return null}}
    async function confirm(request,signal){
        message.textContent=request.prompt;dialog.returnValue='';dialog.showModal();
        return new Promise(resolve=>{
            const form=dialog.querySelector('form');
            const finish=yes=>{signal.removeEventListener('abort',cancel);dialog.removeEventListener('cancel',cancel);form.removeEventListener('submit',submit);dialog.close(yes?'yes':'cancel');resolve(yes)};
            const cancel=event=>{event?.preventDefault?.();finish(false)};
            const submit=event=>{event.preventDefault();finish(event.submitter?.value==='yes')};
            form.addEventListener('submit',submit);dialog.addEventListener('cancel',cancel);signal.addEventListener('abort',cancel,{once:true});
        });
    }
    async function run(){
        active?.abort();if(dialog.open)dialog.close('cancel');
        const controller=new AbortController();active=controller;const {signal}=controller,hash=location.hash;
        let launched=false;
        try{
            const request=parseShareLink(hash);if(!request){forget();return}
            let ticket=ticketFor(hash);
            if(!ticket){forget();if(!await confirm(request,signal)){check(signal);status('已取消分享链接。');return}check(signal);ticket={hash,time:Date.now()};remember(ticket)}
            await ready;check(signal);const player=getPlayer();if(!player)throw Error('游戏尚未启动成功。');
            if(!ticket.files){
                const assets=[request.map,request.record].filter(Boolean);
                const bytes=await Promise.all(assets.map(a=>download(a,signal,status)));check(signal);
                const files={};assets.forEach((asset,i)=>{const type=asset===request.map?'map':'record';files[type]=installSharedFile(player.FS,'/user/ModLoader/'+(type==='map'?'Maps':'TAS'),asset.name,bytes[i]).path});
                await persist();check(signal);ticket.files=files;remember(ticket);
            }
            // A link opened in an existing level starts in a fresh engine session,
            // after persistence, without a second download/confirmation. No game
            // flow message is injected while its current level is being torn down.
            status('正在准备'+(request.action==='tas'?' TAS 回放':'地图')+'…');
            const deadline=Date.now()+60000;let firstProbe=true;
            for(;;){
                check(signal);const state=player._BML_WebTASSharePrepare(firstProbe?1:0);firstProbe=false;
                if(state===1)break;
                if(state===-1){
                    if(ticket.reloaded)throw Error('无法进入关卡选择菜单，请返回主菜单后重试。');
                    ticket.reloaded=true;remember(ticket);await persist();check(signal);location.reload();return;
                }
                if(Date.now()>deadline)throw Error('等待游戏菜单超时。');await wait(signal,50);
            }
            check(signal);
            const {map='',record=''}=ticket.files;
            if((map&&!map.startsWith('/user/ModLoader/Maps/'))||(record&&!record.startsWith('/user/ModLoader/TAS/')))throw Error('分享文件路径无效。');
            if(!player.ccall('BML_WebTASShareLaunch','number',['string','string','number'],[record,map,request.level]))throw Error(player.ccall('BML_WebTASError','string',[],[]));
            launched=true;forget();
            const loadDeadline=Date.now()+65000;
            for(;;){
                check(signal);
                if(map){const state=player._BML_WebMapStatus();if(state<0)throw Error(player.ccall('BML_WebMapError','string',[],[]));if(state===2)break}
                else{const state=JSON.parse(player.ccall('BML_WebTASStatus','string',[],[]));if(state.state==='playing')break;if(state.state==='idle')throw Error('TAS 回放未能启动。')}
                if(Date.now()>loadDeadline)throw Error('关卡加载超时。');await wait(signal,50);
            }
            status(request.action==='map'?'已进入地图 '+map.split('/').pop():'已开始播放 '+record.split('/').pop());focus();
        }catch(error){
            if(signal.aborted)return;
            if(launched)try{getPlayer()?._BML_WebTASStop()}catch{}
            forget();controller.abort();status('分享链接：'+(error?.message||error));
        }
    }
    window.addEventListener('hashchange',()=>void run());void run();
}
