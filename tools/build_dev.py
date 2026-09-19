#!/usr/bin/env python3
"""Build the first native 2.4G development increment from locked 2.3F."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
from build_baseline import ROOT, sha
from prepare_dev import prepare_dev


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--vasm', default=str(ROOT / 'local/vasm/vasmm68k_mot'))
    args = p.parse_args()
    lock = json.loads((ROOT / 'baseline.lock.json').read_text())
    source = ROOT / 'vendor/pt23f'
    for item in lock['files']:
        if sha(source / item['path']) != item['sha256']:
            raise SystemExit('Baseline changed: ' + item['path'])
    out = ROOT / 'build/dev'
    out.mkdir(parents=True, exist_ok=True)
    guard = ROOT / 'src/native/mod_guard.s'
    prepared, counts = prepare_dev((source / 'PT2.3F.s').read_bytes(), guard.read_bytes(), (ROOT / 'src/native/input.s').read_bytes())
    if counts != lock['immediate_spelling_counts']:
        raise SystemExit('Unexpected assembler preparation')
    generated = out / 'PT2.4G.dev.s'
    generated.write_bytes(prepared)
    assembler = shutil.which(args.vasm)
    if not assembler:
        p.error('assembler unavailable')
    command = [assembler, *lock['assembler_flags'], '-o', str(out / 'PT2.4G'), str(generated)]
    result = subprocess.run(command, cwd=source, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    (out / 'assembly.log').write_text(result.stdout)
    print(result.stdout, end='')
    result.check_returncode()
    report = {'upstream_commit': lock['upstream_commit'], 'stage': '2.4G dev2',
              'assembler_sha256': sha(Path(assembler)), 'flags': lock['assembler_flags'],
              'guard_sha256': sha(guard), 'input_sha256': sha(ROOT / 'src/native/input.s'), 'preparer_sha256': sha(ROOT / 'tools/prepare_dev.py'),
              'generated_source_sha256': sha(generated),
              'binary_bytes': (out / 'PT2.4G').stat().st_size,
              'binary_sha256': sha(out / 'PT2.4G')}
    (out / 'build.json').write_text(json.dumps(report, indent=2) + '\n')
    shutil.copyfile(source / 'release/PT23F/PT.HELP', out / 'PT.HELP')
    shutil.copyfile(source / 'LICENSE', out / 'LICENSE-2.3F.txt')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
