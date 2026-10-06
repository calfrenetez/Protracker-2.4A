from pathlib import Path
import subprocess,tempfile,unittest
from test_editor_mixed import MIXED,prepare
from test_editor_mixed_checked import CHECKED
from test_editor_mixed_establish import WORKFLOW
BRIDGES=['src/editor/editor_mixed_bridges.c',*CHECKED]
class EditorBridges(unittest.TestCase):
    def test_metadata_only_editor_bridges_then_full_checked_validation(self):
        with tempfile.TemporaryDirectory() as tmp:
            sources,tree=prepare(tmp);exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core','tests/editor_mixed_bridges_test.c',
                *list(dict.fromkeys([*BRIDGES,*MIXED[1:],*sources,*WORKFLOW])),
                '-o',str(exe)],cwd=tmp,check=True)
            subprocess.run([str(exe)],check=True)
