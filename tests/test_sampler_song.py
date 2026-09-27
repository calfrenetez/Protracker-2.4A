from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class SamplerSong(unittest.TestCase):
    def test_song_owner_lifecycle(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'test'
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            obj=Path(tmp)/'project.o';pcmobj=Path(tmp)/'pcm.o';wrapper=Path(tmp)/'project_count.c'
            wrapper.write_text('\n'.join([
                '#include "project.h"','unsigned pt_test_project_validations,pt_test_pcm_validations;',
                'enum pt_pcm_result pt_pcm_validate_actual(const struct pt_pcm *);',
                'enum pt_pcm_result pt_pcm_validate(const struct pt_pcm *p)',
                '{++pt_test_pcm_validations;return pt_pcm_validate_actual(p);}',
                'enum pt_project_result pt_project_validate_actual(const struct pt_project *,uint32_t *);',
                'enum pt_project_result pt_project_validate(const struct pt_project *p,uint32_t *n)',
                '{++pt_test_project_validations;return pt_project_validate_actual(p,n);}',
            ]))
            subprocess.run([*flags,'-Dpt_project_validate=pt_project_validate_actual','-c','src/core/project.c','-o',str(obj)],cwd=ROOT,check=True)
            subprocess.run([*flags,'-Dpt_pcm_validate=pt_pcm_validate_actual','-c','src/core/pcm.c','-o',str(pcmobj)],cwd=ROOT,check=True)
            subprocess.run([*flags,'-DPT_TEST_PCM_VALIDATION_COUNT','-DPT_TEST_PROJECT_VALIDATION_COUNT','tests/sampler_song_test.c','src/editor/sampler_song.c','src/editor/sampler_studio.c','src/core/studio_song.c','src/core/studio_plan.c','src/core/render.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c','src/core/studio_mix.c','src/core/voice.c',*[s for s in SOURCES[1:] if s not in ('src/core/project.c','src/core/pcm.c')],str(obj),str(pcmobj),str(wrapper),'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
