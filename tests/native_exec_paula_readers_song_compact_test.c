/* Optional exact same shared diagnostic with real RAM allocators. NOT RUN.
 * TypeOfMem/zero-budget assertions become placement evidence only if this exact
 * wrapper is separately qualified at runtime. No DMA/audio/device/card access.
 */
#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
#define PT_COMPACT_NO_MAIN
#include "paula_readers_song_compact_test.c"

static size_t compact_native_chip_bytes;
static unsigned compact_native_chip_calls;
static void *compact_native_allocate(void *context,size_t bytes)
{(void)context;return native_allocate(bytes);}
static void compact_native_release(void *context,void *p)
{(void)context;native_release(p);}
static void *compact_native_chip_allocate(void *context,size_t bytes)
{
    void *p=pt_paula_chip_allocate(context,bytes);
    if(p){
        assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
        ++compact_native_chip_calls;compact_native_chip_bytes+=bytes;
    }
    return p;
}
static void compact_native_chip_release(void *context,void *p,size_t bytes)
{
    assert(p&&bytes<=compact_native_chip_bytes);
    assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
    compact_native_chip_bytes-=bytes;pt_paula_chip_release(context,p,bytes);
}
int main(void)
{
    const struct pt_compact_song_memory memory={{NULL,compact_native_allocate,compact_native_release},
        NULL,compact_native_chip_allocate,compact_native_chip_release};
    int result;
    native_memory_start();result=pt_paula_readers_song_compact_test(&memory);
    assert(compact_native_chip_calls&&!compact_native_chip_bytes);
    printf("EXEC COMPACT CHIP PASS: %u Chip-only allocations, zero owned bytes; no DMA\n",compact_native_chip_calls);
    native_memory_finish();return result;
}
