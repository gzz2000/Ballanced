#!/usr/bin/env python3
"""Exercise ABI guards against real SDK headers and a linked host binary."""
import argparse,json,pathlib,shutil,tempfile
from abi_contract import Reader,check_headers,check_wasm
p=argparse.ArgumentParser();p.add_argument('--sdk',type=pathlib.Path,required=True);p.add_argument('--wasm',type=pathlib.Path,required=True);a=p.parse_args()
sdk=json.loads((a.sdk/'sdk.json').read_text());check_headers(a.sdk/'include',sdk);check_wasm(a.wasm)
def rejected(action,label):
    try:action()
    except ValueError as e:print('REJECTED',label,str(e)[:160]);return
    raise AssertionError('Guard accepted '+label)
rejected(lambda:check_headers(a.sdk/'include',{**sdk,'emscripten':'a-new-compiler'}),'toolchain ABI change')
with tempfile.TemporaryDirectory(prefix='ballance-abi-') as tmp:
    root=pathlib.Path(tmp);shutil.copytree(a.sdk/'include',root/'include')
    header=root/'include/BML/IMod.h';text=header.read_text()
    old='virtual ~IMod()';assert old in text
    header.write_text(text.replace(old,'virtual void BreakingNewVtableSlot() {}\n    '+old,1))
    rejected(lambda:check_headers(root/'include',sdk),'new virtual slot in IMod')
    # Rename an actual export without altering its code: valid WASM, broken ABI.
    data=bytearray(a.wasm.read_bytes());r=Reader(data);r.take(8);found=False
    while r.pos<len(data):
        kind,size=r.byte(),r.uint();start=r.pos;section=r.take(size)
        if kind!=7:continue
        s=Reader(section)
        for _ in range(s.uint()):
            n=s.uint();offset=s.pos;name=s.take(n).decode();s.byte();s.uint()
            if name=='malloc':data[start+offset:start+offset+n]=b'zalloc';found=True
    assert found
    broken=root/'renamed-export.wasm';broken.write_bytes(data)
    rejected(lambda:check_wasm(broken),'removed malloc export')
print('PASS current host ABI plus public-layout, compiler and removed-export rejection')
