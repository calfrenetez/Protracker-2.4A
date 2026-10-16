#!/usr/bin/env python3
"""Pinned real three-source RAM preparation probe; compile/link only.

Run this copy from the exact pinned private export, never from the working tree.
There is no emulator, guard, service, transport, deployment or HUNK execution.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import shlex
import stat
import struct
import subprocess
import sys
import time
import traceback
import zlib

SOURCES = ['src/native/prepared_paula_sources_probe.c', 'src/editor/prepared_paula_sources.c', 'src/editor/sampler_paula.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c', 'src/editor/sampler.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/editor/sampler_lifetime.c', 'src/core/sample_usage.c', 'src/native/native_checked_memory.c']
EXPORT = ['src/core/amigus_reservation.h', 'src/core/amigus_sample_ram.h', 'src/core/amigus_wavetable_cache.h', 'src/core/channels.c', 'src/core/channels.h', 'src/core/document.h', 'src/core/mod_inspect.h', 'src/core/mod_project.h', 'src/core/pattern.c', 'src/core/pattern.h', 'src/core/pcm.c', 'src/core/pcm.h', 'src/core/pcm_filtered.c', 'src/core/pcm_internal.h', 'src/core/playback_internal.h', 'src/core/playback_pcm.c', 'src/core/playback_pcm.h', 'src/core/pp20.h', 'src/core/project.c', 'src/core/project.h', 'src/core/raw.c', 'src/core/raw.h', 'src/core/sample_cache.c', 'src/core/sample_cache.h', 'src/core/sample_usage.c', 'src/core/sample_usage.h', 'src/core/sinc_kernel.h', 'src/core/slices.c', 'src/core/slices.h', 'src/core/svx.c', 'src/core/svx.h', 'src/core/wav.c', 'src/core/wav.h', 'src/editor/prepared_paula_sources.c', 'src/editor/prepared_paula_sources.h', 'src/editor/project_snapshot.h', 'src/editor/sampler.c', 'src/editor/sampler.h', 'src/editor/sampler_internal.h', 'src/editor/sampler_lifetime.c', 'src/editor/sampler_lifetime_internal.h', 'src/editor/sampler_master_guard_internal.h', 'src/editor/sampler_paula.c', 'src/editor/sampler_paula.h', 'src/editor/sampler_paula_internal.h', 'src/editor/sampler_workflow.h', 'src/native/master_memory.h', 'src/native/native_checked_memory.c', 'src/native/native_checked_memory.h', 'src/native/paula_memory.h', 'src/native/prepared_paula_sources_probe.c', 'tools/build_core_tests.py', 'tools/build_diagnostic.py', 'tools/build_prepared_paula_sources_probe.py']
FLAGS = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
         '-Wall', '-Wextra', '-Werror', '-Isrc/core', '-Ibuild/dev', '-fbbb=-']
ASSERTIONS = ['-UNDEBUG']
OBJECT_DIAGNOSTICS = ['-fstack-usage']
RUNTIMES = ['ncrt0.o', 'libnix20.a', 'libnixmain.a', 'libnix.a',
            'libstubs.a', 'libamiga.a', 'libgcc.a']
TARGET = 'PTPreparedPaulaSourcesProbe'
PASS_MARKER = ('PREPARED PAULA SOURCES RAM PASS:mono8/16/24 Fast masters;'
               '3 selective Chip caches;cold/HIT/cancel5;exact master/save '
               'preservation;zero accounting;NO OUTPUT AUTHORITY')
SCOPE = ('Pinned 68k compile/link only. Actual Fast/Chip placement, genuine native '
         'RAM ownership/cancellation, output/IRQ/DMA/timing/quiet and physical '
         'acceptance remain unproved until separate exact-candidate runs.')



def resident(path):
    p = Path(path)
    s = p.lstat()
    if not stat.S_ISREG(s.st_mode) or getattr(s, 'st_flags', 0) & 0x40000000:
        raise ValueError('Expected resident regular input: ' + str(p))
    return s


def digest(path):
    resident(path)
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def record(path):
    p = Path(path)
    return {'path': str(p), 'bytes': resident(p).st_size, 'sha256': digest(p)}


def require_pin(path, expected):
    if digest(path) != expected:
        raise ValueError('Pinned input changed: ' + str(path))


def snapshot(source):
    result = {}
    for p in sorted(source.rglob('*')):
        if p.is_symlink():
            raise ValueError('Private export contains a symlink: ' + str(p))
        if p.is_file():
            result[p.relative_to(source).as_posix()] = digest(p)
        elif not p.is_dir():
            raise ValueError('Private export contains a special file: ' + str(p))
    return result


def require_source(source, expected):
    if set(expected) != set(EXPORT) or snapshot(source) != expected:
        raise ValueError('Exact source/header/recipe private export changed')


def below(path, root):
    return path == root or root in path.parents


def write_json(path, value):
    staged = path.with_suffix(path.suffix + '.writing')
    staged.write_text(json.dumps(value, indent=2) + '\n')
    staged.replace(path)


def dependencies(text):
    text = re.sub(r'\\\r?\n', ' ', text)
    result = []
    for line in text.splitlines():
        colon = re.search(r'(?<!\\):', line)
        if colon:
            result.extend(shlex.split(line[colon.end():]))
    return list(dict.fromkeys(result))


def recipe_contract(source, expected, pins):
    # Existing recipes are parsed as data; neither is imported or executed.
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
    tree = ast.parse(diagnostic.read_text())
    fn = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'runtime_inputs')
    names = next(ast.literal_eval(n.value) for n in fn.body if isinstance(n, ast.Assign)
                 and any(isinstance(t, ast.Name) and t.id == 'names' for t in n.targets))
    if names != RUNTIMES:
        raise ValueError('Committed seven-runtime convention changed')
    return {'flags': FLAGS, 'assertion_addition': ASSERTIONS,
            'object_diagnostic_addition': OBJECT_DIAGNOSTICS,
            'ordered_translation_units': SOURCES, 'runtime_names': RUNTIMES,
            'committed_flag_recipe': record(recipe),
            'committed_runtime_recipe': record(diagnostic)}


def main():
    sys.dont_write_bytecode = True
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source-manifest', type=Path, required=True)
    ap.add_argument('--source-manifest-sha256', required=True)
    ap.add_argument('--pins', type=Path, required=True)
    ap.add_argument('--pins-sha256', required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    source = Path(__file__).resolve().parents[1]
    out = args.out.resolve()
    manifest_input, pin_input = args.source_manifest.resolve(), args.pins.resolve()
    if out.exists() or below(out, source) or below(source, out):
        raise ValueError('Output must be fresh and outside private source')
    if below(manifest_input, out) or below(pin_input, out):
        raise ValueError('Input controls must be outside output')
    require_pin(manifest_input, args.source_manifest_sha256)
    require_pin(pin_input, args.pins_sha256)
    complete, pins = json.loads(manifest_input.read_text()), json.loads(pin_input.read_text())
    expected = complete['files']
    require_source(source, expected)
    native, python = pins['native'], pins['host']['python']
    compiler = native['compiler']['path']
    if Path(sys.executable).resolve() != Path(python['path']).resolve() or sys.version != python['version']:
        raise ValueError('Python executable/version does not match supplied pin')
    require_pin(Path(python['path']).resolve(), python['sha256'])
    require_pin(compiler, native['compiler']['sha256'])
    require_pin(native['linker']['path'], native['linker']['sha256'])
    if set(native['runtime_inputs']) != set(RUNTIMES) or set(native['runtime_paths']) != set(RUNTIMES):
        raise ValueError('Exactly seven runtime inputs and origin paths required')
    sdk_roots = [Path(p).resolve() for p in native['allowed_sdk_roots']]
    sdk_expected = native['sdk_inputs']
    if not sdk_roots or not sdk_expected:
        raise ValueError('Explicit SDK roots and prior SDK content pins required')
    convention = recipe_contract(source, expected, pins)
    builder = record(Path(__file__).resolve())
    out.mkdir(parents=True)
    started = time.monotonic()
    commands, compile_commands, runtime_paths, sdk_inputs, selected_tools = [], [], {}, {}, {}
    result = {'passed': False, 'scope': SCOPE, 'source_export': str(source),
              'source_manifest': record(manifest_input), 'pins': record(pin_input),
              'base_commit': complete['base_commit'], 'base_tree': complete['base_tree'],
              'source_files': expected, 'builder': builder, 'recipe_contract': convention,
              'commands': commands, 'runtime_inputs': {}, 'sdk_inputs': sdk_inputs,
              'translation_units': [], 'progressSidecar': False, 'native_subset': 'cold/HIT/five cancellation cuts; not the HOST25/36 matrix',
              'expected_stdout': PASS_MARKER + '\n'}

    def persist():
        write_json(out / 'compile-commands.json', {'commands': compile_commands})
        result['elapsed_s'] = time.monotonic() - started
        write_json(out / 'manifest.json', result)

    def run(argv, label, timeout=60):
        row = {'label': label, 'argv': argv, 'directory': str(source), 'state': 'prepared'}
        commands.append(row)
        persist()
        stdout, stderr, returncode = b'', b'', None
        try:
            done = subprocess.run(argv, cwd=source, capture_output=True, timeout=timeout)
            stdout, stderr, returncode = done.stdout, done.stderr, done.returncode
        except subprocess.TimeoutExpired as error:
            stdout, stderr = error.stdout or b'', error.stderr or b''
            row['timeout_s'] = timeout
            raise
        finally:
            for channel, data in [('stdout', stdout), ('stderr', stderr)]:
                path = out / (label + '.' + channel)
                path.write_bytes(data)
                row[channel] = record(path)
            row.update(returncode=returncode, state='complete' if returncode is not None else 'interrupted')
            persist()
        if returncode:
            raise subprocess.CalledProcessError(returncode, argv, stdout, stderr)
        return stdout.decode('utf-8', 'replace'), stderr.decode('utf-8', 'replace')

    def current():
        require_pin(compiler, native['compiler']['sha256'])
        require_pin(Path(python['path']).resolve(), python['sha256'])
        require_pin(native['linker']['path'], native['linker']['sha256'])
        for name, path in runtime_paths.items():
            require_pin(path, native['runtime_inputs'][name])
        for path, value in sdk_inputs.items():
            require_pin(path, value)
        for path, value in selected_tools.items():
            require_pin(path, value)

    def dependency(path, allowed=None):
        path = (source / path).resolve()
        if allowed is not None and str(path) not in allowed:
            raise ValueError('Actual preprocessor path absent from this unit dependency closure: ' + str(path))
        if below(path, source):
            name = path.relative_to(source).as_posix()
            if name not in expected:
                raise ValueError('Undeclared source dependency: ' + name)
            require_pin(path, expected[name])
        elif any(below(path, root) for root in sdk_roots):
            value = sdk_expected.get(str(path))
            if value is None:
                raise ValueError('SDK dependency has no supplied content pin: ' + str(path))
            require_pin(path, value)
            sdk_inputs[str(path)] = value
        else:
            raise ValueError('Dependency outside private source and pinned SDK: ' + str(path))
        return str(path)

    try:
        persist()
        text, _ = run([compiler, '--version'], 'compiler-version', 10)
        if text.splitlines()[0] != native['compiler']['version']:
            raise ValueError('Compiler version changed')
        text, _ = run([compiler, '--help=target'], 'compiler-target-help', 10)
        if '-fbbb=' not in text:
            raise ValueError('Required optimizer safety flag unavailable')
        text, _ = run([compiler, '-m68000', '-msoft-float', '-mcrt=nix20',
                       '-print-prog-name=ld'], 'linker-origin', 10)
        linker_name = text.strip()
        if not Path(linker_name).is_absolute():
            raise ValueError('Driver linker query must identify an absolute origin')
        linker = Path(linker_name).resolve()
        if linker != Path(native['linker']['path']).resolve():
            raise ValueError('Actual driver linker origin differs from pin')
        require_pin(linker, native['linker']['sha256'])
        result['selected_linker'] = record(linker)
        for name in RUNTIMES:
            text, _ = run([compiler, '-m68000', '-msoft-float', '-mcrt=nix20',
                           '-print-file-name=' + name], 'runtime-' + name.replace('.', '-'), 10)
            path = Path(text.strip()).resolve()
            if path != Path(native['runtime_paths'][name]).resolve():
                raise ValueError('Actual runtime origin changed: ' + name)
            require_pin(path, native['runtime_inputs'][name])
            runtime_paths[name] = str(path)
            result['runtime_inputs'][name] = record(path)
        objects = []
        for index, unit in enumerate(SOURCES):
            current()
            require_source(source, expected)
            stem = 'unit-%02d' % index
            text, _ = run([compiler, *FLAGS, *ASSERTIONS, '-M', unit], stem + '-dependencies')
            closure = sorted({dependency(name) for name in dependencies(text)})
            if str((source / unit).resolve()) not in closure:
                raise ValueError('Translation unit absent from dependency output')
            pp = out / (stem + '.i')
            run([compiler, *FLAGS, *ASSERTIONS, '-E', '-dD', unit, '-o', str(pp)], stem + '-preprocess')
            text = pp.read_text()
            actual = set()
            for match in re.finditer(r'^#\s+\d+\s+("(?:\\.|[^"\\])*")', text, re.MULTILINE):
                name = ast.literal_eval(match.group(1))
                if name in ('<built-in>', '<command-line>'):
                    continue
                actual.add(dependency(name, closure))
            if not actual:
                raise ValueError('No ordinary actual preprocessor line markers')
            if unit.endswith('_probe.c'):
                macros = re.findall(r'^#define assert\([^\n]*', text, re.MULTILINE)
                if not macros or '__assert_func' not in macros[-1] or text.count('__assert_func (') < 2:
                    raise ValueError('Both fixture assertions must remain enabled')
                if PASS_MARKER not in text:
                    raise ValueError('Sole completion marker absent from fixture preprocessing')
                result['last_assert_macro'] = macros[-1]
            obj = out / (stem + '.o')
            argv = [compiler, *FLAGS, *ASSERTIONS, *OBJECT_DIAGNOSTICS, '-c', unit, '-o', str(obj)]
            _, diagnostic = run([*argv, '-###'], stem + '-driver-vector', 10)
            assembler = []
            for line in diagnostic.splitlines():
                parts = shlex.split(line)
                if parts and Path(parts[0]).name in ('as', 'm68k-amigaos-as'):
                    assembler.append(Path(parts[0]).resolve())
            if len(assembler) != 1 or assembler[0] not in [Path(p).resolve() for p in native['assembler']['allowed_paths']]:
                raise ValueError('Actual driver assembler does not match the supplied finite paths')
            require_pin(assembler[0], native['assembler']['sha256'])
            selected_tools[str(assembler[0])] = native['assembler']['sha256']
            compile_commands.append({'unit': unit, 'directory': str(source), 'argv': argv, 'output': str(obj)})
            run(argv, stem + '-compile')
            stack = obj.with_suffix('.su')
            resident(stack)
            row = {'source': unit, 'dependencies': closure, 'actual_line_marker_paths': sorted(actual),
                   'preprocessed': record(pp), 'object': record(obj), 'raw_stack_usage': record(stack),
                   'assembler': record(assembler[0])}
            result['translation_units'].append(row)
            objects.append(str(obj))
        current()
        require_source(source, expected)
        binary, map_path = out / TARGET, out / (TARGET + '.map')
        argv = [compiler, *FLAGS, *ASSERTIONS, *objects, '-Wl,-Map,' + str(map_path), '-o', str(binary)]
        _, diagnostic = run([*argv, '-###'], 'link-driver-vector', 10)
        direct_linkers, collect2_vectors = [], []
        for line in diagnostic.splitlines():
            parts = shlex.split(line)
            if not parts:
                continue
            if Path(parts[0]).name in ('ld', 'm68k-amigaos-ld'):
                direct_linkers.append(Path(parts[0]).resolve())
            elif Path(parts[0]).name == 'collect2':
                collect2_vectors.append(parts)
        if direct_linkers:
            if len(direct_linkers) != 1 or direct_linkers[0] != linker:
                raise ValueError('Actual link vector selects an unexpected linker origin')
            require_pin(direct_linkers[0], native['linker']['sha256'])
        elif len(collect2_vectors) != 1:
            raise ValueError('Actual link vector exposes neither one pinned ld nor one collect2 invocation')
        result['linker_selection'] = {
            'query_origin': linker_name, 'resolved_origin': str(linker),
            'direct_ld_paths': [str(path) for path in direct_linkers],
            'collect2_vectors': collect2_vectors,
            'scope': 'Pinned absolute GCC ld query; direct ld operand also checked when exposed by -###. Collect2 does not expose its child ld in this diagnostic.'}
        current()
        compile_commands.append({'name': TARGET, 'directory': str(source), 'argv': argv, 'output': str(binary)})
        run(argv, 'link')
        loads = re.findall(r'^LOAD\s+(.+?)\s*$', map_path.read_text(), re.MULTILINE)
        resolved_loads = [str((source / p).resolve()) for p in loads]
        allowed_loads = [*objects, *runtime_paths.values()]
        if set(resolved_loads) != set(allowed_loads) or len(resolved_loads) != len(allowed_loads):
            raise ValueError('Link map inputs differ from the declared objects and seven pinned runtimes')
        raw = binary.read_bytes()
        if len(raw) < 4 or struct.unpack('>I', raw[:4])[0] != 0x3f3 or PASS_MARKER.encode() not in raw:
            raise ValueError('Native HUNK header or sole completion marker missing')
        objdump = Path(compiler).with_name('m68k-amigaos-objdump')
        require_pin(objdump, native['objdump']['sha256'])
        selected_tools[str(objdump)] = native['objdump']['sha256']
        run([str(objdump), '-f', '-h', '-t', '-r', str(binary)], 'binary-format', 20)
        result.update(binary=record(binary), crc32='%08X' % (zlib.crc32(raw) & 0xffffffff),
                      format='HUNK_HEADER0x000003f3', link_map=record(map_path),
                      actual_link_loads=resolved_loads, passed=True)
    except Exception as error:
        result['failure'] = {'kind': type(error).__name__, 'message': str(error), 'traceback': traceback.format_exc()}
    finally:
        try:
            current()
            require_source(source, expected)
            require_pin(manifest_input, args.source_manifest_sha256)
            require_pin(pin_input, args.pins_sha256)
            require_pin(builder['path'], builder['sha256'])
        except Exception as error:
            result['passed'] = False
            result['final_provenance_error'] = {'kind': type(error).__name__, 'message': str(error)}
        persist()
    print(json.dumps({'passed': result['passed'], 'scope': SCOPE, 'manifest': str(out / 'manifest.json')}))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
