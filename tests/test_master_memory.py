from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class MasterMemory(unittest.TestCase):
    def test_native_allocator_pressure_and_ownership(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'exec').mkdir()
            (root / 'proto').mkdir()
            (root / 'exec/memory.h').write_text('''#include <stdint.h>
typedef uint32_t ULONG;
#define MEMF_FAST 4
#define MEMF_CHIP 2
#define MEMF_PUBLIC 1
#define MEMF_TOTAL 8
''')
            (root / 'proto/exec.h').write_text('''ULONG AvailMem(ULONG);
void *AllocMem(ULONG,ULONG);
void FreeMem(void *,ULONG);
''')
            source = root / 'test.c'
            source.write_text(r'''
#include <assert.h>
#include <stdlib.h>
#include "src/native/master_memory.h"
static ULONG fast_total,fast_free,chip_free,last_flags,last_size;
static unsigned calls,fail;
ULONG AvailMem(ULONG f) {
    if(f & MEMF_TOTAL)return fast_total;
    return f & MEMF_FAST ? fast_free : chip_free;
}
void *AllocMem(ULONG n,ULONG f) {
    ++calls;last_flags=f;last_size=n;
    if(fail)return NULL;
    if(f & MEMF_FAST)fast_free-=n;else chip_free-=n;
    return malloc(n);
}
void FreeMem(void *p,ULONG n) {
    assert(n==*(size_t *)p);
    if(last_flags & MEMF_FAST)fast_free+=n;else chip_free+=n;
    free(p);
}
int main(void) {
    struct pt_master_memory m;void *a,*b;size_t initial;unsigned before;
    fast_total=128UL*1024*1024;fast_free=2UL*1024*1024;chip_free=2UL*1024*1024;
    pt_master_memory_init(&m);initial=m.limit;
    assert(initial==fast_free-256UL*1024 && m.flags==MEMF_FAST);
    a=pt_master_allocate(&m,1024);assert(a && last_flags==(MEMF_FAST|MEMF_PUBLIC));
    assert(m.used==1024+sizeof(size_t) && last_size==m.used);
    b=pt_master_allocate(&m,2048);assert(b && m.used==3072+2*sizeof(size_t));
    pt_master_release(&m,a);pt_master_release(&m,b);assert(m.used==0);
    fail=1;assert(!pt_master_allocate(&m,100) && m.used==0);fail=0;
    fast_free=256UL*1024;before=calls;
    assert(!pt_master_allocate(&m,1) && calls==before && chip_free==2UL*1024*1024);
    fast_free=64UL*1024*1024;assert(pt_master_memory_available(&m)==initial);
    before=calls;assert(!pt_master_allocate(&m,SIZE_MAX));assert(!pt_master_allocate(&m,0));
    assert(!pt_master_allocate(&m,initial) && calls==before);
    pt_master_release(&m,NULL);
    /* Fast installed but completely exhausted must never fall back to Chip. */
    fast_free=0;pt_master_memory_init(&m);assert(m.flags==MEMF_FAST && m.limit==0);
    assert(!pt_master_allocate(&m,1));
    fast_total=0;pt_master_memory_init(&m);assert(m.flags==MEMF_CHIP);
    assert(m.limit==chip_free-512UL*1024);
    a=pt_master_allocate(&m,4096);assert(a && last_flags==(MEMF_CHIP|MEMF_PUBLIC));
    pt_master_release(&m,a);assert(m.used==0);
    chip_free=128UL*1024;pt_master_memory_init(&m);assert(m.limit==0);
    assert(!pt_master_allocate(&m,1));return 0;
}
''')
            binary = root / 'test'
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-I'+str(root), '-I'+str(ROOT),
                            str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
