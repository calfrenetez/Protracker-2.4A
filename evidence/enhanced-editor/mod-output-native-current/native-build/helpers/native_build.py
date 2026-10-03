#!/usr/bin/env python3
"""Cross-build exactly the scoped MOD stream Exec fixture from a supplied frozen export.
No test binary, guest, card, register, emulator or physical operation is run.
"""
import ast
import importlib
import json
import re
import struct
import subprocess
import sys
import time
import traceback
from pathlib import Path
from provenance_common import (NATIVE_FLAGS, RUNTIME_NAMES, TARGETS, arguments, assert_pinned,
                               classify, digest, file_record, parse_dependencies,
                               read_canonical_recipe, require_snapshot, setup, write_json)

def python_declarations(source):
    """Read literal host source declarations only; never execute test methods."""
    names = ['tests/test_mod_stream.py']
    inputs, declarations = {}, {}
    for name in names:
        fixture = source / name
        tree = ast.parse(fixture.read_text())
        inputs[name] = digest(fixture)
        units = next([item.value for item in call.args[0].elts if isinstance(item, ast.Constant)
                      and isinstance(item.value, str) and item.value.endswith('.c')]
                     for call in ast.walk(tree) if isinstance(call, ast.Call)
                     and isinstance(call.func, ast.Attribute) and call.func.attr == 'run'
                     and call.args and isinstance(call.args[0], ast.List))
        target = 'PTExecModStreamTest'
        expected = 'tests/mod_stream_test.c'
        if not units or units[0] != expected or len(units) != len(set(units)):
            raise ValueError('Unexpected or duplicate host fixture declaration: ' + target)
        declarations[target] = ['tests/native_exec_mod_stream_test.c', *units[1:]]
    return declarations, inputs


def main():
    sys.dont_write_bytecode = True
    args = arguments(__doc__)
    expected, all_pins, manifest = setup(args, 'Pinned host cross-build only; no native execution/target/physical/audio/timing/listening acceptance')
    pins = all_pins['native']
    compiler = pins['compiler']['path']
    python_pin = all_pins['host']['python']
    assert_pinned(python_pin['path'], python_pin['sha256'])
    if Path(sys.executable).resolve() != Path(python_pin['path']).resolve() or sys.version != python_pin['version']:
        raise ValueError('Python executable/version does not match supplied pins')
    if set(pins['runtime_inputs']) != set(RUNTIME_NAMES):
        raise ValueError('Exactly seven native runtime pins are required')
    assert_pinned(compiler, pins['compiler']['sha256'])
    manifest.update({'compiler': compiler, 'compiler_sha256': pins['compiler']['sha256'],
                     'compiler_version': pins['compiler']['version'], 'runtime_inputs': {},
                     'runtime_paths': {}, 'targets': {}, 'flags': NATIVE_FLAGS,
                     'python': python_pin,
                     'helper_inputs': [file_record(Path(__file__)), file_record(Path(__file__).with_name('provenance_common.py'))]})
    runtime_paths, external_used = {}, {}
    commands = []
    command_path = args.out / 'compile-commands.json'
    manifest_path = args.out / 'manifest.json'
    started = time.monotonic()

    def persist():
        write_json(command_path, {'commands': commands})
        manifest['compile_commands'] = str(command_path)
        manifest['compile_commands_sha256'] = digest(command_path)
        manifest['compile_commands_bytes'] = command_path.stat().st_size
        manifest['elapsed_s'] = time.monotonic() - started
        write_json(manifest_path, manifest)

    def run(argv, label):
        entry = {'label': label, 'argv': argv, 'directory': str(args.source)}
        manifest['commands'].append(entry)
        result = subprocess.run(argv, cwd=args.source, capture_output=True, text=True, timeout=180)
        path = args.out / (label + '.log')
        path.write_text(result.stdout + result.stderr)
        entry.update({'returncode': result.returncode, 'log': file_record(path)})
        persist()
        if result.returncode:
            raise subprocess.CalledProcessError(result.returncode, argv, result.stdout, result.stderr)
        return result.stdout

    def verify_toolchain():
        assert_pinned(compiler, pins['compiler']['sha256'])
        for name, path in runtime_paths.items():
            assert_pinned(path, pins['runtime_inputs'][name])

    try:
        version = run([compiler, '--version'], 'compiler-version').splitlines()[0]
        if version != pins['compiler']['version']:
            raise ValueError('Pinned native compiler version changed')
        target_help = run([compiler, '--help=target'], 'compiler-target-help')
        if '-fbbb=' not in target_help:
            raise ValueError('Pinned compiler does not support required safety flag')
        for name in RUNTIME_NAMES:
            resolved = Path(run([compiler, '-m68000', '-msoft-float', '-mcrt=nix20', '-print-file-name=' + name],
                                'runtime-' + name.replace('.', '-')).strip()).resolve()
            assert_pinned(resolved, pins['runtime_inputs'][name])
            runtime_paths[name] = resolved
            manifest['runtime_paths'][name] = str(resolved)
            manifest['runtime_inputs'][name] = pins['runtime_inputs'][name]
        plans, recipe = read_canonical_recipe(args.source, all_pins['canonical_recipe']['sha256'])
        manifest['canonical_recipe'] = recipe
        declared, declaration_inputs = python_declarations(args.source)
        manifest['python_declaration_inputs'] = declaration_inputs
        manifest['recipe_comparison'] = {}
        for target in TARGETS:
            canonical, comparison = plans[target], declared[target]
            intentional_extras = []
            actual_extras = sorted(set(canonical) - set(comparison))
            if actual_extras != intentional_extras:
                raise ValueError('Unexpected canonical extras vs host declaration: ' + target)
            manifest['recipe_comparison'][target] = {
                'canonical_ordered_translation_units': canonical,
                'python_declared_ordered_translation_units': comparison,
                'same_order': canonical == comparison,
                'additional_host_declared_units': sorted(set(comparison) - set(canonical)),
                'intentional_canonical_extra_units': intentional_extras,
                'policy': 'Build uses committed canonical recipe order unchanged; no canonical extras versus the pinned host declarations are accepted.'}
        persist()
        for target in TARGETS:
            verify_toolchain()
            units = plans[target]
            deps, sdk = {}, {}
            for index, unit in enumerate(units):
                dependency_command = [compiler, *NATIVE_FLAGS, '-M', unit]
                output = run(dependency_command, target + '-dependencies-%02d' % index)
                for name in parse_dependencies(output):
                    path = (args.source / name).resolve()
                    category, key = classify(path, args.source, expected, pins['allowed_sdk_roots'])
                    if category == 'canonical':
                        deps[key] = digest(path)
                    elif category == 'external_sdk':
                        sdk[key] = digest(path)
                        external_used[key] = digest(path)
                    else:
                        raise ValueError('Unexpected generated dependency in native fixture')
            wrapper = units[0]
            preprocessed = run([compiler, *NATIVE_FLAGS, '-E', '-dD', wrapper], target + '-preprocessed-wrapper')
            definitions = re.findall(r'^#define assert\([^\n]*', preprocessed, re.MULTILINE)
            last_assert = definitions[-1] if definitions else None
            if target in {'PTExecInvertSessionTest', 'PTExecSamplerInvertSongTest', 'PTExecEditorInvertStudioTest'}:
                if not last_assert or 'test_failure' not in last_assert or 'exit(20)' not in re.sub(r'\s+', '', preprocessed):
                    raise ValueError('Native EFx RC20 assert macro no longer retained: ' + target)
            binary = args.out / target
            argv = [compiler, *NATIVE_FLAGS, *units, '-o', str(binary)]
            command = {'name': target, 'directory': str(args.source), 'argv': argv,
                       'output': str(binary), 'ordered_translation_units': units}
            commands.append(command)
            run(argv, target + '.build')
            raw = binary.read_bytes()
            if len(raw) < 4 or struct.unpack('>I', raw[:4])[0] != 0x3f3:
                raise ValueError('Native linked output lacks HUNK_HEADER: ' + target)
            manifest['targets'][target] = {
                'scope': 'Exec Fast-pool/TypeOfMem and bounded MOD file workspace fixture; shared output-preservation assertions',
                'flags': NATIVE_FLAGS, 'source_inputs': units,
                'dependencies': dict(sorted(deps.items())), 'external_sdk_dependencies': dict(sorted(sdk.items())),
                'binary_bytes': len(raw), 'binary_sha256': digest(binary), 'format': 'HUNK_HEADER0x000003f3',
                'preprocessed_wrapper': file_record(args.out / (target + '-preprocessed-wrapper.log')),
                'last_assert_macro': last_assert, 'build_log': file_record(args.out / (target + '.build.log'))}
            persist()
            print(target, len(raw), digest(binary), flush=True)
        manifest['passed'] = len(manifest['targets']) == len(TARGETS)
    except Exception as failure:
        manifest['failure'] = {'kind': type(failure).__name__, 'message': str(failure), 'traceback': traceback.format_exc()}
        manifest['passed'] = False
    finally:
        try:
            verify_toolchain()
            for name, sha in external_used.items():
                assert_pinned(name, sha)
            manifest['source_files_after'] = len(require_snapshot(args.source, expected))
            assert_pinned(args.source_manifest, args.source_manifest_sha256)
            assert_pinned(args.pins, args.pins_sha256)
            assert_pinned(python_pin['path'], python_pin['sha256'])
            for entry in manifest['helper_inputs']:
                assert_pinned(entry['path'], entry['sha256'])
        except Exception as failure:
            manifest['final_provenance_error'] = {'kind': type(failure).__name__, 'message': str(failure)}
            manifest['passed'] = False
        persist()
    print(json.dumps({'passed': manifest['passed'], 'targets': list(manifest['targets']), 'manifest': str(manifest_path)}))
    return 0 if manifest['passed'] else 1

if __name__ == '__main__':
    sys.exit(main())
