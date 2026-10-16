"""Prepared RAM-only native fixture transaction over a caller-owned transport.

No connection, process lifecycle, service hook or CLI entry point is provided.
The caller must qualify these exact sources in Amiberry before physical use,
hold the shared reservation/locks, verify the live target and record durable
intent before every exchange. This draft does NOT activate the physical runner.

MAKEDIR must mean successful CreateDir, never 'already exists'. That grants the
cooperating run a new namespace without relying on a negative directory listing.
LAUNCH executes an explicit uploaded file; SCRIPT and its hidden T: files are
never used. All failure paths retain ownership and propagate without recovery.

The explicit CIA observation profiles are separate from fixed-output
fixture acceptance: a consistent RC20 report retains qualification FAILED.
Completion/custody/settlement cannot promote that measurement to PASS. It does
not authorize a live run, resource closure or independent ownership release.
"""
import asyncio
from dataclasses import dataclass
import hashlib
import math
import re
import time
import zlib

import native_file_protocol as wire
import native_fixture_capture as capture
import cia_aperture_capture as aperture
import cia_prepared_capture as prepared_aperture
import clock_pair_capture as clock_pair

DIAGNOSTICS = {aperture.PROFILE: aperture, prepared_aperture.PROFILE: prepared_aperture,
               clock_pair.PROFILE: clock_pair}


def require(value, message):
    if not value:
        raise ValueError(message)


@dataclass(frozen=True)
class Plan:
    run_id: str
    command_id: int
    binary: bytes
    markers: tuple
    stack: int = 524288
    copy_source: str | None = None
    progress_sidecar: bool = False
    observation_profile: str | None = None

    def __post_init__(self):
        require(isinstance(self.run_id, str) and re.fullmatch(r"[0-9a-f]{32}", self.run_id),
                "Exact fresh run identity required")
        require(type(self.command_id) is int and 1 <= self.command_id <= 0xffffffff,
                "Explicit positive command identity required")
        require(isinstance(self.binary, bytes) and 0 < len(self.binary) <= wire.MAX_UPLOAD_BYTES,
                "Frozen bounded candidate required")
        require(type(self.stack) is int and 65536 <= self.stack <= 1048576, "Qualified Stack required")
        require(type(self.progress_sidecar) is bool, "Explicit progress-sidecar flag required")
        require(isinstance(self.markers, tuple) and 0 < len(self.markers) <= 256
                and len(set(self.markers)) == len(self.markers)
                and all(isinstance(x, str) and x and len(x) <= 256 and x.isascii()
                        and x.isprintable() for x in self.markers), "Exact native marker contract required")
        if self.observation_profile is None:
            require(len(self.expected_output) <= capture.MAX_OUTPUT, "Native output contract exceeds bound")
        else:
            require(type(self.observation_profile) is str and self.observation_profile in DIAGNOSTICS
                    and self.markers == DIAGNOSTICS[self.observation_profile].MARKERS and not self.progress_sidecar,
                    "Exact separate diagnostic observation contract required")
        if self.copy_source is not None:
            wire.path(self.copy_source)
            shared = re.fullmatch(re.escape("Dev:Tests/PT24G-" + self.run_id[:16] + "/")
                                  + r"[A-Za-z0-9_-]+", self.copy_source)
            staged = self.copy_source == "RAM:PT24G-" + self.run_id[:16] + "-in/NativeTest"
            require((shared or staged)
                    and self.command_id < 0xffffffff,
                    "Exact run-owned staging source and distinct copy identity required")

    @property
    def directory(self):
        # Keep the component within the classic 30-character name limit. The
        # full identity remains in the record; CreateDir must reject collisions.
        return "RAM:PT24G-" + self.run_id[:16]

    def path(self, name):
        require(name in self.names, "Not a declared run file")
        return self.directory + "/" + name

    @property
    def names(self):
        names = ("NativeTest", "Run-once", "complete.flag", "control.log", "native.log")
        return (names + (("Stage-once", "staged.flag", "stage.log") if self.copy_source else ())
                + (("progress.log",) if self.progress_sidecar else ()))

    @property
    def prefix(self):
        return "PT" + self.run_id[:16] + "-"

    @property
    def script(self):
        return ("FailAt 255\n" + capture.run_script(self.path("NativeTest"), self.path("native.log"),
                                                 self.stack, self.prefix, control=self.path("control.log"),
                                                 progress=self.path("progress.log") if self.progress_sidecar else None)
                + "Echo COMPLETE >" + self.path("complete.flag") + "\n").encode("ascii")

    @property
    def command(self):
        return "C:Execute " + self.path("Run-once")

    @property
    def expected_output(self):
        require(self.observation_profile is None, "Measured diagnostic has no fixed-output oracle")
        return ("\n".join(self.markers) + "\n").encode("ascii")

    @property
    def output_limit(self):
        return DIAGNOSTICS[self.observation_profile].MAX_OUTPUT if self.observation_profile is not None else capture.MAX_OUTPUT

    def observe(self, control, native):
        if self.observation_profile is not None:
            diagnostic = DIAGNOSTICS[self.observation_profile]
            observed = diagnostic.controls("[OK]\n" + control, self.stack, self.prefix)
            observed["diagnostic"] = diagnostic.validate(native, observed["returnCode"])
            return observed  # RC20/FAILED is retained, never converted into a fixture PASS.
        observed = capture.parse_control("[OK]\n" + control, self.stack, self.prefix)
        require(native == self.expected_output, "Native output differs from exact marker contract")
        return observed

    @property
    def staging_script(self):
        require(self.copy_source is not None, "No staging copy source selected")
        failure = f"If WARN\n Echo FAILED >{self.path('staged.flag')}\n Quit 20\nEndIf\n"
        return (f"FailAt 255\nC:Copy {self.copy_source} {self.path('NativeTest')} QUIET\n"
                + failure + f"C:Protect {self.path('NativeTest')} +rwed\n" + failure
                + f"Echo {self.prefix}STAGE-DONE >{self.path('stage.log')}\n"
                + f"Echo STAGED >{self.path('staged.flag')}\n").encode('ascii')

    @property
    def staging_command(self):
        return f"C:Execute {self.path('Stage-once')}"

    def prepared_files(self):
        files = {"NativeTest": self.binary, "Run-once": self.script, "complete.flag": b""}
        if self.copy_source:
            files.update({"Stage-once": self.staging_script, "staged.flag": b"STAGED\n",
                          "stage.log": (self.prefix + "STAGE-DONE\n").encode('ascii')})
        if self.progress_sidecar:
            files['progress.log'] = b'NOT STARTED\n'
        return files

    def description(self):
        result = {"runID": self.run_id, "commandID": self.command_id,
                "directory": self.directory, "files": list(self.names),
                "candidateBytes": len(self.binary), "candidateSHA256": hashlib.sha256(self.binary).hexdigest(),
                "scriptSHA256": hashlib.sha256(self.script).hexdigest(), "command": self.command,
                "stackBytes": self.stack, "expectedMarkers": list(self.markers)}
        if self.copy_source:
            result.update(copySource=self.copy_source, copyCommandID=self.command_id + 1,
                          stagingScriptSHA256=hashlib.sha256(self.staging_script).hexdigest(),
                          stagingCommand=self.staging_command)
            if self.copy_source.startswith("RAM:"):
                result['copySourceKind'] = 'ram-staged'
        if self.progress_sidecar:
            result['progressSidecar'] = self.path('progress.log')
        if self.observation_profile is not None:
            result['observationProfile'] = self.observation_profile
            result['observationScope'] = 'Collection and reported closure only; diagnostic qualification/RC retained separately'
        return result


class Transaction:
    def __init__(self, plan, exchange, record, *, seconds=250, clock=time.monotonic, sleep=asyncio.sleep):
        require(type(plan) is Plan and callable(exchange) and callable(record) and callable(clock)
                and callable(sleep) and type(seconds) in (float, int) and math.isfinite(seconds)
                and 0 < seconds <= 420, "Invalid finite transaction")
        self.plan, self.exchange, self.record = plan, exchange, record
        self.clock, self.sleep = clock, sleep
        self.deadline = clock() + seconds
        self.phase = "NEW"
        self.custody = None
        self.calls = 0

    def remaining(self):
        value = self.deadline - self.clock()
        require(value > 0, "Whole transaction deadline expired")
        return value

    async def ask(self, command, reply_type, seconds=4):
        require(self.phase not in ("FAILED", "SETTLED"), "Transaction is closed or failed")
        require(self.calls < 4096, "Exchange count bound reached")
        timeout = min(seconds, self.remaining())
        self.calls += 1
        # The caller must fsync this record before returning. Payload bytes stay
        # in the separately frozen input/custody file, not in the event ledger.
        intent = {k: v for k, v in command.items() if k != "hexData"}
        if "hexData" in command:
            body = bytes.fromhex(command["hexData"])
            intent.update(bytes=len(body), sha256=hashlib.sha256(body).hexdigest())
        self.record({"event": "dispatch", "number": self.calls, "command": intent})
        try:
            raw = await asyncio.wait_for(self.exchange(command, reply_type, timeout), timeout=timeout)
        except wire.BridgeReplyError as error:
            self.record({"event": "error_reply", "number": self.calls, "raw": error.raw})
            raise
        self.remaining()
        wire.line(raw)
        self.record({"event": "reply", "number": self.calls, "raw": raw})
        return raw

    async def _operation(self, expected, active, operation):
        require(self.phase == expected, "Transaction phase does not permit this operation")
        self.phase = active
        try:
            return await operation()
        except BaseException:
            self.phase = "FAILED"
            raise

    async def checksum(self, name, data):
        await self.checksum_path(self.plan.path(name), data)

    async def checksum_path(self, remote, data):
        wire.path(remote)
        raw = await self.ask({"type": "CHECKSUM", "path": remote}, "CHECKSUM")
        match = re.fullmatch(re.escape("CHECKSUM|" + remote + "|") + r"([0-9a-fA-F]{8})\|(0|[1-9][0-9]*)", raw)
        require(match and int(match[1], 16) == zlib.crc32(data) and int(match[2]) == len(data),
                "Guest bytes/CRC differ from frozen file")

    async def upload(self, name, data):
        for command in wire.write_commands(self.plan.path(name), data):
            raw = await self.ask(command, "OK")
            require(raw == f"OK|WRITEFILE|{len(command['hexData']) // 2}",
                    "Exact write acknowledgement absent")
        await self.checksum(name, data)

    def observer(self, seconds=60):
        return wire.Observation(self.ask, seconds=min(seconds, 60, self.remaining()), clock=self.clock)

    async def completion_size(self, name, size, deadline, allowed):
        """Observe a pending Shell output without opening a missing file.

        Redirection can temporarily remove or partially write the marker. A
        complete directory reply permits those pending states; it cannot prove
        success. Only the exact terminal size permits the existing strict file
        read, content, custody and checksum checks. Errors still end this run.
        """
        entries = await self.observer(deadline - self.clock()).directory(self.plan.directory)
        require(self.clock() < deadline, "Completion observation deadline expired")
        require(all(e.name in allowed and e.kind == "F" for e in entries),
                "Completion namespace contains an unowned entry")
        marker = next((e for e in entries if e.name == name), None)
        require(marker is None or marker.size <= size, "Completion marker exceeds bound")
        return 0 if marker is None else marker.size

    async def copy_candidate(self):
        """Copy caller-pinned shared or RAM staging; never alters its source.

        A RAM source needs a separately qualified transfer, fresh namespace
        ownership and eventual caller cleanup. This operation neither creates
        that source nor grants ownership over an existing file by its name.
        """
        await self.checksum_path(self.plan.copy_source, self.plan.binary)
        await self.upload("Stage-once", self.plan.staging_script)
        await self.upload("staged.flag", b"")
        require(await self.observer().file(self.plan.path("Stage-once")) == self.plan.staging_script,
                "Uploaded staging script differs")
        deadline = min(self.deadline, self.clock() + 30)
        ident, command = self.plan.command_id + 1, self.plan.staging_command
        raw = await self.ask({"type": "LAUNCH", "id": ident, "command": command}, "CMD", 10)
        require(re.fullmatch(re.escape(f"CMD|{ident}|OK|Started") +
                             r"(?: \(proc [1-9][0-9]*\))?: " + re.escape(command), raw),
                "Exact staging launch acknowledgement absent")
        for _ in range(80):
            remaining = deadline - self.clock()
            require(remaining > 0, "Staging completion deadline expired")
            path = self.plan.path("staged.flag")
            size = await self.completion_size("staged.flag", 7, deadline,
                                              {"NativeTest", "Stage-once", "staged.flag", "stage.log"})
            if size == 7:
                flag = await self.observer(deadline - self.clock()).file(path, 7)
                require(self.clock() < deadline and flag == b"STAGED\n", "Exact timely staging success absent")
                break
            await self.sleep(min(.5, max(0, deadline - self.clock())))
        else:
            raise ValueError("Staging observation count exceeded")
        require(await self.observer().file(self.plan.path("stage.log")) ==
                self.plan.prepared_files()["stage.log"], "Staging output differs")
        await self.checksum("NativeTest", self.plan.binary)
        await self.checksum_path(self.plan.copy_source, self.plan.binary)

    async def _prepare_namespace(self):
        self.record({"event": "plan", "plan": self.plan.description()})
        raw = await self.ask({"type": "MAKEDIR", "path": self.plan.directory}, "OK")
        require(raw == "OK|MAKEDIR|" + self.plan.directory, "Fresh directory creation unconfirmed")
        require(await self.observer().directory(self.plan.directory) == (), "New directory is not empty")

    async def _prepare_controls(self):
        for name, data in (("Run-once", self.plan.script), ("complete.flag", b"")):
            await self.upload(name, data)
        if self.plan.progress_sidecar:
            await self.upload('progress.log', self.plan.prepared_files()['progress.log'])
        require(await self.observer().file(self.plan.path("Run-once")) == self.plan.script,
                "Uploaded script bytes differ")
        self.phase = "PREPARED"
        return self.plan.description()

    async def prepare(self):
        async def perform():
            await self._prepare_namespace()
            if self.plan.copy_source:
                await self.copy_candidate()
            else:
                await self.upload("NativeTest", self.plan.binary)
            return await self._prepare_controls()
        return await self._operation("NEW", "PREPARING", perform)

    async def begin_prepare(self):
        """Begin a direct upload split across bounded, separately guarded calls.

        This is a single live transaction, never a failed-run resume. The caller
        must keep the same lease/connection and qualify its outer-call sequence.
        The original whole-transaction deadline continues across every step.
        """
        async def perform():
            require(self.plan.copy_source is None, "Incremental preparation requires direct upload")
            await self._prepare_namespace()
            self._upload_commands = iter(wire.write_commands(self.plan.path("NativeTest"), self.plan.binary))
            self._upload_offset = 0
            self.phase = "UPLOADING"
            return {"phase": self.phase, "nextOffset": 0, "candidateBytes": len(self.plan.binary)}
        return await self._operation("NEW", "PREPARING", perform)

    async def prepare_chunk(self, expected_offset):
        """Send at most four acknowledged frames; stale offsets never replay."""
        async def perform():
            require(type(expected_offset) is int and expected_offset == self._upload_offset,
                    "Exact next upload offset required; replay is forbidden")
            for _ in range(4):
                command = next(self._upload_commands, None)
                if command is None:
                    break
                require(command["offset"] == self._upload_offset, "Upload sequence changed")
                raw = await self.ask(command, "OK")
                size = len(command["hexData"]) // 2
                require(raw == f"OK|WRITEFILE|{size}", "Exact write acknowledgement absent")
                self._upload_offset += size
            self.phase = "UPLOADED" if self._upload_offset == len(self.plan.binary) else "UPLOADING"
            progress = {"phase": self.phase, "nextOffset": self._upload_offset,
                        "candidateBytes": len(self.plan.binary)}
            self.record({"event": "upload_progress", **progress})
            return progress
        return await self._operation("UPLOADING", "SENDING", perform)

    async def finish_prepare(self):
        """Verify all candidate bytes and script before permitting execution."""
        async def perform():
            await self.checksum("NativeTest", self.plan.binary)
            return await self._prepare_controls()
        return await self._operation("UPLOADED", "PREPARING", perform)

    async def execute(self, *, native_seconds=90, diagnostic_progress=False):
        async def perform():
            require(type(native_seconds) in (int, float) and math.isfinite(native_seconds)
                    and 0 < native_seconds <= 90, "Native deadline may not be widened")
            require(type(diagnostic_progress) is bool, "Explicit diagnostic progress flag required")
            require(not diagnostic_progress or self.plan.progress_sidecar,
                    "Live progress requires a separately closed sidecar")
            for name, data in self.plan.prepared_files().items():
                await self.checksum(name, data)
            entries = await self.observer().directory(self.plan.directory)
            require({e.name for e in entries} == set(self.plan.prepared_files())
                    and all(e.kind == "F" for e in entries), "Prepared namespace changed")
            started = self.clock()
            native_deadline = min(self.deadline, started + native_seconds)
            next_progress = started + 20
            ident, command = self.plan.command_id, self.plan.command
            raw = await self.ask({"type": "LAUNCH", "id": ident, "command": command}, "CMD", min(10, native_seconds))
            require(re.fullmatch(re.escape(f"CMD|{ident}|OK|Started") +
                                 r"(?: \(proc [1-9][0-9]*\))?: " + re.escape(command), raw),
                    "Exact launch acknowledgement absent")
            for _ in range(240):
                remaining = native_deadline - self.clock()
                require(remaining > 0, "Native completion deadline expired")
                name = self.plan.path("complete.flag")
                size = await self.completion_size("complete.flag", 9, native_deadline,
                                                  set(self.plan.names))
                if size == 9:
                    flag = await self.observer(native_deadline - self.clock()).file(name, 9)
                    require(self.clock() < native_deadline and flag == b"COMPLETE\n", "Exact timely completion flag absent")
                    break
                if diagnostic_progress and self.clock() >= next_progress:
                    # A separately closed sidecar is diagnostic evidence only.
                    # It cannot establish completion, RC, custody or settlement.
                    # Both reads spend the original native budget; no reads are
                    # attempted after a timeout or another failed exchange.
                    path = self.plan.path("progress.log")
                    raw = await self.ask({"type": "FILEINFO", "path": path}, "FILEINFO",
                                         min(4, native_deadline - self.clock()))
                    require(self.clock() < native_deadline, "Native completion deadline expired")
                    length = wire.file_info(raw, path, capture.MAX_OUTPUT)[0]
                    data, offset = b"", max(0, length - wire.READ_SIZE)
                    if length:
                        count = min(length, wire.READ_SIZE)
                        raw = await self.ask({"type": "READFILE", "path": path,
                                              "offset": offset, "size": count}, "FILE",
                                             min(4, native_deadline - self.clock()))
                        require(self.clock() < native_deadline, "Native completion deadline expired")
                        data = wire.file_chunk(raw, path, offset, count)
                        require(len(data) == count, "Diagnostic suffix shortened during observation")
                    self.record({"event": "native_progress_unconfirmed", "elapsed": self.clock() - started,
                                 "observedLength": length, "offset": offset, "hexData": data.hex()})
                    next_progress = self.clock() + 20
                await self.sleep(min(0.5, max(0, native_deadline - self.clock())))
            else:
                raise ValueError("Completion observation count exceeded")
            files = dict(self.plan.prepared_files(), **{"complete.flag": flag})
            for name in ("control.log", "native.log") + (("progress.log",) if self.plan.progress_sidecar else ()):
                limit = self.plan.output_limit if name == "native.log" else capture.MAX_OUTPUT
                files[name] = await self.observer().file(self.plan.path(name), limit)
            control = files["control.log"].decode("ascii")
            observed = self.plan.observe(control, files["native.log"])
            for name, data in files.items():
                await self.checksum(name, data)
            await self._exact_directory(files)
            self.custody = files
            self.record({"event": "completed", "files": self.manifest(), "observed": observed})
            self.phase = "COMPLETED"
            return {"status": "COMPLETED_CUSTODY_PENDING_SETTLEMENT", "plan": self.plan.description(),
                    "files": self.manifest(), "observed": observed, "launches": 1}
        return await self._operation("PREPARED", "EXECUTING", perform)

    async def _exact_directory(self, files):
        entries = await self.observer().directory(self.plan.directory)
        require({(e.name, e.size, e.kind) for e in entries} == {(n, len(b), "F") for n, b in files.items()},
                "Namespace contains changed, missing or unowned files")

    def manifest(self):
        return {name: {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                for name, data in self.custody.items()}

    async def settle(self, verify_idle_and_custody):
        """Separate normal settlement; callback must verify idle and durable backup.

        The callback's assertion is not independently authenticated by this
        helper. It must be provided by the reviewed caller, never a test result.
        Guard release and independent final absence remain caller operations.
        """
        async def perform():
            require(callable(verify_idle_and_custody), "Independent settlement observer required")
            require(await verify_idle_and_custody(self.plan.description(), self.manifest()) is True,
                    "Independent idle/custody verification absent")
            self.remaining()
            await self._exact_directory(self.custody)
            for name, data in self.custody.items():
                await self.checksum(name, data)
            for name in self.plan.names:
                remote = self.plan.path(name)
                raw = await self.ask({"type": "DELETEFILE", "path": remote}, "OK")
                require(raw == "OK|DELETE|" + remote, "Exact file deletion unconfirmed")
            require(await self.observer().directory(self.plan.directory) == (), "Owned directory is not empty")
            raw = await self.ask({"type": "DELETEFILE", "path": self.plan.directory}, "OK")
            require(raw == "OK|DELETE|" + self.plan.directory, "Exact directory deletion unconfirmed")
            self.phase = "SETTLED"
            return {"status": "EXACT_FILES_DELETED_INDEPENDENT_RELEASE_PENDING", "files": self.manifest()}
        return await self._operation("COMPLETED", "SETTLING", perform)
