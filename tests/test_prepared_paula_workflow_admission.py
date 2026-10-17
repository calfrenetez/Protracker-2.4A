"""Private HOST workflow-admission fixture with genuine unpublished custody.

Cleanup and Copy Range start from a completed real workflow preview before a
QUEUED prepared-Paula custodian attaches. Refusal preserves pending/job/resources;
actual later cleanup/detach permits each transaction and one undo. No fake hook.
Frozen source-only preparation; compilation and execution require later Root review.
"""
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = (
    'tests/prepared_paula_workflow_admission_test.c',
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
FROZEN = {'src/editor/prepared_paula_mutation_barrier.c': '0e0fb90092d07b89904bc9e668aa89651eedb9aed786d7d186104480a6dd9de0', 'src/editor/prepared_paula_mutation_barrier.h': 'fdde8a639f59883f8a7313a97fe9430b1eedf5f4cda833d608d4beca6b3f68c3', 'tests/prepared_paula_workflow_admission_test.c': '1a93c77a27031f0f27b56192bae6482d3cb2ee805a32eeb4bc1f7faf6c25b7b3', 'src/editor/workflow.c': '768dba18085aeb8e5c815a558ecf233f57652bd5762bbbe866d0ffbce519b6ca', 'src/editor/prepared_paula_sources.c': 'f0f40f3100de1691f40ab37b725ad11e1008a6619c49e7b227a1f21658703112', 'src/editor/prepared_paula_sources.h': '74a070e4d7b4e8463afb61e3d182913872a7726ca61f4fdb7d5084bbe764757c', 'src/editor/prepared_paula_lifetime.c': '77e91ab9f056182faf64a04674c54db9f84436b971724336f0e35724b9da0748', 'src/editor/prepared_paula_lifetime.h': '7131371ab2359a7464e638ed4fdc40014edc7cfdbc3dd8297ae4919d66915561'}
# Admit only a complete, consistent C/H snapshot; never a mixed pair.
# The clean pair is identified by body hashes, without a permanent HEAD lock.
EDITOR_SOURCE_PAIRS = {'preserved_working_display': {'src/editor/editor.c': '6301ed250aa5f0518b360c448a2fb742ab48fdc8e828c1a2d3c69df3ab479ea6', 'src/editor/editor.h': '63fb2ab5b528b327a347811dc446bb95fbabf92a873199b832583380ddcec6c3'}, 'committed_editor_baseline': {'src/editor/editor.c': 'c569b9bb445b4af2df5bbe44f6d2fd07166c9d9c668272098aaa8ead32f47ec0', 'src/editor/editor.h': '5b93982695779bafe45a1093cd3b3f4c905524dda5a6556a00f410e528f7aa65'}}
FROZEN_HELPER = 'f73a23edf8c4591dceb44ade3b08d2e1c4c75cd54301b58267d27753f16d58c1'
ORACLE = 'PREPARED PAULA WORKFLOW HOST PASS:2 cases;Cleanup/Copy veto before pending;real completed usage and queued custody;authentic cleanup/detach;postdetach transaction+one undo;zero ledgers;NO NATIVE AUTHORITY\n'
ORACLE_BYTES = 205
LITERAL_PROJECT_BODIES = 85

class PreparedPaulaWorkflowAdmissionHost(unittest.TestCase):
    def test_completed_preview_veto_and_post_detach_transactions(self):
        before = base.preservation_snapshot()
        base.evidence('prepared-paula-workflow-admission-before-preservation.json', before)
        helper_before, _ = base.ordinary_pin(base.ROOT / 'tests/test_native_mixed_causal_rolling_entry.py')
        old_units = base.SOURCES
        pins = {}
        try:
            self.assertEqual(helper_before['sha256'], FROZEN_HELPER)
            self.assertEqual(base.ROOT, Path(__file__).resolve().parents[1],
                             'Snapshot helper must belong to the reviewed prepared tree')
            self.assertEqual(len(SOURCES), 34)
            self.assertEqual(len(set(SOURCES)), 34)
            for required in ('-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                             '-fsanitize=address,undefined'):
                self.assertIn(required, base.CFLAGS)
            self.assertIsInstance(ORACLE_BYTES, int, 'Freeze the sole oracle before execution')
            self.assertEqual(len(ORACLE.encode('ascii')), ORACLE_BYTES)
            self.assertIsInstance(LITERAL_PROJECT_BODIES, int,
                                  'Freeze the literal project closure count before execution')
            self.assertGreater(LITERAL_PROJECT_BODIES, len(SOURCES))
            for sha in FROZEN.values():
                self.assertRegex(sha, r'^[0-9a-f]{64}$',
                                 'Freeze every final selected source pin before execution')
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-prepared-paula-workflow-admission-') as temporary:
                private = Path(temporary)
                pins = base.source_snapshot(private)
                self.assertEqual(len(pins), LITERAL_PROJECT_BODIES)
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
                base.evidence('prepared-paula-workflow-admission-compile.stdout', compiled.stdout)
                base.evidence('prepared-paula-workflow-admission-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr),
                                 (0, b'', b''))
                actual = subprocess.run([str(executable)], cwd=private, env=environment,
                                        capture_output=True, timeout=30)
                base.evidence('prepared-paula-workflow-admission-fixture.stdout', actual.stdout)
                base.evidence('prepared-paula-workflow-admission-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('prepared-paula-workflow-admission-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'additionalHostFlags': list(HOST_FLAGS),
                    'frozenSelectedInputsSha256': FROZEN, 'originalHelperPin': helper_before,
                    'selectedEditorSourcePair': selected_editor_pair,
                    'selectedEditorSourceSha256': editor_inputs,
                    'acceptedCompleteEditorSourcePairs': EDITOR_SOURCE_PAIRS,
                    'coverage': {'cases': 2, 'requests': ['Cleanup', 'Copy Range'],
                        'previewBeforeCustody': 'Completed actual workflow usage preview',
                        'custodyAtRefusal': 'Genuine QUEUED three-source unpublished C/R ownership',
                        'veto': 'No pending request, transaction, job or resource mutation',
                        'afterDetach': 'Actual transaction and one undo per case'},
                    'editorBoundary': 'One actual initialized editor/sampler/document per case; complete real usage preview before genuine queued custody; actual stopped Cleanup/Copy entry refuses without pending/job/resource changes; no fake ready hook or owner copy',
                    'lifetimeBoundary': 'Latch-only real hook followed by later top-level authentic local stop, bounded captured source drain, queue/cache close and separate detach; then real workflow transaction/undo and zero final ledgers',
                    'workingSourceBoundary': 'Exactly one complete consistent editor C/H hash pair is admitted per run: preserved working display or verified committed editor baseline. The existing outer flow qualifies each pair separately; mixed pairs refuse. Original protected16 and classic baseline use the unchanged helper',
                    'sourceSnapshotBoundary': 'The unchanged pinned helper copies only ordered TUs and their literal project include closure from this reviewed prepared tree after Root installs the reviewed workflow/fixture overlays; no runtime canonical overlay copy or extra exporter',
                    'scope': 'HOST-only stopped workflow admission regression; active Stop+Apply and read-only behavior additionally require the unchanged editor_workflow regression; workflow idle is not serviced during prepared custody; no native frontend/runtime/ABI/IRQ, publication/adoption/output/device-quiet/timing/physical/listening authority or full system-header/dynamic-loader closure',
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30})
                base.verify_sources(pins)
        finally:
            base.SOURCES = old_units
            base.verify_sources(pins)
            helper_after, _ = base.ordinary_pin(base.ROOT / 'tests/test_native_mixed_causal_rolling_entry.py')
            self.assertEqual(base.stable_fact(helper_before), base.stable_fact(helper_after))
            after = base.preservation_snapshot()
            base.evidence('prepared-paula-workflow-admission-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
