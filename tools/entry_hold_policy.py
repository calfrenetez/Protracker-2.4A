"""Private host journal only; not a launch, transport or guest-lifetime barrier.

Every caller must enforce the same reservation in all central lifecycle wrappers.
Any storage error requires an external unresolved hold: no target action, retry,
cleanup or code/process unload. Evidence records are authenticated by the caller,
not by this module. Positive booleans below record claims, not OS certificates.
"""

import contextlib
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import stat


SCHEMA = "PRIVATE_ENTRY_HOLD_V1"
MAX_EVENTS = 16
MAX_JOURNAL_BYTES = 65536
MAX_RECORD_BYTES = 8192
HEX64 = re.compile(r"[0-9a-f]{64}\Z")
HEX32 = re.compile(r"[0-9a-f]{32}\Z")
IDENTITY_FIELDS = (
    "target_instance", "target_epoch", "process_identity", "task_identity",
    "controller_identity", "launch_handle",
)
SPEC_FIELDS = set(IDENTITY_FIELDS) | {
    "candidate_sha256", "candidate_bytes", "run_id", "target_kind",
    "pass_rc", "clean_refusal_rc", "pass_markers", "clean_refusal_markers",
}
PROOF_FIELDS = {"kind", "binding", "record_sha256", "observer", "facts"}
POSITIVE_FACTS = {
    "STARTED": {"normal_dos_task", "exact_process", "exact_task", "retained_lifetimes"},
    "OWNERSHIP": {"exclusive_reservation", "peer_clear", "central_barriers_held", "unchanged_target"},
    "RESTORATION": {"source_quiet", "vector_restored", "os_restored", "original_target_verified"},
    "ABSENCE": {"process_absent", "source_absent", "run_handles_closed"},
    "CLEANUP": {"exact_run_paths_only", "open_run_paths_absent", "original_target_idle", "acquired_control_released"},
    "RECOVERY": {"separate_explicit_authorization", "human_or_coordinated_exact_target_review", "exact_owner_enforcement", "no_automatic_retry"},
}
UNCERTAINTY = {
    "TIMEOUT", "DISCONNECT", "MISSING_COMPLETION", "TRANSPORT_UNCERTAIN",
    "PROCESS_IDENTITY_UNCERTAIN", "SOURCE_QUIET_UNCERTAIN", "INVALID_COMPLETION",
    "UNPROVED_OBSERVATION", "CORRUPT_JOURNAL", "EVENT_LIMIT", "STORAGE_UNCERTAIN",
}


class PolicyError(Exception):
    """Refusal; this exception grants no action or lifetime-release permission."""


class StorageError(PolicyError):
    """Durability is unknown: the central caller MUST retain its external hold."""


class OwnershipUncertain(PolicyError):
    """Exact local owner changed; a durable fence forbids old closure claims."""


def _json(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("ascii")


def _sha(value):
    return hashlib.sha256(value).hexdigest()


def _text(value):
    return isinstance(value, str) and 0 < len(value) <= 256 and all(32 <= ord(c) < 127 for c in value)


def _spec(value):
    if not isinstance(value, dict) or set(value) != SPEC_FIELDS:
        raise PolicyError("exact specification fields required")
    if not isinstance(value["candidate_sha256"], str) or not HEX64.fullmatch(value["candidate_sha256"]) or type(value["candidate_bytes"]) is not int or not 0 < value["candidate_bytes"] <= 2**32:
        raise PolicyError("exact positive candidate binding required")
    if not isinstance(value["run_id"], str) or not HEX32.fullmatch(value["run_id"]) or value["run_id"] == "0" * 32:
        raise PolicyError("fresh nonzero run identity required")
    if not isinstance(value["target_kind"], str) or value["target_kind"] not in {"EMULATOR", "PHYSICAL"} or not all(_text(value[k]) for k in IDENTITY_FIELDS):
        raise PolicyError("exact target, epoch, Process/task and controller identities required")
    for key in ("pass_rc", "clean_refusal_rc"):
        if type(value[key]) is not int or not 0 <= value[key] <= 255:
            raise PolicyError("source-contract return code required")
    for key in ("pass_markers", "clean_refusal_markers"):
        markers = value[key]
        if not isinstance(markers, list) or not 1 <= len(markers) <= 8 or not all(_text(x) for x in markers) or len(set(markers)) != len(markers):
            raise PolicyError("bounded exact source-contract markers required")
    return json.loads(_json(value))


def _lexical_directory(path):
    p = Path(path)
    if not p.is_absolute() or ".." in p.parts:
        raise PolicyError("absolute lexical private directory required")
    for component in (p, *p.parents):
        if component.is_symlink():
            raise PolicyError("symlink directory refused before resolution")
    if not p.is_dir():
        raise PolicyError("existing private policy directory required")
    return p


def _read(path, limit):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        metadata = os.fstat(fd)
        if not stat.S_ISREG(metadata.st_mode):
            raise PolicyError("ordinary durable file required")
        size = metadata.st_size
        if not 0 < size <= limit:
            raise PolicyError("missing or oversized durable record")
        data = b""
        while len(data) < size:
            block = os.read(fd, size - len(data))
            if not block:
                raise PolicyError("partial durable record")
            data += block
        return data
    finally:
        os.close(fd)


def _write(fd, data):
    offset = 0
    while offset < len(data):
        count = os.write(fd, data[offset:])
        if count <= 0:
            raise StorageError("short journal write; external hold required")
        offset += count
    os.fsync(fd)


def _new(path, value):
    data = _json(value) + b"\n"
    if len(data) > MAX_RECORD_BYTES:
        raise PolicyError("bounded record exceeded")
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    try:
        _write(fd, data)
    finally:
        os.close(fd)


def _sync_directory(path):
    fd = os.open(path, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


class EntryHoldPolicy:
    """Serialized local record API. No operation invokes a transport or guest.

    The private root/ancestors must remain immutable, ordinary local directories
    under the central controller's ownership. This is not an adversarial-path or
    authorization/security service. Spec/ticket and authentic observations must
    be held by the central caller; knowing them does not prove real ownership.
    """

    def __init__(self, root, spec, ticket):
        self.root = _lexical_directory(root)
        self.spec = _spec(spec)
        self.binding = _sha(_json(self.spec))
        if not isinstance(ticket, str) or not HEX32.fullmatch(ticket):
            raise PolicyError("exact reservation ticket required")
        self.ticket = ticket
        # The reservation is exclusive across runs AND controller identities for
        # the same target, not just the same guest Process or candidate.
        target_key = _sha(_json([self.spec["target_kind"], self.spec["target_instance"]]))
        self.target = self.root / target_key
        self.run = self.target / self.spec["run_id"]
        self.active = self.target / "active.json"
        self.journal = self.run / "journal.jsonl"
        self.reservation = self.run / "reservation.json"
        self.hold = self.run / "hold.json"
        self.corruption = self.run / "corruption.json"
        self.owner = {"schema": SCHEMA, "binding": self.binding, "run_id": self.spec["run_id"], "ticket": self.ticket}

    @contextlib.contextmanager
    def _locked(self):
        _lexical_directory(self.root)
        _lexical_directory(self.target)
        fd = os.open(self.target / "policy.lock", os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW, 0o600)
        try:
            if not stat.S_ISREG(os.fstat(fd).st_mode):
                raise PolicyError("ordinary local lock file required")
            # A busy local controller refuses promptly; it cannot take over a
            # target simply because another command's transport has timed out.
            try:
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError as exc:
                raise PolicyError("local policy controller busy") from exc
            yield
        finally:
            os.close(fd)

    @classmethod
    def reserve(cls, root, spec):
        p = cls(root, spec, secrets.token_hex(16))
        try:
            if not p.target.exists():
                p.target.mkdir(mode=0o700)
                _sync_directory(p.root)
            with p._locked():
                if p.active.exists() or p.active.is_symlink():
                    raise PolicyError("target already reserved, including unresolved holds")
                # Reusing a run, even after positive release, is never a retry.
                p.run.mkdir(mode=0o700, exist_ok=False)
                _new(p.reservation, p.owner)
                p._append([], "RESERVED", {"spec": p.spec, "ticket": p.ticket})
                _new(p.active, p.owner)
                _sync_directory(p.run)
                _sync_directory(p.target)
            return p
        except OSError as exc:
            raise StorageError("reservation not durably established; do not dispatch") from exc

    @classmethod
    def reopen(cls, root, spec, ticket):
        p = cls(root, spec, ticket)
        with p._locked():
            p._view(require_active=False)
        return p

    def _own(self):
        if json.loads(_read(self.active, MAX_RECORD_BYTES)) != self.owner:
            raise PolicyError("exact active reservation owner mismatch")

    def _hold_now(self, reason):
        marker = dict(self.owner, reason=reason)
        if self.hold.exists() or self.hold.is_symlink():
            existing = json.loads(_read(self.hold, MAX_RECORD_BYTES))
            if not isinstance(existing, dict) or set(existing) != set(self.owner) | {"reason"} or {k: existing.get(k) for k in self.owner} != self.owner or not isinstance(existing.get("reason"), str) or existing["reason"] not in UNCERTAINTY:
                raise StorageError("unreadable hold identity; external hold required")
            return
        _new(self.hold, marker)
        _sync_directory(self.run)

    def _fence_now(self, events, reason):
        """Latch this uncertainty BEFORE append; retain first HOLD unchanged."""
        self._hold_now(reason)
        anchor = events[-1]["sha256"]
        path = self.run / ("fence-" + anchor + ".json")
        if path.exists() or path.is_symlink():
            existing = json.loads(_read(path, MAX_RECORD_BYTES))
            expected = dict(self.owner, anchor=anchor)
            if not isinstance(existing, dict) or set(existing) != set(expected) | {"reason"} or {k: existing.get(k) for k in expected} != expected or not isinstance(existing.get("reason"), str) or existing["reason"] not in UNCERTAINTY:
                raise StorageError("uncertainty fence unreadable; external hold required")
            return {"anchor": anchor, "reason": existing["reason"]}
        _new(path, dict(self.owner, anchor=anchor, reason=reason))
        _sync_directory(self.run)
        return {"anchor": anchor, "reason": reason}

    def _corrupt_now(self):
        """Permanent refusal: untrusted/read-failed history is never repaired."""
        value = dict(self.owner, reason="CORRUPT_JOURNAL")
        if self.corruption.exists() or self.corruption.is_symlink():
            if json.loads(_read(self.corruption, MAX_RECORD_BYTES)) != value:
                raise StorageError("corruption latch identity uncertain; external hold required")
            return
        _new(self.corruption, value)
        _sync_directory(self.run)

    def _fences(self):
        fences = {}
        # The root is private and immutable to foreign writers. Even if that
        # contract is violated, stop enumeration after the finite bound.
        for number, path in enumerate(self.run.glob("fence-*.json")):
            if number >= MAX_EVENTS:
                raise StorageError("finite fence bound exceeded; external hold required")
            anchor = path.name[6:-5]
            if not HEX64.fullmatch(anchor):
                raise StorageError("invalid fence filename; external hold required")
            value = json.loads(_read(path, MAX_RECORD_BYTES))
            expected = dict(self.owner, anchor=anchor)
            if not isinstance(value, dict) or set(value) != set(expected) | {"reason"} or {k: value.get(k) for k in expected} != expected or not isinstance(value.get("reason"), str) or value["reason"] not in UNCERTAINTY:
                raise StorageError("fence identity uncertain; external hold required")
            fences[anchor] = value["reason"]
        return fences

    def _consume_pending(self, events):
        consumed = {x["payload"]["anchor"] for x in events if x["kind"] == "HOLD"}
        pending = {k: v for k, v in self._fences().items() if k not in consumed}
        if not pending:
            return
        anchor = events[-1]["sha256"]
        if set(pending) != {anchor}:
            raise StorageError("unresolved fence not at exact journal tail; external hold required")
        self._append(events, "HOLD", {"anchor": anchor, "reason": pending[anchor]})

    def _mark_uncertainty(self, events, proofs, held, reason):
        closure = set(proofs) - {"STARTED", "COMPLETION"}
        if not held or closure:
            payload = self._fence_now(events, reason)
            self._append(events, "HOLD", payload)
        else:
            # Already held with no current closure evidence: repeated reports
            # are idempotent. A prior failed append must first be consumed.
            self._hold_now(reason)
            self._consume_pending(events)

    def _append(self, events, kind, payload):
        if len(events) >= MAX_EVENTS:
            self._fence_now(events, "EVENT_LIMIT")
            raise PolicyError("finite journal full; retained hold")
        record = {"schema": SCHEMA, "binding": self.binding, "sequence": len(events),
                  "previous": events[-1]["sha256"] if events else "0" * 64,
                  "kind": kind, "payload": payload}
        record["sha256"] = _sha(_json(record))
        data = _json(record) + b"\n"
        if len(data) > MAX_RECORD_BYTES:
            raise PolicyError("bounded journal event exceeded")
        flags = os.O_WRONLY | os.O_APPEND | os.O_NOFOLLOW | os.O_NONBLOCK
        if not events:
            flags |= os.O_CREAT | os.O_EXCL
        fd = os.open(self.journal, flags, 0o600)
        try:
            metadata = os.fstat(fd)
            if not stat.S_ISREG(metadata.st_mode):
                raise PolicyError("ordinary journal file required")
            if metadata.st_size + len(data) > MAX_JOURNAL_BYTES:
                self._fence_now(events, "EVENT_LIMIT")
                raise PolicyError("finite journal byte limit; retained hold")
            _write(fd, data)
        except OSError as exc:
            try:
                if events:
                    self._fence_now(events, "STORAGE_UNCERTAIN")
            except (OSError, ValueError, PolicyError):
                pass
            raise StorageError("journal durability unknown; external hold required") from exc
        finally:
            os.close(fd)

    def _proof(self, value, kind):
        if not isinstance(value, dict) or set(value) != PROOF_FIELDS or value["kind"] != kind or value["binding"] != self.binding:
            raise PolicyError("exact separate evidence kind/run binding required")
        if not isinstance(value["record_sha256"], str) or not HEX64.fullmatch(value["record_sha256"]) or not _text(value["observer"]):
            raise PolicyError("bounded authenticated evidence record identity required")
        facts = value["facts"]
        if kind == "COMPLETION":
            if not isinstance(facts, dict) or set(facts) != {"returned", "result", "rc", "markers"} or facts["returned"] is not True:
                raise PolicyError("positive returned completion required")
            if not isinstance(facts["result"], str):
                raise PolicyError("exact terminal result name required")
            prefix = {"SOFTWARE_PASS": "pass", "CLEAN_REFUSAL": "clean_refusal"}.get(facts["result"])
            if not prefix or type(facts["rc"]) is not int or facts["rc"] != self.spec[prefix + "_rc"] or facts["markers"] != self.spec[prefix + "_markers"]:
                raise PolicyError("exact genuine final return code and complete markers required")
        elif not isinstance(facts, dict) or set(facts) != POSITIVE_FACTS[kind] or any(v is not True for v in facts.values()):
            raise PolicyError("every distinct positive observation required")
        return json.loads(_json(value))

    def _view(self, require_active=True):
        _lexical_directory(self.run)
        # Check the immutable per-run credential before any fail-closed marker
        # write, including historical reads after the active file is released.
        try:
            original_owner = json.loads(_read(self.reservation, MAX_RECORD_BYTES))
        except (OSError, ValueError, PolicyError) as exc:
            raise StorageError("original credential durability unknown; external hold required") from exc
        if original_owner != self.owner:
            raise PolicyError("exact original reservation credential mismatch")
        if self.corruption.exists() or self.corruption.is_symlink():
            self._corrupt_now()
            raise StorageError("permanent corruption hold; external reviewed recovery required")
        try:
            raw = _read(self.journal, MAX_JOURNAL_BYTES)
            if not raw.endswith(b"\n"):
                raise PolicyError("partial final journal event")
            events = [json.loads(line) for line in raw.splitlines()]
            if not 1 <= len(events) <= MAX_EVENTS:
                raise PolicyError("invalid finite event count")
            proofs, released, started, held, used_records = {}, False, False, False, set()
            fences, consumed_fences = self._fences(), set()
            previous = "0" * 64
            for sequence, event in enumerate(events):
                if set(event) != {"schema", "binding", "sequence", "previous", "kind", "payload", "sha256"}:
                    raise PolicyError("invalid event fields")
                unsigned = {k: v for k, v in event.items() if k != "sha256"}
                if event["schema"] != SCHEMA or event["binding"] != self.binding or type(event["sequence"]) is not int or event["sequence"] != sequence or event["previous"] != previous or event["sha256"] != _sha(_json(unsigned)):
                    raise PolicyError("journal identity/hash/order mismatch")
                previous = event["sha256"]
                kind, payload = event["kind"], event["payload"]
                if released:
                    raise PolicyError("event after final release")
                if sequence == 0:
                    if kind != "RESERVED" or payload != {"spec": self.spec, "ticket": self.ticket}:
                        # Wrong supplied spec/ticket is a caller refusal, not a
                        # reason to alter another exact owner's history.
                        raise PolicyError("original reservation identity mismatch")
                elif kind in POSITIVE_FACTS or kind == "COMPLETION":
                    if kind in proofs:
                        raise PolicyError("duplicate evidence domain")
                    self._next(proofs, kind, held)
                    proof = self._proof(payload, kind)
                    self._distinct(proofs, proof)
                    if proof["record_sha256"] in used_records:
                        raise PolicyError("old observation reused across hold phases")
                    used_records.add(proof["record_sha256"])
                    proofs[kind] = proof
                    if kind == "STARTED":
                        started = True
                    elif kind == "COMPLETION" and not started:
                        raise PolicyError("completion without exact observed start")
                elif kind == "HOLD":
                    if not isinstance(payload, dict) or set(payload) != {"anchor", "reason"} or payload["anchor"] != event["previous"] or fences.get(payload["anchor"]) != payload["reason"] or payload["anchor"] in consumed_fences:
                        raise PolicyError("invalid uncertainty fence event")
                    consumed_fences.add(payload["anchor"])
                    held = True
                    proofs = {k: v for k, v in proofs.items() if k in {"STARTED", "COMPLETION"}}
                elif kind == "RELEASED":
                    if payload != {"binding": self.binding, "held": held} or not self._eligible(proofs, held, cleanup=True):
                        raise PolicyError("unproved reservation release")
                    released = True
                else:
                    raise PolicyError("unknown journal transition")
            if self.hold.exists() or self.hold.is_symlink():
                self._hold_now("CORRUPT_JOURNAL")  # verifies immutable existing identity
                held = True
                if set(fences) != consumed_fences or not any(x["kind"] == "HOLD" for x in events):
                    proofs = {k: v for k, v in proofs.items() if k in {"STARTED", "COMPLETION"}}
            if "HOLD" in [x["kind"] for x in events] and not self.hold.exists():
                raise PolicyError("durable hold marker missing")
            if fences and not self.hold.exists():
                raise PolicyError("original hold record missing")
            if not released:
                try:
                    self._own()
                except (OSError, ValueError, PolicyError) as exc:
                    self._fence_now(events, "PROCESS_IDENTITY_UNCERTAIN")
                    raise OwnershipUncertain("active owner uncertain") from exc
            return events, proofs, held, released, started
        except (OSError, ValueError, KeyError, TypeError, PolicyError) as exc:
            if isinstance(exc, OwnershipUncertain):
                raise
            # A wrong reopen credential must not create a hold for another run.
            if isinstance(exc, PolicyError) and str(exc) == "original reservation identity mismatch":
                raise
            try:
                self._corrupt_now()
                self._hold_now("CORRUPT_JOURNAL")
            except (OSError, ValueError, PolicyError) as marker_error:
                raise StorageError("journal AND hold durability unknown; external hold required") from marker_error
            raise PolicyError("corrupt journal: durable unresolved hold, no release") from exc

    @staticmethod
    def _next(proofs, kind, held):
        if kind == "COMPLETION" and "STARTED" not in proofs:
            raise PolicyError("completion without exact observed start")
        if kind in {"OWNERSHIP", "RESTORATION"} and "COMPLETION" not in proofs:
            raise PolicyError("release observations must follow genuine completion")
        if kind == "ABSENCE" and not {"COMPLETION", "RESTORATION"} <= set(proofs):
            raise PolicyError("independent absence must follow restoration")
        if kind == "RECOVERY" and (not held or not EntryHoldPolicy._eligible(proofs, False)):
            raise PolicyError("recovery approval requires HOLD and separate positive evidence")
        if kind == "CLEANUP" and not EntryHoldPolicy._eligible(proofs, held):
            raise PolicyError("exact path cleanup requires prior independent evidence")

    @staticmethod
    def _distinct(proofs, proof):
        if any(x["record_sha256"] == proof["record_sha256"] for x in proofs.values()):
            raise PolicyError("separate observation records required")
        if proof["kind"] == "ABSENCE" and proof["observer"] in {
                proofs.get("COMPLETION", {}).get("observer"), proofs.get("RESTORATION", {}).get("observer")}:
            raise PolicyError("independent absence observer required")

    @staticmethod
    def _eligible(proofs, held, cleanup=False):
        required = {"STARTED", "COMPLETION", "OWNERSHIP", "RESTORATION", "ABSENCE"}
        if cleanup:
            required.add("CLEANUP")
        if held:
            required.add("RECOVERY")
        if not required <= set(proofs):
            return False
        records = [proofs[k]["record_sha256"] for k in required]
        return len(set(records)) == len(records) and proofs["ABSENCE"]["observer"] not in {
            proofs["COMPLETION"]["observer"], proofs["RESTORATION"]["observer"]}

    def status(self):
        with self._locked():
            events, proofs, held, released, started = self._view(require_active=False)
            return {"state": "HOLD" if held else "COMPLETED" if "COMPLETION" in proofs else "STARTED" if started else "PREPARED",
                    "held_irreversibly": held, "reservation_released": released,
                    "event_count": len(events), "evidence_kinds": sorted(proofs),
                    "local_cleanup_evidence_complete": not released and self._eligible(proofs, held),
                    "local_release_evidence_complete": not released and self._eligible(proofs, held, cleanup=True),
                    "target_cleanup_permission": False, "authenticity_verified": False,
                    "central_lifecycle_enforced": False, "stop_unload_retry_permission": False,
                    "native_launch_clearance": False}

    def record(self, evidence):
        kind = evidence.get("kind") if isinstance(evidence, dict) else None
        if not isinstance(kind, str) or (kind not in POSITIVE_FACTS and kind != "COMPLETION"):
            raise PolicyError("known separate evidence domain required")
        with self._locked():
            events, proofs, held, released, started = self._view()
            if released or kind in proofs:
                raise PolicyError("final or duplicate observation refused")
            self._consume_pending(events)
            events, proofs, held, released, started = self._view()
            try:
                proof = self._proof(evidence, kind)
                self._distinct(proofs, proof)
                if any(x["kind"] in POSITIVE_FACTS or x["kind"] == "COMPLETION"
                       for x in events if isinstance(x["payload"], dict)
                       and x["payload"].get("record_sha256") == proof["record_sha256"]):
                    raise PolicyError("old observation reused across hold phases")
                self._next(proofs, kind, held)
            except PolicyError:
                self._mark_uncertainty(events, proofs, held, "INVALID_COMPLETION" if kind == "COMPLETION" else "UNPROVED_OBSERVATION")
                raise
            self._append(events, kind, proof)

    def uncertainty(self, reason):
        if not isinstance(reason, str) or reason not in UNCERTAINTY:
            raise PolicyError("fixed uncertainty reason required")
        with self._locked():
            events, proofs, held, released, started = self._view()
            if released:
                raise PolicyError("released run cannot be reused")
            self._mark_uncertainty(events, proofs, held, reason)

    def finish_observation(self):
        """Bounded polling ended; absence of genuine completion becomes HOLD."""
        with self._locked():
            events, proofs, held, released, started = self._view()
            if released:
                raise PolicyError("released run cannot be observed again")
            if "COMPLETION" not in proofs:
                self._mark_uncertainty(events, proofs, held, "MISSING_COMPLETION")

    def release(self):
        """Release LOCAL reservation only; never touch target paths/processes.

        All target restoration/absence/exact cleanup observations must already
        exist. For HOLD, a separate explicit recovery approval is also required.
        HOLD history and this run directory remain permanently preserved.
        """
        with self._locked():
            events, proofs, held, released, started = self._view()
            if released:
                raise PolicyError("released history cannot be released again")
            if not self._eligible(proofs, held, cleanup=True):
                raise PolicyError("separate exact positive release evidence incomplete")
            if not released:
                self._append(events, "RELEASED", {"binding": self.binding, "held": held})
            # Never free/delete the run journal or an uncertain guest owner.
            self._own()
            self.active.unlink()
            _sync_directory(self.target)
