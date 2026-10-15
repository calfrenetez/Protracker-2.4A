"""Focused New/Load diagnostics through the existing complete production recipe."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from test_editor_document_action import (
    ACTION_OVERLAYS, CHECKED, MIXED, WORKFLOW, FLAGS, ROOT, copy_overlay, prepare,
)


class DocumentPublication(unittest.TestCase):
    def test_new_and_load_use_actual_publication_and_cleanup(self):
        with tempfile.TemporaryDirectory(prefix="pt-publication-") as folder:
            sources, _ = prepare(folder)
            private = Path(folder)
            for relative in (*ACTION_OVERLAYS, "tests/editor_document_publication_test.c"):
                copy_overlay(ROOT / relative, private / relative)
            common = list(dict.fromkeys([
                *CHECKED, *MIXED[1:], *sources, *WORKFLOW,
                "src/editor/editor_mixed_bridges.c", "src/editor/editor_document_replacement.c",
                "src/editor/editor_document_action.c",
            ]))
            self.assertEqual(len(common), 83)
            environment = dict(os.environ, LC_ALL="C",
                               ASAN_OPTIONS="halt_on_error=1:abort_on_error=1",
                               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
            for load, name in ((0, "new"), (1, "load")):
                executable = private / ("publication-" + name)
                # Only this fixture deliberately includes uncalled static helpers.
                # Compile every production TU with the ordinary strict flags.
                fixture = private / ("publication-" + name + ".o")
                result = subprocess.run([
                    "cc", *FLAGS, "-Wno-unused-function", "-DPT_DOCUMENT_TYPED_ACTION=1",
                    f"-DPT_DOCUMENT_PUBLICATION_LOAD={load}",
                    "-c", "tests/editor_document_publication_test.c", "-o", str(fixture),
                ], cwd=private, env=environment, timeout=30, capture_output=True)
                self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
                result = subprocess.run([
                    "cc", *FLAGS, "-DPT_DOCUMENT_TYPED_ACTION=1",
                    str(fixture), *common, "-o", str(executable),
                ], cwd=private, env=environment, timeout=120, capture_output=True)
                self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
                result = subprocess.run([str(executable)], cwd=private, env=environment,
                                        timeout=30, capture_output=True)
                expected = (f"DOCUMENT PUBLICATION BEGIN:{name}\n"
                            f"DOCUMENT PUBLICATION PASS:{name}; final-address owner and checked cleanup; injected voices\n").encode()
                self.assertEqual((result.returncode, result.stdout, result.stderr),
                                 (0, expected, b""))


if __name__ == "__main__":
    unittest.main()
