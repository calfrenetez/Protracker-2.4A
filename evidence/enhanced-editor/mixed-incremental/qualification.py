"""Exact committed mixed-preparation fixtures; separately coordinated shared030 only."""
import fcntl, hashlib, json, os, shutil, subprocess, sys, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INFRA = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
CANDIDATE = Path('/private/tmp/protracker-mixed-preflight-incremental-1790982880942013000')
CC = '/Users/james1/Documents/Codex/2026-08-20/work-from-the-design-spec-v1/repo/.cache/amiga/bin/m68k-amigaos-gcc'
sys.path.insert(0, str(INFRA/'scripts'))
from shared_guest import Guest
from emulator import processes

def digest(data):
    return hashlib.sha256(data).hexdigest()

def main():
    manifest = json.loads((CANDIDATE/'build.json').read_text())
    out = ROOT/'build/dev'/('mixed-incremental-qualification-'+str(time.time_ns()))
    out.mkdir()
    result = {'passed': False, 'scope': 'Three exact committed incremental mixed preparation fixtures; injected voices, no physical card/output/timing acceptance', 'cases': []}
    print('EVIDENCE', out, flush=True)
    try:
        tree = subprocess.check_output(['git', 'rev-parse', 'HEAD^{tree}'], cwd=ROOT, text=True).strip()
        result['source_tree'] = tree
        for record in manifest['binaries'].values():
            binary = Path(record['path'])
            if binary.stat().st_size != record['bytes'] or digest(binary.read_bytes()) != record['sha256']:
                raise RuntimeError('Candidate bytes changed before guest access')
            for name, expected in record['inputs'].items():
                data = subprocess.check_output(['git', 'show', tree+':'+name], cwd=ROOT)
                if digest(data) != expected:
                    raise RuntimeError('Committed candidate input differs: '+name)
        if digest(Path(CC).read_bytes()) != manifest['compiler_sha256']:
            raise RuntimeError('Compiler changed')
        for name, expected in manifest['runtime_inputs'].items():
            path = Path(subprocess.check_output([CC, '-m68000', '-msoft-float', '-mcrt=nix20', '-print-file-name='+name], text=True).strip())
            if digest(path.read_bytes()) != expected:
                raise RuntimeError('Runtime changed: '+name)
        # Private immutable runner export excludes unrelated working-tree edits.
        runner = out/'runner'
        runner.mkdir()
        with subprocess.Popen(['git', 'archive', tree, 'tools'], cwd=ROOT, stdout=subprocess.PIPE) as archive:
            subprocess.run(['tar', '-x', '-C', str(runner)], stdin=archive.stdout, check=True)
            archive.stdout.close()
            if archive.wait(): raise RuntimeError('Runner export failed')
        artifacts = runner/'build/dev'
        artifacts.mkdir(parents=True)
        for name, record in manifest['binaries'].items():
            shutil.copyfile(record['path'], artifacts/name)
        result['verified_build_manifest'] = manifest
        result['build_manifest_sha256'] = digest((CANDIDATE/'build.json').read_bytes())
        with (INFRA/'runtime/test.lock').open('a') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX|fcntl.LOCK_NB)
            rows = processes()
            if len(rows) != 1: raise RuntimeError('No sole owned emulator')
            result['original_processes'] = rows
            guest = Guest(INFRA, out)
            status, audio = guest.command('GET_STATUS'), guest.command('GET_AUDIO_STATE')
            if 'Paused=false' not in status.split('\t') or not all('ch%d_dma=0'%i in audio.split('\t') for i in range(4)):
                raise RuntimeError('Initial running/DMA-off guard failed')
            result['initial_status'], result['initial_audio'] = status, audio
        deadline = time.monotonic()+470
        for label, name in [('mixed','PTExecMixedPreflightTest'),('mixed-owner','PTExecMixedOwnerTest'),('editor-mixed','PTExecEditorMixedTest')]:
            if deadline-time.monotonic()<145: raise RuntimeError('Insufficient bounded window for another launch')
            if processes() != rows: raise RuntimeError('Original process changed before launch')
            with (out/(label+'-runner.log')).open('w') as log:
                rc = subprocess.run([str(INFRA/'.venv/bin/python'), 'tools/shared_infra_render_files.py', '--paula-memory', label], cwd=runner, stdout=log, stderr=subprocess.STDOUT, timeout=140).returncode
            raw = (out/(label+'-runner.log')).read_text()
            if rc: raise RuntimeError(label+' runner failed; preserve run and stop')
            run = Path(raw.strip().splitlines()[-1])
            case = json.loads((run/'result.json').read_text())
            if not case.get('passed') or not case.get('run_files_cleaned') or case[name+'_sha256'] != manifest['binaries'][name]['sha256']:
                raise RuntimeError('Native result/cleanup/hash unconfirmed')
            with (INFRA/'runtime/test.lock').open('a') as lock:
                fcntl.flock(lock, fcntl.LOCK_EX|fcntl.LOCK_NB)
                if processes() != rows: raise RuntimeError('Original process changed after run')
                guest = Guest(INFRA, run)
                status, audio = guest.command('GET_STATUS'), guest.command('GET_AUDIO_STATE')
                if 'Paused=false' not in status.split('\t') or not all('ch%d_dma=0'%i in audio.split('\t') for i in range(4)):
                    raise RuntimeError('Independent idle guard failed')
                observations=[]
                for index in range(11):
                    paths={str(path):os.path.lexists(path) for path in (guest.share/run.name, guest.launch)}
                    observations.append(paths)
                    if any(paths.values()): raise RuntimeError('Independent exact path absence failed; no cleanup retry')
                    if index<10: time.sleep(1)
                independent={'passed':True,'processes':rows,'status':status,'audio':audio,'exact_path_absence':observations}
                (run/'independent-cleanup.json').write_text(json.dumps(independent,indent=2)+'\n')
            result['cases'].append({'label':label,'binary':name,'run':str(run),'result':case,'independent_cleanup':independent})
            (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
            print(label, 'PASS', run, flush=True)
        result['passed']=True
    except Exception as error:
        result['error']=str(error)
        raise
    finally:
        (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
        print(out, flush=True)

if __name__=='__main__': main()
