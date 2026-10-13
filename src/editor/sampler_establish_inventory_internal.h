#ifndef PT_SAMPLER_ESTABLISH_INVENTORY_INTERNAL_H
#define PT_SAMPLER_ESTABLISH_INVENTORY_INTERNAL_H
#include "sampler_establish.h"
#include <string.h>
#include <stddef.h>
#define PT_INVENTORY_ALIGN(t) offsetof(struct {char prefix;t value;},value)
/* Pure owner-thread observation of ONE original establishment and the matched
 * producer's distinct existing-master pin job. No mutation/allocation/callback,
 * outputs, pin/quiet/READY certificate or native/IRQ authority. Every original
 * inventory suffix owner stays living and immutable. This bounded topology and
 * numeric consistency check does not freeze each old node payload byte or detect
 * every valid in-place payload edit. It authenticates only constructor-issued
 * nodes/versions; it never adopts current[] or a supplied count as provenance. */
static int pt_establish_inventory_span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int pt_establish_inventory_pending(const struct pt_sampler_pending *n,
    const struct pt_sampler *s,unsigned kind,const void *control,size_t bytes,
    const struct pt_project *p,unsigned slot,unsigned generation)
{
    unsigned k;
    if(n->owner!=s||n->kind!=kind||n->control!=control||n->bytes!=bytes||
       n->project!=p||n->slot!=slot||n->generation!=generation||
       n->retiring_count>3)return 0;
    for(k=0;k<n->retiring_count;++k)
        if(!pt_establish_inventory_span(n->retiring[k].data,n->retiring[k].bytes)||
           !n->retiring[k].bytes)return 0;
    return 1;
}
static int pt_establish_inventory_header_equal(const struct pt_sampler *s,
    const struct pt_sampler *original)
{
    const unsigned char *a=(const unsigned char *)s,*b=(const unsigned char *)original;
    size_t i;
#define PT_INVENTORY_CHANGED(field) (i>=offsetof(struct pt_sampler,field)&& \
    i-offsetof(struct pt_sampler,field)<sizeof(s->field))
    for(i=0;i<sizeof(*s);++i){
        if(PT_INVENTORY_CHANGED(current)||PT_INVENTORY_CHANGED(bytes)||
           PT_INVENTORY_CHANGED(live_head)||PT_INVENTORY_CHANGED(live_count)||
           PT_INVENTORY_CHANGED(pending_head)||PT_INVENTORY_CHANGED(pending_count))continue;
        if(a[i]!=b[i])return 0;
    }
#undef PT_INVENTORY_CHANGED
    return 1;
}
static int pt_sampler_establish_inventory_current(
    const struct pt_sampler_establish *c,const struct pt_sampler_pin_job *extra,
    unsigned extra_slot,const struct pt_sampler *s,const struct pt_project *p,
    const struct pt_sampler *original)
{
    const struct pt_sampler_live *live,*previous=NULL;
    const struct pt_sampler_pending *pending,*before=NULL;
    size_t i,k,prefix=0,expected=0,charged=0,bytes;unsigned slot;
    if(!c||!extra||!s||!p||!original||s->lifetime_fault||
       original->lifetime_fault||s->live_count>PT_SAMPLER_LIVE_CHECK_LIMIT||
       s->pending_count>PT_SAMPLER_LIVE_CHECK_LIMIT-s->live_count||
       (!s->live_head!=!s->live_count)||(!s->pending_head!=!s->pending_count)||
       (!original->live_head!=!original->live_count)||
       (!original->pending_head!=!original->pending_count)||
       original->live_count>s->live_count||original->pending_count>s->pending_count)
        return 0;
    if(!c->self){
        return !c->active&&!c->sampler&&!c->metadata&&!c->job.owner&&
            !c->reservation.owner&&!extra->owner&&!extra->reservation.owner&&
            !memcmp(s,original,sizeof(*s));
    }
    if(c->self!=c||c->sampler!=s||c->project!=p||
       c->generation!=original->generation||c->faulted||
       c->result==PT_ESTABLISH_INVALID||c->result==PT_ESTABLISH_CAPACITY||
       c->result==PT_ESTABLISH_STALE||c->result==PT_ESTABLISH_SEMANTIC||
       c->result==PT_ESTABLISH_ALIAS||c->result==PT_ESTABLISH_FAULT||
       c->slot>p->sample_count||p->sample_count>PT_PROJECT_SAMPLES||
       !pt_sampler_pin_job_inventory_current(&c->job,s,p,c->generation,c->slot)||
       !pt_sampler_pin_job_inventory_current(extra,s,p,c->generation,extra_slot)||
       (extra->reservation.owner&&(c->result!=PT_ESTABLISH_READY||
        extra->value!=extra->previous)))return 0;
    for(slot=0;slot<PT_PROJECT_SAMPLES;++slot){
        if(original->current[slot]){
            if(s->current[slot]!=original->current[slot]||c->issued[slot])return 0;
        }else if(s->current[slot]){
            if(slot>=c->slot||s->current[slot]!=c->issued[slot])return 0;
            ++expected;
        }else if(c->issued[slot])return 0;
    }
    if(c->metadata){
        if(!c->active||c->retiring||c->metadata_live!=(const void *)c->metadata||
           c->metadata_extent.data!=c->metadata||
           c->metadata_extent.bytes!=c->metadata_bytes||
           !pt_establish_inventory_span(c->metadata,c->metadata_bytes)||
           !c->metadata_bytes)return 0;
        ++expected;
    }else if(c->metadata_bytes||c->metadata_live)return 0;
    if(c->job.value&&!c->job.previous)++expected;
    /* All newly inserted nodes are a genuine owned prefix. Existing suffix
     * head/count is unchanged, with complete bounded links/numeric checks. */
    live=s->live_head;
    while(live!=original->live_head){
        int known=0;
        if(!live||!pt_establish_inventory_span(live,sizeof(*live))||
           (uintptr_t)live%PT_INVENTORY_ALIGN(struct pt_sampler_live)||
           prefix>=expected||live->owner!=s||live->previous!=previous||
           !live->count||live->count>3)return 0;
        if(live==c->metadata_live){
            known=live->kind==PT_LIVE_ESTABLISH&&live->count==1&&
                live->spans[0].data==c->metadata_extent.data&&
                live->spans[0].bytes==c->metadata_extent.bytes;
        }else{
            bytes=0;
            for(slot=0;slot<PT_PROJECT_SAMPLES;++slot)
                if(c->issued[slot]&&s->current[slot]==c->issued[slot]){
                    bytes=pt_sampler_version_inventory_bytes(c->issued[slot],s,live);
                    if(bytes)break;
                }
            if(!bytes&&c->job.value&&!c->job.previous)
                bytes=pt_sampler_version_inventory_bytes(c->job.value,s,live);
            if(bytes){if(bytes>SIZE_MAX-charged)return 0;charged+=bytes;known=1;}
        }
        if(!known)return 0;
        previous=live;live=live->next;++prefix;
    }
    if(prefix!=expected||s->live_count-original->live_count!=prefix)return 0;
    for(i=0;i<original->live_count;++i){
        if(!live||!pt_establish_inventory_span(live,sizeof(*live))||
           (uintptr_t)live%PT_INVENTORY_ALIGN(struct pt_sampler_live)||live->owner!=s||live->previous!=previous||!live->count||
           live->count>3||!live->kind||live->kind>PT_LIVE_ESTABLISH)return 0;
        for(k=0;k<live->count;++k)
            if(!live->spans[k].bytes||!pt_establish_inventory_span(
                live->spans[k].data,live->spans[k].bytes))return 0;
        previous=live;live=live->next;
    }
    if(live||charged>SIZE_MAX-original->bytes||
       s->bytes!=original->bytes+charged||s->bytes>s->budget)return 0;
    pending=s->pending_head;prefix=0;expected=0;
    if(c->reservation.owner)++expected;
    if(c->job.reservation.owner)++expected;
    if(extra->reservation.owner)++expected;
    if(c->retirement.owner)++expected;
    while(pending!=original->pending_head){
        int known=0;
        if(!pending||!pt_establish_inventory_span(pending,sizeof(*pending))||
           (uintptr_t)pending%PT_INVENTORY_ALIGN(struct pt_sampler_pending)||
           prefix>=expected||pending->owner!=s||
           pending->previous!=before)return 0;
        if(pending==&c->reservation){
            known=c->active&&pt_establish_inventory_pending(pending,s,
                PT_PENDING_ESTABLISH,c,sizeof(*c),p,0,c->generation)&&
                pending->retiring_count==(c->metadata_extent.bytes?1U:0U)+
                    (c->master_extent.bytes?1U:0U);
            if(known&&c->metadata_extent.bytes)
                known=pending->retiring[0].data==c->metadata_extent.data&&
                    pending->retiring[0].bytes==c->metadata_extent.bytes;
            if(known&&c->master_extent.bytes)
                known=pending->retiring[1].data==c->master_extent.data&&
                    pending->retiring[1].bytes==c->master_extent.bytes;
        }else if(pending==&c->job.reservation)known=1;
        else if(pending==&extra->reservation)known=1;
        else if(pending==&c->retirement){
            known=c->active&&c->busy&&c->retiring&&!c->metadata&&
                pt_establish_inventory_pending(pending,s,PT_PENDING_CALL,
                &c->retirement,sizeof(c->retirement),NULL,0,c->generation)&&
                pending->retiring_count==1&&
                pending->retiring[0].data==c->metadata_extent.data&&
                pending->retiring[0].bytes==c->metadata_extent.bytes;
        }
        if(!known)return 0;
        before=pending;pending=pending->next;++prefix;
    }
    if(prefix!=expected||s->pending_count-original->pending_count!=prefix||
       (!!c->reservation.owner)!=!!c->active||
       (!c->active&&(c->job.owner||c->job.reservation.owner||c->retirement.owner)))
        return 0;
    for(i=0;i<original->pending_count;++i){
        if(!pending||!pt_establish_inventory_span(pending,sizeof(*pending))||
           (uintptr_t)pending%PT_INVENTORY_ALIGN(struct pt_sampler_pending)||pending->owner!=s||pending->previous!=before||
           !pending->kind||pending->kind>PT_PENDING_CALL||!pending->bytes||
           !pt_establish_inventory_span(pending->control,pending->bytes)||
           pending->retiring_count>3)return 0;
        for(k=0;k<pending->retiring_count;++k)
            if(!pending->retiring[k].bytes||!pt_establish_inventory_span(
                pending->retiring[k].data,pending->retiring[k].bytes))return 0;
        before=pending;pending=pending->next;
    }
    if(pending)return 0;
    return pt_establish_inventory_header_equal(s,original);
}
#undef PT_INVENTORY_ALIGN
#endif
