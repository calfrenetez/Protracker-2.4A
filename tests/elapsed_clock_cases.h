#include "../src/core/elapsed_clock.h"
static void elapsed_clock_fixture(void)
{
    struct pt_elapsed_clock c,one,before;uint64_t out=99,whole;unsigned i,j;
    const uint32_t rates[]={44100,48000,192000};
    memset(&c,0,sizeof(c));before=c;
    assert(pt_elapsed_clock_init(&c,0,48000,0,0)==PT_ELAPSED_INVALID);
    assert(pt_elapsed_clock_init(&c,1,192001,0,0)==PT_ELAPSED_INVALID && !memcmp(&c,&before,sizeof(c)));
    for(j=0;j<3;++j) {
        uint64_t ticks=0;
        assert(pt_elapsed_clock_init(&c,700001,rates[j],123,17)==PT_ELAPSED_OK);one=c;
        for(i=0;i<1000;++i) {
            ticks+=i%37;
            assert(pt_elapsed_clock_advance(&c,700001,123+ticks,&out)==PT_ELAPSED_OK);
            assert(out==17+ticks*rates[j]/700001 && c.fraction==ticks*rates[j]%700001);
        }
        assert(pt_elapsed_clock_advance(&one,700001,123+ticks,&out)==PT_ELAPSED_OK);
        assert(!memcmp(&c,&one,sizeof(c)));
        assert(pt_elapsed_clock_advance(&c,700001,c.ticks,&out)==PT_ELAPSED_OK && !memcmp(&c,&one,sizeof(c)));
    }
    assert(pt_elapsed_clock_init(&c,3,2,UINT64_MAX-3,0)==PT_ELAPSED_OK);
    assert(pt_elapsed_clock_advance(&c,3,UINT64_MAX,&out)==PT_ELAPSED_OK && out==2);
    /* Product overflows but the exact quotient fits. */
    assert(pt_elapsed_clock_init(&c,UINT32_MAX,192000,0,0)==PT_ELAPSED_OK);
    assert(pt_elapsed_clock_advance(&c,UINT32_MAX,UINT64_MAX,&out)==PT_ELAPSED_OK);
    assert(out==(UINT64_MAX/UINT32_MAX)*192000 && c.fraction==0);
    assert(pt_elapsed_clock_init(&c,1,1,0,0)==PT_ELAPSED_OK);
    assert(pt_elapsed_clock_advance(&c,1,UINT64_MAX,&out)==PT_ELAPSED_OK && out==UINT64_MAX);
    for(i=0;i<5;++i) {
        enum pt_elapsed_result result=i==0?PT_ELAPSED_REGRESSION:i<3?PT_ELAPSED_FREQUENCY:PT_ELAPSED_OVERFLOW;
        assert(pt_elapsed_clock_init(&c,i==4?3:1,i==4?2:1,9,UINT64_MAX)==PT_ELAPSED_OK);
        before=c;out=99;
        assert(pt_elapsed_clock_advance(&c,i==1?0:i==2?2:c.frequency,i==0?8:i==4?11:10,&out)==result);
        assert(out==99 && c.ticks==before.ticks && c.frames==before.frames && c.fraction==before.fraction);
        assert(pt_elapsed_clock_advance(&c,before.frequency,9,&out)==result && out==99);
    }
    assert(pt_elapsed_clock_init(&c,3,2,0,UINT64_MAX-1)==PT_ELAPSED_OK);
    assert(pt_elapsed_clock_advance(&c,3,2,&out)==PT_ELAPSED_OK && out==UINT64_MAX && c.fraction==1);
    before=c;whole=out;
    assert(pt_elapsed_clock_advance(&c,3,3,&out)==PT_ELAPSED_OVERFLOW && out==whole && c.fraction==before.fraction);
    assert(pt_elapsed_clock_init(&c,7,3,0,0)==PT_ELAPSED_OK);before=c;
    assert(pt_elapsed_clock_advance(&c,7,1,&c.frames)==PT_ELAPSED_INVALID && !memcmp(&c,&before,sizeof(c)));
    assert(pt_elapsed_clock_advance(&c,7,1,NULL)==PT_ELAPSED_INVALID && !memcmp(&c,&before,sizeof(c)));
    puts("ELAPSED CLOCK PASS: rational carry, partition invariance, large quotient, exact limit, sticky overflow/frequency/regression and atomic refusal");
}
