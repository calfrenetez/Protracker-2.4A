"""Real filesystem operations in one private HOST fixture only; no harness/VM."""
import json
import fcntl
import os
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import editor_probe_namespace as ns


class ProbeNamespaceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='pt24g-namespace-HOST-', dir='/private/tmp')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        p = patch.object(ns, 'HARNESS', self.root/'harness'); p.start(); self.addCleanup(p.stop)
        (ns.HARNESS/'runtime/Dev/Tests').mkdir(parents=True)
        self.lock = ns.HARNESS/'runtime/test.lock'; self.lock.touch()
        self.fd = os.open(self.lock,os.O_RDWR|os.O_NOFOLLOW)
        fcntl.flock(self.fd,fcntl.LOCK_EX|fcntl.LOCK_NB)
        self.addCleanup(os.close,self.fd)
        self.window = self.root/'window'; self.window.mkdir()
        self.native = b'fixture native'; self.source = b'fixture source'
        for name, body in [('NATIVE', self.native), ('SOURCE', self.source)]:
            p = patch.object(ns, name, ns.digest(body)); p.start(); self.addCleanup(p.stop)
        native = ns.put(self.root/'native', self.native)
        source = ns.put(self.root/'source.c', self.source)
        receipt = {'state': 'PASSIVE_EDITOR_V2_SOURCE_AND_NATIVE_COMPILE_HOST_CHECKED_NOT_EXECUTED',
                   'binary': {'bytes':len(self.native), 'sha256':ns.digest(self.native)[1]},
                   'sourcePinsEqualBeforeAfter': {'tools/diagnostics/native_editor_pointer_snapshot.c':
                       {'bytes':len(self.source), 'sha256':ns.digest(self.source)[1]}}}
        self.b = {'ownerRunID':'fixture-owner', 'windowDirectory':str(self.window),
                  'nativeCandidate':native, 'nativeSource':source,
                  'compileReceipt':ns.save(self.root/'compile.json',receipt), 'sourcePins':{'fixture':source}}
        self.owner = 'fixture-owner'; self.vm_present = True; self.phase = 'READY'
        self.events = []; self.hook = None
        self.api = SimpleNamespace(stage_owner=self.stage_owner, stopped_owner=self.stopped_owner)
        self.stageout = self.window/'passive-probe-stage'
        self.settleout = self.window/'passive-probe-settlement'
        self.directory = ns.paths(self.owner)[1]

    def stage_owner(self, owner):
        self.events.append(('stage',owner))
        if self.owner != owner or not self.vm_present or self.phase != 'READY':
            raise RuntimeError('synthetic original FRAME/VM owner refusal')
        if self.hook: self.hook('stage', sum(e[0]=='stage' for e in self.events))

    def stopped_owner(self, owner, mode):
        self.events.append(('stopped',owner,mode))
        if self.owner != owner or self.vm_present or self.phase != ('HOLD' if mode=='failed' else 'READY'):
            raise RuntimeError('synthetic independently stopped owner refusal')
        if self.hook: self.hook('stopped',sum(e[0]=='stopped' for e in self.events))

    def stage(self):
        return ns.stage(self.b,self.stageout,self.api,lock_fd=self.fd)

    def stopped(self, mode='normal'):
        self.vm_present=False; self.phase='HOLD' if mode=='failed' else 'READY'

    def settle(self, stage, mode='normal'):
        return ns.settle_stopped(self.b,stage,self.settleout,self.api,mode=mode,lock_fd=self.fd)

    def facts(self, stage, settlement, mode='normal'):
        return ns.release_cleanup_facts(self.b,stage,settlement,self.api,mode=mode,lock_fd=self.fd)

    def test_stage_creates_only_exact_separate_owner_bound_probe(self):
        ref = self.stage(); record=json.loads(ns.pinned(ref))
        self.assertEqual(record['state'],'OWNED_PASSIVE_PROBE_STAGED')
        self.assertEqual(record['fileWriteAttempts'],1)
        self.assertEqual(ns.pinned(record['stagedCandidate']),self.native)
        self.assertEqual([p.name for p in self.directory.iterdir()],['PTEditorPointerV2'])
        self.assertFalse((self.directory.parent/'blank-capture-fixture').exists())
        self.assertEqual(ns.validate_stage(self.b,ref),record)

    def test_stage_replay_refuses_without_changing_binary_or_receipt(self):
        stage=self.stage(); before=ns.pinned(stage); binary=ns.read(self.directory/'PTEditorPointerV2')[1]
        with self.assertRaises(RuntimeError):self.stage()
        self.assertEqual(ns.pinned(stage),before);ns.pinned(binary)

    def test_existing_namespace_is_preserved_without_staging(self):
        self.directory.mkdir();(self.directory/'unrelated').write_bytes(b'preserve')
        with self.assertRaises(RuntimeError):self.stage()
        self.assertEqual((self.directory/'unrelated').read_bytes(),b'preserve')
        self.assertFalse(self.stageout.exists())

    def test_existing_stage_evidence_refuses_without_namespace_creation(self):
        self.stageout.mkdir();(self.stageout/'prior').write_bytes(b'preserve')
        with self.assertRaises(RuntimeError):self.stage()
        self.assertFalse(self.directory.exists())

    def test_owner_or_native_input_refusal_precedes_namespace_mutation(self):
        self.owner='other'
        with self.assertRaises(RuntimeError):self.stage()
        self.owner='fixture-owner';Path(self.b['nativeCandidate']['path']).write_bytes(b'changed')
        with self.assertRaises(RuntimeError):self.stage()
        self.assertFalse(self.directory.exists());self.assertFalse(self.stageout.exists())

    def test_owner_loss_after_write_retains_partial_stage_without_retry(self):
        def hook(kind,count):
            if kind=='stage' and count==4:raise RuntimeError('synthetic lost owner after write')
        self.hook=hook
        with self.assertRaises(RuntimeError):self.stage()
        failed=json.loads((self.stageout/'stage-failed.json').read_text())
        self.assertEqual(failed['fileWriteAttempts'],1)
        self.assertEqual((self.directory/'PTEditorPointerV2').read_bytes(),self.native)
        with self.assertRaises(RuntimeError):self.stage()

    def test_normal_stop_preserves_before_removal_and_yields_only_cleanup_facts(self):
        stage=self.stage();self.stopped();settled=self.settle(stage)
        value=json.loads(ns.pinned(settled))
        self.assertFalse(self.directory.exists())
        self.assertEqual(ns.pinned(value['preservedProbe']),self.native)
        self.assertTrue((self.settleout/'preserved-before-removal.json').is_file())
        self.assertEqual((value['filesRemoved'],value['directoriesRemoved']),(1,1))
        facts=self.facts(stage,settled)
        self.assertTrue(facts['probePreservedAndAbsent'])
        self.assertFalse(facts['guardReleaseClaimed'])
        self.assertFalse(facts['wholeApplicationOrRestorationAccepted'])

    def test_failed_case_uses_separate_held_owner_audit_without_releasing_hold(self):
        stage=self.stage();self.stopped('failed');settled=self.settle(stage,'failed')
        facts=self.facts(stage,settled,'failed')
        self.assertEqual(facts['mode'],'failed');self.assertEqual(self.phase,'HOLD')
        self.assertFalse(facts['guardReleaseClaimed'])
        self.assertFalse(json.loads(ns.pinned(settled))['nativeCleanExitClaimed'])

    def test_missing_stop_or_wrong_settlement_mode_never_removes_probe(self):
        stage=self.stage()
        with self.assertRaises(RuntimeError):self.settle(stage)
        self.stopped('failed')
        with self.assertRaises(RuntimeError):self.settle(stage,'normal')
        with self.assertRaises(RuntimeError):self.settle(stage,'other')
        self.assertEqual((self.directory/'PTEditorPointerV2').read_bytes(),self.native)
        self.assertFalse(self.settleout.exists())

    def test_extra_namespace_content_or_changed_probe_prevents_removal(self):
        stage=self.stage();self.stopped();(self.directory/'unexpected').write_bytes(b'preserve')
        with self.assertRaises(RuntimeError):self.settle(stage)
        self.assertTrue((self.directory/'PTEditorPointerV2').exists())
        self.assertEqual((self.directory/'unexpected').read_bytes(),b'preserve')
        self.assertFalse(self.settleout.exists())

    def test_copy_failure_retains_live_namespace_and_failed_evidence(self):
        stage=self.stage();self.stopped();original=ns.put
        def failed_copy(path,body,mode=0o600):
            if Path(path)==self.settleout/'PTEditorPointerV2':raise OSError('synthetic copy failure')
            return original(path,body,mode)
        with patch.object(ns,'put',side_effect=failed_copy),self.assertRaises(OSError):self.settle(stage)
        self.assertTrue((self.directory/'PTEditorPointerV2').is_file())
        self.assertEqual(json.loads((self.settleout/'settlement-failed.json').read_text())['filesRemoved'],0)
        with self.assertRaises(RuntimeError):self.settle(stage)

    def test_owner_loss_after_preservation_prevents_removal(self):
        stage=self.stage();self.stopped()
        def hook(kind,count):
            if kind=='stopped' and count==3:raise RuntimeError('synthetic owner changed')
        self.hook=hook
        with self.assertRaises(RuntimeError):self.settle(stage)
        self.assertTrue((self.settleout/'PTEditorPointerV2').is_file())
        self.assertTrue((self.directory/'PTEditorPointerV2').is_file())

    def test_failure_after_unlink_is_recorded_without_replaying_partial_cleanup(self):
        stage=self.stage();self.stopped()
        def hook(kind,count):
            if kind=='stopped' and count==4:raise RuntimeError('synthetic failure after unlink')
        self.hook=hook
        with self.assertRaises(RuntimeError):self.settle(stage)
        value=json.loads((self.settleout/'settlement-failed.json').read_text())
        self.assertEqual((value['filesRemoved'],value['directoriesRemoved']),(1,0))
        self.assertTrue(self.directory.is_dir());self.assertFalse((self.directory/'PTEditorPointerV2').exists())
        self.hook=None
        with self.assertRaises((RuntimeError,FileNotFoundError)):self.settle(stage)
        self.assertTrue(self.directory.is_dir())

    def test_binary_change_after_preservation_prevents_unlink(self):
        stage=self.stage();self.stopped()
        def hook(kind,count):
            if kind=='stopped' and count==3:(self.directory/'PTEditorPointerV2').write_bytes(b'changed')
        self.hook=hook
        with self.assertRaises(RuntimeError):self.settle(stage)
        self.assertEqual((self.directory/'PTEditorPointerV2').read_bytes(),b'changed')
        self.assertEqual((self.settleout/'PTEditorPointerV2').read_bytes(),self.native)

    def test_reappeared_namespace_blocks_release_fact_without_deleting_it(self):
        stage=self.stage();self.stopped();settled=self.settle(stage)
        self.directory.mkdir();(self.directory/'new-content').write_bytes(b'preserve')
        with self.assertRaises(RuntimeError):self.facts(stage,settled)
        self.assertEqual((self.directory/'new-content').read_bytes(),b'preserve')

    def test_preserved_evidence_drift_or_mode_mismatch_blocks_release_fact(self):
        stage=self.stage();self.stopped();settled=self.settle(stage)
        self.phase='HOLD'
        with self.assertRaises(RuntimeError):self.facts(stage,settled,'failed')
        self.phase='READY';(self.settleout/'PTEditorPointerV2').write_bytes(b'changed')
        with self.assertRaises(RuntimeError):self.facts(stage,settled)

    def test_release_or_native_exit_claim_and_retry_flags_are_never_adopted(self):
        stage=self.stage();self.stopped();settled=self.settle(stage)
        path=Path(settled['path']);base=json.loads(ns.pinned(settled))
        for key in ('guardReleaseClaimed','nativeCleanExitClaimed','retryAllowed'):
            changed=dict(base);changed[key]=True;path.write_text(json.dumps(changed))
            current=ns.read(path)[1]
            with self.subTest(key=key),self.assertRaises(RuntimeError):self.facts(stage,current)

    def test_unsafe_owner_and_evidence_paths_are_refused(self):
        for owner in ('../other','_leading','',True):
            with self.subTest(owner=owner),self.assertRaises(RuntimeError):ns.paths(owner)
        with self.assertRaises(RuntimeError):ns.stage(self.b,self.window/'other-stage',self.api,lock_fd=self.fd)
        self.assertFalse(self.directory.exists())

    def test_symlink_shared_tests_directory_refuses_before_mutation(self):
        tests=ns.HARNESS/'runtime/Dev/Tests';tests.rmdir()
        elsewhere=self.root/'unrelated';elsewhere.mkdir();tests.symlink_to(elsewhere)
        with self.assertRaises(RuntimeError):self.stage()
        self.assertEqual(list(elsewhere.iterdir()),[])

    def test_wrong_or_competing_lock_descriptor_refuses_before_owner_or_write(self):
        wrong=os.open(self.b['nativeCandidate']['path'],os.O_RDWR)
        try:
            with self.assertRaises(RuntimeError):ns.stage(self.b,self.stageout,self.api,lock_fd=wrong)
        finally:os.close(wrong)
        other=os.open(self.lock,os.O_RDWR)
        try:
            with self.assertRaises(BlockingIOError):ns.stage(self.b,self.stageout,self.api,lock_fd=other)
        finally:os.close(other)
        self.assertEqual(self.events,[]);self.assertFalse(self.directory.exists())

    def test_helpers_retain_callers_original_command_lock_after_return(self):
        stage=self.stage();self.stopped();settled=self.settle(stage);self.facts(stage,settled)
        other=os.open(self.lock,os.O_RDWR)
        try:
            with self.assertRaises(BlockingIOError):fcntl.flock(other,fcntl.LOCK_EX|fcntl.LOCK_NB)
        finally:os.close(other)


if __name__=='__main__':unittest.main()
