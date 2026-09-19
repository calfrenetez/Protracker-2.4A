#!/usr/bin/env python3
"""Assemble the locked, unchanged 2.3F snapshot and retain build evidence."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
from prepare_asm import prepare

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vasm', default=os.environ.get('VASM', 'vasmm68k_mot'))
    parser.add_argument('--out', type=Path, default=ROOT / 'build/baseline')
    args = parser.parse_args()
    assembler = shutil.which(args.vasm)
    if assembler is None:
        parser.error('vasm not found; provide --vasm or set VASM')
    assembler = Path(assembler).resolve()
    lock = json.loads((ROOT / 'baseline.lock.json').read_text())
    source = ROOT / 'vendor/pt23f'
    for item in lock['files']:
        if sha(source / item['path']) != item['sha256']:
            raise SystemExit('Baseline input changed: ' + item['path'])
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    binary = out / 'PT2.3F'
    prepared, counts = prepare((source / 'PT2.3F.s').read_bytes())
    if counts != lock['immediate_spelling_counts']:
        raise SystemExit('Unexpected immediate-instruction preparation counts')
    generated = out / 'PT2.3F.compat.s'
    generated.write_bytes(prepared)
    # Devpac mode disables optimisations, aligns data and uses zero CNOP fill.
    # 020 permits upstream's CPU-guarded MULU.L/DIVU.L; no FPU is permitted.
    command = [str(assembler), *lock['assembler_flags'], '-o', str(binary), str(generated)]
    result = subprocess.run(command, cwd=source, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    (out / 'assembly.log').write_text(result.stdout)
    print(result.stdout, end='')
    if result.returncode:
        raise SystemExit(result.returncode)
    digest = sha(binary)
    report = {
        'source_commit': lock['upstream_commit'],
        'source_tree_sha256_manifest': sha(ROOT / 'baseline.lock.json'),
        'assembler_sha256': sha(assembler),
        'assembler_banner': result.stdout.split('\n\n')[0].strip(),
        'flags': lock['assembler_flags'],
        'generated_source_sha256': sha(generated),
        'immediate_spelling_counts': counts,
        'binary_bytes': binary.stat().st_size,
        'binary_sha256': digest,
        'expected_binary_sha256': lock['expected_binary_sha256'],
        'matches_locked_output': digest == lock['expected_binary_sha256'],
    }
    (out / 'build.json').write_text(json.dumps(report, indent=2) + '\n')
    (out / 'SHA256SUMS').write_text(digest + '  PT2.3F\n')
    for name in ['PT.HELP', 'ReadMe.txt', 'PT2.3F.info']:
        shutil.copyfile(source / 'release/PT23F' / name, out / name)
    shutil.copyfile(source / 'LICENSE', out / 'LICENSE-2.3F.txt')
    print(json.dumps(report, indent=2))
    if not report['matches_locked_output']:
        raise SystemExit('Output differs from lock: inspect assembler version/flags before acceptance')


if __name__ == '__main__':
    main()
