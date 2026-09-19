#!/usr/bin/env python3
"""Build AmiGUSTest and record the actual compiler, flags and binary digest."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cc', default=os.environ.get('AMIGA_CC', 'm68k-amigaos-gcc'))
    args = p.parse_args()
    cc = shutil.which(args.cc)
    if not cc:
        p.error('Set AMIGA_CC to the m68k-amigaos-gcc compiler path')
    flags = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
             '-Wall', '-Wextra', '-Werror', '-Ivendor/amigus-sdk']
    inputs = ['src/diagnostic/main.c', 'src/diagnostic/ownership.c']
    out = ROOT / 'build/diagnostic'
    out.mkdir(parents=True, exist_ok=True)
    lock = json.loads((ROOT / 'amigus-sdk.lock.json').read_text())
    for name, expected in lock['files'].items():
        if digest(ROOT / 'vendor/amigus-sdk' / name) != expected:
            raise SystemExit('SDK input changed: ' + name)
    result = subprocess.run([cc, *flags, *inputs, '-o', str(out / 'AmiGUSTest')],
                            cwd=ROOT, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    (out / 'build.log').write_text(result.stdout)
    print(result.stdout, end='')
    result.check_returncode()
    report = {
        'compiler': subprocess.check_output([cc, '--version'], text=True).splitlines()[0],
        'compiler_sha256': digest(Path(cc)),
        'flags': flags,
        'inputs': {str(f.relative_to(ROOT)): digest(f)
                   for f in sorted((ROOT / 'src/diagnostic').glob('*.[ch]'))},
        'sdk_lock_sha256': digest(ROOT / 'amigus-sdk.lock.json'),
        'binary_bytes': (out / 'AmiGUSTest').stat().st_size,
        'binary_sha256': digest(out / 'AmiGUSTest'),
    }
    (out / 'build.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
