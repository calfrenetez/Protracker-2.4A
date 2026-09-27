from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
from test_sampler_wavetable import EXTRA
ROOT=Path(__file__).resolve().parents[1]
DISPATCH=['src/core/elapsed_clock.c','src/editor/wavetable_song.c','src/editor/wavetable_voices.c','src/editor/wavetable_dispatch.c','src/core/amigus_voice_plan.c','src/core/amigus_render_voice.c','src/core/render.c','src/core/voice.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c']
class WavetableDispatch(unittest.TestCase):
    def test_resolved_sequence_commands(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'test')
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            # Rename only the real validator's translation unit, then count calls
            # through a test-only wrapper. No production instrumentation/shortcut.
            obj=str(Path(tmp)/'project.o');pcmobj=str(Path(tmp)/'pcm.o');voiceobj=str(Path(tmp)/'voice.o')
            wrapper=Path(tmp)/'validation_count.c'
            wrapper.write_text('\n'.join([
                '#include "project.h"',
                '#include "amigus_render_voice.h"',
                'unsigned pt_test_render_voice_plans,pt_test_render_controls,pt_test_render_restores;',
                'int pt_amigus_render_restore_actual(const struct pt_voice *,unsigned,const uint32_t *,const struct pt_playback_format *,uint32_t,uint32_t,struct pt_amigus_restore_plan *);',
                'int pt_amigus_render_restore(const struct pt_voice *v,unsigned r,const uint32_t *g,const struct pt_playback_format *f,uint32_t a,uint32_t b,struct pt_amigus_restore_plan *o)',
                '{++pt_test_render_restores;return pt_amigus_render_restore_actual(v,r,g,f,a,b,o);}',
                'int pt_amigus_render_control_actual(uint64_t,unsigned,const uint32_t *,uint32_t *,uint16_t *,uint16_t *);',
                'int pt_amigus_render_control(uint64_t s,unsigned r,const uint32_t *g,uint32_t *f,uint16_t *l,uint16_t *h)',
                '{++pt_test_render_controls;return pt_amigus_render_control_actual(s,r,g,f,l,h);}',
                'int pt_amigus_render_voice_actual(const struct pt_voice *,unsigned,const uint32_t *,const struct pt_playback_format *,uint32_t,uint32_t,struct pt_amigus_voice_plan *);',
                'int pt_amigus_render_voice(const struct pt_voice *v,unsigned r,const uint32_t *g,const struct pt_playback_format *f,uint32_t a,uint32_t b,struct pt_amigus_voice_plan *o)',
                '{++pt_test_render_voice_plans;return pt_amigus_render_voice_actual(v,r,g,f,a,b,o);}',
                'unsigned pt_test_project_validations,pt_test_pcm_validations;',
                'enum pt_pcm_result pt_pcm_validate_actual(const struct pt_pcm *);',
                'enum pt_pcm_result pt_pcm_validate(const struct pt_pcm *p)',
                '{++pt_test_pcm_validations;return pt_pcm_validate_actual(p);}',
                'enum pt_project_result pt_project_validate_actual(const struct pt_project *,uint32_t *);',
                'enum pt_project_result pt_project_validate(const struct pt_project *p,uint32_t *n)',
                '{++pt_test_project_validations;return pt_project_validate_actual(p,n);}',
            ]))
            subprocess.run([*flags,'-Dpt_project_validate=pt_project_validate_actual','-c','src/core/project.c','-o',obj],cwd=ROOT,check=True)
            subprocess.run([*flags,'-Dpt_pcm_validate=pt_pcm_validate_actual','-c','src/core/pcm.c','-o',pcmobj],cwd=ROOT,check=True)
            subprocess.run([*flags,'-Dpt_amigus_render_voice=pt_amigus_render_voice_actual','-Dpt_amigus_render_control=pt_amigus_render_control_actual','-Dpt_amigus_render_restore=pt_amigus_render_restore_actual','-c','src/core/amigus_render_voice.c','-o',voiceobj],cwd=ROOT,check=True)
            sources=[s for s in SOURCES[1:] if s not in ('src/core/project.c','src/core/pcm.c')]
            subprocess.run([*flags,'-DPT_TEST_PCM_VALIDATION_COUNT','-DPT_TEST_PROJECT_VALIDATION_COUNT','-DPT_TEST_RENDER_VOICE_COUNT','tests/wavetable_dispatch_test.c',*[s for s in DISPATCH if s!='src/core/amigus_render_voice.c'],*EXTRA,*sources,obj,pcmobj,voiceobj,str(wrapper),'-o',exe],cwd=ROOT,check=True)
            subprocess.run([exe],check=True)
