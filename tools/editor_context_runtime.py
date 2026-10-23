"""Pinned V3 runtime loader and read-only observation of the approved service.

No restart, target connection, reservation, startup, recovery or release.
Imports are checked against the newly selected source, before any caller action.
"""
import json
import sys
from types import SimpleNamespace

import editor_pointer_parent as original
import editor_pointer_binding as selected
import editor_context_binding as binding_source


def require_current_service(freeze, observe, validate_records, *, pin=None):
    """Current connected 030 endpoint; the original owner checks its FRAME too."""
    pin = pin or original.pinned
    ref = freeze['actualApprovedServiceReadinessRef']; ready = json.loads(pin(ref))
    records = [json.loads(pin(ready[k])) for k in ('authorizationRef', 'restartIntentRef', 'restartResultRef')]
    # Revalidate the immutable actual approval/restart chain in its recorded
    # EMPTY epoch. Do not compare that historical guard to an active FRAME.
    validate_records(ready, *records, ready['observed'])
    current = observe()
    original.need(current['job'] == ready['observed']['job']
                  and current['processIdentity'] == ready['observed']['processIdentity']
                  and type(current['listenerPID']) is int
                  and current['listenerPID'] == int(ready['observed']['job']['pid'])
                  and current['serial'] == {'connected': True, 'host': '127.0.0.1', 'port': 2345}
                  and current['noRecoveryActivation'] is True,
                  'Current approved V3 service identity/endpoint/recovery changed')
    pin(ref)
    return current


def load(freeze):
    original.require_harness_runtime()
    session, _ = binding_source.source_shape(freeze)
    sys.path[:0] = [str(session), str(original.HARNESS / 'scripts'), str(original.TOOLS)]
    modules = {name: selected.checked_module(name, path, freeze['sources'])
               for name, path in freeze['modulePaths'].items()}
    extras = binding_source.load_extras(freeze)
    readiness = extras['service_readiness']
    def current_service(binding=None):
        return require_current_service(freeze, readiness.observe_current, readiness.validate_records)
    access = modules['launcher_access']; blank = modules['editor_context_blank']
    return SimpleNamespace(guard=modules['launcher_guard'].LauncherGuard,
        lease_input=access.lease_input, install_client=access.install_client,
        install_ipc=access.install_ipc, operation=access.operation,
        original_api=modules['capture_runner'].OriginalAPI, guest=modules['shared_guest'].Guest,
        blank_case=blank.run_case, blank_scope=blank, extras=extras, current_service=current_service)
