"""Pure HOST refusal checks; no PNG dependency, target or Root observation."""
from dataclasses import asdict
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from editor_cursor_pixels import inspect_packed_rgb, CURSOR_COLOURS, PaintedBoundary


class CursorPixelsTests(unittest.TestCase):
    def image(self, points=(), size=(8, 8)):
        body = bytearray(size[0] * size[1] * 3)
        for x, y, colour in points:
            at = (y * size[0] + x) * 3
            body[at:at + 3] = bytes(colour)
        return bytes(body)

    def inspect(self, after, before=None, rectangle=(0, 0, 8, 8), **kw):
        return inspect_packed_rgb(self.image() if before is None else before,
                                  after, (8, 8), rectangle, **kw)

    def test_connected_boundary_and_grid_are_original_pixels_only(self):
        a, b = CURSOR_COLOURS
        report = self.inspect(self.image([(3, 2, a), (3, 3, a), (4, 3, b)]))
        self.assertEqual(report.first_painted_pixel, (3, 2))
        self.assertEqual(report.painted_bounds, (3, 2, 2, 2))
        self.assertEqual(report.ascii_rows, ('0.', '01'))
        self.assertEqual(report.painted_pixels, 3)
        self.assertFalse(report.os_hotspot_established)
        self.assertFalse(report.click_authorized)

    def test_unchanged_palette_pixels_do_not_become_cursor(self):
        image = self.image([(3, 2, CURSOR_COLOURS[0])])
        report = self.inspect(image, before=image)
        self.assertEqual(report.state, 'NO_NEW_PALETTE_COMPONENT')
        self.assertIsNone(report.first_painted_pixel)

    def test_disconnected_or_diagonal_colour_is_ambiguous(self):
        for second in ((6, 6), (4, 3)):
            with self.subTest(second=second), self.assertRaisesRegex(ValueError, 'Multiple'):
                self.inspect(self.image([(3, 2, CURSOR_COLOURS[0]),
                                         (*second, CURSOR_COLOURS[1])]))

    def test_outside_region_and_unlisted_colours_are_not_fitted(self):
        report = self.inspect(self.image([(7, 7, CURSOR_COLOURS[0]), (1, 1, (9, 9, 9))]),
                              rectangle=(0, 0, 4, 4))
        self.assertIsNone(report.first_painted_pixel)

    def test_region_fractional_boolean_empty_or_outside_refuses(self):
        for region in ((True, 0, 4, 4), (0., 0, 4, 4), (-1, 0, 4, 4),
                       (0, 0, 0, 4), (7, 0, 2, 2), (0, 7, 2, 2)):
            with self.subTest(region=region), self.assertRaises(ValueError):
                self.inspect(self.image(), rectangle=region)

    def test_image_size_body_type_and_lengths_refuse(self):
        for size, before, after in (((True, 8), b'', b''), ((0, 8), b'', b''),
                                    ((8, 8), bytearray(192), bytes(192)),
                                    ((8, 8), bytes(191), bytes(192)),
                                    ((2001, 1000), b'', b'')):
            with self.subTest(size=size), self.assertRaises(ValueError):
                inspect_packed_rgb(before, after, size, (0, 0, 1, 1))

    def test_palette_invalid_or_duplicate_refuses(self):
        for palette in ((), ((True, 1, 2),), ((256, 1, 2),),
                        ((1, 2),), ((1, 2, 3), (1, 2, 3))):
            with self.subTest(palette=palette), self.assertRaises(ValueError):
                self.inspect(self.image(), colours=palette)

    def test_region_and_component_budgets_refuse(self):
        size = (65, 65); empty = self.image(size=size)
        with self.assertRaisesRegex(ValueError, 'rectangle'):
            inspect_packed_rgb(empty, empty, size, (0, 0, 65, 65))
        colour = bytes(CURSOR_COLOURS[0]) * (33 * 33)
        with self.assertRaisesRegex(ValueError, 'component exceeds'):
            inspect_packed_rgb(bytes(len(colour)), colour, (33, 33), (0, 0, 33, 33))

    def test_no_expected_target_or_approval_is_accepted(self):
        with self.assertRaises(TypeError):
            self.inspect(self.image(), expected_hotspot=(3, 2))
        report = asdict(self.inspect(self.image()))
        self.assertFalse(report['click_authorized'])
        self.assertFalse(report['os_hotspot_established'])
        with self.assertRaises(TypeError):
            PaintedBoundary('fake', None, None, 0, (0, 0, 1, 1), (), click_authorized=True)


if __name__ == '__main__':
    unittest.main()
