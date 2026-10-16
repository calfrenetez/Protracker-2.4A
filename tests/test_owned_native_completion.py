"""Shell marker replacement model; no target, socket or process operation."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import native_file_protocol as wire
from owned_native_run import Plan, Transaction
from test_owned_native_run import Bridge


class ReplacingBridge(Bridge):
    def __init__(self, plan, marker, states):
        super().__init__(plan)
        self.marker, self.states = marker, list(states)
        self.active = False
        self.pending_reads = 0
        self.pending_directories = 0
        self.foreign = False
        self.directory_error = False

    def advance(self):
        value = self.states.pop(0)
        if value is None:
            self.files.pop(self.plan.path(self.marker), None)
        else:
            self.files[self.plan.path(self.marker)] = value

    async def exchange(self, command, kind, seconds):
        if self.active and command["type"] == "FILEINFO" and command.get("path") == self.plan.path(self.marker):
            if self.plan.path(self.marker) not in self.files:
                self.calls.append(dict(command))
                self.pending_reads += 1
                raise wire.BridgeReplyError("ERR|FILEINFO failed|" + command["path"])
        if self.active and command["type"] == "LISTDIR" and command["path"] == self.plan.directory:
            self.pending_directories += 1
            if self.directory_error:
                self.calls.append(dict(command))
                raise wire.BridgeReplyError("ERR|LISTDIR failed|" + command["path"])
        reply = await super().exchange(command, kind, seconds)
        if command["type"] == "LAUNCH":
            is_stage = command["id"] == self.plan.command_id + 1
            if is_stage == (self.marker == "staged.flag"):
                self.active = True
                self.advance()
                if self.foreign:
                    self.files[self.plan.directory + "/foreign"] = b"preserve"
        elif self.active and command["type"] == "LISTDIR" and command["path"] == self.plan.directory and self.states:
            self.advance()
        return reply


class CompletionTests(unittest.IsolatedAsyncioTestCase):
    def new(self, marker, states):
        plan = Plan("b4" * 16, 123456, b"inert fixture", ("native PASS",),
                    copy_source="RAM:PT24G-" + "b4" * 8 + "-in/NativeTest")
        bridge = ReplacingBridge(plan, marker, states)
        now = [0.0]

        async def sleep(seconds):
            now[0] += seconds

        transaction = Transaction(plan, bridge.exchange, lambda event: None,
                                  clock=lambda: now[0], sleep=sleep)
        return transaction, bridge, now

    async def test_stage_absent_then_partial_then_terminal_has_one_launch(self):
        transaction, bridge, _ = self.new("staged.flag", [None, b"STA", b"STAGED\n"])
        await transaction.prepare()
        self.assertEqual(transaction.phase, "PREPARED")
        self.assertEqual((bridge.copy_launches, bridge.launches), (1, 0))
        self.assertEqual(bridge.pending_reads, 0)
        self.assertGreaterEqual(bridge.pending_directories, 3)
        self.assertEqual(bridge.files[bridge.plan.copy_source], bridge.plan.binary)

    async def test_native_absent_then_partial_then_terminal_requires_exact_capture(self):
        transaction, bridge, _ = self.new("complete.flag", [None, b"COMP", b"COMPLETE\n"])
        await transaction.prepare()
        result = await transaction.execute()
        self.assertEqual(result["observed"]["returnCode"], 0)
        self.assertEqual((bridge.copy_launches, bridge.launches), (1, 1))
        self.assertEqual(bridge.pending_reads, 0)
        self.assertEqual(transaction.phase, "COMPLETED")

    async def test_missing_stage_retains_original_thirty_second_bound(self):
        transaction, bridge, now = self.new("staged.flag", [None])
        with self.assertRaisesRegex(ValueError, "deadline expired"):
            await transaction.prepare()
        self.assertLessEqual(now[0], 30)
        self.assertEqual((bridge.copy_launches, bridge.launches), (1, 0))
        self.assertEqual(transaction.phase, "FAILED")

    async def test_pending_native_keeps_original_deadline_and_no_relaunch(self):
        transaction, bridge, now = self.new("complete.flag", [None])
        await transaction.prepare()
        with self.assertRaisesRegex(ValueError, "deadline expired"):
            await transaction.execute(native_seconds=3)
        self.assertLessEqual(now[0], 3)
        self.assertEqual((bridge.copy_launches, bridge.launches), (1, 1))
        self.assertEqual(transaction.phase, "FAILED")

    async def test_wrong_terminal_content_refuses(self):
        for marker, value in (("staged.flag", b"FAILED\n"), ("complete.flag", b"BROKEN!!\n")):
            with self.subTest(marker=marker):
                transaction, bridge, _ = self.new(marker, [None, value])
                with self.assertRaisesRegex(ValueError, "Exact timely"):
                    await transaction.prepare()
                    await transaction.execute()
                self.assertEqual(transaction.phase, "FAILED")
                self.assertEqual(bridge.copy_launches, 1)
                self.assertLessEqual(bridge.launches, 1)

    async def test_oversized_marker_refuses_before_read(self):
        transaction, bridge, _ = self.new("staged.flag", [b"STAGED\nextra"])
        with self.assertRaisesRegex(ValueError, "exceeds bound"):
            await transaction.prepare()
        self.assertEqual(transaction.phase, "FAILED")
        self.assertEqual(bridge.launches, 0)

    async def test_foreign_entry_refuses_and_is_preserved(self):
        transaction, bridge, _ = self.new("staged.flag", [None, b"STAGED\n"])
        bridge.foreign = True
        with self.assertRaisesRegex(ValueError, "unowned entry"):
            await transaction.prepare()
        self.assertEqual(bridge.files[bridge.plan.directory + "/foreign"], b"preserve")
        self.assertEqual(transaction.phase, "FAILED")

    async def test_actual_directory_error_stops_without_followup(self):
        transaction, bridge, _ = self.new("staged.flag", [None, b"STAGED\n"])
        bridge.directory_error = True
        with self.assertRaises(wire.BridgeReplyError):
            await transaction.prepare()
        count = len(bridge.calls)
        with self.assertRaises(ValueError):
            await transaction.prepare()
        self.assertEqual(len(bridge.calls), count)
        self.assertEqual((bridge.copy_launches, bridge.launches), (1, 0))


if __name__ == "__main__":
    unittest.main()
