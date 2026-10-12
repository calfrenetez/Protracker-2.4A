#include "paula_segment_boundary.h"
#include <stddef.h>
#include <string.h>
#define ALIGNMENT(T) offsetof(struct {char c;T value;},value)
static int span(const void *p,size_t n)
{return p&&n<=UINTPTR_MAX-(uintptr_t)p;}
static int object(const void *p,size_t n,size_t a)
{return span(p,n)&&!((uintptr_t)p%a);}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,n)&&span(b,m)&&(x<=y?n<=y-x:m<=x-y);}
static int same_voice(const struct pt_voice *a,const struct pt_voice *b)
{return a->pcm==b->pcm&&a->repeat_pcm==b->repeat_pcm&&a->phase==b->phase&&a->cycle==b->cycle&&
    a->start==b->start&&a->end==b->end&&a->loop_start==b->loop_start&&a->loop_end==b->loop_end&&
    a->loop==b->loop&&a->looped==b->looped&&a->linear==b->linear&&a->active==b->active&&a->segment==b->segment;}
static int same_key(const struct pt_readers_key *a,const struct pt_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&
    a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
static int known(const struct pt_paula_segment_boundary_scope *s,const struct pt_pcm *p)
{unsigned i;for(i=0;i<s->dependency_count;++i)if(s->dependency[i]==p)return 1;return 0;}
enum pt_paula_segment_boundary_result pt_paula_segment_boundary_admit(
    const struct pt_paula_segment_boundary_scope *scope,unsigned index,uint64_t frame,
    const struct pt_render_plan *plan,const struct pt_readers_key *original,
    const struct pt_readers_key *actual,struct pt_render_action *out)
{
    struct pt_paula_segment_boundary_scope s;struct pt_render_action a,c;
    struct pt_readers_key o,k;unsigned i,j;enum pt_render_action_kind expected;
    if(!object(scope,sizeof(*scope),ALIGNMENT(struct pt_paula_segment_boundary_scope))||
       !object(plan,sizeof(*plan),ALIGNMENT(struct pt_render_plan))||
       !object(out,sizeof(*out),ALIGNMENT(struct pt_render_action))||
       (original&&!object(original,sizeof(*original),ALIGNMENT(struct pt_readers_key)))||
       (actual&&!object(actual,sizeof(*actual),ALIGNMENT(struct pt_readers_key)))||
       !apart(out,sizeof(*out),scope,sizeof(*scope))||!apart(out,sizeof(*out),plan,sizeof(*plan))||
       !apart(scope,sizeof(*scope),plan,sizeof(*plan))||
       (original&&(!apart(out,sizeof(*out),original,sizeof(*original))||
           !apart(scope,sizeof(*scope),original,sizeof(*original))||!apart(plan,sizeof(*plan),original,sizeof(*original))))||
       (actual&&(!apart(out,sizeof(*out),actual,sizeof(*actual))||
           !apart(scope,sizeof(*scope),actual,sizeof(*actual))||!apart(plan,sizeof(*plan),actual,sizeof(*actual)))))return PT_PAULA_BOUNDARY_ALIAS;
    memcpy(&s,scope,sizeof(s));
    if(index>=PT_PAULA_BOUNDARY_EVENTS||s.track>=PT_CHANNEL_LIMIT||!s.dependency_count||
       s.dependency_count>PT_PAULA_BOUNDARY_DEPENDENCIES)return PT_PAULA_BOUNDARY_INVALID;
    for(i=0;i<PT_PAULA_BOUNDARY_EVENTS;++i)if(s.frame[i]==UINT64_MAX||(i&&s.frame[i]<=s.frame[i-1]))return PT_PAULA_BOUNDARY_INVALID;
    for(i=0;i<s.dependency_count;++i){struct pt_pcm pcm;size_t bytes;
        if(!object(s.dependency[i],sizeof(*s.dependency[i]),ALIGNMENT(struct pt_pcm)))return PT_PAULA_BOUNDARY_INVALID;
        if(!apart(out,sizeof(*out),s.dependency[i],sizeof(*s.dependency[i])))return PT_PAULA_BOUNDARY_ALIAS;
        for(j=0;j<i;++j)if(s.dependency[i]==s.dependency[j])return PT_PAULA_BOUNDARY_INVALID;
        /* capacity counts int32_t ELEMENTS, already including channels. Read
         * descriptor metadata only; protect every byte, including spare tail. */
        memcpy(&pcm,s.dependency[i],sizeof(pcm));
        if(!pcm.capacity&&(pcm.data||pcm.frames))return PT_PAULA_BOUNDARY_INVALID;
        if(pcm.capacity>SIZE_MAX/sizeof(*pcm.data))return PT_PAULA_BOUNDARY_INVALID;
        bytes=pcm.capacity*sizeof(*pcm.data);
        if(bytes&&!object(pcm.data,bytes,ALIGNMENT(int32_t)))return PT_PAULA_BOUNDARY_INVALID;
        if(bytes&&!apart(out,sizeof(*out),pcm.data,bytes))return PT_PAULA_BOUNDARY_ALIAS;
    }
    if(frame!=s.frame[index])return PT_PAULA_BOUNDARY_STALE;
    if(!index){if(original||actual)return PT_PAULA_BOUNDARY_INVALID;}
    else{
        if(!original||!actual)return PT_PAULA_BOUNDARY_INVALID;
        o=*original;k=*actual;
        if(!o.queue||!o.session||!o.generation||!o.trigger||!o.owner||!o.serial||o.action||o.slot>=4||
           !same_key(&o,&k))return PT_PAULA_BOUNDARY_STALE;
    }
    expected=index==0?PT_RENDER_SEGMENT:index==3?PT_RENDER_STOP:PT_RENDER_REPEAT;
    if(plan->count!=(index==3?1U:2U))return PT_PAULA_BOUNDARY_UNSUPPORTED;
    memcpy(&a,plan->action,sizeof(a));
    if(a.kind!=expected||a.channel!=s.track||a.gain[0]||a.gain[1]||
       !known(&s,a.voice.pcm)||(a.voice.repeat_pcm&&!known(&s,a.voice.repeat_pcm)))return PT_PAULA_BOUNDARY_UNSUPPORTED;
    if(index==3){if(a.voice.active)return PT_PAULA_BOUNDARY_UNSUPPORTED;}
    else{
        memcpy(&c,plan->action+1,sizeof(c));
        if(c.kind!=PT_RENDER_CONTROL||c.channel!=s.track||!same_voice(&a.voice,&c.voice)||
           !c.voice.step||c.gain[0]>65536||c.gain[1]>65536)return PT_PAULA_BOUNDARY_UNSUPPORTED;
        a.voice.step=c.voice.step;a.gain[0]=c.gain[0];a.gain[1]=c.gain[1];
    }
    memcpy(out,&a,sizeof(a));return PT_PAULA_BOUNDARY_OK;
}
