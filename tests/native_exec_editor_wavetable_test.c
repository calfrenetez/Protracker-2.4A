#include "native_exec_memory.h"
#include "../src/native/eclock.h"
#define malloc native_allocate
#define free native_release
#define PT_EDITOR_WAVETABLE_NATIVE
#include "editor_wavetable_test.c"
static void native_eclock_fixture(void)
{
    struct pt_native_eclock clock={0};struct pt_elapsed_clock elapsed;
    pt_wavetable_clock_read read=pt_native_eclock_read;
    uint64_t ticks=99,previous,first,frames,deadline;uint32_t frequency=77,initial;unsigned i,epoch;
    assert(!read(&clock,&ticks,&frequency) && ticks==99 && frequency==77);
    for(epoch=0;epoch<3;++epoch) {
        assert(pt_native_eclock_open(&clock));assert(!pt_native_eclock_open(&clock));
        assert(read(&clock,&first,&initial) && initial);
        assert(pt_elapsed_clock_init(&elapsed,initial,48000,first,0)==PT_ELAPSED_OK);previous=first;
        for(i=0;i<100;++i) {
            assert(read(&clock,&ticks,&frequency) && frequency==initial && ticks>=previous);previous=ticks;
            assert(pt_elapsed_clock_advance(&elapsed,frequency,ticks,&frames)==PT_ELAPSED_OK);
            assert(pt_elapsed_clock_deadline(&elapsed,frames+48000,&deadline)==PT_ELAPSED_OK && deadline>ticks);
        }
        assert(ticks>first);
        printf("NATIVE ECLOCK observed frequency=%lu reads=101 epoch=%u\n",(unsigned long)initial,epoch);
        pt_native_eclock_close(&clock);pt_native_eclock_close(&clock);ticks=99;frequency=77;
        assert(!clock.port && !clock.request && !clock.opened);
        assert(!read(&clock,&ticks,&frequency) && ticks==99 && frequency==77);
    }
    puts("NATIVE ECLOCK PASS: real timer.device reads, stable frequency, monotonic64-bit counter, checked frame/deadline conversion, repeated close/reopen; no timer requests or audio output");
}
int main(void){int result;native_memory_start();native_eclock_fixture();result=editor_wavetable_fixture();native_memory_finish();return result;}
