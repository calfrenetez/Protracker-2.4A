from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class VoiceSegment(unittest.TestCase):
    def test_initial_segment_and_repeat_handoff(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'segment'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/voice_segment_test.c','src/core/voice.c','src/core/pcm.c','-o',str(binary)],cwd=ROOT,check=True)
            result=subprocess.check_output([str(binary)],text=True)
            q=1<<32
            for k,step in enumerate([1,q//3,q,9*q,(1<<64)-1]):
                digest=2166136261
                for frame in range(100):
                    travel=frame*step
                    index=5+travel//q if travel<2*q else 1+((travel-2*q)%(3*q))//q
                    digest=((digest^(index*100))*16777619)&0xffffffff
                self.assertIn(f'SEGMENT trajectory {k} hash={digest:08x}',result)
            print(result,end='')
