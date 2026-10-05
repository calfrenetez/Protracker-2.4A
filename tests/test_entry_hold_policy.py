"""Local-filesystem model tests; no transport, guest or source probe.

All observation hashes/identities are deliberately synthetic. These tests can
qualify local ordering/durability/refusal mechanics only, never genuine native
completion, quiet, restoration, task/IRQ provenance or stack permission.
"""

import fcntl
import hashlib
import json
import os
import stat
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import entry_hold_policy as policy


MARKER = "ENTRY HOLD POLICY MODEL PASS: durable exact-run reservations, irreversible holds and separately evidenced release; local host policy only"


def specification(**changes):
    result = {
        "candidate_sha256": "a" * 64, "candidate_bytes": 12345,
        "run_id": "1" * 32, "target_kind": "EMULATOR",
        "target_instance": "model-emulator-030", "target_epoch": "model-pid-42-start-17-profile-bbbb",
        "process_identity": "model-guest-Process-00010000-session-1",
        "task_identity": "model-guest-Task-00010000-session-1",
        "controller_identity": "model-controller-A", "launch_handle": "model-launch-envelope-1",
        "pass_rc": 0, "clean_refusal_rc": 20,
        "pass_markers": ["MODEL ENTRY PASS session1", "MODEL QUIET session1"],
        "clean_refusal_markers": ["MODEL CLEAN REFUSAL session1", "MODEL QUIET session1"],
    }
    result.update(changes)
    return result


def observation(p, kind, **changes):
    facts = {key: True for key in policy.POSITIVE_FACTS[kind]} if kind != "COMPLETION" else {
        "returned": True, "result": "SOFTWARE_PASS", "rc": 0,
        "markers": p.spec["pass_markers"],
    }
    result = {"kind": kind, "binding": p.binding,
              "record_sha256": hashlib.sha256(("model-independent-record-" + kind).encode()).hexdigest(),
              "observer": "model-observer-" + kind, "facts": facts}
    result.update(changes)
    return result


class HoldPolicyModel(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="pt-entry-hold-model-")
        # macOS /tmp may itself be a symlink; supply the already-resolved real
        # private path, then independently test lexical symlink refusals below.
        self.root = Path(self.temp.name).resolve()
        self.spec = specification()
        self.candidate = self.root / "model-candidate-evidence.bin"
        self.candidate_bytes = b"MODEL ONLY: saved candidate bytes must survive every uncertain run\n"
        self.candidate.write_bytes(self.candidate_bytes)
        self.spec["candidate_bytes"] = len(self.candidate_bytes)
        self.spec["candidate_sha256"] = hashlib.sha256(self.candidate_bytes).hexdigest()
        self.p = policy.EntryHoldPolicy.reserve(self.root, self.spec)

    def tearDown(self):
        # Deletes only disposable model records, never a guest run or HUNK.
        self.temp.cleanup()

    def reopen(self):
        return policy.EntryHoldPolicy.reopen(self.root, self.spec, self.p.ticket)

    def start(self):
        self.p.record(observation(self.p, "STARTED"))

    def complete(self):
        self.start()
        self.p.record(observation(self.p, "COMPLETION"))

    def positives(self):
        for kind in ("OWNERSHIP", "RESTORATION", "ABSENCE"):
            self.p.record(observation(self.p, kind))

    def assert_no_actions(self, p=None):
        state = (p or self.p).status()
        self.assertFalse(state["stop_unload_retry_permission"])
        self.assertFalse(state["native_launch_clearance"])
        self.assertFalse(state["target_cleanup_permission"])
        self.assertFalse(state["authenticity_verified"])
        self.assertFalse(state["central_lifecycle_enforced"])

    def test_01_exact_durable_reservation_reopens_without_launch_permission(self):
        self.assertEqual(self.p.status()["state"], "PREPARED")
        self.assertEqual(self.reopen().status(), self.p.status())
        for path in (self.p.active, self.p.journal, self.p.reservation):
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
        self.spec["candidate_bytes"] = 1  # input copy cannot mutate retained spec
        self.assertEqual(self.p.spec["candidate_bytes"], len(self.candidate_bytes))
        self.assert_no_actions()

    def test_02_same_target_is_exclusive_across_controller_run_epoch_and_candidate(self):
        before = self.p.journal.read_bytes()
        variants = [specification(run_id="2" * 32), specification(controller_identity="model-controller-B"),
                    specification(target_epoch="model-reboot-new-pid"), specification(candidate_sha256="b" * 64)]
        for spec in variants:
            with self.subTest(spec=spec):
                with self.assertRaises(policy.PolicyError):
                    policy.EntryHoldPolicy.reserve(self.root, spec)
        self.assertEqual(self.p.journal.read_bytes(), before)
        self.assertTrue(self.p.active.exists())

    def test_03_unknown_or_unbounded_identity_refuses_before_reservation(self):
        invalid = [specification(run_id="0" * 32), specification(process_identity=""),
                   specification(candidate_bytes=True), specification(pass_rc=True),
                   specification(target_epoch="x" * 257), specification(pass_markers=[]),
                   specification(pass_markers=["same", "same"]), specification(candidate_sha256=123)]
        for spec in invalid:
            with self.subTest(spec=spec):
                with self.assertRaises(policy.PolicyError):
                    policy.EntryHoldPolicy.reserve(self.root, spec)
        self.assertEqual(self.p.status()["event_count"], 1)

    def test_04_completion_alone_never_authorizes_cleanup_or_release(self):
        self.complete()
        self.assertEqual(self.p.status()["state"], "COMPLETED")
        self.assertFalse(self.p.status()["local_cleanup_evidence_complete"])
        with self.assertRaises(policy.PolicyError):
            self.p.release()
        with self.assertRaises(policy.PolicyError):
            self.p.record(observation(self.p, "CLEANUP"))
        self.assertTrue(self.p.active.exists())
        self.assert_no_actions()

    def test_05_separate_positive_restoration_absence_and_cleanup_gate_release(self):
        self.complete()
        for kind in ("OWNERSHIP", "RESTORATION"):
            self.p.record(observation(self.p, kind))
            self.assertFalse(self.p.status()["local_cleanup_evidence_complete"])
        self.p.record(observation(self.p, "ABSENCE"))
        self.assertTrue(self.p.status()["local_cleanup_evidence_complete"])
        self.assertFalse(self.p.status()["local_release_evidence_complete"])
        self.p.record(observation(self.p, "CLEANUP"))
        self.assertTrue(self.p.status()["local_release_evidence_complete"])
        self.p.release()
        self.assertFalse(self.p.active.exists())
        self.assertTrue(self.reopen().status()["reservation_released"])
        self.assertTrue(self.p.journal.exists())
        self.assertFalse(self.p.status()["local_cleanup_evidence_complete"])
        released = {x: x.read_bytes() for x in (self.p.journal, self.p.reservation)}
        with self.assertRaises(policy.PolicyError):
            self.p.release()
        self.assertFalse(self.p.hold.exists())
        for path, data in released.items():
            self.assertEqual(path.read_bytes(), data)
        self.assert_no_actions()

    def test_06_clean_refusal_has_its_own_exact_rc_and_complete_markers(self):
        self.start()
        proof = observation(self.p, "COMPLETION")
        proof["facts"] = {"returned": True, "result": "CLEAN_REFUSAL", "rc": 20,
                          "markers": self.p.spec["clean_refusal_markers"]}
        self.p.record(proof)
        self.assertEqual(self.p.status()["state"], "COMPLETED")
        self.positives()
        self.assertTrue(self.p.status()["local_cleanup_evidence_complete"])

    def test_07_partial_wrong_rc_or_async_ack_is_irreversible_hold(self):
        variants = [{"rc": 1}, {"markers": ["MODEL ENTRY PASS session1"]}, {"returned": False},
                    {"rc": True}, {"result": ["SOFTWARE_PASS"]}]
        for number, change in enumerate(variants, 3):
            spec = specification(run_id=f"{number:032x}", target_instance=f"model-target-{number}")
            p = policy.EntryHoldPolicy.reserve(self.root, spec)
            p.record(observation(p, "STARTED"))
            proof = observation(p, "COMPLETION")
            proof["facts"].update(change)
            with self.assertRaises(policy.PolicyError):
                p.record(proof)
            self.assertEqual(p.status()["state"], "HOLD")
            self.assertTrue(p.active.exists())
            self.assertFalse(p.status()["local_cleanup_evidence_complete"])
            self.assert_no_actions(p)

    def test_08_bounded_observation_exhaustion_without_completion_is_hold(self):
        self.start()
        self.p.finish_observation()
        count = self.p.status()["event_count"]
        self.p.finish_observation()
        self.assertEqual(self.p.status()["event_count"], count)
        self.assertEqual(self.reopen().status()["state"], "HOLD")
        with self.assertRaises(policy.PolicyError):
            self.p.release()
        self.assertTrue(self.p.active.exists())

    def test_09_timeout_disconnect_missing_identity_and_uncertain_source_retain(self):
        reasons = ("TIMEOUT", "DISCONNECT", "TRANSPORT_UNCERTAIN", "PROCESS_IDENTITY_UNCERTAIN", "SOURCE_QUIET_UNCERTAIN")
        for number, reason in enumerate(reasons, 20):
            p = policy.EntryHoldPolicy.reserve(self.root, specification(run_id=f"{number:032x}", target_instance=f"model-{number}"))
            p.uncertainty(reason)
            count = p.status()["event_count"]
            marker = p.hold.read_bytes()
            p.uncertainty("DISCONNECT")
            self.assertEqual(p.hold.read_bytes(), marker)
            self.assertEqual(p.status()["event_count"], count)
            self.assertEqual(p.status()["state"], "HOLD")
            self.assertTrue(p.active.exists())
            self.assert_no_actions(p)

    def test_10_late_positive_completion_does_not_clear_hold_or_enable_retry(self):
        self.start()
        self.p.uncertainty("TIMEOUT")
        hold = self.p.hold.read_bytes()
        self.p.record(observation(self.p, "COMPLETION"))
        self.positives()
        self.assertEqual(self.p.status()["state"], "HOLD")
        self.assertFalse(self.p.status()["local_cleanup_evidence_complete"])
        with self.assertRaises(policy.PolicyError):
            self.p.release()
        self.p.record(observation(self.p, "RECOVERY"))
        self.assertTrue(self.p.status()["local_cleanup_evidence_complete"])
        self.p.record(observation(self.p, "CLEANUP"))
        self.p.release()
        reopened = self.reopen()
        self.assertEqual(reopened.status()["state"], "HOLD")
        self.assertTrue(reopened.status()["held_irreversibly"])
        self.assertTrue(reopened.status()["reservation_released"])
        self.assertEqual(self.p.hold.read_bytes(), hold)
        with self.assertRaises(policy.PolicyError):
            policy.EntryHoldPolicy.reserve(self.root, self.spec)
        fresh = policy.EntryHoldPolicy.reserve(self.root, specification(run_id="2" * 32))
        self.assertEqual(fresh.status()["state"], "PREPARED")
        self.assert_no_actions(reopened)

    def test_11_recovery_is_separate_and_cannot_preapprove_a_future_hold(self):
        self.complete()
        self.positives()
        with self.assertRaises(policy.PolicyError):
            self.p.record(observation(self.p, "RECOVERY"))
        self.assertEqual(self.p.status()["state"], "HOLD")
        self.p.uncertainty("SOURCE_QUIET_UNCERTAIN")
        self.assertFalse(self.p.status()["local_release_evidence_complete"])

    def test_12_false_restoration_and_same_record_alias_fail_closed(self):
        self.complete()
        proof = observation(self.p, "RESTORATION")
        proof["facts"]["source_quiet"] = False
        with self.assertRaises(policy.PolicyError):
            self.p.record(proof)
        self.assertEqual(self.p.status()["state"], "HOLD")
        alias = observation(self.p, "OWNERSHIP", record_sha256=observation(self.p, "COMPLETION")["record_sha256"])
        with self.assertRaises(policy.PolicyError):
            self.p.record(alias)
        self.assertNotIn("OWNERSHIP", self.p.status()["evidence_kinds"])
        self.assertTrue(self.p.active.exists())

    def test_13_independent_absence_observer_cannot_be_completion_or_restoration(self):
        for number, kind in enumerate(("COMPLETION", "RESTORATION"), 31):
            p = policy.EntryHoldPolicy.reserve(self.root, specification(run_id=f"{number:032x}", target_instance=f"model-{number}"))
            p.record(observation(p, "STARTED"))
            p.record(observation(p, "COMPLETION"))
            p.record(observation(p, "RESTORATION"))
            with self.assertRaises(policy.PolicyError):
                p.record(observation(p, "ABSENCE", observer=observation(p, kind)["observer"]))
            self.assertEqual(p.status()["state"], "HOLD")
            self.assertNotIn("ABSENCE", p.status()["evidence_kinds"])

    def test_14_wrong_credential_or_candidate_cannot_modify_another_history(self):
        original = {p: p.read_bytes() for p in (self.p.reservation, self.p.journal, self.p.active)}
        with self.assertRaises(policy.PolicyError):
            policy.EntryHoldPolicy.reopen(self.root, self.spec, "f" * 32)
        with self.assertRaises(policy.PolicyError):
            policy.EntryHoldPolicy.reopen(self.root, specification(candidate_sha256="b" * 64), self.p.ticket)
        self.assertFalse(self.p.hold.exists())
        for path, data in original.items():
            self.assertEqual(path.read_bytes(), data)

    def test_15_changed_target_or_wrong_run_evidence_never_releases(self):
        self.complete()
        wrong = observation(self.p, "OWNERSHIP", binding="b" * 64)
        with self.assertRaises(policy.PolicyError):
            self.p.record(wrong)
        self.assertEqual(self.p.status()["state"], "HOLD")
        self.assertFalse(self.p.status()["local_release_evidence_complete"])
        self.assertTrue(self.p.active.exists())

    def test_16_truncated_or_rehashed_out_of_order_journal_cannot_release(self):
        self.start()
        with self.p.journal.open("ab") as stream:
            stream.write(b"{\"partial\":")
        with self.assertRaises(policy.PolicyError):
            self.p.status()
        marker = json.loads(self.p.hold.read_bytes())
        self.assertEqual(marker["reason"], "CORRUPT_JOURNAL")
        with self.assertRaises(policy.PolicyError):
            self.p.release()
        self.assertTrue(self.p.active.exists())
        p = policy.EntryHoldPolicy.reserve(self.root, specification(run_id="3" * 32, target_instance="model-other"))
        rows = [json.loads(x) for x in p.journal.read_bytes().splitlines()]
        rows[0]["sequence"] = 1
        unsigned = {k: v for k, v in rows[0].items() if k != "sha256"}
        rows[0]["sha256"] = hashlib.sha256(json.dumps(unsigned, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        p.journal.write_bytes(json.dumps(rows[0], sort_keys=True, separators=(",", ":")).encode() + b"\n")
        with self.assertRaises(policy.PolicyError):
            p.status()
        self.assertTrue(p.hold.exists())

    def test_17_durable_marker_survives_interrupted_hold_event_and_is_never_reset(self):
        self.start()
        with mock.patch.object(self.p, "_append", side_effect=OSError("model host write interruption")):
            with self.assertRaises(OSError):
                self.p.uncertainty("DISCONNECT")
        marker = self.p.hold.read_bytes()
        reopened = self.reopen()
        self.assertEqual(reopened.status()["state"], "HOLD")
        reopened.record(observation(reopened, "COMPLETION"))
        self.assertEqual(reopened.status()["state"], "HOLD")
        self.assertEqual(reopened.hold.read_bytes(), marker)
        kinds = [json.loads(x)["kind"] for x in reopened.journal.read_bytes().splitlines()]
        self.assertEqual(kinds, ["RESERVED", "STARTED", "HOLD", "COMPLETION"])

    def test_18_append_fsync_failure_preserves_external_hold_contract(self):
        self.start()
        real_fsync = os.fsync
        calls = 0

        def fail_first(fd):
            nonlocal calls
            calls += 1
            if calls == 1:
                raise OSError("model fsync uncertainty")
            return real_fsync(fd)

        with mock.patch.object(policy.os, "fsync", side_effect=fail_first):
            with self.assertRaises(policy.StorageError):
                self.p.record(observation(self.p, "COMPLETION"))
        self.assertEqual(self.reopen().status()["state"], "HOLD")
        self.assertTrue(self.p.active.exists())
        self.assert_no_actions()

    def test_19_finite_capacity_and_unknown_reason_refuse_without_unbounded_polling(self):
        with self.assertRaises(policy.PolicyError):
            self.p.uncertainty("retry-after-deadline")
        self.start()
        with mock.patch.object(policy, "MAX_EVENTS", 2):
            with self.assertRaises(policy.PolicyError):
                self.p.record(observation(self.p, "COMPLETION"))
        self.assertEqual(self.p.status()["state"], "HOLD")
        self.assertEqual(self.p.status()["event_count"], 2)
        self.assertTrue(self.p.active.exists())

    def test_20_local_lock_busy_and_lexical_symlinks_never_take_over(self):
        fd = os.open(self.p.target / "policy.lock", os.O_RDWR)
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            with self.assertRaises(policy.PolicyError):
                self.p.status()
        finally:
            os.close(fd)
        link = self.root / "linked-root"
        link.symlink_to(self.root, target_is_directory=True)
        with self.assertRaises(policy.PolicyError):
            policy.EntryHoldPolicy.reserve(link, specification(run_id="4" * 32))
        self.assertEqual(self.p.status()["state"], "PREPARED")

    def test_21_dangling_active_and_nonordinary_journal_are_refusals(self):
        root = self.root / "separate-policy"
        root.mkdir()
        p = policy.EntryHoldPolicy(root, specification(), "e" * 32)
        p.target.mkdir()
        p.active.symlink_to(root / "missing")
        with self.assertRaises(policy.PolicyError):
            policy.EntryHoldPolicy.reserve(root, specification())
        # A FIFO must be refused without a blocking open or content traversal.
        self.p.journal.unlink()
        os.mkfifo(self.p.journal)
        with self.assertRaises(policy.PolicyError):
            self.p.status()
        self.assertTrue(self.p.hold.exists())

    def test_22_release_requires_exact_active_owner_and_never_deletes_run_records(self):
        self.complete()
        self.positives()
        self.p.record(observation(self.p, "CLEANUP"))
        data = self.p.active.read_bytes()
        active = json.loads(data)
        active["ticket"] = "d" * 32
        self.p.active.write_text(json.dumps(active))
        with self.assertRaises(policy.PolicyError):
            self.p.release()
        self.assertTrue(self.p.journal.exists())
        self.assertTrue(self.p.reservation.exists())
        self.assertTrue(self.p.active.exists())
        self.p.active.write_bytes(data)  # disposable model recovery, not target action
        self.assertEqual(self.p.status()["state"], "HOLD")
        with self.assertRaises(policy.PolicyError):
            self.p.release()
        self.assertTrue(self.p.journal.exists())

    def test_23_uncertainty_and_forbidden_release_preserve_candidate_and_run_artifacts(self):
        self.start()
        evidence_log = self.p.run / "saved-observation.stdout"
        evidence_log.write_bytes(b"MODEL saved partial output, no genuine final completion\n")
        self.p.uncertainty("TRANSPORT_UNCERTAIN")
        retained = {path: path.read_bytes() for path in (
            self.candidate, evidence_log, self.p.journal, self.p.reservation, self.p.hold, self.p.active)}
        for _ in range(3):
            with self.assertRaises(policy.PolicyError):
                self.p.release()
            self.assert_no_actions(self.reopen())
        for path, original in retained.items():
            self.assertEqual(path.read_bytes(), original)
        self.assertEqual(hashlib.sha256(self.candidate.read_bytes()).hexdigest(), self.p.spec["candidate_sha256"])

    def test_24_hold_invalidates_old_closure_observations_and_requires_fresh_recovery(self):
        self.complete()
        self.positives()
        self.p.record(observation(self.p, "CLEANUP"))
        self.assertTrue(self.p.status()["local_release_evidence_complete"])
        self.p.uncertainty("SOURCE_QUIET_UNCERTAIN")
        self.assertEqual(self.p.status()["evidence_kinds"], ["COMPLETION", "STARTED"])
        with self.assertRaises(policy.PolicyError):
            self.p.record(observation(self.p, "OWNERSHIP"))  # retained old hash
        with self.assertRaises(policy.PolicyError):
            self.p.record(observation(self.p, "RECOVERY"))  # no fresh closure proof
        self.p.record(observation(self.p, "OWNERSHIP", record_sha256=hashlib.sha256(b"OWNERSHIP-post-hold").hexdigest()))
        self.p.uncertainty("DISCONNECT")  # invalidate even fresh post-HOLD evidence
        self.assertEqual(self.p.status()["evidence_kinds"], ["COMPLETION", "STARTED"])
        for kind in ("OWNERSHIP", "RESTORATION", "ABSENCE"):
            self.p.record(observation(self.p, kind, record_sha256=hashlib.sha256((kind + "-after-second-hold").encode()).hexdigest(),
                                      observer="model-fresh-observer-" + kind))
        self.p.record(observation(self.p, "RECOVERY"))
        self.assertFalse(self.p.status()["local_release_evidence_complete"])
        self.p.record(observation(self.p, "CLEANUP", record_sha256=hashlib.sha256(b"CLEANUP-post-hold").hexdigest()))
        self.p.release()
        self.assertEqual(self.reopen().status()["state"], "HOLD")
        self.assertTrue(self.p.status()["reservation_released"])
        self.assert_no_actions()

    def test_25_invalid_later_receipt_fences_old_closure_before_interrupted_append(self):
        self.complete()
        self.p.uncertainty("TIMEOUT")
        self.positives()
        self.p.record(observation(self.p, "RECOVERY"))
        self.assertTrue(self.p.status()["local_cleanup_evidence_complete"])
        original_hold = self.p.hold.read_bytes()
        original_journal = self.p.journal.read_bytes()
        bad = observation(self.p, "CLEANUP")
        bad["facts"]["open_run_paths_absent"] = False
        with mock.patch.object(self.p, "_append", side_effect=OSError("model second uncertainty append interruption")):
            with self.assertRaises(OSError):
                self.p.record(bad)
        reopened = self.reopen()
        self.assertEqual(reopened.status()["state"], "HOLD")
        self.assertEqual(reopened.status()["evidence_kinds"], ["COMPLETION", "STARTED"])
        self.assertFalse(reopened.status()["local_cleanup_evidence_complete"])
        with self.assertRaises(policy.PolicyError):
            reopened.release()
        self.assertEqual(self.p.hold.read_bytes(), original_hold)
        self.assertEqual(self.p.journal.read_bytes(), original_journal)
        self.assertEqual(self.candidate.read_bytes(), self.candidate_bytes)
        reopened.record(observation(reopened, "OWNERSHIP", record_sha256=hashlib.sha256(b"OWNERSHIP-after-invalid-later-receipt").hexdigest()))
        self.assertNotIn("RECOVERY", reopened.status()["evidence_kinds"])
        self.assertFalse(reopened.status()["local_release_evidence_complete"])
        self.assertTrue(self.p.active.exists())
        self.assert_no_actions(reopened)

    def test_26_missing_original_hold_marker_permanently_blocks_old_closure(self):
        self.complete()
        self.p.uncertainty("TIMEOUT")
        self.positives()
        self.p.record(observation(self.p, "RECOVERY"))
        self.p.record(observation(self.p, "CLEANUP"))
        self.assertTrue(self.p.status()["local_release_evidence_complete"])
        journal = self.p.journal.read_bytes()
        self.p.hold.unlink()  # disposable-model corruption, never guest cleanup
        with self.assertRaises(policy.PolicyError):
            self.p.status()
        self.assertTrue(self.p.corruption.exists())
        latch = self.p.corruption.read_bytes()
        for _ in range(2):
            with self.assertRaises(policy.PolicyError):
                self.p.status()
            with self.assertRaises(policy.PolicyError):
                self.p.release()
        self.assertEqual(self.p.corruption.read_bytes(), latch)
        self.assertEqual(self.p.journal.read_bytes(), journal)
        self.assertTrue(self.p.active.exists())
        self.assertEqual(self.candidate.read_bytes(), self.candidate_bytes)

    def test_27_transient_journal_read_failure_permanently_blocks_old_closure(self):
        self.complete()
        self.p.uncertainty("DISCONNECT")
        self.positives()
        self.p.record(observation(self.p, "RECOVERY"))
        self.p.record(observation(self.p, "CLEANUP"))
        journal = self.p.journal.read_bytes()
        original_read = policy._read

        def fail_journal_only(path, limit):
            if path == self.p.journal:
                raise OSError("model transient journal read failure")
            return original_read(path, limit)

        with mock.patch.object(policy, "_read", side_effect=fail_journal_only):
            with self.assertRaises(policy.PolicyError):
                self.p.status()
        # The original bytes now read normally, but no local auto-repair or
        # reinterpretation may restore release eligibility after corruption.
        self.assertEqual(self.p.journal.read_bytes(), journal)
        with self.assertRaises(policy.PolicyError):
            self.reopen()
        with self.assertRaises(policy.PolicyError):
            self.p.release()
        self.assertTrue(self.p.corruption.exists())
        self.assertTrue(self.p.active.exists())

    def test_28_release_directory_fsync_failure_is_not_central_barrier_proof(self):
        self.complete()
        self.positives()
        self.p.record(observation(self.p, "CLEANUP"))
        original_fsync = os.fsync
        target = self.p.target.stat()

        def fail_target_directory(fd):
            info = os.fstat(fd)
            if stat.S_ISDIR(info.st_mode) and (info.st_dev, info.st_ino) == (target.st_dev, target.st_ino):
                raise OSError("model release directory sync uncertainty")
            return original_fsync(fd)

        with mock.patch.object(policy.os, "fsync", side_effect=fail_target_directory):
            with self.assertRaises(OSError):
                self.p.release()
        self.assertFalse(self.p.active.exists())
        self.assertTrue(self.p.status()["reservation_released"])
        self.assert_no_actions()
        self.assertEqual(self.candidate.read_bytes(), self.candidate_bytes)
        # A central caller MUST retain its external hold after the exception.
        # An absent local active file or RELEASED record cannot authorize run.


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(HoldPolicyModel)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print(MARKER)
    sys.exit(0 if result.wasSuccessful() else 1)
