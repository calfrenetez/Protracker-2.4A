"""Host-only transport-contract tests; no service, sockets or targets."""
import asyncio
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import native_file_protocol as p


class WireTests(unittest.TestCase):
    def test_directory(self):
        self.assertEqual(p.directory_page("DIR|T:|2|one(0,F),two(12,D)", "T:", 0),
                         (2, (p.Entry("one", 0, "F"), p.Entry("two", 12, "D"))))
        self.assertEqual(p.directory_page("DIR|T:|0|", "T:", 0), (0, ()))

    def test_reject_malformed_directory(self):
        for text in ("", "DIR|RAM:|0|", "DIR|T:|junk|", "DIR|T:|00|", "DIR|T:|-1|",
                     "DIR|T:|4097|", "DIR|T:|1|", "DIR|T:|0|x(0,F)", "DIR|T:|1|x",
                     "DIR|T:|1|x(0,Q)", "DIR|T:|1|x(-1,F)", "DIR|T:|1|x(00,F)",
                     "DIR|T:|1|x(4294967296,F)", "DIR|T:|1|x(0,F),", "DIR|T:|1|x(0,F)junk",
                     "DIR|T:|1|x,(0,F)", "DIR|T:|1|x(y(0,F)", "DIR|T:|1| x(0,F)",
                     "DIR|T:|1|x (0,F)", "DIR|T:|1|x/y(0,F)", "DIR|T:|1|x:y(0,F)",
                     "DIR|T:|1|x|y(0,F)", "DIR|T:|1|..(0,F)", "DIR|T:|1|é(0,F)",
                     "DIR|T:|2|one(0,F),ONE(0,F)", "DIR|T:|0|\n",
                     "DIR|T:|1|" + "a" * 108 + "(0,F)"):
            with self.subTest(raw=text), self.assertRaises(ValueError):
                p.directory_page(text, "T:", 0)

    def test_paths(self):
        for value in ("T:", "RAM:PT-123/fixture.out", "DH0:Temp/"):
            self.assertEqual(p.path(value), value)
        for value in ("", None, "T:../x", "RAM:x/./a", "T:x;Delete", "T:x|y", "T:x\n", "T:x//y"):
            with self.subTest(path=value), self.assertRaises(ValueError):
                p.path(value)

    def test_wire_file_offset_precedes_length(self):
        self.assertEqual(p.file_chunk("FILE|T:out|256|2|a0FF", "T:out", 256, 256), b"\xa0\xff")
        self.assertEqual(p.file_chunk("FILE|T:out|258|0|", "T:out", 258, 1), b"")

    def test_file_rejects_wrong_or_partial_fields(self):
        for text in ("FILE|T:other|256|2|0011", "FILE|T:out|2|256|0011", "FILE|T:out|256|3|0011",
                     "FILE|T:out|256|1|0011", "FILE|T:out|256|2|0x11", "FILE|T:out|256|0|00",
                     "FILE|T:out|0256|2|0011", "FILE|T:out|256|2|00 11", "FILE|T:out|256|2|0011|",
                     "ERR|READFILE failed|T:out", "FILE|T:out|256|2|0011\r\n"):
            with self.subTest(raw=text), self.assertRaises(ValueError):
                p.file_chunk(text, "T:out", 256, 2)

    def test_explicit_script_plan_matches_bridge_bytes(self):
        for text in ("Echo hello", "Echo hello\n", "Echo hello\n\n", "\n"):
            plan = p.ScriptPlan(4294967295, text)
            self.assertEqual(plan.paths, ("T:ab_script_4294967295", "T:ab_sout_4294967295"))
            serialized = plan.command()["script"].replace(";", "\n")
            expected = b"FailAt 255\n" + serialized.encode()
            if not serialized.endswith("\n"):
                expected += b"\n"
            self.assertEqual(plan.body, expected + b"Echo ###ABDONE###\n")
            plan.acknowledge("CMD|4294967295|OK|ASYNC|T:ab_sout_4294967295")

    def test_script_rejects_aliases_and_unbounded_input(self):
        for ident in (True, 0, -1, 4294967296, "17"):
            with self.subTest(ident=ident), self.assertRaises(ValueError):
                p.ScriptPlan(ident, "Echo hi")
        for text in ("", "Echo hi;Delete", "Echo x|y", "Echo ###ABDONE###", "Echo é", "Echo\rhi", "x" * 4097):
            with self.subTest(text=text[:40]), self.assertRaises(ValueError):
                p.ScriptPlan(17, text)

    def test_ack_must_match_id_status_path_and_async_mode(self):
        plan = p.ScriptPlan(17, "Echo hi")
        for raw in ("CMD|18|OK|ASYNC|T:ab_sout_17", "CMD|17|ERR|ASYNC|T:ab_sout_17",
                    "CMD|17|OK|ASYNC|T:ab_sout_18", "CMD|17|OK|hi", "CMD|17|OK|ASYNC| T:ab_sout_17",
                    "CMD|17|OK|ASYNC|T:ab_sout_17|", "CMD|17|OK|ASYNC|T:ab_sout_17\n"):
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                plan.acknowledge(raw)

    def test_complete_sentinel_is_terminal_line(self):
        self.assertIsNone(p.completed_capture(b"still running\n"))
        self.assertIsNone(p.completed_capture(b"###ABDONE###"))
        self.assertIsNone(p.completed_capture(b"hi\n###ABDONE###"))
        self.assertEqual(p.completed_capture(b"###ABDONE###\n"), b"")
        self.assertEqual(p.completed_capture(b"hi\n###ABDONE###\n"), b"hi\n")
        for data in (b"hi###ABDONE###\n", b"###ABDONE###\nmore\n",
                     b"###ABDONE###\n###ABDONE###\n", b"hi\r###ABDONE###\n", b"x" * 65537):
            with self.subTest(data=data[:40]), self.assertRaises(ValueError):
                p.completed_capture(data)

    def test_collision_case_insensitive(self):
        entries = p.directory_page("DIR|T:|1|AB_SCRIPT_17(4,F)", "T:", 0)[1]
        with self.assertRaises(ValueError):
            p.absent(entries, p.ScriptPlan(17, "Echo hi").names)
        p.absent(entries, p.ScriptPlan(18, "Echo hi").names)


class Observations(unittest.IsolatedAsyncioTestCase):
    def observer(self, replies, **kwargs):
        self.calls = []
        async def exchange(command, kind, timeout):
            self.calls.append((command, kind, timeout))
            result = replies.pop(0)
            if isinstance(result, BaseException):
                raise result
            return result
        return p.Observation(exchange, **kwargs)

    async def test_paged_directory(self):
        observer = self.observer(["DIR|T:|3|one(0,F)", "DIR|T:|3|two(12,D),three(2,F)"])
        self.assertEqual([e.name for e in await observer.directory("T:")], ["one", "two", "three"])
        self.assertEqual([c[0]["offset"] for c in self.calls], [0, 1])

    async def test_partial_directory_never_becomes_success(self):
        for second in (TimeoutError(), "ERR|LISTDIR failed|T:", "DIR|T:|2|", "DIR|T:|3|two(2,F)",
                       "DIR|T:|2|ONE(2,F)", "DIR|RAM:|2|two(2,F)"):
            with self.subTest(second=second):
                observer = self.observer(["DIR|T:|2|one(0,F)", second])
                with self.assertRaises((TimeoutError, ValueError)):
                    await observer.directory("T:")
                with self.assertRaises(ValueError):
                    await observer.directory("T:")
                self.assertEqual(len(self.calls), 2)

    async def test_page_count_is_bounded(self):
        replies = [f"DIR|T:|65|f{i}(0,F)" for i in range(65)]
        observer = self.observer(replies)
        with self.assertRaisesRegex(ValueError, "page bound"):
            await observer.directory("T:")
        self.assertEqual(len(self.calls), 64)

    async def test_short_chunk_is_not_eof(self):
        info = "FILEINFO|T:out|3|FILE|00000000|"
        observer = self.observer([info, "FILE|T:out|0|2|4142", "FILE|T:out|2|1|43", "FILE|T:out|3|0|", info])
        self.assertEqual(await observer.file("T:out"), b"ABC")
        self.assertEqual([c[0]["offset"] for c in self.calls if c[1] == "FILE"], [0, 2, 3])

    async def test_exact_limit_still_checks_eof(self):
        info = "FILEINFO|T:out|2|FILE|00000000|"
        observer = self.observer([info, "FILE|T:out|0|2|4142", "FILE|T:out|2|0|", info])
        self.assertEqual(await observer.file("T:out", 2), b"AB")
        self.assertEqual([c[0]["size"] for c in self.calls if c[1] == "FILE"], [2, 1])
        observer = self.observer(["FILEINFO|T:out|3|FILE|00000000|"])
        with self.assertRaisesRegex(ValueError, "exceeds"):
            await observer.file("T:out", 2)
        self.assertEqual(len(self.calls), 1)

    async def test_file_second_read_failure_keeps_partial_unaccepted(self):
        observer = self.observer(["FILEINFO|T:out|2|FILE|00000000|", "FILE|T:out|0|1|41", TimeoutError()])
        with self.assertRaises(TimeoutError):
            await observer.file("T:out")
        with self.assertRaises(ValueError):
            await observer.file("T:out")
        self.assertEqual(len(self.calls), 3)

    async def test_read_count_is_bounded(self):
        observer = self.observer(["FILEINFO|T:out|513|FILE|00000000|"] +
                                [f"FILE|T:out|{i}|1|41" for i in range(513)])
        with self.assertRaisesRegex(ValueError, "read bound"):
            await observer.file("T:out")
        self.assertEqual(len(self.calls), 513)

    async def test_empty_file(self):
        info = "FILEINFO|T:out|0|FILE|00000000|"
        observer = self.observer([info, "FILE|T:out|0|0|", info])
        self.assertEqual(await observer.file("T:out", 0), b"")

    async def test_premature_eof_and_metadata_change_refused(self):
        info = "FILEINFO|T:out|2|FILE|00000000|"
        for replies in ([info, "FILE|T:out|0|0|"],
                        [info, "FILE|T:out|0|2|4142", "FILE|T:out|2|1|43"],
                        [info, "FILE|T:out|0|2|4142", "FILE|T:out|2|0|", "FILEINFO|T:out|3|FILE|00000000|"],
                        [info, "FILE|T:out|0|2|4142", "FILE|T:out|2|0|", "FILEINFO|T:out|2|FILE|00000001|"],
                        ["FILEINFO|T:out|0|DIR|00000000|"], ["FILEINFO|T:other|0|FILE|00000000|"]):
            observer = self.observer(replies)
            with self.assertRaises(ValueError):
                await observer.file("T:out")
            self.assertTrue(observer.failed)

    async def test_whole_deadline_before_and_after_reply(self):
        now = [0.0]
        observer = self.observer(["DIR|T:|0|"], seconds=1, clock=lambda: now[0])
        now[0] = 1.0
        with self.assertRaisesRegex(ValueError, "deadline"):
            await observer.directory("T:")
        self.assertEqual(self.calls, [])
        async def late(*args):
            now[0] = 5
            return "DIR|T:|0|"
        now[0] = 0
        observer = p.Observation(late, seconds=1, clock=lambda: now[0])
        with self.assertRaisesRegex(ValueError, "deadline"):
            await observer.directory("T:")

    async def test_cancellation_stops_without_cleanup_or_retry(self):
        observer = self.observer([asyncio.CancelledError()])
        with self.assertRaises(asyncio.CancelledError):
            await observer.file("T:out")
        with self.assertRaises(ValueError):
            await observer.directory("T:")
        self.assertEqual(len(self.calls), 1)

    async def test_concurrent_observation_refused_before_dispatch(self):
        started, release = asyncio.Event(), asyncio.Event()
        calls = []
        async def exchange(command, *_):
            calls.append(command)
            started.set()
            await release.wait()
            return "DIR|T:|0|"
        observer = p.Observation(exchange)
        first = asyncio.create_task(observer.directory("T:"))
        await started.wait()
        with self.assertRaises(ValueError):
            await observer.directory("T:")
        release.set()
        self.assertEqual(await first, ())
        self.assertEqual(len(calls), 1)

    async def test_real_async_timeout_is_finite(self):
        async def stalled(*args):
            await asyncio.Event().wait()
        observer = p.Observation(stalled, seconds=0.01)
        with self.assertRaises(TimeoutError):
            await observer.file("T:out")
        self.assertTrue(observer.failed)


if __name__ == "__main__":
    unittest.main()
