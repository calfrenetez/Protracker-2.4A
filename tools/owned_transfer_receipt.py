"""Caller-owned transfer witnesses, checked before external input adoption.

These local records are coordination assertions, not credentials or content
custody. The trusted Root caller observes the actual transfer and pins the
receipt hash in its admitted request. InputStage still independently checks the
remote one-file inventory and candidate checksum and retains its normal lifetime.
No transport, file I/O, ownership mutation or native execution occurs here.
"""
import hashlib
import json
import math
import re
import time

from owned_native_input import InputPlan
from owned_native_run import require


WITNESSES = ('namespace', 'complete', 'placement')


def _json(raw):
    require(type(raw) is bytes and 0 < len(raw) <= 65536, 'Bounded transfer record required')
    def pairs(items):
        value = {}
        for key, item in items:
            require(key not in value, 'Duplicate transfer record key')
            value[key] = item
        return value
    return json.loads(raw, object_pairs_hook=pairs)


def verify_receipt(raw, witnesses, *, expected_sha256, plan, native_plan_sha256,
                   owner_run_id, target, download_name, now):
    """Validate exact, recent witnesses; the caller must establish their truth."""
    require(type(plan) is InputPlan and plan.external_transfer,
            'Explicit external input plan required')
    for value, width in ((expected_sha256, 64), (native_plan_sha256, 64), (owner_run_id, 32)):
        require(type(value) is str and re.fullmatch('[0-9a-f]{%d}' % width, value),
                'Exact transfer binding required')
    require(target in ('real-a1200', 'amiberry-030'), 'Exact transfer target required')
    require(type(download_name) is str and re.fullmatch(r'[A-Za-z0-9_-]{1,30}', download_name),
            'Exact flat transfer filename required')
    require(type(raw) is bytes and hashlib.sha256(raw).hexdigest() == expected_sha256,
            'Root-pinned transfer receipt changed')
    receipt = _json(raw)
    require(type(receipt) is dict and set(receipt) == {
        'state', 'description', 'nativePlanSHA256', 'ownerRunID', 'target',
        'downloadName', 'observedUnixSeconds', 'witnessSHA256'}, 'Transfer receipt shape differs')
    require(receipt['state'] == 'ROOT_OBSERVED_FRESH_TRANSFER_COMPLETE'
            and receipt['description'] == plan.description()
            and receipt['nativePlanSHA256'] == native_plan_sha256
            and receipt['ownerRunID'] == owner_run_id and receipt['target'] == target
            and receipt['downloadName'] == download_name, 'Transfer receipt binding differs')
    stamp = receipt['observedUnixSeconds']
    require(type(stamp) in (int, float) and type(now) in (int, float)
            and math.isfinite(stamp) and math.isfinite(now) and 0 <= now - stamp <= 30,
            'Transfer receipt is stale or from the future')
    require(type(witnesses) is dict and set(witnesses) == set(WITNESSES)
            and type(receipt['witnessSHA256']) is dict
            and set(receipt['witnessSHA256']) == set(WITNESSES), 'Exact transfer witnesses required')
    common = {'runID': plan.execution.run_id, 'ownerRunID': owner_run_id, 'target': target}
    expected = {
        'namespace': {**common, 'state': 'FRESH_INPUT_NAMESPACE_CREATED_EMPTY',
                      'directory': plan.directory},
        'complete': {**common, 'state': 'EXACT_TRANSFER_COMPLETION_OBSERVED',
                     'method': 'MINIFTP_UI' if target == 'real-a1200' else 'OWNED_EMULATOR_TRANSFER',
                     'downloadName': download_name, 'bytes': len(plan.execution.binary)},
        'placement': {**common, 'state': 'SINGLE_NATIVE_FILE_PLACED_CONTROL_RELEASED',
                      'path': plan.path('NativeTest'), 'bytes': len(plan.execution.binary),
                      'members': ['NativeTest'], 'downloadPathAbsent': True,
                      'nativeTaskAbsent': True, 'uiControlReleased': True}}
    for name, body in witnesses.items():
        value = _json(body)
        require(hashlib.sha256(body).hexdigest() == receipt['witnessSHA256'][name],
                'Transfer witness changed')
        # JSON True and 1 compare equal in Python; canonical encoding also
        # preserves the exact scalar types required by this witness contract.
        require(json.dumps(value, sort_keys=True) == json.dumps(expected[name], sort_keys=True),
                'Transfer witness binding differs: ' + name)
    return True


class ReceiptObserver:
    """Service callback using its existing bounded reader and live guard.

    Fixed basenames prevent receipt contents from choosing a host read path.
    The Root request supplies the expected receipt hash, never the receipt itself.
    """
    def __init__(self, *, read, admit, plan, expected_sha256, native_plan_sha256,
                 owner_run_id, target, download_name, clock=time.time):
        require(all(callable(f) for f in (read, admit, clock)), 'Transfer observer callbacks required')
        self.read, self.admit, self.clock = read, admit, clock
        self.binding = dict(expected_sha256=expected_sha256, plan=plan,
                            native_plan_sha256=native_plan_sha256, owner_run_id=owner_run_id,
                            target=target, download_name=download_name)

    async def __call__(self, description, manifest):
        plan = self.binding['plan']
        expected = {name: {'bytes': len(body), 'sha256': hashlib.sha256(body).hexdigest()}
                    for name, body in plan.files().items()}
        require(description == plan.description() and manifest == expected,
                'External observer plan or manifest differs')
        require(self.admit() is True, 'Live transfer ownership absent')
        raw = self.read('root-transfer.json')
        witnesses = {name: self.read('root-transfer-' + name + '.json') for name in WITNESSES}
        verify_receipt(raw, witnesses, now=self.clock(), **self.binding)
        require(self.admit() is True, 'Live transfer ownership changed')
        return True
