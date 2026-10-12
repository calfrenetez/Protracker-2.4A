#include "paula_segment_repeat_plan.h"
#include <stddef.h>
#include <string.h>
#define ALIGNMENT(T) offsetof(struct {char before;T object;},object)
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int object(const void *p,size_t n,size_t alignment)
{return p&&span(p,n)&&!((uintptr_t)p%alignment);}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int pcm_same(const struct pt_pcm *a,const struct pt_pcm *b)
{return a->data==b->data&&a->capacity==b->capacity&&a->frames==b->frames&&a->rate==b->rate&&a->channels==b->channels&&a->bits==b->bits;}
static int source_same(const struct pt_paula_segment_source *a,const struct pt_paula_segment_source *b)
{
    unsigned i;
    if(a->descriptor!=b->descriptor||!pcm_same(&a->master,&b->master)||a->version!=b->version||
       a->sampler_owner!=b->sampler_owner||a->project_identity!=b->project_identity||
       a->cache_owner!=b->cache_owner||a->cache_version!=b->cache_version||a->generation!=b->generation||
       a->sample!=b->sample||a->lease.slot!=b->lease.slot||a->lease.serial!=b->lease.serial||
       a->cache_data!=b->cache_data||a->cache_bytes!=b->cache_bytes||a->guard_count!=b->guard_count)return 0;
    for(i=0;i<a->guard_count;++i)if(a->guard[i].data!=b->guard[i].data||a->guard[i].bytes!=b->guard[i].bytes)return 0;
    return 1;
}
/* Snapshot shape only. Do not follow opaque versions/owners or read PCM/cache. */
static int source_shape(const struct pt_paula_segment_source *s)
{
    const struct pt_pcm *p=&s->master;size_t cache_bytes;unsigned i;
    if(!s->version||!s->sampler_owner||!s->project_identity||!s->cache_owner||!s->cache_version||s->sample>=255||
       s->lease.slot>=PT_CACHE_SLOTS||!s->lease.serial||!s->descriptor||
       !object(s->descriptor,sizeof(*s->descriptor),ALIGNMENT(struct pt_pcm))||
       p->channels!=1||(p->bits!=8&&p->bits!=16&&p->bits!=24)||!p->rate||p->rate>192000||
       !p->frames||p->capacity<p->frames||p->capacity>SIZE_MAX/sizeof(*p->data)||
       !object(p->data,p->capacity*sizeof(*p->data),ALIGNMENT(int32_t))||
       s->guard_count>PT_PAULA_SEGMENT_SOURCE_GUARDS)return 0;
    cache_bytes=(size_t)p->frames;if(cache_bytes&1){if(cache_bytes==SIZE_MAX)return 0;++cache_bytes;}
    if(s->cache_bytes!=cache_bytes||!object(s->cache_data,s->cache_bytes,2))return 0;
    for(i=0;i<s->guard_count;++i)if(!s->guard[i].bytes||!span(s->guard[i].data,s->guard[i].bytes))return 0;
    return 1;
}
static int source_output(const struct pt_paula_segment_source *s,const void *out,size_t bytes)
{
    unsigned i;
    if(!source_shape(s)||!apart(out,bytes,s->descriptor,sizeof(*s->descriptor))||
       !apart(out,bytes,s->master.data,s->master.capacity*sizeof(*s->master.data))||
       !apart(out,bytes,s->cache_data,s->cache_bytes))return 0;
    for(i=0;i<s->guard_count;++i)if(!apart(out,bytes,s->guard[i].data,s->guard[i].bytes))return 0;
    return 1;
}
static int range(uint32_t a,uint32_t b,const struct pt_paula_segment_source *s,struct pt_paula_segment_range *out)
{
    uint32_t length;
    if(a>=b||b>s->master.frames||(a&1))return 0;
    length=b-a;if((length&1)||length<2||length>131070)return 0;
    if((size_t)a>s->cache_bytes||(size_t)length>s->cache_bytes-a)return 0;
    out->frame_offset=a;out->frame_length=length;out->byte_offset=a;out->byte_length=length;
    out->words=(uint16_t)(length/2);return 1;
}
enum pt_paula_segment_result pt_paula_segment_plan_build(const struct pt_render_action *action,
    const struct pt_paula_segment_source *current,const struct pt_paula_segment_source *next,
    uint64_t frame,unsigned output_rate,unsigned slot,const struct pt_paula_render_caps *caps,
    struct pt_paula_segment_plan *out)
{
    struct pt_render_action a;struct pt_paula_segment_source c,n;struct pt_paula_render_caps cap;
    struct pt_paula_segment_plan p;struct pt_pcm actual;const struct pt_voice *v;unsigned same;
    if(!object(action,sizeof(*action),ALIGNMENT(struct pt_render_action))||
       !object(current,sizeof(*current),ALIGNMENT(struct pt_paula_segment_source))||
       !object(next,sizeof(*next),ALIGNMENT(struct pt_paula_segment_source))||
       !object(caps,sizeof(*caps),ALIGNMENT(struct pt_paula_render_caps))||
       !object(out,sizeof(*out),ALIGNMENT(struct pt_paula_segment_plan))||
       !apart(out,sizeof(*out),action,sizeof(*action))||!apart(out,sizeof(*out),current,sizeof(*current))||
       !apart(out,sizeof(*out),next,sizeof(*next))||!apart(out,sizeof(*out),caps,sizeof(*caps)))return PT_PAULA_SEGMENT_ALIAS;
    memcpy(&a,action,sizeof(a));memcpy(&c,current,sizeof(c));memcpy(&n,next,sizeof(n));memcpy(&cap,caps,sizeof(cap));
    if(!source_shape(&c)||!source_shape(&n))return PT_PAULA_SEGMENT_INVALID;
    if(!source_output(&c,out,sizeof(*out))||!source_output(&n,out,sizeof(*out)))return PT_PAULA_SEGMENT_ALIAS;
    memcpy(&actual,c.descriptor,sizeof(actual));if(!pcm_same(&actual,&c.master))return PT_PAULA_SEGMENT_STALE;
    memcpy(&actual,n.descriptor,sizeof(actual));if(!pcm_same(&actual,&n.master))return PT_PAULA_SEGMENT_STALE;
    if(c.sampler_owner!=n.sampler_owner||c.project_identity!=n.project_identity||c.cache_owner!=n.cache_owner||
       c.generation!=n.generation||c.cache_version!=n.cache_version)return PT_PAULA_SEGMENT_STALE;
    if(frame==UINT64_MAX||a.channel>=16||slot>=4||(output_rate!=44100&&output_rate!=48000)||
       !pt_paula_render_caps_valid(&cap))return PT_PAULA_SEGMENT_INVALID;
    if(a.kind!=PT_RENDER_SEGMENT&&a.kind!=PT_RENDER_REPEAT)return PT_PAULA_SEGMENT_UNSUPPORTED;
    v=&a.voice;same=(unsigned)source_same(&c,&n);
    if(v->pcm!=c.descriptor||v->active!=1||v->linear||v->segment!=1||v->loop!=PT_VOICE_FORWARD||v->looped||
       v->start>=v->end||v->end>c.master.frames||v->phase<((uint64_t)v->start<<32)||
       v->phase>=((uint64_t)v->end<<32)||v->loop_start>=v->loop_end||
       v->loop_end>n.master.frames||v->cycle!=((uint64_t)(v->loop_end-v->loop_start)<<32))return PT_PAULA_SEGMENT_UNSUPPORTED;
    if(a.kind==PT_RENDER_SEGMENT){
        if(v->repeat_pcm||!same||v->phase!=((uint64_t)v->start<<32))return PT_PAULA_SEGMENT_UNSUPPORTED;
    }else if(v->repeat_pcm!=n.descriptor||c.master.bits!=n.master.bits||c.master.rate!=n.master.rate||
             (!same&&(c.master.bits!=8||n.master.bits!=8)))return PT_PAULA_SEGMENT_UNSUPPORTED;
    memset(&p,0,sizeof(p));p.kind=a.kind==PT_RENDER_SEGMENT?PT_PAULA_SEGMENT_INITIAL:PT_PAULA_SEGMENT_REPROGRAM;
    p.frame=frame;p.phase_q32=v->phase;p.track=a.channel;p.slot=slot;p.source_count=same?1U:2U;
    p.write_initial=a.kind==PT_RENDER_SEGMENT;p.write_repeat=1;
    if(!range(v->start,v->end,&c,&p.initial)||!range(v->loop_start,v->loop_end,&n,&p.repeat)||
       !pt_paula_render_control(v->step,output_rate,a.gain,slot,&cap,&p.period,&p.volume))return PT_PAULA_SEGMENT_UNSUPPORTED;
    p.source[0]=c;if(!same)p.source[1]=n;*out=p;return PT_PAULA_SEGMENT_OK;
}
int pt_paula_segment_plan_identity_equal(const struct pt_paula_segment_plan *plan,
    const struct pt_paula_segment_source *current,const struct pt_paula_segment_source *next)
{
    struct pt_paula_segment_plan p;struct pt_paula_segment_source c,n;
    if(!object(plan,sizeof(*plan),ALIGNMENT(struct pt_paula_segment_plan))||
       !object(current,sizeof(*current),ALIGNMENT(struct pt_paula_segment_source))||
       !object(next,sizeof(*next),ALIGNMENT(struct pt_paula_segment_source)))return 0;
    memcpy(&p,plan,sizeof(p));memcpy(&c,current,sizeof(c));memcpy(&n,next,sizeof(n));
    if((p.source_count!=1&&p.source_count!=2)||!source_shape(&c)||!source_shape(&n)||
       !source_shape(p.source)||!source_same(p.source,&c))return 0;
    return p.source_count==1?source_same(&c,&n):source_shape(p.source+1)&&source_same(p.source+1,&n);
}
enum pt_paula_segment_result pt_paula_segment_plan_stop(const struct pt_paula_segment_plan *plan,
    uint64_t frame,struct pt_paula_segment_plan *out)
{
    struct pt_paula_segment_plan p;unsigned i;
    if(!object(plan,sizeof(*plan),ALIGNMENT(struct pt_paula_segment_plan))||
       !object(out,sizeof(*out),ALIGNMENT(struct pt_paula_segment_plan))||
       !apart(out,sizeof(*out),plan,sizeof(*plan)))return PT_PAULA_SEGMENT_ALIAS;
    memcpy(&p,plan,sizeof(p));
    if((p.kind!=PT_PAULA_SEGMENT_INITIAL&&p.kind!=PT_PAULA_SEGMENT_REPROGRAM&&p.kind!=PT_PAULA_SEGMENT_STOP_REQUIREMENTS)||
       (p.source_count!=1&&p.source_count!=2)||p.track>=16||p.slot>=4||frame==UINT64_MAX||frame<p.frame)return PT_PAULA_SEGMENT_INVALID;
    for(i=0;i<p.source_count;++i)if(!source_output(p.source+i,out,sizeof(*out)))return PT_PAULA_SEGMENT_ALIAS;
    p.kind=PT_PAULA_SEGMENT_STOP_REQUIREMENTS;p.frame=frame;p.write_initial=p.write_repeat=0;p.period=0;p.volume=0;
    memset(&p.initial,0,sizeof(p.initial));memset(&p.repeat,0,sizeof(p.repeat));*out=p;return PT_PAULA_SEGMENT_OK;
}
