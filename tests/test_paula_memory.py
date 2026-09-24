from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class PaulaMemory(unittest.TestCase):
    def test_live_chip_pressure_evicts_only_unpinned_caches(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'exec').mkdir()
            (root / 'proto').mkdir()
            (root / 'exec/memory.h').write_text('''#include <stdint.h>
typedef uint32_t ULONG;
#define MEMF_CHIP 2
#define MEMF_PUBLIC 1
''')
            (root / 'proto/exec.h').write_text('''ULONG AvailMem(ULONG);
void *AllocMem(ULONG,ULONG);
void FreeMem(void *,ULONG);
''')
            source = root / 'test.c'
            source.write_text(r'''
#include <assert.h>
#include <stdlib.h>
#include "src/native/paula_memory.h"
#include "sample_cache.h"
static ULONG available;static unsigned calls,fail;static size_t owned;
ULONG AvailMem(ULONG flags) {assert(flags==MEMF_CHIP);return available;}
void *AllocMem(ULONG n,ULONG flags) {
    void *p;++calls;assert(flags==(MEMF_CHIP|MEMF_PUBLIC));
    assert(n<=available && available-n>=PT_PAULA_CHIP_RESERVE);
    if(fail)return NULL;
    p=malloc(n);assert(p);available-=n;owned+=n;return p;
}
void FreeMem(void *p,ULONG n) {assert(p && n<=owned);owned-=n;available+=n;free(p);}
int main(void) {
    struct pt_sample_cache cache;struct pt_cache_lease active,idle,next;
    void *p;unsigned before;
    available=PT_PAULA_CHIP_RESERVE-1;
    assert(!pt_paula_chip_available());before=calls;
    assert(!pt_paula_chip_allocate(NULL,1) && calls==before);
    available=PT_PAULA_CHIP_RESERVE+4096;
    assert(pt_paula_chip_available()==4096);
    assert(!pt_paula_chip_allocate(NULL,0));
    assert(!pt_paula_chip_allocate(NULL,SIZE_MAX));
    assert(!pt_paula_chip_allocate(NULL,4097) && calls==before);
    p=pt_paula_chip_allocate(NULL,4096);assert(p && available==PT_PAULA_CHIP_RESERVE);
    assert(!pt_paula_chip_allocate(NULL,1));pt_paula_chip_release(NULL,p,4096);
    fail=1;assert(!pt_paula_chip_allocate(NULL,2) && !owned);fail=0;
    available=PT_PAULA_CHIP_RESERVE+8192;
    pt_cache_init(&cache,NULL,pt_paula_chip_allocate,pt_paula_chip_release,pt_paula_chip_available());
    assert(pt_cache_take(&cache,1,1,2048,&active)==PT_CACHE_LOAD);
    assert(pt_cache_publish(&cache,active));
    *(unsigned char *)pt_cache_data(&cache,active)=123;
    assert(pt_cache_take(&cache,2,1,2048,&idle)==PT_CACHE_LOAD);
    assert(pt_cache_publish(&cache,idle));assert(pt_cache_unpin(&cache,idle));
    /* Another application consumes the remaining headroom after cache init. */
    available=PT_PAULA_CHIP_RESERVE;
    assert(pt_cache_take(&cache,3,1,2048,&next)==PT_CACHE_LOAD);
    assert(cache.bytes==4096 && owned==4096 && available==PT_PAULA_CHIP_RESERVE);
    assert(*(unsigned char *)pt_cache_data(&cache,active)==123);
    assert(!pt_cache_data(&cache,idle));
    assert(pt_cache_publish(&cache,next));
    assert(pt_cache_take(&cache,4,1,1,&idle)==PT_CACHE_CAPACITY);
    assert(owned==4096 && pt_cache_data(&cache,active) && pt_cache_data(&cache,next));
    assert(pt_cache_unpin(&cache,next));
    fail=1;assert(pt_cache_take(&cache,4,1,1024,&idle)==PT_CACHE_CAPACITY);fail=0;
    assert(owned==2048 && *(unsigned char *)pt_cache_data(&cache,active)==123);
    assert(pt_cache_unpin(&cache,active));assert(pt_cache_clear(&cache));
    assert(!owned && !cache.bytes);
    return 0;
}
''')
            binary = root / 'test'
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-I'+str(root), '-I'+str(ROOT),
                            '-I'+str(ROOT / 'src/core'), str(source),
                            str(ROOT / 'src/core/sample_cache.c'), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
