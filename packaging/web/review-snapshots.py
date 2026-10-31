#!/usr/bin/env python3
"""Verify repeated logical cursors, scores and semantic restore commits."""
import csv,importlib.util,json,pathlib,re,struct,sys
here=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('gate',here/'validate-headless.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
def verify(control,branch,sequence):
    raw=(branch/'stdout.log').read_text(errors='replace');log=(branch/'user/ModLoader/ModLoader.log').read_text(errors='replace')
    expected=[];saved=None;repeats=0
    for i,action in enumerate(sequence.split(',')):
        kind,tick=action[0],int(action[1:]);assert kind in 'sr'
        if kind=='s':saved=tick
        else:assert saved is not None;repeats+=tick-saved
        expected.append((i,kind,tick,saved))
    actions=[(int(i),k,int(t),int(s)) for i,k,t,s in re.findall(r'Snapshot sequence action=(\d+) kind=([sr]) tick=(\d+) saved=(\d+)\.',log)]
    assert actions==expected,('actions',actions,expected)
    restores=sum(k=='r' for _,k,_,_ in expected)
    result={'actions':actions,'repeated_visits':repeats,'streams':{}}
    fields=[('identity',20,24)]+gate.FIELDS
    for name in ('player','retail'):
        ref=gate.rows(control/(name+'.trace'));rows=gate.rows(branch/(name+'.trace'))
        tick=lambda x:struct.unpack_from('<Q',x)[0]
        lookup={tick(x):x for x in ref};seen={};differences=[];self_diff=[];rewinds=0
        for i,row in enumerate(rows):
            t=tick(row);assert t in lookup,('unknown cursor',t)
            if i and t!=tick(rows[i-1])+1:rewinds+=1
            bad=[n for n,l,h in fields if row[l:h]!=lookup[t][l:h]]
            if bad:differences.append((t,bad))
            if t in seen and any(row[l:h]!=seen[t][l:h] for _,l,h in fields):self_diff.append(t)
            seen[t]=row
        # Capture zero precedes the deterministic recording clock on native too.
        # Report it, never normalize it into a matching row.
        assert not [x for x in differences if x[0]!=0],('trace differs',name,differences[:8])
        assert not self_diff,('repeated visit differs',name,self_diff[:8])
        assert set(seen)==set(lookup) and len(rows)-len(seen)==repeats and rewinds==restores
        result['streams'][name]=dict(rows=len(rows),control_rows=len(ref),startup_differences=differences)
    with (control/'score.csv').open() as f:ref=list(csv.DictReader(f))
    with (branch/'score.csv').open() as f:rows=list(csv.DictReader(f))
    expected_scores={r['cursor']:r for r in ref};assert len(expected_scores)==len(ref)
    assert all(expected_scores.get(r['cursor'])==r for r in rows),'score differs at restored cursor'
    assert {r['cursor'] for r in rows}==set(expected_scores),'missing scores'
    for marker in ('Exact transform restore:','Exact 2D restore:','Renderer callback restore committed.','Sound runtime restore committed.'):
        assert log.count(marker)==restores,(marker,log.count(marker),restores)
    for marker in ('COLLISION_GRAPH_COMMIT','PHYSICS_HOST_COMMIT'):
        assert raw.count(marker)==restores,(marker,raw.count(marker),restores)
    checks=[tuple(map(int,m)) for m in re.findall(r'COLLISION_STATE_COMPARE checked=(\d+) differences=(\d+)',raw)]
    assert len(checks)==restores and all(n>0 and d==0 for n,d in checks),('collision restore',checks)
    assert f'Snapshot sequence completed actions={len(expected)}.' in log and 'Goodbye!' in log
    assert {k:log.count('On Message '+k) for k in ('LevelFinish','BallOff','Dead')}==dict(LevelFinish=1,BallOff=0,Dead=0)
    result['padding_only_checks']=log.count('Snapshot core fields match; object padding differs.');result['score_rows']=len(rows);result['collision_checks']=checks;return result

if __name__=='__main__':
    control,branch,sequence=sys.argv[1:]
    control=pathlib.Path(control);branch=pathlib.Path(branch)
    first=json.loads((control/'browser-result.json').read_text());second=json.loads((branch/'browser-result.json').read_text())
    assert first['candidate']==second['candidate'] and first['tape']==second['tape']
    assert not second.get('error') and second['exit']==0
    assert json.loads((control/'comparison.json').read_text())['passed']
    result=dict(passed=True,restore=verify(control,branch,sequence))
    (branch/'comparison.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
