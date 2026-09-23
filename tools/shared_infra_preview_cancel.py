#!/usr/bin/env python3
"""Interactive preview cancellation; reserve shared030 before invocation.

Uses existing shared Guest guards/lock. No lifecycle, configuration or hardware
operations. Build PT24GEdit and generate preview-cancel.ptg before running.
"""
import argparse
import fcntl
import hashlib
import json
from pathlib import Path
import shutil
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
INFRA = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', type=Path, default=ROOT / 'build/dev/PT24GEdit')
    args = parser.parse_args()
    sys.path.insert(0, str(INFRA / 'scripts'))
    from shared_guest import Guest
    out = ROOT / 'build/dev' / ('preview-cancel-ui-' + str(time.time_ns()))
    out.mkdir()
    with (INFRA / 'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        guest = Guest(INFRA, out)
        run = guest.share / out.name
        run.mkdir()
        result = {'passed': False, 'scope': 'interactive emulator preview cancellation', 'guest_directory': str(run)}
        launched = False
        deadline = time.monotonic() + 120
        def log():
            p = run / 'editor.log'
            return p.read_text(errors='replace') if p.exists() else ''
        def wait(predicate):
            while not predicate():
                if time.monotonic() > deadline:
                    raise RuntimeError('Interactive preview deadline: ' + log()[-500:])
                time.sleep(.02)
        def tap(raw, control=False):
            if control: guest.command('SEND_KEY', 0x63, 1)
            try: guest.tap(raw)
            finally:
                if control: guest.command('SEND_KEY', 0x63, 0)
        try:
            for src, name in [('PT24GEdit', 'PT24GEdit'), ('preview-cancel.ptg', 'input.ptg')]:
                shutil.copyfile(args.editor if name == 'PT24GEdit' else ROOT / 'build/dev' / src, run / name)
                result[name + '_sha256'] = hashlib.sha256((run / name).read_bytes()).hexdigest()
            guest.launch.write_text('\n'.join(['FailAt 21', 'Stack 65536', 'CD ' + guest.device + run.name,
                 'If EXISTS ENV:PT24G_RECENT_PREFIX',
                'Copy ENV:PT24G_RECENT_PREFIX old-recent-env', 'EndIf',
                'SetEnv PT24G_RECENT_PREFIX ' + guest.device + run.name + '/recent',
                'PT24GEdit input.ptg saved.ptg >editor.log', 'Echo $RC >editor.rc',
                'If EXISTS old-recent-env', 'Copy old-recent-env ENV:PT24G_RECENT_PREFIX',
                'Else', 'Delete ENV:PT24G_RECENT_PREFIX', 'EndIf',
                'If EXISTS ENV:PT24G_RECENT_PREFIX', 'Copy ENV:PT24G_RECENT_PREFIX restored-recent-env',
                'EndIf', 'Echo done >done' ]) + '\n')
            launched = True
            guest.start()
            wait(lambda: 'status=READY -' in log())
            tap(0x28, True)  # Sampler panel.
            wait(lambda: 'panel=5' in log())
            time.sleep(.8)
            guest.command('SCREENSHOT', out / 'before.png')
            offset = len(log())
            tap(0x57)  # F8 sample audition.
            wait(lambda: 'EDITOR CONVERSION progress=' in log()[offset:])
            tap(0x45)  # Escape is handled inside the modal conversion callback.
            wait(lambda: 'PREVIEW CANCELLED' in log()[offset:])
            assert 'EDITOR CONVERSION cancelled=' in log()[offset:]
            result['audio_after_cancel'] = guest.command('GET_AUDIO_STATE')
            assert all('ch%d_dma=0' % i in result['audio_after_cancel'].split('\t') for i in range(4))
            assert 'revision=0 dirty=0 status=PREVIEW CANCELLED' in log()[offset:]
            time.sleep(.8)
            guest.command('SCREENSHOT', out / 'cancelled.png')
            tap(0x21, True)
            wait(lambda: 'PROJECT SAVED' in log())
            assert (run / 'saved.ptg').read_bytes() == (run / 'input.ptg').read_bytes()
            result['exact_master_project_preserved'] = True
            assert 'prefix=' + guest.device + run.name + '/recent' in log()
            result['isolated_recents'] = True
            tap(0x45)
            if not (run / 'editor.rc').exists(): tap(0x45)
            wait(lambda: (run / 'done').exists())
            assert (run / 'editor.rc').read_text().strip() == '0'
            previous, restored = run / 'old-recent-env', run / 'restored-recent-env'
            assert previous.exists() == restored.exists()
            if previous.exists(): assert previous.read_bytes() == restored.read_bytes()
            result['recent_environment_restored'] = True
            assert 'EDITOR EXIT clean' in log()
            result['passed'] = True
        except Exception as exc:
            result['error'] = str(exc)
            raise
        finally:
            # Bounded normal cancellation/exit attempt only while our editor is
            # still pending. Never reboot or kill an unrelated guest process.
            if launched and not (run / 'editor.rc').exists() and 'EDITOR FRAME' in log():
                for _ in range(3):
                    if (run / 'editor.rc').exists(): break
                    tap(0x45)
                    time.sleep(.5)
            (out / 'editor.log').write_text(log())
            # The launcher restores the previous environment before marking done.
            end = time.monotonic() + 5
            while (run / 'editor.rc').exists() and not (run / 'done').exists() and time.monotonic() < end:
                time.sleep(.1)
            finished = (run / 'done').exists()
            if finished:
                result['returncode'] = (run / 'editor.rc').read_text().strip()
                result['audio_after_exit'] = guest.command('GET_AUDIO_STATE')
                previous, restored = run / 'old-recent-env', run / 'restored-recent-env'
                restored_ok = previous.exists() == restored.exists() and (not previous.exists() or previous.read_bytes() == restored.read_bytes())
                result['recent_environment_restored'] = restored_ok
                if restored_ok and all('ch%d_dma=0' % i in result['audio_after_exit'].split('\t') for i in range(4)):
                    guest.launch.unlink()
                    shutil.rmtree(run)
                    result['run_files_cleaned'] = True
            (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
            print(out)

if __name__ == '__main__':
    main()
