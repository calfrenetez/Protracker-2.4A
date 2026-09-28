#include "capture.h"
#include <string.h>
enum pt_capture_result pt_capture_open(struct pt_capture *c,const struct pt_allocator *a,unsigned bits,unsigned channels,uint32_t rate,uint32_t frames,size_t budget)
{
    int32_t *data;size_t count,bytes;
    if(!c || c->pcm.data || c->active || c->finished || c->failed || !a || !a->allocate || !a->release ||
       (bits!=8 && bits!=16 && bits!=24) || (channels!=1 && channels!=2) || !rate || rate>192000 || !frames)
        return PT_CAPTURE_INVALID;
    if(frames>SIZE_MAX/sizeof(int32_t)/channels)return PT_CAPTURE_CAPACITY;
    count=(size_t)frames*channels;bytes=count*sizeof(int32_t);
    if(bytes>budget)return PT_CAPTURE_CAPACITY;
    data=a->allocate(a->context,bytes);if(!data)return PT_CAPTURE_CAPACITY;
    c->allocator=*a;c->pcm=(struct pt_pcm){data,count,0,rate,(uint8_t)channels,(uint8_t)bits};
    c->limit=frames;c->bytes=bytes;c->active=1;return PT_CAPTURE_OK;
}
enum pt_capture_result pt_capture_append(struct pt_capture *c,const struct pt_pcm *p)
{
    size_t count;
    if(!c || !c->active)return PT_CAPTURE_INVALID;
    if(c->failed)return (enum pt_capture_result)c->failed;
    if(!p || !p->frames || p->frames>256 || p->bits!=c->pcm.bits || p->channels!=c->pcm.channels ||
       p->rate!=c->pcm.rate || pt_pcm_validate(p)!=PT_PCM_OK) {c->failed=PT_CAPTURE_INVALID;return PT_CAPTURE_INVALID;}
    if(p->frames>c->limit-c->pcm.frames) {c->failed=PT_CAPTURE_OVERRUN;return PT_CAPTURE_OVERRUN;}
    count=(size_t)p->frames*p->channels;
    memmove(c->pcm.data+(size_t)c->pcm.frames*p->channels,p->data,count*sizeof(int32_t));
    c->pcm.frames+=p->frames;return PT_CAPTURE_OK;
}
void pt_capture_overrun(struct pt_capture *c) {if(c && c->active)c->failed=PT_CAPTURE_OVERRUN;}
enum pt_capture_result pt_capture_finish(struct pt_capture *c)
{
    if(!c || !c->pcm.data)return PT_CAPTURE_INVALID;
    if(c->failed)return (enum pt_capture_result)c->failed;
    c->active=0;c->finished=1;return PT_CAPTURE_OK;
}
const struct pt_pcm *pt_capture_pcm(const struct pt_capture *c)
{return c && c->finished && !c->active && !c->failed && c->pcm.frames?&c->pcm:NULL;}
void pt_capture_close(struct pt_capture *c)
{
    if(!c)return;
    if(c->pcm.data)c->allocator.release(c->allocator.context,c->pcm.data);
    memset(c,0,sizeof(*c));
}
