#!/usr/bin/env python3
"""Build only the opt-in RAM entry diagnostic; never execute its product.

Compilation, local stack annotations and linking do not clear task/IRQ stack,
native execution, CIA timing, source quiet, audio or application PLAY. The caller
must review the exact candidate and separately coordinate any target operation.
Every command and first failure is retained in a fresh external output folder.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
import sys
import traceback

TARGET = 'PTExecReadersRamEntryTest'
SOURCES = [
    'src/diagnostic/native_ram_entry.c',
    'src/native/readers_ram/native_ram_port.c',
    'src/native/readers_ram/native_ram_irq_dispatch.c',
    'src/native/readers_ram/native_cia_ram_adapter.c',
    'src/native/native_checked_memory.c',
    'src/core/elapsed_clock.c',
    'src/core/scheduled_readers.c',
    'src/core/readers_activation.c',
    'src/native/readers_ram/native_ram_irq.s',
]
FLAGS = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
         '-Wall', '-Wextra', '-Werror', '-Isrc/core', '-Isrc/native',
         '-Isrc/native/readers_ram', '-Ibuild/dev', '-fbbb=-']
RUNTIMES = ['ncrt0.o', 'libnix20.a', 'libnixmain.a', 'libnix.a',
            'libstubs.a', 'libamiga.a', 'libgcc.a']


def record(path):
    p = Path(path)
    b = p.read_bytes()
    return {'path': str(p), 'resolved_path': str(p.resolve()), 'bytes': len(b),
            'mode': p.stat().st_mode & 0o777, 'sha256': hashlib.sha256(b).hexdigest()}


def lexical(path):
    """Reject dangling symlinks before resolve can hide them."""
    p = Path(os.path.abspath(path))
    for part in [p, *p.parents]:
        if part.is_symlink():
            raise ValueError('Symlink path component: ' + str(part))
    return p


def below(path, root):
    return path == root or root in path.parents


def write_json(path, value):
    p = path.with_suffix(path.suffix + '.writing')
    p.write_text(json.dumps(value, indent=2) + '\n')
    p.replace(path)


def dependency_paths(text, root):
    text = re.sub(r'\\\r?\n', ' ', text)
    colon = re.search(r'(?<!\\):', text)
    if colon is None:
        raise ValueError('Missing dependency target separator')
    names = shlex.split(text[colon.end():])
    return list(dict.fromkeys(Path(n) if Path(n).is_absolute() else root / n
                              for n in names))


def quoted_closure(root):
    """Snapshot project-local quoted includes without importing build recipes."""
    pending = [root / p for p in SOURCES]
    found = {}
    search = [root / 'src/core', root / 'src/native', root / 'src/native/readers_ram']
    while pending:
        p = lexical(pending.pop())
        name = str(p.relative_to(root))
        if name in found:
            continue
        found[name] = record(p)
        text = p.read_text()
        if p.suffix == '.s':
            if re.search(r'^\s*(?:\.include|\.incbin|#\s*include)\b', text, re.M):
                raise ValueError('Assembly has an untracked external include')
            continue
        for inc in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"', text, re.M):
            match = next((q for q in [p.parent / inc, *[d / inc for d in search]]
                          if q.is_file()), None)
            if match is None or not below(lexical(match), root):
                raise ValueError('Unresolved project quoted include: ' + inc)
            pending.append(match)
    return dict(sorted(found.items()))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cc', default=os.environ.get('AMIGA_CC', 'm68k-amigaos-gcc'))
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    root = lexical(Path(__file__).absolute().parents[1])
    out = lexical(args.out)
    if out.exists() or below(out, root) or below(root, out):
        raise ValueError('Output must be fresh and outside the source tree')
    cc_name = shutil.which(args.cc)
    if cc_name is None:
        raise ValueError('Compiler executable not found: ' + args.cc)
    cc = str(Path(cc_name).resolve())
    source_before = quoted_closure(root)
    out.mkdir(parents=True, exist_ok=False)
    manifest = {'status': 'RUNNING_COMPILER_ONLY', 'target': TARGET,
                'source_root': str(root), 'output_root': str(out), 'flags': FLAGS,
                'ordered_units': SOURCES, 'source_before': source_before,
                'builder': record(__file__), 'tools': {'compiler': record(cc)}, 'runtime_inputs': {},
                'commands': [], 'units': [], 'native_run': 'NOT_RUN',
                'IRQ_run': 'NOT_RUN', 'task_stack_total': 'UNKNOWN',
                'system_IRQ_stack_total': 'UNKNOWN', 'launcher65536': 'NOT_CLEARED'}
    checkpoint = out / 'manifest.json'
    env = dict(os.environ, LC_ALL='C')

    def run(argv, label):
        i = len(manifest['commands'])
        stdout, stderr = out / ('%02d-%s.stdout' % (i, label)), out / ('%02d-%s.stderr' % (i, label))
        row = {'argv': argv, 'cwd': str(root), 'state': 'STARTED',
               'returncode': None, 'stdout_path': str(stdout), 'stderr_path': str(stderr)}
        manifest['commands'].append(row)
        write_json(checkpoint, manifest)
        with stdout.open('wb') as o, stderr.open('wb') as e:
            result = subprocess.run(argv, cwd=root, env=env, stdout=o, stderr=e)
        row.update({'state': 'COMPLETED', 'returncode': result.returncode,
                    'stdout': record(stdout), 'stderr': record(stderr)})
        write_json(checkpoint, manifest)
        if result.returncode != 0:
            raise RuntimeError('First failed command retained: ' + label)
        return stdout.read_text()

    try:
        run([cc, '--version'], 'compiler-version')
        for tool in ['cc1', 'as', 'ld', 'collect2']:
            value = run([cc, '-print-prog-name=' + tool], 'tool-' + tool).strip()
            path = value if Path(value).is_absolute() else shutil.which(value)
            if not path or not Path(path).is_file():
                raise ValueError('Compiler tool path unresolved: ' + tool)
            manifest['tools'][tool] = record(Path(path).resolve())
        for name in RUNTIMES:
            value = run([cc, '-m68000', '-msoft-float', '-mcrt=nix20',
                         '-print-file-name=' + name], 'runtime-' + name).strip()
            p = Path(value)
            if not p.is_absolute() or not p.is_file():
                raise ValueError('Runtime path unresolved: ' + name)
            manifest['runtime_inputs'][name] = record(p)
        objects = []
        for i, unit in enumerate(SOURCES):
            obj, dep = out / ('tu%02d.o' % i), out / ('tu%02d.d' % i)
            argv = [cc, *FLAGS, '-fstack-usage', '-MD', '-MF', str(dep),
                    '-c', unit, '-o', str(obj)]
            run(argv, 'compile-tu%02d' % i)
            row = {'source': unit, 'object': record(obj), 'compile_argv': argv}
            if unit.endswith('.c'):
                row['dependency_file'] = record(dep)
                row['dependencies'] = [record(p) for p in dependency_paths(dep.read_text(), root)]
                su = obj.with_suffix('.su')
                row['local_stack_usage'] = record(su)
            else:
                # Plain GAS .s has no preprocessor headers. Its include/incbin
                # prohibition is checked in the project closure before compiling.
                row['dependencies'] = [record(root / unit)]
                row['dependency_file'] = record(dep) if dep.is_file() else None
                row['local_stack_usage'] = None
            manifest['units'].append(row)
            objects.append(str(obj))
            write_json(checkpoint, manifest)
        product, link_map = out / TARGET, out / (TARGET + '.map')
        run([cc, *FLAGS, *objects, '-Wl,-Map,' + str(link_map), '-o', str(product)], 'link')
        manifest['product'] = record(product)
        manifest['link_map'] = record(link_map)
        if product.read_bytes()[:4] != struct.pack('>I', 0x3f3):
            raise ValueError('Product is not an executable HUNK_HEADER')
        if quoted_closure(root) != source_before:
            raise ValueError('Project source changed during build')
        for row in manifest['units']:
            for old in row['dependencies']:
                if record(old['path']) != old:
                    raise ValueError('Recorded dependency changed during build')
        for old in [manifest['builder'], *manifest['tools'].values(), *manifest['runtime_inputs'].values()]:
            if record(old['path']) != old:
                raise ValueError('Recorded tool/runtime changed during build')
        manifest['source_after'] = quoted_closure(root)
        manifest['status'] = 'PASS_COMPILE_LINK_ONLY_PRODUCT_NEVER_EXECUTED'
        return 0
    except BaseException as error:
        manifest['status'] = 'FAILED_FIRST_OUTCOME_RETAINED_NO_RETRY'
        manifest['failure'] = str(error)
        manifest['traceback'] = traceback.format_exc()
        return 1
    finally:
        # Retain partial object/dependency/stack/map/product files on any failure.
        manifest['retained_files'] = {p.name: record(p) for p in sorted(out.iterdir())
                                      if p.is_file() and p.name not in
                                      ('manifest.json', 'manifest.json.writing')}
        write_json(checkpoint, manifest)


if __name__ == '__main__':
    sys.exit(main())
