#!/usr/bin/env python3
"""Replay WASM TAS controls and repeated full-world snapshot branches.

Compares every captured logical cursor and score; also requires semantic restore
providers to commit. This is trajectory/restore coverage, not all-world byte parity.
"""
import argparse,bisect,concurrent.futures,csv,importlib.util,json,os,pathlib,re,shutil,struct,subprocess,time
here=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('gate',here/'validate-headless.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
p=argparse.ArgumentParser(description=__doc__)
for name in ('player','game','template-user','manifest','output'):p.add_argument('--'+name,type=pathlib.Path,required=True)
p.add_argument('--web-checkpoints',action='store_true');p.add_argument('--jobs',type=int,default=8);p.add_argument('--case');p.add_argument('--sequence');p.add_argument('--keep-traces',action='store_true')
a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
candidate=a.output/'candidate';candidate.mkdir()
for suffix in ('.js','.wasm'):shutil.copy2(a.player.with_suffix(suffix),candidate/a.player.with_suffix(suffix).name)
player=candidate/a.player.name;identity={suffix:gate.sha(player.with_suffix('.'+suffix)) for suffix in ('js','wasm')}
cases=json.loads(a.manifest.read_text())['cases'];cases=[c for c in cases if not a.case or c['id']==a.case];assert cases

def launch(case,work,sequence=''):
    user=work/'user';(user/'ModLoader/TAS').mkdir(parents=True);(user/'ModLoader/Configs').mkdir()
    for name in ('Player.ini','Database.tdb'):shutil.copy2(a.template_user/name,user/name)
    shutil.copy2(case['tape'],user/'ModLoader/TAS/qualification.tas')
    (user/'ModLoader/Configs/BallanceTAS.cfg').write_text('TAS {\n B Enable 1\n B StopOnFinish 1\n B AutoRestart 0\n}\nStartup {\n B Enabled 1\n S Project qualification\n S Level '+case['level']+'\n B AutoLoad 1\n}\nOSD {\n B ShowOSD 0\n}\n')
    env={k:v for k,v in os.environ.items() if not k.startswith('BALLANCE_')}
    env.update(BALLANCE_HEADLESS='1',BALLANCE_FAST_FORWARD_BATCH='64',BALLANCE_AUTOMATE_QUIT_AFTER_FAST_FORWARD='1',
        BALLANCE_AUTOMATE_QUIT_AFTER_MS='120000',BALLANCE_TAS_TRACE_PLAYER_CORE='1',
        BALLANCE_TAS_STATE_TRACE=str(work/'player.trace'),BALLANCE_TAS_RETAIL_STATE_TRACE=str(work/'retail.trace'),
        BALLANCE_GAMEPLAY_SCORE_TRACE=str(work/'score.csv'),SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',BALLANCE_SNAPSHOT_AUDIT='1',BALLANCE_COMPARE_COLLISION_GRAPH='1')
    if sequence:env['BALLANCE_TAS_SNAPSHOT_SEQUENCE']=sequence
    if a.web_checkpoints:env['BALLANCE_TEST_WEB_CHECKPOINTS']='1'
    cmd=['node',str(player),'--root-path',str(a.game.resolve()),'--cmo',str(a.game.resolve()/'base.cmo'),'--user-data-path',str(user),'--rasterizer','null']
    with (work/'stdout.log').open('w') as log:result=subprocess.run(cmd,env=env,stdout=log,stderr=subprocess.STDOUT)
    assert result.returncode==0,('exit',result.returncode)

snapshot_spec=importlib.util.spec_from_file_location('snapshots',here/'review-snapshots.py')
snapshots=importlib.util.module_from_spec(snapshot_spec);snapshot_spec.loader.exec_module(snapshots)
verify=snapshots.verify

def run(case):
    start=time.monotonic();work=a.output/case['id'];work.mkdir();result=dict(id=case['id'],passed=False,candidate=identity)
    try:
        assert gate.sha(case['tape'])==case['tape_sha256'] and gate.sha(case['reference'])==case['reference_sha256']
        control=work/'control';branch=work/'branch';launch(case,control)
        result['control']=gate.compare(case,control);assert result['control']['passed'],'independent original reference gate failed'
        raw=(control/'stdout.log').read_text(errors='replace')
        gates=[tuple(map(int,m)) for m in re.findall(r'TAS_RETAIL_GATE tick=(\d+) eligible=(\d+)',raw)];ticks=[t for t,e in gates]
        eligible=[struct.unpack_from('<Q',r)[0] for r in gate.rows(control/'retail.trace') if gates[bisect.bisect_right(ticks,struct.unpack_from('<Q',r)[0])-1][1]]
        sequence=a.sequence or ','.join(f's{t},r{t+200},r{t+200}' for t in [eligible[int(len(eligible)*f)] for f in (.25,.5,.75)])
        result['sequence']=sequence;launch(case,branch,sequence);result['restore']=verify(control,branch,sequence);result['passed']=True
    except Exception as error:result['error']=repr(error)
    result['seconds']=round(time.monotonic()-start,2)
    (a.output/(case['id']+'.json')).write_text(json.dumps(result,indent=2)+'\n')
    if result['passed'] and not a.keep_traces:shutil.rmtree(work)
    print(json.dumps(result),flush=True);return result
with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
    results=list(pool.map(run,cases))
(a.output/'summary.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(0 if all(r['passed'] for r in results) else 1)
