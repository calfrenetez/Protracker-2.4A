/* Same genuine master/cache and split command/reader fixture, with real Exec
 * Fast/Chip allocators. Backend clocks/receipts remain an explicit task model.
 * No native provider, IRQ, DMA, voice stop, audio or scheduling qualification. */
#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
static size_t readers_native_chip_bytes;
static unsigned readers_native_chip_calls;
static void *readers_native_chip_allocate(size_t bytes)
{
    void *p=pt_paula_chip_allocate(NULL,bytes);
    if(p){
        assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
        assert((TypeOfMem((unsigned char *)p+bytes-1)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
        ++readers_native_chip_calls;readers_native_chip_bytes+=bytes;
    }
    return p;
}
static void readers_native_chip_release(void *p,size_t bytes)
{
    assert(p && bytes && bytes<=readers_native_chip_bytes);
    assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
    assert((TypeOfMem((unsigned char *)p+bytes-1)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
    readers_native_chip_bytes-=bytes;pt_paula_chip_release(NULL,p,bytes);
}
#define PT_READERS_NATIVE_MEMORY
#define malloc native_allocate
#define free native_release
#define main sampler_paula_readers_fixture
#include "sampler_paula_readers_test.c"
#undef main
int main(void)
{
    int result;native_memory_start();result=sampler_paula_readers_fixture();
    assert(readers_native_chip_calls && !readers_native_chip_bytes);
    puts("EXEC READERS CHIP PASS: Chip-only cache extents, zero owned bytes; modeled backend, no DMA or audio");
    native_memory_finish();return result;
}
