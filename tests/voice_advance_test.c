#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "voice.h"
static void compare(struct pt_voice a)
{
    struct pt_voice b=a,before;unsigned block,i;int32_t out[2];
    for(block=0;block<20;++block) {
        unsigned n=(block*53)%257;
        for(i=0;i<n;++i)assert(pt_voice_frame(&a,out)==PT_PCM_OK);
        assert(pt_voice_advance(&b,1,n)==PT_PCM_OK);
        assert(!memcmp(&a,&b,sizeof(a)));
    }
    before=b;assert(pt_voice_advance(&b,1,257)==PT_PCM_INVALID && !memcmp(&b,&before,sizeof(b)));
    assert(pt_voice_advance(&b,17,1)==PT_PCM_INVALID && !memcmp(&b,&before,sizeof(b)));
}
int main(void)
{
    int32_t data[8]={1,-257,33,444,51,61,71,81},other[8]={9,19,29,39,49,59,69,79};
    struct pt_pcm p={data,8,8,48000,1,24},q={other,8,8,48000,1,24};
    struct pt_voice v;unsigned loop,i;
    uint64_t steps[]={1,1ULL<<31,1ULL<<32,(7ULL<<32)+91,UINT64_MAX};
    for(loop=0;loop<3;++loop)for(i=0;i<5;++i) {
        assert(pt_voice_init(&v,&p,0,8,(enum pt_voice_loop)loop,loop?2:0,loop?7:0,steps[i],1)==PT_PCM_OK);compare(v);
        if(loop!=PT_VOICE_PINGPONG) {assert(pt_voice_set_repeat_source(&v,&q,1,5)==PT_PCM_OK);compare(v);}
        assert(pt_voice_init_segment(&v,&p,5,8,0,1,steps[i],1)==PT_PCM_OK);compare(v);
    }
    memset(&v,0,sizeof(v));assert(pt_voice_advance(&v,1,256)==PT_PCM_OK);
    assert(pt_voice_advance(NULL,0,256)==PT_PCM_OK && pt_voice_advance(NULL,1,1)==PT_PCM_INVALID);
    /* The mirror must never read sample data: these descriptors deliberately
       have no data. Only initialized state is retained for the phase operation. */
    assert(pt_voice_init(&v,&p,0,8,PT_VOICE_ONCE,0,0,1ULL<<32,0)==PT_PCM_OK);
    p.data=NULL;assert(pt_voice_advance(&v,1,8)==PT_PCM_OK && !v.active);
    puts("VOICE advance PASS: exact phase/handoff equivalence without PCM reads");return 0;
}
