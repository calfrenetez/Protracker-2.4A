from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class NativeTaskPriority(unittest.TestCase):
    def test_scope_failures(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)
            (p/'exec').mkdir();(p/'proto').mkdir()
            (p/'exec/tasks.h').write_text('''#pragma once
#include <stdint.h>
typedef int8_t BYTE;typedef int32_t LONG;
struct Task{struct {BYTE ln_Pri;} tc_Node;};
''')
            (p/'proto/exec.h').write_text('''#include <exec/tasks.h>
struct Task *FindTask(const char *);BYTE SetTaskPri(struct Task *,LONG);
''')
            exe=p/'test'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+tmp,'tests/native_task_priority_test.c','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
