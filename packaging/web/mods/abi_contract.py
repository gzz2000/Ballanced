"""Conservative C++ SDK guard and linked WASM export/signature inspection."""
import hashlib
import json
import pathlib

ABI_FILE = pathlib.Path(__file__).with_name('abi') / 'v1.json'

def contract():
    return json.loads(ABI_FILE.read_text())

def check_headers(include, sdk):
    abi = contract()
    for key, value in abi['toolchain'].items():
        if sdk.get(key) != value:
            raise ValueError(f'ABI v1 toolchain mismatch: {key}; requires {value!r}')
    actual = {p.relative_to(include).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(include.rglob('*')) if p.is_file()}
    changed = sorted(k for k in actual.keys() | abi['headers'].keys()
                     if actual.get(k) != abi['headers'].get(k))
    if changed:
        raise ValueError('ABI v1 public headers changed; preserve the published layouts/API, '
                         'review extensions separately: ' + ', '.join(changed[:15]))
    return abi

class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0
    def byte(self):
        b = self.data[self.pos]; self.pos += 1; return b
    def uint(self):
        n = shift = 0
        while True:
            b = self.byte(); n |= (b & 127) << shift
            if b < 128: return n
            shift += 7
            if shift > 64: raise ValueError('Invalid WASM integer')
    def take(self, n):
        value = self.data[self.pos:self.pos+n]; self.pos += n
        if len(value) != n: raise ValueError('Truncated WASM')
        return value
    def name(self): return self.take(self.uint()).decode('utf-8')
    def limits(self):
        flags = self.uint(); self.uint()
        if flags & 1: self.uint()
    def global_type(self): return [self.byte(), self.byte()]

def wasm_exports(path):
    r = Reader(pathlib.Path(path).read_bytes())
    if r.take(8) != b'\0asm\x01\0\0\0': raise ValueError('Not WASM v1')
    sections = {}
    while r.pos < len(r.data):
        kind = r.byte(); data = r.take(r.uint())
        if kind: sections[kind] = data
    types, functions, globals_, exports = [], [], [], {}
    r = Reader(sections.get(1, b'\0'))
    for _ in range(r.uint()):
        if r.byte() != 0x60: raise ValueError('Unsupported WASM type in ABI guard')
        types.append({'params':list(r.take(r.uint())), 'results':list(r.take(r.uint()))})
    r = Reader(sections.get(2, b'\0'))
    for _ in range(r.uint()):
        r.name(); r.name(); kind = r.byte()
        if kind == 0: functions.append(r.uint())
        elif kind == 1: r.byte(); r.limits()
        elif kind == 2: r.limits()
        elif kind == 3: globals_.append(r.global_type())
        elif kind == 4: r.byte(); r.uint()
        else: raise ValueError('Unsupported WASM import')
    r = Reader(sections.get(3, b'\0'))
    for _ in range(r.uint()): functions.append(r.uint())
    r = Reader(sections.get(6, b'\0'))
    for _ in range(r.uint()):
        globals_.append(r.global_type())
        # Emscripten wasm32 global initializers: const/global.get/ref.func only.
        while True:
            op = r.byte()
            if op == 0x0b: break
            if op in (0x41,0x42,0x23,0xd2): r.uint()
            elif op == 0x43: r.take(4)
            elif op == 0x44: r.take(8)
            elif op == 0xd0: r.byte()
            else: raise ValueError(f'Unsupported WASM initializer {op:#x}')
    r = Reader(sections.get(7, b'\0'))
    for _ in range(r.uint()):
        name, kind, index = r.name(), r.byte(), r.uint()
        if kind == 0: exports[name] = {'kind':'function', **types[functions[index]]}
        elif kind == 3: exports[name] = {'kind':'global','type':globals_[index]}
        else: exports[name] = {'kind':{1:'table',2:'memory',4:'tag'}[kind]}
    return exports

def check_wasm(path):
    abi, actual = contract(), wasm_exports(path)
    bad = sorted(k for k, v in abi['exports'].items() if actual.get(k) != v)
    if bad:
        raise ValueError('ABI v1 missing/changed WASM exports: ' + ', '.join(bad[:15]))
    return len(abi['exports'])

if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--wasm',type=pathlib.Path,required=True)
    a = p.parse_args(); print(f'ABI v1: {check_wasm(a.wasm)} export signatures preserved')
