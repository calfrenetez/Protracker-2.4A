"""Compare descriptor queries with their frozen f85bc067 predecessor.

Only a private include-closure copy contains dual predicate execution and
temporary adverse controls. Every genuine descriptor call retains its original
fixture assertion and oracle. Complete wrapper, preparation, descriptor and
publisher beforeimages are restored before the fixture resumes. Counts report
observations, not independent cases or native performance qualification.
"""
from pathlib import Path
import hashlib
import re
import subprocess
import tempfile
import unittest
import test_native_mixed_causal_rolling_entry as base

MODE2 = 'tests/editor_mixed_causal_joint_source_lineage_test.c'
REFERENCE = 'tests/descriptor_scope_reference.inc'
DRIVER = 'tests/descriptor_scope_comparison.inc'
REFERENCE_SHA256 = '9dcd610e446e105c386f1cd7f20e34bd287342e76ec6fa2e37e73390ca419879'


class DescriptorScopeProof(unittest.TestCase):
    def test_original_descriptor_outcomes_and_read_order_refusals(self):
        before = base.preservation_snapshot()
        base.evidence('descriptor-proof-before-preservation.json', before)
        original_units = base.SOURCES
        pins = {}
        try:
            base.SOURCES = (*original_units, MODE2)
            with tempfile.TemporaryDirectory(prefix='pt-descriptor-scope-proof-') as temporary:
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
                start = text.index('static int source_descriptor(struct pt_editor_mixed_causal_prepare *s)\n{')
                end = text.index('static int source_inventory_owner_current(', start)
                proposed = text[start:end].replace('source_descriptor(', 'pt_desc_proposed(', 1)
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
                        base.evidence('descriptor-proof-' + label + '-compile.stdout', compiled.stdout)
                        base.evidence('descriptor-proof-' + label + '-compile.stderr', compiled.stderr)
                        self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr),
                                         (0, b'', b''))
                        report = root / (label + '-counts.txt')
                        run = subprocess.run([str(executable)], cwd=root,
                                             env={**env, 'PT_DESC_REPORT': str(report)},
                                             capture_output=True, timeout=30)
                        base.evidence('descriptor-proof-' + label + '.stdout', run.stdout)
                        base.evidence('descriptor-proof-' + label + '.stderr', run.stderr)
                        self.assertEqual((run.returncode, run.stdout, run.stderr), (0, expected, b''))
                        counts = [list(map(int, line.split())) for line in report.read_text().splitlines()]
                        self.assertEqual(len(counts), 3)
                        self.assertEqual([row[0] for row in counts], [0, 1, 2])
                        self.assertGreater(counts[mode][1], 0)
                        self.assertGreater(counts[mode][2], 0)
                        results.append({'fixture': label, 'modeRefusalAcceptanceCounts': counts,
                                        'exactOriginalOraclePassed': True,
                                        'oldNewDescriptorOutcomesMatched': True})
                base.evidence('descriptor-proof-counts.json', results)
                base.verify_sources(pins)
        finally:
            base.SOURCES = original_units
            base.verify_sources(pins)
            after = base.preservation_snapshot()
            base.evidence('descriptor-proof-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
