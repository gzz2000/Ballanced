import createPlayer from '/PlayerWeb.mjs';

const stdout=[];
const started=performance.now();
const result={done:true,exit:-1};
let player;
const environment={
  BALLANCE_HEADLESS:'1',BALLANCE_FAST_FORWARD_BATCH:'64',BALLANCE_AUTOMATE_QUIT_AFTER_MS:'30000',
  BALLANCE_TAS_TRACE_PLAYER_CORE:'1',BALLANCE_TAS_STATE_TRACE:'/player.trace',
  BALLANCE_TAS_RETAIL_STATE_TRACE:'/retail.trace',BALLANCE_GAMEPLAY_SCORE_TRACE:'/score.csv',
  SDL_VIDEODRIVER:'dummy',SDL_AUDIODRIVER:'dummy',
};
async function upload(name, body) {
  const response=await fetch('/result/'+encodeURIComponent(name),{method:'POST',body});
  if(!response.ok) throw new Error(`Result upload failed: ${name}`);
}
try {
  const manifest=await (await fetch('/manifest.json')).json();
  player=await createPlayer({
    preRun:[module=>Object.assign(module.ENV,environment)],
    print:line=>stdout.push(line), printErr:line=>stdout.push(line),
    onExit:code=>{result.exit=code;},
    onAbort:reason=>{result.error=String(reason);},
  });
  player.FS.mkdirTree('/game/Bin');
  player.FS.mkdirTree('/user/ModLoader/Configs');
  let cursor=0,loaded=0;
  await Promise.all(Array.from({length:8},async()=>{
    while(cursor<manifest.assets.length){
      const asset=manifest.assets[cursor++];
      const response=await fetch(asset.url);
      if(!response.ok) throw new Error(`Asset load failed: ${asset.vfs}`);
      const bytes=new Uint8Array(await response.arrayBuffer());
      player.FS.mkdirTree(asset.vfs.slice(0,asset.vfs.lastIndexOf('/')));
      player.FS.writeFile(asset.vfs,bytes);
      if(++loaded%20===0) postMessage({phase:'assets',loaded,total:manifest.assets.length});
    }
  }));
  player.FS.writeFile('/user/ModLoader/Configs/BallanceTAS.cfg',manifest.config);
  postMessage({phase:'running',assets:loaded});
  const code=player.callMain(['--root-path','/game','--cmo','/game/base.cmo','--user-data-path','/user','--rasterizer','null']);
  if(result.exit===-1 && Number.isInteger(code)) result.exit=code;
} catch(error) {result.error=String(error.stack || error);}
result.elapsed_ms=performance.now()-started;
await upload('stdout.log',stdout.join('\n'));
if(player) {
  for(const [name,path] of [['ModLoader.log','/user/ModLoader/ModLoader.log'],['player.trace','/player.trace'],['retail.trace','/retail.trace'],['score.csv','/score.csv']]){
    try {await upload(name,player.FS.readFile(path));}
    catch(error) {result.error ??= String(error);}
  }
}
await upload('result.json',JSON.stringify(result));
postMessage(result);
