#include "invert_bank.h"
#include <string.h>
void pt_invert_bank_close(struct pt_invert_bank *b)
{
    if(!b)return;
    if(b->storage)b->allocator.release(b->allocator.context,b->storage);
    if(b->entries)b->allocator.release(b->allocator.context,b->entries);
    memset(b,0,sizeof(*b));
}
enum pt_invert_bank_result pt_invert_bank_open(struct pt_invert_bank *out,
    const struct pt_sample *samples,size_t count,const uint8_t *selected,
    size_t budget,const struct pt_allocator *a)
{
    struct pt_invert_bank b;size_t i,values=0,meta,bytes,offset=0;
    if(!out || out->entries || out->storage || out->count || out->allocated_bytes ||
       !samples || !selected || !count || count>PT_PROJECT_SAMPLES ||
       !a || !a->allocate || !a->release)return PT_INVERT_BANK_INVALID;
    meta=count*sizeof(*b.entries);
    for(i=0;i<count;++i) {
        const struct pt_sample *s=samples+i;struct pt_invert_loop loop;
        if(selected[i]>1)return PT_INVERT_BANK_INVALID;
        if(!selected[i])continue;
        memset(&loop,0,sizeof(loop));
        if(pt_pcm_validate(&s->pcm)!=PT_PCM_OK || s->pcm.bits!=8 || s->pcm.channels!=1 ||
           s->pcm.frames<2 || s->pcm.frames>131070 || (s->pcm.frames&1) ||
           s->loop>PT_LOOP_FORWARD || s->interpolation ||
           !pt_invert_loop_bind(&loop,s->pcm.frames,s->loop?s->loop_start:0,s->loop?s->loop_end:2))return PT_INVERT_BANK_INVALID;
        if(s->pcm.frames>SIZE_MAX-values)return PT_INVERT_BANK_BUDGET;
        values+=s->pcm.frames;
    }
    if(values>(SIZE_MAX-meta)/sizeof(int32_t))return PT_INVERT_BANK_BUDGET;
    bytes=values*sizeof(int32_t);
    if(meta+bytes>budget)return PT_INVERT_BANK_BUDGET;
    memset(&b,0,sizeof(b));b.allocator=*a;b.count=count;b.allocated_bytes=meta+bytes;
    b.entries=a->allocate(a->context,meta);
    if(!b.entries)return PT_INVERT_BANK_MEMORY;
    memset(b.entries,0,meta);
    if(bytes) {
        b.storage=a->allocate(a->context,bytes);
        if(!b.storage){pt_invert_bank_close(&b);return PT_INVERT_BANK_MEMORY;}
    }
    for(i=0;i<count;++i)if(selected[i]) {
        if(pt_invert_pcm_init(b.entries+i,&samples[i].pcm,b.storage+offset,samples[i].pcm.frames)!=PT_PCM_OK) {
            pt_invert_bank_close(&b);return PT_INVERT_BANK_INVALID;
        }
        offset+=samples[i].pcm.frames;
    }
    *out=b;return PT_INVERT_BANK_OK;
}
enum pt_pcm_result pt_invert_bank_reset(struct pt_invert_bank *b)
{
    size_t i;enum pt_pcm_result result;
    if(!b || !b->entries || !b->count)return PT_PCM_INVALID;
    for(i=0;i<b->count;++i)if(b->entries[i].source) {
        result=pt_invert_pcm_reset(b->entries+i);if(result!=PT_PCM_OK)return result;
    }
    return PT_PCM_OK;
}
