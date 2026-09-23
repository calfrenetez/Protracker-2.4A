from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class RecentMemory(unittest.TestCase):
    def test_allocator_and_interrupted_io(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp)
            for faults in (False,True):
                exe=tmp/'test'
                subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    *(['-DPT_IO_FAULTS','-Dread=pt_test_read','-Dwrite=pt_test_write','tests/recent_io_faults.c'] if faults else []),
                    'tests/recent_memory_test.c','src/platform/recent_file.c','src/core/recent.c','-o',str(exe)],cwd=ROOT,check=True)
                subprocess.run([str(exe),str(tmp/str(faults))],check=True)
