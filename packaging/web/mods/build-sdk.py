#!/usr/bin/env python3
"""Export the exact public headers and compilation contract of this browser host."""
import argparse,pathlib,hashlib,json,shutil
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--engine',type=pathlib.Path,required=True);p.add_argument('--bml',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
out=a.output;out.mkdir(parents=True,exist_ok=True);inc=out/'include'
if inc.exists():shutil.rmtree(inc)
inc.mkdir()
for source in (a.engine/'CK2/include',a.engine/'VxMath/include',a.bml/'include'):
    for file in sorted(source.rglob('*')):
        if file.is_file() and file.suffix in ('.h','.hpp','.inc','.inl'):
            target=inc/file.relative_to(source);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(file,target)
for name in ('imgui.h','imconfig.h','imgui_internal.h','imstb_textedit.h','imstb_rectpack.h','imstb_truetype.h'):
    shutil.copy2(a.bml/'deps/imgui'/name,inc/name)
shutil.copy2(a.bml/'src/BMLImGuiConfig.h',inc/'BMLImGuiConfig.h')
contract={'formatVersion':1,'arch':'wasm32','emscripten':'6.0.13','cxx':20,'exceptions':'emscripten','threads':False,'flags':['-fPIC','-ffp-contract=off','-fexceptions'],'bml':'0.3.14'}
h=hashlib.sha256(json.dumps(contract,sort_keys=True).encode())
for file in sorted(inc.rglob('*')):
    if file.is_file():h.update(str(file.relative_to(inc)).encode()+b'\0'+file.read_bytes())
for name in ('WebModHost.cpp','WebMods.cmake','host-exports.py','web-mod-link.js','BallanceWebMod.cmake'):
    h.update(name.encode()+b'\0'+(pathlib.Path(__file__).parent/name).read_bytes())
contract['id']='bw1-'+h.hexdigest()[:24]
(out/'sdk.json').write_text(json.dumps(contract,indent=2)+'\n')
for name in ('pack-mod.py','BallanceWebMod.cmake','FORMAT.md'):
    shutil.copy2(pathlib.Path(__file__).parent/name,out/name)
shutil.copytree(pathlib.Path(__file__).parent/'example',out/'example',dirs_exist_ok=True)
licenses=out/'licenses';licenses.mkdir(exist_ok=True)
for name,source in [('Ballanced',a.engine.parent/'LICENSE'),('CK2',a.engine/'CK2/LICENSE'),('VxMath',a.engine/'VxMath/LICENSE'),('BML',a.bml/'LICENSE'),('ImGui',a.bml/'deps/imgui/LICENSE.txt')]:
    shutil.copy2(source,licenses/(name+'.txt'))
print(contract['id'])
