"""HOST source-preview veto with genuine prepared source/queue custody.

Four MOD/PP20 x source_only calls refuse while the actual unpublished owner is
queued. Authentic later cleanup/detach permits four actual bounded donor loads.
Original dispatch and barrier fixture mains are not imported or included.
"""
from pathlib import Path
import hashlib
import subprocess
import tempfile
import time
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = (
    'tests/sample_source_preview_barrier_test.c',
    'src/editor/prepared_paula_sources.c',
    'src/editor/sampler_paula.c',
    'src/core/sample_cache.c',
    'src/core/playback_pcm.c',
    'src/editor/sampler.c',
    'src/editor/slots.c',
    'src/core/pcm_filtered.c',
    'src/core/slices.c',
    'src/core/pattern.c',
    'src/core/document.c',
    'src/core/pp20.c',
    'src/core/project.c',
    'src/core/mod_project.c',
    'src/core/mod_inspect.c',
    'src/core/channels.c',
    'src/core/pcm.c',
    'src/core/wav.c',
    'src/core/svx.c',
    'src/core/raw.c',
    'src/editor/sampler_lifetime.c',
    'src/core/sample_usage.c',
    'src/editor/prepared_paula_lifetime.c',
    'src/core/scheduled_readers.c',
    'src/core/elapsed_clock.c',
    'src/editor/editor.c',
    'src/editor/song.c',
    'src/editor/workflow.c',
    'src/editor/sample_range.c',
    'src/editor/wave_summary.c',
    'src/core/event_resource.c',
    'src/core/flow.c',
    'src/core/pitch.c',
    'src/editor/prepared_paula_mutation_barrier.c',
    'src/platform/sample_import.c',
    'src/platform/raw_import.c',
    'src/platform/mod_import.c',
    'src/platform/pp20_import.c',
)
HOST_FLAGS = ('-DPT_PREPARED_PAULA_ENABLE_HOST_LIFETIME=1', '-DPT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER=1')
FROZEN = {'src/platform/sample_import.c': '0fee44980b58c88b55e48480b3a6e53807e36cbd47fce670bba53d00bd28e300', 'tests/sample_source_preview_barrier_test.c': '9083811395b4261ac9625ea4a7efae43b6a23773a7b45d6eaeff365624c11746', 'src/editor/prepared_paula_mutation_barrier.c': '0e0fb90092d07b89904bc9e668aa89651eedb9aed786d7d186104480a6dd9de0', 'src/editor/prepared_paula_mutation_barrier.h': 'fdde8a639f59883f8a7313a97fe9430b1eedf5f4cda833d608d4beca6b3f68c3', 'src/editor/prepared_paula_sources.c': 'f0f40f3100de1691f40ab37b725ad11e1008a6619c49e7b227a1f21658703112', 'src/editor/prepared_paula_sources.h': '74a070e4d7b4e8463afb61e3d182913872a7726ca61f4fdb7d5084bbe764757c', 'src/editor/prepared_paula_lifetime.c': '77e91ab9f056182faf64a04674c54db9f84436b971724336f0e35724b9da0748', 'src/editor/prepared_paula_lifetime.h': '7131371ab2359a7464e638ed4fdc40014edc7cfdbc3dd8297ae4919d66915561'}
EDITOR_SOURCE_PAIRS = {'preserved_working_display': {'src/editor/editor.c': '6301ed250aa5f0518b360c448a2fb742ab48fdc8e828c1a2d3c69df3ab479ea6', 'src/editor/editor.h': '63fb2ab5b528b327a347811dc446bb95fbabf92a873199b832583380ddcec6c3'}, 'committed_editor_baseline': {'src/editor/editor.c': 'c569b9bb445b4af2df5bbe44f6d2fd07166c9d9c668272098aaa8ead32f47ec0', 'src/editor/editor.h': '5b93982695779bafe45a1093cd3b3f4c905524dda5a6556a00f410e528f7aa65'}}
FROZEN_HELPER = 'f73a23edf8c4591dceb44ade3b08d2e1c4c75cd54301b58267d27753f16d58c1'
ORACLE = 'SAMPLE SOURCE PREVIEW HOST PASS:MOD/PP20 x source_only0/1 veto;real queued3-source owner;preview/budget/master/save/cache preserved;authentic cleanup/detach then bounded loads;zero ledgers;NO NATIVE AUTHORITY\n'
ORACLE_BYTES = 209
LITERAL_PROJECT_BODIES = 93


def donors():
    # Independent synthetic classic MOD, and literal-only PP20 envelope. The
    # encoder models the format bit order; it calls no production decoder.
    mod = bytearray(1084 + 1024 + 4)
    mod[:13] = b'PREVIEW DONOR'
    mod[20:26] = b'SOURCE'
    mod[42:44] = b'\0\2'
    mod[45] = 64
    for i in range(31):
        mod[20 + i*30 + 29] = 1
    mod[950] = 1
    mod[951] = 127
    mod[1080:1084] = b'M.K.'
    mod[-4:] = bytes((128, 127, 18, 238))
    bits = [0]
    def put(value, width):
        bits.extend((value >> shift) & 1 for shift in range(width-1, -1, -1))
    count = len(mod)-1
    while count >= 3:
        put(3, 2)
        count -= 3
    put(count, 2)
    for value in reversed(mod):
        put(value, 8)
    bits.extend([0] * (-len(bits) % 32))
    packed = bytes(sum(bits[i+j] << j for j in range(8))
                   for i in range(0, len(bits), 8))[::-1]
    pp20 = b'PP20' + bytes((9, 10, 12, 13)) + packed + len(mod).to_bytes(3, 'big') + b'\0'
    return bytes(mod), pp20


class SampleSourcePreviewBarrierHost(unittest.TestCase):
    def test_actual_preview_refusal_and_post_detach_bounded_loads(self):
        before = base.preservation_snapshot()
        base.evidence('source-preview-before-preservation.json', before)
        helper_before, _ = base.ordinary_pin(base.ROOT / 'tests/test_native_mixed_causal_rolling_entry.py')
        old_units = base.SOURCES
        pins = {}
        try:
            self.assertEqual(helper_before['sha256'], FROZEN_HELPER)
            self.assertEqual(len(SOURCES), 38)
            self.assertEqual(len(set(SOURCES)), 38)
            self.assertIn('-UNDEBUG', base.CFLAGS)
            self.assertIn('-fsanitize=address,undefined', base.CFLAGS)
            self.assertEqual(len(ORACLE.encode('ascii')), ORACLE_BYTES)
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-source-preview-barrier-') as temporary:
                private = Path(temporary)
                pins = base.source_snapshot(private)
                self.assertEqual(len(pins), LITERAL_PROJECT_BODIES)
                self.assertTrue(set(SOURCES) <= set(pins))
                for relative, sha in FROZEN.items():
                    self.assertEqual(pins[relative]['sha256'], sha)
                editor_inputs = {relative: pins[relative]['sha256']
                    for relative in ('src/editor/editor.c', 'src/editor/editor.h')}
                matches = [name for name, pair in EDITOR_SOURCE_PAIRS.items() if editor_inputs == pair]
                self.assertEqual(len(matches), 1, 'complete consistent editor C/H pair required')
                data = donors()
                paths = (private / 'donor.mod', private / 'donor.pp20')
                for path, raw in zip(paths, data):
                    with path.open('xb') as stream:
                        self.assertEqual(stream.write(raw), len(raw))
                generated = {path.name: {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}
                             for path, raw in zip(paths, data)}
                environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                    'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                executable = private / 'fixture'
                deadline = time.monotonic() + 120
                compiled = subprocess.run(['/usr/bin/cc', *base.CFLAGS, *HOST_FLAGS,
                                           *SOURCES, '-o', str(executable)], cwd=private,
                    env=environment, capture_output=True,
                    timeout=max(0.1, deadline-time.monotonic()))
                base.evidence('source-preview-compile.stdout', compiled.stdout)
                base.evidence('source-preview-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr), (0, b'', b''))
                actual = subprocess.run([str(executable), *(str(path) for path in paths)],
                    cwd=private, env=environment, capture_output=True, timeout=30)
                base.evidence('source-preview-fixture.stdout', actual.stdout)
                base.evidence('source-preview-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('source-preview-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'additionalHostFlags': list(HOST_FLAGS), 'frozenSelectedInputsSha256': FROZEN,
                    'originalHelperPin': helper_before, 'selectedEditorSourcePair': matches[0],
                    'selectedEditorSourceSha256': editor_inputs, 'generatedDonors': generated,
                    'coverage': {'vetoCalls': 4, 'postDetachLoads': 4, 'preexistingDonorLoads': 1,
                        'formats': ['MOD', 'PP20'], 'sourceOnlyValues': [0, 1],
                        'actualThreeSourceUnpublishedQueueOwners': 1, 'backendCalls': 0},
                    'boundary': 'HOST entry regression only; genuine owner/cache/queue and actual bounded loaders; format sniff IO allowed; no native runtime/frontend/output/IRQ/quiet/timing or physical authority',
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30})
                base.verify_sources(pins)
        finally:
            base.SOURCES = old_units
            base.verify_sources(pins)
            helper_after, _ = base.ordinary_pin(base.ROOT / 'tests/test_native_mixed_causal_rolling_entry.py')
            self.assertEqual(base.stable_fact(helper_before), base.stable_fact(helper_after))
            after = base.preservation_snapshot()
            base.evidence('source-preview-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
