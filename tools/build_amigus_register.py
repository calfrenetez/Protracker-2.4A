#!/usr/bin/env python3
"""Build injected register fixtures without rebuilding the dirty editor."""
import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path
from build_diagnostic import ROOT, digest, compiler_safety_flags, runtime_inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('AMIGA_CC', 'm68k-amigaos-gcc'))
    args = parser.parse_args()
    cc = shutil.which(args.cc)
    if not cc:
        parser.error('provide the pinned AMIGA_CC')
    flags = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
             '-Wall', '-Wextra', '-Werror', '-Isrc/core', *compiler_safety_flags(cc)]
    groups = {
        'PTAmiGusRegisterTest': ['tests/amigus_register_port_test.c',
                               'src/core/amigus_register_port.c'],
        'PTExecAmiGusRegisterSessionTest': ['tests/native_exec_amigus_register_session_test.c',
            'src/core/amigus_register_port.c', 'src/core/amigus_session.c',
            'src/core/studio_consumer.c', 'src/core/studio_queue.c',
            'src/core/amigus_fifo.c', 'src/core/amigus_pcm_pack.c', 'src/core/pcm.c'],
    }
    out = ROOT / 'build/dev'
    out.mkdir(parents=True, exist_ok=True)
    dependencies = {str(Path(__file__).resolve().relative_to(ROOT)), 'tools/build_diagnostic.py'}
    for name, sources in groups.items():
        subprocess.run([cc, *flags, *sources, '-o', str(out / name)], cwd=ROOT, check=True)
        for source in sources:
            raw = subprocess.check_output([cc, *flags, '-MM', source], cwd=ROOT,
                                          text=True).replace('\\\n', ' ')
            dependencies.update(str((ROOT / p).resolve().relative_to(ROOT))
                                for p in raw.split(':', 1)[1].split())
    report = dict(scope='Compile/link injected bus fixtures only; no card or MMIO',
        compiler_sha256=digest(Path(cc)), runtime_inputs=runtime_inputs(cc), flags=flags,
        inputs={p: digest(ROOT / p) for p in sorted(dependencies)},
        binaries={name: dict(sha256=digest(out / name), bytes=(out / name).stat().st_size)
                  for name in groups})
    (out / 'amigus-register-build.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report['binaries'], indent=2))


if __name__ == '__main__':
    main()
