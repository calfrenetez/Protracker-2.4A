from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_recovery_file import ROOT, SOURCES

class RecoveryRequesterModel(unittest.TestCase):
    def test_actual_input_model_and_controller(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory / 'proto').mkdir()
            (directory / 'dos').mkdir()
            (directory / 'dos/var.h').write_text(
                '#define GVF_GLOBAL_ONLY 256\n#define GVF_BINARY_VAR 1024\n')
            (directory / 'proto/dos.h').write_text('''#include <stdint.h>
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef intptr_t BPTR;
typedef char *STRPTR;
struct FileInfoBlock { LONG fib_DirEntryType; char fib_FileName[108]; };
#define ACCESS_READ -2
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_NO_MORE_ENTRIES 232
LONG GetVar(STRPTR, STRPTR, LONG, ULONG);
LONG IoErr(void);
BPTR Lock(STRPTR, LONG);
void UnLock(BPTR);
LONG Examine(BPTR, struct FileInfoBlock *);
LONG ExNext(BPTR, struct FileInfoBlock *);
LONG NameFromLock(BPTR, STRPTR, LONG);
LONG AddPart(STRPTR, STRPTR, LONG);
''')
            exe = directory / 'test'
            subprocess.run([
                'cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-pedantic', '-UNDEBUG', '-fsanitize=address,undefined',
                '-fno-omit-frame-pointer', '-I' + str(directory), '-Isrc/core',
                'tests/native_recovery_requester_model_test.c',
                'src/native/recovery_preferences.c',
                'src/native/recovery_requester_model.c', *SOURCES, '-o', str(exe),
            ], cwd=ROOT, check=True, timeout=120)
            env = os.environ.copy()
            env['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1:abort_on_error=1'
            env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
            result = subprocess.run([str(exe)], cwd=ROOT, env=env, check=True,
                timeout=30, capture_output=True, text=True)
            self.assertEqual(result.stderr, '')
            self.assertEqual(result.stdout,
                'RECOVERY REQUESTER MODEL HOST PASS: 6 groups; input/focus, caps/cancel, real controller apply/bind/refusal\n'
                'NOT TESTED: native window/IDCMP/rendering, editor dispatch, main timer/modal lifecycle, emulator or A1200\n')

if __name__ == '__main__':
    unittest.main()
