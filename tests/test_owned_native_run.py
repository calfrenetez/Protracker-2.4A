"""Fault-injected host model for the prepared RAM-only native launch route."""
import asyncio
from pathlib import Path
import sys
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from owned_native_run import Plan, Transaction


class Bridge:
    def __init__(self, plan):
        self.plan = plan
        self.files = {"RAM:original": b"preserve", "T:ab_script_1": b"old script"}
        self.directories = {"RAM:", "T:"}
        self.calls = []
        self.fail_at = None
        self.native_rc = 0
        self.launches = 0
        self.complete = True
        self.change_reply = lambda command, reply: reply

    async def exchange(self, command, kind, seconds):
        assert 0 < seconds <= 10
        self.calls.append(dict(command))
        if len(self.calls) == self.fail_at:
            raise TimeoutError("injected transport failure")
        typ, path = command["type"], command.get("path")
        if typ == "MAKEDIR":
            if path in self.directories or path in self.files:
                reply = "ERR|MAKEDIR failed|" + path
            else:
                self.directories.add(path)
                reply = "OK|MAKEDIR|" + path
        elif typ == "LISTDIR":
            prefix = path if path.endswith(":") else path + "/"
            entries = sorted((n[len(prefix):], len(data)) for n, data in self.files.items()
                             if n.startswith(prefix) and "/" not in n[len(prefix):])
            body = ",".join(f"{n}({size},F)" for n, size in entries[command["offset"]:])
            reply = f"DIR|{path}|{len(entries)}|{body}"
        elif typ == "WRITEFILE":
            assert path.rsplit("/", 1)[0] in self.directories
            data = bytes.fromhex(command["hexData"])
            offset = command["offset"]
            previous = self.files.get(path, b"") if offset else b""
            assert len(previous) == offset
            self.files[path] = previous + data
            reply = f"OK|WRITEFILE|{len(data)}"
        elif typ == "CHECKSUM":
            data = self.files[path]
            reply = f"CHECKSUM|{path}|{zlib.crc32(data):08x}|{len(data)}"
        elif typ == "FILEINFO":
            reply = f"FILEINFO|{path}|{len(self.files[path])}|FILE|00000000|"
        elif typ == "READFILE":
            data = self.files[path][command["offset"]:command["offset"] + command["size"]]
            reply = f"FILE|{path}|{command['offset']}|{len(data)}|{data.hex()}"
        elif typ == "LAUNCH":
            assert command == {"type": "LAUNCH", "id": self.plan.command_id, "command": self.plan.command}
            self.launches += 1
            prefix = self.plan.prefix
            lines = [prefix + "BEFORE", prefix + "STACK-QUERY-BEGIN", f"Current stack size is {self.plan.stack} bytes",
                     prefix + "STACK-QUERY-END", prefix + "NATIVE-BEGIN", prefix + f"NATIVE-RC {self.native_rc}",
                     prefix + "NATIVE-END"]
            self.files[self.plan.path("control.log")] = ("\n".join(lines) + "\n").encode()
            self.files[self.plan.path("native.log")] = self.plan.expected_output
            if self.complete:
                self.files[self.plan.path("complete.flag")] = b"COMPLETE\n"
            reply = f"CMD|{self.plan.command_id}|OK|Started (proc 1): {self.plan.command}"
        elif typ == "DELETEFILE":
            if path in self.files:
                del self.files[path]
            else:
                assert not any(n.startswith(path + "/") for n in self.files)
                self.directories.remove(path)
            reply = "OK|DELETE|" + path
        else:
            raise AssertionError("Unexpected operation: " + typ)
        return self.change_reply(command, reply)


class TransactionTests(unittest.IsolatedAsyncioTestCase):
    def new(self, **kwargs):
        self.plan = Plan("a1" * 16, 123456, bytes(range(256)) * 36, ("native first PASS", "native final PASS"))
        self.bridge = Bridge(self.plan)
        self.events = []
        return Transaction(self.plan, self.bridge.exchange, self.events.append, **kwargs)

    async def idle(self, plan, manifest):
        self.assertEqual(plan, self.plan.description())
        self.assertEqual(set(manifest), set(self.plan.names))
        self.idle_calls += 1
        return True

    async def complete(self, transaction):
        await transaction.prepare()
        result = await transaction.execute()
        self.assertEqual(result["launches"], 1)
        self.assertEqual(transaction.phase, "COMPLETED")
        return result

    async def test_prepare_launch_complete_and_separate_settlement(self):
        transaction = self.new()
        self.idle_calls = 0
        result = await self.complete(transaction)
        self.assertEqual(result["observed"]["returnCode"], 0)
        self.assertEqual(self.bridge.launches, 1)
        self.assertFalse(any(c["type"] in ("DELETEFILE", "SCRIPT") for c in self.bridge.calls))
        self.assertEqual(self.events[0]["event"], "plan")
        self.assertLessEqual(len(self.plan.directory.split(":")[1]), 30)
        self.assertLess(len(self.plan.command), 128)
        await transaction.settle(self.idle)
        self.assertEqual(transaction.phase, "SETTLED")
        self.assertEqual(self.idle_calls, 1)
        self.assertEqual(self.bridge.files, {"RAM:original": b"preserve", "T:ab_script_1": b"old script"})
        self.assertFalse(any(c["type"] == "SCRIPT" or c.get("path", "").startswith("T:") for c in self.bridge.calls))

    async def test_existing_directory_never_adopted(self):
        transaction = self.new()
        self.bridge.directories.add(self.plan.directory)
        self.bridge.files[self.plan.path("NativeTest")] = b"not ours"
        before = dict(self.bridge.files)
        with self.assertRaises(ValueError):
            await transaction.prepare()
        self.assertEqual(len(self.bridge.calls), 1)
        self.assertEqual(self.bridge.files, before)
        self.assertEqual(transaction.phase, "FAILED")

    async def test_timeout_at_every_exchange_stops_without_followup(self):
        transaction = self.new()
        self.idle_calls = 0
        await self.complete(transaction)
        await transaction.settle(self.idle)
        count = len(self.bridge.calls)
        self.assertGreater(count, 30)
        for failure in range(1, count + 1):
            with self.subTest(failure=failure):
                transaction = self.new()
                self.bridge.fail_at = failure
                with self.assertRaises(TimeoutError):
                    await transaction.prepare()
                    await transaction.execute()
                    await transaction.settle(self.idle)
                self.assertEqual(transaction.phase, "FAILED")
                self.assertEqual(len(self.bridge.calls), failure)
                for action in (transaction.prepare, transaction.execute, lambda: transaction.settle(self.idle)):
                    with self.assertRaises(ValueError):
                        await action()
                self.assertEqual(len(self.bridge.calls), failure)
                self.assertEqual(self.bridge.files["RAM:original"], b"preserve")
                self.assertEqual(self.bridge.files["T:ab_script_1"], b"old script")

    async def test_custody_declined_prevents_cleanup(self):
        transaction = self.new()
        await self.complete(transaction)
        count = len(self.bridge.calls)
        async def absent(*args):
            return False
        with self.assertRaises(ValueError):
            await transaction.settle(absent)
        self.assertEqual(len(self.bridge.calls), count)

    async def test_changed_or_unowned_file_prevents_deletion(self):
        for changed in ("extra", "NativeTest", "native.log"):
            with self.subTest(file=changed):
                transaction = self.new()
                self.idle_calls = 0
                await self.complete(transaction)
                self.bridge.files[self.plan.directory + "/" + changed] = b"changed"
                with self.assertRaises(ValueError):
                    await transaction.settle(self.idle)
                self.assertFalse(any(c["type"] == "DELETEFILE" for c in self.bridge.calls))

    async def test_nonzero_native_return_retains_files(self):
        transaction = self.new()
        self.bridge.native_rc = 20
        await transaction.prepare()
        with self.assertRaises(ValueError):
            await transaction.execute()
        self.assertEqual(self.bridge.launches, 1)
        self.assertFalse(any(c["type"] == "DELETEFILE" for c in self.bridge.calls))

    async def test_bad_launch_ack_is_not_completion(self):
        transaction = self.new()
        await transaction.prepare()
        self.bridge.change_reply = lambda c, r: "CMD|123456|OK|Started: other" if c["type"] == "LAUNCH" else r
        count = len(self.bridge.calls)
        with self.assertRaisesRegex(ValueError, "launch acknowledgement"):
            await transaction.execute()
        self.assertEqual(self.bridge.calls[-1]["type"], "LAUNCH")
        self.assertGreater(len(self.bridge.calls), count)

    async def test_candidate_change_blocks_launch(self):
        transaction = self.new()
        await transaction.prepare()
        self.bridge.files[self.plan.path("NativeTest")] = b"other"
        with self.assertRaises(ValueError):
            await transaction.execute()
        self.assertEqual(self.bridge.launches, 0)

    async def test_short_write_ack_stops_without_remaining_chunks_or_launch(self):
        transaction = self.new()
        self.bridge.change_reply = lambda c, r: 'OK|WRITEFILE|2048' if c['type'] == 'WRITEFILE' else r
        with self.assertRaisesRegex(ValueError, 'Exact write acknowledgement absent'):
            await transaction.prepare()
        self.assertEqual(transaction.phase, 'FAILED')
        self.assertEqual(sum(c['type'] == 'WRITEFILE' for c in self.bridge.calls), 1)
        self.assertEqual(self.bridge.launches, 0)
        self.assertFalse(any(c['type'] in ('DELETEFILE', 'CHECKSUM') for c in self.bridge.calls))

    async def test_full_sized_candidate_preparation_keeps_original_checks(self):
        transaction = self.new()
        self.plan = Plan(self.plan.run_id, self.plan.command_id,
                         (bytes(range(256)) * 1660)[:424732], self.plan.markers)
        self.bridge = Bridge(self.plan)
        transaction = Transaction(self.plan, self.bridge.exchange, self.events.append)
        await transaction.prepare()
        self.assertEqual(self.bridge.files[self.plan.path('NativeTest')], self.plan.binary)
        writes = [c for c in self.bridge.calls if c['type'] == 'WRITEFILE'
                  and c['path'] == self.plan.path('NativeTest')]
        self.assertEqual(len(writes), 105)
        self.assertEqual({c['path'] for c in self.bridge.calls if c['type'] == 'CHECKSUM'},
                         {self.plan.path(n) for n in ('NativeTest', 'Run-once', 'complete.flag')})
        self.assertTrue(any(c['type'] == 'READFILE' and c['path'] == self.plan.path('Run-once')
                            for c in self.bridge.calls))
        self.assertEqual(self.bridge.launches, 0)

    async def test_ledger_failure_prevents_target_dispatch(self):
        transaction = self.new()
        def failed(*args):
            raise OSError("custody disk full")
        transaction.record = failed
        with self.assertRaises(OSError):
            await transaction.prepare()
        self.assertEqual(self.bridge.calls, [])

    async def test_cancelled_exchange_retains_state(self):
        transaction = self.new()
        async def cancelled(*args):
            raise asyncio.CancelledError()
        transaction.exchange = cancelled
        with self.assertRaises(asyncio.CancelledError):
            await transaction.prepare()
        self.assertEqual(transaction.phase, "FAILED")
        self.assertEqual(len(self.events), 2)

    async def test_native_wait_uses_original_deadline(self):
        now = [0]
        async def sleep(seconds):
            now[0] += seconds
        transaction = self.new(clock=lambda: now[0], sleep=sleep)
        self.bridge.complete = False
        await transaction.prepare()
        with self.assertRaisesRegex(ValueError, "Native completion deadline"):
            await transaction.execute(native_seconds=2)
        self.assertEqual(now[0], 2)
        self.assertEqual(self.bridge.launches, 1)
        self.assertFalse(any(c["type"] == "DELETEFILE" for c in self.bridge.calls))

    async def test_deadline_cannot_be_widened(self):
        transaction = self.new()
        await transaction.prepare()
        count = len(self.bridge.calls)
        with self.assertRaises(ValueError):
            await transaction.execute(native_seconds=91)
        self.assertEqual(len(self.bridge.calls), count)

    async def test_wrong_phase_blocks_repeat_launch_and_early_settlement(self):
        transaction = self.new()
        with self.assertRaises(ValueError):
            await transaction.execute()
        with self.assertRaises(ValueError):
            await transaction.settle(self.idle)
        self.assertEqual(self.bridge.calls, [])
        await self.complete(transaction)
        count = len(self.bridge.calls)
        with self.assertRaises(ValueError):
            await transaction.execute()
        self.assertEqual(len(self.bridge.calls), count)


if __name__ == "__main__":
    unittest.main()
