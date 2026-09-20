#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "voice.h"
#define Q (1ULL<<32)
static void check(struct pt_voice *v,const int32_t *expected,unsigned count)
{
    unsigned i;int32_t out[2];
    for(i=0;i<count;++i) {assert(pt_voice_frame(v,out)==PT_PCM_OK);assert(out[0]==expected[i] && out[1]==expected[i]);}
}
int main(void)
{
    int32_t data[]={0,100,200,300,400,500,600,700};struct pt_pcm p={data,8,8,48000,1,24};struct pt_voice v,before;
    const int32_t earlier[]={500,600,200,300,200,300,200};
    const int32_t later[]={100,200,500,600,500,600};
    const int32_t crossing[]={0,100,200,300,400,500,200,300,200};
    const int32_t blend[]={500,550,600,400,200,250,300,250,200};
    const int32_t overlap_blend[]={100,150,200,200,200,250,300,350,400,450,500,350,200};
    const int32_t single[]={600,700,100,100,100};
    assert(pt_voice_init_segment(&v,&p,5,7,2,4,Q,0)==PT_PCM_OK);check(&v,earlier,7);
    assert(pt_voice_init_segment(&v,&p,1,3,5,7,Q,0)==PT_PCM_OK);check(&v,later,6);
    assert(pt_voice_init_segment(&v,&p,0,6,2,4,Q,0)==PT_PCM_OK);check(&v,crossing,9);
    assert(pt_voice_init_segment(&v,&p,5,7,2,4,Q/2,1)==PT_PCM_OK);check(&v,blend,9);
    assert(pt_voice_init_segment(&v,&p,6,8,1,2,Q,0)==PT_PCM_OK);check(&v,single,5);
    assert(pt_voice_init_segment(&v,&p,1,3,2,6,Q/2,1)==PT_PCM_OK);check(&v,overlap_blend,13);
    before=v;
    assert(pt_voice_init_segment(&v,&p,3,3,1,2,Q,0)==PT_PCM_INVALID && !memcmp(&v,&before,sizeof(v)));
    assert(pt_voice_init_segment(&v,&p,1,9,1,2,Q,0)==PT_PCM_INVALID && !memcmp(&v,&before,sizeof(v)));
    assert(pt_voice_init_segment(&v,&p,1,3,2,2,Q,0)==PT_PCM_INVALID && !memcmp(&v,&before,sizeof(v)));
    assert(pt_voice_init_segment(&v,&p,1,3,2,9,Q,0)==PT_PCM_INVALID && !memcmp(&v,&before,sizeof(v)));
    assert(pt_voice_init_segment(&v,&p,1,3,2,4,0,0)==PT_PCM_INVALID && !memcmp(&v,&before,sizeof(v)));
    /* Independent absolute-travel oracle, including steps near UINT64_MAX. */
    {
        const uint64_t steps[]={1,Q/3,Q,Q*9,UINT64_MAX};unsigned k,i;int32_t out[2];
        for(k=0;k<5;++k) {
            uint64_t phase=0,step=steps[k],cycle=3*Q;unsigned entered=0;uint32_t hash=2166136261UL;
            assert(pt_voice_init_segment(&v,&p,5,7,1,4,step,0)==PT_PCM_OK);
            for(i=0;i<100;++i) {
                unsigned index=entered?1+(unsigned)(phase/Q):5+(unsigned)(phase/Q);
                assert(pt_voice_frame(&v,out)==PT_PCM_OK && out[0]==data[index]);
                hash=(hash^(uint32_t)out[0])*16777619UL;
                if(!entered) {
                    uint64_t remaining=2*Q-phase;
                    if(step<remaining)phase+=step;
                    else {entered=1;phase=(step-remaining)%cycle;}
                } else {uint64_t amount=step%cycle;phase=amount>=cycle-phase?amount-(cycle-phase):phase+amount;}
            }
            printf("SEGMENT trajectory %u hash=%08lx\n",k,(unsigned long)hash);
        }
    }
    puts("VOICE segment PASS: independent initial/repeat ranges, interpolation handoff, huge steps, one-frame repeats and invalid-range preservation");
    return 0;
}
