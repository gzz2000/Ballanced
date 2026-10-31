#!/usr/bin/env python3
"""Apply the same exact trace/score gate to a browser qualification result."""
import importlib.util
import json
from pathlib import Path
import sys

spec = importlib.util.spec_from_file_location("qualification", Path(__file__).with_name("validate-headless.py"))
qualification = importlib.util.module_from_spec(spec)
spec.loader.exec_module(qualification)
manifest, case_id, output = sys.argv[1:]
case = next(case for case in json.loads(Path(manifest).read_text())["cases"] if case["id"] == case_id)
work = Path(output)
browser = json.loads((work / "browser-result.json").read_text())
assert browser.get("exit") == 0 and not browser.get("error"), browser
assert qualification.sha(case["tape"]) == case["tape_sha256"] == browser["tape"]
assert qualification.sha(case["reference"]) == case["reference_sha256"]
result = qualification.compare(case, work)
result.update(case=case_id, candidate=browser["candidate"], browser=browser["browser"],
              tape=case["tape_sha256"], reference=case["reference_sha256"])
(work / "comparison.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({key: result[key] for key in ("case", "passed", "rows", "score_rows")}))
raise SystemExit(0 if result["passed"] else 1)
