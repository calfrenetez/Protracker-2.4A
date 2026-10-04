#!/usr/bin/env python3
"""Run the scoped native-workflow host regression suite; never controls targets."""
import argparse,hashlib,json,os,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
GROUPS=('sample_usage','sampler_workflow','sampler_copy_range_boundaries','sample_range','wave_summary','event_resource',
        'editor_workflow','workflow_performance','workflow_view','sampler','slots','editor','pattern','song',
        'slices','flow','editor_guard','editor_capture','sampler_pin_job',
        'sampler_paula','sampler_wavetable','project','project_validation','document')
def sources():
    files=[p for folder in ('src','tests') for p in (ROOT/folder).rglob('*')
           if p.is_file() and p.suffix in ('.c','.h','.py')]
    files += [Path(__file__),ROOT/'tools/build_core_tests.py']
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence-dir',required=True)
    parser.add_argument('--group',action='append',choices=GROUPS,help='Run only selected affected groups; default runs the full suite')
    args=parser.parse_args();out=Path(args.evidence_dir).resolve();out.mkdir(parents=True,exist_ok=True)
    before=sources();rows=[];env=os.environ.copy();env['PYTHONDONTWRITEBYTECODE']='1'
    for group in (args.group or GROUPS):
        if not (ROOT/'tests'/('test_'+group+'.py')).is_file():raise SystemExit('Missing test group '+group)
        command=[sys.executable,'-B','-m','unittest','discover','-s','tests','-p','test_'+group+'.py','-v']
        start=time.monotonic()
        with (out/(group+'.log')).open('w') as log:
            run=subprocess.run(command,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT)
        rows.append(dict(group=group,command=command,returncode=run.returncode,seconds=time.monotonic()-start,log=group+'.log'))
        print(group,run.returncode,round(rows[-1]['seconds'],3),flush=True)
    report=dict(scope='Host sanitizer/controller/format regressions; no native or physical execution',
                python=sys.executable,compiler=subprocess.check_output(['cc','--version'],text=True),
                source_before=before,source_after=sources(),checks=rows)
    report['source_unchanged']=report['source_before']==report['source_after']
    report['passed']=report['source_unchanged'] and all(row['returncode']==0 for row in rows)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1
if __name__=='__main__':sys.exit(main())
