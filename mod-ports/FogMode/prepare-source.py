#!/usr/bin/env python3
"""Fetch a pinned FogMode source revision and apply narrow WASM/BML+ changes.
Upstream has no declared license; this recipe intentionally does not vendor it.
"""
import argparse,hashlib,pathlib,urllib.request
REV='2de9635c2a7bc91ca610d8e9a43d658b0696dc22'
HASHES={'FogMode.h': '0d1bf15e4faeb12c5d80c118998617777f229bcee19cd16efd377ff74871f1e0', 'FogMode.cpp': '56c78052b01bfc09b99bfa9381963accfc4de8ba098824744aba5c116945bb45'}
p=argparse.ArgumentParser();p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--source',type=pathlib.Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
for name in ['FogMode.h','FogMode.cpp']:
    if a.source:data=(a.source/'FogMode'/name).read_bytes()
    else:data=urllib.request.urlopen(f'https://raw.githubusercontent.com/Ghomist/FogMode4Ballance/{REV}/FogMode/{name}').read()
    if hashlib.sha256(data).hexdigest()!=HASHES[name]:raise ValueError('Upstream source hash mismatch: '+name)
    s=data.decode('utf8').replace('CKSTRING','const char *').replace('BOOL','CKBOOL').replace('__declspec(dllexport) IMod* BMLEntry(IBML* bml);','MOD_EXPORT IMod* BMLEntry(IBML* bml);\n MOD_EXPORT void BMLExit(IMod* mod);').replace('m_bml','m_BML').replace('return BML_VERSION;','return "0.3.10-web.1";')
    if name.endswith('.cpp'):
        s=s.replace('IMod* BMLEntry(IBML* bml)', 'MOD_EXPORT IMod* BMLEntry(IBML* bml)').replace('void FogMode::OnLoad()', 'MOD_EXPORT void BMLExit(IMod* mod) { delete mod; }\n\nvoid FogMode::OnLoad()').replace('sscanf_s','std::sscanf').replace('int r = 0, g = 0, b = 0;', 'unsigned r = 0, g = 0, b = 0;').replace('"%d,%d,%d"','"%u,%u,%u"').replace("else if (_str[0] == '#')", "else if (!_str.empty() && _str[0] == '#')")
        s='#include <cstdio>\n'+s
    (a.output/name).write_text(s)
