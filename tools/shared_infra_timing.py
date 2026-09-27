#!/usr/bin/env python3
"""Qualify the finite no-argument timing diagnostic through shared core promotion.

Uses a process-local one-case profile; does not alter the installed project
adapter, shared scripts, emulator configuration or physical target selection.
A separately coordinated emulator window is required. Failures retain owned files.
"""
import asyncio,datetime,fcntl,hashlib,json,shutil,sys
from pathlib import Path
from types import SimpleNamespace
ROOT=Path(__file__).resolve().parents[1]
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
BINARY='PTExecWavetableTimingTest'
EXPECTED='NATIVE TIMING PASS: finite clock/alarm/watchdog and fake-voice observations, snapshot byte parity, all leases and Fast allocations released; no audio or AmiGUS access'

def profile(repo):
    binary=repo/'build/dev'/BINARY
    manifest=json.loads((repo/'build/dev/wavetable-timing-build.json').read_text())
    if binary.is_symlink() or not binary.is_file():raise RuntimeError('Expected a regular timing candidate')
    data=binary.read_bytes()
    if '-DPT_NATIVE_TIMING_ONLY' not in manifest['flags'] or len(data)!=manifest['binary_bytes'] or hashlib.sha256(data).hexdigest()!=manifest['binary_sha256']:
        raise RuntimeError('Timing candidate does not match its dedicated build manifest')
    return {'repo':str(repo),'artifact':'build/dev/'+BINARY,'build_evidence':'build/dev/wavetable-timing-build.json',
            'test_cases':[{'artifact':'build/dev/'+BINARY,'expected':EXPECTED}]},manifest

def completed_files(run,expected):
    if run.is_symlink() or sorted(p.name for p in run.iterdir())!=[BINARY,BINARY+'.log']:
        raise RuntimeError('Unexpected owned run contents; retain for inspection')
    binary=run/BINARY;logfile=run/(BINARY+'.log')
    if binary.is_symlink() or logfile.is_symlink() or hashlib.sha256(binary.read_bytes()).hexdigest()!=expected:
        raise RuntimeError('Owned candidate changed; retain for inspection')
    log=logfile.read_text()
    for marker in ('NATIVE ECLOCK PASS:','NATIVE ALARM PASS:','NATIVE SIGNAL PASS:','NATIVE SONG GATE PASS:',
                   'NATIVE COST PASS:','PROJECT SNAPSHOT PASS:','zero owned bytes, budget refusal without Chip fallback'):
        if marker not in log:raise RuntimeError('Missing completion marker: '+marker)
    if not log.rstrip().endswith(EXPECTED):raise RuntimeError('Final timing completion marker absent')
    return log

def main():
    cfg,build=profile(ROOT)
    sys.path.insert(0,str(INFRA/'scripts'))
    import amiga
    from shared_guest import Guest
    from evidence import write_result
    from shared_infra_render_files import require_running_guest
    out=INFRA/'results'/datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ');out.mkdir()
    record={'target':'amiberry-030','project':'protracker','suite':'core','passed':False,
            'attempted_artifacts':{BINARY:build['binary_sha256']},'scope':'finite timing/memory diagnostic; fake voice bus, no audio/card access'}
    write_result(out/'result.json',record)
    try:
        with (INFRA/'runtime/test.lock').open('a') as lock:
            fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
            guest=Guest(INFRA,out);require_running_guest(guest,out,'before-core-test')
            # Same shared core runner and exact-byte latest-result promotion gate.
            # No arguments or inputs; process-local profile leaves shared config intact.
            amiga.PROJECTS['protracker']=cfg
            asyncio.run(amiga.test(SimpleNamespace(project='protracker',target='amiberry-030',suite='core'),out))
            record=json.loads((out/'result.json').read_text())
            hashes={BINARY:build['binary_sha256']}
            if record.get('passed') is not True or record.get('tested_artifacts')!=hashes or record.get('completed_cases')!=[BINARY]:
                raise RuntimeError('Shared core did not complete the exact candidate')
            run=guest.share/out.name;log=completed_files(run,build['binary_sha256'])
            audio=guest.command('GET_AUDIO_STATE')
            if not all('ch%d_dma=0'%i in audio.split('\t') for i in range(4)):raise RuntimeError('Audio DMA remains active')
            (out/(BINARY+'.log')).write_text(log)
            shutil.copyfile(ROOT/'build/dev/wavetable-timing-build.json',out/'build-evidence.json')
            # Exact completed run only; no launcher or arbitrary recursive targets.
            for name in (BINARY,BINARY+'.log'):(run/name).unlink()
            run.rmdir();assert not run.exists()
            record.update(cleanup_audio=audio,run_files_cleaned=True,status='passed',scope='finite clock/memory/fake-voice diagnostic; no audio/card access')
            write_result(out/'result.json',record)
    except BaseException as exc:
        record=json.loads((out/'result.json').read_text())
        record.update(passed=False,status='failed',error=str(exc) or type(exc).__name__)
        write_result(out/'result.json',record);raise
    finally:print(out,flush=True)
if __name__=='__main__':main()
