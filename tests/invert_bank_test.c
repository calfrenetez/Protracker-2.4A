#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "invert_bank.h"
struct memory {unsigned calls,fail,live;};
static void *allocate(void *ctx,size_t n){struct memory *m=ctx;void *p;if(++m->calls==m->fail)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void release(void *ctx,void *p){struct memory *m=ctx;assert(m->live);--m->live;free(p);}
static void preparation(void)
{
    struct memory memory={0};struct pt_allocator allocator={&memory,allocate,release};
    struct pt_sample samples[3]={0};uint8_t selected[3]={1,0,1};
    struct pt_invert_bank bank={0};struct pt_invert_bank_job job={0},before;
    int32_t master[2050];unsigned i,phase,ready,steps;size_t exact=3*sizeof(struct pt_invert_pcm)+4100*sizeof(int32_t);
    for(i=0;i<2050;++i)master[i]=(int)(i%256)-128;
    for(i=0;i<3;++i)samples[i].pcm=(struct pt_pcm){master,2050,2050,48000,1,8};
    samples[1].pcm.bits=0; /* Unselected invalid descriptor remains ignored. */
    for(phase=0;phase<9;++phase) {
        assert(pt_invert_bank_begin(&job,&bank,samples,3,selected,exact,&allocator)==PT_INVERT_BANK_OK);
        assert(memory.live==2 && !bank.entries && !bank.storage);
        for(steps=0;steps<phase;++steps) {
            size_t copy=job.copy.copied;unsigned slot=(unsigned)job.slot,copying=job.copy.destination!=NULL;
            ready=77;assert(pt_invert_bank_prepare(&job,&ready)==PT_INVERT_BANK_OK && !ready);
            if(copying) {
                if(slot==job.slot)assert(job.copy.copied-copy<=4096/sizeof(int32_t));
                else assert(samples[slot].pcm.frames-copy<=4096/sizeof(int32_t));
            } else assert(!job.copy.copied);
            assert(!bank.entries && !bank.storage && memory.live==2);
        }
        pt_invert_bank_cancel(&job);pt_invert_bank_cancel(&job);assert(!memory.live && !bank.entries);
    }
    assert(pt_invert_bank_begin(&job,&bank,samples,3,selected,exact,&allocator)==PT_INVERT_BANK_OK);before=job;
    assert(pt_invert_bank_prepare(&job,(unsigned *)master)==PT_INVERT_BANK_INVALID);
    assert(pt_invert_bank_prepare(&job,(unsigned *)&bank)==PT_INVERT_BANK_INVALID);
    assert(pt_invert_bank_prepare(&job,(unsigned *)samples)==PT_INVERT_BANK_INVALID);
    assert(pt_invert_bank_prepare(&job,(unsigned *)job.bank.entries)==PT_INVERT_BANK_INVALID);
    assert(pt_invert_bank_prepare(&job,(unsigned *)job.bank.storage)==PT_INVERT_BANK_INVALID);
    assert(pt_invert_bank_prepare(&job,(unsigned *)&job.slot)==PT_INVERT_BANK_INVALID);
    assert(!memcmp(&job,&before,sizeof(job)) && master[0]==-128 && !bank.entries);
    for(steps=0,ready=0;!ready;++steps){assert(steps<9);assert(pt_invert_bank_prepare(&job,&ready)==PT_INVERT_BANK_OK);}
    assert(steps==9 && memory.live==2 && bank.allocated_bytes==exact);
    assert(!memcmp(bank.entries[0].pcm.data,master,sizeof(master)) && !memcmp(bank.entries[2].pcm.data,master,sizeof(master)));
    pt_invert_bank_cancel(&job);assert(memory.live==2);pt_invert_bank_close(&bank);assert(!memory.live);
    assert(pt_invert_bank_begin(&job,&bank,samples,3,selected,exact,&allocator)==PT_INVERT_BANK_OK);
    assert(pt_invert_bank_prepare(&job,&ready)==PT_INVERT_BANK_OK && !ready);
    samples[0].pcm.rate=44100;
    assert(pt_invert_bank_prepare(&job,&ready)==PT_INVERT_BANK_INVALID && !memory.live && !bank.entries);
    samples[0].pcm.rate=48000;
    {union {struct pt_invert_bank_job job;int32_t data[1024];} alias;
        memset(&alias,0,sizeof(alias));samples[0].pcm=(struct pt_pcm){alias.data,1024,1024,48000,1,8};
        assert(pt_invert_bank_begin(&alias.job,&bank,samples,3,selected,exact,&allocator)==PT_INVERT_BANK_INVALID);
        assert(!alias.data[0] && !memory.live);
        assert(pt_invert_bank_begin(&job,(struct pt_invert_bank *)alias.data,samples,3,selected,exact,&allocator)==PT_INVERT_BANK_INVALID);
        assert(!alias.data[0] && !memory.live);
        assert(pt_invert_bank_begin(&job,&bank,samples,3,(const uint8_t *)&bank,exact,&allocator)==PT_INVERT_BANK_INVALID);
    }
    for(i=0;i<2050;++i)assert(master[i]==(int)(i%256)-128);
}
int main(void)
{
    struct memory m={0};struct pt_allocator a={&m,allocate,release};
    struct pt_sample samples[3];struct pt_invert_bank b={0};
    struct pt_invert_loop clock={0};int32_t pcm[4]={0,1,2,3};uint8_t selected[3]={1,0,1};
    size_t exact=3*sizeof(struct pt_invert_pcm)+8*sizeof(int32_t);unsigned i;
    memset(samples,0,sizeof(samples));
    for(i=0;i<3;++i){samples[i].pcm.data=pcm;samples[i].pcm.capacity=4;samples[i].pcm.frames=4;samples[i].pcm.rate=8000;samples[i].pcm.bits=8;samples[i].pcm.channels=1;}
    samples[1].pcm.bits=24; /* Unselected high precision master is untouched. */
    assert(pt_invert_bank_open(&b,samples,3,selected,exact-1,&a)==PT_INVERT_BANK_BUDGET && !m.calls);
    for(i=1;i<=2;++i){m.calls=0;m.fail=i;assert(pt_invert_bank_open(&b,samples,3,selected,exact,&a)==PT_INVERT_BANK_MEMORY);assert(!m.live && !b.entries && !b.storage);}
    m.fail=0;m.calls=0;
    assert(pt_invert_bank_open(&b,samples,3,selected,exact,&a)==PT_INVERT_BANK_OK);
    assert(b.allocated_bytes==exact && m.live==2 && !b.entries[1].source);
    assert(b.entries[0].source==&samples[0].pcm && b.entries[2].source==&samples[2].pcm);
    assert(pt_invert_bank_open(&b,samples,3,selected,exact,&a)==PT_INVERT_BANK_INVALID);
    assert(pt_invert_loop_bind(&clock,4,0,4) && pt_invert_loop_speed(&clock,15));
    assert(pt_invert_pcm_update(b.entries,&clock)==PT_PCM_OK);
    assert(b.entries[0].pcm.data[1]==-2 && pcm[1]==1 && b.entries[2].pcm.data[1]==1);
    assert(pt_invert_bank_reset(&b)==PT_PCM_OK && b.entries[0].pcm.data[1]==1);
    pt_invert_bank_close(&b);pt_invert_bank_close(&b);assert(!m.live && !b.count);
    selected[1]=1;m.calls=0;
    assert(pt_invert_bank_open(&b,samples,3,selected,exact,&a)==PT_INVERT_BANK_INVALID && !m.calls);
    preparation();return 0;
}
