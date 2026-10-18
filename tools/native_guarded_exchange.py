"""Raw reply capture on an existing, externally guarded bridge connection.

The owning private service must install the shared launcher hooks before
registering its tool. This helper never opens a connection or writes a socket
directly: conn.send remains the shared guard's checked leaf. admit() is the
caller's additional live identity check, not a substitute for launcher hooks.
on_rx is an observation callback in the pinned upstream SerialConnection.
An optional error_record callback durably records one validated, unambiguous
raw error before generic error conversion. It must finish synchronously and
return None; its failure is terminal. It grants no retry or target authority.
"""
import asyncio
import math

from native_file_protocol import BridgeReplyError, line, require


ALLOWED = {
    "MAKEDIR": "OK", "LISTDIR": "DIR", "WRITEFILE": "OK", "FILEINFO": "FILEINFO",
    "READFILE": "FILE", "CHECKSUM": "CHECKSUM", "LAUNCH": "CMD", "DELETEFILE": "OK",
}


class Exchange:
    def __init__(self, connection, admit, *, error_record=None):
        require(callable(admit), "Caller live-admission check required")
        require(error_record is None or callable(error_record), "Synchronous raw error recorder required")
        self.connection, self.admit = connection, admit
        self.error_record = error_record
        self.busy = False
        self.failed = False

    async def __call__(self, command, reply_type, seconds):
        require(not self.failed and not self.busy, "Raw exchange busy or failed")
        self.busy = True
        prior = callback = None
        installed = False
        try:
            require(type(command) is dict and command.get("type") in ALLOWED
                    and ALLOWED[command["type"]] == reply_type, "Unsupported raw exchange")
            require(type(seconds) in (int, float) and math.isfinite(seconds) and 0 < seconds <= 10,
                    "Finite raw exchange budget required")
            require(self.admit() is True and self.connection.connected is True, "Live admitted connection required")
            prior = self.connection.on_rx
            require(prior is None or callable(prior), "Invalid existing observation callback")
            require(not getattr(prior, "_pt24g_raw_observer", False), "Another raw observer is active")
            queue = asyncio.Queue(maxsize=2)
            overflow = False

            def receive(raw):
                nonlocal overflow
                try:
                    if prior is not None:
                        prior(raw)
                finally:
                    if isinstance(raw, str) and raw.split("|", 1)[0] in (reply_type, "ERR"):
                        if queue.full():
                            overflow = True
                        else:
                            queue.put_nowait(raw)

            callback = receive
            callback._pt24g_raw_observer = True
            self.connection.on_rx = callback
            installed = True
            # Subscribe first: a synchronous/immediate reply must not be lost.
            require(self.admit() is True and self.connection.connected is True, "Admission changed before send")
            self.connection.send(command)
            raw = await asyncio.wait_for(queue.get(), timeout=seconds)
            require(not overflow and queue.empty(), "Ambiguous duplicate bridge response")
            line(raw)
            if raw.startswith("ERR|"):
                # Private evidence only, after unique bounded line validation.
                # Recording refusal is terminal and is never swallowed.
                if self.error_record is not None:
                    require(self.error_record({"event": "exchange_error_reply",
                        "source": "native_guarded_exchange",
                        "commandType": command["type"], "replyType": reply_type,
                        "raw": raw}) is None, "Raw error recorder must finish synchronously")
                raise BridgeReplyError(raw)
            require(self.admit() is True and self.connection.connected is True, "Admission changed before success")
            return raw
        except BaseException:
            self.failed = True
            raise
        finally:
            if installed:
                if self.connection.on_rx is callback:
                    self.connection.on_rx = prior
                else:
                    # Preserve the unexpected replacement rather than clobber
                    # another observer. No target operation can follow.
                    self.failed = True
                    self.busy = False
                    raise RuntimeError("Raw observation callback changed during exchange")
            self.busy = False
