from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_editor import ROOT, SOURCES

class RecoveryDispatch(unittest.TestCase):
    def test_real_busy_job_and_default_settings_action(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'recovery-dispatch'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra',
                '-Werror', '-pedantic', '-UNDEBUG', '-fsanitize=address,undefined',
                '-fno-omit-frame-pointer', '-Isrc/core',
                'tests/editor_recovery_dispatch_test.c', *SOURCES[1:], '-o', str(binary)],
                cwd=ROOT, check=True, timeout=120)
            env = os.environ.copy()
            env['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1:abort_on_error=1'
            env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
            run = subprocess.run([str(binary)], cwd=ROOT, env=env, check=True,
                timeout=30, capture_output=True, text=True)
            self.assertEqual(run.stderr, '')
            self.assertEqual(run.stdout,
                'RECOVERY DISPATCH HOST PASS: real busy copy retained; default Settings emits only action; other-panel Cancel/Escape preserved\n'
                'NOT TESTED: native audio ownership gate, requester window/input/redraw, main timer lifecycle, emulator or A1200\n')

if __name__ == '__main__':
    unittest.main()
