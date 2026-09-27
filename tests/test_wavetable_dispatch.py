from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
from test_sampler_wavetable import EXTRA
ROOT=Path(__file__).resolve().parents[1]
DISPATCH=['src/editor/wavetable_song.c','src/editor/wavetable_voices.c','src/editor/wavetable_dispatch.c','src/core/amigus_voice_plan.c','src/core/amigus_render_voice.c','src/core/render.c','src/core/voice.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c']
class WavetableDispatch(unittest.TestCase):
    def test_resolved_sequence_commands(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'test')
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            # Rename only the real validator's translation unit, then count calls
            # through a test-only wrapper. No production instrumentation/shortcut.
            obj=str(Path(tmp)/'project.o')
            wrapper=Path(tmp)/'validation_count.c'
            wrapper.write_text('\n'.join([
                '#include "project.h"',
                'unsigned pt_test_project_validations;',
                'enum pt_project_result pt_project_validate_actual(const struct pt_project *,uint32_t *);',
                'enum pt_project_result pt_project_validate(const struct pt_project *p,uint32_t *n)',
                '{++pt_test_project_validations;return pt_project_validate_actual(p,n);}',
            ]))
            subprocess.run([*flags,'-Dpt_project_validate=pt_project_validate_actual','-c','src/core/project.c','-o',obj],cwd=ROOT,check=True)
            sources=[s for s in SOURCES[1:] if s!='src/core/project.c']
            subprocess.run([*flags,'-DPT_TEST_PROJECT_VALIDATION_COUNT','tests/wavetable_dispatch_test.c',*DISPATCH,*EXTRA,*sources,obj,str(wrapper),'-o',exe],cwd=ROOT,check=True)
            subprocess.run([exe],check=True)
