from pathlib import Path
import resource
import signal
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
RENDER=['src/platform/render_file.c','src/platform/stem_file.c','src/core/stems.c','src/core/render.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c','src/core/voice.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class RenderFile(unittest.TestCase):
    def test_verified_stream_publication_and_io_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);binary=tmp/'files';cli=tmp/'render';out=tmp/'output';out.mkdir()
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            subprocess.run([*flags,'tests/render_file_test.c','src/core/wav.c',*RENDER,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary),str(out)],check=True)
            self.assertEqual(sorted(p.name for p in out.iterdir()),['race.wav','saved.wav'])
            fault_out=tmp/'read-faults';fault_out.mkdir()
            fault_binary=tmp/'read-faults-test'
            subprocess.run([*flags,'-Dread=pt_test_read','tests/render_file_test.c','tests/render_read_faults.c',
                            'src/core/wav.c',*RENDER,'-o',str(fault_binary)],cwd=ROOT,check=True)
            subprocess.run([str(fault_binary),str(fault_out)],check=True)
            self.assertEqual(sorted(p.name for p in fault_out.iterdir()),['race.wav','saved.wav'])
            subprocess.run([*flags,'tools/pt24g_render.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c',*RENDER,'-o',str(cli)],cwd=ROOT,check=True)
            fixture=ROOT/'evidence/enhanced-editor/dev28/native/speed.mod';wav=out/'cli.wav'
            args=[str(cli),str(fixture),str(wav),'--rate','44100','--bits','16','--gain','65536']
            result=subprocess.run(args,capture_output=True,text=True,check=True)
            self.assertIn('verified=1 new_file=1',result.stdout);data=wav.read_bytes()
            self.assertEqual(data[:4],b'RIFF');self.assertEqual(int.from_bytes(data[24:28],'little'),44100)
            self.assertEqual(int.from_bytes(data[34:36],'little'),16)
            self.assertEqual(subprocess.run(args,capture_output=True).returncode,20);self.assertEqual(wav.read_bytes(),data)
            # File-size cap is process-local. Ignore SIGXFSZ so the failed/short
            # write reaches normal cleanup instead of terminating the process.
            def limited():
                resource.setrlimit(resource.RLIMIT_FSIZE,(1024,1024));signal.signal(signal.SIGXFSZ,signal.SIG_IGN)
            failed=out/'short.wav'
            result=subprocess.run([str(cli),str(fixture),str(failed)],capture_output=True,text=True,preexec_fn=limited)
            self.assertEqual(result.returncode,20);self.assertIn('save_phase=4',result.stderr);self.assertFalse(failed.exists())
            self.assertFalse(list(out.glob('*.pttmp-*')))
            for tail in [['--bits','8'],['--rate','0'],['--tracks','0'],['--gain','-1'],['--pattern','4294967296'],['--unknown','1']]:
                result=subprocess.run([str(cli),str(fixture),str(failed),*tail],capture_output=True)
                self.assertEqual(result.returncode,20);self.assertFalse(failed.exists())
