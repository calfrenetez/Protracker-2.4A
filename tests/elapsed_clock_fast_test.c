#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/core/elapsed_clock.h"
#include "elapsed_clock_cases.h"
/* Host-only independent wide-product oracle across both numerator paths. */
static uint64_t random_state=UINT64_C(0x731cb498642ade01);
static uint64_t next_random(void)
{random_state^=random_state<<13;random_state^=random_state>>7;random_state^=random_state<<17;return random_state;}
int main(void)
{
    unsigned i,j;struct pt_elapsed_clock c,before;uint64_t out,delta,ticks,frames;
    elapsed_clock_fixture();elapsed_deadline_fixture();
    for(i=0;i<20000;++i) {
        uint32_t frequency=(uint32_t)next_random();uint32_t rate=(uint32_t)(next_random()%192000)+1;
        if(!frequency)frequency=1;
        assert(pt_elapsed_clock_init(&c,frequency,rate,next_random(),next_random())==PT_ELAPSED_OK);
        c.fraction=(uint32_t)(next_random()%frequency);before=c;
        for(j=0;j<10;++j) {
            uint64_t bound=(UINT32_MAX-c.fraction)/rate;
            __uint128_t numerator,increment;
            enum pt_elapsed_result result;
            c=before;
            delta=j==0?0:j==1?bound:j==2?bound+1:j==3 && bound?bound-1:
                j==4?frequency:j==5?(uint64_t)UINT32_MAX+1:j==6?UINT32_MAX:next_random();
            if(delta>UINT64_MAX-c.ticks)delta=UINT64_MAX-c.ticks;
            ticks=c.ticks+delta;out=UINT64_C(0xfedcba9876543210);
            numerator=(__uint128_t)delta*rate+c.fraction;increment=numerator/frequency;
            result=pt_elapsed_clock_advance(&c,frequency,ticks,&out);
            if(increment>UINT64_MAX-before.frames) {
                assert(result==PT_ELAPSED_OVERFLOW && out==UINT64_C(0xfedcba9876543210));
                c.failure=PT_ELAPSED_OK;assert(!memcmp(&c,&before,sizeof(c)));
            }else {
                assert(result==PT_ELAPSED_OK && out==before.frames+(uint64_t)increment);
                assert(c.frames==out && c.ticks==ticks && c.fraction==numerator%frequency);
            }
            c=before;bound=UINT32_MAX/frequency;
            delta=j==0?0:j==1?bound:j==2?bound+1:j==3 && bound?bound-1:
                j==4?rate:j==5?(uint64_t)UINT32_MAX+1:j==6?UINT32_MAX:next_random();
            if(delta>UINT64_MAX-c.frames)delta=UINT64_MAX-c.frames;
            frames=c.frames+delta;out=UINT64_C(0xfedcba9876543210);
            if(delta) {
                numerator=(__uint128_t)delta*frequency-c.fraction;
                increment=numerator/rate+(numerator%rate!=0);
            }else increment=0;
            result=pt_elapsed_clock_deadline(&c,frames,&out);
            assert(!memcmp(&c,&before,sizeof(c)));
            if(increment>UINT64_MAX-before.ticks)
                assert(result==PT_ELAPSED_OVERFLOW && out==UINT64_C(0xfedcba9876543210));
            else assert(result==PT_ELAPSED_OK && out==before.ticks+(uint64_t)increment);
        }
    }
    puts("ELAPSED FAST ORACLE PASS:400000 advance/deadline comparisons,32-bit path boundaries, carry and atomic overflow; host only");return 0;
}
