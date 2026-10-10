from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_recovery_file import ROOT, SOURCES

class RecoveryRequesterLoop(unittest.TestCase):
    def test_actual_renderer_and_controller_with_owned_port_stubs(self):
        with tempfile.TemporaryDirectory() as temporary:
            executable = Path(temporary) / 'recovery-requester-loop'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra',
                '-Werror', '-pedantic', '-UNDEBUG', '-fsanitize=address,undefined',
                '-fno-omit-frame-pointer', '-Itests/recovery_requester_host_stubs',
                '-Isrc/core', 'tests/native_recovery_requester_loop_test.c',
                'src/native/recovery_preferences.c', 'src/native/recovery_requester_model.c',
                'src/native/recovery_requester.c', *SOURCES, '-o', str(executable)],
                cwd=ROOT, check=True, timeout=120)
            environment = os.environ.copy()
            environment['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1:abort_on_error=1'
            environment['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
            result = subprocess.run([str(executable)], cwd=ROOT, env=environment,
                check=True, timeout=30, capture_output=True, text=True)
            self.assertEqual(result.stderr, '')
            self.assertEqual(result.stdout,
                'RECOVERY REQUESTER LOOP HOST PASS: 8 groups; actual renderer/controller with deterministic owned-port stubs\n'
                'NOT TESTED: real Intuition ABI/input, parent IDCMP isolation, main timer, emulator or A1200\n')

if __name__ == '__main__':
    unittest.main()
