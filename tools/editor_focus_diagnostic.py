"""Offline explanation of a retained GUI readiness refusal, never admission.

The installed native predicate combines host focus, capture and guest heartbeat.
GET_GUI_AUTOMATION_STATE exposes only its aggregate. Do not turn that aggregate
into an invented observation of any individual component, or retry permission.
"""
import copy


def explain_refusal(before, action):
    """Accept only the retained, unapplied focus-readiness failure shape.

    The outside-transaction settings are not the temporary settings used by the
    original GUI service. Its failure text identifies the failing aggregate,
    not which native term failed. This result has no executable authority.
    """
    if not (
        action.get('ok') is False
        and action.get('code') == 'input_not_ready'
        and action.get('failure_phase') == 'readiness'
        and action.get('execution_state') == 'not_started'
        and action.get('applied_coordinates') == []
        and action.get('cleanup_state') == 'confirmed'
        and action.get('message') == 'Amiberry input focus is not ready'
        and action.get('action') == 'move'
        and action.get('deduplicated') is False
    ):
        raise ValueError('Exact original unapplied focus refusal required')
    if not (
        type(before.get('focus_ready')) is bool
        and before.get('button_mask') == 0
        and type(before.get('button_mask')) is int
        and before.get('geometry_valid') is True
        and before.get('pending_effective_diverged') is False
        and before.get('pending_tablet_mode') == before.get('effective_tablet_mode')
        and before.get('pending_mouse_untrap') == before.get('effective_mouse_untrap')
        and before.get('effective_tablet_mode') in ('off', 'mousehack', 'real')
        and before.get('effective_mouse_untrap') in ('off', 'middle', 'magic', 'both')
        and type(before.get('runtime_id')) is str and bool(before['runtime_id'])
        and before.get('schema_version') == action.get('schema_version') == 1
        and before['runtime_id'] == action.get('runtime_id')
        and type(before.get('geometry_revision')) is int
        and before['geometry_revision'] >= 0
        and before['geometry_revision'] == action.get('geometry_revision')
    ):
        raise ValueError('Same-runtime settled released-button observation required')
    return {
        'state': 'OFFLINE_NATIVE_FOCUS_REFUSAL_EXPLANATION_ONLY',
        'outsideTransaction': copy.deepcopy(before),
        'nativePredicate': 'isfocus() > 0 && (ismouseactive() || '
                           '(tablet >= MOUSEHACK && MAGIC && mousehack_alive()))',
        'possibleFailedTerms': ['nonpositive_native_focus',
                               'uncaptured_without_live_mousehack_heartbeat'],
        'individualNativeTermsObserved': False,
        'rootCauseEstablished': False,
        'hostRaiseProvesInputReadiness': False,
        'outsideTransactionFocusRequired': False,
        'temporarySettingsObservedHere': False,
        'originalGUIReadinessAndNativeCallbackStillRequired': True,
        'retryAllowed': False,
        'targetOperations': 0,
        'runtimeAdmission': False,
        'physicalAcceptance': False,
    }
