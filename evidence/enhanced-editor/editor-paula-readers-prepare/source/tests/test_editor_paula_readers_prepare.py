"""Exact committed-source plus four new overlays; standalone host evidence only."""
from pathlib import Path
import ast, hashlib, io, json, subprocess, sys, tarfile, tempfile, time
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT.parents[1] / 'outputs/editor-paula-readers-prepare/v1'
OWN = ['src/editor/editor_paula_readers_prepare.h', 'src/editor/editor_paula_readers_prepare.c',
       'tests/editor_paula_readers_prepare_test.c', 'tests/test_editor_paula_readers_prepare.py']
BASELINE = '86cb65fdef27a19082a4d469acee8aa3bb39e0bb'
def constants(folder, path, key):
    values = {}
    for node in ast.parse((folder / path).read_text()).body:
        if isinstance(node, ast.Assign):
            try: value = ast.literal_eval(node.value)
            except (ValueError, TypeError): continue
            for target in node.targets:
                if isinstance(target, ast.Name): values[target.id] = value
        elif isinstance(node, ast.AugAssign) and isinstance(node.target, ast.Name) and isinstance(node.op, ast.Add):
            values[node.target.id] += ast.literal_eval(node.value)
    return values[key]
def fingerprint(path):
    return {'bytes': path.stat().st_size, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
def main():
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    attempt = Path(tempfile.mkdtemp(prefix='attempt-', dir=EVIDENCE))
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    if head != BASELINE: raise RuntimeError('unexpected baseline HEAD')
    index = subprocess.check_output(['git', 'ls-files', '--stage'], cwd=ROOT)
    raw = subprocess.check_output(['git', 'archive', head, 'src', 'tests'], cwd=ROOT)
    folder = attempt / 'source'; folder.mkdir()
    with tarfile.open(fileobj=io.BytesIO(raw)) as ar: ar.extractall(folder, filter='data')
    for path in OWN:
        target = folder / path; target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes((ROOT / path).read_bytes())
    font = subprocess.check_output(['git', 'show', head + ':vendor/pt23f/raw/ptfont.raw'], cwd=ROOT)
    (folder / 'pt_font.h').write_text('static const unsigned char pt_font[580] = {' + ','.join(str(b) for b in font) + '};\n')
    sampler = constants(folder, 'tests/test_sampler.py', 'SOURCES')[1:]
    readers = ['src/editor/sampler_paula.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c',
               'src/core/scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/core/paula_render_voice.c', *sampler,
               'src/editor/paula_preflight.c', *['src/core/' + n + '.c' for n in
               ['render', 'pitch', 'timeline', 'frame_clock', 'flow', 'voice']]]
    sources = list(dict.fromkeys(constants(folder, 'tests/test_editor.py', 'SOURCES')[1:] +
        constants(folder, 'tests/test_editor_studio.py', 'EXTRA') + constants(folder, 'tests/test_wavetable_dispatch.py', 'DISPATCH') +
        ['src/editor/sampler_wavetable.c'] + ['src/core/' + n + '.c' for n in
        ['amigus_reservation', 'amigus_wavetable_cache', 'amigus_sample_ram', 'sample_cache', 'playback_pcm']] +
        ['src/editor/editor_wavetable.c', 'src/platform/sample_import.c', 'src/platform/raw_import.c',
         'src/platform/mod_import.c', 'src/platform/pp20_import.c']))
    preflight = ['src/editor/mixed_preflight.c', *constants(folder, 'tests/test_paula_preflight.py', 'SOURCES')[1:],
                 *constants(folder, 'tests/test_wavetable_dispatch.py', 'DISPATCH'), 'src/editor/sampler_wavetable.c',
                 *['src/core/' + n + '.c' for n in ['amigus_reservation', 'amigus_wavetable_cache', 'amigus_sample_ram', 'sample_cache', 'playback_pcm']], *sampler]
    mixed = ['src/editor/mixed_owner.c', 'src/editor/mixed_transport.c', 'src/editor/paula_voices.c',
             'src/editor/sampler_paula.c', 'src/editor/paula_dispatch.c', *preflight]
    establish = ['src/editor/editor_mixed.c', 'src/editor/editor_mixed_establish.c', 'src/editor/sampler_establish.c',
                 'src/editor/sampler_prepare_project.c', 'src/editor/sampler_prepare_memory.c']
    flags = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-Isrc/core', '-I.']
    report = {'status': 'PREPARED_NOT_RUN', 'head': head, 'scope': 'HOST_ONLY ASan/UBSan; no native/physical/device/timing/PLAY acceptance',
              'source': str(folder), 'inputs': {str(p.relative_to(folder)): fingerprint(p) for p in sorted(folder.rglob('*')) if p.is_file()},
              'calls': [], 'attempt': str(attempt)}
    report_path = attempt / 'run.json'
    def call(label, argv, timeout):
        report['calls'].append({'label': label, 'argv': argv, 'status': 'ATTEMPTED', 'timeout_seconds': timeout})
        report_path.write_text(json.dumps(report, indent=2) + '\n'); started = time.monotonic()
        try: result = subprocess.run(argv, cwd=folder, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
        except Exception as error:
            report['calls'][-1].update(status='FAILED_EXCEPTION', error=repr(error), elapsed_seconds=time.monotonic()-started); raise
        (attempt / (label + '.stdout')).write_bytes(result.stdout); (attempt / (label + '.stderr')).write_bytes(result.stderr)
        report['calls'][-1].update(status='PASS' if result.returncode == 0 else 'FAIL', returncode=result.returncode,
                                 elapsed_seconds=time.monotonic()-started)
        report_path.write_text(json.dumps(report, indent=2) + '\n')
        if result.returncode: raise RuntimeError(label + ' failed; first logs preserved')
    try:
        groups = [('controller', 'tests/editor_paula_readers_prepare_test.c', ['src/editor/editor_paula_readers_prepare.c', *readers, *establish, *mixed, *sources]),
                  ('workspace', 'tests/paula_readers_song_workspace_test.c', readers),
                  ('establish', 'tests/editor_mixed_establish_test.c', [*establish, *mixed, *sources])]
        for label, fixture, body in groups:
            exe = attempt / label
            call(label + '-compile', ['cc', *flags, fixture, *list(dict.fromkeys(body)), '-o', str(exe)], 120)
            call(label + '-run', [str(exe)], 180)
        if subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip() != head: raise RuntimeError('HEAD changed')
        if subprocess.check_output(['git', 'ls-files', '--stage'], cwd=ROOT) != index: raise RuntimeError('index changed')
        report['status'] = 'PASS'
    except Exception as error: report['status'] = 'FAIL'; report['error'] = repr(error); raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n'); print(str(report_path), flush=True)
if __name__ == '__main__': main()
