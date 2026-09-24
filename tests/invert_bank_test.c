#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "invert_bank.h"
struct memory {unsigned calls,fail,live;};
static void *allocate(void *ctx,size_t n){struct memory *m=ctx;void *p;if(++m->calls==m->fail)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void release(void *ctx,void *p){struct memory *m=ctx;assert(m->live);--m->live;free(p);}
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
    return 0;
}
