#!/usr/bin/env python3
"""Native project/conversion tests in an explicitly reserved private emulator.

Uses disposable files, checks exact profile identity, restores the launch script
and quits its own instance. Never starts a tracker or touches the real Amiga.
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
    if matching_socket() or Path('/tmp/amiberry.sock').exists():
        raise SystemExit('An emulator socket exists; acquire an exclusive window first')
    env = json.loads((ROOT / 'local/environment.json').read_text())
    share = Path(env['share']); launch = share / 'launch'; original = launch.read_bytes()
    nonce = 'project' + str(time.time_ns()); run = share / nonce; run.mkdir()
    cases = [
        ('midi-ownership', 'PTMidiTest', '', 0),
        ('record-quantization', 'PTRecordTest', '', 0),
        ('pattern-edit', 'PTPatternTest', '', 0),
        ('slices', 'PTSlicesTest', '', 0),
        ('channels', 'PTChannelsTest', '', 0),
        ('pcm', 'PTPcmTest', '', 0),
        ('file-safety', 'PTFileSafetyTest', '', 0),
        ('project', 'PTProjectTest', 'mixed.ptg', 0),
        ('mod-roundtrip', 'PTModProjectTest', 'mod.good', 0),
        ('document-faults', 'PTDocumentTest', 'mod.good', 0),
        ('write-project', 'PT24GConvert', 'project mod.good song.ptg', 0),
        ('write-mod', 'PT24GConvert', 'mod song.ptg roundtrip.mod', 0),
        ('refuse-existing-project', 'PT24GConvert', 'project mod.good song.ptg', 20),
        ('refuse-existing-mod', 'PT24GConvert', 'mod song.ptg mod.good', 20),
        ('inspect-mixed', 'PT24GConvert', 'inspect mixed.ptg', 0),
        ('refuse-loss', 'PT24GConvert', 'mod mixed.ptg forbidden.mod', 20),
    ]
    for name in {case[1] for case in cases}:
        shutil.copyfile(ROOT / 'build/dev' / name, run / name)
    shutil.copyfile(ROOT / 'evidence/baseline/mod.baseline', run / 'mod.good')
    lines = ['FailAt 21', 'Wait 5', 'Stack 65536', 'CD PTDEV:' + nonce]
    for name, program, args, expected in cases:
        lines += [f'{program} {args} >{name}.log', f'Echo $RC >{name}.rc']
    lines += [f'Echo "{nonce}" >done']
    process = emu = None
    started = time.monotonic()
    try:
        launch.write_text('\n'.join(lines) + '\n')
        with (run / 'emulator.log').open('wb') as log:
            process = subprocess.Popen([env['emulator_binary'], '--config', env['profile'],
                                        '-G', '-m', 'PTDEV:' + str(share), '--log'],
                                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True)
        while time.monotonic() - started < 240:
            if process.poll() is not None: raise RuntimeError('Emulator exited')
            if not emu:
                matches = matching_socket()
                if len(matches) > 1: raise RuntimeError('Ambiguous test emulator')
                if matches: emu = Emulator(matches[0])
            if (run / 'done').exists(): break
            if any('Program aborted' in p.read_text(errors='replace') for p in run.glob('*.log') if p.name != 'emulator.log'):
                raise RuntimeError('Native assertion aborted the script: ' + str(run))
            time.sleep(.5)
        else: raise RuntimeError('Native project-suite deadline expired')
        if (run / 'done').read_text().strip() != nonce: raise RuntimeError('Wrong run nonce')
        results = []
        for name, program, args, expected in cases:
            rc = int((run / (name + '.rc')).read_text().strip())
            log = (run / (name + '.log')).read_text()
            passed = rc == expected
            if program != 'PT24GConvert': passed = passed and 'PASS' in log and 'FAIL' not in log
            results.append({'case': name, 'program': program, 'args': args, 'expected': expected,
                            'actual': rc, 'log': log, 'pass': passed, 'binary_sha256': digest(run / program)})
        roundtrip = (run / 'roundtrip.mod').exists() and (run / 'roundtrip.mod').read_bytes() == (ROOT / 'evidence/baseline/mod.baseline').read_bytes()
        original_intact = (run / 'mod.good').read_bytes() == (ROOT / 'evidence/baseline/mod.baseline').read_bytes()
        no_forbidden = not (run / 'forbidden.mod').exists()
        no_temporary = not list(run.glob('*.pttmp-*'))
        report = {'run_id': nonce, 'elapsed_seconds': round(time.monotonic()-started, 3),
                  'cases': results, 'roundtrip_byte_identity': roundtrip,
                  'original_intact': original_intact, 'lossy_output_not_created': no_forbidden,
                  'temporary_files_cleaned': no_temporary,
                  'environment': {c: emu.command(c) for c in ['GET_VERSION', 'GET_STATUS', 'GET_CPU_MODEL', 'GET_MEMORY_CONFIG']}}
        (ROOT / 'build/dev/project-native-tests.json').write_text(json.dumps(report, indent=2) + '\n')
        for name in ['mixed.ptg', 'song.ptg', 'roundtrip.mod']:
            if (run / name).exists(): shutil.copyfile(run / name, ROOT / 'build/dev' / name)
        if not all(x['pass'] for x in results) or not all([roundtrip, original_intact, no_forbidden, no_temporary]):
            raise RuntimeError('Native project suite failed: inspect build/dev/project-native-tests.json')
        print('PASS: 16 native MIDI/record/project/conversion/file-safety cases; MOD byte identity and existing-file preservation')
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            try:
                if emu: Emulator(emu.path).command('QUIT')
                else: process.terminate()
            finally:
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired: process.terminate()


if __name__ == '__main__':
    main()
