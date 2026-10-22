"""Offline retained-evidence tests; no target or synthetic runtime admission."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from editor_focus_diagnostic import explain_refusal


class FocusDiagnosticTests(unittest.TestCase):
    def setUp(self):
        self.before = dict(focus_ready=False, button_mask=0, geometry_valid=True,
            pending_effective_diverged=False, pending_tablet_mode='mousehack',
            effective_tablet_mode='mousehack', pending_mouse_untrap='middle',
            effective_mouse_untrap='middle', schema_version=1, runtime_id='HOST-runtime',
            geometry_revision=12)
        self.action = dict(ok=False, code='input_not_ready', failure_phase='readiness',
            execution_state='not_started', applied_coordinates=[], cleanup_state='confirmed',
            message='Amiberry input focus is not ready', action='move', deduplicated=False,
            retryable=True, schema_version=1, runtime_id='HOST-runtime', geometry_revision=12)

    def test_false_aggregate_does_not_identify_host_focus(self):
        result = explain_refusal(self.before, self.action)
        self.assertEqual(len(result['possibleFailedTerms']), 2)
        self.assertFalse(result['rootCauseEstablished'])
        self.assertFalse(result['individualNativeTermsObserved'])
        self.assertFalse(result['runtimeAdmission'])

    def test_retryable_api_field_is_not_replay_authority(self):
        self.assertFalse(explain_refusal(self.before, self.action)['retryAllowed'])

    def test_positive_earlier_focus_does_not_override_later_refusal(self):
        self.before['focus_ready'] = True
        result = explain_refusal(self.before, self.action)
        self.assertFalse(result['rootCauseEstablished'])
        self.assertFalse(result['hostRaiseProvesInputReadiness'])

    def test_middle_is_outside_transaction_not_a_missing_magic_diagnosis(self):
        result = explain_refusal(self.before, self.action)
        self.assertFalse(result['temporarySettingsObservedHere'])
        self.assertFalse(result['outsideTransactionFocusRequired'])
        self.assertTrue(result['originalGUIReadinessAndNativeCallbackStillRequired'])

    def test_applied_ambiguous_or_different_failures_refused(self):
        variants = [('ok', True), ('code', 'input_rejected'),
            ('failure_phase', 'execution'), ('execution_state', 'unknown'),
            ('applied_coordinates', [{'x': 1, 'y': 2}]),
            ('cleanup_state', 'uncertain'), ('message', 'Temporary input settings did not become effective'),
            ('action', 'click'), ('deduplicated', True)]
        for key, value in variants:
            with self.subTest(key=key):
                action = dict(self.action, **{key: value})
                with self.assertRaises(ValueError): explain_refusal(self.before, action)

    def test_unsettled_or_unreleased_state_refused(self):
        variants = [('focus_ready', 0), ('button_mask', True), ('button_mask', 1),
            ('geometry_valid', False), ('pending_effective_diverged', True),
            ('pending_tablet_mode', 'real'), ('pending_mouse_untrap', 'both'),
            ('effective_tablet_mode', None)]
        for key, value in variants:
            with self.subTest(key=key):
                before = dict(self.before, **{key: value})
                with self.assertRaises(ValueError): explain_refusal(before, self.action)

    def test_result_cannot_mutate_retained_input(self):
        original = copy.deepcopy(self.before)
        result = explain_refusal(self.before, self.action)
        result['outsideTransaction']['focus_ready'] = True
        self.assertEqual(self.before, original)

    def test_other_runtime_or_geometry_cannot_be_combined(self):
        for key, value in [('runtime_id', 'other'), ('geometry_revision', 13),
                           ('schema_version', 2)]:
            with self.subTest(key=key):
                with self.assertRaises(ValueError):
                    explain_refusal(self.before, dict(self.action, **{key: value}))


if __name__ == '__main__': unittest.main()
