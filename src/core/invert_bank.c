#include "invert_bank.h"
#include "pcm_internal.h"
#include <string.h>
static int overlaps(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!an || !bn)return 0;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return 1;
    return x<y+bn && y<x+an;
}
void pt_invert_bank_close(struct pt_invert_bank *b)
{
    if(!b)return;
    if(b->storage)b->allocator.release(b->allocator.context,b->storage);
    if(b->entries)b->allocator.release(b->allocator.context,b->entries);
    memset(b,0,sizeof(*b));
}
void pt_invert_bank_cancel(struct pt_invert_bank_job *j)
{
    if(!j)return;
    pt_invert_pcm_cancel(&j->copy);pt_invert_bank_close(&j->bank);memset(j,0,sizeof(*j));
}
enum pt_invert_bank_result pt_invert_bank_begin(struct pt_invert_bank_job *j,struct pt_invert_bank *out,
    const struct pt_sample *samples,size_t count,const uint8_t *selected,
    size_t budget,const struct pt_allocator *a)
{
    struct pt_invert_bank b;size_t i,values=0,meta,bytes;
    if(!j || j->destination || j->bank.entries || j->bank.storage || !out ||
       out->entries || out->storage || out->count || out->allocated_bytes ||
       !samples || !selected || !count || count>PT_PROJECT_SAMPLES ||
       !a || !a->allocate || !a->release)return PT_INVERT_BANK_INVALID;
    if(overlaps(j,sizeof(*j),out,sizeof(*out)) || overlaps(j,sizeof(*j),samples,count*sizeof(*samples)) ||
       overlaps(j,sizeof(*j),selected,count) || overlaps(j,sizeof(*j),a,sizeof(*a)) ||
       overlaps(out,sizeof(*out),samples,count*sizeof(*samples)) ||
       overlaps(out,sizeof(*out),selected,count) || overlaps(out,sizeof(*out),a,sizeof(*a)))return PT_INVERT_BANK_INVALID;
    meta=count*sizeof(*b.entries);
    for(i=0;i<count;++i) {
        const struct pt_sample *s=samples+i;struct pt_invert_loop loop;
        if(selected[i]>1)return PT_INVERT_BANK_INVALID;
        /* Unselected slots keep the old validation semantics. Recognized
         * geometry permits an alias check without validating their values. */
        if(pt_pcm_shape(&s->pcm)==PT_PCM_OK &&
           (overlaps(j,sizeof(*j),s->pcm.data,(size_t)s->pcm.frames*s->pcm.channels*sizeof(int32_t)) ||
            overlaps(out,sizeof(*out),s->pcm.data,(size_t)s->pcm.frames*s->pcm.channels*sizeof(int32_t))))return PT_INVERT_BANK_INVALID;
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
        /* Keep the validated original descriptor until this entry is copied.
         * It also identifies selected slots without another metadata array. */
        b.entries[i].source=&samples[i].pcm;b.entries[i].pcm=samples[i].pcm;
    }
    memset(j,0,sizeof(*j));j->bank=b;j->destination=out;j->samples=samples;return PT_INVERT_BANK_OK;
}
enum pt_invert_bank_result pt_invert_bank_prepare(struct pt_invert_bank_job *j,unsigned *ready)
{
    struct pt_invert_pcm *entry;enum pt_pcm_result result;unsigned copied;size_t i;
    if(!j || !j->destination || !ready)return PT_INVERT_BANK_INVALID;
    if(overlaps(ready,sizeof(*ready),j,sizeof(*j)) ||
       overlaps(ready,sizeof(*ready),j->destination,sizeof(*j->destination)) ||
       overlaps(ready,sizeof(*ready),j->samples,j->bank.count*sizeof(*j->samples)) ||
       overlaps(ready,sizeof(*ready),j->bank.entries,j->bank.count*sizeof(*j->bank.entries)) ||
       overlaps(ready,sizeof(*ready),j->bank.storage,j->bank.allocated_bytes-j->bank.count*sizeof(*j->bank.entries)))return PT_INVERT_BANK_INVALID;
    for(i=0;i<j->bank.count;++i) {
        const struct pt_pcm *p=&j->samples[i].pcm;
        if(pt_pcm_shape(p)==PT_PCM_OK &&
           overlaps(ready,sizeof(*ready),p->data,(size_t)p->frames*p->channels*sizeof(int32_t)))return PT_INVERT_BANK_INVALID;
    }
    *ready=0;
    if(j->copy.destination) {
        result=pt_invert_pcm_prepare(&j->copy,&copied);
        if(result!=PT_PCM_OK)goto fail;
        if(copied){j->offset+=j->bank.entries[j->slot].pcm.frames;++j->slot;pt_invert_pcm_cancel(&j->copy);}
        return PT_INVERT_BANK_OK;
    }
    while(j->slot<j->bank.count && !j->bank.entries[j->slot].source)++j->slot;
    if(j->slot==j->bank.count) {
        if(j->destination->entries || j->destination->storage || j->destination->count || j->destination->allocated_bytes)goto fail;
        *j->destination=j->bank;memset(&j->bank,0,sizeof(j->bank));memset(j,0,sizeof(*j));*ready=1;
        return PT_INVERT_BANK_OK;
    }
    entry=j->bank.entries+j->slot;
    if(entry->source->data!=entry->pcm.data || entry->source->capacity!=entry->pcm.capacity ||
       entry->source->frames!=entry->pcm.frames || entry->source->rate!=entry->pcm.rate ||
       entry->source->channels!=entry->pcm.channels || entry->source->bits!=entry->pcm.bits)goto fail;
    result=pt_invert_pcm_begin_prepared(&j->copy,entry,entry->source,j->bank.storage+j->offset,entry->pcm.frames);
    if(result==PT_PCM_OK)return PT_INVERT_BANK_OK;
fail:
    pt_invert_bank_cancel(j);return PT_INVERT_BANK_INVALID;
}
enum pt_invert_bank_result pt_invert_bank_open(struct pt_invert_bank *out,
    const struct pt_sample *samples,size_t count,const uint8_t *selected,
    size_t budget,const struct pt_allocator *a)
{
    struct pt_invert_bank_job j={0};unsigned ready=0;
    enum pt_invert_bank_result result=pt_invert_bank_begin(&j,out,samples,count,selected,budget,a);
    while(result==PT_INVERT_BANK_OK && !ready)result=pt_invert_bank_prepare(&j,&ready);
    pt_invert_bank_cancel(&j);return result;
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
