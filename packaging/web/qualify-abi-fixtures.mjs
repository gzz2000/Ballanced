// Frozen old binaries on a new host. Use with serviceWorkers:'block': the
// production offline manifest intentionally hashes the production mod packages.
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import path from 'node:path';
export async function installLegacyModFixtures(context,root){
 const origin=JSON.parse(await readFile(path.join(root,'origin.json'),'utf8'));
 for(const [name,hash] of Object.entries(origin.files)){
  const actual=createHash('sha256').update(await readFile(path.join(root,name))).digest('hex');
  if(actual!==hash)throw Error('Frozen ABI fixture changed: '+name);
 }
 const catalog=JSON.parse(await readFile(path.join(root,'mods.json'),'utf8'));
 await context.route('**/mods.json',async route=>{
  const response=await route.fetch(),host=await response.json();
  // Keep the new host ABI/support policy; substitute only the old packages.
  host.mods=catalog.mods;await route.fulfill({response,json:host});
 });
 await context.route('**/mods/*.bwmod',async route=>{
  const name=path.basename(new URL(route.request().url()).pathname);
  if(!catalog.mods.some(m=>m.url==='mods/'+name))throw Error('Unknown fixture '+name);
  await route.fulfill({status:200,contentType:'application/octet-stream',body:await readFile(path.join(root,'mods',name))});
 });
}
