"""Synthetic HOST V3 data; no guest execution, admission or input."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from editor_pointer_context import parse_editor_pointer_context, EditorPointerContext
import test_editor_pointer_snapshot as original_fixture


class EditorPointerContextTests(unittest.TestCase):
    def body(self):
        pair = original_fixture.EditorPointerSnapshotTests().body(prefix='').replace(b'_V2 ', b'_V3 ')
        return pair + (
            b'PT_EDITOR_CONTEXT_V3 view_dx=129 view_dy=44 intuition_mouse_x=219 '
            b'intuition_mouse_y=425 active_screen=1 active_window=1 '
            b'window_pointer_override=0 window_pointer_width=0 window_pointer_height=0 '
            b'window_pointer_x_offset=0 window_pointer_y_offset=0\n')

    def test_context_preserves_data_without_mapping_or_input_authority(self):
        value = parse_editor_pointer_context(self.body())
        self.assertEqual(value.screen['mouse_x'], 219)
        self.assertEqual(value.context['view_dx'], 129)
        self.assertEqual(value.context['window_pointer_override'], 0)
        self.assertFalse(value.os_hotspot_mapping_accepted)
        self.assertFalse(value.input_authorized)

    def test_optional_ok_wrapper(self):
        self.assertEqual(parse_editor_pointer_context(b'[OK]\n' + self.body()),
                         parse_editor_pointer_context(self.body()))

    def test_inactive_and_different_global_position_remain_diagnostic_data(self):
        body = self.body().replace(b'active_screen=1', b'active_screen=0')
        body = body.replace(b'active_window=1', b'active_window=0')
        body = body.replace(b'intuition_mouse_x=219', b'intuition_mouse_x=-8')
        value = parse_editor_pointer_context(body)
        self.assertEqual(value.context['intuition_mouse_x'], -8)
        self.assertFalse(value.input_authorized)

    def test_pointer_override_offsets_are_signed_data_not_default_hotspot_proof(self):
        body = self.body().replace(b'window_pointer_override=0', b'window_pointer_override=1')
        body = body.replace(b'window_pointer_x_offset=0', b'window_pointer_x_offset=-1')
        value = parse_editor_pointer_context(body)
        self.assertEqual(value.context['window_pointer_x_offset'], -1)
        self.assertFalse(value.os_hotspot_mapping_accepted)

    def test_word_byte_and_boolean_ranges(self):
        for old, new in ((b'view_dx=129', b'view_dx=32768'),
                         (b'intuition_mouse_x=219', b'intuition_mouse_x=-32769'),
                         (b'active_screen=1', b'active_screen=2'),
                         (b'window_pointer_override=0', b'window_pointer_override=-1'),
                         (b'window_pointer_width=0', b'window_pointer_width=128'),
                         (b'window_pointer_y_offset=0', b'window_pointer_y_offset=-129')):
            with self.subTest(new=new), self.assertRaises(ValueError):
                parse_editor_pointer_context(self.body().replace(old, new))

    def test_existing_screen_window_and_title_checks_still_apply(self):
        for old, new in ((b'screen_width=640', b'screen_width=639'),
                         (b'border_left=0', b'border_left=1'),
                         (b'default_title_state=2', b'default_title_state=0'),
                         (b'window_flags=2304', b'window_flags=0')):
            with self.subTest(new=new), self.assertRaises(ValueError):
                parse_editor_pointer_context(self.body().replace(old, new))

    def test_no_old_mixed_duplicate_or_truncated_record_can_be_adopted(self):
        body = self.body()
        for other in (body.replace(b'_V3 ', b'_V2 '),
                      body.replace(b'PT_EDITOR_WINDOW_V3', b'PT_EDITOR_WINDOW_V2'),
                      body + body, body[:-8], b'[ERROR]\n' + body, body + b'address=1\n'):
            with self.subTest(other=other[:40]), self.assertRaises(ValueError):
                parse_editor_pointer_context(other)

    def test_canonical_numbers_field_order_and_no_extra_fields(self):
        for old, new in ((b'view_dx=129', b'view_dx=0129'),
                         (b'view_dx=129', b'view_dx=+129'),
                         (b'active_screen=1 active_window=1', b'active_window=1 active_screen=1'),
                         (b'window_pointer_y_offset=0', b'window_pointer_y_offset=0 address=1')):
            with self.subTest(new=new), self.assertRaises(ValueError):
                parse_editor_pointer_context(self.body().replace(old, new))

    def test_nonbytes_nonascii_and_oversize_refuse(self):
        for other in (self.body().decode(), self.body() + b'\0', b'X' * 3073):
            with self.subTest(kind=type(other)), self.assertRaises(ValueError):
                parse_editor_pointer_context(other)

    def test_constructor_cannot_grant_authority(self):
        with self.assertRaises(TypeError):
            EditorPointerContext({}, {}, {}, input_authorized=True)


if __name__ == '__main__':
    unittest.main()
