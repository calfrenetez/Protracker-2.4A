"""Pure/injected caller checks: no environment, private files, target or guard imports."""
import hashlib


def candidate_bytes(data, expected):
    actual = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    require_candidate_record(actual, expected)
    return actual


def require_candidate_record(actual, expected):
    assert set(expected) == {'bytes', 'sha256'}
    assert type(expected['bytes']) is int and expected['bytes'] > 0
    assert isinstance(expected['sha256'], str) and len(expected['sha256']) == 64
    assert actual == expected, 'Candidate bytes/record differ from immutable promotion'
    return actual


def hold_start_failure(guard, local_lease, controller, expected_binding,
                       target='amiberry-030'):
    """Latch only this attempt's actual reserve result; never borrow stale input.

    Sanitized identity/binding checks precede hold. The injected guard's hold
    validates the complete exact lease atomically; errors disclose no tokens,
    private paths, state payload or exception text. Existing HOLD is retained.
    """
    if local_lease is None:
        return {'status': 'NO_LOCAL_RESERVATION_NO_HOLD'}
    try:
        assert guard is not None and isinstance(expected_binding, dict)
        assert isinstance(local_lease, dict) and set(local_lease) == {'run_id', 'token'}
        state = guard.status()
        current = state['reservation']
        assert current is not None
        assert current['target'] == target and current['controller'] == controller
        assert current['lease']['run_id'] == local_lease['run_id']
        assert current['binding'] == expected_binding
        # Full token ownership is checked again under the production guard's
        # own lock; no file serialization or inherited environment is required.
        guard.hold(local_lease, 'ProTracker preparation-session first startup failure; no automatic retry or recovery')
        return {'status': 'THIS_ATTEMPT_HOLD_RETAINED'}
    except BaseException as error:
        return {'status': 'OWNERSHIP_UNCERTAIN_NO_FOREIGN_MODIFICATION',
                'errorType': type(error).__name__}
