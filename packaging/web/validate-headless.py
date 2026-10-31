#!/usr/bin/env python3
"""Compare a Node-hosted WASM TAS corpus with captured original-player traces.

The input manifest supplies licensed game-independent recording/reference paths.
Each case has id, level, tape, tape_sha256, reference, reference_sha256 and score.
Successful scratch traces are removed; summaries retain hashes and exact counts.
This gate covers captured player fields, not complete-world save/restore.
"""
import argparse
import concurrent.futures
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

FIELDS = [("dt", 8, 12), ("sr_timer", 16, 20), ("keys", 24, 28),
          ("position", 28, 40), ("quaternion", 40, 56), ("matrix", 56, 120),
          ("physics_average", 120, 124), ("physics_times", 124, 148),
          ("physics_time_code", 148, 152), ("core", 152, 344)]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def rows(path):
    data = Path(path).read_bytes()
    if (data[:8] != b"BALTRCE\0" or
            struct.unpack_from("<II", data, 8) not in ((1, 344), (2, 344)) or
            (len(data) - 16) % 344):
        raise ValueError(f"Invalid or incomplete trace: {path}")
    return [data[i:i + 344] for i in range(16, len(data), 344)]


def compare(case, work):
    raw = (work / "stdout.log").read_text(errors="replace")
    log = (work / "user/ModLoader/ModLoader.log").read_text(errors="replace")
    primary = rows(work / "player.trace")
    side = rows(work / "retail.trace")
    reference = rows(case["reference"])
    gates = [tuple(map(int, m)) for m in re.findall(
        r"TAS_RETAIL_GATE tick=(\d+) eligible=(\d+)", raw)]
    assert gates and gates[0][0] == 0
    assert all(a[0] < b[0] for a, b in zip(gates, gates[1:]))
    tick = lambda row: struct.unpack_from("<Q", row)[0]
    assert len(primary) == len(side)
    assert all(a[:8] == b[:8] for a, b in zip(primary, side))
    assert all(tick(b) == tick(a) + 1 for a, b in zip(primary, primary[1:]))
    selected, gate = [], 0
    for row in side:
        while gate + 1 < len(gates) and gates[gate + 1][0] <= tick(row):
            gate += 1
        if gates[gate][1]:
            selected.append(row)
    fields = {}
    for name, begin, end in FIELDS:
        bad = [i for i, (a, b) in enumerate(zip(selected, reference))
               if a[begin:end] != b[begin:end]]
        fields[name] = {"differences": len(bad), "first": bad[:5]}
    identities, reverse, identity_conflicts = {}, {}, 0
    for a, b in zip(selected, reference):
        a, b = struct.unpack_from("<i", a, 20)[0], struct.unpack_from("<i", b, 20)[0]
        if ((a == 0) != (b == 0) or identities.get(a, b) != b or reverse.get(b, a) != a):
            identity_conflicts += 1
        identities[a], reverse[b] = b, a
    with (work / "score.csv").open() as file:
        score = list(csv.DictReader(file))
    with Path(case["score"]).open() as file:
        score_ref = list(csv.DictReader(file))
    score_fields = {key: sum(a[key] != b[key] for a, b in zip(score, score_ref))
                    for key in ("valid_mask", "points", "lives", "level_points")}
    outcome = {key: log.count("On Message " + key) for key in ("LevelFinish", "BallOff", "Dead")}
    passed = (len(selected) == len(reference) > 0 and
              not identity_conflicts and not any(v["differences"] for v in fields.values()) and
              len(score) == len(score_ref) > 0 and not any(score_fields.values()) and
              outcome == dict(LevelFinish=1, BallOff=0, Dead=0) and
              "Goodbye!" in log and "precise x87 (TAS)" in log and
              "native fast" in log and "TAS initialization failed" not in log)
    return dict(passed=passed, rows=[len(selected), len(reference)], fields=fields,
                identity_conflicts=identity_conflicts, outcome=outcome,
                score_rows=[len(score), len(score_ref)], score_fields=score_fields,
                precise_to_fast_transition="native fast" in log)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--player", type=Path, required=True)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--template-user", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--node", default="node")
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--quit-ms", type=int, default=120000)
    parser.add_argument("--keep-traces", action="store_true")
    args = parser.parse_args()
    assert 1 <= args.jobs <= 18
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    cases = json.loads(args.manifest.read_text())["cases"]
    assert len({case["id"] for case in cases}) == len(cases)
    source_player = args.player.resolve()
    candidate = args.output / "candidate"
    candidate.mkdir()
    player = candidate / source_player.name
    # Keep one immutable pair for the whole batch while developers rebuild the
    # working tree. Every process must execute the candidate named in its result.
    identity = {"js": sha(source_player), "wasm": sha(source_player.with_suffix(".wasm"))}
    for suffix in (".js", ".wasm"):
        shutil.copy2(source_player.with_suffix(suffix), player.with_suffix(suffix))
        assert sha(player.with_suffix(suffix)) == identity[suffix[1:]]
    results = []

    def run(case):
        started = time.monotonic()
        work = args.output / case["id"]
        assert work.parent == args.output  # Case IDs are single directory names.
        work.mkdir()
        result = dict(id=case["id"], passed=False, candidate=identity)
        try:
            assert sha(case["tape"]) == case["tape_sha256"]
            assert sha(case["reference"]) == case["reference_sha256"]
            result.update(tape=case["tape_sha256"], reference=case["reference_sha256"],
                          score_reference=sha(case["score"]))
            user = work / "user"
            (user / "ModLoader/TAS").mkdir(parents=True)
            (user / "ModLoader/Configs").mkdir()
            for name in ("Player.ini", "Database.tdb"):
                shutil.copy2(args.template_user / name, user / name)
            shutil.copy2(case["tape"], user / "ModLoader/TAS/qualification.tas")
            (user / "ModLoader/Configs/BallanceTAS.cfg").write_text(
                "TAS {\n B Enable 1\n B AutoRestart 0\n B StopOnFinish 1\n}\n"
                "Startup {\n B Enabled 1\n S Project qualification\n S Level " +
                case["level"] + "\n B AutoLoad 1\n}\nOSD {\n B ShowOSD 0\n}\n")
            env = {k: v for k, v in os.environ.items() if not k.startswith("BALLANCE_")}
            env.update(BALLANCE_HEADLESS="1", BALLANCE_FAST_FORWARD_BATCH="64",
                       BALLANCE_AUTOMATE_QUIT_AFTER_MS=str(args.quit_ms),
                       BALLANCE_TAS_TRACE_PLAYER_CORE="1",
                       BALLANCE_TAS_STATE_TRACE=str(work / "player.trace"),
                       BALLANCE_TAS_RETAIL_STATE_TRACE=str(work / "retail.trace"),
                       BALLANCE_GAMEPLAY_SCORE_TRACE=str(work / "score.csv"),
                       SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
            command = [args.node, str(player), "--root-path", str(args.game.resolve()),
                       "--cmo", str(args.game.resolve() / "base.cmo"),
                       "--user-data-path", str(user), "--rasterizer", "null"]
            (work / "launch.json").write_text(json.dumps(dict(command=command,
                env={k: v for k, v in env.items() if k.startswith(("BALLANCE_", "SDL_"))}), indent=2))
            with (work / "stdout.log").open("x") as stdout:
                process = subprocess.Popen(command, env=env, stdout=stdout, stderr=subprocess.STDOUT)
                (work / "pid.txt").write_text(str(process.pid))
                result["exit"] = process.wait()  # Observe actual exit; never restart on a polling timeout.
            result.update(compare(case, work))
            result["passed"] &= result["exit"] == 0
        except Exception as error:
            result.update(passed=False, error=repr(error))
        result["elapsed_seconds"] = round(time.monotonic() - started, 3)
        if result["passed"] and not args.keep_traces:
            shutil.rmtree(work)
        (args.output / (case["id"] + ".json")).write_text(json.dumps(result, indent=2) + "\n")
        return result

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for future in concurrent.futures.as_completed([pool.submit(run, case) for case in cases]):
            result = future.result()
            results.append(result)
            print(json.dumps({k: result[k] for k in ("id", "passed", "elapsed_seconds")}), flush=True)
            (args.output / "summary.json").write_text(json.dumps(results, indent=2) + "\n")
    passed = results and all(r["passed"] for r in results)
    if passed and not args.keep_traces:
        shutil.rmtree(candidate)
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
