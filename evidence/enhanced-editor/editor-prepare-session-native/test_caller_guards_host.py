"""Isolated host mocks and actual caller AST fragments; production actions forbidden."""
import ast
import copy
import hashlib
import json
import os
from pathlib import Path
import tempfile
from types import SimpleNamespace
from unittest.mock import patch
import sys
from caller_guards import candidate_bytes, require_candidate_record, hold_start_failure

HERE = Path(__file__).resolve().parent
OWNER = 'ProTracker root01a0b5ae-c04f-7661-8859-e89df20c3290'
COMMIT = '86cb65fdef27a19082a4d469acee8aa3bb39e0bb'
effects = {'productionGuard': 0, 'target': 0, 'network': 0, 'process': 0, 'privateLeaseRead': 0}
cases = []


class MockGuard:
    def __init__(self, foreign=False):
        self.actual = {'run_id': 'owned-synthetic-run', 'token': 'synthetic-token'}
        self.reservation = None
        self.holds = []
        self.hold_calls = 0
        self.status_calls = 0
        if foreign:
            self.reservation = {'target': 'real-a1200', 'controller': 'foreign-controller',
                                'lease': {'run_id': 'foreign-run'}, 'binding': {}}

    def reserve(self, target, controller, purpose, bound):
        assert self.reservation is None, 'Synthetic foreign reservation retained'
        self.reservation = {'target': target, 'controller': controller,
                            'lease': {'run_id': self.actual['run_id']}, 'binding': copy.deepcopy(bound)}
        return dict(self.actual)

    def status(self):
        self.status_calls += 1
        return copy.deepcopy({'reservation': self.reservation, 'holds': self.holds})

    def hold(self, lease, reason):
        self.hold_calls += 1
        assert lease == self.actual, 'Full synthetic token mismatch'
        assert self.reservation['lease']['run_id'] == lease['run_id']
        self.holds.append({'reason': reason})


def execute_fragment(nodes, namespace):
    tree = ast.fix_missing_locations(ast.Module(body=nodes, type_ignores=[]))
    exec(compile(tree, '<isolated-actual-caller-fragment>', 'exec'), namespace)


def start_fragment():
    tree = ast.parse((HERE / 'start_once.py').read_text())
    outer = next(n for n in tree.body if isinstance(n, ast.Try))
    init = next(n for n in tree.body if isinstance(n, ast.Assign) and
                any(isinstance(t, ast.Name) and t.id == 'local_lease' for t in n.targets))
    assert isinstance(init.value, ast.Constant) and init.value.value is None and init.lineno < outer.lineno
    latch = next(n for n in ast.walk(outer.handlers[0]) if isinstance(n, ast.Call) and
                 isinstance(n.func, ast.Name) and n.func.id == 'hold_start_failure')
    assert [n.id for n in latch.args] == ['local_guard', 'local_lease', 'controller', 'reservation_binding']
    index = next(i for i, n in enumerate(outer.body) if isinstance(n, ast.Assign) and
                 any(isinstance(t, ast.Name) and t.id == 'reservation_binding' for t in n.targets))
    return ast.Try(body=outer.body[index:], handlers=outer.handlers, orelse=[], finalbody=[])


def startup_case(label, directory):
    guard = MockGuard(foreign=label == 'foreign-before-reserve')
    result = {'reserveCalls': 0, 'startCalls': 0}
    child_calls = []
    stage = directory / label
    stage.mkdir()
    local_os = SimpleNamespace(**{name: getattr(os, name) for name in
                                 ('open', 'close', 'fsync', 'O_WRONLY', 'O_CREAT', 'O_EXCL', 'O_NOFOLLOW')},
                              environ={'AMIGA_LAUNCH_LEASE': 'stale-inherited-private-file'})

    class Stream:
        def __init__(self, fd):
            self.stream = os.fdopen(fd, 'wb')
        def __enter__(self):
            return self
        def __exit__(self, *args):
            self.stream.close()
        def write(self, data):
            if label == 'serialization-write':
                raise OSError('Injected local serialization failure')
            return self.stream.write(data)
        def flush(self):
            return self.stream.flush()
        def fileno(self):
            return self.stream.fileno()
    local_os.fdopen = lambda fd, mode: Stream(fd)

    def pack(value):
        if label == 'reserve-before-serialization':
            raise ValueError('Injected failure before private serialization')
        if label.startswith('foreign-after-reserve-'):
            field = label.split('foreign-after-reserve-', 1)[1]
            if field == 'target':
                guard.reservation['target'] = 'real-a1200'
            elif field == 'controller':
                guard.reservation['controller'] = 'foreign-controller'
            elif field == 'run':
                guard.reservation['lease']['run_id'] = 'foreign-run'
            else:
                guard.reservation['binding']['sourceCommit'] = '0' * 40
            raise ValueError('Injected foreign-state failure')
        return json.dumps(value).encode()

    def save(name, value):
        if label == 'serialized-before-preflight':
            raise OSError('Injected preflight save failure')
        # No production/private values are logged. Local fake lease write remains
        # available to check that failure latching is independent of that file.

    def run(*args, **kwargs):
        child_calls.append(1)
        assert kwargs['timeout'] == 110
        if label == 'lifecycle-exception':
            raise TimeoutError('Injected synthetic lifecycle timeout')
        return SimpleNamespace(returncode=20 if label == 'lifecycle-nonzero' else 0, stdout=b'', stderr=b'')

    namespace = {'HERE': stage, 'INFRA': directory / 'forbidden-production-placeholder',
                 'result': result, 'guard': guard, 'local_guard': guard,
                 'local_lease': None, 'reservation_binding': None, 'controller': OWNER,
                 'product': {'bytes': 395508, 'sha256': 'b9b9a71351a012f20b5dc67228604f2f6e02d7dce93d07b2fed28fcb8cf83d10'},
                 'binding': lambda: {'sourceCommit': COMMIT, 'python': 'mock-only-python'},
                 'fp': lambda p: {'bytes': 1, 'sha256': '1' * 64}, 'os': local_os,
                 '_pack': pack, 'save': save, 'subprocess': SimpleNamespace(run=run),
                 'processes': lambda: [] if label == 'lifecycle-success-owned-check' else [(123, 'synthetic-owned-profile')],
                 'hold_start_failure': hold_start_failure}
    execute_fragment([start_fragment()], namespace)
    if label == 'success':
        assert result['status'] == 'PASS_ONE_OWNED_030_START' and not guard.holds
    elif label == 'foreign-before-reserve':
        assert result['holdAttempt']['status'] == 'NO_LOCAL_RESERVATION_NO_HOLD'
        assert guard.hold_calls == guard.status_calls == 0 and not guard.holds
    elif label.startswith('foreign-after-reserve-'):
        assert result['holdAttempt']['status'] == 'OWNERSHIP_UNCERTAIN_NO_FOREIGN_MODIFICATION'
        assert guard.hold_calls == 0 and not guard.holds
    else:
        assert result['holdAttempt']['status'] == 'THIS_ATTEMPT_HOLD_RETAINED'
        assert len(guard.holds) == guard.hold_calls == 1
        assert guard.reservation['lease']['run_id'] == namespace['local_lease']['run_id']
    if label in ('reserve-before-serialization', 'serialization-write',
                 'serialized-before-preflight', 'foreign-before-reserve') or label.startswith('foreign-after-reserve-'):
        assert child_calls == []
    else:
        assert child_calls == [1]
    cases.append({'case': label, 'kind': 'actual-start-try/except-fragment',
                  'syntheticLifecycleCalls': len(child_calls), 'ownedHoldCalls': guard.hold_calls})


def no_borrow_cases():
    guard = MockGuard(foreign=True)
    before = copy.deepcopy(guard.reservation)
    for label, fake_environment in [('none-local/stale-env', {'AMIGA_LAUNCH_LEASE': 'stale-private'}),
                                    ('none-local/no-env', {})]:
        with patch.dict(os.environ, fake_environment, clear=True):
            assert hold_start_failure(guard, None, OWNER, {}) == {'status': 'NO_LOCAL_RESERVATION_NO_HOLD'}
        assert guard.reservation == before and guard.status_calls == guard.hold_calls == 0
        cases.append({'case': label, 'kind': 'no-file/no-environment-borrow', 'guardStatusCalls': 0})
    guard = MockGuard()
    expected = {'sourceCommit': COMMIT, 'candidateBytes': 395508, 'candidateSHA256': 'a' * 64}
    local = guard.reserve('amiberry-030', OWNER, 'mock', expected)
    local['token'] = 'wrong-full-synthetic-token'
    before = copy.deepcopy(guard.reservation)
    reply = hold_start_failure(guard, local, OWNER, expected)
    assert reply == {'status': 'OWNERSHIP_UNCERTAIN_NO_FOREIGN_MODIFICATION', 'errorType': 'AssertionError'}
    assert guard.reservation == before and not guard.holds
    assert 'token' not in json.dumps(reply) and 'private' not in json.dumps(reply)
    cases.append({'case': 'full-token-atomic-refusal', 'kind': 'guard.hold-exact-lease', 'foreignModifications': 0})


def candidate_cases():
    data = b'unchanged-exact-isolated-fixture'
    expected = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    assert candidate_bytes(data, expected) == expected
    assert require_candidate_record(dict(expected), expected) == expected
    cases.append({'case': 'exact-bytes-and-stage-pass', 'kind': 'immutable-candidate-comparison'})
    for label, value, byte_mode in [('mutated-acquisition', data[:-1] + b'X', True),
                                   ('short-acquisition', data[:-1], True),
                                   ('staged-fingerprint-mismatch', {**expected, 'sha256': '0' * 64}, False)]:
        dispatched = []
        try:
            (candidate_bytes if byte_mode else require_candidate_record)(value, expected)
            dispatched.append(1)
        except AssertionError:
            pass
        assert not dispatched
        cases.append({'case': label, 'kind': 'refusal-before-checksum-or-launch', 'dispatches': 0})
    # Execute the equality expressions extracted from each actual product caller,
    # rather than merely testing a helper that callers might have forgotten.
    for filename, candidate_name in [('run_once.py', 'r'), ('settle_once.py', 'run'),
                                     ('verify_release_once.py', 'run')]:
        tree = ast.parse((HERE / filename).read_text())
        node = next(n for n in ast.walk(tree) if isinstance(n, ast.Expr) and
                    isinstance(n.value, ast.Call) and isinstance(n.value.func, ast.Name) and
                    n.value.func.id == 'require_candidate_record')
        for changed in (False, True):
            actual = {**expected, 'sha256': 'f' * 64} if changed else dict(expected)
            namespace = {candidate_name: {'candidate': actual}, 'immutable_candidate': expected,
                         'binding': lambda: {'candidate': expected},
                         'require_candidate_record': require_candidate_record}
            refused = False
            try:
                execute_fragment([node], namespace)
            except AssertionError:
                refused = True
            assert refused == changed
            cases.append({'case': filename + ('-changed-refuses' if changed else '-exact-passes'),
                          'kind': 'actual-caller-candidate-gate', 'targetEffects': 0})
    run_tree = ast.parse((HERE / 'run_once.py').read_text())
    byte_node = next(n for n in ast.walk(run_tree) if isinstance(n, ast.Expr) and
                     isinstance(n.value, ast.Call) and isinstance(n.value.func, ast.Name) and
                     n.value.func.id == 'candidate_bytes')
    for changed in (False, True):
        refused = False
        try:
            execute_fragment([byte_node], {'binary': data[:-1] if changed else data,
                                          'immutable_candidate': expected, 'candidate_bytes': candidate_bytes})
        except AssertionError:
            refused = True
        assert refused == changed
        cases.append({'case': 'actual-run-acquisition-' + ('changed-refuses' if changed else 'exact-passes'),
                      'kind': 'actual-before-staging-byte-gate', 'targetEffects': 0})


def main():
    output = HERE / 'host-mock-tests.json'
    assert not output.exists(), 'Never overwrite a first test result'
    directory = Path(tempfile.mkdtemp(prefix='mock-root-', dir=HERE))
    record = {'status': 'IN_PROGRESS', 'cases': cases, 'scope': 'Actual AST fragments/pure helpers in isolated host mocks; production imports/actions forbidden',
              'effects': effects, 'temporaryMockRootPreserved': str(directory)}
    # Make accidental imports of production guard/client modules fail, even if
    # a future test edit introduces one. No target wrapper is imported/executed.
    forbidden = {'launcher_guard': None, 'launcher_access': None, 'emulator': None,
                 'amiberry_mcp': None, 'mcp': None}
    try:
        with patch.dict(sys.modules, forbidden):
            for label in ('reserve-before-serialization', 'serialization-write', 'serialized-before-preflight',
                          'lifecycle-exception', 'lifecycle-nonzero', 'lifecycle-success-owned-check',
                          'foreign-before-reserve', 'foreign-after-reserve-target',
                          'foreign-after-reserve-controller', 'foreign-after-reserve-run',
                          'foreign-after-reserve-binding', 'success'):
                startup_case(label, directory)
            no_borrow_cases()
            candidate_cases()
        assert all(count == 0 for count in effects.values())
        record['status'] = 'PASS_ISOLATED_HOST_27_CASES_NO_PRODUCTION_ACTIONS'
        assert len(cases) == 27
    except BaseException as error:
        record.update(status='FIRST_HOST_MOCK_FAILURE_PRESERVED_NO_RETRY', errorType=type(error).__name__, error=str(error)[:400])
        raise
    finally:
        with output.open('x') as stream:
            json.dump(record, stream, indent=2); stream.write('\n')
    print('PASS27 isolated actual-start/candidate caller cases; zero production effects')


if __name__ == '__main__':
    main()
