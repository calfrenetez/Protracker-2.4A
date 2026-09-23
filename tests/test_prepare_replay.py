from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from prepare_replay import prepare_replay
class ReplayAdapter(unittest.TestCase):
    def test_adapter_ownership_and_unchanged_effect_routines(self):
        raw=(ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_bytes()
        wrapper=(ROOT/'src/native/replay_abi.s').read_bytes()
        adapted=prepare_replay(raw,wrapper)
        self.assertNotIn(b'INCBIN "music.mod"',adapted)
        self.assertNotIn(b'BEQ\tResetCIAInt',adapted)
        self.assertIn(b'RemInt\tLEA\tMusicIntServer(PC),A1\n\tJSR\tRemICRVector',adapted)
        from prepare_asm import prepare
        # The effect/timing implementation, including the finetune tables, is
        # unchanged apart from equivalent opcode spelling and the final volume
        # and DMA observation hooks. Neither tracker volume nor effect state is gated.
        original=raw[raw.index(b'\nmt_PlayVoice\n'):raw.index(b'\n\tCNOP 0,4\nmt_audchan1temp')]
        self.assertEqual(raw.count(b'\tMOVE.W\tD0,8(A5)'),6)
        self.assertEqual(adapted.count(b'BSR.W\tpt_write_volume'),6)
        self.assertIn(prepare(original.replace(b'\tMOVE.W\tD0,8(A5)',b'\tBSR.W\tpt_write_volume').replace(b'\tMOVE.W\tD0,$DFF096',b'\tBSR.W\tpt_start_dma'))[0],adapted)
        with self.assertRaises(ValueError):
            prepare_replay(raw.replace(b'\tMOVE.W\tD0,8(A5)',b'CHANGED',1),wrapper)
        for anchor in [b'RemInt\tLEA',b'\tLEA\tmt_data,A0',b'\nmt_GetNewNote\n']:
            with self.assertRaises(ValueError):prepare_replay(raw.replace(anchor,b'CHANGED',1),wrapper)
