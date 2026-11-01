#!/usr/bin/env python3
"""Build a reproducible .bwmod v1 package from a WASM side module and resources."""
import argparse,gzip,hashlib,json,pathlib,re,struct
MAGIC=b'BWMOD1\0\0'
def exports(data):
    def leb(p):
        n=shift=0
        while True:
            b=data[p];p+=1;n|=(b&127)<<shift
            if b<128:return n,p
            shift+=7
    if data[:8]!=b'\0asm\x01\0\0\0':raise ValueError('Not WebAssembly v1')
    p=8;names=[];dynamic=False
    while p<len(data):
        section=data[p];size,p=leb(p+1);end=p+size
        if section==0:
            n,q=leb(p);dynamic|=data[q:q+n]==b'dylink.0'
        if section==7:
            count,q=leb(p)
            for _ in range(count):
                n,q=leb(q);name=data[q:q+n].decode();q+=n;kind=data[q];_,q=leb(q+1)
                if kind==0:names.append(name)
        p=end
    if not dynamic:raise ValueError('Expected an Emscripten SIDE_MODULE with dylink.0')
    return names

def pack(sdk,mod_id,version,module,create,destroy,requires=(),resources=()):
    if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_-]{0,63}',mod_id):raise ValueError('Invalid mod id')
    if not isinstance(version,str) or not version or len(version)>80:raise ValueError('Invalid version')
    for dep in requires:
        if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_-]{0,63}(?:@[^\s@]{1,80})?',dep):raise ValueError('Invalid dependency '+dep)
    data=pathlib.Path(module).read_bytes();symbols=exports(data)
    for name in (create,destroy):
        if name not in symbols:raise ValueError('Missing entrypoint '+name)
    files={'module.wasm':data}
    for source,path in resources:
        if path.startswith('/') or '\0' in path or '\\' in path or any(x in ('','.','..') for x in path.split('/')):raise ValueError('Unsafe resource path')
        if path in files:raise ValueError('Duplicate path '+path)
        files[path]=pathlib.Path(source).read_bytes()
    manifest={'format':'ballanceweb-mod','formatVersion':1,'id':mod_id,'version':version,
        'sdk':sdk['id'],'entry':'module.wasm','create':create,'destroy':destroy,'requires':[{'id':r.split('@',1)[0],'version':r.split('@',1)[1] if '@' in r else None} for r in requires],
        'webExports':[s for s in symbols if s.startswith('BML_Web')],
        'files':[{'path':p,'size':len(d),'sha256':hashlib.sha256(d).hexdigest()} for p,d in files.items()]}
    meta=json.dumps(manifest,ensure_ascii=False,separators=(',',':')).encode()
    return gzip.compress(MAGIC+struct.pack('<I',len(meta))+meta+b''.join(files.values()),mtime=0),manifest

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('sdk','id','version','module','create','destroy','output'):p.add_argument('--'+name,required=True)
    p.add_argument('--requires',action='append',default=[])
    p.add_argument('--resource',action='append',default=[],help='local-file=package/path')
    a=p.parse_args();data,manifest=pack(json.loads(pathlib.Path(a.sdk).read_text()),a.id,a.version,a.module,a.create,a.destroy,a.requires,[x.split('=',1) for x in a.resource]);pathlib.Path(a.output).write_bytes(data);print(json.dumps(manifest,indent=2))
if __name__=='__main__':main()
