"""Private frozen HOST-only exclusive editor mutation/source custody fixture.

A real initialized editor supplies three established precision masters. No donor
main or opaque owner copy is used. Thirteen cases complete7 and retain6 full fault
contexts. This is neither native frontend integration nor output/quiet authority.
"""
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = (
    'tests/prepared_paula_mutation_barrier_test.c',
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
)
HOST_FLAGS = ('-DPT_PREPARED_PAULA_ENABLE_HOST_LIFETIME=1',
              '-DPT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER=1')
FROZEN = {'src/editor/prepared_paula_mutation_barrier.c': 'd5dcb03c2e27db45d86933fa1beacf78c19fbdd8ce00bffa4ddd09986b030c41', 'src/editor/prepared_paula_mutation_barrier.h': '8d5ee9ae0c0aad1e8d28b3366a838dd002c3a00afe260b0850977e595f6539b1', 'tests/prepared_paula_mutation_barrier_test.c': '549f7f6335a8a1f675832e7487e2027e45ee2cb141eb84908ce68032ed089a08', 'src/editor/prepared_paula_sources.c': 'f0f40f3100de1691f40ab37b725ad11e1008a6619c49e7b227a1f21658703112', 'src/editor/prepared_paula_sources.h': '74a070e4d7b4e8463afb61e3d182913872a7726ca61f4fdb7d5084bbe764757c', 'src/editor/prepared_paula_lifetime.c': '77e91ab9f056182faf64a04674c54db9f84436b971724336f0e35724b9da0748', 'src/editor/prepared_paula_lifetime.h': '7131371ab2359a7464e638ed4fdc40014edc7cfdbc3dd8297ae4919d66915561'}
# Admit only a complete, consistent C/H snapshot; never a mixed pair.
# The clean pair is identified by body hashes, without a permanent HEAD lock.
EDITOR_SOURCE_PAIRS = {'preserved_working_display': {'src/editor/editor.c': '6301ed250aa5f0518b360c448a2fb742ab48fdc8e828c1a2d3c69df3ab479ea6', 'src/editor/editor.h': '63fb2ab5b528b327a347811dc446bb95fbabf92a873199b832583380ddcec6c3'}, 'committed_editor_baseline': {'src/editor/editor.c': 'c569b9bb445b4af2df5bbe44f6d2fd07166c9d9c668272098aaa8ead32f47ec0', 'src/editor/editor.h': '5b93982695779bafe45a1093cd3b3f4c905524dda5a6556a00f410e528f7aa65'}}
FROZEN_HELPER = 'f73a23edf8c4591dceb44ade3b08d2e1c4c75cd54301b58267d27753f16d58c1'
ORACLE = 'PREPARED PAULA MUTATION HOST PASS:13 cases;7 completed;6 retained;real editor mono8/16/24;exclusive veto;deferred actual source/queue/cache cleanup;separate detach;project/master/save preserved;NO OUTPUT OR NATIVE AUTHORITY\n'

class PreparedPaulaMutationBarrierHost(unittest.TestCase):
    def test_real_editor_unpublished_custody_and_deferred_detach(self):
        before = base.preservation_snapshot()
        base.evidence('prepared-paula-mutation-before-preservation.json', before)
        helper_before, _ = base.ordinary_pin(base.ROOT / 'tests/test_native_mixed_causal_rolling_entry.py')
        old_units = base.SOURCES
        pins = {}
        try:
            self.assertEqual(helper_before['sha256'], FROZEN_HELPER)
            self.assertEqual(len(SOURCES), 34)
            self.assertEqual(len(set(SOURCES)), 34)
            self.assertIn('-UNDEBUG', base.CFLAGS)
            self.assertIn('-fsanitize=address,undefined', base.CFLAGS)
            self.assertEqual(len(ORACLE.encode('ascii')), 224)
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-prepared-paula-mutation-') as temporary:
                private = Path(temporary)
                pins = base.source_snapshot(private)
                self.assertEqual(len(pins), 85)
                self.assertTrue(set(SOURCES) <= set(pins))
                for relative, sha in FROZEN.items():
                    self.assertEqual(pins[relative]['sha256'], sha)
                editor_inputs = {relative: pins[relative]['sha256']
                    for relative in ('src/editor/editor.c', 'src/editor/editor.h')}
                matches = [name for name, pair in EDITOR_SOURCE_PAIRS.items()
                           if editor_inputs == pair]
                self.assertEqual(len(matches), 1, 'complete consistent editor C/H pair required')
                selected_editor_pair = matches[0]
                environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                    'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                executable = private / 'fixture'
                deadline = time.monotonic() + 120
                compiled = subprocess.run(['/usr/bin/cc', *base.CFLAGS, *HOST_FLAGS,
                                           *SOURCES, '-o', str(executable)], cwd=private,
                    env=environment, capture_output=True,
                    timeout=max(0.1, deadline-time.monotonic()))
                base.evidence('prepared-paula-mutation-compile.stdout', compiled.stdout)
                base.evidence('prepared-paula-mutation-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr),
                                 (0, b'', b''))
                actual = subprocess.run([str(executable)], cwd=private, env=environment,
                                        capture_output=True, timeout=30)
                base.evidence('prepared-paula-mutation-fixture.stdout', actual.stdout)
                base.evidence('prepared-paula-mutation-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('prepared-paula-mutation-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'additionalHostFlags': list(HOST_FLAGS),
                    'frozenSelectedInputsSha256': FROZEN, 'originalHelperPin': helper_before,
                    'selectedEditorSourcePair': selected_editor_pair,
                    'selectedEditorSourceSha256': editor_inputs,
                    'acceptedCompleteEditorSourcePairs': EDITOR_SOURCE_PAIRS,
                    'coverage': {'cases': 13, 'completed': 7, 'retained': 6,
                        'atomicAdmissionMatrix': 1, 'genuineCancelCuts': 6,
                        'preparedCut': 'Actual source PREPARED/completed3/masterpins3/leases3/no jobs and exact cache bytes before queue construction',
                        'chipAllocationReentry': 1, 'queueAllocationReentry': 1,
                        'cacheReleaseReentry': 1, 'knownControlAlias': 1,
                        'fullBackendAndAllocatorContextTailAlias': 1, 'foreignHook': 1},
                    'editorBoundary': 'One actual initialized editor/sampler/document; no donor main or opaque owner/sampler copy; real note/undo/sample-attribute/table/dispose refusals preserve project/master/cache/save and ownership with explicit UI/status allowance',
                    'lifetimeBoundary': 'Only genuine unpublished source/queue transitions; hook never performs child/backend/allocator work; explicit later cleanup and matched detach; child/barrier workspace inaccessible before later real editor edit/disposal',
                    'retainedBoundary': 'Six complete contexts remain reachable, with actual acquired/consumed ledger state preserved; no force cleanup, failed transaction retry, stolen-hook repair or former generation restoration',
                    'workingSourceBoundary': 'Exactly one complete consistent editor C/H hash pair is admitted: preserved working display or verified committed editor baseline; a mixed pair refuses. The selected pair is recorded, copied and preserved. No ABI equivalence between pairs or permanent HEAD lock is claimed; original protected16 and classic baseline use the original helper',
                    'scope': 'HOST-only exclusive unpublished mutation barrier; no native UI New/Load/exit integration, native ABI/IRQ, publication/ACTIVE/backend timing/output/device quiet/physical/listening authority or full system-header/dyld audit',
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30})
                base.verify_sources(pins)
        finally:
            base.SOURCES = old_units
            base.verify_sources(pins)
            helper_after, _ = base.ordinary_pin(base.ROOT / 'tests/test_native_mixed_causal_rolling_entry.py')
            self.assertEqual(base.stable_fact(helper_before), base.stable_fact(helper_after))
            after = base.preservation_snapshot()
            base.evidence('prepared-paula-mutation-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
