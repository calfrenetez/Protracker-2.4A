#!/usr/bin/env python3
"""Build native core tests and a real-DOS harness for the tracker guard."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
from build_diagnostic import digest, runtime_inputs, ROOT
from make_mod_corpus import cases


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cc', default=os.environ.get('AMIGA_CC', 'm68k-amigaos-gcc'))
    args = p.parse_args()
    cc = shutil.which(args.cc)
    if not cc:
        p.error('provide AMIGA_CC')
    out = ROOT / 'build/dev'
    out.mkdir(parents=True, exist_ok=True)
    inputs = {
        'PTInputProbe': ['tests/native_input_probe.c'],
        'PTChannelsTest': ['tests/channels_test.c', 'src/core/channels.c'],
        'PTPcmTest': ['tests/pcm_test.c', 'src/core/pcm.c', 'src/core/wav.c'],
    }
    flags = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
             '-Wall', '-Wextra', '-Werror', '-Isrc/core']
    for name, sources in inputs.items():
        subprocess.run([cc, *flags, *sources, '-o', str(out / name)], cwd=ROOT, check=True)
    corpus = ROOT / 'local/share/guard'
    corpus.mkdir(parents=True, exist_ok=True)
    rows, manifest = [], []
    for index, (name, data, rc, status) in enumerate(cases()):
        filename = f'f{index:02d}.mod'
        (corpus / filename).write_bytes(data)
        expected = -1 if rc == 20 and status != 'unsupported-format' else 0
        rows += [f"\tdc.b '{filename}',0", f'\tds.b {29-len(filename)}', f'\tdc.w {expected}']
        manifest.append({'index': index, 'case': name, 'expected_guard_result': expected,
                         'scope': 'legacy-dispatch-only' if status == 'unsupported-format' else 'classic-preflight'})
    harness = (ROOT / 'tests/native_guard_harness.s').read_bytes()
    harness += ('\nCaseCount EQU ' + str(len(manifest)) + '\nCases\n' + '\n'.join(rows) + '\n').encode()
    harness += (ROOT / 'src/native/mod_guard.s').read_bytes() + b'\nEND\n'
    generated = out / 'PTGuardTest.s'
    generated.write_bytes(harness)
    subprocess.run([str(ROOT / 'local/vasm/vasmm68k_mot'), '-devpac', '-m68000', '-no-fpu',
                    '-Fhunkexe', '-kick1hunks', '-hunkpad=0', '-nosym', '-o', str(out / 'PTGuardTest'),
                    str(generated)], check=True)
    report = {'compiler': subprocess.check_output([cc, '--version'], text=True).splitlines()[0],
              'compiler_sha256': digest(Path(cc)), 'runtime_inputs': runtime_inputs(cc), 'flags': flags,
              'binaries': {name: {'sha256': digest(out / name), 'bytes': (out / name).stat().st_size}
                           for name in [*inputs, 'PTGuardTest']},
              'sources': {name: digest(ROOT / name) for name in
                          sorted(set(sum(inputs.values(), [])) | {'src/core/channels.h', 'src/core/pcm.h',
                          'src/core/wav.h', 'src/native/mod_guard.s', 'tests/native_guard_harness.s'})},
              'guard_cases': manifest}
    (out / 'core-build.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report['binaries'], indent=2))


if __name__ == '__main__':
    main()
