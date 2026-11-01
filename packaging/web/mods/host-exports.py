#!/usr/bin/env python3
"""Keep the public CK/BML SDK and host services imported by bundled side modules.
Do not export unrelated IVP diagnostic/unused softfloat routines: those builds
intentionally contain incomplete, unreferenced legacy entry points.
"""
import argparse,pathlib,subprocess,json
from abi_contract import contract
p=argparse.ArgumentParser();p.add_argument('--nm',required=True);p.add_argument('--node',required=True);p.add_argument('--build',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
core=[a.build/'lib'/('lib'+n+'.a') for n in ('CK2Static','CK2_3DStatic','VxMathStatic','BMLPlus')]
def symbols(paths):
    output=subprocess.check_output([a.nm,'--defined-only','--extern-only','--format=posix',*map(str,paths)],text=True)
    return {v[0] for line in output.splitlines() if len(v:=line.split())>=3 and v[1] in ('T','W','D','R','B','V')}
names=symbols(core)
modules=list((a.build/'web-mods').glob('*.wasm'))
js="const fs=require('fs');console.log(JSON.stringify(process.argv.slice(1).flatMap(p=>WebAssembly.Module.imports(new WebAssembly.Module(fs.readFileSync(p))).map(x=>x.name))))"
imports=set(json.loads(subprocess.check_output([a.node,'-e',js,*map(str,modules)],text=True)))
# Preserve already-published host services even when current default mods no longer use them.
names|={name for name,entry in contract()['exports'].items() if entry['kind'] in ('function','global')}
names|=imports # --export-if-defined also retains libc/libc++ services used only by side modules
a.output.write_text('\n'.join('-Wl,--export-if-defined='+s for s in sorted(names))+'\n')
print(len(names),'host SDK/service exports')
