"""Portable opt-in document actions, preserving unrelated working-tree editor edits."""
import os
from pathlib import Path
import stat
import subprocess
import tempfile
import unittest

from test_editor_document_replacement import (
    CHECKED, MIXED, WORKFLOW, FLAGS, OVERLAYS, CHECKED_TERMINAL, TYPED_TERMINAL, prepare,
)

ROOT = Path(__file__).resolve().parents[1]
ACTION_OVERLAYS = (*OVERLAYS, "src/editor/editor_document_action.c",
                   "src/editor/editor_document_action.h", "tests/editor_document_action_focus_test.c",
                   "tests/editor_document_action_case_test.c", "tests/editor_document_actions_test.c")
ACTION_TERMINAL = (b"DOCUMENT ACTION PASS:13 prewrite admission/reset, NEW/load preservation, cancel,"
                   b" WAITING and final-address publication groups; HOST only\n")
ACTION_CASES = (
    "admit-candidate-overlap", "admit-context-overlap", "reset-overlap",
    "new-no-memory", "load-invalid", "new-budget", "new-channel-limit",
    "cancel-new", "cancel-load", "publish-new", "publish-load",
    "publish-waiting", "publish-no-owner",
)


def copy_overlay(source, destination):
    before = source.lstat()
    fields = ("st_dev", "st_ino", "st_mode", "st_nlink", "st_size", "st_uid", "st_gid",
              "st_mtime_ns", "st_ctime_ns", "st_flags")
    identity = tuple(getattr(before, field, 0) for field in fields)
    if (not stat.S_ISREG(before.st_mode) or before.st_nlink != 1 or
            getattr(before, "st_flags", 0) & 0x40000000 or before.st_size > 2 * 1024 * 1024):
        raise AssertionError("overlay is not a resident bounded ordinary file")
    descriptor = os.open(source, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        if tuple(getattr(os.fstat(descriptor), field, 0) for field in fields) != identity:
            raise AssertionError("overlay identity changed before read")
        chunks = []
        size = 0
        while True:
            chunk = os.read(descriptor, 65536)
            if not chunk:
                break
            size += len(chunk)
            if size > 2 * 1024 * 1024:
                raise AssertionError("overlay grew during read")
            chunks.append(chunk)
        if tuple(getattr(os.fstat(descriptor), field, 0) for field in fields) != identity:
            raise AssertionError("overlay changed during read")
    finally:
        os.close(descriptor)
    if tuple(getattr(source.lstat(), field, 0) for field in fields) != identity or size != before.st_size:
        raise AssertionError("overlay changed after read")
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(b"".join(chunks))


class TypedDocumentAction(unittest.TestCase):
    def test_detached_prepare_cancel_aliases_and_final_address_publication(self):
        with tempfile.TemporaryDirectory(prefix="pt-document-action-") as folder:
            sources, _ = prepare(folder)
            private = Path(folder)
            for relative in ACTION_OVERLAYS:
                copy_overlay(ROOT / relative, private / relative)
            common = list(dict.fromkeys([
                *CHECKED, *MIXED[1:], *sources, *WORKFLOW,
                "src/editor/editor_mixed_bridges.c", "src/editor/editor_document_replacement.c",
                "src/editor/editor_document_action.c",
            ]))
            self.assertEqual(len(common), 83)
            self.assertEqual(len(set(common)), 83)
            environment = dict(os.environ, LC_ALL="C",
                               ASAN_OPTIONS="halt_on_error=1:abort_on_error=1",
                               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")

            def run(command, timeout):
                result = subprocess.run(command, cwd=private, env=environment, check=True,
                                        timeout=timeout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                self.assertEqual(result.stderr, b"")
                return result.stdout

            def build(name, fixture, linkage=None):
                executable = private / name
                self.assertEqual(run(["cc", *FLAGS, "-DPT_DOCUMENT_TYPED_ACTION=1",
                                      fixture, *(common if linkage is None else linkage), "-o", str(executable)], 120), b"")
                return executable

            # Admission must not put a complete editor/owner snapshot on the
            # task stack. This compiler gate catches a large local copy even
            # when the functional fixture passes with the host's roomy stack.
            self.assertEqual(run(["cc", *FLAGS, "-DPT_DOCUMENT_TYPED_ACTION=1",
                                  "-Wframe-larger-than=4096", "-c",
                                  "src/editor/editor_document_action.c", "-o",
                                  str(private / "action-frame-check.o")], 30), b"")
            executable = build("document-action-test", "tests/editor_document_action_focus_test.c")
            body = run([str(executable)], 30)
            self.assertEqual(body.count(CHECKED_TERMINAL), 1)
            self.assertTrue(body.endswith(ACTION_TERMINAL))
            component = b"".join((f"DOCUMENT ACTION CASE BEGIN:{case}\n"
                                  f"DOCUMENT ACTION CASE PASS:{case}\n").encode()
                                 for case in ACTION_CASES)
            component += b"DOCUMENT ACTION COMPONENT PASS:13 action cases; prerequisites excluded\n"
            self.assertEqual(run([str(executable), "actions"], 30), component)
            for case in ACTION_CASES:
                with self.subTest(diagnostic=case):
                    self.assertEqual(run([str(executable), "case", case], 30),
                                     (f"DOCUMENT ACTION CASE BEGIN:{case}\n"
                                      f"DOCUMENT ACTION CASE PASS:{case}\n").encode())
            for arguments in (["case"], ["case", "unknown"], ["case", "0"],
                              ["case", "publish-new", "extra"], ["unexpected"],
                              ["actions", "extra"]):
                with self.subTest(invalid_diagnostic=arguments):
                    refused = subprocess.run([str(executable), *arguments], cwd=private,
                                             env=environment, timeout=30, capture_output=True)
                    self.assertEqual((refused.returncode, refused.stdout, refused.stderr),
                                     (20, b"", b""))
            fixed = build("document-action-case-test", "tests/editor_document_action_case_test.c")
            self.assertEqual(run([str(fixed)], 30),
                             b"DOCUMENT ACTION CASE BEGIN:admit-candidate-overlap\n"
                             b"DOCUMENT ACTION CASE PASS:admit-candidate-overlap\n")
            actions = build("document-actions-test", "tests/editor_document_actions_test.c")
            self.assertEqual(run([str(actions)], 30), component)
            for case in range(19):
                with self.subTest(alias=case):
                    self.assertEqual(run([str(executable), "alias", str(case)], 30),
                                     b"DOCUMENT ACTION ALIAS REFUSAL PASS\n")
            for case in (1, 2, 4):
                with self.subTest(uncertain=case):
                    self.assertEqual(run([str(executable), "uncertain", str(case)], 30),
                                     b"DOCUMENT ACTION UNCERTAIN HOLD PASS\n")
            baseline = build("typed-replacement-test", "tests/editor_document_replacement_focus_test.c")
            body = run([str(baseline)], 30)
            self.assertEqual(body.count(CHECKED_TERMINAL), 1)
            self.assertTrue(body.endswith(TYPED_TERMINAL))
            internal = private / "reset-private-controls-test.c"
            internal.write_text(
                '#include "src/editor/editor_document_action.c"\n'
                '#include <assert.h>\n#include <stdio.h>\n'
                'int main(void){struct pt_editor_document_action saved=expected;\n'
                'assert(!active&&!expected.self);\n'
                'assert(!pt_editor_document_action_reset(&expected));\n'
                'assert(!pt_editor_document_action_reset((void *)&active));\n'
                'assert(!active&&!memcmp(&saved,&expected,sizeof(saved)));\n'
                'puts("DOCUMENT ACTION PRIVATE RESET EXCLUSIONS PASS:2 zero-self no-write controls");return 0;}\n')
            executable = build("reset-private-controls-test", str(internal),
                               [source for source in common if source != "src/editor/editor_document_action.c"])
            self.assertEqual(run([str(executable)], 30),
                             b"DOCUMENT ACTION PRIVATE RESET EXCLUSIONS PASS:2 zero-self no-write controls\n")


if __name__ == "__main__":
    unittest.main()
