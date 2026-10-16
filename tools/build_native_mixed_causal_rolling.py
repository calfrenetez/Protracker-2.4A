#!/usr/bin/env python3
"""Pinned standalone native rolling68 software fixture; compile/link only.

Derived from the existing standalone compiler harness, retaining its exact flags,
seven-runtime, full frozen-source and native SDK provenance checks. The original
scheduled-output builder remains unchanged and read-only. No target is executed.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import sys
import time
import traceback

SOURCES = ['tests/native_exec_mixed_causal_rolling_test.c', 'src/editor/editor_mixed_causal_prepare.c', 'src/editor/sampler_mixed_readers.c', 'src/core/mixed_readers_causal.c', 'src/core/amigus_trigger_levels.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/editor/editor.c', 'src/editor/song.c', 'src/editor/view.c', 'src/editor/workflow.c', 'src/editor/sample_range.c', 'src/editor/wave_summary.c', 'src/core/sample_usage.c', 'src/core/event_resource.c', 'src/core/flow.c', 'src/core/pitch.c', 'src/core/amigus_reservation.c', 'src/editor/sampler_invert_song.c', 'src/core/render_invert.c', 'src/core/invert_bank.c', 'src/core/invert_sequence.c', 'src/core/invert_pcm.c', 'src/core/invert_loop.c', 'src/core/amigus_session.c', 'src/core/amigus_fifo.c', 'src/core/amigus_pcm_pack.c', 'src/core/studio_consumer.c', 'src/core/studio_pump.c', 'src/core/studio_queue.c', 'src/editor/editor_studio.c', 'src/editor/sampler_song.c', 'src/editor/sampler_studio.c', 'src/core/studio_song.c', 'src/core/studio_plan.c', 'src/core/render.c', 'src/core/timeline.c', 'src/core/frame_clock.c', 'src/core/studio_mix.c', 'src/core/voice.c', 'src/editor/wavetable_song.c', 'src/editor/wavetable_voices.c', 'src/editor/wavetable_dispatch.c', 'src/core/amigus_render_voice.c', 'src/editor/paula_preflight.c', 'src/core/paula_render_voice.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c', 'src/editor/editor_mixed.c', 'src/editor/mixed_owner.c', 'src/editor/mixed_transport.c', 'src/editor/mixed_preflight.c', 'src/editor/paula_voices.c', 'src/editor/paula_dispatch.c', 'src/editor/editor_wavetable.c', 'src/platform/sample_import.c', 'src/platform/raw_import.c', 'src/platform/mod_import.c', 'src/platform/pp20_import.c', 'src/editor/editor_mixed_causal_song_pair.c', 'src/editor/editor_mixed_causal_song_lineage.c', 'src/editor/sampler_establish.c', 'src/editor/mixed_quantized_audit.c', 'src/editor/mixed_readers_plan_normalize.c', 'src/native/readers_ram/native_mixed_causal_ram_port.c', 'src/editor/sampler_lifetime.c']
FLAGS = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
         '-Wall', '-Wextra', '-Werror', '-Isrc/core', '-Ibuild/dev', '-fbbb=-']
EXTRA_COMPILE_FLAGS = ['-ffunction-sections', '-fdata-sections']
EXTRA_LINK_FLAGS = ['-Wl,--gc-sections']
RUNTIMES = ['ncrt0.o', 'libnix20.a', 'libnixmain.a', 'libnix.a',
            'libstubs.a', 'libamiga.a', 'libgcc.a']
TARGET = 'PTMixedCausalRollingTest'
SCOPE = ('Portable pinned 68k compile/link only; no HUNK execution, actual clock/bus, native/Exec '
         'allocator, Chip/DMA, device, song, IRQ, audio or hardware timing proof')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def record(path):
    p = Path(path)
    return {'path': str(p), 'bytes': p.stat().st_size, 'sha256': digest(p)}


def require_pin(path, expected):
    p = Path(path)
    if not p.is_file() or digest(p) != expected:
        raise ValueError('Pinned input absent or changed: ' + str(p))


def snapshot(source):
    result = {}
    for p in sorted(source.rglob('*')):
        if p.is_symlink():
            raise ValueError('Frozen source contains a symlink: ' + str(p))
        if p.is_file():
            result[p.relative_to(source).as_posix()] = digest(p)
    return result


def require_source(source, expected):
    if snapshot(source) != expected:
        raise ValueError('Complete frozen source inventory changed')


def source_fingerprint(expected):
    return hashlib.sha256(json.dumps(expected, sort_keys=True,
                                     separators=(',', ':')).encode()).hexdigest()


def below(p, root):
    return p == root or root in p.parents


def write_json(path, value):
    staged = path.with_suffix(path.suffix + '.writing')
    staged.write_text(json.dumps(value, indent=2) + '\n')
    staged.replace(path)


def dependencies(output):
    output = re.sub(r'\\\r?\n', ' ', output)
    names = []
    for line in output.splitlines():
        colon = re.search(r'(?<!\\):', line)
        if colon:
            names.extend(shlex.split(line[colon.end():]))
    return list(dict.fromkeys(names))


def recipe_contract(source, expected, pins):
    """Read declarations as data, without importing existing build/test code."""
    recipe = source / 'tools/build_core_tests.py'
    require_pin(recipe, pins['canonical_recipe']['sha256'])
    tree = ast.parse(recipe.read_text())
    flags = next(n.value for n in ast.walk(tree) if isinstance(n, ast.Assign)
                 and any(isinstance(t, ast.Name) and t.id == 'flags' for t in n.targets))
    literals = [n.value for n in flags.elts if isinstance(n, ast.Constant)]
    expansions = [ast.unparse(n.value) for n in flags.elts if isinstance(n, ast.Starred)]
    if literals != FLAGS[:-1] or expansions != ['compiler_safety_flags(cc)']:
        raise ValueError('Committed native flag convention changed')
    diagnostic = source / 'tools/build_diagnostic.py'
    require_pin(diagnostic, expected['tools/build_diagnostic.py'])
    d = ast.parse(diagnostic.read_text())
    fn = next(n for n in d.body if isinstance(n, ast.FunctionDef) and n.name == 'runtime_inputs')
    declared = next(ast.literal_eval(n.value) for n in fn.body if isinstance(n, ast.Assign)
                    and any(isinstance(t, ast.Name) and t.id == 'names' for t in n.targets))
    if declared != RUNTIMES:
        raise ValueError('Committed seven-runtime convention changed')
    host = source / 'tests/test_native_mixed_causal_rolling_entry.py'
    h = ast.parse(host.read_text())
    units = next(ast.literal_eval(n.value) for n in h.body if isinstance(n, ast.Assign)
                 and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in n.targets))
    units = list(units)  # Preserve the literal tuple's exact ordering.
    if units != SOURCES or len(units) != len(set(units)):
        raise ValueError('Host ordered source declaration changed')
    return {'committed_flag_recipe': record(recipe), 'committed_runtime_recipe': record(diagnostic),
            'host_declaration': record(host), 'exact_ordered_translation_units': SOURCES,
            'flags': FLAGS, 'runtime_names': RUNTIMES,
            'policy': 'Standalone one-target build, reusing committed flags/runtime conventions without executing or editing the shared recipe'}


def main():
    sys.dont_write_bytecode = True
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source-manifest', type=Path, required=True)
    ap.add_argument('--source-manifest-sha256', required=True)
    ap.add_argument('--pins', type=Path, required=True)
    ap.add_argument('--pins-sha256', required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--diagnostic-case', type=int, choices=range(69),
                    help='Separate diagnostic: one original case (1..68), or all68 (0); never a full68 qualification marker')
    args = ap.parse_args()
    units = list(SOURCES)
    extra_compile = list(EXTRA_COMPILE_FLAGS)
    target = TARGET
    marker = b'ROLLING RAM NATIVE PASS:'
    if args.diagnostic_case is not None:
        units[0] = 'tests/native_exec_mixed_causal_rolling_diagnostic.c'
        extra_compile.append('-DPT_ROLLING_DIAGNOSTIC_CASE=' + str(args.diagnostic_case))
        target = 'PTMixedRollingDiagnostic'
        marker = b'ROLLING DIAGNOSTIC PASS:'
    source = Path(__file__).resolve().parents[1]
    out = args.out.resolve()
    manifest_input, pin_input = args.source_manifest.resolve(), args.pins.resolve()
    if out.exists() or below(out, source) or below(source, out):
        raise ValueError('Output must be fresh and outside source')
    if below(manifest_input, out) or below(pin_input, out):
        raise ValueError('Input controls must be outside output')
    require_pin(manifest_input, args.source_manifest_sha256)
    require_pin(pin_input, args.pins_sha256)
    complete = json.loads(manifest_input.read_text())
    expected = complete['files']
    if not isinstance(expected, dict) or not expected:
        raise ValueError('Full source manifest dictionary required')
    require_source(source, expected)
    all_pins = json.loads(pin_input.read_text())
    pins, python = all_pins['native'], all_pins['host']['python']
    compiler = pins['compiler']['path']
    if Path(sys.executable).resolve() != Path(python['path']).resolve() or sys.version != python['version']:
        raise ValueError('Python executable/version does not match supplied pin')
    require_pin(python['path'], python['sha256'])
    require_pin(compiler, pins['compiler']['sha256'])
    if set(pins['runtime_inputs']) != set(RUNTIMES):
        raise ValueError('Exactly seven runtime pins required')
    sdk_roots = [Path(p).resolve() for p in pins['allowed_sdk_roots']]
    if not sdk_roots:
        raise ValueError('Explicit native SDK roots required')
    convention = recipe_contract(source, expected, all_pins)
    builder = record(Path(__file__).resolve())
    out.mkdir(parents=True)
    started = time.monotonic()
    commands, compile_commands, runtime_paths, sdk_inputs, canonical_inputs = [], [], {}, {}, {}
    result = {'scope': SCOPE, 'passed': False, 'source_export': str(source),
              'source_manifest': record(manifest_input), 'pins': record(pin_input),
              'base_commit': complete['base_commit'], 'base_tree': complete['base_tree'],
              'source_files_before': len(expected), 'source_fingerprint': source_fingerprint(expected),
              'compiler': pins['compiler'], 'python': python, 'builder': builder,
              'recipe_contract': convention, 'flags': FLAGS, 'extra_compile_flags': extra_compile, 'extra_link_flags': EXTRA_LINK_FLAGS, 'runtime_inputs': {},
              'diagnostic_case': args.diagnostic_case,
              'runtime_paths': runtime_paths, 'commands': commands, 'targets': {}}

    def persist():
        command_path = out / 'compile-commands.json'
        write_json(command_path, {'commands': compile_commands})
        result['compile_commands'] = record(command_path)
        result['elapsed_s'] = time.monotonic() - started
        write_json(out / 'manifest.json', result)

    def run(argv, label):
        row = {'label': label, 'argv': argv, 'directory': str(source)}
        commands.append(row)
        persist()
        completed = subprocess.run(argv, cwd=source, capture_output=True, text=True, timeout=180)
        log = out / (label + '.log')
        log.write_text(completed.stdout + completed.stderr)
        row.update(returncode=completed.returncode, log=record(log))
        persist()
        if completed.returncode:
            raise subprocess.CalledProcessError(completed.returncode, argv, completed.stdout, completed.stderr)
        return completed.stdout

    def toolchain_current():
        require_pin(compiler, pins['compiler']['sha256'])
        require_pin(python['path'], python['sha256'])
        for name, path in runtime_paths.items():
            require_pin(path, pins['runtime_inputs'][name])

    try:
        persist()
        if run([compiler, '--version'], 'compiler-version').splitlines()[0] != pins['compiler']['version']:
            raise ValueError('Native compiler version changed')
        if '-fbbb=' not in run([compiler, '--help=target'], 'compiler-target-help'):
            raise ValueError('Required late-optimizer safety flag unavailable')
        for name in RUNTIMES:
            path = Path(run([compiler, '-m68000', '-msoft-float', '-mcrt=nix20', '-print-file-name=' + name],
                            'runtime-' + name.replace('.', '-')).strip()).resolve()
            require_pin(path, pins['runtime_inputs'][name])
            runtime_paths[name] = str(path)
            result['runtime_inputs'][name] = pins['runtime_inputs'][name]
        for index, unit in enumerate(units):
            toolchain_current()
            text = run([compiler, *FLAGS, *extra_compile, '-M', unit], 'dependencies-%02d' % index)
            for name in dependencies(text):
                path = (source / name).resolve()
                if below(path, source):
                    key = path.relative_to(source).as_posix()
                    if key not in expected:
                        raise ValueError('Unknown canonical dependency: ' + key)
                    require_pin(path, expected[key]);canonical_inputs[key] = expected[key]
                elif any(below(path, root) for root in sdk_roots):
                    sdk_inputs[str(path)] = digest(path)
                else:
                    raise ValueError('Dependency outside frozen source and pinned SDK roots: ' + str(path))
        text = run([compiler, *FLAGS, *extra_compile, '-E', '-dD', units[0]], 'preprocessed-fixture')
        assertions = re.findall(r'^#define assert\([^\n]*', text, re.MULTILINE)
        assertion = assertions[-1] if assertions else None
        if not assertion or 'sl_check(!!(expression)' not in assertion or text.count('__assert_func (') < 2:
            raise ValueError('Enabled SDK donor assertions and exact rolling sl_check macro required')
        if marker.decode('ascii') not in text:
            raise ValueError('Native rolling fixture marker missing')
        toolchain_current()
        require_source(source, expected)
        binary = out / target
        argv = [compiler, *FLAGS, *extra_compile, *units, *EXTRA_LINK_FLAGS, '-o', str(binary)]
        compile_commands.append({'name': target, 'directory': str(source), 'argv': argv,
                                 'output': str(binary), 'ordered_translation_units': units})
        run(argv, target + '.build')
        raw = binary.read_bytes()
        if len(raw) < 4 or struct.unpack('>I', raw[:4])[0] != 0x3f3:
            raise ValueError('Linked native product lacks HUNK_HEADER0x3f3')
        if marker not in raw:
            raise ValueError('Linked product lacks shared assertion-completion marker')
        result['targets'][target] = {'scope': SCOPE, 'source_inputs': units, 'flags': FLAGS,
                                     'extra_compile_flags': extra_compile, 'extra_link_flags': EXTRA_LINK_FLAGS,
                                     'dependencies': dict(sorted(canonical_inputs.items())),
                                     'external_sdk_dependencies': dict(sorted(sdk_inputs.items())),
                                     'binary': record(binary), 'format': 'HUNK_HEADER0x000003f3',
                                     'last_assert_macro': assertion,
                                     'preprocessed_fixture': record(out / 'preprocessed-fixture.log'),
                                     'build_log': record(out / (target + '.build.log'))}
        result['passed'] = True
    except Exception as error:
        result['failure'] = {'kind': type(error).__name__, 'message': str(error), 'traceback': traceback.format_exc()}
    finally:
        try:
            toolchain_current()
            for path, value in sdk_inputs.items():
                require_pin(path, value)
            require_source(source, expected)
            require_pin(manifest_input, args.source_manifest_sha256)
            require_pin(pin_input, args.pins_sha256)
            require_pin(builder['path'], builder['sha256'])
            for name in ['tools/build_core_tests.py', 'tools/build_diagnostic.py', 'tests/test_native_mixed_causal_rolling_entry.py']:
                require_pin(source / name, expected[name])
            result['source_files_after'] = len(expected)
        except Exception as error:
            result['passed'] = False
            result['final_provenance_error'] = {'kind': type(error).__name__, 'message': str(error)}
        persist()
    print(json.dumps({'passed': result['passed'], 'scope': SCOPE, 'manifest': str(out / 'manifest.json')}))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
