"""Preserved HOST_ONLY attempts from c4cbc04 plus four own and two root overlays."""
from pathlib import Path
import ast, hashlib, io, json, subprocess, sys, tarfile, tempfile, time
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
BASELINE = 'c4cbc0419a920e70ed4db1194373a75a6b1a871e'
EVIDENCE = ROOT.parents[1] / 'outputs/mixed-readers-activation/v1'
OWN = ['src/core/mixed_readers_activation.h', 'src/core/mixed_readers_activation.c',
       'tests/mixed_readers_activation_test.c', 'tests/test_mixed_readers_activation.py']
ROOT_OVERLAYS = {
    'src/core/mixed_scheduled_readers.c': '6c0e5b7851056b0cdb72cdad0c408b15efdc6ba21d1357609edfa3fb9cd15de4',
    'src/core/mixed_scheduled_readers.h': 'da39231a26d4629b6707ace68a58ddb3b6e38a1be354e2700d28f4a79f37aed5',
}
PROTECTED = ['AGENTS.md', 'amiga-test.json', 'src/editor/editor.c', 'src/editor/editor.h',
             'src/editor/view.c', 'src/editor/view.h', 'src/native/editor_main.c',
             'src/native/pattern_display.c', 'src/native/pattern_display.h', 'tests/editor_test.c',
             'tests/native_prepared_test.c', 'tests/test_editor.py', 'tools/build_core_tests.py',
             'tools/test_sampler_emulator.py', 'tools/shared_infra_sampler.py', 'tools/shared_infra_ui.py']
def fingerprint(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(), 'mode': path.stat().st_mode & 0o777}
def constants(folder, name, key):
    for node in ast.parse((folder / name).read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == key for t in node.targets):
            return ast.literal_eval(node.value)
    raise RuntimeError('missing source constant')
def main():
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    attempt = Path(tempfile.mkdtemp(prefix='attempt-', dir=EVIDENCE))
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    if head != BASELINE: raise RuntimeError('unexpected HEAD')
    index = subprocess.check_output(['git', 'ls-files', '--stage'], cwd=ROOT)
    protected = {p: fingerprint(ROOT / p) for p in PROTECTED}
    overlay = {p: fingerprint(ROOT / p) for p in [*OWN, *ROOT_OVERLAYS]}
    if any(overlay[p]['sha256'] != sha for p, sha in ROOT_OVERLAYS.items()): raise RuntimeError('root guard overlays changed')
    raw = subprocess.check_output(['git', 'archive', head, 'src', 'tests'], cwd=ROOT)
    source = attempt / 'source'; source.mkdir()
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive: archive.extractall(source, filter='data')
    for name in [*OWN, *ROOT_OVERLAYS]:
        path = source / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes((ROOT / name).read_bytes())
    font = subprocess.check_output(['git', 'show', head + ':vendor/pt23f/raw/ptfont.raw'], cwd=ROOT)
    (source / 'pt_font.h').write_text('static const unsigned char pt_font[580] = {' + ','.join(str(b) for b in font) + '};\n')
    sampler = constants(source, 'tests/test_sampler.py', 'SOURCES')[1:]
    body = list(dict.fromkeys(['src/core/mixed_readers_activation.c', 'src/core/mixed_scheduled_readers.c',
           'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c',
           'src/core/amigus_voice_plan.c', *sampler, *['src/core/' + n + '.c' for n in
           ['amigus_reservation', 'amigus_wavetable_cache', 'amigus_sample_ram', 'sample_cache', 'playback_pcm']]]))
    flags = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-Isrc/core', '-I.']
    report = {'status': 'PREPARED_NOT_RUN', 'head': head, 'scope': 'HOST_ONLY injected paired RAM model; no timer/native/physical/device/IRQ/stack/timing/audio qualification',
              'attempt': str(attempt), 'inputs': {str(p.relative_to(source)): fingerprint(p) for p in sorted(source.rglob('*')) if p.is_file()},
              'protected': protected, 'overlays': overlay, 'calls': []}
    record = attempt / 'run.json'
    def call(label, argv, timeout):
        row = {'label': label, 'argv': argv, 'timeout_seconds': timeout, 'status': 'ATTEMPTED'}
        report['calls'].append(row); record.write_text(json.dumps(report, indent=2) + '\n')
        start = time.monotonic()
        try: result = subprocess.run(argv, cwd=source, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
        except Exception as error:
            row.update(status='FAILED_EXCEPTION', error=repr(error), elapsed_seconds=time.monotonic()-start); raise
        (attempt / (label + '.stdout')).write_bytes(result.stdout); (attempt / (label + '.stderr')).write_bytes(result.stderr)
        row.update(status='PASS' if result.returncode == 0 and not result.stderr else 'FAIL', returncode=result.returncode, elapsed_seconds=time.monotonic()-start)
        record.write_text(json.dumps(report, indent=2) + '\n')
        if result.returncode or result.stderr: raise RuntimeError(label + ' first failure preserved')
    try:
        groups = [('activation', 'tests/mixed_readers_activation_test.c', body),
                  ('paired', 'tests/mixed_scheduled_readers_test.c', [p for p in body if p != 'src/core/mixed_readers_activation.c']),
                  ('abi2', 'tests/scheduled_readers_test.c', ['src/core/elapsed_clock.c'])]
        for label, fixture, units in groups:
            exe = attempt / label
            call(label + '-compile', ['cc', *flags, fixture, *units, '-o', str(exe)], 120)
            call(label + '-run', [str(exe)], 180)
        if subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip() != head: raise RuntimeError('HEAD changed')
        if subprocess.check_output(['git', 'ls-files', '--stage'], cwd=ROOT) != index: raise RuntimeError('index changed')
        if {p: fingerprint(ROOT / p) for p in PROTECTED} != protected: raise RuntimeError('protected paths changed')
        if {p: fingerprint(ROOT / p) for p in [*OWN, *ROOT_OVERLAYS]} != overlay: raise RuntimeError('controlled overlays changed')
        report['status'] = 'PASS'
    except Exception as error:
        report['status'] = 'FAIL'; report['error'] = repr(error); raise
    finally:
        record.write_text(json.dumps(report, indent=2) + '\n'); print(record, flush=True)
if __name__ == '__main__': main()
