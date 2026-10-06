#include <stddef.h>
#include "amigus_trigger_levels.h"

static int span(uintptr_t p,size_t bytes)
{return !bytes||(p&&bytes<=UINTPTR_MAX-p);}
static int apart(uintptr_t a,size_t an,uintptr_t b,size_t bn)
{return span(a,an)&&span(b,bn)&&(!an||!bn||a>=b+bn||b>=a+an);}
/* address is numeric: guarding uninitialized local scratch neither reads it
 * nor exposes a const pointer as a writable child result. */
static int known_apart(const struct pt_sample *s,const struct pt_playback_format *f,
    const struct pt_amigus_trigger_levels_request *r,uintptr_t address,size_t bytes)
{
    return apart(address,bytes,(uintptr_t)s,sizeof(*s))&&
        apart(address,bytes,(uintptr_t)f,sizeof(*f))&&
        apart(address,bytes,(uintptr_t)r,sizeof(*r))&&
        apart(address,bytes,(uintptr_t)s->pcm.data,s->pcm.capacity*sizeof(*s->pcm.data))&&
        apart(address,bytes,(uintptr_t)s->slices,(size_t)s->slice_count*sizeof(*s->slices));
}
int pt_amigus_trigger_levels_prepare(const struct pt_sample *s,
    const struct pt_playback_format *f,const struct pt_amigus_trigger_levels_request *r,
    uint32_t address,uint32_t logical_bytes,struct pt_amigus_voice_plan *out)
{
    struct pt_amigus_voice_request geometry;
    struct pt_amigus_voice_plan plan;
    size_t pcm_bytes,slice_bytes;uint64_t values;
    if(!span((uintptr_t)s,sizeof(*s))||(uintptr_t)s%_Alignof(struct pt_sample)||
       !span((uintptr_t)f,sizeof(*f))||(uintptr_t)f%_Alignof(struct pt_playback_format)||
       !span((uintptr_t)r,sizeof(*r))||(uintptr_t)r%_Alignof(struct pt_amigus_trigger_levels_request)||
       !span((uintptr_t)out,sizeof(*out))||(uintptr_t)out%_Alignof(struct pt_amigus_voice_plan)||
       !apart((uintptr_t)s,sizeof(*s),(uintptr_t)f,sizeof(*f))||
       !apart((uintptr_t)s,sizeof(*s),(uintptr_t)r,sizeof(*r))||
       !apart((uintptr_t)f,sizeof(*f),(uintptr_t)r,sizeof(*r)))return 0;
    /* Complete declared capacities, including spare PCM, are guarded before
     * any request field is read or any local legacy scratch is initialized. */
    if(s->pcm.capacity>SIZE_MAX/sizeof(*s->pcm.data)||s->slice_count>PT_PROJECT_SLICES)return 0;
    pcm_bytes=s->pcm.capacity*sizeof(*s->pcm.data);
    slice_bytes=(size_t)s->slice_count*sizeof(*s->slices);
    if(!span((uintptr_t)s->pcm.data,pcm_bytes)||
       (pcm_bytes&&(uintptr_t)s->pcm.data%_Alignof(int32_t))||
       !span((uintptr_t)s->slices,slice_bytes)||
       (slice_bytes&&(uintptr_t)s->slices%_Alignof(uint32_t)))return 0;
    values=(uint64_t)s->pcm.frames*s->pcm.channels;
    if(values>s->pcm.capacity)return 0;
    if(!apart((uintptr_t)s,sizeof(*s),(uintptr_t)s->pcm.data,pcm_bytes)||
       !apart((uintptr_t)f,sizeof(*f),(uintptr_t)s->pcm.data,pcm_bytes)||
       !apart((uintptr_t)r,sizeof(*r),(uintptr_t)s->pcm.data,pcm_bytes)||
       !apart((uintptr_t)s,sizeof(*s),(uintptr_t)s->slices,slice_bytes)||
       !apart((uintptr_t)f,sizeof(*f),(uintptr_t)s->slices,slice_bytes)||
       !apart((uintptr_t)r,sizeof(*r),(uintptr_t)s->slices,slice_bytes)||
       !apart((uintptr_t)s->pcm.data,pcm_bytes,(uintptr_t)s->slices,slice_bytes)||
       !known_apart(s,f,r,(uintptr_t)out,sizeof(*out))||
       !known_apart(s,f,r,(uintptr_t)&geometry,sizeof(geometry))||
       !known_apart(s,f,r,(uintptr_t)&plan,sizeof(plan))||
       !apart((uintptr_t)&geometry,sizeof(geometry),(uintptr_t)out,sizeof(*out))||
       !apart((uintptr_t)&plan,sizeof(plan),(uintptr_t)out,sizeof(*out))||
       !apart((uintptr_t)&geometry,sizeof(geometry),(uintptr_t)&plan,sizeof(plan)))return 0;
    if(r->geometry.volume||r->geometry.pan)return 0;
    geometry=r->geometry;
    if(!pt_amigus_voice_plan_prepare(s,f,&geometry,address,logical_bytes,&plan))return 0;
    plan.left=r->left;plan.right=r->right;
    *out=plan;return 1;
}
