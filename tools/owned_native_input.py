"""Bounded Dev-to-RAM input preparation for a separately owned native run.

This opt-in helper opens no transport and launches no native executable. It
creates a fresh input directory, copies the exact caller-owned Dev file once,
and retains custody until the caller positively proves that all readers and
the execution transaction have finished. Its deadline is additional accounting,
not permission to reset or extend the caller's overall guest deadline.

The caller supplies live shared guards, durable per-exchange records, exact Dev
source ownership and independent settlement. Failures retain ownership. This
does not implement MiniFTP/archive transfer or grant ownership of existing RAM.
"""
import asyncio
from dataclasses import dataclass
import hashlib
import math
import re
import time
import zlib

import native_file_protocol as wire
from owned_native_run import Plan, require


@dataclass(frozen=True)
class InputPlan:
    execution: Plan
    source: str
    command_id: int

    def __post_init__(self):
        require(type(self.execution) is Plan, "Exact execution plan required")
        require(self.execution.copy_source ==
                "RAM:PT24G-" + self.execution.run_id[:16] + "-in/NativeTest",
                "Execution must select its exact separate RAM input")
        require(isinstance(self.source, str) and re.fullmatch(
                re.escape("Dev:Tests/PT24G-" + self.execution.run_id[:16] + "/")
                + r"[A-Za-z0-9_-]+", self.source), "Exact run-owned Dev input required")
        wire.path(self.source)
        require(type(self.command_id) is int and 1 <= self.command_id <= 0xffffffff
                and self.command_id not in (self.execution.command_id, self.execution.command_id + 1),
                "Distinct positive input command identity required")

    @property
    def directory(self):
        return self.execution.copy_source.rsplit("/", 1)[0]

    @property
    def names(self):
        return ("NativeTest", "Prepare-input", "ready.flag", "prepare.log")

    def path(self, name):
        require(name in self.names, "Not a declared input file")
        return self.directory + "/" + name

    @property
    def script(self):
        failed = f"If WARN\n Echo FAILED >{self.path('ready.flag')}\n Quit 20\nEndIf\n"
        return (f"FailAt 255\nC:Copy {self.source} {self.path('NativeTest')} QUIET\n" + failed
                + f"C:Protect {self.path('NativeTest')} +rwed\n" + failed
                + f"Echo {self.execution.prefix}INPUT-DONE >{self.path('prepare.log')}\n"
                + f"Echo READY >{self.path('ready.flag')}\n").encode("ascii")

    @property
    def command(self):
        return "C:Execute " + self.path("Prepare-input")

    def files(self):
        return {"NativeTest": self.execution.binary, "Prepare-input": self.script,
                "ready.flag": b"READY\n",
                "prepare.log": (self.execution.prefix + "INPUT-DONE\n").encode("ascii")}

    def description(self):
        return {"runID": self.execution.run_id, "commandID": self.command_id,
                "source": self.source, "directory": self.directory,
                "copySource": self.execution.copy_source, "command": self.command,
                "files": list(self.names),
                "candidateBytes": len(self.execution.binary),
                "candidateSHA256": hashlib.sha256(self.execution.binary).hexdigest(),
                "scriptSHA256": hashlib.sha256(self.script).hexdigest()}


class InputStage:
    def __init__(self, plan, exchange, record, *, seconds=250,
                 clock=time.monotonic, sleep=asyncio.sleep):
        require(type(plan) is InputPlan and callable(exchange) and callable(record)
                and callable(clock) and callable(sleep) and type(seconds) in (int, float)
                and math.isfinite(seconds) and 0 < seconds <= 250, "Bounded input stage required")
        self.plan, self.exchange, self.record = plan, exchange, record
        self.clock, self.sleep = clock, sleep
        started = clock()
        self.deadline = started + seconds
        self.prepare_deadline = min(self.deadline, started + 60)
        self.phase, self.calls = "NEW", 0

    def remaining(self):
        deadline = min(self.deadline, self.prepare_deadline) if self.phase == "PREPARING" else self.deadline
        value = deadline - self.clock()
        require(value > 0, "Input stage deadline expired")
        return value

    async def _ask(self, command, reply_type, seconds=4):
        require(self.phase in ("PREPARING", "SETTLING"), "Input stage is not active")
        require(self.calls < 256, "Input exchange count bound reached")
        timeout = min(seconds, self.remaining())
        self.calls += 1
        intent = {key: value for key, value in command.items() if key != "hexData"}
        if "hexData" in command:
            body = bytes.fromhex(command["hexData"])
            intent.update(bytes=len(body), sha256=hashlib.sha256(body).hexdigest())
        self.record({"event": "input_dispatch", "number": self.calls, "command": intent})
        raw = await asyncio.wait_for(self.exchange(command, reply_type, timeout), timeout)
        self.remaining()
        wire.line(raw)
        self.record({"event": "input_reply", "number": self.calls, "raw": raw})
        return raw

    def _observer(self, seconds=60):
        return wire.Observation(self._ask, seconds=min(seconds, self.remaining()), clock=self.clock)

    async def _checksum(self, path, data):
        raw = await self._ask({"type": "CHECKSUM", "path": path}, "CHECKSUM")
        match = re.fullmatch(re.escape("CHECKSUM|" + path + "|") +
                             r"([0-9a-fA-F]{8})\|(0|[1-9][0-9]*)", raw)
        require(match and int(match[1], 16) == zlib.crc32(data) and int(match[2]) == len(data),
                "Input checksum/size differs from frozen file")

    async def _upload(self, name, data):
        for command in wire.write_commands(self.plan.path(name), data):
            raw = await self._ask(command, "OK")
            require(raw == f"OK|WRITEFILE|{len(command['hexData']) // 2}",
                    "Input write acknowledgement absent")
        await self._checksum(self.plan.path(name), data)

    async def _custody(self):
        files = self.plan.files()
        entries = await self._observer().directory(self.plan.directory)
        require({(entry.name, entry.size, entry.kind) for entry in entries} ==
                {(name, len(data), "F") for name, data in files.items()},
                "Input namespace contains changed, missing or unowned files")
        for name, data in files.items():
            await self._checksum(self.plan.path(name), data)
        await self._checksum(self.plan.source, self.plan.execution.binary)

    async def prepare(self):
        require(self.phase == "NEW", "Input stage cannot be replayed")
        self.phase = "PREPARING"
        try:
            self.record({"event": "input_plan", "plan": self.plan.description()})
            # Check the caller's source before any mutation; its checksum is not
            # an ownership grant. The caller must keep its Dev copy pinned.
            await self._checksum(self.plan.source, self.plan.execution.binary)
            raw = await self._ask({"type": "MAKEDIR", "path": self.plan.directory}, "OK")
            require(raw == "OK|MAKEDIR|" + self.plan.directory,
                    "Fresh input directory creation unconfirmed")
            require(await self._observer().directory(self.plan.directory) == (),
                    "Fresh input directory is not empty")
            await self._upload("Prepare-input", self.plan.script)
            await self._upload("ready.flag", b"")
            require(await self._observer().file(self.plan.path("Prepare-input")) == self.plan.script,
                    "Input preparation script differs")
            deadline = min(self.deadline, self.clock() + 30)
            ident, command = self.plan.command_id, self.plan.command
            raw = await self._ask({"type": "LAUNCH", "id": ident, "command": command}, "CMD", 10)
            require(re.fullmatch(re.escape(f"CMD|{ident}|OK|Started") +
                                 r"(?: \(proc [1-9][0-9]*\))?: " + re.escape(command), raw),
                    "Exact input-copy launch acknowledgement absent")
            for _ in range(80):
                remaining = deadline - self.clock()
                require(remaining > 0, "Input-copy completion deadline expired")
                path = self.plan.path("ready.flag")
                raw = await self._ask({"type": "FILEINFO", "path": path}, "FILEINFO", min(4, remaining))
                require(self.clock() < deadline, "Input-copy completion deadline expired")
                size = wire.file_info(raw, path, 7)[0]
                require(size in (0, 6), "Input-copy success flag absent")
                if size:
                    flag = await self._observer(deadline - self.clock()).file(path, 6)
                    require(self.clock() < deadline and flag == b"READY\n", "Timely input success absent")
                    break
                await self.sleep(min(.5, max(0, deadline - self.clock())))
            else:
                raise ValueError("Input-copy observation count exceeded")
            require(await self._observer().file(self.plan.path("prepare.log")) ==
                    self.plan.files()["prepare.log"], "Input-copy output differs")
            await self._custody()
            result = {"status": "INPUT_READY_CUSTODY_RETAINED", "plan": self.plan.description(),
                      "files": self.manifest()}
            self.record({"event": "input_ready", **result})
            self.phase = "READY"
            return result
        except BaseException:
            self.phase = "FAILED"
            raise

    def manifest(self):
        return {name: {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                for name, data in self.plan.files().items()}

    async def settle(self, verify_execution_idle_and_custody):
        """The reviewed caller proves execution settled, no reader, saved custody.

        The callback is a caller obligation, not authentication supplied by this
        helper. Input READY, native RC0 or a matching filename is insufficient.
        The original stage deadline still applies, including time spent waiting
        for readers. An expired or uncertain stage retains ownership for recovery.
        """
        require(self.phase == "READY", "Only a prepared live input can settle")
        self.phase = "SETTLING"
        try:
            self.remaining()
            require(callable(verify_execution_idle_and_custody), "Independent input settlement observer required")
            verified = await asyncio.wait_for(
                verify_execution_idle_and_custody(self.plan.description(), self.manifest()), self.remaining())
            require(verified is True,
                    "Independent execution-idle/input-custody verification absent")
            self.remaining()
            await self._custody()
            for name in self.plan.names:
                remote = self.plan.path(name)
                raw = await self._ask({"type": "DELETEFILE", "path": remote}, "OK")
                require(raw == "OK|DELETE|" + remote, "Input file deletion unconfirmed")
            require(await self._observer().directory(self.plan.directory) == (),
                    "Input directory not empty after exact cleanup")
            raw = await self._ask({"type": "DELETEFILE", "path": self.plan.directory}, "OK")
            require(raw == "OK|DELETE|" + self.plan.directory, "Input directory deletion unconfirmed")
            self.record({"event": "input_settled", "files": self.manifest()})
            self.phase = "SETTLED"
            return {"status": "INPUT_DELETED_INDEPENDENT_RELEASE_PENDING", "files": self.manifest()}
        except BaseException:
            self.phase = "FAILED"
            raise
