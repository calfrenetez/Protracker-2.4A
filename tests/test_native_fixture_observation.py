import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from native_fixture_observation import BEGIN,END,expected_cases,progress_snapshot,exception_observation
def full_log():
    lines=[BEGIN];n=0
    for group,bits,mode in expected_cases():
        for phase in ('BEGIN','END'):
            n+=1;lines.append(f'PTPROGRESS {n} {group} {phase} {bits} {mode}'.encode())
    return b'\n'.join([*lines,END])+b'\n'
class Observation(unittest.TestCase):
    def test_exact_full_and_partial_workload(self):
        log=full_log();r=progress_snapshot(log,True)
        self.assertEqual((r['completedCases'],r['progressRecords']),(342,684))
        prefix=BEGIN+b'\nPTPROGRESS 1 OWNER BEGIN 8 0\nPTPROGRESS 2 OWNER E'
        r=progress_snapshot(prefix);self.assertEqual(r['completedCases'],0)
        self.assertEqual(r['lastRecord'],('OWNER','BEGIN',8,0))
        self.assertFalse(r['ended'])
    def test_missing_gap_reorder_duplicate_or_false_completion_refuses(self):
        log=full_log()
        bad=[log.replace(b'PTPROGRESS 2 OWNER END 8 0\n',b''),
             log.replace(b'PTPROGRESS 2 OWNER END 8 0',b'PTPROGRESS 2 OWNER END 16 0'),
             BEGIN+b'\n'+END+b'\n',log+END+b'\n',log+BEGIN+b'\n',log[:-1],
             b'x'*65537,log.replace(b'PTPROGRESS 1 ',b'PTPROGRESS 0 ',1)]
        for data in bad:
            with self.subTest(size=len(data)):
                with self.assertRaises(ValueError):progress_snapshot(data,True)
    def test_nested_failure_keeps_leaf_and_traceback(self):
        try:
            raise ExceptionGroup('outer',[ExceptionGroup('inner',[AssertionError('finite fixture deadline')])])
        except ExceptionGroup as e:
            r=exception_observation(e)
        self.assertEqual(r['leaves'],[{'type':'AssertionError','message':'finite fixture deadline'}])
        self.assertIn('AssertionError: finite fixture deadline',r['traceback'])
