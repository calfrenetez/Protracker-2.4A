"""Private shared provenance helpers. Import performs no build/test operation."""
import argparse
import ast
import hashlib
import json
import re
import shlex
import tempfile
from pathlib import Path

RUNTIME_NAMES = ['ncrt0.o', 'libnix20.a', 'libnixmain.a', 'libnix.a',
                 'libstubs.a', 'libamiga.a', 'libgcc.a']
TARGETS = ['PTExecModStreamTest']
NATIVE_FLAGS = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
                '-Wall', '-Wextra', '-Werror', '-Isrc/core', '-Ibuild/dev', '-fbbb=-']
VALUE_OPTIONS = {'-I', '-D', '-U', '-include', '-imacros', '-isystem', '-isysroot',
                 '-iquote', '-idirafter', '-target', '--sysroot', '-x', '-arch', '-Xclang'}

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def file_record(path):
    path = Path(path)
    return {'path': str(path), 'bytes': path.stat().st_size, 'sha256': digest(path)}

def write_json(path, value):
    path = Path(path)
    staging = path.with_suffix(path.suffix + '.writing')
    staging.write_text(json.dumps(value, indent=2) + '\n')
    staging.replace(path)

def below(path, root):
    try:
        Path(path).resolve().relative_to(Path(root).resolve())
        return True
    except ValueError:
        return False

def expected_files(manifest):
    values = manifest.get('source', manifest.get('files', manifest.get('export_file_hashes')))
    if isinstance(values, dict):
        return {name: row if isinstance(row, str) else row['sha256'] for name, row in values.items()}
    if 'source_inputs' in manifest:
        return {row['path']: row['sha256'] for row in manifest['source_inputs']}
    raise ValueError('Source manifest has no complete recognized file inventory')

def snapshot(source):
    source = Path(source)
    return {path.relative_to(source).as_posix(): digest(path)
            for path in sorted(source.rglob('*')) if path.is_file()}

def require_snapshot(source, expected):
    actual = snapshot(source)
    if actual != expected:
        extra = sorted(set(actual) - set(expected))
        changed = sorted(name for name, sha in expected.items() if actual.get(name) != sha)
        raise ValueError('Immutable source inventory mismatch: extra=%r changed/missing=%r' % (extra, changed))
    return actual

def assert_pinned(path, sha):
    if not Path(path).is_file() or digest(path) != sha:
        raise ValueError('Pinned input changed or absent: ' + str(path))

def parse_dependencies(output):
    """Parse compiler make syntax after joining all escaped continuation lines."""
    joined = re.sub(r'\\\r?\n', ' ', output)
    result = []
    for line in joined.splitlines():
        colon = re.search(r'(?<!\\):', line)
        if colon:
            result.extend(shlex.split(line[colon.end():], posix=True))
    return list(dict.fromkeys(result))

def cpp_arguments(argv):
    """Keep exact preprocessor defines/options; drop link inputs, outputs and -c."""
    result, units = [], []
    position = 1
    while position < len(argv):
        argument = argv[position]
        if argument == '-o':
            position += 2
            continue
        if argument in {'-c', '-shared', '-static'} or argument.startswith(('-Wl,', '-l', '-L')):
            position += 1
            continue
        if argument in VALUE_OPTIONS:
            if position + 1 == len(argv):
                raise ValueError('Compiler option lacks value: ' + argument)
            result.extend(argv[position:position + 2])
            position += 2
            continue
        if argument.endswith(('.c', '.cc', '.cpp', '.s', '.S')):
            units.append(argument)
        elif argument.endswith(('.o', '.a', '.dylib', '.so')):
            pass
        elif argument.startswith('-'):
            result.append(argument)
        else:
            raise ValueError('Unclassified compiler argument: ' + argument)
        position += 1
    if not units:
        raise ValueError('No translation units in compiler call')
    return result, units

def compiler_input_arguments(argv):
    result, position = [], 1
    while position < len(argv):
        argument = argv[position]
        if argument == '-o' or argument in VALUE_OPTIONS:
            position += 2
            continue
        if argument.endswith(('.c', '.cc', '.cpp', '.s', '.S', '.o', '.a')):
            result.append(argument)
        position += 1
    return result

def classify(path, source, expected, sdk_roots, generated=None):
    path = Path(path).resolve()
    if below(path, source):
        relative = path.relative_to(Path(source).resolve()).as_posix()
        if relative not in expected or digest(path) != expected[relative]:
            raise ValueError('Canonical dependency absent/changed: ' + relative)
        return 'canonical', relative
    if generated and (str(path) in generated or any(below(path, root) for root in generated.values())):
        return 'generated', str(path)
    if any(below(path, root) for root in sdk_roots):
        return 'external_sdk', str(path)
    raise ValueError('Unrecognized external dependency: ' + str(path))

def arguments(description):
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--source-manifest', type=Path, required=True)
    parser.add_argument('--source-manifest-sha256', required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--pins', type=Path, required=True)
    parser.add_argument('--pins-sha256', required=True)
    args = parser.parse_args()
    args.source = args.source.resolve()
    args.source_manifest = args.source_manifest.resolve()
    args.out = args.out.resolve()
    args.pins = args.pins.resolve()
    if not args.source.is_dir() or below(args.out, args.source) or not any(
            below(args.out, root) for root in [Path('/private/tmp'), Path(tempfile.gettempdir())]):
        parser.error('Output must be outside a valid immutable source directory')
    assert_pinned(args.source_manifest, args.source_manifest_sha256)
    assert_pinned(args.pins, args.pins_sha256)
    args.out.mkdir(parents=True, exist_ok=True)
    if any(args.out.iterdir()):
        parser.error('Output directory must be new/empty to preserve prior evidence')
    return args

def setup(args, scope):
    manifest = json.loads(args.source_manifest.read_text())
    expected = expected_files(manifest)
    pins = json.loads(args.pins.read_text())
    inputs = require_snapshot(args.source, expected)
    record = {'scope': scope, 'source_export': str(args.source),
              'source_manifest': file_record(args.source_manifest), 'pins': file_record(args.pins),
              'base_commit': manifest.get('base_commit', manifest.get('baseline_commit')),
              'base_tree': manifest.get('base_tree', manifest.get('baseline_tree')),
              'source_files_before': len(inputs),
              'export_tree_sha256': hashlib.sha256(json.dumps(inputs, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
              'commands': [], 'passed': False}
    return expected, pins, record

def read_canonical_recipe(source, expected_sha):
    """Evaluate only pure list/dict declarations, never invoke builder main/imports."""
    recipe = Path(source) / 'tools/build_core_tests.py'
    assert_pinned(recipe, expected_sha)
    tree = ast.parse(recipe.read_text(), filename=str(recipe))
    main = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == 'main')
    flags_node = next(node.value for node in main.body if isinstance(node, ast.Assign)
                      and any(isinstance(target, ast.Name) and target.id == 'flags' for target in node.targets))
    if not isinstance(flags_node, ast.List) or len(flags_node.elts) != len(NATIVE_FLAGS):
        raise ValueError('Canonical native compiler flags declaration changed')
    literals, safety = flags_node.elts[:-1], flags_node.elts[-1]
    if [node.value if isinstance(node, ast.Constant) else None for node in literals] != NATIVE_FLAGS[:-1] or not (
            isinstance(safety, ast.Starred) and isinstance(safety.value, ast.Call)
            and isinstance(safety.value.func, ast.Name) and safety.value.func.id == 'compiler_safety_flags'
            and len(safety.value.args) == 1 and isinstance(safety.value.args[0], ast.Name)
            and safety.value.args[0].id == 'cc' and not safety.value.keywords):
        raise ValueError('Canonical native compiler flags/order no longer match pinned recipe')
    namespace = {'__builtins__': {'dict': dict}}
    selected = []
    def is_inputs(target):
        return isinstance(target, ast.Subscript) and isinstance(target.value, ast.Name) and target.value.id == 'inputs'
    def declaration(node):
        if isinstance(node, ast.Assign):
            return all(is_inputs(target) or (isinstance(target, ast.Name) and target.id in {'inputs', 'render_sources', 'invert_sources'}) for target in node.targets)
        if isinstance(node, ast.AugAssign):
            return is_inputs(node.target)
        if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call):
            call = node.value
            return isinstance(call.func, ast.Attribute) and call.func.attr == 'append' and is_inputs(call.func.value)
        return False
    for node in main.body:
        if isinstance(node, ast.If) and ast.unparse(node.test) == 'args.target':
            break
        if declaration(node):
            selected.append(node)
        elif isinstance(node, ast.For) and node.body and all(declaration(child) for child in node.body):
            selected.append(node)
    for node in selected:
        for child in ast.walk(node):
            if isinstance(child, ast.Call):
                function = child.func
                allowed = isinstance(function, ast.Attribute) and (
                    (isinstance(function.value, ast.Name) and function.value.id == 'dict' and function.attr == 'fromkeys') or
                    (is_inputs(function.value) and function.attr == 'append') or
                    (isinstance(function.value, ast.Name) and function.value.id == 'name' and function.attr == 'replace'
                     and all(isinstance(value, ast.Constant) and isinstance(value.value, str) for value in child.args)
                     and not child.keywords))
                if not allowed:
                    raise ValueError('Non-pure call in recipe declaration: ' + ast.unparse(child))
    pure = ast.Module(body=selected, type_ignores=[])
    exec(compile(pure, str(recipe) + ':source-list-declarations-only', 'exec'), namespace, namespace)
    plans = {target: namespace['inputs'][target] for target in TARGETS}
    for target, units in plans.items():
        expected_wrapper = 'tests/native_exec_mod_stream_test.c'
        if len(units) != len(set(units)) or units[0] != expected_wrapper:
            raise ValueError('Duplicate units or wrong native wrapper: ' + target)
        if any(not (Path(source) / unit).is_file() for unit in units):
            raise ValueError('Missing canonical recipe unit: ' + target)
    return plans, file_record(recipe)
