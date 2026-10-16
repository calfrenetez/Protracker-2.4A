"""Compare lineage-current queries with their frozen b1e929ce predecessor.

The six original scope/current predicates are bound independently of the current
implementation. Only a private include-closure copy contains dual evaluation,
restored adverse controls and protected payloads. Every original rolling68 and
joint31 assertion/oracle remains. Counts are observations, not independent cases
or native performance/ownership/timing qualification.
"""
from pathlib import Path
import hashlib
import re
import subprocess
import tempfile
import unittest
import test_native_mixed_causal_rolling_entry as base

MODE2 = 'tests/editor_mixed_causal_joint_source_lineage_test.c'
REFERENCE = 'tests/lineage_current_reference.inc'
DRIVER = 'tests/lineage_current_comparison.inc'
REFERENCE_SHA256 = '41efbbc87214cca41f955b73505b6aba80d7f2d7f3691e8d0b279af2fbf4b3a2'


class LineageCurrentProof(unittest.TestCase):
    def test_original_current_outcomes_and_stage_before_payload_refusals(self):
        before = base.preservation_snapshot()
        base.evidence('lineage-current-before-preservation.json', before)
        original_units = base.SOURCES
        pins = {}
        try:
            base.SOURCES = (*original_units, MODE2)
            with tempfile.TemporaryDirectory(prefix='pt-lineage-current-proof-') as temporary:
                root = Path(temporary)
                pins = base.source_snapshot(root)
                extra = {}
                for name in (REFERENCE, DRIVER):
                    pin, body = base.ordinary_pin(base.ROOT / name)
                    pins[name] = pin
                    extra[name] = body.decode('ascii')
                self.assertEqual(hashlib.sha256(extra[REFERENCE].encode()).hexdigest(),
                                 REFERENCE_SHA256)
                source = root / 'src/editor/editor_mixed_causal_prepare.c'
                text = source.read_text()
                start = text.index('static int lineage_current(struct pt_editor_mixed_causal_prepare *s)\n{')
                end = text.index('/* Fixed readable controls only;', start)
                proposed = text[start:end].replace('lineage_current(', 'pt_lc_proposed(', 1)
                source.chmod(0o600)
                source.write_text(text[:start] + extra[REFERENCE] + proposed + extra[DRIVER] + text[end:])
                source.chmod(0o400)
                raw_pin, raw = base.ordinary_pin(base.ROOT / 'vendor/pt23f/raw/ptfont.raw')
                self.assertEqual(raw_pin['sha256'], base.RAW_FONT_SHA256)
                (root / 'pt_font.h').write_text('static const unsigned char pt_font[580] = {' +
                                              ','.join(str(x) for x in raw) + '};\n')
                env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(root),
                       'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                       'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                results = []
                for label, units, mode in (('rolling68', original_units, 1),
                                           ('joint31', (MODE2, *original_units[1:]), 0)):
                    with self.subTest(fixture=label):
                        expected = base.ORACLE.encode()
                        if mode == 0:
                            markers = re.findall(r'puts\("([^"\n]+)"\);', (root / MODE2).read_text())
                            expected = (markers[-1] + '\n').encode('ascii')
                            self.assertTrue(expected.startswith(b'EDITOR MIXED JOINT SOURCE LINEAGE PASS:31 '))
                        executable = root / label
                        compiled = subprocess.run(['/usr/bin/cc', *base.CFLAGS, *units,
                                                   '-o', str(executable)], cwd=root, env=env,
                                                  capture_output=True, timeout=120)
                        base.evidence('lineage-current-' + label + '-compile.stdout', compiled.stdout)
                        base.evidence('lineage-current-' + label + '-compile.stderr', compiled.stderr)
                        self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr),
                                         (0, b'', b''))
                        report = root / (label + '-counts.txt')
                        run = subprocess.run([str(executable)], cwd=root,
                                             env={**env, 'PT_LC_REPORT': str(report)},
                                             capture_output=True, timeout=30)
                        base.evidence('lineage-current-' + label + '.stdout', run.stdout)
                        base.evidence('lineage-current-' + label + '.stderr', run.stderr)
                        self.assertEqual((run.returncode, run.stdout, run.stderr), (0, expected, b''))
                        lines = report.read_text().splitlines()
                        self.assertEqual(len(lines), 4)
                        counts = [list(map(int, line.split())) for line in lines[:3]]
                        self.assertEqual([row[0] for row in counts], [0, 1, 2])
                        self.assertGreater(counts[mode][1], 0)
                        self.assertGreater(counts[mode][2], 0)
                        protected = lines[3].split()
                        self.assertEqual(protected[0], 'protected')
                        controls = list(map(int, protected[1:]))
                        self.assertEqual(controls, [2, 1] if mode == 1 else [0, 0])
                        results.append({'fixture': label, 'modeRefusalAcceptanceCounts': counts,
                                        'protectedStageActivationChecks': controls,
                                        'exactOriginalOraclePassed': True,
                                        'oldNewLineageCurrentOutcomesMatched': True})
                base.evidence('lineage-current-counts.json', results)
                base.verify_sources(pins)
        finally:
            base.SOURCES = original_units
            base.verify_sources(pins)
            after = base.preservation_snapshot()
            base.evidence('lineage-current-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
