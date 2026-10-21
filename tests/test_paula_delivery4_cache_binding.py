"""HOST-only current-source cache binding over genuine retained owners.
Runs the original 85 TASK snapshot/window cases plus nine source-pairing cases.
Does not enable the historical inhibited driver or grant native activation.
"""
import hashlib
from pathlib import Path
import subprocess
import tempfile
import unittest

import test_paula_delivery4_genuine_owner as basis

FIXTURE = 'tests/paula_delivery4_cache_binding_test.c'
FIXTURE_PIN = {'bytes': 2356, 'sha256': 'c76505e822bc34da0773cc064725030bc2bb8c88bbd322327629c012750efe59'}
CURRENT_SOURCE_PINS = {'src/editor/sampler_lifetime.c': {'bytes': 8529, 'sha256': '975e50324af6d280b7d4c82a75a41d56d1bc940412a14ff2ba89a676887700bb'}, 'src/editor/sampler_lifetime_internal.h': {'bytes': 2998, 'sha256': '489b613ba9d848708948bda0a956f79bff249434740a466bc77786f659c7479d'}, 'src/core/document.h': {'bytes': 3706, 'sha256': '9b6613ca8109c59599635ef1fef3b36a90b30703b3e7e3aa4f2674722e69316a'}, 'src/editor/paula_delivery4_task.c': {'bytes': 11662, 'sha256': '3b7298f8a5588ff24e5fb609b56c898e9e4901095cb12ba8fce72b14bfdf7264'}, 'src/editor/sampler_paula_internal.h': {'bytes': 3526, 'sha256': '18f67906cfc939dfd124c5513864bd4d84185fe913464d2024577af072cfad8d'}, 'src/core/document.c': {'bytes': 33775, 'sha256': '64973ff3b3f2b2806ea523c752244689d3275aa9ec66b964bf4db74fea72e2b6'}, 'src/core/document_retirement_internal.h': {'bytes': 1949, 'sha256': 'eb033fe27358b69706b2772ff72abc6d9910e654a30b73e3725b697f232c65f1'}, 'src/core/playback_pcm.c': {'bytes': 12122, 'sha256': '8c6847671c90fa8af25ba696cc16899e7a0104465c2cf9e4fc2dc269cda28c88'}, 'src/core/playback_internal.h': {'bytes': 2796, 'sha256': '266eb7c841cad43ecc34868c219bf723a3b5b836f2f733f6f25f45c68adfbef6'}, 'src/editor/sampler_paula.c': {'bytes': 7988, 'sha256': 'f0bd08c0c8fb1a9fd46dbf6f813ea1ebf8e769aa55b6b5425c9b875c77fc035d'}, 'tests/paula_delivery4_genuine_owner_test.c': {'bytes': 24805, 'sha256': 'd2f7fccbda7d450a61196fbf7a495e1e0235e59aebb399f84863bfe8f45f397d'}}
EXTRA_ORACLE = "delivery4 cache binding: 9 cases; 3 genuine controls, 6 wrong-source live leases refused\n"


class Delivery4CacheBinding(unittest.TestCase):
    maxDiff = None

    def test_live_leases_bound_to_their_source(self):
        original_sources = basis.SOURCES
        original_pins = basis.FIXTURE_PINS
        before = basis.preservation_snapshot()
        source_pins = {}
        try:
            # The unchanged 85-case behavior executes before the source-pair matrix.
            basis.SOURCES = [FIXTURE, *original_sources[1:]]
            basis.FIXTURE_PINS = {**original_pins, **CURRENT_SOURCE_PINS, FIXTURE: FIXTURE_PIN}
            self.assertEqual(len(basis.SOURCES), 29)
            self.assertEqual(len(basis.FIXTURE_PINS), 79)
            with tempfile.TemporaryDirectory(prefix="pt-delivery4-cache-binding-") as directory:
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
