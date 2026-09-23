from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class FileSaveMemory(unittest.TestCase):
    def test_bounded_file_save(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp)
            for faults in (False,True):
                out=tmp/str(faults);out.mkdir();binary=tmp/'test'
                subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',
                                *(['-Dread=pt_test_read','tests/render_read_faults.c'] if faults else []),
                                'tests/file_save_memory_test.c','src/platform/file_save.c','src/core/safe_save.c','-o',str(binary)],cwd=ROOT,check=True)
                subprocess.run([str(binary),str(out)],check=True)
                self.assertEqual(sorted(p.name for p in out.iterdir()),['empty.bin','saved.bin'])
