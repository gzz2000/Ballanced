// Same reference gate as the Worker runner, with real WebGPU presentation.
import createPlayer from '/PlayerWeb.mjs';
import {createBallanceGpu} from '/webgpu.mjs';

document.querySelector('#start').onclick = async () => {
  document.querySelector('#start').disabled = true;
  const stdout = [], started = performance.now();
  const result = {done:true,exit:-1};
  let player, gpu, monitor, finished=false;
  async function upload(name,body) {
    const response=await fetch('/result/'+encodeURIComponent(name),{method:'POST',body});
    if(!response.ok)throw new Error(`Result upload failed: ${name}`);
  }
  async function finish() {
    if(finished)return;finished=true;
    clearInterval(monitor);
    result.elapsed_ms=performance.now()-started;
    result.graphics={frames:gpu?.frames,draws:gpu?.draws,errors:gpu?.errors};
    if(gpu?.errors.length)result.error ??= 'WebGPU validation errors';
    if(!(gpu?.frames>0&&gpu?.draws>0))result.error ??= 'No real GPU draws';
    try {
      await upload('stdout.log',stdout.join('\n'));
      if(player)for(const [name,path] of [['ModLoader.log','/user/ModLoader/ModLoader.log'],['player.trace','/player.trace'],['retail.trace','/retail.trace'],['score.csv','/score.csv']])
        await upload(name,player.FS.readFile(path));
    } catch(error){result.error ??= String(error);}
    await upload('result.json',JSON.stringify(result));
    window.qualificationResult=result;
  }
  window.addEventListener('error',event=>{result.error ??= event.message;void finish();});
  try {
    const manifest=await(await fetch('/manifest.json')).json();
    const canvas=document.querySelector('canvas');
    gpu=await createBallanceGpu(canvas,await(await fetch('/shaders.json')).json());
    const environment={BALLANCE_FAST_FORWARD_BATCH:'64',BALLANCE_AUTOMATE_QUIT_AFTER_MS:'120000',
      BALLANCE_TAS_TRACE_PLAYER_CORE:'1',BALLANCE_TAS_STATE_TRACE:'/player.trace',
      BALLANCE_TAS_RETAIL_STATE_TRACE:'/retail.trace',BALLANCE_GAMEPLAY_SCORE_TRACE:'/score.csv',SDL_AUDIODRIVER:'dummy'};
    player=await createPlayer({canvas,ballanceGpu:gpu,
      preRun:[module=>Object.assign(module.ENV,environment)],
      print:line=>stdout.push(line),printErr:line=>stdout.push(line),
      onBallanceExit:()=>{result.exit=0;void finish();},
      onAbort:reason=>{result.error=String(reason);void finish();},
    });
    player.FS.mkdirTree('/game/Bin');player.FS.mkdirTree('/user/ModLoader/Configs');
    let cursor=0;
    await Promise.all(Array.from({length:8},async()=>{
      while(cursor<manifest.assets.length){
        const asset=manifest.assets[cursor++],response=await fetch(asset.url);
        if(!response.ok)throw new Error(`Asset load failed: ${asset.vfs}`);
        player.FS.mkdirTree(asset.vfs.slice(0,asset.vfs.lastIndexOf('/')));
        player.FS.writeFile(asset.vfs,new Uint8Array(await response.arrayBuffer()));
      }
    }));
    player.FS.writeFile('/user/ModLoader/Configs/BallanceTAS.cfg',manifest.config);
    const code=player.callMain(['--root-path','/game','--cmo','/game/base.cmo','--user-data-path','/user','--rasterizer','webgpu']);
    if(code)throw new Error(`Player initialization failed: ${code}`);
    canvas.focus();
    monitor=setInterval(()=>{
      const log=player.FS.readFile('/user/ModLoader/ModLoader.log',{encoding:'utf8'});
      if(log.includes('On Message LevelFinish')&&
         log.lastIndexOf('native fast')>log.lastIndexOf('precise x87 (TAS)')) {
        clearInterval(monitor);player._BallanceWebRequestQuit();
      }
    },100);
  }catch(error){result.error=String(error.stack||error);await finish();}
};
