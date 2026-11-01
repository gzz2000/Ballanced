// Release gate: frozen old plugins and a newly built SDK example on the new host.
import {spawn} from 'node:child_process';
import {mkdir,writeFile,readFile} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const [url,output,tape,newMod]=process.argv.slice(2);
if(!url||!output||!tape||!newMod)throw Error('Usage: qualify-abi.mjs URL OUTPUT ORIGINAL_HS01_TAPE NEW_WEBHELLO_BWMOD');
const here=path.dirname(fileURLToPath(import.meta.url)),fixtures=path.join(here,'mods/abi/fixtures'),out=path.resolve(output);
await mkdir(out,{recursive:true});
const cases=[
 {id:'old-mod',script:'qualify-mods.mjs',args:[path.join(fixtures,'WebHello.bwmod')],result:'result.json'},
 {id:'new-mod',script:'qualify-mods.mjs',args:[path.resolve(newMod)],result:'result.json'},
 {id:'old-tas',script:'qualify-share-playback.mjs',args:['builtin'],result:'builtin.json',env:{BALLANCE_TEST_DEFAULT_MODS:fixtures,BALLANCE_TEST_TAPE:path.resolve(tape),BALLANCE_TEST_SNAPSHOTS:'s1000,r1500,r1500,s3000,r3500,r3500'}}
];
const results=await Promise.all(cases.map(async test=>{
 const folder=path.join(out,test.id);let log='';
 const child=spawn(process.execPath,[path.join(here,test.script),url,folder,...test.args],{env:{...process.env,...test.env},stdio:['ignore','pipe','pipe']});
 child.stdout.on('data',x=>log+=x);child.stderr.on('data',x=>log+=x);
 const code=await new Promise((resolve,reject)=>{child.once('error',reject);child.once('exit',resolve)});
 await writeFile(path.join(out,test.id+'.log'),log);
 const result=JSON.parse(await readFile(path.join(folder,test.result),'utf8'));
 return {id:test.id,code,passed:code===0&&result.passed,result};
}));
const summary={passed:results.every(x=>x.passed),results};
await writeFile(path.join(out,'result.json'),JSON.stringify(summary,null,2)+'\n');
console.log(JSON.stringify({passed:summary.passed,tests:results.map(({id,passed})=>({id,passed}))}));
if(!summary.passed)process.exitCode=1;
