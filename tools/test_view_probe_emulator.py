#!/usr/bin/env python3
"""Validate the no-argument physical-readiness probe in a reserved private emulator."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import time
from build_diagnostic import ROOT, digest
from emulator_ipc import Emulator
from test_diagnostic_emulator import matching_socket


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--title-checks',action='store_true',help='Run native MOD title/import regressions instead of the benchmark')
    args=parser.parse_args()
    if matching_socket() or Path('/tmp/amiberry.sock').exists():
        raise SystemExit('Emulator already owned')
    env=json.loads((ROOT/'local/environment.json').read_text())
    manifest=json.loads((ROOT/'build/dev/core-build.json').read_text())
    assert all(digest(ROOT/p)==h for p,h in manifest['sources'].items())
    programs=['PTDocumentTest','PTSourceTest'] if args.title_checks else ['PTViewProbe']
    for name in programs:
        assert digest(ROOT/'build/dev'/name)==manifest['binaries'][name]['sha256']
    share=Path(env['share']);launch=share/'launch';original=launch.read_bytes()
    run=share/('viewprobe'+str(time.time_ns()));run.mkdir()
    out=ROOT/'build/dev/view-probe-evidence'/run.name;out.mkdir(parents=True)
    for name in programs:shutil.copyfile(ROOT/'build/dev'/name,run/name)
    if args.title_checks:
        from make_mod_sample_fixture import make
        baseline=(ROOT/'evidence/baseline/mod.baseline').read_bytes()
        (run/'baseline.mod').write_bytes(baseline);(run/'donor.mod').write_bytes(make(baseline))
    commands=(['PTDocumentTest baseline.mod >document.log','Echo $RC >document.rc','PTSourceTest donor.mod >source.log','Echo $RC >source.rc'] if args.title_checks else ['PTViewProbe >probe.log','Echo $RC >probe.rc'])
    process=emu=None;start=time.monotonic()
    try:
        launch.write_text('\n'.join(['FailAt 21','Wait 5','Stack 65536','CD PTDEV:'+run.name,
            *commands,'Echo '+run.name+' >done'])+'\n')
        with (run/'emulator.log').open('wb') as log:
            process=subprocess.Popen([env['emulator_binary'],'--config',env['profile'],'-G','-m','PTDEV:'+str(share),'--log'],stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        while time.monotonic()-start<150:
            if process.poll() is not None:raise RuntimeError('Emulator exited')
            if emu is None:
                matches=matching_socket()
                if len(matches)>1:raise RuntimeError('Ambiguous emulator')
                if matches:emu=Emulator(matches[0])
            if (run/'done').exists():break
            time.sleep(.2)
        else:raise RuntimeError('Probe deadline: '+str(run))
        assert (run/'done').read_text().strip()==run.name
        cases={}
        for name in (['document','source'] if args.title_checks else ['probe']):
            log=(run/(name+'.log')).read_text();rc=(run/(name+'.rc')).read_text().strip()
            assert rc=='0' and 'PASS' in log and 'FAIL' not in log,(rc,log)
            if name=='probe':assert 'identical=1' in log and 'PT24G VIEW PASS' in log
            cases[name]={'rc':rc,'log':log}
            shutil.copyfile(run/(name+'.log'),out/(name+'.log'))
        report={'run_id':run.name,'elapsed_seconds':round(time.monotonic()-start,3),
            'binaries':{n:manifest['binaries'][n] for n in programs},'cases':cases,
            'title_checks':args.title_checks,'physical_hardware_tested':False,
            'environment':{c:emu.command(c) for c in ['GET_VERSION','GET_CPU_MODEL','GET_MEMORY_CONFIG']}}
        (out/'native-view-probe.json').write_text(json.dumps(report,indent=2)+'\n')
        print('PASS: '+str(out),flush=True)
    finally:
        launch.write_bytes(original)
        if process and process.poll() is None:
            if emu is None:
                matches=matching_socket()
                if len(matches)==1:emu=Emulator(matches[0])
            if emu is None:raise RuntimeError('No guarded socket; emulator ownership retained for recovery')
            Emulator(emu.path).command('QUIT');process.wait(timeout=10)
if __name__=='__main__':main()
