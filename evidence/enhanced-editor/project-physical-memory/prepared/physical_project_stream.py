#!/usr/bin/env python3
"""Prepared ONLY: one exact physical project-stream case after root TAKE.

This uses the shared DevBench, not an independent Safari transport. It acquires
both documented global and dedicated physical locks. It never retries, starts or
stops an emulator, changes hardware/driver settings, or automatically recovers.
Imports of central live APIs are deferred until explicitly approved execution.
"""
import argparse
import asyncio
import fcntl
import gzip
import hashlib
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
INFRA = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
NAME = 'PTExecProjectStreamTest'
SHA = 'd4a3da30620fef5e644ca8dae5a8a48d3483cc1522a50acbbd7b57835c380451'
BYTES = 65656
SCOPE = 'single-physical-project-stream'
TOTAL_SECONDS = 300
NATIVE_SECONDS = 90


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_json(path):
    data = Path(path).read_bytes()
    return json.loads(gzip.decompress(data) if str(path).endswith('.gz') else data)


def prepared_package(expected_sha):
    raw = (HERE / 'prepared-files.json').read_bytes()
    require(digest(raw) == expected_sha, 'Prepared package provenance changed')
    records = json.loads(raw)['files']
    for name, record in records.items():
        require(Path(name).name == name and name not in ('.', '..'), 'Unsafe prepared package member')
        data = (HERE / name).read_bytes()
        require(len(data) == record['bytes'] and digest(data) == record['sha256'], 'Prepared package member changed: ' + name)


def require(value, message):
    if not value:
        raise RuntimeError(message)


def ack(text, marker):
    require(text.splitlines().count(marker) == 1, 'Exact script marker missing/duplicated: ' + marker)


def inventory(text, expected):
    lines = text.splitlines()
    markers = ['PTG-FILES-BEGIN', 'PTG-FILES-END', 'PTG-DIRS-BEGIN', 'PTG-DIRS-END']
    for marker in markers:
        ack(text, marker)
    indexes = [lines.index(marker) for marker in markers]
    require(indexes == sorted(indexes), 'Typed inventory markers reversed')
    names = lines[indexes[0] + 1:indexes[1]]
    directories = lines[indexes[2] + 1:indexes[3]]
    require(not directories, 'Unexpected directory in private physical run; retain: ' + repr(directories))
    require(len(names) == len(expected) and set(names) == set(expected),
            'Unexpected physical child/type/listing output; retain files: ' + repr(names))
    return names


def native_log(data):
    text = data.decode('utf-8', errors='strict')
    lines = [line for line in text.splitlines() if line]
    require(len(lines) == 3, 'Unexpected native output; complete marker proof required')
    match = re.fullmatch(r'EXEC MEMORY pool_limit=(\d+) reserve=262144 flags=4', lines[0])
    require(match is not None and int(match.group(1)) > 65536, 'Fast pool marker invalid')
    require(lines[1] == 'PROJECT STREAM PASS: exact golden layout/CRC, mixed masters/loops/slices/extensions, sink refusal, bounded workspace=7164', 'Project stream marker invalid')
    require(lines[2] == 'EXEC MEMORY PASS: 6 Fast allocations, zero owned bytes, budget refusal without Chip fallback', 'Fast allocator-zero marker invalid')
    return dict(pool_limit=int(match.group(1)), workspace=7164, fast_allocations=6, owned_bytes=0)


def gate_record(path, expected_sha, now=None):
    raw = Path(path).read_bytes()
    require(digest(raw) == expected_sha, 'Root coordination gate hash changed')
    gate = json.loads(raw)
    require(gate.get('scope') == SCOPE and gate.get('target') == 'real-a1200' and
            gate.get('candidate_sha256') == SHA and gate.get('original_pid') == 19081,
            'Root gate scope/target/candidate/process mismatch')
    for field in ('no_conflicting_claims', 'recovery_holds_clear', 'safari_view_only', 'browser_control_not_acquired'):
        require(gate.get(field) is True, 'Root gate missing positive condition: ' + field)
    require(isinstance(gate.get('coordination_take_ack'), str) and bool(gate['coordination_take_ack'].strip()),
            'Fresh actual root-reported TAKE acknowledgment required')
    observed = datetime.fromisoformat(gate['observed_utc'].replace('Z', '+00:00'))
    require(observed.tzinfo is not None, 'Timezone-aware gate timestamp required')
    age = ((now or datetime.now(timezone.utc)) - observed).total_seconds()
    require(0 <= age <= 600, 'Root ownership/Safari gate is stale or future-dated')
    return gate


def host_qualification(plan):
    """File/git reads only; no compiler, Guest, bridge, locks or native execution."""
    for path, record in plan['pinned_files'].items():
        data = Path(path).read_bytes()
        require(len(data) == record['bytes'] and digest(data) == record['sha256'], 'Pinned host input changed: ' + path)
    manifest = read_json(plan['native_manifest'])
    require(manifest.get('passed') is True and set(manifest['targets']) == {NAME}, 'One successful native build required')
    target = manifest['targets'][NAME]
    require(target['binary_bytes'] == BYTES and target['binary_sha256'] == SHA and len(target['dependencies']) == 21,
            'Candidate identity/closure changed')
    tree = subprocess.check_output(['git', 'rev-parse', 'HEAD^{tree}'], cwd=REPO, text=True, timeout=10).strip()
    for path, expected in target['dependencies'].items():
        data = (REPO / path).read_bytes()
        committed = subprocess.check_output(['git', 'show', tree + ':' + path], cwd=REPO, timeout=10)
        require(digest(data) == expected == digest(committed), 'Current committed source changed: ' + path)
    qualification = read_json(plan['emulator_qualification'])
    require(qualification.get('passed') is True and len(qualification['cases']) == 1, 'Exact emulator top PASS missing')
    case = qualification['cases'][0]
    require(case['candidate']['sha256'] == SHA and case['candidate']['bytes'] == BYTES and
            case['result'].get('passed') is True and case['result'].get('project-stream_returncode') == '0' and
            case['result'].get('sample_staging_clean') is True and case['result'].get('run_files_cleaned') is True,
            'Exact emulator native completion/staging missing')
    require(all('ch%d_dma=0' % i in case['result'].get('cleanup_audio', '').split('\t') for i in range(4)), 'Exact emulator cleanup DMA guard missing')
    independent = case['independent_cleanup']
    require(independent.get('passed') is True and len(independent.get('observations', [])) == 11 and
            all(row and not any(row.values()) for row in independent['observations']), 'Independent emulator cleanup missing')
    require(qualification['original_processes'] == plan['original_processes'], 'Original emulator identity mismatch')
    native_log(case['native_log'].encode())
    binary = Path(plan['binary'])
    require(binary.stat().st_size == BYTES and digest(binary.read_bytes()) == SHA, 'Candidate binary changed')
    require(manifest['runtime_inputs'] == plan['runtime_hashes'], 'Runtime manifest mismatch')
    for name, expected in manifest['runtime_inputs'].items():
        require(digest(Path(manifest['runtime_paths'][name]).read_bytes()) == expected, 'Runtime changed: ' + name)
    for path, expected in target['external_sdk_dependencies'].items():
        require(digest(Path(path).read_bytes()) == expected, 'SDK header changed: ' + path)
    require(digest(Path(manifest['compiler']).read_bytes()) == manifest['compiler_sha256'], 'Compiler changed')
    return dict(source_tree=tree, canonical_dependencies=21, sdk_headers=len(target['external_sdk_dependencies']), binary_sha256=SHA, binary_bytes=BYTES)


class Budget:
    def __init__(self, seconds=TOTAL_SECONDS, clock=time.monotonic):
        self.clock = clock
        self.started = clock()
        self.deadline = self.started + seconds

    def remaining(self, minimum=0):
        value = self.deadline - self.clock()
        require(value > minimum, 'Bounded physical window expired/insufficient; no next operation')
        return value

    async def wait(self, operation, limit):
        return await asyncio.wait_for(operation(), timeout=min(limit, self.remaining()))


def list_script(destination):
    return ('Echo PTG-FILES-BEGIN\nList ' + destination + ' FILES NOHEAD LFORMAT "%N"\n'
            'If WARN\n Quit 20\nEndIf\nEcho PTG-FILES-END\n'
            'Echo PTG-DIRS-BEGIN\nList ' + destination + ' DIRS NOHEAD LFORMAT "%N"\n'
            'If WARN\n Quit 20\nEndIf\nEcho PTG-DIRS-END\n')


def absence_script(destination, marker):
    return ('If EXISTS ' + destination + '\n Quit 20\nEndIf\nEcho ' + marker + '\n')


async def transaction(out, binary, backend, result, budget):
    """Dependency-injected transaction; fake backend is used for host tests."""
    destination = 'RAM:ptg-project-' + out.name
    require(re.fullmatch(r'physical-project-\d+', out.name) is not None, 'Unsafe unique run name')
    result.update(guest_directory=destination, calls=[], physical_absence_observations=[], native_launches=0)
    remote = destination + '/' + NAME
    logfile = destination + '/project.log'
    fixture = destination + '/project.ptg'

    def save():
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')

    async def call(name, arguments, limit=25):
        backend.guard_original()
        backend.require_target('real-a1200')
        budget.remaining()
        row = dict(tool=name, arguments=arguments, complete=False)
        result['calls'].append(row)
        result['operation_pending'] = True
        save()
        text = await budget.wait(lambda: backend.call(name, arguments), limit)
        row.update(complete=True, result=text)
        result['operation_pending'] = False
        save()
        backend.guard_original()
        backend.require_target('real-a1200')
        return text

    backend.guard_original()
    backend.require_target('amiberry-030')
    budget.remaining(185)
    result['target_switch_attempted'] = True
    result['recovery_hold'] = True
    save()
    await budget.wait(lambda: backend.connect('real-a1200'), 25)
    backend.guard_original()
    result['physical_target'] = backend.require_target('real-a1200')
    save()
    identity = await call('amiga_run_script', dict(script='Version\nCPU\nAvail\nVersion bsdsocket.library\nShowNetStatus\nEcho PTG-PHYSICAL-IDENTITY-DONE\n', timeout=20))
    ack(identity, 'PTG-PHYSICAL-IDENTITY-DONE')
    require('68030' in identity and "192.168.0.156 (on interface 'plipbox')" in identity, 'Physical CPU/network identity mismatch')
    result['physical_identity'] = identity
    bridge = Path(backend.plan['bridge_snapshot']).read_bytes()
    check = await call('amiga_checksum', dict(path=backend.plan['bridge_path']))
    backend.checksum(check, bridge)
    result['installed_bridge_sha256'] = digest(bridge)
    fresh = await call('amiga_run_script', dict(script=absence_script(destination, 'PTG-FRESH-ABSENT'), timeout=20))
    ack(fresh, 'PTG-FRESH-ABSENT')
    made = await call('amiga_run_script', dict(script='MakeDir ' + destination + '\nIf WARN\n Quit 20\nEndIf\nEcho PTG-DIRECTORY-CREATED\n', timeout=20))
    ack(made, 'PTG-DIRECTORY-CREATED')
    result['directory_created'] = True
    inventory(await call('amiga_run_script', dict(script=list_script(destination), timeout=20)), [])
    await call('amiga_push_file', dict(local_path=str(binary), amiga_path=remote), 40)
    backend.checksum(await call('amiga_checksum', dict(path=remote)), binary.read_bytes())
    returned = out / 'staged-readback.bin'
    await call('amiga_pull_file', dict(amiga_path=remote, local_path=str(returned)), 40)
    require(returned.stat().st_size == BYTES and digest(returned.read_bytes()) == SHA, 'Physical staged binary readback changed')
    result['binary_readback_sha256'] = SHA
    inventory(await call('amiga_run_script', dict(script=list_script(destination), timeout=20)), [NAME])
    budget.remaining(150)
    result['native_launches'] = 1
    result['native_running_or_unknown'] = True
    save()
    script = ('FailAt 21\nStack 65536\nCD ' + destination + '\n' + NAME + ' project.ptg >project.log\n'
              'Echo PTG-PROJECT-RC $RC\nCD RAM:\nEcho PTG-PROJECT-DONE\n')
    execution = await call('amiga_run_script', dict(script=script, timeout=NATIVE_SECONDS), 105)
    ack(execution, 'PTG-PROJECT-DONE')
    require(sum(line.startswith('PTG-PROJECT-RC ') for line in execution.splitlines()) == 1, 'Native RC marker incomplete')
    result['native_completed'] = True
    result['native_running_or_unknown'] = False
    ack(execution, 'PTG-PROJECT-RC 0')
    local_log = out / 'project.log'
    await call('amiga_pull_file', dict(amiga_path=logfile, local_path=str(local_log)), 30)
    result['native_markers'] = native_log(local_log.read_bytes())
    backend.checksum(await call('amiga_checksum', dict(path=logfile)), local_log.read_bytes())
    result['native_log_sha256'] = digest(local_log.read_bytes())
    inventory(await call('amiga_run_script', dict(script=list_script(destination), timeout=20)), [NAME, 'project.log'])
    ack(await call('amiga_run_script', dict(script=absence_script(fixture, 'PTG-FIXTURE-ABSENT'), timeout=20)), 'PTG-FIXTURE-ABSENT')
    result['fixture_and_temporary_children_absent'] = True
    # No cleanup in finally: any preceding native/protocol/guard failure retains
    # target/owned paths. Exactly these two verified files and empty directory.
    clean = ('CD RAM:\nDelete ' + remote + '\nIf WARN\n Quit 20\nEndIf\nDelete ' + logfile +
             '\nIf WARN\n Quit 20\nEndIf\nDelete ' + destination + '\nIf WARN\n Quit 20\nEndIf\n' +
             absence_script(destination, 'PTG-CLEANUP-ABSENT'))
    ack(await call('amiga_run_script', dict(script=clean, timeout=20)), 'PTG-CLEANUP-ABSENT')
    result['exact_nonrecursive_cleanup_acknowledged'] = True
    started = budget.clock()
    for index in range(11):
        marker = 'PTG-INDEPENDENT-ABSENT-' + str(index)
        observation = {'index': index, 'elapsed_seconds': budget.clock() - started, 'completed': False}
        result['physical_absence_observations'].append(observation)
        save()
        response = await call('amiga_run_script', dict(script=absence_script(destination, marker), timeout=15), 20)
        ack(response, marker)
        observation.update(completed=True, path=destination, exists=False)
        save()
        if index < 10:
            await budget.wait(lambda: backend.pause(1.01), 2)
    require(budget.clock() - started > 10, 'Physical independent absence interval too short')
    result['physical_independent_absence_passed'] = True
    result['physical_absence_seconds'] = budget.clock() - started
    budget.remaining(25)
    backend.guard_original()
    backend.require_target('real-a1200')
    result['return_target_switch_attempted'] = True
    save()
    await budget.wait(lambda: backend.connect('amiberry-030'), 25)
    result['returned_target'] = backend.require_target('amiberry-030')
    result['final_original_idle'] = backend.guard_original(local=True)
    result['devbench_returned'] = True
    budget.remaining()
    result['passed'] = True
    result['recovery_hold'] = False
    result['root_separate_release_check_required'] = True
    save()


class RealBackend:
    def __init__(self, plan, out, lock_records):
        # This class is constructed ONLY after explicit root gate + both locks.
        sys.path.insert(0, str(INFRA / 'scripts'))
        from amiga import connect
        from bridge_checks import target, require_reply, checksum
        from emulator import processes
        from shared_guest import Guest
        from mcp import ClientSession
        from mcp.client.streamable_http import streamable_http_client
        self.connect_api, self.target_api = connect, target
        self.reply_api, self.checksum = require_reply, checksum
        self.processes, self.Guest = processes, Guest
        self.ClientSession, self.stream_client = ClientSession, streamable_http_client
        self.plan, self.out, self.lock_records = plan, out, lock_records
        self.guest = Guest(INFRA, out)
        self.session = None

    def guard_original(self, local=False):
        for path, fd, identity in self.lock_records:
            a, b = os.fstat(fd), Path(path).lstat()
            require((a.st_dev, a.st_ino, a.st_mode) == identity == (b.st_dev, b.st_ino, b.st_mode), 'Lock path/inode redirected')
        require(self.processes() == [tuple(row) for row in self.plan['original_processes']], 'Original sole PID/full command changed')
        profile = Path(self.plan['original_profile'])
        require(digest(profile.read_bytes()) == self.plan['original_profile_sha256'], 'Original command profile changed')
        cpu = self.guest.command('GET_CPU_MODEL')
        status = self.guest.command('GET_STATUS')
        audio = self.guest.command('GET_AUDIO_STATE')
        require('model=68030' in cpu.split('\t') and 'Paused=false' in status.split('\t') and all('ch%d_dma=0' % i in audio.split('\t') for i in range(4)), 'Original030 not running/DMA-off')
        if local:
            self.require_target('amiberry-030')
            self.Guest(INFRA, self.out)
        return dict(processes=self.plan['original_processes'], cpu=cpu, status=status, audio=audio,
                    command_profile=str(profile), profile_sha256=self.plan['original_profile_sha256'])

    def require_target(self, name):
        return self.target_api(INFRA, name)

    async def connect(self, name):
        await self.connect_api(name)

    async def call(self, name, arguments):
        if name == 'amiga_run_script':
            require(1 <= arguments['timeout'] <= NATIVE_SECONDS, 'Unbounded native script timeout')
        reply = await self.session.call_tool(name, arguments)
        text = self.reply_api(reply)
        if name == 'amiga_run_script':
            require('[OK]' in text, 'Native script completion acknowledgment absent')
        return text

    async def pause(self, seconds):
        await asyncio.sleep(seconds)

    async def run(self, out, binary, result, budget):
        async with self.stream_client('http://127.0.0.1:3000/mcp') as (rd, wr, _):
            async with self.ClientSession(rd, wr) as session:
                self.session = session
                await budget.wait(session.initialize, 10)
                await transaction(out, binary, self, result, budget)


def lock_existing(path):
    fd = os.open(path, os.O_RDWR | os.O_NOFOLLOW)
    try:
        a, b = os.fstat(fd), Path(path).lstat()
        require(stat.S_ISREG(a.st_mode) and (a.st_dev, a.st_ino, a.st_mode) == (b.st_dev, b.st_ino, b.st_mode), 'Redirected/nonregular lock')
        # Existing coordination users leave these lock files empty. A nonempty
        # lock is an unrecognized lease format, not authorization to overwrite it.
        require(a.st_size == 0, 'Unrecognized lock lease metadata; root must inspect')
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return (str(path), fd, (a.st_dev, a.st_ino, a.st_mode))
    except BaseException:
        os.close(fd)
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute-approved-single-case', action='store_true')
    parser.add_argument('--coordination-gate', type=Path)
    parser.add_argument('--coordination-gate-sha256')
    parser.add_argument('--prepared-manifest-sha256')
    args = parser.parse_args()
    require(args.execute_approved_single_case and args.coordination_gate and args.coordination_gate_sha256 and args.prepared_manifest_sha256,
            'Preparation only until explicit root TAKE/Safari gate supplied')
    prepared_package(args.prepared_manifest_sha256)
    plan = read_json(HERE / 'plan.json')
    budget = Budget()
    out = HERE / ('physical-project-' + str(time.time_ns()))
    out.mkdir()
    result = dict(passed=False, scope='One exact physical RAM-only streamed project save/Fast allocator fixture; no card/device/MMIO/IRQ/driver/audio/timing/listening acceptance',
                  native_launches=0, target_switch_attempted=False, recovery_hold=False, root_release_not_performed=True)
    locks = []
    try:
        result['host_qualification'] = host_qualification(plan)
        result['root_reported_coordination_gate'] = gate_record(args.coordination_gate, args.coordination_gate_sha256)
        result['root_gate_path'] = str(args.coordination_gate)
        result['root_gate_sha256'] = args.coordination_gate_sha256
        binary = out / NAME
        shutil.copyfile(plan['binary'], binary)
        require(digest(binary.read_bytes()) == SHA and binary.stat().st_size == BYTES, 'Private candidate copy changed')
        for name in ('test.lock', 'real-a1200-safari.lock'):
            locks.append(lock_existing(INFRA / 'runtime' / name))
        # Recheck root freshness after acquiring both resources. Free locks alone
        # are not a current peer reservation or Safari/remote-control clearance.
        gate_record(args.coordination_gate, args.coordination_gate_sha256)
        backend = RealBackend(plan, out, locks)
        result['initial_original_idle'] = backend.guard_original(local=True)
        # Exclusive one-attempt receipt: an interrupted process consumes the
        # scope too. No second target attempt from this prepared package.
        with (HERE / 'physical-execution-claim.json').open('x') as claim:
            claim.write(json.dumps({'scope': SCOPE, 'out': str(out), 'gate_sha256': args.coordination_gate_sha256,
                                    'candidate_sha256': SHA, 'created_utc': datetime.now(timezone.utc).isoformat()}, indent=2) + '\n')
        result['exclusive_one_attempt_claim'] = str(HERE / 'physical-execution-claim.json')
        asyncio.run(asyncio.wait_for(backend.run(out, binary, result, budget), timeout=budget.remaining()))
    except BaseException as error:
        result['passed'] = False
        result['error'] = str(error) or type(error).__name__
        result['recovery_hold'] = bool(result.get('target_switch_attempted'))
        result['host_prelaunch_not_run'] = not result.get('target_switch_attempted')
        raise
    finally:
        result['elapsed_seconds'] = budget.clock() - budget.started
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        for _, fd, _ in reversed(locks):
            os.close(fd)
        print(out, flush=True)


if __name__ == '__main__':
    main()
