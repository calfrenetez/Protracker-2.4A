#!/usr/bin/env python3
"""Build native core tests and a real-DOS harness for the tracker guard."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
from build_diagnostic import digest, runtime_inputs, compiler_safety_flags, ROOT
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
    font = (ROOT / 'vendor/pt23f/raw/ptfont.raw').read_bytes()
    (out / 'pt_font.h').write_text('/* Pinned ProTracker 2.3F bitmap font; see vendor/pt23f license. */\nstatic const unsigned char pt_font[580] = {' + ','.join(str(b) for b in font) + '};\n')
    inputs = {
        'PT24GEdit': ['src/native/editor_main.c', 'src/editor/editor.c', 'src/editor/view.c', 'src/platform/file_save.c', 'src/core/safe_save.c', 'src/core/document.c', 'src/core/pattern.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTPatternTest': ['tests/pattern_test.c', 'src/core/pattern.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTSlicesTest': ['tests/slices_test.c', 'src/core/slices.c', 'src/core/pcm.c'],
        'PTFileSafetyTest': ['tests/native_file_test.c'],
        'PTProjectTest': ['tests/project_test.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTModProjectTest': ['tests/mod_project_test.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTDocumentTest': ['tests/document_test.c', 'src/core/document.c', 'src/core/safe_save.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PT24GConvert': ['tools/pt24g_convert.c', 'src/platform/file_save.c', 'src/core/document.c', 'src/core/safe_save.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTInputProbe': ['tests/native_input_probe.c'],
        'PTChannelsTest': ['tests/channels_test.c', 'src/core/channels.c'],
        'PTPcmTest': ['tests/pcm_test.c', 'src/core/pcm.c', 'src/core/wav.c'],
    }
    flags = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
             '-Wall', '-Wextra', '-Werror', '-Isrc/core', '-Ibuild/dev', *compiler_safety_flags(cc)]
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
                          'src/core/wav.h', 'src/editor/editor.h', 'src/editor/view.h', 'src/platform/file_save.h', 'vendor/pt23f/raw/ptfont.raw', 'src/core/project.h', 'src/core/mod_project.h', 'src/core/document.h', 'src/core/safe_save.h', 'src/core/pattern.h', 'src/core/slices.h', 'src/core/mod_inspect.h', 'src/native/mod_guard.s', 'tests/native_guard_harness.s'})},
              'guard_cases': manifest}
    (out / 'core-build.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report['binaries'], indent=2))


if __name__ == '__main__':
    main()
