const VERSION=__VERSION__,FILES=__FILES__,PACKS=__PACKS__;
const CACHE='ballanced-web-'+VERSION;
// Persist per-document versions so a newly activated worker (including after
// browser worker eviction) never gives an old running game a new Wasm/JS file.
const ROUTING='ballanced-client-routing';
const routeURL=id=>new URL('__ballance_client__/'+encodeURIComponent(id),self.registration.scope);
const routing=()=>caches.open(ROUTING);
const readRoute=async id=>(await (await routing()).match(routeURL(id)))?.text();
const writeRoute=async(id,name)=>{if(id)await (await routing()).put(routeURL(id),new Response(name))};
const notify=async message=>{for(const client of await self.clients.matchAll({includeUncontrolled:true}))client.postMessage(message)};
const digest=async bytes=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),x=>x.toString(16).padStart(2,'0')).join('');
const mime=url=>url.endsWith('.wasm')?'application/wasm':/\.(mjs|js)$/.test(url)?'text/javascript':url.endsWith('.html')?'text/html':url.endsWith('.css')?'text/css':url.endsWith('.json')?'application/json':'application/octet-stream';
self.addEventListener('install',event=>event.waitUntil((async()=>{
    const cache=await caches.open(CACHE),controller=new AbortController();
    const byURL=new Map(FILES.map(file=>[file.url,file]));
    const packed=new Set(PACKS.flatMap(pack=>pack.files));
    const totalBytes=PACKS.reduce((n,pack)=>n+pack.size,0);
    let next=0,completed=0,received=0,lastNotice=0,failure;
    const progress=async(force=false)=>{
        const now=Date.now();if(!force&&now-lastNotice<80)return;lastNotice=now;
        await notify({cacheProgress:completed,total:FILES.length,cacheBytes:received,totalBytes,cacheVersion:VERSION,cacheByteKind:'compressed-packs'});
    };
    async function put(file,bytes){
        if(bytes.byteLength!==file.size||await digest(bytes)!==file.sha256)throw Error('Checksum mismatch: '+file.url);
        controller.signal.throwIfAborted();
        await cache.put(new URL(file.url,self.registration.scope),new Response(bytes,{headers:{'Content-Type':mime(file.url),'Content-Length':String(file.size)}}));
        completed++;await progress();
    }
    async function download(pack){
        const response=await fetch(new URL(pack.url,self.registration.scope),{cache:'reload',signal:controller.signal});
        if(!response.ok)throw Error('HTTP '+response.status+' '+pack.url);
        const reader=response.body.getReader(),chunks=[];let size=0;
        for(;;){
            const {done,value}=await reader.read();if(done)break;
            size+=value.byteLength;if(size>pack.size){await reader.cancel();throw Error('Oversized resource pack: '+pack.url)}
            chunks.push(value);received+=value.byteLength;await progress();
        }
        const compressed=new Uint8Array(size);let offset=0;
        for(const chunk of chunks){compressed.set(chunk,offset);offset+=chunk.byteLength}
        if(size!==pack.size||await digest(compressed)!==pack.sha256)throw Error('Checksum mismatch: '+pack.url);
        const bytes=new Uint8Array(await new Response(new Blob([compressed]).stream().pipeThrough(new DecompressionStream('gzip'))).arrayBuffer());
        offset=0;
        for(const url of pack.files){const file=byURL.get(url);if(!file)throw Error('Unknown packed resource: '+url);await put(file,bytes.slice(offset,offset+file.size));offset+=file.size}
        if(offset!==bytes.byteLength)throw Error('Resource pack length mismatch: '+pack.url);
    }
    // Publish zero immediately, then stream actual compressed payload bytes.
    // There is no HEAD pass and no wait for all downloads before showing progress.
    await progress(true);
    const jobs=[...FILES.filter(file=>!packed.has(file.url)).map(file=>async()=>{
        const response=await fetch(new URL(file.url,self.registration.scope),{cache:'reload',signal:controller.signal});
        if(!response.ok)throw Error('HTTP '+response.status+' '+file.url);
        await put(file,await response.arrayBuffer());
    }),...PACKS.map(pack=>()=>download(pack))];
    await Promise.all(Array.from({length:4},async()=>{
        try{while(next<jobs.length&&!controller.signal.aborted)await jobs[next++]()}
        catch(error){if(!failure)failure=error;controller.abort(error)}
    }));
    // All writes have settled before deletion; a failed parallel download must
    // not recreate a partially deleted cache or replace a working old version.
    if(failure){await caches.delete(CACHE);await notify({cacheError:String(failure),cacheVersion:VERSION});throw failure}
    await progress(true);await self.skipWaiting();
})()));
self.addEventListener('activate',event=>event.waitUntil((async()=>{
    const names=(await caches.keys()).filter(name=>name.startsWith('ballanced-web-')&&name!==CACHE);
    // The metadata entry is available after the first upgraded activation. For
    // legacy workers, the old complete cache is the last cache containing HTML.
    let previous=await readRoute('active');
    if(!previous||!names.includes(previous)){
        previous=null;
        for(const name of names)if(await (await caches.open(name)).match(new URL('index.html',self.registration.scope)))previous=name;
    }
    const clients=(await self.clients.matchAll({type:'window',includeUncontrolled:true})).filter(client=>client.url.startsWith(self.registration.scope));
    const live=new Set(clients.map(client=>client.id)),keep=new Set([CACHE]);
    for(const client of clients){
        const pinned=await readRoute(client.id)||previous||CACHE;
        await writeRoute(client.id,pinned);keep.add(pinned);
    }
    await writeRoute('active',CACHE);
    await self.clients.claim();
    // Retain caches used by open documents, including older tabs. Collected on
    // the next activation; no periodic polling, no IndexedDB/user-data deletion.
    for(const name of names)if(!keep.has(name))await caches.delete(name);
    const routes=await routing();
    for(const request of await routes.keys()){
        const id=decodeURIComponent(new URL(request.url).pathname.split('/').pop());
        if(id!=='active'&&!live.has(id))await routes.delete(request);
    }
    await notify({cacheActivated:VERSION});
})()));
self.addEventListener('message',event=>{
    if(event.data?.type==='VERSION')event.ports[0]?.postMessage({version:VERSION});
});
self.addEventListener('fetch',event=>{
    const url=new URL(event.request.url),scope=new URL(self.registration.scope);
    if(event.request.method!=='GET'||url.origin!==scope.origin||!url.pathname.startsWith(scope.pathname))return;
    event.respondWith((async()=>{
        if(event.request.mode==='navigate'){
            await writeRoute(event.resultingClientId,CACHE);
            return (await (await caches.open(CACHE)).match(new URL('index.html',scope)))||fetch(event.request);
        }
        const selected=event.clientId?await readRoute(event.clientId):null;
        const cache=await caches.open(selected||CACHE);
        return (await cache.match(event.request,{ignoreSearch:false}))||fetch(event.request);
    })());
});
