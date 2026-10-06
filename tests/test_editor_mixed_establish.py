from pathlib import Path
import subprocess,tempfile,unittest
from test_editor_mixed import MIXED,prepare
ROOT=Path(__file__).resolve().parents[1]
WORKFLOW=['src/editor/workflow.c','src/editor/sample_range.c','src/editor/wave_summary.c',
          'src/core/sample_usage.c','src/core/event_resource.c','src/core/flow.c','src/core/pitch.c']
class EditorEstablish(unittest.TestCase):
    def test_indexed_editor_preborrow_cancellation(self):
        with tempfile.TemporaryDirectory() as tmp:
            sources,tree=prepare(tmp);exe=Path(tmp)/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Isrc/core','tests/editor_mixed_establish_test.c',
                *list(dict.fromkeys(['src/editor/editor_mixed.c','src/editor/editor_mixed_establish.c',
                    'src/editor/sampler_establish.c',*MIXED[1:],*sources,*WORKFLOW])),
                '-o',str(exe)],cwd=tmp,check=True)
            subprocess.run([str(exe)],check=True)
