"""Host checks for the routed MIDI runner. The emulator and bridge are fakes."""

import asyncio
from contextlib import asynccontextmanager, contextmanager
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import test_routed_midi_emulator as runner


class RoutedMidiEmulatorRunner(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        parent = Path(directory.name).resolve()
        self.root = parent / "harness"
        self.out = parent / "evidence"
        self.share = self.root / "runtime/Dev/Tests"
        self.profile = self.root / "runtime/amiberry/Configurations/030.uae"
        self.share.mkdir(parents=True)
        self.profile.parent.mkdir(parents=True)
        self.out.mkdir()
        self.profile.write_bytes(b"test profile")
        (self.root / "runtime/test.lock").touch()
        self.binary = b"test executable"
        self.args = SimpleNamespace(fixture="wire", pid=32, service_pid=37, sha256=runner.digest(self.binary),
                                    profile_sha256=runner.digest(self.profile.read_bytes()))
        self.calls = []
        self.checksum_ok = True
        self.completion = "PT-ROUTED-RC 0"
        self.guest_lines = [runner.REPLAY_PASS, runner.MIDI_PASS]
        self.service_current = True

        @contextmanager
        def guarded(*_args, **_kwargs):
            yield

        @asynccontextmanager
        async def http(url):
            self.assertEqual(url, "http://127.0.0.1:3000/mcp")
            yield None, None, None

        test = self

        class Session:
            def __init__(self, *_io):
                pass

            async def __aenter__(self):
                return self

            async def __aexit__(self, *_exception):
                return False

            async def initialize(self):
                pass

            async def call_tool(self, name, arguments):
                return test.reply(name, arguments)

        fake_modules = {
            "emulator": SimpleNamespace(R=self.root, PROFILES={runner.TARGET: "030"},
                                        owns=lambda command, target: (command, target) == ("owned", runner.TARGET),
                                        processes=lambda: [(32, "owned")], owned_listener=lambda pid, _: pid == 32),
            "launcher_access": SimpleNamespace(install_client=lambda: None, operation=guarded, transport=guarded),
            "bridge_checks": SimpleNamespace(checksum=self.checksum, target=lambda *_: None,
                                            require_reply=lambda reply: reply.content[0].text),
            "mcp": SimpleNamespace(ClientSession=Session),
            "mcp.client.streamable_http": SimpleNamespace(streamable_http_client=http),
        }
        modules = patch.dict(sys.modules, fake_modules)
        modules.start()
        self.addCleanup(modules.stop)
        listener = patch.object(runner, "devbench_listener_is", lambda pid: pid == 37 and self.service_current)
        listener.start()
        self.addCleanup(listener.stop)

    def checksum(self, text, binary):
        self.assertEqual(binary, self.binary)
        if not self.checksum_ok:
            raise RuntimeError("Guest checksum mismatch")
        self.assertEqual(text, "expected checksum")

    def reply(self, name, arguments):
        self.calls.append(name)
        run_dirs = list(self.share.iterdir())
        self.assertEqual(len(run_dirs), 1)
        executable = runner.FIXTURES[self.args.fixture][0]
        staged = run_dirs[0] / executable
        self.assertEqual(staged.read_bytes(), self.binary)
        if arguments.get("script") == "CPU":
            text = "System: 68030"
        elif name == "amiga_checksum":
            self.assertEqual(arguments["path"], f"Dev:Tests/{run_dirs[0].name}/{executable}")
            text = "expected checksum"
        else:
            self.assertIn(f"CD Dev:Tests/{run_dirs[0].name}", arguments["script"])
            self.assertIn(f"{executable} >{runner.GUEST_LOG}", arguments["script"])
            (run_dirs[0] / runner.GUEST_LOG).write_text("\n".join(self.guest_lines) + "\n")
            text = self.completion
        return SimpleNamespace(content=[SimpleNamespace(text=text)])

    def test_runs_exact_staged_bytes_and_preserves_log_before_cleanup(self):
        asyncio.run(runner.run(self.args, self.binary, self.out))
        self.assertEqual(self.calls, ["amiga_run_script", "amiga_checksum", "amiga_run_script"])
        self.assertEqual(list(self.share.iterdir()), [])
        self.assertTrue(json.loads((self.out / "result.json").read_text())["core_assertions_passed"])
        self.assertIn(runner.MIDI_PASS, (self.out / runner.GUEST_LOG).read_text())

    def test_mismatched_guest_checksum_never_launches_fixture(self):
        self.checksum_ok = False
        with self.assertRaisesRegex(RuntimeError, "checksum"):
            asyncio.run(runner.run(self.args, self.binary, self.out))
        self.assertEqual(self.calls, ["amiga_run_script", "amiga_checksum"])
        self.assertFalse((self.out / "result.json").exists())
        self.assertEqual(len(list(self.share.iterdir())), 1)

    def test_nonzero_exit_retains_produced_evidence_and_owned_files(self):
        self.completion = "PT-ROUTED-RC 20"
        with self.assertRaisesRegex(RuntimeError, "did not report clean completion"):
            asyncio.run(runner.run(self.args, self.binary, self.out))
        self.assertTrue((self.out / runner.GUEST_LOG).exists())
        self.assertFalse((self.out / "result.json").exists())
        self.assertEqual(len(list(self.share.iterdir())), 1)

    def test_schedule_fixture_requires_its_own_completion(self):
        self.args.fixture = "schedule"
        with self.assertRaisesRegex(RuntimeError, "did not report clean completion"):
            asyncio.run(runner.run(self.args, self.binary, self.out))
        self.assertTrue((self.out / runner.GUEST_LOG).exists())
        self.assertFalse((self.out / "result.json").exists())
        self.assertEqual(len(list(self.share.iterdir())), 1)

    def test_schedule_fixture_preserves_its_completion(self):
        self.args.fixture = "schedule"
        self.guest_lines = [runner.SCHEDULE_PASS]
        asyncio.run(runner.run(self.args, self.binary, self.out))
        self.assertEqual(list(self.share.iterdir()), [])
        self.assertEqual(json.loads((self.out / "result.json").read_text())["fixture"], "schedule")

    def test_replaced_service_refuses_before_staging(self):
        self.service_current = False
        with self.assertRaisesRegex(RuntimeError, "service identity changed"):
            asyncio.run(runner.run(self.args, self.binary, self.out))
        self.assertEqual(self.calls, [])
        self.assertEqual(list(self.share.iterdir()), [])
