import importlib.util
from pathlib import Path
import tempfile
import sys
from types import SimpleNamespace
import json
from unittest.mock import patch
import shutil
import unittest
import fcntl

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('shared_render',ROOT/'tools/shared_infra_render_files.py')
runner=importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)

class Guest:
    def __init__(self,share,status):
        self.share,self.status,self.commands=share,status,[]
        self.device="Dev:Tests/"
    def command(self,command):
        self.commands.append(command)
        if isinstance(self.status,Exception):raise self.status
        return self.status

class SharedRenderGuard(unittest.TestCase):
    def test_busy_lock_refusal_keeps_existing_owner_and_records_no_staging(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);out=root/'evidence';out.mkdir();path=root/'test.lock'
            with path.open('a') as first,path.open('a') as second,path.open('a') as third:
                fcntl.flock(first,fcntl.LOCK_EX|fcntl.LOCK_NB)
                result={'passed':False}
                with self.assertRaises(BlockingIOError):runner.acquire_shared_lock(second,out,result)
                with self.assertRaises(BlockingIOError):fcntl.flock(third,fcntl.LOCK_EX|fcntl.LOCK_NB)
                recorded=json.loads((out/'result.json').read_text())
                self.assertTrue(recorded['prelaunch_refused'])
                self.assertFalse(recorded['run_files_staged']);self.assertFalse(recorded['passed'])
                self.assertIn('lock busy',recorded['refusal_reason'])
                fcntl.flock(first,fcntl.LOCK_UN)
                runner.acquire_shared_lock(second,out,{'passed':False})
    def test_refusal_leaves_guest_files_untouched(self):
        for status in ('OK\tPaused=true','OK\tConfig=autosave',
                       'OK\tPaused=false\tPaused=true','OK\tPaused=unknown',
                       RuntimeError('IPC unavailable')):
            with self.subTest(status=status),tempfile.TemporaryDirectory() as td:
                root=Path(td);out=root/'evidence';share=root/'guest';out.mkdir();share.mkdir()
                sentinel=share/'existing';sentinel.write_bytes(b'preserve')
                guest=Guest(share,status)
                with self.assertRaises(RuntimeError):runner.prepare_run(guest,out)
                self.assertEqual(guest.commands,['GET_STATUS'])
                self.assertEqual(list(share.iterdir()),[sentinel])
                self.assertEqual(sentinel.read_bytes(),b'preserve')

    def test_running_guest_can_stage_but_later_pause_refuses_launch(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);out=root/'evidence';share=root/'guest';out.mkdir();share.mkdir()
            guest=Guest(share,'OK\tPaused=false\tConfig=autosave')
            run=runner.prepare_run(guest,out)
            self.assertEqual(run,share/out.name);self.assertTrue(run.is_dir())
            candidate=run/'candidate';candidate.write_bytes(b'preserve for inspection')
            guest.status='OK\tPaused=true'
            with self.assertRaises(RuntimeError):runner.require_running_guest(guest,out,'before-launch')
            self.assertEqual(guest.commands,['GET_STATUS','GET_STATUS'])
            self.assertEqual(candidate.read_bytes(),b'preserve for inspection')


    def test_cleanup_requires_observed_absence_and_records_failures(self):
        for mode in ('normal','missing-launcher','recreated-directory','recreated-link','remove-error','active-dma','ipc-error','incomplete'):
            with self.subTest(mode=mode),tempfile.TemporaryDirectory() as td:
                root=Path(td);out=root/'render-files-test';share=root/'guest';out.mkdir();share.mkdir()
                run=share/out.name;run.mkdir();(run/'candidate').write_bytes(b'owned')
                other=share/'unrelated';other.write_bytes(b'preserve')
                guest=Guest(share,'OK\tch0_dma=0\tch1_dma=0\tch2_dma=0\tch3_dma=0')
                guest.launch=share/('launch-'+out.name);guest.launch.write_bytes(b'owned')
                if mode=='active-dma':guest.status='OK\tch0_dma=1\tch1_dma=0\tch2_dma=0\tch3_dma=0'
                if mode=='ipc-error':guest.status=RuntimeError('lost bridge')
                if mode=='missing-launcher':guest.launch.unlink()
                result={'passed':True}
                real_remove=shutil.rmtree
                def cleanup(current,evidence,script):
                    if mode=='remove-error':raise OSError('cannot remove')
                    guest.launch.unlink(missing_ok=True);real_remove(run)
                    if mode=='recreated-directory':run.mkdir()
                    if mode=='recreated-link':guest.launch.symlink_to(share/'missing')
                    return '[OK]\nPTG-CLEANUP-ABSENT-'+run.name
                with patch.object(runner,'execute_guest_cleanup',side_effect=cleanup) as deletion, patch.object(runner.time,'sleep'), patch.object(runner.shutil,'rmtree') as host_deletion:
                    if mode in ('normal','missing-launcher','incomplete'):
                        runner.finish_run(guest,run,out,result,mode!='incomplete',True)
                    else:
                        with self.assertRaises((RuntimeError,OSError)):
                            runner.finish_run(guest,run,out,result,True,True)
                host_deletion.assert_not_called()
                recorded=json.loads((out/'result.json').read_text())
                self.assertEqual(recorded['run_files_cleaned'],mode in ('normal','missing-launcher'))
                self.assertEqual(recorded['passed'],mode in ('normal','missing-launcher','incomplete'))
                self.assertEqual(other.read_bytes(),b'preserve')
                self.assertEqual(deletion.call_count,0 if mode in ('active-dma','ipc-error','incomplete') else 1)
                if mode in ('active-dma','ipc-error','incomplete'):
                    self.assertTrue(guest.launch.exists());self.assertTrue((run/'candidate').exists())
                if mode=='recreated-directory':self.assertTrue(run.is_dir())
                if mode=='recreated-link':self.assertTrue(guest.launch.is_symlink())
                if mode not in ('normal','missing-launcher','incomplete'):self.assertIn('cleanup_error',recorded)

    def test_delayed_recreation_fails_without_a_second_delete(self):
        for mode in ('directory','dangling-launcher'):
            with self.subTest(mode=mode),tempfile.TemporaryDirectory() as td:
                root=Path(td);out=root/'render-files-test';share=root/'guest';out.mkdir();share.mkdir()
                run=share/out.name;run.mkdir()
                unrelated=share/'unrelated';unrelated.write_bytes(b'preserve')
                guest=Guest(share,'OK\tch0_dma=0\tch1_dma=0\tch2_dma=0\tch3_dma=0')
                guest.launch=share/('launch-'+out.name);guest.launch.write_bytes(b'owned')
                sleeps=[]
                def delayed_recreate(interval):
                    sleeps.append(interval)
                    if len(sleeps)==3:
                        if mode=='directory':(run/'cia-timing').mkdir(parents=True)
                        else:guest.launch.symlink_to(share/'missing')
                result={'passed':True}
                def cleanup(current,evidence,script):
                    guest.launch.unlink();shutil.rmtree(run)
                    return '[OK]\nPTG-CLEANUP-ABSENT-'+run.name
                with patch.object(runner,'execute_guest_cleanup',side_effect=cleanup) as deletion, patch.object(runner.time,'sleep',side_effect=delayed_recreate):
                    with self.assertRaisesRegex(RuntimeError,'reappeared'):
                        runner.finish_run(guest,run,out,result,True,True)
                recorded=json.loads((out/'result.json').read_text())
                self.assertFalse(recorded['passed']);self.assertFalse(recorded['run_files_cleaned'])
                self.assertEqual(len(recorded['cleanup_absence_observations']),4)
                self.assertFalse(any(recorded['cleanup_absence_observations'][0]['paths'].values()))
                self.assertTrue(any(recorded['cleanup_absence_observations'][-1]['paths'].values()))
                self.assertEqual(deletion.call_count,1)
                self.assertEqual(unrelated.read_bytes(),b'preserve')

    def test_guest_cleanup_refuses_links_unsafe_names_and_wrong_scope(self):
        for mode in ('symlink-file','symlink-directory','unsafe-token','wrong-run','wrong-launcher'):
            with self.subTest(mode=mode),tempfile.TemporaryDirectory() as td:
                root=Path(td);out=root/'render-files-test';share=root/'share';out.mkdir();share.mkdir()
                run=share/out.name;run.mkdir();guest=Guest(share,'OK')
                guest.launch=share/('launch-'+out.name);guest.launch.write_text('owned')
                foreign=root/'foreign';foreign.mkdir();sentinel=foreign/'keep';sentinel.write_text('preserve')
                if mode=='symlink-file':(run/'link').symlink_to(sentinel)
                if mode=='symlink-directory':(run/'link').symlink_to(foreign,target_is_directory=True)
                if mode=='unsafe-token':(run/'bad#?.name').write_text('owned')
                if mode=='wrong-run':run=foreign
                if mode=='wrong-launcher':guest.launch=share/'other'
                with self.assertRaises(RuntimeError):runner.guest_cleanup_script(guest,run,out)
                self.assertEqual(sentinel.read_text(),'preserve');self.assertTrue(run.exists())

    def test_guest_script_is_exact_and_deletes_deepest_directories_last(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);out=root/'render-files-test';share=root/'share';out.mkdir();share.mkdir()
            run=share/out.name;deep=run/'case'/'stems';deep.mkdir(parents=True)
            (deep/'track-01.wav').write_bytes(b'owned');(run/'done').write_text('done')
            guest=Guest(share,'OK');guest.launch=share/('launch-'+out.name)
            script=runner.guest_cleanup_script(guest,run,out)
            deletes=[line.strip() for line in script.splitlines() if line.strip().startswith('Delete ')]
            self.assertEqual(deletes,['Delete Dev:Tests/launch-render-files-test',
                'Delete Dev:Tests/render-files-test/case/stems/track-01.wav',
                'Delete Dev:Tests/render-files-test/done',
                'Delete Dev:Tests/render-files-test/case/stems',
                'Delete Dev:Tests/render-files-test/case','Delete Dev:Tests/render-files-test'])
            self.assertNotIn(' ALL',script);self.assertIn('CD RAM:',script)

    def test_partial_cleanup_marker_retains_paths_without_host_fallback(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);out=root/'render-files-test';share=root/'share';out.mkdir();share.mkdir()
            run=share/out.name;run.mkdir();(run/'candidate').write_text('owned')
            guest=Guest(share,'OK');guest.launch=share/('launch-'+out.name);guest.launch.write_text('owned')
            result={'passed':True}
            with patch.object(runner,'execute_guest_cleanup',side_effect=RuntimeError('still running')),patch.object(runner.shutil,'rmtree') as remove:
                with self.assertRaises(RuntimeError):runner.finish_run(guest,run,out,result,True,False)
            remove.assert_not_called();self.assertTrue((run/'candidate').exists());self.assertTrue(guest.launch.exists())
            self.assertTrue(result['cleanup_script_pending']);self.assertFalse(result['passed']);self.assertFalse(result['run_files_cleaned'])

    def test_transport_requires_finished_acknowledgement_and_running_target(self):
        for text,error in (('[OK]\nPTG-CLEANUP-ABSENT-render-files-test',False),
                ('[STILL RUNNING]\nPTG-CLEANUP-ABSENT-render-files-test',True),
                ('[ERROR]\nPTG-CLEANUP-ABSENT-render-files-test',True)):
            with self.subTest(text=text),tempfile.TemporaryDirectory() as td:
                out=Path(td);guest=Guest(out,'OK\tPaused=false')
                reply=SimpleNamespace(returncode=0,stdout=json.dumps({'isError':False,'content':[{'type':'text','text':text}]}),stderr='')
                with patch.dict(sys.modules,{'shared_guest':SimpleNamespace(Guest=lambda *args:guest)}),patch.object(runner.subprocess,'run',return_value=reply) as call:
                    if error:
                        with self.assertRaises(RuntimeError):runner.execute_guest_cleanup(guest,out,'exact script')
                    else:self.assertEqual(runner.execute_guest_cleanup(guest,out,'exact script'),text)
                    self.assertEqual(call.call_count,1)
                    self.assertEqual(guest.commands,['GET_STATUS'])
                    guest.status='OK\tPaused=true'
                    with self.assertRaises(RuntimeError):runner.execute_guest_cleanup(guest,out,'exact script')
                    self.assertEqual(call.call_count,1)

if __name__=='__main__':unittest.main()
