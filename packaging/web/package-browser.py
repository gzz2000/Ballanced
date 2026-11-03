#!/usr/bin/env python3
"""Assemble a local, versioned browser distribution from your game assets."""
import argparse,gzip,hashlib,json,pathlib,shutil,subprocess,sys,importlib.util
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--player',type=pathlib.Path,required=True)
p.add_argument('--shaders',type=pathlib.Path,required=True)
p.add_argument('--gl-shaders',type=pathlib.Path,required=True)
p.add_argument('--game',type=pathlib.Path,required=True)
p.add_argument('--template-user',type=pathlib.Path,required=True)
p.add_argument('--bml-source',type=pathlib.Path,required=True)
p.add_argument('--output',type=pathlib.Path,required=True)
p.add_argument('--mod-build',type=pathlib.Path,help='Engine build with web-mods side modules')
a=p.parse_args();here=pathlib.Path(__file__).resolve().parent;out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
def digest(data):return hashlib.sha256(data).hexdigest()
def copy(source,name):
    path=out/name;path.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,path);return path
for name in ('index.html','app.mjs','style.css','tas-grid.mjs','share-links.mjs','mods.mjs'):copy(here/'host'/name,name)
copy(a.player,'PlayerBrowser.mjs');copy(a.player.with_suffix('.wasm'),'PlayerBrowser.wasm')
copy(a.shaders,'shaders.json')
copy(a.gl_shaders,'shaders-gl.json')
for name in ('webgpu.mjs','webgl2.mjs','renderer.mjs'):
    copy(here/'../../Source/RenderEngine/src/CKRasterizer/CKWebGpuRasterizer'/name,name)
mod_build=(a.mod_build or a.player.resolve().parents[3])
sdk_dir=mod_build/'web-sdk'
subprocess.run([sys.executable,str(here/'mods/build-sdk.py'),'--engine',str(here.parents[1]/'Source'),'--bml',str(a.bml_source),'--output',str(sdk_dir)],check=True)
subprocess.run([sys.executable,str(here/'mods/abi_contract.py'),'--wasm',str(a.player.with_suffix('.wasm'))],check=True)
sdk=json.loads((sdk_dir/'sdk.json').read_text())
spec=importlib.util.spec_from_file_location('pack_mod',here/'mods/pack-mod.py');packer=importlib.util.module_from_spec(spec);spec.loader.exec_module(packer)
mods=[]
for name,entry,exit,requires in [
    ('BallanceTAS','TAS_Static_Entry','TAS_Static_Exit',['PracticeTools@1.0.0']),
    ('LegacyTASRuntime','LegacyTAS_Static_Entry','LegacyTAS_Static_Exit',[]),
    ('PracticeTools','Practice_Static_Entry','Practice_Static_Exit',['BallanceTAS@1.0.0','LegacyTASRuntime@1.0.0'])]:
    data,meta=packer.pack(sdk,name,'1.0.0',mod_build/'web-mods'/f'{name}.wasm',entry,exit,requires)
    target=out/'mods'/f'{name}.bwmod';target.parent.mkdir(exist_ok=True);target.write_bytes(data)
    mods.append(dict(id=name,url='mods/'+target.name,sha256=digest(data)))
# Same independently packaged side module that users could previously import.
# Include it before manifest/offline-pack hashing so it is available offline too.
debug=copy(mod_build/'Source/Player/src/DebugUtilities.bwmod','mods/DebugUtilities.bwmod')
mods.append(dict(id='DebugUtilities',url='mods/'+debug.name,sha256=digest(debug.read_bytes())))
(out/'mods.json').write_text(json.dumps(dict(formatVersion=1,sdk=sdk,mods=mods),indent=2)+'\n')
assets=[]
def asset(source,vfs,default=False):
    name=f'assets/{len(assets):04d}.bin';path=copy(source,name)
    assets.append(dict(url='./'+name,vfs=vfs,default=default,sha256=digest(path.read_bytes())))
for folder in ('3D Entities','Sounds','Text','Textures'):
    for file in sorted((a.game/folder).rglob('*')):
        if file.is_file():asset(file,'/game/'+file.relative_to(a.game).as_posix())
asset(a.game/'base.cmo','/game/base.cmo')
for name in ('Player.ini','Database.tdb'):asset(a.template_user/name,'/user/'+name,True)
for file in sorted((a.bml_source/'packaging/runtime/ModLoader/Fonts').glob('*')):
    if file.is_file():asset(file,'/user/ModLoader/Fonts/'+file.name,True)
(out/'assets.json').write_text(json.dumps(dict(assets=assets),ensure_ascii=False,indent=2)+'\n')
files=[dict(url=f.relative_to(out).as_posix(),size=f.stat().st_size,sha256=digest(f.read_bytes())) for f in sorted(out.rglob('*')) if f.is_file()]
sw=(here/'host/sw.template.js').read_text();version=digest((json.dumps(files,sort_keys=True)+sw).encode())[:24]
# Version the template inputs first, then stamp HTML and hash its final bytes.
# This avoids a self-referential hash while keeping every cached file verified.
html=out/'index.html';html.write_text(html.read_text().replace('__BUILD_VERSION__',version))
for file in files:
    if file['url']=='index.html':
        file.update(size=html.stat().st_size,sha256=digest(html.read_bytes()))
# Explicit gzip payloads avoid hundreds of HEAD probes and give a fixed byte
# denominator even when a CDN uses chunked HTTP compression without lengths.
# Originals stay available for hosts/tests without Service Worker control.
packs=[];group=[];group_size=0
packed=[f for f in files if f['url'].startswith(('assets/','mods/')) or f['url'] in ('PlayerBrowser.wasm','shaders.json','shaders-gl.json')]
def flush_pack():
    if not group:return
    raw=b''.join((out/f['url']).read_bytes() for f in group)
    compressed=gzip.compress(raw,compresslevel=6,mtime=0)
    name='offline/'+digest(compressed)[:24]+'.pack'
    path=out/name;path.parent.mkdir(exist_ok=True);path.write_bytes(compressed)
    packs.append(dict(url=name,size=len(compressed),sha256=digest(compressed),files=[f['url'] for f in group]))
for file in packed:
    if group and group_size+file['size']>16*1024*1024:
        flush_pack();group=[];group_size=0
    group.append(file);group_size+=file['size']
flush_pack()
(out/'sw.js').write_text(sw.replace('__VERSION__',json.dumps(version)).replace('__FILES__',json.dumps(files)).replace('__PACKS__',json.dumps(packs)))
(out/'build.json').write_text(json.dumps(dict(version=version,files=files,packs=packs),indent=2)+'\n')
copy(here/'serve.py','serve.py') # Local server helper; not a browser/offline asset.
print(json.dumps(dict(output=str(out),version=version,files=len(files),bytes=sum(f.stat().st_size for f in out.rglob('*') if f.is_file()))))
