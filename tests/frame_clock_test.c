#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frame_clock.h"
static void tests(void)
{
    struct pt_frame_clock c,before;uint32_t frames,i;uint64_t total;
    memset(&c,0x55,sizeof(c));before=c;
    assert(pt_frame_clock_init(&c,0,1)==PT_CLOCK_INVALID && !memcmp(&c,&before,sizeof(c)));
    assert(pt_frame_clock_init(&c,192001,1)==PT_CLOCK_INVALID && !memcmp(&c,&before,sizeof(c)));
    assert(pt_frame_clock_init(&c,48000,0)==PT_CLOCK_INVALID && !memcmp(&c,&before,sizeof(c)));
    assert(pt_frame_clock_init(NULL,48000,100)==PT_CLOCK_INVALID);
    assert(pt_frame_clock_init(&c,48000,UINT64_MAX)==PT_CLOCK_OK);
    assert(pt_frame_clock_tick(&c,125,&frames)==PT_CLOCK_OK && frames==960 && c.frames==960 && !c.fraction);
    before=c;frames=123;
    assert(pt_frame_clock_tick(&c,31,&frames)==PT_CLOCK_INVALID && frames==123 && !memcmp(&c,&before,sizeof(c)));
    assert(pt_frame_clock_tick(&c,256,&frames)==PT_CLOCK_INVALID && frames==123 && !memcmp(&c,&before,sizeof(c)));
    assert(pt_frame_clock_tick(&c,125,NULL)==PT_CLOCK_INVALID && !memcmp(&c,&before,sizeof(c)));
    assert(pt_frame_clock_tick(&c,125,&c.rate)==PT_CLOCK_INVALID && !memcmp(&c,&before,sizeof(c)));
    assert(pt_frame_clock_init(&c,44100,44100)==PT_CLOCK_OK);
    for(i=0;i<50;++i)assert(pt_frame_clock_tick(&c,125,&frames)==PT_CLOCK_OK && frames==882);
    assert(c.frames==44100 && !c.fraction);before=c;frames=123;
    assert(pt_frame_clock_tick(&c,125,&frames)==PT_CLOCK_LIMIT && frames==123 && !memcmp(&c,&before,sizeof(c)));
    assert(pt_frame_clock_init(&c,192000,UINT64_MAX)==PT_CLOCK_OK);c.frames=UINT64_MAX-14999;before=c;
    assert(pt_frame_clock_tick(&c,32,&frames)==PT_CLOCK_LIMIT && !memcmp(&c,&before,sizeof(c)));
    --c.frames;assert(pt_frame_clock_tick(&c,32,&frames)==PT_CLOCK_OK && frames==15000 && c.frames==UINT64_MAX);
    /* Fraction survives a tempo switch; independent tick rounding loses it. */
    assert(pt_frame_clock_init(&c,48000,10000)==PT_CLOCK_OK);
    assert(pt_frame_clock_tick(&c,128,&frames)==PT_CLOCK_OK && frames==937 && c.fraction==0x80000000UL);
    assert(pt_frame_clock_tick(&c,192,&frames)==PT_CLOCK_OK && frames==625 && c.fraction==0x80000000UL);
    assert(pt_frame_clock_tick(&c,128,&frames)==PT_CLOCK_OK && frames==938 && !c.fraction && c.frames==2500);
    /* Frame rates below tick frequency may emit zero frames; phase still moves. */
    assert(pt_frame_clock_init(&c,1,10)==PT_CLOCK_OK);total=0;
    for(i=0;i<256;++i) {assert(pt_frame_clock_tick(&c,128,&frames)==PT_CLOCK_OK);total+=frames;}
    assert(total==5 && c.frames==5 && !c.fraction);
    puts("FRAME CLOCK PASS: fractional tempo changes, exact rates, bounded drift, zero-frame ticks, limits/overflow and atomic failure");
}
int main(int argc,char **argv)
{
    struct pt_frame_clock c;unsigned i,count,rate;uint32_t frames;uint64_t hash=1469598103934665603ULL;
    if(argc==1) {tests();return 0;}
    assert(argc==3);rate=(unsigned)strtoul(argv[1],NULL,10);count=(unsigned)strtoul(argv[2],NULL,10);
    assert(count && count<=1000000 && pt_frame_clock_init(&c,rate,UINT64_MAX)==PT_CLOCK_OK);
    for(i=0;i<count;++i) {
        unsigned bpm=32+(i*97UL)%224;
        assert(pt_frame_clock_tick(&c,bpm,&frames)==PT_CLOCK_OK);
        hash=(hash^frames)*1099511628211ULL;
        if(i<10 || (i+1)%10000==0 || i+1==count)
            printf("CLOCK %u %u %lu %lu %lu %lu\n",i+1,bpm,(unsigned long)frames,(unsigned long)(c.frames>>32),(unsigned long)(uint32_t)c.frames,(unsigned long)c.fraction);
    }
    printf("HASH %08lx%08lx\n",(unsigned long)(hash>>32),(unsigned long)(uint32_t)hash);return 0;
}
