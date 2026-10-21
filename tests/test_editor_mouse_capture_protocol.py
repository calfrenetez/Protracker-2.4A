"""HOST-only strict selection observations; no GUI or shared harness calls."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from editor_blank_capture_protocol import BLANK_FRAME, InvalidBlankLog, recent_line
from editor_mouse_capture_protocol import (
    SELECTED_FRAME, first_selection_point, inspect_first_selection,
)


class FirstMouseProtocolTests(unittest.TestCase):
    label = 'blank-capture-firstmouse-r1'

    def setUp(self):
        self.baseline = (recent_line(self.label) + b'\n'
                         b'EDITOR READY channels=4 patterns=1 bitmap_bytes=163840 history_bytes=182388\n'
                         + BLANK_FRAME + b'\n')

    def inspect(self, tail=b'', **kwargs):
        return inspect_first_selection(self.baseline + tail, self.baseline,
                                       self.label, expected_history_bytes=182388, **kwargs)

    def test_complete_selection_and_refresh(self):
        observed = self.inspect(BLANK_FRAME+b'\n'+SELECTED_FRAME+b'\n'+SELECTED_FRAME+b'\n')
        self.assertEqual(observed.state, 'SELECTED_ROW4_CHANNEL1')
        self.assertEqual(observed.selected_frames, 2)

    def test_all_partial_selected_records_wait(self):
        for size in range(len(SELECTED_FRAME)+1):
            with self.subTest(size=size):
                self.assertEqual(self.inspect(SELECTED_FRAME[:size]).state, 'WAITING')

    def test_partial_after_complete_selection_is_not_accepted(self):
        self.assertEqual(self.inspect(SELECTED_FRAME+b'\nEDITOR FRAME').state, 'WAITING')

    def test_no_selection_remains_waiting(self):
        self.assertEqual(self.inspect().state, 'WAITING')
        self.assertEqual(self.inspect(BLANK_FRAME+b'\n').state, 'WAITING')

    def test_wrong_target_or_revision_or_dirty_is_refused(self):
        for old, new in [(b'row=4',b'row=3'),(b'channel=1',b'channel=2'),
                         (b'revision=0',b'revision=1'),(b'dirty=0',b'dirty=1'),
                         (b'panel=0',b'panel=2'),(b'status=READY',b'status=ERROR')]:
            with self.subTest(field=old), self.assertRaises(InvalidBlankLog):
                self.inspect(SELECTED_FRAME.replace(old,new)+b'\n')

    def test_reversion_and_unknown_actions_refused_after_success(self):
        for line in [BLANK_FRAME,b'EDITOR REPLAY active=1',b'EDITOR EXIT clean=1',b'ERROR',b'']:
            with self.subTest(line=line), self.assertRaises(InvalidBlankLog):
                self.inspect(SELECTED_FRAME+b'\n'+line+b'\n')

    def test_exact_baseline_identity_required(self):
        for body in [self.baseline.replace(b'row=0',b'row=1'),self.baseline[:-1],
                     self.baseline.replace(b'firstmouse-r1',b'firstmouse-r2')]:
            with self.subTest(body=body), self.assertRaises(InvalidBlankLog):
                inspect_first_selection(body,self.baseline,self.label,expected_history_bytes=182388)

    def test_complete_baseline_and_candidate_history_required(self):
        for baseline in [b'',self.baseline[:-1]]:
            with self.subTest(baseline=baseline), self.assertRaises(InvalidBlankLog):
                inspect_first_selection(baseline,baseline,self.label,expected_history_bytes=182388)
        with self.assertRaises(InvalidBlankLog):
            inspect_first_selection(self.baseline,self.baseline,self.label,expected_history_bytes=1)

    def test_frame_limit_and_ascii_bound(self):
        self.assertEqual(self.inspect((SELECTED_FRAME+b'\n')*31).state,'SELECTED_ROW4_CHANNEL1')
        for tail in [(SELECTED_FRAME+b'\n')*32,b'\r',b'\x00',b'x'*1025,b'x'*16385]:
            with self.subTest(size=len(tail)),self.assertRaises(InvalidBlankLog):
                self.inspect(tail)

    def test_actual_unscaled_capture_point(self):
        self.assertEqual(first_selection_point((76,38,640,512),(756,576),native_size=(640,512)),(294,462))
        # Channel1 starts188, note field ends248; row4 strip spans418..429.
        native=(294-76,462-38)
        self.assertTrue(188 <= native[0] < 248 and 418 <= native[1] < 430)

    def test_workbench_scale_and_clipped_rectangles_refused(self):
        for rect in [(76,38,640,256),(76,38,640,1024),(76,38,1280,512),
                     (-1,38,640,512),(120,38,640,512),(76,70,640,512)]:
            with self.subTest(rect=rect),self.assertRaises(ValueError):
                first_selection_point(rect,(756,576),native_size=(640,512))

    def test_scaled_workbench_with_same_painted_bounds_refused(self):
        # Workbench can paint the same640x512 rectangle at1x2 scale. The
        # independent native screen size must prevent treating it as the editor.
        for native in [(640,256),(640,True),(640,512.0),None]:
            with self.subTest(native=native),self.assertRaises(ValueError):
                first_selection_point((76,38,640,512),(756,576),native_size=native)

    def test_boolean_fractional_and_malformed_geometry_refused(self):
        for rect,size in [((True,38,640,512),(756,576)),((76,38.5,640,512),(756,576)),
                          ((76,38,640),(756,576)),((76,38,640,512),(True,576)),
                          ((76,38,640,512),(756,0)),('76,38,640,512',(756,576))]:
            with self.subTest(rect=rect,size=size),self.assertRaises(ValueError):
                first_selection_point(rect,size,native_size=(640,512))


if __name__ == '__main__':
    unittest.main()
