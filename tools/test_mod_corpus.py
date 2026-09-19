#!/usr/bin/env python3
"""Verify generated corpus through the host CLI and retain each actual result."""
import json
from pathlib import Path
import subprocess
from make_mod_corpus import generate

ROOT = Path(__file__).resolve().parents[1]


def main():
    out = ROOT / 'build/mod-corpus'
    cases = generate(out)
    for case in cases:
        result = subprocess.run([str(ROOT / 'build/host/PTModCheck'), str(out / case['file'])],
                                capture_output=True, text=True)
        case['actual_return_code'] = result.returncode
        case['stdout'] = result.stdout
        case['stderr'] = result.stderr
        case['pass'] = (result.returncode == case['expected_return_code'] and
                        f" result={case['expected_status']}" in result.stdout)
    (out / 'results.json').write_text(json.dumps(cases, indent=2) + '\n')
    if not all(case['pass'] for case in cases):
        raise SystemExit('Corpus failure: inspect build/mod-corpus/results.json')
    print(f'PASS: {len(cases)} synthetic MOD corpus cases (host preflight only)')


if __name__ == '__main__':
    main()
