#!/usr/bin/env python3
"""Coordinated RAM-only probes, including qualified disabled Mini PCM MMIO.

No playback enable or interrupt installation; each write mode requires its
exact native qualification, confirmed reset/release and idle AHI restoration.
"""
import argparse
import asyncio
import fcntl
import hashlib
import json
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INFRA = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
IDLE_DRIVER_SHA256 = '3ad6ec0b598ebbb215121a72a3945f3360f64963df133d53d45a65b144c84c27'
IDLE_DRIVER_ID = '$VER: AmiGUS.audio 4.023 (30.8.26) 020 SAS/C cross'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def qualification(binary, build, emulator, independent, ownership=False, idle_driver=False, registers=False, reset=False, fifo=False):
    """Refuse physical selection before matching the current exact candidate."""
    if registers and not idle_driver:
        raise RuntimeError('Register qualification requires an idle-driver window')
    built = json.loads(build.read_text())
    native = json.loads(emulator.read_text())
    if fifo and (not reset or native.get('amigus-fifo_returncode') != '5'):
        raise RuntimeError('Exact disabled FIFO qualification missing')
    if reset and (not registers or native.get('amigus-reset_returncode') != '5'):
        raise RuntimeError('Exact silent reset qualification missing')
    cleanup = json.loads(independent.read_text())
    actual = digest(binary)
    returns_pass = (all(native.get(name+'_returncode') == '5' for name in ('amigus-discover','amigus-ownership')) and all(native.get(name+'_returncode') == '0' for name in ('ownership-fixture','native-abi'))) if ownership else native.get('amigus-discovery_returncode') == '0'
    if (built.get('binary_sha256') != actual or
            built.get('binary_bytes') != binary.stat().st_size or
            native.get('AmiGUSTest_sha256' if ownership else 'PTAmiGusDiscovery_sha256') != actual or
            native.get('passed') is not True or
            not returns_pass or
            native.get('run_files_cleaned') is not True):
        raise RuntimeError('Exact discovery bytes have not passed emulator qualification')
    expected = {str(INFRA / 'runtime/Dev/Tests' / emulator.parent.name): False,
                str(INFRA / 'runtime/Dev/Tests' / ('launch-' + emulator.parent.name)): False}
    if cleanup.get('passed') is not True or cleanup.get('paths') != expected:
        raise RuntimeError('Independent exact discovery cleanup has not passed')
    if ([x for x in cleanup.get('status', '').split('\t') if x.startswith('Paused=')] != ['Paused=false'] or
            'model=68030' not in cleanup.get('cpu', '').split('\t') or
            not all('ch%d_dma=0' % i in cleanup.get('audio', '').split('\t') for i in range(4))):
        raise RuntimeError('Independent guest identity/audio verification incomplete')
    for name, expected_digest in built['inputs'].items():
        committed = subprocess.check_output(['git', 'show', 'HEAD:' + name], cwd=ROOT)
        if digest(ROOT / name) != expected_digest or hashlib.sha256(committed).hexdigest() != expected_digest:
            raise RuntimeError('Discovery source input differs from current committed candidate: ' + name)
    lock = ROOT / 'amigus-sdk.lock.json'
    if digest(lock) != built['sdk_lock_sha256']:
        raise RuntimeError('SDK lock changed')
    for name, expected_digest in json.loads(lock.read_text())['files'].items():
        if digest(ROOT / 'vendor/amigus-sdk' / name) != expected_digest:
            raise RuntimeError('SDK header changed')
    if idle_driver:
        if not ownership or native.get('amigus-idle_returncode') != '5':
            raise RuntimeError('Exact idle-driver mode unavailable-library check missing')
        fixtures = [('PTDriverWindowTest', 'driver-window-build', 'driver-window'),
                    ('PTDriverExecTest', 'driver-exec-build', 'driver-exec')]
        if registers:
            if native.get('amigus-registers_returncode') != '5':
                raise RuntimeError('Exact register mode unavailable-library check missing')
            fixtures.append(('PTPcmReadTest', 'pcm-read-build', 'pcm-read'))
        for tool, stem, case in fixtures:
            manifest = json.loads((ROOT / 'build/diagnostic' / (stem + '.json')).read_text())
            fixture = ROOT / 'build/diagnostic' / tool
            if (native.get(case + '_returncode') != '0' or
                    native.get(tool + '_sha256') != digest(fixture) or
                    manifest['binary_sha256'] != digest(fixture)):
                raise RuntimeError('Idle-driver native guard/Exec fixture not qualified')
            for name, expected_digest in manifest['inputs'].items():
                committed = subprocess.check_output(['git', 'show', 'HEAD:' + name], cwd=ROOT)
                if digest(ROOT / name) != expected_digest or hashlib.sha256(committed).hexdigest() != expected_digest:
                    raise RuntimeError('Idle-driver fixture source changed: ' + name)
    return actual


def idle_driver_info(text, require_idle=True):
    """Native code repeats these public guards under Exec serialization."""
    if (not re.search(r'^\s*Version:\s+4\.23\s*$', text, re.M) or
            not re.search(r'^\s*ID string:\s+' + re.escape(IDLE_DRIVER_ID) + r'\s*$', text, re.M) or
            (require_idle and not re.search(r'^\s*Open count:\s+0\s*$', text, re.M))):
        raise RuntimeError('AmiGUS.audio is active or outside the verified idle-driver contract')


def idle_restoration_finished(output, registers=False, reset=False, fifo=False):
    label = 'IDLE-FIFO' if fifo else 'IDLE-RESET' if reset else 'IDLE-REGISTERS' if registers else 'IDLE-OWNERSHIP'
    matches = re.findall(r'^'+label+r' result=(PASS|SKIP|FAIL) unloaded=([01]) restored=([01]) restore_needed=([01]) rc=(0|5|20)$', output, re.M)
    if len(matches) != 1:
        return False
    status, unloaded, restored, pending, rc = matches[0]
    return (pending == '0' and rc == {'PASS':'0','SKIP':'5','FAIL':'20'}[status] and
            (unloaded == '0' or restored == '1') and
            (status != 'PASS' or unloaded == restored == '1'))


async def run(out, binary, result, ownership=False, library_contract=None, idle_driver=None, registers=False, reset=False, fifo=False):
    if fifo and not reset:
        raise RuntimeError('Disabled FIFO probe requires confirmed reset path')
    if reset and not registers:
        raise RuntimeError('Silent reset requires register qualification')
    if registers and (not ownership or not idle_driver):
        raise RuntimeError('Register probe requires the qualified idle-driver transaction')
    sys.path.insert(0, str(INFRA / 'scripts'))
    from amiga import connect
    from bridge_checks import target, require_reply, checksum
    from shared_guest import Guest
    from mcp import ClientSession
    from mcp.client.streamable_http import streamable_http_client

    if binary.name not in ('PTAmiGusDiscovery', 'AmiGUSTest'):
        raise RuntimeError('Unsupported diagnostic executable name')
    guest = Guest(INFRA, out)
    status = guest.command('GET_STATUS')
    audio = guest.command('GET_AUDIO_STATE')
    if [x for x in status.split('\t') if x.startswith('Paused=')] != ['Paused=false'] or not all('ch%d_dma=0' % i in audio.split('\t') for i in range(4)):
        raise RuntimeError('Shared emulator is not idle/running before target selection')
    result.update(emulator_environment=guest.env, emulator_before=status, audio_before=audio)
    result['target_switch_attempted'] = True
    await connect('real-a1200')
    result['physical_target'] = target(INFRA, 'real-a1200')
    destination = 'RAM:ptg-discovery-' + out.name
    result['guest_directory'] = destination
    result['calls'] = []

    def record():
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')

    async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd, wr, _):
        async with ClientSession(rd, wr) as session:
            await session.initialize()

            async def call(name, arguments):
                target(INFRA, 'real-a1200')
                if name == 'amiga_run_script':
                    # Uncertain execution retains both target and exact paths.
                    result['script_pending'] = True
                record()
                reply = await session.call_tool(name, arguments)
                text = '\n'.join(x.text for x in reply.content if hasattr(x, 'text'))
                result['calls'].append(dict(tool=name, arguments=arguments, result=text))
                record()
                require_reply(reply)
                if name == 'amiga_run_script':
                    if '[OK]' not in text:
                        raise RuntimeError('Script completion acknowledgement absent')
                    result['script_pending'] = False
                record()
                return text

            try:
                identity = await call('amiga_run_script', dict(script='Version\nCPU\nAvail\nVersion bsdsocket.library\nShowNetStatus\nEcho PTG-IDENTITY-DONE\n', timeout=20))
                if '68030' not in identity or "192.168.0.156 (on interface 'plipbox')" not in identity or 'PTG-IDENTITY-DONE' not in identity.splitlines():
                    raise RuntimeError('Physical CPU/network identity did not match')
                result['identity'] = identity
                bridge = INFRA / 'runtime/physical-staging/bridge-20260923'
                hardware = json.loads((INFRA / 'config/hardware.json').read_text())
                if digest(bridge) != hardware['bridge_sha256']:
                    raise RuntimeError('Qualified bridge snapshot changed')
                checksum(await call('amiga_checksum', dict(path=hardware['bridge_path'])), bridge.read_bytes())
                result['bridge_sha256'] = digest(bridge)
                if ownership:
                    if not library_contract:
                        raise RuntimeError('Pinned ownership driver contract required')
                    installed = await call('amiga_checksum', dict(path='LIBS:amigus.library'))
                    matched = False
                    for row in library_contract['libraries']:
                        snapshot = Path(row['path'])
                        if digest(snapshot) != row['sha256']:
                            raise RuntimeError('Pinned driver distribution snapshot changed')
                        try:
                            checksum(installed, snapshot.read_bytes())
                        except RuntimeError:
                            continue
                        result['installed_library_sha256'] = row['sha256']
                        matched = True
                        break
                    if not matched:
                        raise RuntimeError('Installed driver is outside the verified NULL-owner probe contract')
                if idle_driver:
                    snapshot = Path(idle_driver['path'])
                    if idle_driver.get('sha256') != IDLE_DRIVER_SHA256 or digest(snapshot) != IDLE_DRIVER_SHA256:
                        raise RuntimeError('Pinned RC6 AHI driver snapshot changed')
                    checksum(await call('amiga_checksum', dict(path='DEVS:AHI/AmiGUS.audio')), snapshot.read_bytes())
                    info = await call('amiga_lib_info', dict(name='AmiGUS.audio'))
                    idle_driver_info(info)
                    ahi = await call('amiga_dev_info', dict(name='ahi.device'))
                    if not re.search(r'^\s*Open count:\s+0\s*$', ahi, re.M):
                        raise RuntimeError('AHI has open users; idle-driver test refused')
                    result.update(idle_driver_before=info, ahi_before=ahi, idle_driver_sha256=IDLE_DRIVER_SHA256)
                made = await call('amiga_run_script', dict(script=f'MakeDir {destination}\nIf WARN\n Quit 20\nEndIf\nEcho PTG-DIRECTORY-CREATED\n', timeout=20))
                if 'PTG-DIRECTORY-CREATED' not in made.splitlines():
                    raise RuntimeError('Fresh RAM directory was not reserved')
                result['directory_created'] = True
                remote = destination + '/' + binary.name
                await call('amiga_push_file', dict(local_path=str(binary), amiga_path=remote))
                checksum(await call('amiga_checksum', dict(path=remote)), binary.read_bytes())
                command = binary.name + (' --idle-fifo' if fifo else ' --idle-reset' if reset else ' --idle-registers' if registers else ' --idle-ownership' if idle_driver else ' --ownership' if ownership else '')
                if idle_driver:
                    result['restoration_pending'] = True
                execution = await call('amiga_run_script', dict(script=f'FailAt 21\nStack 65536\nCD {destination}\n{command} >discovery.log\nEcho PTG-DISCOVERY-RC $RC\nCD RAM:\nEcho PTG-DISCOVERY-DONE\n', timeout=45))
                if 'PTG-DISCOVERY-DONE' not in execution.splitlines():
                    result['script_pending'] = True
                    raise RuntimeError('Discovery execution did not reach its completion marker')
                result['execution_finished'] = True
                await call('amiga_pull_file', dict(amiga_path=destination + '/discovery.log', local_path=str(out / 'discovery.log')))
                output = (out / 'discovery.log').read_text(errors='replace')
                if idle_driver:
                    # Known termination is not sufficient: positively verify the
                    # restoration obligation before any deletion/target release.
                    if not idle_restoration_finished(output, registers, reset, fifo):
                        raise RuntimeError('Idle-driver restoration state unconfirmed; retain target/files')
                    info = await call('amiga_lib_info', dict(name='AmiGUS.audio'))
                    idle_driver_info(info, require_idle=False)
                    checksum(await call('amiga_checksum', dict(path='DEVS:AHI/AmiGUS.audio')), snapshot.read_bytes())
                    result.update(idle_driver_after=info, driver_restoration_verified=True)
                    result['restoration_pending'] = False
                marker = 'REGISTERS result=PASS reason=idle rc=0' if registers else 'SUMMARY result=PASS reason=complete' if ownership else 'AMIGUS DISCOVERY PASS:'
                if 'PTG-DISCOVERY-RC 0' not in execution.splitlines() or marker not in output:
                    raise RuntimeError('Physical discovery test failed')
                result['discovery_passed'] = True
                if registers:
                    if 'REGISTER RELEASE confirmed=1 retained=0 driver=0x00000000' not in output:
                        raise RuntimeError('Register probe final release confirmation missing')
                    result['register_probe_passed'] = True
                    if reset:
                        if not re.search(r'^PCM QUIESCE requested=1 confirmed=1 status=1 polls=[123]$', output, re.M):
                            raise RuntimeError('Silent reset readback confirmation missing')
                        result['silent_reset_passed'] = True
                    if fifo:
                        if not all(f'PCM FIFO long={i} pending_words={i*2}' in output.splitlines() for i in range(1,4)) or 'PCM FIFO result=PASS pending_words=6 capacity=NOT_TESTED ordering=NOT_TESTED' not in output.splitlines():
                            raise RuntimeError('Disabled FIFO word counts unconfirmed')
                        result['disabled_fifo_passed'] = True
                elif ownership:
                    if not all(f'block={i} result=PASS stage=complete driver=0x00000000 release=0x00000000 confirmed=1 retained=0' in output for i in (1,2)):
                        raise RuntimeError('PCM/wavetable final-release confirmations incomplete')
                    result['ownership_passed'] = True
                else:
                    result['card_detected'] = all(f'pass={i} status=1 library=1 cards=1 pcm_cards=1 closed=1' in output for i in range(2))
            finally:
                held = result.get('script_pending') or result.get('restoration_pending')
                if result.get('directory_created') and not held:
                    clean = await call('amiga_run_script', dict(script=f'CD RAM:\nIf EXISTS {destination}/{binary.name}\n Delete {destination}/{binary.name}\n If WARN\n  Quit 20\n EndIf\nEndIf\nIf EXISTS {destination}/discovery.log\n Delete {destination}/discovery.log\n If WARN\n  Quit 20\n EndIf\nEndIf\nDelete {destination}\nIf WARN\n Quit 20\nEndIf\nIf EXISTS {destination}\n Quit 20\nEndIf\nEcho PTG-CLEANUP-ABSENT\n', timeout=20))
                    if 'PTG-CLEANUP-ABSENT' not in clean.splitlines():
                        raise RuntimeError('Exact physical cleanup not confirmed')
                    result['run_files_cleaned'] = True
                    separate = await call('amiga_run_script', dict(script=f'If EXISTS {destination}\n Quit 20\nEndIf\nEcho PTG-INDEPENDENT-ABSENCE\n', timeout=20))
                    if 'PTG-INDEPENDENT-ABSENCE' not in separate.splitlines():
                        raise RuntimeError('Independent physical absence check failed')
                    result['independent_cleanup_passed'] = True
                if not held:
                    await connect('amiberry-030')
                    result['returned_target'] = target(INFRA, 'amiberry-030')
                    final = Guest(INFRA, out)
                    result['audio_after'] = final.command('GET_AUDIO_STATE')
                    if not all('ch%d_dma=0' % i in result['audio_after'].split('\t') for i in range(4)):
                        raise RuntimeError('Emulator audio state changed')
                    result['devbench_returned'] = True
                else:
                    result['recovery_hold'] = True
                record()
    result['passed'] = bool(result.get('discovery_passed') and result.get('independent_cleanup_passed') and result.get('devbench_returned'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--emulator-result', type=Path, required=True)
    parser.add_argument('--independent-cleanup', type=Path, required=True)
    parser.add_argument('--ownership', action='store_true')
    parser.add_argument('--library-contract', type=Path)
    parser.add_argument('--idle-driver-contract', type=Path,
                        help='Explicitly approved temporary unused RC6 AHI unload/test/restore transaction')
    parser.add_argument('--registers', action='store_true', help='Read-only Mini PCM status; requires idle driver window')
    parser.add_argument('--reset', action='store_true', help='Silent one-shot disable/reset with bounded readback; requires registers')
    parser.add_argument('--fifo', action='store_true', help='Three zero long stores while disabled, exact count then confirmed reset; requires reset')
    args = parser.parse_args()
    if args.idle_driver_contract and not args.ownership:
        parser.error('idle-driver-contract requires ownership')
    if args.registers and not args.idle_driver_contract:
        parser.error('registers requires idle-driver-contract')
    if args.reset and not args.registers:
        parser.error('reset requires registers')
    if args.fifo and not args.reset:
        parser.error('fifo requires reset')
    out = ROOT / 'build/dev' / (('physical-amigus-ownership-' if args.ownership else 'physical-amigus-discovery-') + str(time.time_ns()))
    out.mkdir()
    result = dict(passed=False, target='real-a1200', scope='RAM-only PCM/wavetable exclusive reservation and confirmed release; no MMIO, interrupts, output or listening acceptance' if args.ownership else 'RAM-only discovery; no reserve/release, MMIO, interrupts, output or listening acceptance')
    try:
        source = ROOT / ('build/diagnostic/AmiGUSTest' if args.ownership else 'build/dev/physical-discovery-candidate/PTAmiGusDiscovery')
        build = ROOT / ('build/diagnostic/build.json' if args.ownership else 'build/dev/amigus-discovery-build.json')
        result['binary_sha256'] = qualification(source, build, args.emulator_result, args.independent_cleanup, args.ownership, bool(args.idle_driver_contract), args.registers, args.reset, args.fifo)
        contract = json.loads(args.library_contract.read_text()) if args.library_contract else None
        if args.ownership and not contract:
            raise RuntimeError('Ownership requires the verified pinned library contract')
        result['emulator_evidence'] = str(args.emulator_result)
        result['independent_emulator_cleanup'] = str(args.independent_cleanup)
        idle = json.loads(args.idle_driver_contract.read_text()) if args.idle_driver_contract else None
        if idle:
            result['scope'] = 'Approved cooperative unused RC6 AHI unload, PCM/wavetable ownership/final release, verified driver restoration; no MMIO/interrupt/audio/config change'
        if args.registers:
            result['scope'] = 'Read-only five Mini PCM status registers under exclusive PCM lease; verified release and unused AHI restoration; no register writes/IRQ/output/config change'
        if args.reset:
            result['scope'] = 'Initially idle Mini PCM: one-shot rate disable/playback IRQ clear/FIFO reset, bounded readback confirmation, verified release and AHI restoration; no data/enable/IRQ/output/config changes'
        if args.fifo:
            result['scope'] = 'Initially idle Mini: three disabled zero-data long stores, exact 2/4/6 word counts, confirmed nonempty FIFO reset/release/AHI restoration; no playback enable/IRQ/capacity/ordering/audio acceptance'
        binary = out / source.name
        shutil.copyfile(source, binary)
        if digest(binary) != result['binary_sha256']:
            raise RuntimeError('Snapshot changed before physical selection')
        with (INFRA / 'runtime/test.lock').open('a') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            asyncio.run(run(out, binary, result, args.ownership, contract, idle, args.registers, args.reset, args.fifo))
    except Exception as error:
        result['passed'] = False
        result['error'] = str(error)
        if result.get('target_switch_attempted') and not result.get('devbench_returned'):
            result['recovery_hold'] = True
        raise
    finally:
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        print(out, flush=True)


if __name__ == '__main__':
    main()
