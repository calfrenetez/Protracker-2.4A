from pathlib import Path
import subprocess
import tempfile
import unittest
from test_recovery_file import ROOT, SOURCES


class NativeRecoveryConfig(unittest.TestCase):
    def test_complete_configuration_reads(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory / 'proto').mkdir()
            (directory / 'dos').mkdir()
            (directory / 'dos/var.h').write_text('''
#define GVF_GLOBAL_ONLY 256
#define GVF_BINARY_VAR 1024
''')
            (directory / 'proto/dos.h').write_text('''
#include <stdint.h>
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef intptr_t BPTR;
typedef char *STRPTR;
struct FileInfoBlock { LONG fib_DirEntryType; char fib_FileName[108]; };
#define ACCESS_READ -2
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_NO_MORE_ENTRIES 232
LONG GetVar(STRPTR,STRPTR,LONG,ULONG);
LONG IoErr(void);
BPTR Lock(STRPTR,LONG);
void UnLock(BPTR);
LONG Examine(BPTR,struct FileInfoBlock *);
LONG ExNext(BPTR,struct FileInfoBlock *);
LONG NameFromLock(BPTR,STRPTR,LONG);
LONG AddPart(STRPTR,STRPTR,LONG);
''')
            executable = directory / 'config-test'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra',
                            '-Werror', '-fsanitize=address,undefined',
                            '-I' + str(directory), '-Isrc/core',
                            'tests/native_recovery_config_test.c', *SOURCES,
                            '-o', str(executable)], cwd=ROOT, check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == '__main__':
    unittest.main()
