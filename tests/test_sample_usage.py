from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/sample_usage_test.c','src/core/sample_usage.c','src/core/channels.c','src/core/project.c','src/core/pcm.c']
class SampleUsage(unittest.TestCase):
    def test_conservative_stored_references(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'sample-usage'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
