"""Synthetic V2 HOST parser cases. These never admit or execute an observer."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from editor_pointer_snapshot import parse_editor_pointer_snapshot, SCREEN_TITLE, EditorPointerSnapshot


class EditorPointerSnapshotTests(unittest.TestCase):
    def body(self, screen='', window='', prefix='[OK]\n'):
        first = ('PT_EDITOR_SCREEN_V2 screen_width=640 screen_height=512 mouse_x=219 mouse_y=425 '
                 'left=0 top=0 viewport_dx=0 viewport_dy=0 viewport_modes=32772 screen_flags=15 '
                 'default_title_state=2 default_title_hex=' + SCREEN_TITLE.encode().hex())
        second = ('PT_EDITOR_WINDOW_V2 left=0 top=0 width=640 height=512 mouse_x=219 mouse_y=425 '
                  'border_left=0 border_top=0 border_right=0 border_bottom=0 window_flags=2304 '
                  'title_state=0 title_hex=-')
        if screen: first = first.replace(*screen)
        if window: second = second.replace(*window)
        return (prefix + first + '\n' + second + '\n').encode()

    def test_absent_window_title_is_explicit_valid_data_not_hotspot_proof(self):
        value = parse_editor_pointer_snapshot(self.body())
        self.assertIsNone(value.window['title'])
        self.assertEqual(value.screen['mouse_x'], 219)
        self.assertFalse(value.os_hotspot_mapping_accepted)
        self.assertFalse(value.input_authorized)

    def test_empty_window_title_is_distinguished_from_absent(self):
        value = parse_editor_pointer_snapshot(self.body(window=('title_state=0', 'title_state=1')))
        self.assertEqual(value.window['title'], '')

    def test_unwrapped_exact_stdout_is_allowed(self):
        parse_editor_pointer_snapshot(self.body(prefix=''))

    def test_workbench_dimensions_title_or_noncustom_screen_refuse(self):
        for pair in (('screen_height=512', 'screen_height=256'),
                     ('title_hex=' + SCREEN_TITLE.encode().hex(), 'title_hex=416d696761444f53'),
                     ('screen_flags=15', 'screen_flags=1')):
            with self.subTest(pair=pair), self.assertRaises(ValueError):
                parse_editor_pointer_snapshot(self.body(screen=pair))

    def test_title_state_hex_mismatch_and_truncation_refuse(self):
        for pair in (('title_state=0', 'title_state=2'), ('title_hex=-', 'title_hex=61'),
                     ('title_state=0', 'title_state=3'), ('title_state=0 title_hex=-', 'title_state=2 title_hex=abc')):
            with self.subTest(pair=pair), self.assertRaises(ValueError):
                parse_editor_pointer_snapshot(self.body(window=pair))

    def test_modal_title_borders_origin_or_wrong_size_refuse(self):
        for pair in (('title_state=0 title_hex=-', 'title_state=2 title_hex=4d6f64616c'),
                     ('border_top=0', 'border_top=11'), ('left=0', 'left=1'),
                     ('width=640', 'width=639'), ('window_flags=2304', 'window_flags=0')):
            with self.subTest(pair=pair), self.assertRaises(ValueError):
                parse_editor_pointer_snapshot(self.body(window=pair))

    def test_incoherent_or_offscreen_pointer_refuses(self):
        for pair in (('mouse_x=219', 'mouse_x=220'), ('mouse_y=425', 'mouse_y=-1')):
            with self.subTest(pair=pair), self.assertRaises(ValueError):
                parse_editor_pointer_snapshot(self.body(window=pair))
        for pair in (('mouse_x=219', 'mouse_x=640'), ('mouse_y=425', 'mouse_y=512')):
            with self.subTest(pair=pair), self.assertRaises(ValueError):
                parse_editor_pointer_snapshot(self.body(screen=pair))

    def test_wrong_modes_flags_ranges_and_source_origin_refuse(self):
        for pair in (('viewport_modes=32772', 'viewport_modes=32768'),
                     ('viewport_modes=32772', 'viewport_modes=32804'),
                     ('screen_flags=15', 'screen_flags=65551'),
                     ('viewport_dx=0', 'viewport_dx=32768'), ('top=0', 'top=1')):
            with self.subTest(pair=pair), self.assertRaises(ValueError):
                parse_editor_pointer_snapshot(self.body(screen=pair))
        with self.assertRaises(ValueError):
            parse_editor_pointer_snapshot(self.body(window=('window_flags=2304', 'window_flags=4294967296')))

    def test_duplicate_record_noise_error_and_partial_reply_refuse(self):
        body = self.body()
        for other in (body + body, b'[ERROR]\n' + body, body + b'noise\n',
                      body[:-10], b'PT_EDITOR_POINTER_V2 FAIL invalid_screen_window_or_title\n'):
            with self.subTest(other=other[:20]), self.assertRaises(ValueError):
                parse_editor_pointer_snapshot(other)

    def test_noncanonical_numbers_extra_fields_binary_and_oversize_refuse(self):
        for other in (self.body(screen=('mouse_x=219', 'mouse_x=0219')),
                      self.body(screen=('mouse_x=219', 'mouse_x=+219')),
                      self.body(window=('title_hex=-', 'extra=1 title_hex=-')),
                      self.body() + b'\x00', b'X' * 2049, self.body().decode()):
            with self.subTest(kind=type(other)), self.assertRaises(ValueError):
                parse_editor_pointer_snapshot(other)

    def test_parser_has_no_input_authority_constructor_override(self):
        with self.assertRaises(TypeError):
            EditorPointerSnapshot({}, {}, input_authorized=True)


if __name__ == '__main__': unittest.main()
