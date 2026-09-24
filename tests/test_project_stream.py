from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class ProjectStream(unittest.TestCase):
    def test_streamed_master_project(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            for faults in (False,True):
                subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',
                            *(['-Dread=pt_test_read','tests/render_read_faults.c'] if faults else []),
                            'tests/project_stream_test.c','src/core/project.c','src/core/channels.c','src/core/pcm.c',
                            'src/platform/project_file.c','src/platform/file_save.c','src/core/safe_save.c','-o',str(exe)],cwd=ROOT,check=True)
                subprocess.run([str(exe),str(Path(tmp)/'master.ptg')],check=True)
            self.assertFalse([p for p in Path(tmp).iterdir() if p.name not in ('test','test.dSYM')])
