"""Exercise the real native cache wrapper with host-injected Exec memory calls.

These models validate diagnostic output and allocation accounting, not physical
Fast/Chip placement. The default diagnostic output remains available.
"""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

from test_sampler import SOURCES

ROOT = Path(__file__).resolve().parents[1]
POOL = b"EXEC MEMORY POOL PASS: Fast-only pool, reserve and zero-budget refusal"
FINISH = (b"EXEC MEMORY PASS: Fast allocations, zero owned bytes, "
          b"budget refusal without Chip fallback")


def memory_model(out):
    (out / "exec").mkdir()
    (out / "proto").mkdir()
    (out / "exec/memory.h").write_text(
        "#ifndef MODEL_EXEC_MEMORY_H\n#define MODEL_EXEC_MEMORY_H\n"
        "#include <stdint.h>\ntypedef uint32_t ULONG;\n"
        "#define MEMF_PUBLIC 1\n#define MEMF_CHIP 2\n"
        "#define MEMF_FAST 4\n#define MEMF_TOTAL 8\n#endif\n")
    (out / "proto/exec.h").write_text(
        "ULONG AvailMem(ULONG);\nvoid *AllocMem(ULONG,ULONG);\n"
        "void FreeMem(void *,ULONG);\nULONG TypeOfMem(const void *);\n")
    model = out / "exec_model.c"
    model.write_text(r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <exec/memory.h>
struct block {void *data; ULONG bytes,flags;};
static struct block blocks[1024];
static ULONG fast=128UL*1024*1024,chip=2UL*1024*1024;
ULONG AvailMem(ULONG flags)
{return flags&MEMF_FAST?fast:chip;}
void *AllocMem(ULONG bytes,ULONG flags)
{
    unsigned i;void *data;ULONG *available=flags&MEMF_FAST?&fast:&chip;
    assert(bytes && ((flags&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST ||
             (flags&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP));
    if(bytes>*available)return NULL;
    for(i=0;i<1024 && blocks[i].data;++i){}assert(i<1024);
    data=malloc(bytes);if(!data)return NULL;
    blocks[i]=(struct block){data,bytes,flags};*available-=bytes;return data;
}
ULONG TypeOfMem(const void *data)
{
    uintptr_t p=(uintptr_t)data;unsigned i;
    for(i=0;i<1024;++i)if(blocks[i].data &&
        p>=(uintptr_t)blocks[i].data && p-(uintptr_t)blocks[i].data<blocks[i].bytes)
        return blocks[i].flags;
    assert(0);return 0;
}
void FreeMem(void *data,ULONG bytes)
{
    unsigned i;for(i=0;i<1024 && blocks[i].data!=data;++i){}assert(i<1024);
    assert(data && bytes==blocks[i].bytes);
    if(blocks[i].flags&MEMF_FAST)fast+=bytes;else chip+=bytes;
    free(data);blocks[i]=(struct block){0};
}
''')
    return model


class NativeMemoryMarkers(unittest.TestCase):
    def test_fixed_and_default_cache_wrapper_output(self):
        with tempfile.TemporaryDirectory(prefix="pt-exec-memory-markers-") as folder:
            out = Path(folder)
            model = memory_model(out)
            common = ["tests/native_exec_sampler_paula_test.c",
                      "src/editor/sampler_paula.c", "src/core/sample_cache.c",
                      "src/core/playback_pcm.c", *SOURCES[1:], str(model)]
            outputs = []
            for fixed in (False, True):
                executable = out / ("fixed" if fixed else "default")
                flags = ["-DPT_TEST_NATIVE_MEMORY_FIXED_MARKERS=1"] if fixed else []
                build = subprocess.run(
                    ["cc", "-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                     "-fsanitize=address,undefined", "-I" + str(out), "-Isrc/core",
                     *flags, *common, "-o", str(executable)], cwd=ROOT, timeout=120,
                    check=True, capture_output=True)
                self.assertEqual((build.stdout, build.stderr), (b"", b""))
                result = subprocess.run([str(executable)], timeout=30, check=True,
                                        capture_output=True)
                self.assertEqual(result.stderr, b"")
                outputs.append(result.stdout.splitlines())
            default, fixed = outputs
            self.assertEqual(len(default), 4)
            self.assertEqual(len(fixed), 4)
            self.assertRegex(default[0], rb"^EXEC MEMORY pool_limit=[1-9][0-9]* reserve=262144 flags=4$")
            count = re.fullmatch(rb"EXEC MEMORY PASS: ([1-9][0-9]*) Fast allocations, "
                                 rb"zero owned bytes, budget refusal without Chip fallback", default[-1])
            self.assertIsNotNone(count)
            self.assertEqual(fixed[0], POOL)
            self.assertEqual(fixed[-1], FINISH)
            self.assertEqual(default[1:3], fixed[1:3])
            self.assertTrue(fixed[1].startswith(b"PAULA CHIP JOB PASS:"))
            self.assertTrue(fixed[2].startswith(b"SAMPLER PAULA PASS:"))


if __name__ == "__main__":
    unittest.main()
