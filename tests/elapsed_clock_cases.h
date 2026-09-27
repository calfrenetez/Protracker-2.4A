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

static void elapsed_deadline_fixture(void)
{
    const uint32_t frequencies[]={1,3,700001,UINT32_MAX},rates[]={1,2,48000,192000};
    struct pt_elapsed_clock c,copy,before;uint64_t out,frame,reached,expected;unsigned f,r,i;
    for(f=0;f<4;++f)for(r=0;r<4;++r) {
        assert(pt_elapsed_clock_init(&c,frequencies[f],rates[r],123,17)==PT_ELAPSED_OK);
        assert(pt_elapsed_clock_advance(&c,frequencies[f],153,&reached)==PT_ELAPSED_OK);before=c;
        assert(pt_elapsed_clock_deadline(&c,c.frames,&out)==PT_ELAPSED_OK && out==153);
        for(i=1;i<68;++i) {
            frame=c.frames+i;
            /* Independent small-product oracle from original epoch. */
            expected=123+((frame-17)*frequencies[f]+rates[r]-1)/rates[r];
            assert(pt_elapsed_clock_deadline(&c,frame,&out)==PT_ELAPSED_OK && out==expected);
            assert(!memcmp(&c,&before,sizeof(c)));copy=c;
            assert(pt_elapsed_clock_advance(&copy,frequencies[f],out,&reached)==PT_ELAPSED_OK && reached>=frame);
            copy=c;
            assert(out>c.ticks && pt_elapsed_clock_advance(&copy,frequencies[f],out-1,&reached)==PT_ELAPSED_OK && reached<frame);
        }
    }
    assert(pt_elapsed_clock_init(&c,1,1,0,0)==PT_ELAPSED_OK);
    assert(pt_elapsed_clock_deadline(&c,UINT64_MAX,&out)==PT_ELAPSED_OK && out==UINT64_MAX);
    /* Carry borrowing must happen before the whole product overflow test. */
    assert(pt_elapsed_clock_init(&c,10,1,0,0)==PT_ELAPSED_OK);
    assert(pt_elapsed_clock_advance(&c,10,9,&out)==PT_ELAPSED_OK && c.fraction==9);
    frame=(UINT64_MAX-9)/10+1;
    assert(pt_elapsed_clock_deadline(&c,frame,&out)==PT_ELAPSED_OK && out==frame*10);
    before=c;out=99;
    assert(pt_elapsed_clock_deadline(&c,UINT64_MAX,&out)==PT_ELAPSED_OVERFLOW && out==99 && !memcmp(&c,&before,sizeof(c)));
    assert(pt_elapsed_clock_init(&c,3,2,UINT64_MAX-1,7)==PT_ELAPSED_OK);before=c;
    assert(pt_elapsed_clock_deadline(&c,8,&out)==PT_ELAPSED_OVERFLOW && out==99 && !memcmp(&c,&before,sizeof(c)));
    assert(pt_elapsed_clock_deadline(&c,6,&out)==PT_ELAPSED_INVALID && out==99);
    assert(pt_elapsed_clock_deadline(&c,7,&c.ticks)==PT_ELAPSED_INVALID && !memcmp(&c,&before,sizeof(c)));
    assert(pt_elapsed_clock_advance(&c,3,0,&out)==PT_ELAPSED_REGRESSION);
    assert(pt_elapsed_clock_deadline(&c,7,&out)==PT_ELAPSED_REGRESSION && out==99);
    puts("ELAPSED DEADLINE PASS: earliest reaching tick, nonintegral carry, low-frequency skipped frames, extreme bounds, borrowing and atomic refusal");
}
