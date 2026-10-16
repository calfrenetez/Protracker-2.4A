/* Same genuine master/cache and split command/reader fixture, with real Exec
 * Fast/Chip allocators. Backend clocks/receipts remain an explicit task model.
 * No native provider, IRQ, DMA, voice stop, audio or scheduling qualification. */
#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
#if defined(PT_READERS_DIAGNOSTIC_TRACE) && PT_READERS_DIAGNOSTIC_TRACE
#include "sampler_paula_readers_trace.h"
#define PT_READERS_TRACE(stage,x,y,n) readers_memory_trace(stage,x,y,n)
#endif
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
#if defined(PT_READERS_SOURCE_BINDING_FIXTURE) && PT_READERS_SOURCE_BINDING_FIXTURE
#include "sampler_paula_readers_source_binding_test.c"
#else
#define main sampler_paula_readers_fixture
#include "sampler_paula_readers_test.c"
#undef main
#endif
int main(int argc,char **argv)
{
    int result;
#if defined(PT_READERS_DIAGNOSTIC_TRACE) && PT_READERS_DIAGNOSTIC_TRACE
    readers_memory_trace_begin(argc,argv);
    PT_READERS_TRACE("entered",0,0,0);
#else
    (void)argc;(void)argv;
#endif
    native_memory_start();PT_READERS_TRACE("memory-started",0,0,0);
#if defined(PT_READERS_SOURCE_BINDING_FIXTURE) && PT_READERS_SOURCE_BINDING_FIXTURE
    result=readers_binding_fixture();
#else
    result=sampler_paula_readers_fixture();
#endif
    assert(readers_native_chip_calls && !readers_native_chip_bytes);
    PT_READERS_TRACE("chip-closed",0,0,0);
    puts("EXEC READERS CHIP PASS: Chip-only cache extents, zero owned bytes; modeled backend, no DMA or audio");
    native_memory_finish();PT_READERS_TRACE("finished",0,0,0);return result;
}
