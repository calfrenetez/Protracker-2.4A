import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('shared_render',ROOT/'tools/shared_infra_render_files.py')
runner=importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)

class Guest:
    def __init__(self,share,status):
        self.share,self.status,self.commands=share,status,[]
    def command(self,command):
        self.commands.append(command)
        if isinstance(self.status,Exception):raise self.status
        return self.status

class SharedRenderGuard(unittest.TestCase):
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

if __name__=='__main__':unittest.main()
