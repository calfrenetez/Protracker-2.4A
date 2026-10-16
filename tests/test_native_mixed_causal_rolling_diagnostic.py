"""HOST audit of the separately scoped original-case native diagnostic.

Both the all68 selector and the exact first native diagnostic are compiled with
ASan/UBSan and enabled assertions. Partial progress is never a success oracle.
The canonical full68 fixture and its existing oracle remain unchanged.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest
import test_native_mixed_causal_rolling_entry as base

ENTRY = 'tests/native_exec_mixed_causal_rolling_diagnostic.c'
SOURCES = (ENTRY, *base.SOURCES[1:])


def oracle(selected):
    cases = range(1, 69) if selected == 0 else (selected,)
    lines = [f'ROLLING DIAGNOSTIC ENTRY:case={selected};SOFTWARE_MODELS_ONLY',
             'ROLLING HEADER CONTROLS PASS']
    for number in cases:
        lines.extend((f'ROLLING CASE {number} BEGIN', f'ROLLING CASE {number} END'))
    lines.append(f'ROLLING DIAGNOSTIC PASS:case={selected} cases={len(cases)} of68;SOFTWARE_MODELS_ONLY')
    return ('\n'.join(lines) + '\n').encode('ascii')


class NativeRollingDiagnosticHost(unittest.TestCase):
    def test_full_dispatch_and_exact_first_case(self):
        before = base.preservation_snapshot()
        base.evidence('diagnostic-before-preservation.json', before)
        source_pins = {}
        original_sources = base.SOURCES
        try:
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-rolling-diagnostic-') as temporary:
                private = Path(temporary)
                source_pins = base.source_snapshot(private)
                pin, raw = base.ordinary_pin(base.ROOT / 'vendor/pt23f/raw/ptfont.raw')
                self.assertEqual(pin['sha256'], base.RAW_FONT_SHA256)
                header = private / 'pt_font.h'
                header.write_text('static const unsigned char pt_font[580] = {' +
                                  ','.join(str(value) for value in raw) + '};\n')
                self.assertEqual(base.ordinary_pin(header)[0]['sha256'], base.FONT_HEADER_SHA256)
                env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                       'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                       'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                for selected in (0, 1):
                    with self.subTest(selected=selected):
                        fixture = private / f'diagnostic-{selected}'
                        compiled = subprocess.run(
                            ['/usr/bin/cc', *base.CFLAGS,
                             f'-DPT_ROLLING_DIAGNOSTIC_CASE={selected}', *SOURCES,
                             '-o', str(fixture)], cwd=private, env=env,
                            capture_output=True, timeout=120)
                        base.evidence(f'diagnostic-{selected}-compile.stdout', compiled.stdout)
                        base.evidence(f'diagnostic-{selected}-compile.stderr', compiled.stderr)
                        self.assertEqual(compiled.returncode, 0, compiled.stderr.decode('utf-8', 'replace'))
                        self.assertEqual((compiled.stdout, compiled.stderr), (b'', b''))
                        actual = subprocess.run([str(fixture)], cwd=private, env=env,
                                                capture_output=True, timeout=30)
                        base.evidence(f'diagnostic-{selected}-fixture.stdout', actual.stdout)
                        base.evidence(f'diagnostic-{selected}-fixture.stderr', actual.stderr)
                        self.assertEqual(actual.returncode, 0, actual.stderr.decode('utf-8', 'replace'))
                        self.assertEqual(actual.stderr, b'')
                        self.assertEqual(actual.stdout, oracle(selected))
                base.verify_sources(source_pins)
        finally:
            base.SOURCES = original_sources
            base.verify_sources(source_pins)
            after = base.preservation_snapshot()
            base.evidence('diagnostic-after-preservation.json', after)
            self.assertEqual(base.stable_fact(after), base.stable_fact(before))


if __name__ == '__main__':
    unittest.main()
