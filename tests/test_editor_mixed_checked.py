from pathlib import Path
import subprocess,tempfile,unittest
from test_editor_mixed import MIXED,prepare
from test_editor_mixed_establish import WORKFLOW
ROOT=Path(__file__).resolve().parents[1]
CHECKED=['src/editor/editor_mixed.c','src/editor/editor_mixed_checked.c','src/editor/editor_mixed_establish.c',
         'src/editor/sampler_establish.c','src/editor/mixed_owner_established.c',
         'src/editor/sampler_prepare_project.c','src/editor/sampler_prepare_memory.c']
class EditorChecked(unittest.TestCase):
    def test_indexed_editor_checked_owner_finish_after_transport(self):
        with tempfile.TemporaryDirectory() as tmp:
            sources,tree=prepare(tmp);exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core','tests/editor_mixed_checked_test.c',
                *list(dict.fromkeys([*CHECKED,*MIXED[1:],*sources,*WORKFLOW])),
                '-o',str(exe)],cwd=tmp,check=True)
            subprocess.run([str(exe)],check=True)
