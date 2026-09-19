#!/usr/bin/env python3
"""Run native core tests, then leave the private development tracker for UI tests.

Requires an explicitly coordinated exclusive Amiberry window. Close the guarded
instance after UI checks; the host launch script is restored on every exit.
"""
import json
from pathlib import Path
import shutil
import subprocess
import time
from build_diagnostic import digest, ROOT
from test_diagnostic_emulator import matching_socket
from emulator_ipc import Emulator


def main():
    if matching_socket():
        raise SystemExit('ProTracker emulator already running')
    env = json.loads((ROOT / 'local/environment.json').read_text())
    share = Path(env['share'])
    launch = share / 'launch'
    original = launch.read_bytes()
    nonce = 'dev' + str(time.time_ns())
    run = share / nonce
    run.mkdir()
    programs = ['PTChannelsTest', 'PTPcmTest', 'PTGuardTest']
    for name in programs + ['PT2.4G', 'PT.HELP', 'PTInputProbe']:
        shutil.copyfile(ROOT / 'build/dev' / name, run / name)
    shutil.copyfile(ROOT / 'evidence/baseline/mod.baseline', run / 'mod.good')
    # Short, safe names for selecting only from the disposable emulator directory.
    for old, new in [('mod.invalid-256-patterns', 'mod.bad'),
                     ('mod.invalid-short-sample', 'mod.short')]:
        shutil.copyfile(ROOT / 'build/mod-corpus' / old, run / new)
    lines = ['FailAt 21', 'Wait 5', 'Stack 65536', 'CD PTDEV:' + nonce]
    for name in programs:
        lines += [f'{name} >{name}.log', f'Echo $RC >{name}.rc']
    lines += ['PT2.4G', 'Wait 3', 'PTInputProbe >PTInputProbe.log', 'Echo $RC >PTInputProbe.rc', f'Echo "{nonce}" >done']
    process = None
    emu = None
    success = False
    try:
        launch.write_text('\n'.join(lines) + '\n')
        with (run / 'emulator.log').open('wb') as log:
            process = subprocess.Popen([env['emulator_binary'], '--config', env['profile'],
                                        '-G', '-m', 'PTDEV:' + str(share), '--log'],
                                       stdin=subprocess.DEVNULL, stdout=log,
                                       stderr=subprocess.STDOUT, start_new_session=True)
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError('Emulator exited')
            if not emu:
                matches = matching_socket()
                if len(matches) > 1: raise RuntimeError('Ambiguous test emulator')
                if matches: emu = Emulator(matches[0])
            if (run / 'done').exists(): break
            time.sleep(.5)
        else: raise RuntimeError('Native suite deadline expired')
        if (run / 'done').read_text().strip() != nonce: raise RuntimeError('Wrong run nonce')
        cases = []
        for name in programs + ['PTInputProbe']:
            rc = int((run / (name + '.rc')).read_text().strip())
            log = (run / (name + '.log')).read_text()
            passed = rc == 0 and 'PASS' in log and 'FAIL' not in log
            if name == 'PTGuardTest': passed = passed and log.count('result=PASS') == 15
            cases.append({'name': name, 'rc': rc, 'log': log, 'pass': passed,
                          'binary_sha256': digest(run / name)})
        report = {'run_id': nonce, 'cases': cases,
                  'environment': {cmd: emu.command(cmd) for cmd in
                                  ['GET_VERSION', 'GET_STATUS', 'GET_CPU_MODEL', 'GET_MEMORY_CONFIG']}}
        (ROOT / 'build/dev/native-tests.json').write_text(json.dumps(report, indent=2) + '\n')
        if not all(case['pass'] for case in cases):
            raise RuntimeError('Native test failed: inspect build/dev/native-tests.json')
        state = {'pid': process.pid, 'socket': emu.path, 'run_directory': str(run),
                 'tracker_sha256': digest(run / 'PT2.4G')}
        (ROOT / 'local/dev-emulator.json').write_text(json.dumps(state, indent=2) + '\n')
        success = True
        print('PASS: native channel, PCM/WAV, 15 MOD guard cases and input.device probe. Tracker left running for UI checks.')
        print(json.dumps(state, indent=2))
    finally:
        launch.write_bytes(original)
        if not success and process and process.poll() is None:
            try:
                if emu: Emulator(emu.path).command('QUIT')
                else: process.terminate()
            finally:
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired: process.terminate()


if __name__ == '__main__':
    main()
