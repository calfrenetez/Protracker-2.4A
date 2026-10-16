"""Receipt/service seam models only; no FTP, service or target operations."""
import copy
import hashlib
import json
import unittest

import test_owned_native_transferred_input as transferred
from owned_transfer_receipt import ReceiptObserver, verify_receipt


def encoded(value):
    return (json.dumps(value, sort_keys=True) + '\n').encode()


class ReceiptTests(unittest.IsolatedAsyncioTestCase):
    setUp = transferred.TransferredInputTests.setUp
    new = transferred.TransferredInputTests.new

    def prepare(self, target='real-a1200'):
        self.new()
        self.now, self.reads, self.guards = 100.0, [], []
        self.owner = 'd5' * 16
        self.common = {'runID': self.execution.run_id, 'ownerRunID': self.owner, 'target': target}
        self.witnesses = {
            'namespace': {**self.common, 'state': 'FRESH_INPUT_NAMESPACE_CREATED_EMPTY',
                          'directory': self.plan.directory},
            'complete': {**self.common, 'state': 'EXACT_TRANSFER_COMPLETION_OBSERVED',
                         'method': 'MINIFTP_UI' if target == 'real-a1200' else 'OWNED_EMULATOR_TRANSFER',
                         'downloadName': 'PT24G-TEST', 'bytes': len(self.execution.binary)},
            'placement': {**self.common, 'state': 'SINGLE_NATIVE_FILE_PLACED_CONTROL_RELEASED',
                          'path': self.execution.copy_source, 'bytes': len(self.execution.binary),
                          'members': ['NativeTest'], 'downloadPathAbsent': True,
                          'nativeTaskAbsent': True, 'uiControlReleased': True}}
        self.receipt = {'state': 'ROOT_OBSERVED_FRESH_TRANSFER_COMPLETE',
                        'description': self.plan.description(), 'nativePlanSHA256': 'e6' * 32,
                        'ownerRunID': self.owner, 'target': target, 'downloadName': 'PT24G-TEST',
                        'observedUnixSeconds': 100.0, 'witnessSHA256': {}}
        self.reseal()
        self.binding = dict(expected_sha256=hashlib.sha256(self.records['root-transfer.json']).hexdigest(),
                            plan=self.plan, native_plan_sha256='e6' * 32, owner_run_id=self.owner,
                            target=target, download_name='PT24G-TEST')

    def reseal(self):
        self.records = {'root-transfer-' + n + '.json': encoded(v) for n, v in self.witnesses.items()}
        self.receipt['witnessSHA256'] = {n: hashlib.sha256(self.records['root-transfer-' + n + '.json']).hexdigest()
                                       for n in self.witnesses}
        self.records['root-transfer.json'] = encoded(self.receipt)
        if hasattr(self, 'binding'):
            self.binding['expected_sha256'] = hashlib.sha256(self.records['root-transfer.json']).hexdigest()

    def read(self, name):
        self.reads.append(name)
        return self.records[name]

    def admit(self):
        self.guards.append(len(self.reads))
        return True

    def observer(self, **changes):
        return ReceiptObserver(read=self.read, admit=self.admit, clock=lambda: self.now,
                               **{**self.binding, **changes})

    async def refuses(self, observer=None):
        before = dict(self.bridge.files)
        with self.assertRaises((ValueError, OSError, KeyError)):
            await self.stage.prepare_transferred(observer or self.observer())
        self.assertEqual(self.stage.phase, 'FAILED')
        self.assertEqual(self.bridge.calls, [])
        self.assertEqual(self.bridge.files, before)
        with self.assertRaises(ValueError): await self.stage.prepare_transferred(self.observer())

    async def test_service_callback_admits_exact_target_records_then_only_content_reads(self):
        for target in ('real-a1200', 'amiberry-030'):
            self.prepare(target)
            ready = await self.stage.prepare_transferred(self.observer())
            self.assertEqual(ready['plan']['transfer'], 'external')
            self.assertEqual(self.guards, [0, 4])
            self.assertEqual(self.reads, ['root-transfer.json', 'root-transfer-namespace.json',
                                         'root-transfer-complete.json', 'root-transfer-placement.json'])
            self.assertEqual([c['type'] for c in self.bridge.calls], ['LISTDIR', 'CHECKSUM'])

    async def test_lease_target_candidate_or_native_plan_mismatch_refuses_before_protocol(self):
        for field, value in (('owner_run_id', 'f7' * 16), ('target', 'amiberry-030'),
                             ('native_plan_sha256', 'f7' * 32), ('download_name', 'OTHER'),
                             ('expected_sha256', 'f7' * 32)):
            self.prepare()
            await self.refuses(self.observer(**{field: value}))

    async def test_every_changed_witness_field_even_with_resealed_hash_is_refused(self):
        self.prepare()
        fields = [(name, key) for name, v in self.witnesses.items() for key in v]
        for name, key in fields:
            self.prepare()
            old = self.witnesses[name][key]
            self.witnesses[name][key] = 'changed' if not isinstance(old, list) else ['NativeTest', 'other']
            self.reseal()
            await self.refuses()

    async def test_numeric_one_cannot_claim_true_ownership_or_control_flags(self):
        for key in ('downloadPathAbsent', 'nativeTaskAbsent', 'uiControlReleased'):
            self.prepare()
            self.witnesses['placement'][key] = 1
            self.reseal()
            await self.refuses()

    async def test_changed_witness_body_requires_the_pinned_receipt_hash(self):
        for name in ('namespace', 'complete', 'placement'):
            self.prepare()
            self.records['root-transfer-' + name + '.json'] += b' '
            await self.refuses()

    async def test_wrong_transfer_backend_cannot_cross_targets(self):
        for target, wrong in (('real-a1200', 'OWNED_EMULATOR_TRANSFER'), ('amiberry-030', 'MINIFTP_UI')):
            self.prepare(target)
            self.witnesses['complete']['method'] = wrong
            self.reseal()
            await self.refuses()

    async def test_receipt_expiry_future_and_nonfinite_timestamps_are_refused(self):
        for stamp in (69.99, 100.01, True, float('nan'), float('inf')):
            self.prepare()
            self.receipt['observedUnixSeconds'] = stamp
            self.reseal()
            await self.refuses()
        for stamp in (70, 100):
            self.prepare()
            self.receipt['observedUnixSeconds'] = stamp
            self.reseal()
            await self.stage.prepare_transferred(self.observer())

    async def test_live_guard_refusal_or_owner_change_prevents_remote_reads(self):
        for when in (0, 4):
            self.prepare()
            def guard(): return len(self.reads) != when
            observer = self.observer()
            observer.admit = guard
            await self.refuses(observer)
            self.assertEqual(len(self.reads), when)

    async def test_missing_record_and_duplicate_key_never_start_transport(self):
        for name in ('root-transfer.json', 'root-transfer-namespace.json',
                     'root-transfer-complete.json', 'root-transfer-placement.json'):
            self.prepare()
            self.records.pop(name)
            await self.refuses()
        self.prepare()
        raw = self.records['root-transfer.json'].replace(b'{', b'{"state":"ignored",', 1)
        self.records['root-transfer.json'] = raw
        self.binding['expected_sha256'] = hashlib.sha256(raw).hexdigest()
        await self.refuses()

    async def test_receipt_cannot_choose_host_read_paths_or_extra_witnesses(self):
        self.prepare()
        self.receipt['hostPath'] = '/unowned/file'
        self.reseal()
        await self.refuses()
        self.assertEqual(len(self.reads), 4)
        self.assertTrue(all(n.startswith('root-transfer') and '/' not in n for n in self.reads))

    async def test_callback_cannot_admit_a_different_manifest(self):
        self.prepare()
        altered = copy.deepcopy(self.stage.manifest())
        altered['NativeTest']['sha256'] = 'f7' * 32
        with self.assertRaisesRegex(ValueError, 'manifest differs'):
            await self.observer()(self.plan.description(), altered)
        self.assertEqual((self.guards, self.reads, self.bridge.calls), ([], [], []))

    def test_bounded_records_flat_download_name_and_exact_bindings(self):
        self.prepare()
        raw = self.records['root-transfer.json']
        witnesses = {n: self.records['root-transfer-' + n + '.json'] for n in self.witnesses}
        for key, val in (('download_name', '../other'), ('owner_run_id', ''),
                         ('native_plan_sha256', 'bad'), ('target', 'stock')):
            with self.assertRaises(ValueError):
                verify_receipt(raw, witnesses, now=self.now, **{**self.binding, key: val})
        for raw in (b'', b' ' * 65537):
            with self.assertRaises(ValueError):
                verify_receipt(raw, witnesses, now=self.now,
                               **{**self.binding, 'expected_sha256': hashlib.sha256(raw).hexdigest()})


if __name__ == '__main__': unittest.main()
