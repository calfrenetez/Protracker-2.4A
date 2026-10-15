"""HOST-only callback fault matrix over genuine retained Paula source owners.

The original 75 provider cases also execute. This separate ordinary host entry
neither enables the older inhibited driver nor executes a native/future backend.
Source-body, include-closure and preservation checks remain in force. The four
changed document/lifetime bodies and their new retirement include are pinned
explicitly for this current-graph qualification; historical pins stay separate.
"""
import hashlib
from pathlib import Path
import subprocess
import tempfile
import unittest

import test_paula_segment_provider as basis

FIXTURE = "tests/paula_provider_callback_test.c"
FIXTURE_PIN = {'bytes': 9435, 'sha256': 'fa274ea88cd85061ae8e8128d4b6c1717890b18b24e4e237f8b1ce51de1279df'}
CURRENT_SOURCE_PINS = {'src/core/document.c': {'bytes': 33775, 'sha256': '64973ff3b3f2b2806ea523c752244689d3275aa9ec66b964bf4db74fea72e2b6'}, 'src/core/document.h': {'bytes': 3706, 'sha256': '9b6613ca8109c59599635ef1fef3b36a90b30703b3e7e3aa4f2674722e69316a'}, 'src/core/document_retirement_internal.h': {'bytes': 1949, 'sha256': 'eb033fe27358b69706b2772ff72abc6d9910e654a30b73e3725b697f232c65f1'}, 'src/editor/sampler_lifetime.c': {'bytes': 8529, 'sha256': '975e50324af6d280b7d4c82a75a41d56d1bc940412a14ff2ba89a676887700bb'}, 'src/editor/sampler_lifetime_internal.h': {'bytes': 2998, 'sha256': '489b613ba9d848708948bda0a956f79bff249434740a466bc77786f659c7479d'}}
EXTRA_ORACLE = (
    "provider callback matrix: 36 cases; 12 seams x holder/original/reentry faults\n"
    "20 independently drained providers; 16 process-lifetime HOLD providers; source owner retained\n"
    "HOST callback mutation detection only; between-entry mutation/native/IRQ/timing NOT QUALIFIED\n"
)


class ProviderCallbacks(unittest.TestCase):
    def test_all_callback_fault_seams(self):
        original_sources = basis.SOURCES
        original_pins = basis.FIXTURE_PINS
        before = basis.preservation_snapshot()
        source_pins = {}
        try:
            # The original fixture remains in the include closure and its 75
            # cases are called directly alongside the current dependency graph.
            basis.SOURCES = [FIXTURE, *original_sources[1:]]
            basis.FIXTURE_PINS = {**original_pins, **CURRENT_SOURCE_PINS, FIXTURE: FIXTURE_PIN}
            self.assertEqual(len(basis.SOURCES), 37)
            self.assertEqual(len(basis.FIXTURE_PINS), 95)
            with tempfile.TemporaryDirectory(prefix="pt-provider-callbacks-") as directory:
                private = Path(directory)
                source_pins = basis.source_snapshot(private)
                self.assertEqual(set(source_pins), set(basis.FIXTURE_PINS))
                font_pin, raw_font = basis.ordinary_pin(basis.PRESERVATION_ROOT / "vendor/pt23f/raw/ptfont.raw")
                self.assertEqual(font_pin["sha256"], basis.RAW_FONT_SHA256)
                font = ("static const unsigned char pt_font[580] = {" +
                        ",".join(str(value) for value in raw_font) + "};\n").encode("ascii")
                self.assertEqual(hashlib.sha256(font).hexdigest(), basis.FONT_HEADER_SHA256)
                header = private / "pt_font.h"
                header.write_bytes(font)
                header.chmod(0o400)
                environment = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "TMPDIR": str(private),
                               "ASAN_OPTIONS": "halt_on_error=1:abort_on_error=1",
                               "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
                executable = private / "fixture"
                for command, timeout, expected in (
                    (["/usr/bin/cc", *basis.CFLAGS, *basis.SOURCES, "-o", str(executable)], 120, b""),
                    ([str(executable)], 30, (basis.ORACLE + EXTRA_ORACLE).encode("ascii")),
                ):
                    result = subprocess.run(command, cwd=private, env=environment,
                                            timeout=timeout, capture_output=True)
                    self.assertEqual((result.returncode, result.stdout, result.stderr), (0, expected, b""))
                self.assertEqual(basis.ordinary_pin(header)[0]["sha256"], basis.FONT_HEADER_SHA256)
                basis.verify_sources(source_pins)
                print(result.stdout.decode("ascii"), end="")
        finally:
            try:
                basis.verify_sources(source_pins)
            finally:
                basis.SOURCES = original_sources
                basis.FIXTURE_PINS = original_pins
                self.assertEqual(basis.preservation_snapshot(), before)


if __name__ == "__main__":
    unittest.main()
