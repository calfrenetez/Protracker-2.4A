from fractions import Fraction
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class FrameClock(unittest.TestCase):
    def test_fractional_tempo_changes_against_exact_rationals(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'clock'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/frame_clock_test.c','src/core/frame_clock.c','-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
            for rate in [1,44100,48000,192000]:
                result=subprocess.run([str(binary),str(rate),'100000'],check=True,capture_output=True,text=True)
                observed={int(parts[1]):parts for line in result.stdout.splitlines() if (parts:=line.split())[0]=='CLOCK'}
                exact=Fraction(0);fixed=0;previous_frames=0;hash_value=1469598103934665603
                for index in range(100000):
                    bpm=32+(index*97)%224;exact+=Fraction(rate*5,bpm*2);fixed+=(rate*5*(1<<31))//bpm
                    frames=(fixed>>32)-previous_frames;previous_frames=fixed>>32
                    hash_value=((hash_value^frames)*1099511628211)&((1<<64)-1)
                    if index+1 in observed:
                        parts=observed[index+1];whole=(int(parts[4])<<32)|int(parts[5]);fraction=int(parts[6])
                        self.assertEqual(int(parts[2]),bpm);self.assertEqual(int(parts[3]),frames)
                        self.assertEqual((whole<<32)|fraction,fixed)
                        error=exact-Fraction(fixed,1<<32)
                        self.assertGreaterEqual(error,0);self.assertLess(error,Fraction(index+1,1<<32))
                        self.assertLessEqual(abs(whole-int(exact)),1)
                self.assertIn(f'HASH {hash_value:016x}',result.stdout)
