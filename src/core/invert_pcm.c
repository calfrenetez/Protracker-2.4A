#include "invert_pcm.h"
#include <string.h>
static int overlaps(const void *a,size_t na,const void *b,size_t nb)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!na || !nb)return 0;
    if(na>UINTPTR_MAX-x || nb>UINTPTR_MAX-y)return 1;
    return x<y+nb && y<x+na;
}
enum pt_pcm_result pt_invert_pcm_init(struct pt_invert_pcm *w,const struct pt_pcm *s,int32_t *data,size_t capacity)
{
    struct pt_invert_pcm next;size_t bytes;enum pt_pcm_result result;
    if(!w || !s || !data)return PT_PCM_INVALID;
    result=pt_pcm_validate(s);if(result!=PT_PCM_OK)return result;
    if(s->bits!=8 || s->channels!=1 || s->frames<2 || s->frames>131070 || (s->frames&1))return PT_PCM_INVALID;
    if(capacity<s->frames)return PT_PCM_CAPACITY;
    bytes=(size_t)s->frames*sizeof(*data);
    if(overlaps(data,bytes,s->data,bytes) || overlaps(w,sizeof(*w),s,sizeof(*s)) ||
       overlaps(w,sizeof(*w),s->data,bytes) || overlaps(w,sizeof(*w),data,bytes) ||
       overlaps(data,bytes,s,sizeof(*s)))return PT_PCM_ALIAS;
    memset(&next,0,sizeof(next));next.source=s;next.pcm=*s;next.pcm.data=data;next.pcm.capacity=capacity;
    memcpy(data,s->data,bytes);*w=next;return PT_PCM_OK;
}
enum pt_pcm_result pt_invert_pcm_reset(struct pt_invert_pcm *w)
{
    if(!w)return PT_PCM_INVALID;
    return pt_invert_pcm_init(w,w->source,w->pcm.data,w->pcm.capacity);
}
enum pt_pcm_result pt_invert_pcm_update(struct pt_invert_pcm *w,struct pt_invert_loop *s)
{
    uint32_t index;
    if(!w || !s || !w->source || !w->pcm.data || s->speed>15 || s->bound>1 ||
       (s->bound && (s->start>=s->end || s->end>w->pcm.frames || ((s->start|s->end)&1) || s->cursor<s->start || s->cursor>=s->end)))return PT_PCM_INVALID;
    if(overlaps(s,sizeof(*s),w,sizeof(*w)) || overlaps(s,sizeof(*s),w->pcm.data,(size_t)w->pcm.frames*sizeof(int32_t)) ||
       overlaps(s,sizeof(*s),w->source,sizeof(*w->source)) || overlaps(s,sizeof(*s),w->source->data,(size_t)w->source->frames*sizeof(int32_t)))return PT_PCM_ALIAS;
    if(pt_invert_loop_update(s,&index))w->pcm.data[index]=-1-w->pcm.data[index];
    return PT_PCM_OK;
}
