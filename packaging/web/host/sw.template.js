const VERSION=__VERSION__,FILES=__FILES__;
const CACHE='ballanced-web-'+VERSION;
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
    }catch(error){await caches.delete(CACHE);await notify({cacheError:String(error)});throw error}
})()));
self.addEventListener('activate',event=>event.waitUntil((async()=>{
    await self.clients.claim();
    for(const name of await caches.keys())if(name.startsWith('ballanced-web-')&&name!==CACHE)await caches.delete(name);
})()));
self.addEventListener('fetch',event=>{
    const url=new URL(event.request.url),scope=new URL(self.registration.scope);
    if(event.request.method!=='GET'||url.origin!==scope.origin||!url.pathname.startsWith(scope.pathname))return;
    event.respondWith((async()=>{
        const cache=await caches.open(CACHE);
        if(event.request.mode==='navigate')return (await cache.match(new URL('index.html',scope)))||fetch(event.request);
        return (await cache.match(event.request,{ignoreSearch:false}))||fetch(event.request);
    })());
});
