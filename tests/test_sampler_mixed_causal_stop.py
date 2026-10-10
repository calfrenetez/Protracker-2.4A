"""Finite factory-only STOP adapter HOST acceptance, separate from native timing/device stop."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/sampler_mixed_causal_stop_test.c', 'src/editor/sampler_mixed_readers.c', 'src/core/amigus_trigger_levels.c', 'src/core/mixed_readers_causal.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c']
ORACLE='SAMPLER CAUSAL STOP FACTORY HOST PASS:55 genuine heap cases;24 mixed4/12 or16card 8/16/24-master cache8/16 lifetimes;distinct genuine empty STOP binding;completed first plus original ACTIVE handles;one new C and zero new R/persistent pin/cache/upload;actual transfer and independent C/R/source quiet;full fixed handle/output/context and whole-covered misaligned binding/output STOP-open guards;allocation/alias/mutation/reentry/publication uncertainty;independent requested absolute frame/window assertions and master-save beforeimages;NO_CONTROLLER_PRODUCER_NATIVE_APERTURE_OR_DEVICE_STOP_QUALIFICATION\n'
class SamplerCausalStopHost(unittest.TestCase):
 def test_genuine_factory_stop_lifetimes(self):
  env=dict(os.environ,ASAN_OPTIONS='halt_on_error=1:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
  with tempfile.TemporaryDirectory(prefix='pt-factory-stop-') as temporary:
   exe=Path(temporary)/'fixture'
   built=subprocess.run(['/usr/bin/cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-UNDEBUG','-fsanitize=address,undefined','-Isrc/core','-I.',*SOURCES,'-o',str(exe)],cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True,timeout=120)
   self.assertEqual(built.stdout,b'');self.assertEqual(built.stderr,b'')
   actual=subprocess.run([str(exe)],cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True,timeout=30)
   self.assertEqual(actual.stderr,b'');self.assertEqual(actual.stdout,ORACLE.encode('ascii'))
if __name__=='__main__':unittest.main()
