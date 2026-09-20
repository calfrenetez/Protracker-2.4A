from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from prepare_flow_trace import prepare_flow_trace
from prepare_replay import prepare_replay
from test_flow_emulator import decode_trace

class FlowTrace(unittest.TestCase):
    def test_diagnostic_is_separate_and_anchor_checked(self):
        raw=(ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_bytes()
        wrapper=(ROOT/'src/native/replay_abi.s').read_bytes()
        normal=prepare_replay(raw,wrapper);trace=prepare_flow_trace(raw,wrapper)
        self.assertNotIn(b'pt_flow_record',normal)
        self.assertEqual(trace.count(b'\tBSR.W pt_flow_record'),1)
        self.assertEqual(trace.count(b'\tADDQ.W #1,_pt_flow_fetches'),1)
        self.assertIn(b'\tBEQ.W\tpt_flow_restore',trace)
        self.assertEqual(prepare_replay(raw,wrapper),normal)
        with self.assertRaises(ValueError):
            prepare_flow_trace(raw.replace(b'mt_exit\tMOVEM.L',b'mt_exit MOVEM.L'),wrapper)

    def test_trace_validation_rejects_partial_and_inconsistent_records(self):
        record=bytearray(36);record[:4]=(1).to_bytes(4,'big')
        def log(data=record,reason='native-stop',count=1):
            return f'FLOW schema=1 bytes=36 count={count} reason={reason}\nT {data.hex()}\nFLOW PASS dma=0\n'
        self.assertEqual(decode_trace(log(),10),(bytes(record),'native-stop'))
        enabled=bytearray(record);enabled[14]=255
        self.assertEqual(decode_trace(log(enabled,'tick-budget'),1),(bytes(enabled),'tick-budget'))
        for bad,budget in [(log()[:-4],10),(log(count=2),10),(log(enabled),10),
                           (log(record,'tick-budget'),1),(log(enabled,'tick-budget'),2),
                           (log(bytearray(36)),10),(log().replace('00\n','zz\n'),10),
                           (log().replace('native-stop','deadline'),10)]:
            with self.assertRaises(ValueError):decode_trace(bad,budget)
