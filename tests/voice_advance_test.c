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
/* Preserved pre-optimization scalar transition, independent of block math.
 * Wide synthetic phase states need no sample storage/read. Small sources below
 * additionally compare against the unchanged public frame reader. */
static void scalar(struct pt_voice *v)
{
    uint64_t distance,amount;
    if(!v->active || !v->pcm)return;
    if(!v->loop) {
        distance=((uint64_t)v->end<<32)-v->phase;
        if(v->step>=distance){v->active=0;v->phase=(uint64_t)v->end<<32;}
        else v->phase+=v->step;
    }else if(!v->looped) {
        distance=((uint64_t)(v->segment?v->end:v->loop_start)<<32)-v->phase;
        if(v->step<distance)v->phase+=v->step;
        else {if(v->repeat_pcm){v->pcm=v->repeat_pcm;v->repeat_pcm=NULL;}
            v->looped=1;v->phase=v->cycle?(v->step-distance)%v->cycle:0;}
    }else if(v->cycle) {
        amount=v->step%v->cycle;distance=v->cycle-v->phase;
        v->phase=amount>=distance?amount-distance:v->phase+amount;
    }
}
static uint32_t random_state=0x82a75931UL;
static uint32_t random_word(void)
{random_state=random_state*1664525UL+1013904223UL;return random_state;}
static void bulk_fixture(const struct pt_pcm *p,const struct pt_pcm *q)
{
    struct pt_voice v[16],reference[16];unsigned test,ch,i,n;uint64_t step;
#ifdef PT_TEST_ADVANCE_NATIVE
    const unsigned cases=32;
#else
    const unsigned cases=4096;
#endif
#ifdef PT_TEST_ADVANCE_NATIVE
    puts("VOICE PHASE: randomized partitions start");fflush(stdout);
#endif
    for(test=0;test<cases;++test) {
        for(ch=0;ch<16;++ch) {
            unsigned loop=random_word()%3,segment=random_word()%2;
            step=((uint64_t)random_word()<<32)|random_word();
            switch((test+ch)%6){case 0:step=1;break;case 1:step=(1ULL<<32)/7;break;
                case 2:step=1ULL<<32;break;case 3:step=(7ULL<<32)+91;break;case 4:step=UINT64_MAX;break;default:break;}
            if(!step)step=1;
            if(segment)assert(pt_voice_init_segment(v+ch,p,5,8,1,7,step,ch&1)==PT_PCM_OK);
            else assert(pt_voice_init(v+ch,p,0,8,(enum pt_voice_loop)loop,loop?2:0,loop?7:0,step,ch&1)==PT_PCM_OK);
            for(i=0;i<(test+ch)%5;++i)scalar(v+ch);
            if(v[ch].active && v[ch].loop!=PT_VOICE_PINGPONG && (random_word()&1))
                assert(pt_voice_set_repeat_source(v+ch,q,0,8)==PT_PCM_OK);
            if(ch==15)memset(v+ch,0,sizeof(*v));
        }
        memcpy(reference,v,sizeof(v));n=random_word()%257;
        for(i=0;i<n;++i)for(ch=0;ch<16;++ch)scalar(reference+ch);
        assert(pt_voice_advance(v,16,n)==PT_PCM_OK && !memcmp(v,reference,sizeof(v)));
    }
#ifdef PT_TEST_ADVANCE_NATIVE
    puts("VOICE PHASE: wide scalar trajectories start");fflush(stdout);
#endif
    /* Near-64-bit range/cycle, preloop/end, exact crossing, huge steps and
     * degenerate pingpong. These are phase-only states; never dereference PCM. */
    for(test=0;test<256;++test) {
        memset(v,0,sizeof(v));v[0].pcm=p;v[0].repeat_pcm=q;v[0].active=1;
        v[0].end=UINT32_MAX;v[0].loop_start=1;v[0].loop_end=UINT32_MAX;
        v[0].loop=(uint8_t)(test%3);v[0].looped=(uint8_t)((test/3)%2);v[0].segment=1;
        v[0].cycle=test&1?UINT64_MAX-1:0;
        v[0].phase=v[0].looped?(test&1?v[0].cycle-1:0):((uint64_t)v[0].end<<32)-(test+1);
        v[0].step=test%4==0?UINT64_MAX:test%4==1?1:test%4==2?test+1:(UINT64_MAX>>1)+test;
        if(!v[0].looped && (test&4))v[0].phase=0;
        if(!v[0].loop){v[0].repeat_pcm=NULL;v[0].phase=test&4?0:((uint64_t)v[0].end<<32)-(test+1);}
        reference[0]=v[0];n=test+1;for(i=0;i<n;++i)scalar(reference);
        assert(pt_voice_advance(v,1,n)==PT_PCM_OK && !memcmp(v,reference,sizeof(v[0])));
    }
    printf("VOICE bulk PASS: %u randomized16voice partitions plus256 wide scalar trajectories; exact frame-reader/handoff tests, no PCM reads\n",cases);
}
int main(void)
{
    int32_t data[8]={1,-257,33,444,51,61,71,81},other[8]={9,19,29,39,49,59,69,79};
    struct pt_pcm p={data,8,8,48000,1,24},q={other,8,8,48000,1,24};
    struct pt_voice v;unsigned loop,i;
    uint64_t steps[]={1,1ULL<<31,1ULL<<32,(7ULL<<32)+91,UINT64_MAX};
#ifdef PT_TEST_ADVANCE_NATIVE
    puts("VOICE PHASE: frame-reader equivalence start");fflush(stdout);
#endif
    for(loop=0;loop<3;++loop)for(i=0;i<5;++i) {
        assert(pt_voice_init(&v,&p,0,8,(enum pt_voice_loop)loop,loop?2:0,loop?7:0,steps[i],1)==PT_PCM_OK);compare(v);
        if(loop!=PT_VOICE_PINGPONG) {assert(pt_voice_set_repeat_source(&v,&q,1,5)==PT_PCM_OK);compare(v);}
        assert(pt_voice_init_segment(&v,&p,5,8,0,1,steps[i],1)==PT_PCM_OK);compare(v);
    }
    bulk_fixture(&p,&q);
    memset(&v,0,sizeof(v));assert(pt_voice_advance(&v,1,256)==PT_PCM_OK);
    assert(pt_voice_advance(NULL,0,256)==PT_PCM_OK && pt_voice_advance(NULL,1,1)==PT_PCM_INVALID);
    /* The mirror must never read sample data: these descriptors deliberately
       have no data. Only initialized state is retained for the phase operation. */
    assert(pt_voice_init(&v,&p,0,8,PT_VOICE_ONCE,0,0,1ULL<<32,0)==PT_PCM_OK);
    p.data=NULL;assert(pt_voice_advance(&v,1,8)==PT_PCM_OK && !v.active);
    puts("VOICE advance PASS: exact phase/handoff equivalence without PCM reads");return 0;
}
