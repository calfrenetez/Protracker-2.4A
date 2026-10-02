from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeRamBus(unittest.TestCase):
    def test_synthetic_descriptor_and_registers(self):
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
            sources = ['tests/native_amigus_ram_bus_test.c', 'src/native/amigus_ram_bus.c',
                'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c',
                'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c',
                'src/core/playback_pcm.c', 'src/core/pcm.c']
            exe = shim / 'test'
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-I'+str(shim), '-Ivendor/amigus-sdk',
                *sources, '-o', str(exe)], cwd=ROOT, check=True)
            subprocess.run([str(exe)], check=True)
