#!/usr/bin/env python3
"""One coordinated RAM-only diagnostic using existing shared physical promotion.

Requires current user hardware authority and a reserved physical/DevBench window.
Never resets, retries, installs software or changes network settings. A failure
retains guest files. Always attempts to restore and verify the emulator selection.
"""
import asyncio
import datetime
import fcntl
import json
import re
import sys
from types import SimpleNamespace
from shared_infra_timing import ROOT, INFRA, BINARY, EXPECTED, profile


def validate_completion(record, log, sha):
    if record.get('passed') is not True or record.get('tested_artifacts') != {BINARY: sha} or record.get('completed_cases') != [BINARY]:
        raise RuntimeError('Physical completion does not cover exact diagnostic')
    for marker in ('NATIVE ECLOCK PASS:', 'NATIVE ALARM PASS:', 'NATIVE SIGNAL PASS:',
                   'NATIVE SONG GATE PASS:', 'NATIVE COST PASS:', 'PROJECT SNAPSHOT PASS:',
                   'zero owned bytes, budget refusal without Chip fallback'):
        if marker not in log:
            raise RuntimeError('Missing physical completion: ' + marker)
    if not log.rstrip().endswith(EXPECTED):
        raise RuntimeError('Physical final completion marker absent')


def cleanup_script(run_name):
    if not re.fullmatch(r'\d{8}T\d{12}Z', run_name):
        raise ValueError('Expected a generated shared-run timestamp')
    dest = 'RAM:infra-' + run_name
    lines = ['CD RAM:']
    for name in ('payload.bin', BINARY, BINARY + '.log'):
        lines += ['If EXISTS ' + dest + '/' + name, ' Delete ' + dest + '/' + name + ' QUIET', 'EndIf']
    lines += ['Delete ' + dest + ' QUIET', 'If EXISTS ' + dest,
              ' Echo TIMING-CLEANUP-FAILED', 'Else', ' Echo TIMING-CLEAN', 'EndIf']
    return '\n'.join(lines) + '\n'


async def coordinated_run(cfg, out, promote, connect, execute, finish, restored):
    # Promotion refusal causes no selection, upload or hardware access.
    promote(cfg['test_cases'], ROOT)
    try:
        await connect('real-a1200')
        await execute(SimpleNamespace(project='protracker', target='real-a1200', suite='core'), out, {'protracker': cfg})
        await finish()
    finally:
        await connect('amiberry-030')
        restored()


def main():
    cfg, build = profile(ROOT)
    sys.path.insert(0, str(INFRA / 'scripts'))
    import amiga
    import physical
    from bridge_checks import target, require_reply
    from evidence import write_result
    from shared_guest import Guest
    from shared_infra_render_files import require_running_guest
    from mcp import ClientSession
    from mcp.client.streamable_http import streamable_http_client
    out = INFRA / 'results' / datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    out.mkdir()
    write_result(out / 'result.json', {'target': 'real-a1200', 'project': 'protracker', 'passed': False})

    def record_update(**fields):
        record = json.loads((out / 'result.json').read_text())
        record.update(fields)
        write_result(out / 'result.json', record)

    def restored():
        target(INFRA, 'amiberry-030')
        guest = Guest(INFRA, out)
        require_running_guest(guest, out, 'after-physical-restore')
        record_update(emulator_selection_restored=True)

    async def finish():
        record = json.loads((out / 'result.json').read_text())
        log = (out / (BINARY + '.log')).read_text()
        validate_completion(record, log, build['binary_sha256'])
        async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd, wr, _):
            async with ClientSession(rd, wr) as session:
                await session.initialize()
                async def call(name, args):
                    target(INFRA, 'real-a1200')
                    reply = await session.call_tool(name, args)
                    text = '\n'.join(x.text for x in reply.content if hasattr(x, 'text'))
                    record.setdefault('cleanup_calls', []).append({'tool': name, 'arguments': args, 'result': text})
                    write_result(out / 'result.json', record)
                    require_reply(reply)
                    return text
                audio = await call('amiga_audio_channels', {})
                if not all(re.search(r'^\s*Channel %d: DMA=off\s'%i, audio, re.M) for i in range(4)):
                    raise RuntimeError('Physical DMA state not confirmed idle; retain files')
                response = await call('amiga_run_script', {'script': cleanup_script(out.name), 'timeout': 20})
                if 'TIMING-CLEAN' not in response.splitlines() or 'TIMING-CLEANUP-FAILED' in response:
                    raise RuntimeError('Exact physical cleanup not confirmed')
                verify = 'If EXISTS RAM:infra-' + out.name + '\n Echo TIMING-REMAINS\nElse\n Echo TIMING-ABSENT\nEndIf\n'
                response = await call('amiga_run_script', {'script': verify, 'timeout': 20})
                if 'TIMING-ABSENT' not in response.splitlines() or 'TIMING-REMAINS' in response:
                    raise RuntimeError('Independent physical cleanup verification failed')
                record_update(run_files_cleaned=True, physical_dma_off=True,
                              scope='finite RAM-only clock/memory/fake-voice observation; no audio or AmiGUS acceptance')
    try:
        with (INFRA / 'runtime/test.lock').open('a') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            guest = Guest(INFRA, out)
            require_running_guest(guest, out, 'before-physical-selection')
            asyncio.run(coordinated_run(cfg, out, physical.promotion, amiga.connect, physical.test, finish, restored))
    except BaseException as exc:
        record_update(passed=False, error=str(exc) or type(exc).__name__)
        raise
    finally:
        print(out, flush=True)


if __name__ == '__main__':
    main()
