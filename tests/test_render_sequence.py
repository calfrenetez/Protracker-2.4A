from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/render_sequence_test.c','src/core/studio_plan.c','src/core/studio_mix.c','src/core/render.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c',
                            'src/core/voice.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class RenderSequence(unittest.TestCase):
    def test_incremental_song(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'render'
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            obj=Path(tmp)/'pcm.o';wrapper=Path(tmp)/'pcm_count.c'
            wrapper.write_text('\n'.join([
                '#include "pcm.h"', 'unsigned pt_test_pcm_validations;',
                'enum pt_pcm_result pt_pcm_validate_actual(const struct pt_pcm *);',
                'enum pt_pcm_result pt_pcm_validate(const struct pt_pcm *p)',
                '{++pt_test_pcm_validations;return pt_pcm_validate_actual(p);}',
            ]))
            subprocess.run([*flags,'-Dpt_pcm_validate=pt_pcm_validate_actual','-c','src/core/pcm.c','-o',str(obj)],cwd=ROOT,check=True)
            subprocess.run([*flags,'-DPT_TEST_PCM_VALIDATION_COUNT',*[s for s in SOURCES if s!='src/core/pcm.c'],str(obj),str(wrapper),'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
