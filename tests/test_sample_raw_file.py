from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class SampleRawFile(unittest.TestCase):
    def test_bounded_lossless_raw(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core','-Isrc/platform',
                            'tests/sample_raw_file_test.c','src/platform/sample_raw_file.c',
                            'src/platform/file_save.c','src/core/safe_save.c',
                            'src/core/raw.c','src/core/pcm.c','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe),str(Path(tmp)/'sample.raw')],check=True)
            self.assertFalse([p for p in Path(tmp).iterdir() if p.name not in ('test','test.dSYM')])
