from pathlib import Path
import re,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class InvertLoop(unittest.TestCase):
    def test_reference_clock_and_cursor(self):
        source=(ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_text()
        table=[int(x) for x in re.search(r'mt_FunkTable\s+dc.b ([0-9,]+)',source).group(1).split(',')]
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'invert'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/invert_loop_test.c','src/core/invert_loop.c','-o',str(exe)],cwd=ROOT,check=True)
            lines=subprocess.check_output([str(exe)],text=True).splitlines()
        expected=[]
        for speed,increment in enumerate(table):
            if not increment:continue
            interval=(128+increment-1)//increment
            for count,tick in enumerate(range(interval-1,512,interval),1):
                expected.append(f'{speed} {tick} {2+count%6}')
        self.assertEqual(lines[:-1],expected)
        self.assertEqual(lines[-1],'INVERT clock PASS')
