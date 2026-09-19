#!/usr/bin/env python3
"""Build native diagnostic tools and record compiler/runtime/input digests."""
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


def compiler_safety_flags(cc):
    """Disable the custom late optimizer with a verified field-address bug."""
    target_help = subprocess.check_output([cc, '--help=target'], text=True)
    return ['-fbbb=-'] if '-fbbb=' in target_help else []


def runtime_inputs(cc):
    """Record the actual startup object and archives selected by this CRT."""
    names = ['ncrt0.o', 'libnix20.a', 'libnixmain.a', 'libnix.a',
             'libstubs.a', 'libamiga.a', 'libgcc.a']
    result = {}
    for name in names:
        path = Path(subprocess.check_output(
            [cc, '-m68000', '-msoft-float', '-mcrt=nix20', '-print-file-name=' + name],
            text=True).strip())
        if not path.is_file():
            raise SystemExit('Cannot resolve runtime input: ' + name)
        result[name] = digest(path)
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cc', default=os.environ.get('AMIGA_CC', 'm68k-amigaos-gcc'))
    p.add_argument('--tool', choices=['AmiGUSTest', 'PTModCheck'], default='AmiGUSTest')
    args = p.parse_args()
    cc = shutil.which(args.cc)
    if not cc:
        p.error('Set AMIGA_CC to the m68k-amigaos-gcc compiler path')
    flags = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
             '-Wall', '-Wextra', '-Werror', *compiler_safety_flags(cc), '-Ivendor/amigus-sdk']
    inputs = ['src/diagnostic/main.c', 'src/diagnostic/ownership.c']
    headers = ['src/diagnostic/amigus_calls.h', 'src/diagnostic/ownership.h']
    if args.tool == 'PTModCheck':
        flags[-1] = '-Isrc/core'
        inputs = ['tools/modcheck.c', 'src/core/mod_inspect.c']
        headers = ['src/core/mod_inspect.h']
    out = ROOT / 'build/diagnostic'
    out.mkdir(parents=True, exist_ok=True)
    sdk_digest = None
    if args.tool == 'AmiGUSTest':
        lock = json.loads((ROOT / 'amigus-sdk.lock.json').read_text())
        for name, expected in lock['files'].items():
            if digest(ROOT / 'vendor/amigus-sdk' / name) != expected:
                raise SystemExit('SDK input changed: ' + name)
        sdk_digest = digest(ROOT / 'amigus-sdk.lock.json')
    result = subprocess.run([cc, *flags, *inputs, '-o', str(out / args.tool)],
                            cwd=ROOT, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    stem = 'build' if args.tool == 'AmiGUSTest' else 'modcheck-build'
    (out / (stem + '.log')).write_text(result.stdout)
    print(result.stdout, end='')
    result.check_returncode()
    report = {
        'compiler': subprocess.check_output([cc, '--version'], text=True).splitlines()[0],
        'compiler_sha256': digest(Path(cc)),
        'runtime_inputs': runtime_inputs(cc),
        'flags': flags,
        'inputs': {name: digest(ROOT / name) for name in inputs + headers},
        'sdk_lock_sha256': sdk_digest,
        'binary_bytes': (out / args.tool).stat().st_size,
        'binary_sha256': digest(out / args.tool),
    }
    (out / (stem + '.json')).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
