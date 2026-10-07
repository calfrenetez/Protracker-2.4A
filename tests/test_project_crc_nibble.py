"""SOURCE-only exact HOST recipe. Root executes once in an isolated source tree.

No target/service/lease/guard or source adoption. Root's collector separately
binds the actual source, protected paths, full streams and positive child reap.
"""
from pathlib import Path
import hashlib
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PT_PROJECT_CRC_NIBBLE_TEST_ENTRY = "project_crc_nibble_host"
PT_PROJECT_CRC_NIBBLE_TEST_VERSION = 1
CFLAGS = ["-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
          "-fsanitize=address,undefined", "-Isrc/core"]
SANITIZER_ENV = {"ASAN_OPTIONS": "detect_leaks=0:abort_on_error=1",
                 "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
GOLDENS = {
    "mixed.ptg": "e373b2abdfd931d86727672ce31a8fea890edcdb53b254a93b8b03b7a7b6a831",
    "song.ptg": "c1de3e61ef286cb71dcd5be570f179d6bc960b804e965d2b937111886b067377",
}
CRC_ORACLE = ("PROJECT CRC NIBBLE PASS: independent bitwise transitions/all 256 bytes/state basis; "
              "split/hole/read failures; mixed/song fixed goldens, exact encode/stream/positional decode; "
              "no reduced validation\n")
CORE = ["src/core/project.c", "src/core/channels.c", "src/core/pcm.c"]
STREAM = ["tests/project_stream_test.c", *CORE, "src/platform/project_file.c",
          "src/platform/file_save.c", "src/core/safe_save.c"]


def compile_and_run(binary, units, arguments, environment):
    subprocess.run(["cc", *CFLAGS, *units, "-o", str(binary)], cwd=ROOT,
                   env=environment, check=True, timeout=120)
    subprocess.run([str(binary), *map(str, arguments)], cwd=ROOT,
                   env=environment, check=True, timeout=30)


def main():
    environment = os.environ.copy()
    environment.update(SANITIZER_ENV)
    fixtures = ROOT / "tests/fixtures/project-v1"
    for name, digest in GOLDENS.items():
        assert hashlib.sha256((fixtures / name).read_bytes()).hexdigest() == digest
    with tempfile.TemporaryDirectory(prefix="pt-project-crc-nibble-") as temporary:
        workspace = Path(temporary)
        # The focused test includes project.c to inspect the PRIVATE update.
        # Do not also link project.c here: its selected body is included once.
        compile_and_run(workspace / "crc", ["tests/project_crc_nibble_test.c",
            "src/core/channels.c", "src/core/pcm.c"],
            [fixtures / "mixed.ptg", fixtures / "song.ptg"], environment)
        produced = workspace / "mixed.ptg"
        compile_and_run(workspace / "project", ["tests/project_test.c", *CORE],
                        [produced], environment)
        assert produced.read_bytes() == (fixtures / "mixed.ptg").read_bytes()
        print("PROJECT CRC GOLDEN PASS: exact fixed mixed.ptg original bytes", flush=True)
        for faults in (False, True):
            units = (["-Dread=pt_test_read", "tests/render_read_faults.c"] if faults else []) + STREAM
            compile_and_run(workspace / ("stream-faults" if faults else "stream"), units,
                            [workspace / "master.ptg"], environment)
        compile_and_run(workspace / "reader", ["tests/project_reader_test.c", *CORE],
                        [fixtures / "mixed.ptg"], environment)
    for name, digest in GOLDENS.items():
        assert hashlib.sha256((fixtures / name).read_bytes()).hexdigest() == digest


if __name__ == "__main__":
    main()
