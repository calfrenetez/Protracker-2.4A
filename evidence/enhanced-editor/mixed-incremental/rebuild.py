#!/usr/bin/env python3
"""Print the exact recorded commands; rebuild only with --build-output NEW_DIR.

Uses the frozen exported sources, not the current repository or Git index.
The native wrapper includes each fixture C file. Those fixture files are
dependency hashes only, never additional compiler translation units.
"""
import argparse
import hashlib
import json
import shlex
import subprocess
from pathlib import Path

BASE = Path(__file__).resolve().parent
COMMANDS_SHA256 = '2ad059a374e8407eb40842cc491eb3b442fc50b3db5646a68e7bf6ab78303977'
FIXTURES = {'tests/mixed_preflight_test.c', 'tests/mixed_owner_test.c',
            'tests/editor_mixed_test.c'}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require_hash(path, expected):
    if digest(path) != expected:
        raise SystemExit('Hash mismatch: ' + str(path))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, help='Relocated frozen source export')
    parser.add_argument('--check', action='store_true',
                        help='Validate source/compiler hashes without building')
    parser.add_argument('--build-output', type=Path,
                        help='Explicitly rebuild all three into a new directory')
    args = parser.parse_args()
    commands_path = BASE / 'compile-commands.json'
    require_hash(commands_path, COMMANDS_SHA256)
    recovered = json.loads(commands_path.read_text())
    manifest_path = BASE / 'build.json'
    require_hash(manifest_path, recovered['build_manifest_sha256'])
    manifest = json.loads(manifest_path.read_text())
    source = (args.source or Path(manifest['source_export'])).resolve()
    for command in recovered['commands']:
        ordered = command['ordered_sources']
        if len(ordered) != len(set(ordered)) or FIXTURES.intersection(ordered):
            raise SystemExit('Duplicate/included fixture translation unit')
        if command['argv'][1:-2] != manifest['flags'] + ordered:
            raise SystemExit('Recorded argv/source ordering mismatch')
    if args.check or args.build_output:
        compiler = Path(recovered['commands'][0]['argv'][0])
        require_hash(compiler, manifest['compiler_sha256'])
        checked = set()
        for binary in manifest['binaries'].values():
            for name, expected in binary['inputs'].items():
                if name not in checked:
                    require_hash(source / name, expected)
                    checked.add(name)
        print('Source/compiler hashes and wrapper exclusions verified.')
    if not args.build_output:
        if not args.check:
            for command in recovered['commands']:
                print('cd ' + shlex.quote(str(source)))
                print(shlex.join(command['argv']))
        return
    # Recheck the CRT archives selected by the recorded compiler before linking.
    for name, expected in manifest['runtime_inputs'].items():
        path = Path(subprocess.check_output(
            [str(compiler), '-m68000', '-msoft-float', '-mcrt=nix20',
             '-print-file-name=' + name], text=True).strip())
        require_hash(path, expected)
    output = args.build_output.resolve()
    if output == BASE:
        raise SystemExit('Use a fresh output directory; preserve original candidates.')
    output.mkdir(parents=True, exist_ok=False)
    for command in recovered['commands']:
        argv = command['argv'][:-1] + [str(output / command['name'])]
        subprocess.run(argv, cwd=source, check=True)
        binary = output / command['name']
        print(command['name'], binary.stat().st_size, digest(binary))


if __name__ == '__main__':
    main()
