from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeWavetableRead(unittest.TestCase):
    def run_fixture(self, *sources):
        with tempfile.TemporaryDirectory() as tmp:
            shim = Path(tmp)
            (shim / 'clib').mkdir()
            (shim / 'exec').mkdir()
            # Host-only scalar ABI shim. The real pinned SDK header supplies the
            # descriptor; actual68k layout is tested by the separate native build.
            (shim / 'clib/compiler-specific.h').write_text(
                '#define __ASM__\n#define __REG__(r,arg) arg\n')
            (shim / 'exec/types.h').write_text(
                '#include <stdint.h>\ntypedef void *APTR; typedef char *STRPTR;\n'
                'typedef uint32_t ULONG; typedef int32_t LONG;\n'
                'typedef uint16_t UWORD; typedef uint8_t UBYTE;\n')
            exe = shim / 'test'
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-I'+str(shim), '-Ivendor/amigus-sdk',
                'src/native/amigus_wavetable_read.c', *sources, '-o', str(exe)], cwd=ROOT, check=True)
            subprocess.run([str(exe)], check=True)

    def test_synthetic_descriptor_and_registers(self):
        self.run_fixture('tests/native_wavetable_read_test.c')

    def test_one_voice_start_and_stop_register_snapshots(self):
        self.run_fixture('tests/amigus_one_voice_snapshot_test.c', 'src/core/amigus_voice_plan.c')
