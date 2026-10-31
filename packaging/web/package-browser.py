#!/usr/bin/env python3
"""Assemble a local, versioned browser distribution from your game assets."""
import argparse,hashlib,json,pathlib,shutil
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--player',type=pathlib.Path,required=True)
p.add_argument('--shaders',type=pathlib.Path,required=True)
p.add_argument('--game',type=pathlib.Path,required=True)
p.add_argument('--template-user',type=pathlib.Path,required=True)
p.add_argument('--bml-source',type=pathlib.Path,required=True)
p.add_argument('--output',type=pathlib.Path,required=True)
a=p.parse_args();here=pathlib.Path(__file__).resolve().parent;out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
def digest(data):return hashlib.sha256(data).hexdigest()
def copy(source,name):
    path=out/name;path.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,path);return path
for name in ('index.html','app.mjs','style.css'):copy(here/'host'/name,name)
copy(a.player,'PlayerBrowser.mjs');copy(a.player.with_suffix('.wasm'),'PlayerBrowser.wasm')
copy(a.shaders,'shaders.json')
copy(here/'../../Source/RenderEngine/src/CKRasterizer/CKWebGpuRasterizer/webgpu.mjs','webgpu.mjs')
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
(out/'sw.js').write_text(sw.replace('__VERSION__',json.dumps(version)).replace('__FILES__',json.dumps(files)))
(out/'build.json').write_text(json.dumps(dict(version=version,files=files),indent=2)+'\n')
copy(here/'serve.py','serve.py') # Local server helper; not a browser/offline asset.
print(json.dumps(dict(output=str(out),version=version,files=len(files),bytes=sum(f.stat().st_size for f in out.rglob('*') if f.is_file()))))
