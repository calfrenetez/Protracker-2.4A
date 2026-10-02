from pathlib import Path
import subprocess,tempfile,unittest
from test_sampler import SOURCES
from test_paula_preflight import SOURCES as PREFLIGHT
ROOT=Path(__file__).resolve().parents[1]
COMMON=list(dict.fromkeys(['src/editor/paula_song.c','src/core/elapsed_clock.c',
    'src/editor/paula_dispatch.c','src/editor/paula_voices.c','src/editor/sampler_paula.c',
    'src/core/sample_cache.c','src/core/playback_pcm.c',*SOURCES[1:],*PREFLIGHT[1:]]))
class PaulaSongSelected(unittest.TestCase):
    def test_full_per_bit_native_selection(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp);wrapper=folder/'selected.c'
            wrapper.write_text('#define PT_TEST_SONG_EXEC\n#include "paula_song_test.c"\nint main(void){return paula_song_fixture();}\n')
            for bits in (8,16,24):
                with self.subTest(bits=bits):
                    exe=folder/('song'+str(bits))
                    subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined','-Itests','-Isrc/core',
                        '-DPT_TEST_SONG_EXEC_BITS='+str(bits),str(wrapper),*COMMON,'-o',str(exe)],cwd=ROOT,check=True)
                    result=subprocess.run([str(exe)],capture_output=True,text=True,check=True)
                    markers=[line for line in result.stdout.splitlines() if line.startswith('PAULA SONG CASE PASS:')]
                    self.assertEqual(markers,['PAULA SONG CASE PASS: bits='+str(bits)+' full per-bit assertions and runtime bounds'])
                    self.assertIn('PAULA SONG PASS:',result.stdout)
                    print(result.stdout,end='')
    def test_invalid_or_non_native_selection_refuses(self):
        with tempfile.TemporaryDirectory() as tmp:
            wrapper=Path(tmp)/'refused.c'
            for native,bits,message in ((True,32,'must be 8, 16 or 24'),(False,8,'requires the native fixture entry')):
                with self.subTest(native=native,bits=bits):
                    wrapper.write_text(('#define PT_TEST_SONG_EXEC\n' if native else '')+'#include "paula_song_test.c"\n')
                    result=subprocess.run(['cc','-std=c99','-Itests','-Isrc/core',
                        '-DPT_TEST_SONG_EXEC_BITS='+str(bits),'-fsyntax-only',str(wrapper)],cwd=ROOT,capture_output=True,text=True)
                    self.assertNotEqual(result.returncode,0)
                    self.assertIn(message,result.stderr)
