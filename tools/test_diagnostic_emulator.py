#!/usr/bin/env python3
"""Run bounded CLI diagnostics in the existing isolated ProTracker guest.

Requires local/environment.json and its privately copied disk/startup hook.
Never controls another project's emulator. Restores the launch script afterward.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import socket
import subprocess
import time
from emulator_ipc import Emulator

ROOT = Path(__file__).resolve().parents[1]


def matching_socket():
    matches = []
    for path in Path('/tmp').glob('amiberry*.sock'):
        try:
            # Short read-only probe so an unrelated unresponsive instance
            # cannot hold up our deadline. The Emulator constructor rechecks.
            with socket.socket(socket.AF_UNIX) as s:
                s.settimeout(.2)
                s.connect(str(path))
                s.sendall(b'GET_STATUS\n')
                status = s.recv(4096).decode()
                if 'Config=ProTracker isolated baseline' in status.strip().split('\t'):
                    matches.append(str(path))
        except OSError:
            pass
    return matches


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--environment', type=Path, default=ROOT / 'local/environment.json')
    args = p.parse_args()
    env = json.loads(args.environment.read_text())
    if matching_socket():
        raise SystemExit('An isolated ProTracker emulator is already running; close it first')
    share = Path(env['share'])
    launch = share / 'launch'
    original = launch.read_bytes()
    nonce = 'diag' + str(time.time_ns())
    out = share / nonce
    out.mkdir()
    binary = ROOT / 'build/diagnostic/AmiGUSTest'
    shutil.copyfile(binary, out / 'AmiGUSTest')
    cases = [('discover', '--discover', 5), ('ownership', '--ownership', 5),
             ('invalid', '--invalid', 20)]
    cases += [('repeat' + str(i), '--discover', 5) for i in range(10)]
    script = ['FailAt 21', 'Wait 5', 'Stack 65536', 'CD PTDEV:' + nonce]
    for name, mode, _ in cases:
        script += [f'AmiGUSTest {mode} >{name}.log', f'Echo $RC >{name}.rc']
    script += [f'Echo "{nonce}" >done']
    process = None
    emu = None
    try:
        launch.write_text('\n'.join(script) + '\n')
        with (out / 'emulator.log').open('wb') as log:
            process = subprocess.Popen([env['emulator_binary'], '--config', env['profile'],
                                        '-G', '-m', 'PTDEV:' + str(share), '--log'],
                                       stdin=subprocess.DEVNULL, stdout=log,
                                       stderr=subprocess.STDOUT, start_new_session=True)
        deadline = time.monotonic() + 100
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError('Emulator exited before test completion')
            if not emu:
                matches = matching_socket()
                if len(matches) > 1:
                    raise RuntimeError('Ambiguous ProTracker emulator instances')
                if matches:
                    emu = Emulator(matches[0])
            if (out / 'done').exists():
                break
            time.sleep(.5)
        else:
            raise RuntimeError('Guest diagnostic deadline expired')
        if (out / 'done').read_text().strip() != nonce:
            raise RuntimeError('Guest run identity mismatch')
        results = []
        for name, mode, rc in cases:
            actual = int((out / (name + '.rc')).read_text().strip())
            log = (out / (name + '.log')).read_text()
            if actual != rc:
                raise RuntimeError(f'{name}: expected rc={rc}, got {actual}: {log}')
            if rc == 5 and 'SUMMARY result=SKIP reason=library-unavailable cards=0' not in log:
                raise RuntimeError('Expected missing-library SKIP, not a hardware pass')
            if rc == 20 and not log.startswith('usage: AmiGUSTest'):
                raise RuntimeError('Invalid command was not rejected')
            results.append({'case': name, 'mode': mode, 'return_code': actual,
                            'log': log, 'expectation_met': True})
        evidence = {'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
                    'scope': 'CLI/absent-library only; no physical AmiGUS acceptance',
                    'run_id': nonce, 'cases': results,
                    'emulator': {cmd: emu.command(cmd) for cmd in
                                 ['GET_VERSION', 'GET_STATUS', 'GET_CPU_MODEL', 'GET_MEMORY_CONFIG']}}
        report = ROOT / 'build/diagnostic/emulator.json'
        report.write_text(json.dumps(evidence, indent=2) + '\n')
        print(f'PASS: {len(cases)} native CLI executions; {report}')
    finally:
        try:
            if emu and process and process.poll() is None:
                # Revalidate the pinned socket immediately before shutdown.
                Emulator(emu.path).command('QUIT')
        finally:
            if process and process.poll() is None:
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.terminate()  # Only the process created above.
            launch.write_bytes(original)


if __name__ == '__main__':
    main()
