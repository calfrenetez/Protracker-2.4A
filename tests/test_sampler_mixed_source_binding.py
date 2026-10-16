"""Current mixed source graph; genuine factory/backend HOST checks only.

Historical once-only qualifiers retain their original source pins and results.
This driver neither imports nor relaxes them, and performs no target operations.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_sampler import SOURCES as SAMPLER_SOURCES
ROOT=Path(__file__).resolve().parents[1]
# The fixture includes the production mixed owner once, plus the original
# factory constructor suite; the retained donor main remains uncalled.
SOURCES=['tests/sampler_mixed_source_binding_test.c','src/core/mixed_readers_causal.c','src/core/amigus_trigger_levels.c',
    'src/core/mixed_scheduled_readers.c','src/core/elapsed_clock.c',
    'src/editor/sampler_paula.c','src/editor/sampler_wavetable.c','src/core/amigus_voice_plan.c',
    *SAMPLER_SOURCES[1:],*['src/core/'+n+'.c' for n in
    ['amigus_reservation','amigus_wavetable_cache','amigus_sample_ram','sample_cache','playback_pcm']]]
MARKER=('MIXED SOURCE BINDING PASS:495 cases;90 controls405 refusals;Paula signed8 and AmiGUS8/16 '
    'key binding before action/enqueue/active readiness;master and bus bytes preserved;independent retirement;SOFTWARE_ONLY')
FACTORY_MARKER=('SAMPLER MIXED FACTORY PASS:complete original constructor assertions;'
    'bounded completion marker;SOFTWARE_ONLY')
COMPONENTS={str(bits)+side:(bits,channels) for bits in (8,16,24)
            for side,channels in (('mono',1),('stereo',2))}
def component_marker(name):
    _,channels=COMPONENTS[name]
    counts='75 cases;15 controls60 refusals' if channels==1 else '90 cases;15 controls75 refusals'
    return ('MIXED SOURCE BINDING COMPONENT PASS:'+name+';'+counts+
            ';Paula and AmiGUS8/16;independent retirement;legacy constructor excluded;SOFTWARE_ONLY')
class MixedSourceBinding(unittest.TestCase):
    def check_fixture(self,defines,markers):
        with tempfile.TemporaryDirectory(prefix='pt-mixed-source-binding-') as folder:
            exe=Path(folder)/'fixture'
            r=subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-UNDEBUG',
                '-fsanitize=address,undefined','-Isrc/core','-I.',*defines,*SOURCES,'-o',str(exe)],
                cwd=ROOT,capture_output=True,timeout=120)
            self.assertEqual(r.returncode,0,(r.stdout+r.stderr).decode('utf-8','replace'))
            self.assertEqual((r.stdout,r.stderr),(b'',b''))
            r=subprocess.run([str(exe)],capture_output=True,timeout=60)
            self.assertEqual(r.returncode,0,(r.stdout+r.stderr).decode('utf-8','replace'))
            self.assertEqual(r.stderr,b'')
            lines=r.stdout.decode('ascii').splitlines()
            self.assertEqual(lines,markers)
    def test_genuine_mixed_factory_source_identity(self):
        self.check_fixture([],[FACTORY_MARKER,MARKER])
    def test_all_six_components_keep_actual_assertions_and_exact_counts(self):
        for name,(bits,channels) in COMPONENTS.items():
            with self.subTest(component=name):
                self.check_fixture(['-DPT_MIXED_BINDING_BITS='+str(bits),
                    '-DPT_MIXED_BINDING_CHANNELS='+str(channels)],[component_marker(name)])
if __name__=='__main__':unittest.main()
