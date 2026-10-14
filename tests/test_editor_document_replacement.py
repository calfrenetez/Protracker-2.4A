"""Portable typed document replacement checks using the existing checked-owner recipe."""
import os
from pathlib import Path
import stat
import subprocess
import tempfile
import unittest

from test_editor_mixed import MIXED, prepare
from test_editor_mixed_checked import CHECKED
from test_editor_mixed_establish import WORKFLOW

ROOT = Path(__file__).resolve().parents[1]
OVERLAYS = (
    "src/core/document.c", "src/core/document.h",
    "src/core/document_retirement_internal.h",
    "src/editor/editor_mixed_checked.c", "src/editor/editor_mixed_checked.h",
    "src/editor/mixed_owner_established.c",
    "src/editor/mixed_owner_established_internal.h",
    "src/editor/editor_document_replacement.c",
    "src/editor/editor_document_replacement.h",
    "tests/editor_document_replacement_focus_test.c",
)
FLAGS = (
    "-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
    "-fsanitize=address,undefined", "-Isrc/core",
    "-DPT_DOCUMENT_TYPED_REPLACEMENT=1",
)
CHECKED_TERMINAL = (
    b"EDITOR CHECKED PASS:39 actual establishment/editor/checked-owner lifecycle cases;"
    b"18 whole-control allocator-alias and unchanged public-admission cases;"
    b"15 legacy transport regressions; host injected clocks/voices only\n"
)
TYPED_TERMINAL = (
    b"TYPED DOCUMENT REPLACEMENT PASS:10 focused lifecycle groups; HOST only;"
    b" final uncertain group retained\n"
)


class TypedDocumentReplacement(unittest.TestCase):
    def test_prepare_retire_and_construct_at_final_document_address(self):
        # The existing preparation exports the index tree, preserving unrelated
        # working-tree editor/display edits. Only this feature's files overlay it.
        with tempfile.TemporaryDirectory(prefix="pt-typed-document-") as folder:
            sources, _ = prepare(folder)
            private = Path(folder)
            for relative in OVERLAYS:
                source = ROOT / relative
                before = source.lstat()
                self.assertTrue(stat.S_ISREG(before.st_mode))
                self.assertEqual(before.st_nlink, 1)
                self.assertFalse(getattr(before, "st_flags", 0) & 0x40000000,
                                 "required candidate file is not resident")
                self.assertLessEqual(before.st_size, 2 * 1024 * 1024)
                fields = ("st_dev", "st_ino", "st_mode", "st_nlink", "st_size",
                          "st_uid", "st_gid", "st_mtime_ns", "st_ctime_ns", "st_flags")
                before_identity = tuple(getattr(before, field, 0) for field in fields)
                descriptor = os.open(source, os.O_RDONLY | os.O_NOFOLLOW)
                try:
                    opened = os.fstat(descriptor)
                    self.assertEqual(tuple(getattr(opened, field, 0) for field in fields),
                                     before_identity)
                    parts = []
                    amount = 0
                    while True:
                        piece = os.read(descriptor, 65536)
                        if not piece:
                            break
                        amount += len(piece)
                        self.assertLessEqual(amount, 2 * 1024 * 1024)
                        parts.append(piece)
                    self.assertEqual(tuple(getattr(os.fstat(descriptor), field, 0)
                                           for field in fields), before_identity)
                finally:
                    os.close(descriptor)
                self.assertEqual(tuple(getattr(source.lstat(), field, 0)
                                       for field in fields), before_identity)
                body = b"".join(parts)
                self.assertEqual(len(body), before.st_size)
                destination = private / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(body)
            ordered = [
                "tests/editor_document_replacement_focus_test.c",
                *dict.fromkeys([
                    *CHECKED, *MIXED[1:], *sources, *WORKFLOW,
                    "src/editor/editor_mixed_bridges.c",
                    "src/editor/editor_document_replacement.c",
                ]),
            ]
            self.assertEqual(len(ordered), 83)
            self.assertEqual(len(set(ordered)), 83)
            executable = private / "typed-document-test"
            environment = dict(os.environ, LC_ALL="C",
                ASAN_OPTIONS="halt_on_error=1:abort_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
            compiled = subprocess.run(
                ["cc", *FLAGS, *ordered, "-o", str(executable)],
                cwd=private, env=environment, check=True, timeout=120,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.assertEqual(compiled.stdout, b"")
            self.assertEqual(compiled.stderr, b"")
            actual = subprocess.run(
                [str(executable)], cwd=private, env=environment,
                check=True, timeout=30, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            self.assertEqual(actual.stderr, b"")
            self.assertEqual(actual.stdout.count(CHECKED_TERMINAL), 1)
            self.assertTrue(actual.stdout.endswith(TYPED_TERMINAL))


if __name__ == "__main__":
    unittest.main()
