#include "invert_pcm.h"
#include "pcm_internal.h"
#include <string.h>
static int overlaps(const void *a,size_t na,const void *b,size_t nb)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!na || !nb)return 0;
    if(na>UINTPTR_MAX-x || nb>UINTPTR_MAX-y)return 1;
    return x<y+nb && y<x+na;
}
static enum pt_pcm_result begin(struct pt_invert_pcm_job *j,struct pt_invert_pcm *w,
    const struct pt_pcm *s,int32_t *data,size_t capacity,unsigned prepared)
{
    struct pt_invert_pcm next;size_t bytes;enum pt_pcm_result result;
    if(!j || j->destination || !w || !s || !data)return PT_PCM_INVALID;
    result=prepared?pt_pcm_shape(s):pt_pcm_validate(s);if(result!=PT_PCM_OK)return result;
    if(s->bits!=8 || s->channels!=1 || s->frames<2 || s->frames>131070 || (s->frames&1))return PT_PCM_INVALID;
    if(capacity<s->frames)return PT_PCM_CAPACITY;
    bytes=(size_t)s->frames*sizeof(*data);
    if(overlaps(data,bytes,s->data,bytes) || overlaps(w,sizeof(*w),s,sizeof(*s)) ||
       overlaps(w,sizeof(*w),s->data,bytes) || overlaps(w,sizeof(*w),data,bytes) ||
       overlaps(data,bytes,s,sizeof(*s)) || overlaps(j,sizeof(*j),w,sizeof(*w)) ||
       overlaps(j,sizeof(*j),s,sizeof(*s)) || overlaps(j,sizeof(*j),s->data,bytes) ||
       overlaps(j,sizeof(*j),data,bytes))return PT_PCM_ALIAS;
    memset(&next,0,sizeof(next));next.source=s;next.pcm=*s;next.pcm.data=data;next.pcm.capacity=capacity;
    memset(j,0,sizeof(*j));j->value=next;j->source=*s;j->destination=w;return PT_PCM_OK;
}
enum pt_pcm_result pt_invert_pcm_begin(struct pt_invert_pcm_job *j,struct pt_invert_pcm *w,
    const struct pt_pcm *s,int32_t *data,size_t capacity)
{return begin(j,w,s,data,capacity,0);}
enum pt_pcm_result pt_invert_pcm_begin_prepared(struct pt_invert_pcm_job *j,struct pt_invert_pcm *w,
    const struct pt_pcm *s,int32_t *data,size_t capacity)
{return begin(j,w,s,data,capacity,1);}
enum pt_pcm_result pt_invert_pcm_prepare(struct pt_invert_pcm_job *j,unsigned *ready)
{
    const struct pt_pcm *p,*q;size_t n;
    if(!j || !j->destination || !ready)return PT_PCM_INVALID;
    n=(size_t)j->source.frames*sizeof(int32_t);
    if(overlaps(ready,sizeof(*ready),j,sizeof(*j)) ||
       overlaps(ready,sizeof(*ready),j->destination,sizeof(*j->destination)) ||
       overlaps(ready,sizeof(*ready),j->value.source,sizeof(*j->value.source)) ||
       overlaps(ready,sizeof(*ready),j->source.data,n) ||
       overlaps(ready,sizeof(*ready),j->value.pcm.data,n))return PT_PCM_ALIAS;
    *ready=0;if(j->failure)return j->failure;
    if(j->complete){*ready=1;return PT_PCM_OK;}
    p=j->value.source;q=&j->source;
    if(p->data!=q->data || p->capacity!=q->capacity || p->frames!=q->frames ||
       p->rate!=q->rate || p->channels!=q->channels || p->bits!=q->bits) {
        j->failure=PT_PCM_INVALID;return j->failure;
    }
    n=q->frames-j->copied;if(n>PT_INVERT_PCM_COPY_MAX/sizeof(int32_t))n=PT_INVERT_PCM_COPY_MAX/sizeof(int32_t);
    memcpy(j->value.pcm.data+j->copied,q->data+j->copied,n*sizeof(int32_t));j->copied+=n;
    if(j->copied==q->frames){*j->destination=j->value;j->complete=1;*ready=1;}
    return PT_PCM_OK;
}
void pt_invert_pcm_cancel(struct pt_invert_pcm_job *j)
{if(j)memset(j,0,sizeof(*j));}
enum pt_pcm_result pt_invert_pcm_init(struct pt_invert_pcm *w,const struct pt_pcm *s,int32_t *data,size_t capacity)
{
    struct pt_invert_pcm_job j={0};unsigned ready=0;
    enum pt_pcm_result result=pt_invert_pcm_begin(&j,w,s,data,capacity);
    while(result==PT_PCM_OK && !ready)result=pt_invert_pcm_prepare(&j,&ready);
    pt_invert_pcm_cancel(&j);return result;
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
