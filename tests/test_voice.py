from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Voice(unittest.TestCase):
    def test_high_resolution_voice_and_mixer(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'voice'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',
                            'tests/voice_test.c','src/core/voice.c','src/core/pcm.c','-o',str(binary)],cwd=ROOT,check=True)
            result=subprocess.run([str(binary)],check=True,capture_output=True,text=True)
            # Independent unbounded-integer trajectory, not the C incremental
            # phase state. Exercises wrap of a UINT64_MAX step and pingpong.
            q=1<<32;source=[(i*123457)%16000000-8000000 for i in range(64)]
            settings=[(1,30,7,29,(1<<64)-1,1),(0,31,2,31,q*9+q//3,2),(4,21,0,0,q//7,0)]
            gains=[(65536,0),(0,65536),(32768,32768)]
            def rounded(n,d):return (1 if n>=0 else -1)*((abs(n)+d//2)//d)
            hash_value=2166136261;clipped=0
            for frame in range(1024):
                sums=[0,0]
                for (start,end,a,b,step,loop),gain in zip(settings,gains):
                    phase=start*q+step*frame
                    if loop and phase>=a*q:
                        cycle=(b-a-(loop==2))*q*(2 if loop==2 else 1)
                        travel=(phase-a*q)%cycle
                        if loop==2:travel=min(travel,cycle-travel)
                        phase=a*q+travel
                    elif not loop and phase>=end*q:continue
                    index,fraction=divmod(phase,q);nxt=index+1
                    if loop and nxt==b:nxt=a if loop==1 else index
                    elif nxt==end:nxt=index
                    for side in range(2):
                        value=source[index*2+side]+rounded((source[nxt*2+side]-source[index*2+side])*fraction,q)
                        sums[side]+=value*gain[side]
                for total in sums:
                    value=rounded(total,65536);limited=max(-8388608,min(8388607,value));clipped+=value!=limited
                    for byte in (limited&0xffffffff).to_bytes(4,'little'):hash_value=((hash_value^byte)*16777619)&0xffffffff
            self.assertEqual((hash_value,clipped),(0x990c42f0,40))
            self.assertIn(f'VOICE partition PASS hash={hash_value:08x} clips={clipped}',result.stdout)
            print(result.stdout,end='')
