#!/usr/bin/env python3
"""Run isolated real-WebGPU original-reference and optional snapshot cases."""
import argparse
import bisect
import concurrent.futures
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

here = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('gate', here / 'validate-headless.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)
parser = argparse.ArgumentParser(description=__doc__)
for name in ('player', 'game', 'template-user', 'manifest', 'shaders', 'output'):
    parser.add_argument('--' + name, type=Path, required=True)
parser.add_argument('--renderer', type=Path, default=here / '../../Source/RenderEngine/src/CKRasterizer/CKWebGpuRasterizer/webgpu.mjs')
parser.add_argument('--jobs', type=int, default=4)
parser.add_argument('--case')
parser.add_argument('--snapshots', action='store_true')
parser.add_argument('--web-checkpoints', action='store_true')
parser.add_argument('--keep-traces', action='store_true')
a = parser.parse_args()
a.output = a.output.resolve()
a.output.mkdir(parents=True, exist_ok=False)
candidate = a.output / 'candidate'
candidate.mkdir()
for source, name in ((a.player, 'PlayerBrowser.mjs'), (a.player.with_suffix('.wasm'), 'PlayerBrowser.wasm'),
                     (a.shaders, 'shaders.json'), (a.renderer, 'webgpu.mjs')):
    shutil.copy2(source, candidate / name)
cases = [c for c in json.loads(a.manifest.read_text())['cases'] if not a.case or c['id'] == a.case]
assert cases, 'No matching corpus case'
env = dict(os.environ)
if a.web_checkpoints:
    env['BALLANCE_TEST_WEB_CHECKPOINTS'] = '1'
else:
    env.pop('BALLANCE_TEST_WEB_CHECKPOINTS', None)


def sequence_for(control):
    text = (control / 'stdout.log').read_text(errors='replace')
    gates = [tuple(map(int, m)) for m in re.findall(r'TAS_RETAIL_GATE tick=(\d+) eligible=(\d+)', text)]
    ticks = [t for t, _ in gates]
    eligible = [struct.unpack_from('<Q', r)[0] for r in gate.rows(control / 'retail.trace')
                if gates[bisect.bisect_right(ticks, struct.unpack_from('<Q', r)[0]) - 1][1]]
    return ','.join(f's{t},r{t + 200},r{t + 200}' for t in
                    [eligible[int(len(eligible) * fraction)] for fraction in (.25, .5, .75)])


def run(case):
    started = time.monotonic()
    work = a.output / case['id']
    work.mkdir()
    result = dict(id=case['id'], passed=False, web_checkpoints=a.web_checkpoints)
    try:
        for label in ('control', 'branch') if a.snapshots else ('control',):
            command = ['node', str(here / 'qualify-browser.mjs'), str(candidate / 'PlayerBrowser.mjs'),
                       str(a.game.resolve()), str(a.template_user.resolve()), str(a.manifest.resolve()),
                       case['id'], str(work / label), str(candidate / 'shaders.json'), str(candidate / 'webgpu.mjs')]
            if label == 'branch':
                result['sequence'] = sequence_for(work / 'control')
                command += [result['sequence'], str(work / 'control')]
            with (work / (label + '.log')).open('w') as log:
                subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
            result[label] = json.loads((work / label / 'comparison.json').read_text())
            result[label]['browser'] = json.loads((work / label / 'browser-result.json').read_text())
        result['passed'] = True
    except Exception as error:
        result['error'] = repr(error)
    result['seconds'] = round(time.monotonic() - started, 2)
    (a.output / (case['id'] + '.json')).write_text(json.dumps(result, indent=2) + '\n')
    if result['passed'] and not a.keep_traces:
        shutil.rmtree(work)
    print(case['id'], result['passed'], result.get('error', ''), flush=True)
    return result


with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
    results = list(pool.map(run, cases))
(a.output / 'summary.json').write_text(json.dumps(results, indent=2) + '\n')
raise SystemExit(0 if all(c['passed'] for c in results) else 1)
