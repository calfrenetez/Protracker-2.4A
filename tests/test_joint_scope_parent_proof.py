"""Compare joint scope with its frozen predecessor on genuine live fixtures.

The private test copy evaluates both pure predicates on every call and adds
temporary scalar/guard-length controls, restoring the complete owner bytes.
Production never contains this dual execution, mutation or report writer.
"""
from pathlib import Path
import hashlib
import re
import subprocess
import tempfile
import unittest
import test_native_mixed_causal_rolling_entry as base

MODE2 = 'tests/editor_mixed_causal_joint_source_lineage_test.c'
REFERENCE = 'tests/joint_scope_reference.inc'
DRIVER = 'tests/joint_scope_comparison.inc'
REFERENCE_SHA256 = '8d1c71021c53a1417df5ee73e5940bc25edf67e9de7cb7590615250a8d7e2261'


class JointScopeParentProof(unittest.TestCase):
    def test_original_scope_outcomes_and_complete_parent_refusals(self):
        before = base.preservation_snapshot()
        base.evidence('joint-proof-before-preservation.json', before)
        original_units = base.SOURCES
        pins = {}
        try:
            base.SOURCES = (*original_units, MODE2)
            with tempfile.TemporaryDirectory(prefix='pt-joint-scope-proof-') as temporary:
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
                start = text.index('static struct pt_editor_mixed_causal_lineage_source_prepare *lineage_source_scope(')
                end = text.index('static struct pt_editor_mixed_causal_lineage_prepare *lineage_scope(', start)
                proposed = text[start:end].replace('lineage_source_scope(', 'pt_scope_proposed(', 1)
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
                        base.evidence('joint-proof-' + label + '-compile.stdout', compiled.stdout)
                        base.evidence('joint-proof-' + label + '-compile.stderr', compiled.stderr)
                        self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr), (0, b'', b''))
                        report = root / (label + '-counts.txt')
                        run = subprocess.run([str(executable)], cwd=root,
                                             env={**env, 'PT_SCOPE_REPORT': str(report)},
                                             capture_output=True, timeout=30)
                        base.evidence('joint-proof-' + label + '.stdout', run.stdout)
                        base.evidence('joint-proof-' + label + '.stderr', run.stderr)
                        self.assertEqual((run.returncode, run.stdout, run.stderr), (0, expected, b''))
                        counts = [list(map(int, line.split())) for line in report.read_text().splitlines()]
                        self.assertEqual(len(counts), 3)
                        self.assertEqual([row[0] for row in counts], [0, 1, 2])
                        self.assertGreater(counts[mode][1], 0)
                        self.assertGreater(counts[mode][2], 0)
                        results.append({'fixture': label, 'modeRefusalAcceptanceCounts': counts,
                                        'exactOriginalOraclePassed': True})
                base.evidence('joint-proof-counts.json', results)
                base.verify_sources(pins)
        finally:
            base.SOURCES = original_units
            base.verify_sources(pins)
            after = base.preservation_snapshot()
            base.evidence('joint-proof-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
