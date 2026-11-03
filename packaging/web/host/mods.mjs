// .bwmod v1: bounded gzip stream -> magic + LE metadata length + JSON + file bytes.
const MAX_BYTES=128*1024*1024,MAX_UNPACKED=256*1024*1024;
const idPattern=/^[A-Za-z][A-Za-z0-9_-]{0,63}$/,symbolPattern=/^[A-Za-z_][A-Za-z0-9_]*$/;
export const modStore='/user/ModLoader/WebMods';
export async function sha256(bytes){return Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('')}
export async function unpackMod(bytes,sdk){
 if(bytes.byteLength>MAX_BYTES)throw Error('插件包超过128MB');
 const reader=new Blob([bytes]).stream().pipeThrough(new DecompressionStream('gzip')).getReader();let length=0;const chunks=[];
 try{for(;;){const {done,value}=await reader.read();if(done)break;length+=value.length;if(length>MAX_UNPACKED)throw Error('插件解压超过256MB');chunks.push(value)}}catch(e){await reader.cancel();throw e}
 const data=new Uint8Array(length);let offset=0;for(const c of chunks){data.set(c,offset);offset+=c.length}
 if(length<12||new TextDecoder().decode(data.subarray(0,8))!=='BWMOD1\0\0')throw Error('不是 BallanceWeb 插件包');
 const metaLength=new DataView(data.buffer).getUint32(8,true);if(metaLength>1024*1024||12+metaLength>length)throw Error('插件清单长度无效');
 const m=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(data.subarray(12,12+metaLength)));
 if(m.format!=='ballanceweb-mod'||m.formatVersion!==1||typeof m.id!=='string'||!idPattern.test(m.id)||typeof m.version!=='string'||!m.version||m.version.length>80)throw Error('插件清单格式无效');
 if(typeof m.sdk!=='string')throw Error('插件缺少 SDK 构建标识');
 if(m.abi!==undefined){
  if(!m.abi||!sdk.abi||m.abi.id!==sdk.abi.id||!Number.isSafeInteger(m.abi.minRevision)||m.abi.minRevision<0||m.abi.minRevision>sdk.abi.revision)throw Error(`${m.id} 的 ABI 不兼容，需要升级宿主或插件`);
 }else if(m.sdk!==sdk.id&&!sdk.legacySdks?.includes(m.sdk))throw Error(`${m.id} 的旧版 SDK 未获兼容认证：${m.sdk}`);
 if(typeof m.create!=='string'||typeof m.destroy!=='string'||!symbolPattern.test(m.create)||!symbolPattern.test(m.destroy)||m.create===m.destroy)throw Error('插件入口无效');
 if(!Array.isArray(m.requires)||m.requires.some(x=>!x||typeof x.id!=='string'||!idPattern.test(x.id)||(x.version!==null&&(typeof x.version!=='string'||!x.version||x.version.length>80))))throw Error('插件依赖无效');
 if(!Array.isArray(m.webExports)||m.webExports.some(x=>typeof x!=='string'||!symbolPattern.test(x)))throw Error('插件网页接口无效');
 if(!Array.isArray(m.files)||!m.files.length||m.files.length>2048)throw Error('插件文件清单无效');
 const files=new Map();offset=12+metaLength;
 for(const f of m.files){
  if(!f||typeof f.path!=='string'||f.path.startsWith('/')||f.path.includes('\\')||f.path.includes('\0')||f.path.split('/').some(x=>!x||x==='.'||x==='..')||files.has(f.path))throw Error('插件包含非法/重复路径');
  if(!Number.isSafeInteger(f.size)||f.size<0||offset+f.size>length)throw Error('插件文件长度无效');
  const contents=data.slice(offset,offset+f.size);offset+=f.size;
  if(await sha256(contents)!==f.sha256)throw Error('插件校验失败：'+f.path);
  files.set(f.path,contents);
 }
 if(offset!==length||!files.has(m.entry))throw Error('插件入口或文件边界无效');
 for(const path of files.keys()){const parts=path.split('/');parts.pop();while(parts.length){if(files.has(parts.join('/')))throw Error('插件文件与目录冲突');parts.pop()}}
 const wasm=files.get(m.entry);
 if(wasm.byteLength<8||new DataView(wasm.buffer,wasm.byteOffset,wasm.byteLength).getUint32(0,true)!==0x6d736100)throw Error('插件入口不是 WASM');
 // Compile/inspect before installing. No constructors or native code run here.
 const module=await WebAssembly.compile(wasm),exports=new Set(WebAssembly.Module.exports(module).filter(x=>x.kind==='function').map(x=>x.name));
 if(!WebAssembly.Module.customSections(module,'dylink.0').length)throw Error('需要 Emscripten SIDE_MODULE');
 for(const name of [m.create,m.destroy,...m.webExports])if(!exports.has(name))throw Error('缺少插件导出：'+name);
 return {manifest:m,files,module};
}
export function validateModSet(mods){
 const ids=new Set(),symbols=new Set();
 for(const {manifest:m} of mods){if(ids.has(m.id))throw Error('重复插件：'+m.id);ids.add(m.id);for(const s of [m.create,m.destroy,...m.webExports]){if(symbols.has(s))throw Error('插件接口冲突：'+s);symbols.add(s)}}
 // Cyclic service dependencies are allowed: every module is linked before any
 // IMod is created. Registration order is the catalog order, then sorted imports.
 for(const {manifest:m} of mods)for(const dep of m.requires){const other=mods.find(x=>x.manifest.id===dep.id)?.manifest;if(!other)throw Error(m.id+' 缺少依赖 '+dep.id);if(dep.version!==null&&other.version!==dep.version)throw Error(m.id+' 需要 '+dep.id+'@'+dep.version);}
}
export async function prepareMods(player,catalog,progress=()=>{}){
 const mods=[];player.FS.mkdirTree(modStore);
 for(const item of catalog.mods){
  progress('正在加载插件 '+item.id+'…');
  const response=await fetch(item.url);if(!response.ok)throw Error('下载插件失败：'+item.id);
  const bytes=new Uint8Array(await response.arrayBuffer());if(await sha256(bytes)!==item.sha256)throw Error('默认插件校验失败：'+item.id);
  const mod=await unpackMod(bytes,catalog.sdk);if(mod.manifest.id!==item.id)throw Error('插件目录身份不一致');mods.push({...mod,source:'default'});
 }
 player.webDefaultMods=mods.map(({manifest})=>manifest);
 for(const file of player.FS.readdir(modStore).sort())if(file.endsWith('.bwmod')){
  const mod=await unpackMod(player.FS.readFile(modStore+'/'+file),catalog.sdk);
  // A previously optional plugin can become bundled after a site update.
  // Keep the old package on disk, but load only the current bundled version.
  if(catalog.mods.some(x=>x.id===mod.manifest.id))continue;
  mods.push({...mod,source:'user'});
 }
 validateModSet(mods);
 // Validate direct imports before any side-module constructor runs. Emscripten's
 // GOT-only finalizer does not catch lazy direct-function stubs such as roundf.
 const provided=new Set(mods.flatMap(m=>WebAssembly.Module.exports(m.module).map(x=>x.name)));
 for(const mod of mods){
  const metadata=player.bwModMetadata(mod.module),missing=[];
  if(metadata.neededDynlibs.length)throw Error(mod.manifest.id+' 需要未打包的动态库：'+metadata.neededDynlibs.join(', '));
  for(const entry of WebAssembly.Module.imports(mod.module)){
   if(entry.kind==='memory'||entry.kind==='table'||['__memory_base','__table_base'].includes(entry.name)||metadata.weakImports.has(entry.name))continue;
   if(!['env','GOT.mem','GOT.func'].includes(entry.module))throw Error(mod.manifest.id+' 使用不支持的导入模块：'+entry.module);
   if(!provided.has(entry.name)&&!player.bwHasModSymbol(entry.name))missing.push(entry.name);
  }
  if(missing.length)throw Error(mod.manifest.id+' 缺少宿主或依赖接口：'+[...new Set(missing)].join(', '));
 }
 for(const {manifest:m} of mods)for(const name of m.webExports)if(player['_'+name])throw Error('主程序接口冲突：'+name);
 for(const mod of mods){
  const m=mod.manifest,root='/web-mods/'+m.id;player.FS.mkdirTree(root);
  for(const [name,bytes] of mod.files){const path=root+'/'+name;player.FS.mkdirTree(path.slice(0,path.lastIndexOf('/')));player.FS.writeFile(path,bytes)}
  await player.loadDynamicLibrary(root+'/'+m.entry,{loadAsync:true,global:true,nodelete:true,allowUndefined:true});
 }
 player.bwFinalizeMods();
 for(const {manifest:m} of mods){
  if(!player.ccall('BallanceWebRegisterMod','number',['string','string','string','string'],[m.id,m.create,m.destroy,'/web-mods/'+m.id]))throw Error('注册插件失败：'+m.id);
  for(const name of m.webExports){if(player['_'+name])throw Error('主程序接口冲突：'+name);const ptr=player.ccall('BallanceWebModSymbol','number',['string'],[name]);if(!ptr)throw Error('网页接口不可用：'+name);player['_'+name]=player.bwModFunction(ptr)}
 }
 return mods.map(({manifest,source})=>({...manifest,source}));
}
