"""Bounded raw file observations for the existing guarded Amiga transport.

This module cannot connect, reserve, launch, write or delete. The caller supplies
an already admitted exchange which subscribes before sending, checks live
ownership/target identity, and returns one complete raw bridge line. Do not feed
the upstream lossy DIR/FILE dictionaries into these validators. Exceptions and
cancellation propagate without retries, cleanup or recovery requests.

Directory observations describe replies, not an atomic filesystem snapshot.
Exclusive ownership and a qualified daemon remain caller requirements. In
particular the historical daemon does not distinguish ExNext I/O failure from
end-of-directory; matching observations alone cannot repair that limitation.
"""
import asyncio
from dataclasses import dataclass
import math
import re
import time


MAX_LINE = 8192
MAX_ENTRIES = 4096
MAX_PAGES = 64
MAX_BYTES = 65536
READ_SIZE = 256
MAX_READS = 512


def require(ok, message):
    if not ok:
        raise ValueError(message)


def path(value):
    require(isinstance(value, str) and 0 < len(value) <= 192
            and re.fullmatch(r"[A-Za-z][A-Za-z0-9_-]*:(?:[A-Za-z0-9_.-]+/?)*", value),
            "Exact ordinary Amiga pathname required")
    require(all(x not in (".", "..") for x in value.split(":")[1].split("/")),
            "Parent/current-directory aliases are not supported")
    return value


def line(value):
    require(isinstance(value, str) and 0 < len(value) <= MAX_LINE
            and value.isascii() and value.isprintable(), "Incomplete or invalid bridge line")
    return value


def decimal(value):
    require(isinstance(value, str) and re.fullmatch(r"0|[1-9][0-9]{0,9}", value),
            "Noncanonical unsigned bridge field")
    number = int(value)
    require(number <= 0xffffffff, "Bridge field exceeds ULONG")
    return number


@dataclass(frozen=True)
class Entry:
    name: str
    size: int
    kind: str


def directory_page(raw, requested, offset):
    """Require the true total and every entry; reject ambiguous filename syntax."""
    path(requested)
    require(type(offset) is int and 0 <= offset <= MAX_ENTRIES, "Invalid directory offset")
    parts = line(raw).split("|", 3)
    require(len(parts) == 4 and parts[:2] == ["DIR", requested], "Directory reply path/type differs")
    total = decimal(parts[2])
    require(offset <= total <= MAX_ENTRIES, "Directory count exceeds bound or regressed")
    entries = []
    position = 0
    body = parts[3]
    while position < len(body):
        match = re.match(r"([^|(),:/]+)\((0|[1-9][0-9]{0,9}),([FD])\)(,|$)", body[position:])
        require(match is not None, "Ambiguous or malformed directory entry")
        name = match[1]
        require(name == name.strip() and name not in (".", "..") and 0 < len(name) <= 107,
                "Unsupported directory name")
        entries.append(Entry(name, decimal(match[2]), match[3]))
        position += match.end()
        require(not match[4] or position < len(body), "Trailing directory delimiter")
    require(len(entries) <= total - offset and (entries or offset == total),
            "Partial directory page made no progress or exceeded total")
    require(len({e.name.lower() for e in entries}) == len(entries), "Duplicate directory name")
    return total, tuple(entries)


def file_chunk(raw, requested, offset, count):
    """Wire order is FILE|path|offset|length|hex, regardless of upstream labels."""
    path(requested)
    require(type(offset) is int and 0 <= offset <= MAX_BYTES and type(count) is int
            and 1 <= count <= READ_SIZE, "Invalid bounded file request")
    parts = line(raw).split("|")
    require(len(parts) == 5 and parts[:2] == ["FILE", requested], "File reply path/type differs")
    require(decimal(parts[2]) == offset, "File reply offset differs")
    size = decimal(parts[3])
    require(size <= count and len(parts[4]) == 2 * size
            and re.fullmatch(r"[0-9A-Fa-f]*", parts[4]) is not None,
            "File reply length or encoding differs")
    return bytes.fromhex(parts[4])


def file_info(raw, requested, limit):
    path(requested)
    parts = line(raw).split("|")
    require(len(parts) == 6 and parts[:2] == ["FILEINFO", requested]
            and parts[3] == "FILE" and re.fullmatch(r"[0-9A-Fa-f]{8}", parts[4]),
            "Exact ordinary file metadata required")
    size = decimal(parts[2])
    require(size <= limit, "File exceeds exact observation bound")
    return size, parts[4].lower(), parts[5]


class Observation:
    """One finite observation group. A failed request permanently stops the group.

    exchange(command, reply_type, seconds) returns a raw line without CR/LF.
    It must not silently accept unrelated replies or replay a request. Each
    group is single-consumer; concurrent calls are rejected before dispatch.
    """
    def __init__(self, exchange, seconds=20, *, clock=time.monotonic):
        require(callable(exchange) and callable(clock) and type(seconds) in (int, float)
                and math.isfinite(seconds) and 0 < seconds <= 60, "Invalid observation budget")
        self.exchange = exchange
        self.clock = clock
        self.deadline = clock() + seconds
        self.failed = False
        self.busy = False

    async def _request(self, command, reply_type):
        require(not self.failed, "Observation group already failed")
        remaining = self.deadline - self.clock()
        require(remaining > 0, "Whole observation deadline expired")
        result = await asyncio.wait_for(self.exchange(command, reply_type, min(4, remaining)),
                                        timeout=min(4, remaining))
        require(self.clock() < self.deadline, "Reply exceeded whole observation deadline")
        return result

    async def _run(self, operation):
        require(not self.busy and not self.failed, "Observation group busy or failed")
        self.busy = True
        try:
            return await operation()
        except BaseException:
            self.failed = True
            raise
        finally:
            self.busy = False

    async def directory(self, requested):
        async def collect():
            path(requested)
            entries, names, total = [], set(), None
            for _ in range(MAX_PAGES):
                raw = await self._request({"type": "LISTDIR", "path": requested,
                                           "offset": len(entries)}, "DIR")
                count, page = directory_page(raw, requested, len(entries))
                require(total is None or count == total, "Directory changed during pagination")
                total = count
                for item in page:
                    require(item.name.lower() not in names, "Directory page repeated a name")
                    names.add(item.name.lower())
                    entries.append(item)
                if len(entries) == total:
                    return tuple(entries)
            raise ValueError("Directory page bound reached")
        return await self._run(collect)

    async def file(self, requested, limit=8192):
        async def collect():
            path(requested)
            require(type(limit) is int and 0 <= limit <= MAX_BYTES, "Invalid file size bound")
            before = file_info(await self._request({"type": "FILEINFO", "path": requested}, "FILEINFO"),
                               requested, limit)
            size = before[0]
            data = bytearray()
            for _ in range(MAX_READS):
                count = min(READ_SIZE, size - len(data)) if len(data) < size else 1
                raw = await self._request({"type": "READFILE", "path": requested,
                                           "offset": len(data), "size": count}, "FILE")
                chunk = file_chunk(raw, requested, len(data), count)
                require(len(data) + len(chunk) <= size, "File grew during observation")
                if not chunk:
                    require(len(data) == size, "Premature EOF or failed seek; partial file refused")
                    after = file_info(await self._request({"type": "FILEINFO", "path": requested}, "FILEINFO"),
                                      requested, limit)
                    require(after == before, "File metadata changed during observation")
                    return bytes(data)
                data.extend(chunk)
                # A short reply may reflect the daemon's wire-size cap, not EOF.
                # Continue until explicit zero bytes; never return a partial file.
            raise ValueError("File read bound reached")
        return await self._run(collect)


def absent(entries, names):
    """Check exact case-insensitive names in a previously complete observation."""
    require(isinstance(entries, tuple) and all(isinstance(e, Entry) for e in entries),
            "Validated complete directory observation required")
    require(isinstance(names, tuple) and names and all(isinstance(n, str) and
            re.fullmatch(r"[A-Za-z0-9_.-]+", n) for n in names), "Exact names required")
    require(not {n.lower() for n in names} & {e.name.lower() for e in entries},
            "Requested name already exists or remains")


@dataclass(frozen=True)
class ScriptPlan:
    command_id: int
    script: str

    def __post_init__(self):
        require(type(self.command_id) is int and 1 <= self.command_id <= 0xffffffff,
                "One explicit ULONG command identity required")
        require(isinstance(self.script, str) and 0 < len(self.script) <= 4096
                and all(c == "\n" or 32 <= ord(c) <= 126 for c in self.script)
                and not any(x in self.script for x in (";", "|", "###ABDONE###")),
                "Reviewed bounded ASCII script without wire delimiters required")

    @property
    def names(self):
        return (f"ab_script_{self.command_id}", f"ab_sout_{self.command_id}")

    @property
    def paths(self):
        return tuple("T:" + n for n in self.names)

    @property
    def body(self):
        script = self.script if self.script.endswith("\n") else self.script + "\n"
        return ("FailAt 255\n" + script + "Echo ###ABDONE###\n").encode("ascii")

    def command(self):
        return {"type": "SCRIPT", "id": self.command_id, "script": self.script.replace("\n", ";")}

    def acknowledge(self, raw):
        require(line(raw) == f"CMD|{self.command_id}|OK|ASYNC|{self.paths[1]}",
                "Exact asynchronous script acknowledgment required")


def completed_capture(data):
    """Completion is a single terminal sentinel line, never a substring match."""
    require(isinstance(data, bytes) and len(data) <= MAX_BYTES, "Capture exceeds bound")
    sentinel = b"###ABDONE###"
    if sentinel not in data:
        return None
    if data.count(sentinel) == 1 and data.endswith(sentinel) and (
            data == sentinel or data[-len(sentinel)-1:-len(sentinel)] == b"\n"):
        return None  # The echo's final newline has not been observed yet.
    require(data.count(sentinel) == 1 and data.endswith(sentinel + b"\n")
            and (data == sentinel + b"\n" or data[-len(sentinel)-2:-len(sentinel)-1] == b"\n"),
            "Ambiguous or incomplete completion sentinel")
    return data[:-len(sentinel)-1]
