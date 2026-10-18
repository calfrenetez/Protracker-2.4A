import asyncio
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from native_guarded_exchange import Exchange
from native_file_protocol import BridgeReplyError, MAX_LINE


class Connection:
    def __init__(self, replies):
        self.connected = True
        self.received, self.sent = [], []
        self.on_rx = self.received.append
        self.original = self.on_rx
        self.replies = replies

    def send(self, command):
        self.sent.append(command)
        for raw in self.replies:
            self.on_rx(raw)


class Tests(unittest.IsolatedAsyncioTestCase):
    command = {"type": "LISTDIR", "path": "RAM:", "offset": 0}

    async def test_immediate_reply_and_existing_callback_preserved(self):
        connection = Connection(["HB|1|2|3", "DIR|RAM:|0|"])
        exchange = Exchange(connection, lambda: True)
        self.assertEqual(await exchange(self.command, "DIR", 1), "DIR|RAM:|0|")
        self.assertEqual(connection.received, connection.replies)
        self.assertIs(connection.on_rx, connection.original)
        self.assertEqual(connection.sent, [self.command])

    async def test_errors_duplicates_and_malformed_reply_stop(self):
        for replies in (["ERR|LISTDIR failed|RAM:"], ["DIR|RAM:|0|"] * 2, ["DIR|RAM:|0|"] * 3,
                        ["DIR|RAM:|0|\n"], ["DIR|RAM:|0|", "ERR|late|RAM:"]):
            with self.subTest(replies=replies):
                connection = Connection(replies)
                exchange = Exchange(connection, lambda: True)
                with self.assertRaises(ValueError):
                    await exchange(self.command, "DIR", 1)
                with self.assertRaises(ValueError):
                    await exchange(self.command, "DIR", 1)
                self.assertEqual(connection.sent, [self.command])
                self.assertIs(connection.on_rx, connection.original)

    async def test_error_reply_is_bounded_evidence_and_not_a_success(self):
        for raw in ("ERR|FILEINFO failed|RAM:fixture/complete.flag",
                    "ERR|" + "x" * (MAX_LINE - 4)):
            connection = Connection([raw])
            exchange = Exchange(connection, lambda: True)
            with self.assertRaises(BridgeReplyError) as failure:
                await exchange(self.command, "DIR", 1)
            self.assertEqual(failure.exception.raw, raw)
            self.assertNotIn(raw, str(failure.exception))
            self.assertTrue(exchange.failed)
            self.assertFalse(exchange.busy)
            self.assertIs(connection.on_rx, connection.original)
            with self.assertRaises(ValueError):
                await exchange(self.command, "DIR", 1)
            self.assertEqual(connection.sent, [self.command])

    async def test_ambiguous_or_invalid_error_is_not_promoted_to_exact_evidence(self):
        for replies in (["ERR|a", "ERR|b"], ["ERR|x", "DIR|RAM:|0|"],
                        ["ERR|bad\nline"], ["ERR|\u00e9"], ["ERR|" + "x" * MAX_LINE]):
            connection = Connection(replies)
            exchange = Exchange(connection, lambda: True)
            with self.assertRaises(ValueError) as failure:
                await exchange(self.command, "DIR", 1)
            self.assertNotIsInstance(failure.exception, BridgeReplyError)
            self.assertTrue(exchange.failed)
            self.assertIs(connection.on_rx, connection.original)
            self.assertEqual(connection.sent, [self.command])

    async def test_missing_reply_times_out_without_retry(self):
        connection = Connection([])
        exchange = Exchange(connection, lambda: True)
        with self.assertRaises(TimeoutError):
            await exchange(self.command, "DIR", .01)
        self.assertEqual(connection.sent, [self.command])
        self.assertIs(connection.on_rx, connection.original)
        self.assertTrue(exchange.failed)

    async def test_refused_admission_never_sends(self):
        for when in (1, 2):
            connection = Connection([])
            checks = [0]
            def admit():
                checks[0] += 1
                return checks[0] != when
            exchange = Exchange(connection, admit)
            with self.assertRaises(ValueError):
                await exchange(self.command, "DIR", 1)
            self.assertEqual(connection.sent, [])
            self.assertIs(connection.on_rx, connection.original)

    async def test_disconnected_or_retargeted_before_success(self):
        connection = Connection(["DIR|RAM:|0|"])
        checks = [0]
        def admit():
            checks[0] += 1
            return checks[0] < 3
        exchange = Exchange(connection, admit)
        with self.assertRaises(ValueError):
            await exchange(self.command, "DIR", 1)
        self.assertTrue(exchange.failed)
        self.assertIs(connection.on_rx, connection.original)

    async def test_guard_send_refusal_keeps_original_error(self):
        connection = Connection([])
        def refused(command):
            raise PermissionError("guard refused")
        connection.send = refused
        exchange = Exchange(connection, lambda: True)
        with self.assertRaises(PermissionError):
            await exchange(self.command, "DIR", 1)
        self.assertEqual(connection.sent, [])
        self.assertIs(connection.on_rx, connection.original)

    async def test_cancellation_restores_observation_without_target_cleanup(self):
        connection = Connection([])
        exchange = Exchange(connection, lambda: True)
        task = asyncio.create_task(exchange(self.command, "DIR", 10))
        while not connection.sent:
            await asyncio.sleep(0)
        task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertIs(connection.on_rx, connection.original)
        self.assertEqual(len(connection.sent), 1)

    async def test_unexpected_callback_change_is_preserved_and_fails(self):
        connection = Connection([])
        replacement = lambda raw: None
        def send(command):
            connection.sent.append(command)
            callback = connection.on_rx
            connection.on_rx = replacement
            callback("DIR|RAM:|0|")
        connection.send = send
        exchange = Exchange(connection, lambda: True)
        with self.assertRaisesRegex(RuntimeError, "callback changed"):
            await exchange(self.command, "DIR", 1)
        self.assertIs(connection.on_rx, replacement)
        self.assertTrue(exchange.failed)

    async def test_script_and_unbounded_requests_are_not_supported(self):
        for command, kind, seconds in (({"type": "SCRIPT"}, "CMD", 1), (self.command, "FILE", 1),
                                        (self.command, "DIR", 11), (self.command, "DIR", float("inf"))):
            connection = Connection([])
            exchange = Exchange(connection, lambda: True)
            with self.assertRaises(ValueError):
                await exchange(command, kind, seconds)
            self.assertEqual(connection.sent, [])

    async def test_second_observer_cannot_borrow_active_connection(self):
        connection = Connection([])
        first, second = Exchange(connection, lambda: True), Exchange(connection, lambda: True)
        task = asyncio.create_task(first(self.command, "DIR", 10))
        while not connection.sent:
            await asyncio.sleep(0)
        with self.assertRaisesRegex(ValueError, "Another raw observer"):
            await second(self.command, "DIR", 1)
        self.assertEqual(len(connection.sent), 1)
        connection.on_rx("DIR|RAM:|0|")
        self.assertEqual(await task, "DIR|RAM:|0|")
        self.assertIs(connection.on_rx, connection.original)


    async def test_error_recorder_runs_once_before_error_propagation(self):
        for raw in ("ERR|LAUNCH failed|command", "ERR|" + "x" * (MAX_LINE - 4)):
            with self.subTest(bytes=len(raw)):
                connection = Connection([raw])
                recorded = []
                def record(event):
                    self.assertTrue(exchange.busy)
                    self.assertFalse(exchange.failed)
                    recorded.append(dict(event))
                exchange = Exchange(connection, lambda: True, error_record=record)
                with self.assertRaises(BridgeReplyError) as failure:
                    await exchange(self.command, "DIR", 1)
                self.assertEqual(recorded, [{"event": "exchange_error_reply",
                    "source": "native_guarded_exchange", "commandType": "LISTDIR",
                    "replyType": "DIR", "raw": raw}])
                self.assertEqual(failure.exception.raw, raw)
                self.assertTrue(exchange.failed)
                self.assertIs(connection.on_rx, connection.original)
                with self.assertRaises(ValueError):
                    await exchange(self.command, "DIR", 1)
                self.assertEqual(connection.sent, [self.command])
                self.assertEqual(len(recorded), 1)

    async def test_error_recorder_failure_propagates_without_followup(self):
        connection = Connection(["ERR|LAUNCH failed"])
        observed = []
        def record(event):
            observed.append(event)
            raise OSError("private journal refused")
        exchange = Exchange(connection, lambda: True, error_record=record)
        with self.assertRaisesRegex(OSError, "private journal refused"):
            await exchange(self.command, "DIR", 1)
        self.assertEqual(len(observed), 1)
        self.assertTrue(exchange.failed)
        self.assertFalse(exchange.busy)
        self.assertIs(connection.on_rx, connection.original)
        with self.assertRaises(ValueError):
            await exchange(self.command, "DIR", 1)
        self.assertEqual(connection.sent, [self.command])

    async def test_error_recorder_is_not_called_for_success(self):
        connection = Connection(["DIR|RAM:|0|"])
        recorded = []
        exchange = Exchange(connection, lambda: True, error_record=recorded.append)
        self.assertEqual(await exchange(self.command, "DIR", 1), "DIR|RAM:|0|")
        self.assertEqual(recorded, [])
        self.assertFalse(exchange.failed)
        self.assertIs(connection.on_rx, connection.original)

    async def test_invalid_or_ambiguous_error_never_calls_recorder(self):
        for replies in (["ERR|bad\nline"], ["ERR|\u00e9"],
                        ["ERR|" + "x" * MAX_LINE], ["ERR|a", "ERR|b"],
                        ["ERR|a", "DIR|RAM:|0|"]):
            with self.subTest(replies=replies):
                connection = Connection(replies)
                recorded = []
                exchange = Exchange(connection, lambda: True, error_record=recorded.append)
                with self.assertRaises(ValueError):
                    await exchange(self.command, "DIR", 1)
                self.assertEqual(recorded, [])
                self.assertTrue(exchange.failed)
                self.assertIs(connection.on_rx, connection.original)
                with self.assertRaises(ValueError):
                    await exchange(self.command, "DIR", 1)
                self.assertEqual(connection.sent, [self.command])

    async def test_error_recorder_requires_callable_and_synchronous_completion(self):
        connection = Connection(["ERR|failed"])
        with self.assertRaises(ValueError):
            Exchange(connection, lambda: True, error_record=object())
        self.assertEqual(connection.sent, [])
        exchange = Exchange(connection, lambda: True, error_record=lambda event: 1)
        with self.assertRaisesRegex(ValueError, "finish synchronously"):
            await exchange(self.command, "DIR", 1)
        self.assertTrue(exchange.failed)
        self.assertIs(connection.on_rx, connection.original)

    async def test_error_recorder_cannot_replace_original_observer(self):
        connection = Connection(["ERR|failed"])
        replacement = lambda raw: None
        recorded = []
        def record(event):
            recorded.append(event)
            connection.on_rx = replacement
        exchange = Exchange(connection, lambda: True, error_record=record)
        with self.assertRaisesRegex(RuntimeError, "callback changed"):
            await exchange(self.command, "DIR", 1)
        self.assertEqual(len(recorded), 1)
        self.assertIs(connection.on_rx, replacement)
        self.assertTrue(exchange.failed)
        with self.assertRaises(ValueError):
            await exchange(self.command, "DIR", 1)
        self.assertEqual(connection.sent, [self.command])


if __name__ == "__main__":
    unittest.main()
