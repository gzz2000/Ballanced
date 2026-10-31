const VERSION=__VERSION__,FILES=__FILES__;
const CACHE='ballanced-web-'+VERSION;
// Persist per-document versions so a newly activated worker (including after
// browser worker eviction) never gives an old running game a new Wasm/JS file.
const ROUTING='ballanced-client-routing';
const routeURL=id=>new URL('__ballance_client__/'+encodeURIComponent(id),self.registration.scope);
const routing=()=>caches.open(ROUTING);
const readRoute=async id=>(await (await routing()).match(routeURL(id)))?.text();
const writeRoute=async(id,name)=>{if(id)await (await routing()).put(routeURL(id),new Response(name))};
const notify=async message=>{for(const client of await self.clients.matchAll({includeUncontrolled:true}))client.postMessage(message)};
self.addEventListener('install',event=>event.waitUntil((async()=>{
    const cache=await caches.open(CACHE);let next=0,completed=0,completedBytes=0;
    try{
        // HEAD exposes encoded Content-Length even though fetch decodes GET
        // bodies. Count completed transfers, never decoded chunks as wire bytes.
        const sizes=new Map();
        await Promise.all(Array.from({length:6},async()=>{
            while(next<FILES.length){
                const file=FILES[next++];let size=null;
                try{
                    const response=await fetch(new URL(file.url,self.registration.scope),{method:'HEAD',cache:'reload'});
                    const length=response.headers.get('Content-Length');
                    if(response.ok&&length!==null&&Number.isSafeInteger(Number(length))&&Number(length)>=0)size=Number(length);
                    else if(response.ok&&!response.headers.get('Content-Encoding'))size=file.size??null;
                }catch{} // Unknown wire size does not prevent offline installation.
                sizes.set(file.url,size);
            }
        }));
        const totalBytes=[...sizes.values()].every(n=>n!==null)?[...sizes.values()].reduce((a,b)=>a+b,0):null;
        next=0;await notify({cacheProgress:0,total:FILES.length,cacheBytes:0,totalBytes,cacheVersion:VERSION});
        await Promise.all(Array.from({length:6},async()=>{
            while(next<FILES.length){
                const file=FILES[next++],url=new URL(file.url,self.registration.scope);
                const response=await fetch(url,{cache:'reload'});
                if(!response.ok)throw new Error('HTTP '+response.status+' '+file.url);
                const bytes=await response.clone().arrayBuffer();
                const digest=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),x=>x.toString(16).padStart(2,'0')).join('');
                if(digest!==file.sha256)throw new Error('Checksum mismatch: '+file.url);
                await cache.put(url,response);completedBytes+=sizes.get(file.url)??0;
                await notify({cacheProgress:++completed,total:FILES.length,cacheBytes:completedBytes,totalBytes,cacheVersion:VERSION});
            }
        }));
    }catch(error){await caches.delete(CACHE);await notify({cacheError:String(error),cacheVersion:VERSION});throw error}
    await self.skipWaiting();
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
