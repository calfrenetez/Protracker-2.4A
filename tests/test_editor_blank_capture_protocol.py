from pathlib import Path
import hashlib
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from editor_blank_capture_protocol import (
    BLANK_FRAME, MAX_HISTORY_BYTES, MAX_LINE_BYTES, MAX_LOG_BYTES, MAX_CLEAN_FRAMES,
    InvalidBlankLog, inspect_blank_log as raw_inspect, recent_line,
    require_unchanged_blank_snapshots as raw_pair,
)

LABEL = 'blank-capture-host-fixture'
RECENT = b'EDITOR RECENT loaded=0 prefix=Dev:Tests/blank-capture-host-fixture/recent'
# Synthetic protocol fixture, not an observed native result or struct-size proof.
READY = b'EDITOR READY channels=4 patterns=1 bitmap_bytes=163840 history_bytes=123456'
BODY = RECENT+b'\n'+READY+b'\n'+BLANK_FRAME+b'\n'


def inspect_blank_log(body, label):
    return raw_inspect(body, label, expected_history_bytes=123456)


def require_unchanged_blank_snapshots(before, after, label):
    return raw_pair(before, after, label, expected_history_bytes=123456)


class BlankCaptureProtocol(unittest.TestCase):
    def test_exact_clean_prefix_only(self):
        self.assertEqual(recent_line(LABEL), RECENT)
        result = inspect_blank_log(BODY, LABEL)
        self.assertEqual(result.state, 'CLEAN_BLANK_PREFIX')
        self.assertEqual(result.history_bytes, 123456)
        self.assertEqual(result.byte_count, len(BODY))
        self.assertEqual(result.sha256, hashlib.sha256(BODY).hexdigest())

    def test_every_strict_prefix_waits(self):
        for end in range(len(BODY)):
            with self.subTest(end=end):
                self.assertEqual(inspect_blank_log(BODY[:end], LABEL).state, 'WAITING')

    def test_clean_marker_never_hides_later_activity(self):
        for extra in [b'EDITOR REPLAY active=1 ticks=1\n', b'EDITOR REPLAY active=1',
                      b'EDITOR EXIT HELD\n', b'EDITOR EXIT clean\n', b'unknown\n',
                      b'EDITOR FRAME row=1 channel=0\n', b'\n', b' ']:
            with self.subTest(extra=extra):
                with self.assertRaises(InvalidBlankLog): inspect_blank_log(BODY+extra, LABEL)

    def test_wrong_identity_dimensions_and_counts(self):
        changes = [(b'loaded=0', b'loaded=1'), (b'host-fixture', b'other-run'),
                   (b'channels=4', b'channels=8'), (b'patterns=1', b'patterns=2'),
                   (b'bitmap_bytes=163840', b'bitmap_bytes=81920'),
                   (b'history_bytes=123456', b'history_bytes=0'),
                   (b'history_bytes=123456', b'history_bytes=0123456'),
                   (b'history_bytes=123456', b'history_bytes='+str(MAX_HISTORY_BYTES+1).encode())]
        for old,new in changes:
            with self.subTest(old=old,new=new):
                with self.assertRaises(InvalidBlankLog): inspect_blank_log(BODY.replace(old,new), LABEL)

    def test_frame_must_remain_blank_clean_and_unmodal(self):
        for old,new in [(b'row=0',b'row=1'), (b'channel=0',b'channel=1'),
                        (b'revision=0',b'revision=1'), (b'dirty=0',b'dirty=1'),
                        (b'panel=0',b'panel=1'), (b'status=READY',b'status=ERROR')]:
            with self.subTest(new=new):
                with self.assertRaises(InvalidBlankLog): inspect_blank_log(BODY.replace(old,new), LABEL)

    def test_duplicate_or_reordered_records_refuse(self):
        records = [RECENT,READY,BLANK_FRAME]
        for rows in [records+[RECENT], [READY,RECENT,BLANK_FRAME],
                     [RECENT,READY,READY,BLANK_FRAME], [RECENT,BLANK_FRAME],
                     [RECENT,b'',READY,BLANK_FRAME], [b'EDITOR: failed',*records]]:
            with self.subTest(rows=rows):
                with self.assertRaises(InvalidBlankLog): inspect_blank_log(b'\n'.join(rows)+b'\n',LABEL)

    def test_unexpected_partial_refuses_before_lf(self):
        for body in [b'EDITOR REPLAY', RECENT+b'\nEDITOR READY channels=8',
                     RECENT+b'\n'+READY+b'\nEDITOR FRAME row=1']:
            with self.subTest(body=body):
                with self.assertRaises(InvalidBlankLog): inspect_blank_log(body,LABEL)

    def test_bytes_and_control_character_bounds(self):
        for body in [BODY.decode(), bytearray(BODY), b'x'*(MAX_LOG_BYTES+1),
                     b'x'*(MAX_LINE_BYTES+1), BODY.replace(b'\n',b'\r\n'),
                     BODY+b'\0', BODY+b'\t', BODY+b'\x7f', BODY+b'\xff']:
            with self.subTest(kind=type(body),length=len(body)):
                with self.assertRaises(InvalidBlankLog): inspect_blank_log(body,LABEL)

    def test_label_cannot_inject_path_or_commands(self):
        for label in ['', '../x', 'blank-capture-', 'blank-capture-X',
                      'blank-capture-a/b', 'blank-capture-a\nEcho bad',
                      'blank-capture-'+('a'*49), None]:
            with self.subTest(label=label):
                with self.assertRaises(InvalidBlankLog): inspect_blank_log(b'',label)

    def test_two_snapshots_require_complete_unchanged_bytes(self):
        self.assertEqual(require_unchanged_blank_snapshots(BODY,BODY,LABEL).state,
                         'CLEAN_BLANK_PREFIX')
        for first,second in [(BODY[:-1],BODY), (BODY,BODY[:-1]),
                             (BODY,BODY.replace(b'123456',b'123457')),
                             (BODY,BODY+b'EDITOR REPLAY active=1\n')]:
            with self.subTest(first=len(first),second=len(second)):
                with self.assertRaises(InvalidBlankLog):
                    require_unchanged_blank_snapshots(first,second,LABEL)

    def test_ready_like_malformed_record_cannot_be_ignored(self):
        # Valid-shaped duplicates and malformed rows with trailing fields must
        # both refuse. The old filter missed the latter and accepted its old frame.
        for tail in [b'EDITOR READY channels=4 patterns=1 bitmap_bytes=1 history_bytes=1\n',
                     b'EDITOR READY channels=4 patterns=1 bitmap_bytes=163840 history_bytes=1 extra\n']:
            with self.assertRaises(InvalidBlankLog): inspect_blank_log(BODY+tail,LABEL)

    def test_history_size_is_bound_to_the_candidate(self):
        with self.assertRaises(InvalidBlankLog):
            raw_inspect(BODY,LABEL,expected_history_bytes=123457)
        with self.assertRaises(TypeError): raw_inspect(BODY,LABEL)
        for value in [True, 0, -1, MAX_HISTORY_BYTES+1, '123456', None]:
            with self.subTest(value=value):
                with self.assertRaises(InvalidBlankLog):
                    raw_inspect(BODY,LABEL,expected_history_bytes=value)

    def test_identical_clean_refresh_frames_are_bounded(self):
        # Actual main sets frame_log after IDCMP_REFRESHWINDOW too; an identical
        # clean repaint is allowed without accepting a changed state or error.
        body=RECENT+b'\n'+READY+b'\n'+(BLANK_FRAME+b'\n')*MAX_CLEAN_FRAMES
        self.assertEqual(inspect_blank_log(body,LABEL).clean_frames,MAX_CLEAN_FRAMES)
        with self.assertRaises(InvalidBlankLog):
            inspect_blank_log(body+BLANK_FRAME+b'\n',LABEL)
        with self.assertRaises(InvalidBlankLog):
            require_unchanged_blank_snapshots(BODY,BODY+BLANK_FRAME+b'\n',LABEL)


if __name__ == '__main__': unittest.main()
