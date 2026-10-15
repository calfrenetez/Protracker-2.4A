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
            for relative in (*ACTION_OVERLAYS, "tests/editor_mixed_checked_test.c",
                             "tests/editor_document_publication_test.c"):
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
            for load, name, trace in ((0, "new", 0), (1, "load", 0), (0, "new", 1), (1, "load", 1)):
                executable = private / ("publication-" + name + str(trace))
                # Only this fixture deliberately includes uncalled static helpers.
                # Compile every production TU with the ordinary strict flags.
                fixture = private / ("publication-" + name + str(trace) + ".o")
                result = subprocess.run([
                    "cc", *FLAGS, "-Wno-unused-function", "-DPT_DOCUMENT_TYPED_ACTION=1",
                    f"-DPT_DOCUMENT_PUBLICATION_LOAD={load}",
                    f"-DPT_DOCUMENT_ACTION_TRACE={trace}",
                    "-c", "tests/editor_document_publication_test.c", "-o", str(fixture),
                ], cwd=private, env=environment, timeout=30, capture_output=True)
                self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
                result = subprocess.run([
                    "cc", *FLAGS, "-DPT_DOCUMENT_TYPED_ACTION=1",
                    str(fixture), *common, "-o", str(executable),
                ], cwd=private, env=environment, timeout=120, capture_output=True)
                self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
                arguments = [str(executable)]
                progress = private / ('RAM:PT24G-' + ('a1' if load else 'a0') * 8) / 'progress.log'
                if trace:
                    progress.parent.mkdir()
                    progress.write_bytes(b'NOT STARTED\n')
                    arguments.append(str(progress.relative_to(private)))
                result = subprocess.run(arguments, cwd=private, env=environment,
                                        timeout=30, capture_output=True)
                stages = (
                    'entered', 'fixture-allocated', 'initial-document-created', 'initial-editor-created',
                    'initial-masters-begun', 'initial-masters-ready', 'checked-editor-ready',
                    'checked-owner-begun', 'source-encoded', 'fixture-ready', 'prepared',
                    'published', 'committed', 'stopped-and-disposed', 'paula-closed',
                    'amigus-closed', 'finished',
                )
                diagnostic = ''.join(f'DOCUMENT ACTION TRACE:{s}\n' for s in stages) if trace else ''
                expected = (f"DOCUMENT PUBLICATION BEGIN:{name}\n" + diagnostic +
                            f"DOCUMENT PUBLICATION PASS:{name}; final-address owner and checked cleanup; injected voices\n").encode()
                self.assertEqual((result.returncode, result.stdout, result.stderr),
                                 (0, expected, b""))
                if trace:
                    self.assertEqual(progress.read_bytes(), b'DOCUMENT ACTION TRACE:finished\n')


if __name__ == "__main__":
    unittest.main()
