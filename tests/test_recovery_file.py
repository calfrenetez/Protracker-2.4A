from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['src/core/recovery.c','src/platform/recovery_file.c','src/platform/project_file.c','src/platform/project_import.c','src/platform/file_save.c','src/core/document.c','src/core/pp20.c','src/core/safe_save.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class RecoveryFile(unittest.TestCase):
    def test_master_snapshot_transaction(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test';fixture=ROOT/'tests/fixtures/project-v1/mixed.ptg'
            original=fixture.read_bytes()
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/recovery_file_test.c',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe),str(fixture),tmp],check=True)
            self.assertEqual(fixture.read_bytes(),original)
            self.assertFalse([p for p in Path(tmp).iterdir() if p.name not in ('test','test.dSYM')])
