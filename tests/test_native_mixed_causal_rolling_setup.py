"""HOST sanitizer checks for independently closed early rolling components.

These are constructor/adoption/one-step cancellation components, not full-song
or native progress qualification. Real processes, devices and sockets are unused.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = ('tests/native_exec_mixed_causal_rolling_setup_test.c', *base.SOURCES[1:])


class RollingSetupHost(unittest.TestCase):
    def test_construct_adopt_and_one_step_close_with_zero_owned_bytes(self):
        before = base.preservation_snapshot()
        base.evidence('setup-before-preservation.json', before)
        pins = {}
        original_sources = base.SOURCES
        try:
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-rolling-setup-') as directory:
                private = Path(directory)
                pins = base.source_snapshot(private)
                font_pin, font = base.ordinary_pin(base.ROOT / 'vendor/pt23f/raw/ptfont.raw')
                self.assertEqual(font_pin['sha256'], base.RAW_FONT_SHA256)
                header = private / 'pt_font.h'
                header.write_text('static const unsigned char pt_font[580] = {' +
                                  ','.join(str(value) for value in font) + '};\n')
                self.assertEqual(base.ordinary_pin(header)[0]['sha256'], base.FONT_HEADER_SHA256)
                env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                       'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                       'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                for mode in range(3):
                    with self.subTest(mode=mode):
                        fixture = private / ('setup-' + str(mode))
                        result = subprocess.run(['/usr/bin/cc', *base.CFLAGS,
                            '-DPT_ROLLING_SETUP_MODE=' + str(mode), *SOURCES, '-o', str(fixture)],
                            cwd=private, env=env, capture_output=True, timeout=120)
                        base.evidence(f'setup-{mode}-compile.stdout', result.stdout)
                        base.evidence(f'setup-{mode}-compile.stderr', result.stderr)
                        self.assertEqual(result.returncode, 0, result.stderr.decode('utf8', 'replace'))
                        self.assertEqual((result.stdout, result.stderr), (b'', b''))
                        owned = private / ('owned-' + str(mode));owned.mkdir()
                        progress = owned / 'progress.log';progress.write_bytes(b'NOT STARTED\n')
                        actual = subprocess.run([str(fixture), str(progress)], cwd=private,
                                                env=env, capture_output=True, timeout=30)
                        base.evidence(f'setup-{mode}-fixture.stdout', actual.stdout)
                        base.evidence(f'setup-{mode}-fixture.stderr', actual.stderr)
                        base.evidence(f'setup-{mode}-progress.log', progress.read_bytes())
                        self.assertEqual(actual.returncode, 0, actual.stderr.decode('utf8', 'replace'))
                        self.assertEqual(actual.stderr, b'')
                        self.assertEqual(actual.stdout, f'ROLLING SETUP NATIVE PASS:mode={mode};MODELED_ONLY\n'.encode())
                        steps = 1 if mode == 2 else 0
                        self.assertEqual(progress.read_bytes(), f'ROLLING SETUP DONE mode={mode} steps={steps} phase=0\n'.encode())
                        # A foreign/missing argument never starts a component
                        # or overwrites an unrecognized progress file.
                        progress.write_bytes(b'FOREIGN CONTENT\n')
                        bad = subprocess.run([str(fixture), str(progress)], cwd=private,
                                             env=env, capture_output=True, timeout=30)
                        self.assertEqual((bad.returncode, bad.stdout, bad.stderr), (20, b'', b''))
                        self.assertEqual(progress.read_bytes(), b'FOREIGN CONTENT\n')
                        missing = subprocess.run([str(fixture)], cwd=private, env=env,
                                                 capture_output=True, timeout=30)
                        self.assertEqual((missing.returncode, missing.stdout, missing.stderr), (20, b'', b''))
                base.verify_sources(pins)
        finally:
            base.SOURCES = original_sources
            base.verify_sources(pins)
            after = base.preservation_snapshot()
            base.evidence('setup-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__': unittest.main()
