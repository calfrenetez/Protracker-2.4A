#!/usr/bin/env python3
"""Build a no-hardware reservation fixture with the real library adapter linked."""
import argparse, json, os, shutil, subprocess
from pathlib import Path
from build_diagnostic import ROOT, digest, compiler_safety_flags, runtime_inputs

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--discovery', action='store_true', help='Build discovery-only native library probe')
    args = parser.parse_args()
    cc = shutil.which(os.environ.get('AMIGA_CC', 'm68k-amigaos-gcc'))
    if not cc:
        raise SystemExit('Set AMIGA_CC to the pinned compiler')
    lock = json.loads((ROOT / 'amigus-sdk.lock.json').read_text())
    for name, expected in lock['files'].items():
        if digest(ROOT / 'vendor/amigus-sdk' / name) != expected:
            raise SystemExit('SDK input changed: ' + name)
    inputs = ['tests/native_exec_amigus_reserved_session_test.c',
              'src/core/amigus_reservation.c', 'src/native/amigus_reservation.c']
    inputs += ['src/core/' + n + '.c' for n in ['amigus_session', 'amigus_fifo',
               'amigus_pcm_pack', 'studio_consumer', 'studio_queue', 'pcm']]
    headers = ['tests/amigus_reserved_session_test.c', 'tests/native_exec_memory.h',
               'src/native/master_memory.h', 'tests/amigus_reservation_test.c', 'src/core/amigus_reservation.h',
               'src/native/amigus_reservation.h', 'src/diagnostic/amigus_calls.h']
    if args.discovery:
        inputs = ['tools/amigus_discovery.c', 'src/core/amigus_reservation.c',
                  'src/native/amigus_reservation.c']
        headers = ['src/core/amigus_reservation.h', 'src/native/amigus_reservation.h',
                   'src/diagnostic/amigus_calls.h']
    flags = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
             '-Wall', '-Wextra', '-Werror', *compiler_safety_flags(cc),
             '-Isrc/core', '-Ivendor/amigus-sdk']
    out = ROOT / 'build/dev/PTExecAmiGusReservedSessionTest'
    if args.discovery:
        out = ROOT / 'build/dev/PTAmiGusDiscovery'
    out.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([cc, *flags, *inputs, '-o', str(out)], cwd=ROOT, check=True)
    report = {'scope': 'compile/link only; discovery-only native library probe' if args.discovery else 'compile/link only; fixture uses fake library',
              'compiler_sha256': digest(Path(cc)), 'flags': flags,
              'runtime_inputs': runtime_inputs(cc),
              'inputs': {p: digest(ROOT / p) for p in inputs + headers},
              'sdk_lock_sha256': digest(ROOT / 'amigus-sdk.lock.json'),
              'binary_sha256': digest(out), 'binary_bytes': out.stat().st_size}
    (out.parent / ('amigus-discovery-build.json' if args.discovery else 'amigus-reservation-build.json')).write_text(json.dumps(report, indent=2)+'\n')
    print('Built', out)

if __name__ == '__main__':
    main()
