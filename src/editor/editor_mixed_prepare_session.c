#include "editor_mixed_prepare_session.h"
#include "editor_mixed_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>

static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);}
static int inside(struct pt_sampler_storage_span s,const void *p,size_t n)
{return n&&span(s.data,s.bytes)&&span(p,n)&&s.bytes>=n&&(uintptr_t)p>=(uintptr_t)s.data&&
    (uintptr_t)p-(uintptr_t)s.data<=s.bytes-n;}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static enum pt_editor_mixed_session_result fail(struct pt_editor_mixed_prepare_session *s,
    enum pt_editor_mixed_session_result r)
{
    if(!s->first_error)s->first_error=r;
    s->result=s->first_error;
    if(s->phase!=PT_EDITOR_MIXED_SESSION_FINISHED)s->phase=PT_EDITOR_MIXED_SESSION_FAILED;
    return s->result;
}
static int finish_session(void *context);
static int no_other_owner(struct pt_editor_mixed_prepare_session *s)
{
    struct pt_editor_mixed *o=s->saved.binding;
    return !o->transport&&(!o->owner||o->owner==s->captured)&&
        (!o->preparation_context||o->preparation_context==s->saved.establish)&&
        (!o->owner_finish_context||o->owner_finish_context==s||o->owner_finish_context==s->saved.checked);
}
static int controls_idle(const struct pt_editor_mixed *o)
{return !o->owner&&!o->transport&&!o->preparation_close&&!o->preparation_context&&
    !o->owner_finish&&!o->owner_finish_context;}
static int output_safe(struct pt_editor_mixed *o,const void *p,size_t n)
{return apart(p,n,o->editor,sizeof(*o->editor))&&apart(p,n,o,sizeof(*o))&&
    pt_sampler_output_disjoint(&o->editor->sampler,p,n)&&
    pt_render_project_storage_output_disjoint(o->editor->project,p,n);}

enum pt_editor_mixed_session_result pt_editor_mixed_prepare_session_begin(
    struct pt_editor_mixed_prepare_session *s,const struct pt_editor_mixed_prepare_session_inputs *in)
{
    struct pt_editor_mixed_prepare_session_inputs v;struct pt_editor_mixed *o;
    struct pt_amigus_wavetable_cache *b;struct pt_sampler_storage_span controls[6];
    const void *opaque[10];unsigned i,j;
    if(!s||!in||!apart(s,sizeof(*s),in,sizeof(*in)))return PT_EDITOR_MIXED_SESSION_INVALID;
    memcpy(&v,in,sizeof(v));o=v.binding;b=v.devices.backend;
    if(!pt_editor_mixed_attached(o)||!v.establish||!v.bridges||!v.checked||
       !b||!b->reservation||!v.work||v.work>4096||
       !inside(v.contexts,s,sizeof(*s))||!output_safe(o,s,sizeof(*s))||
       !apart(s,sizeof(*s),b,sizeof(*b))||!apart(s,sizeof(*s),b->reservation,sizeof(*b->reservation)))
        return PT_EDITOR_MIXED_SESSION_INVALID;
    controls[0]=(struct pt_sampler_storage_span){v.establish,sizeof(*v.establish)};
    controls[1]=(struct pt_sampler_storage_span){v.bridges,sizeof(*v.bridges)};
    controls[2]=(struct pt_sampler_storage_span){v.checked,sizeof(*v.checked)};
    controls[3]=(struct pt_sampler_storage_span){o,sizeof(*o)};
    controls[4]=(struct pt_sampler_storage_span){o->editor,sizeof(*o->editor)};
    controls[5]=(struct pt_sampler_storage_span){in,sizeof(*in)};
    for(i=0;i<3;++i) {
        if(!output_safe(o,controls[i].data,controls[i].bytes)||
           !apart(controls[i].data,controls[i].bytes,v.contexts.data,v.contexts.bytes)||
           !apart(controls[i].data,controls[i].bytes,b,sizeof(*b))||
           !apart(controls[i].data,controls[i].bytes,b->reservation,sizeof(*b->reservation)))
            return PT_EDITOR_MIXED_SESSION_INVALID;
        for(j=i+1;j<6;++j)if(!apart(controls[i].data,controls[i].bytes,controls[j].data,controls[j].bytes))
            return PT_EDITOR_MIXED_SESSION_INVALID;
    }
    if(!apart(v.contexts.data,v.contexts.bytes,o,sizeof(*o))||
       !apart(v.contexts.data,v.contexts.bytes,o->editor,sizeof(*o->editor)))return PT_EDITOR_MIXED_SESSION_INVALID;
    opaque[0]=o->editor->sampler.allocator.context;opaque[1]=v.devices.chip_context;
    opaque[2]=v.devices.paula.context;opaque[3]=v.devices.amigus.context;
    opaque[4]=v.devices.paula_quiesce_context;opaque[5]=v.devices.amigus_quiesce_context;
    opaque[6]=b->context;opaque[7]=b->arena.context;opaque[8]=b->cache.context;opaque[9]=b->reservation->api.context;
    for(i=0;i<10;++i)if(opaque[i]&&(!inside(v.contexts,opaque[i],1)||!apart(s,sizeof(*s),opaque[i],1)))
        return PT_EDITOR_MIXED_SESSION_INVALID;
    if(s->busy)return fail(s,PT_EDITOR_MIXED_SESSION_FAULT);
    if(!controls_idle(o)||!zero(s,sizeof(*s))||!zero(v.establish,sizeof(*v.establish))||
       !zero(v.bridges,sizeof(*v.bridges))||!zero(v.checked,sizeof(*v.checked)))return PT_EDITOR_MIXED_SESSION_INVALID;
    /* No writes or callbacks before the complete admission above. Both later
     * allocators protect this composite and the separate downstream controls. */
    s->inputs=in;memcpy(&s->saved,&v,sizeof(v));s->phase=PT_EDITOR_MIXED_SESSION_MASTERS;
    s->result=PT_EDITOR_MIXED_SESSION_PENDING;s->revision=o->editor->history.revision;
    s->generation=o->editor->sampler.generation;s->busy=1;
    s->parents[0]=v.contexts;s->parents[1]=controls[1];s->parents[2]=controls[2];
    s->parents[3]=(struct pt_sampler_storage_span){b,sizeof(*b)};
    s->parents[4]=(struct pt_sampler_storage_span){b->reservation,sizeof(*b->reservation)};
    s->parents[5]=controls[5];
    s->establish_result=pt_editor_mixed_establish_begin(v.establish,o,s->parents,6);
    s->busy=0;
    if(s->first_error)return s->result;
    if(s->establish_result!=PT_ESTABLISH_PENDING)return fail(s,PT_EDITOR_MIXED_SESSION_ESTABLISH_ERROR);
    return s->result;
}

enum pt_editor_mixed_session_result pt_editor_mixed_prepare_session_get(struct pt_editor_mixed_prepare_session *s)
{
    struct pt_editor_mixed *o;
    if(!s||!s->phase)return PT_EDITOR_MIXED_SESSION_INVALID;
    if(s->busy)return fail(s,PT_EDITOR_MIXED_SESSION_FAULT);
    if(s->phase==PT_EDITOR_MIXED_SESSION_FINISHED||s->phase==PT_EDITOR_MIXED_SESSION_FAILED)return s->result;
    o=s->saved.binding;
    if(!pt_editor_mixed_attached(o)||!no_other_owner(s))return fail(s,PT_EDITOR_MIXED_SESSION_CANCELLED);
    if(s->phase==PT_EDITOR_MIXED_SESSION_MASTERS||s->phase==PT_EDITOR_MIXED_SESSION_HANDOFF) {
        if(o->preparation_context!=s->saved.establish||s->saved.establish->binding!=o||
           !s->saved.establish->job.active)return fail(s,PT_EDITOR_MIXED_SESSION_CANCELLED);
    }else if(o->owner_finish!=finish_session||o->owner_finish_context!=s||!o->owner)
        return fail(s,PT_EDITOR_MIXED_SESSION_CANCELLED);
    if(o->editor->history.revision!=s->revision||o->editor->sampler.generation!=s->generation||
       memcmp(s->inputs,&s->saved,sizeof(s->saved)))return fail(s,PT_EDITOR_MIXED_SESSION_CANCELLED);
    return s->result;
}

enum pt_editor_mixed_session_result pt_editor_mixed_prepare_session_advance(struct pt_editor_mixed_prepare_session *s)
{
    enum pt_editor_mixed_session_result status=pt_editor_mixed_prepare_session_get(s);
    struct pt_editor_mixed *o;enum pt_mixed_owner_result r;
    if(status!=PT_EDITOR_MIXED_SESSION_PENDING)return status;
    o=s->saved.binding;s->busy=1;
    if(s->phase==PT_EDITOR_MIXED_SESSION_MASTERS) {
        s->establish_result=pt_editor_mixed_establish_step(s->saved.establish,s->saved.work);
        if(!s->first_error) {
            if(s->establish_result==PT_ESTABLISH_READY)s->phase=PT_EDITOR_MIXED_SESSION_HANDOFF;
            else if(s->establish_result!=PT_ESTABLISH_PENDING)fail(s,PT_EDITOR_MIXED_SESSION_ESTABLISH_ERROR);
        }
    }else if(s->phase==PT_EDITOR_MIXED_SESSION_HANDOFF) {
        s->establish_result=pt_sampler_establish_get(&s->saved.establish->job,s->revision,s->generation);
        if(s->establish_result!=PT_ESTABLISH_READY)fail(s,PT_EDITOR_MIXED_SESSION_ESTABLISH_ERROR);
        else if(!pt_editor_mixed_stop(o))fail(s,PT_EDITOR_MIXED_SESSION_FAULT);
        else if(!s->first_error) {
            if(!controls_idle(o)||o->editor->history.revision!=s->revision||o->editor->sampler.generation!=s->generation)
                fail(s,PT_EDITOR_MIXED_SESSION_CANCELLED);
            else if(!pt_editor_mixed_bridges_bind(s->saved.bridges,o,&s->saved.devices,&s->saved.contexts,1))
                fail(s,PT_EDITOR_MIXED_SESSION_BRIDGE_ERROR);
            else {
                s->bridges_bound=1;
                r=pt_editor_mixed_checked_begin(o,s->saved.checked,&s->saved.bridges->paula,&s->saved.bridges->amigus,
                    &s->saved.options,&s->saved.caps,&s->saved.format,s->saved.contexts,s->saved.work);
                s->checked_result=r;
                s->captured=o->owner;
                /* Private opt-in finish adapter; the original checked hook
                 * already guarded its first allocator callback. Never yield an
                 * admitted idle bridge without this retained closure hook. */
                if(o->owner_finish&&o->owner_finish_context==s->saved.checked) {
                    s->checked_finish=o->owner_finish;s->checked_finish_context=o->owner_finish_context;
                }else if(o->owner_finish||o->owner_finish_context)fail(s,PT_EDITOR_MIXED_SESSION_FAULT);
                if(!o->owner_finish||o->owner_finish_context==s->saved.checked) {
                    o->owner_finish=finish_session;o->owner_finish_context=s;
                }
                if(!s->first_error) {
                    if(r==PT_MIXED_OWNER_PREPARING)s->phase=PT_EDITOR_MIXED_SESSION_CHECKED;
                    else fail(s,PT_EDITOR_MIXED_SESSION_CHECKED_ERROR);
                }
            }
        }
    }else if(s->phase==PT_EDITOR_MIXED_SESSION_CHECKED) {
        s->checked_result=pt_editor_mixed_prepare(o,NULL);
        if(!s->first_error) {
            if(s->checked_result==PT_MIXED_OWNER_OK) {
                s->phase=PT_EDITOR_MIXED_SESSION_PREPARED;s->result=PT_EDITOR_MIXED_SESSION_READY;
            }else if(s->checked_result!=PT_MIXED_OWNER_PREPARING)fail(s,PT_EDITOR_MIXED_SESSION_CHECKED_ERROR);
        }
    }else fail(s,PT_EDITOR_MIXED_SESSION_FAULT);
    s->busy=0;return s->result;
}

static int finish_session(void *context)
{
    struct pt_editor_mixed_prepare_session *s=context;struct pt_editor_mixed *o=s->saved.binding;
    unsigned outer=s->busy;int p,w;
    if(s->finishing||(s->busy&&!s->close_call)||s->saved.checked->starting||s->saved.checked->storage.memory.busy) {
        fail(s,PT_EDITOR_MIXED_SESSION_FAULT);return 0;
    }
    if(o->owner||o->transport||o->preparation_close||o->preparation_context||
       o->owner_finish!=finish_session||o->owner_finish_context!=s)return 0;
    s->busy=s->finishing=1;
    p=pt_paula_voices_close(&s->saved.bridges->paula);
    w=pt_wavetable_voices_close(&s->saved.bridges->amigus);
    if(!p||!w){s->finishing=0;s->busy=outer;return 0;}
    s->bridges_bound=0;
    if(s->checked_finish&&s->checked_finish(s->checked_finish_context)!=1) {
        s->finishing=0;s->busy=outer;return 0;
    }
    if(!s->close_call&&!s->first_error)fail(s,PT_EDITOR_MIXED_SESSION_CANCELLED);
    s->phase=PT_EDITOR_MIXED_SESSION_FINISHED;
    if(!s->first_error)s->result=PT_EDITOR_MIXED_SESSION_CLOSED;
    s->finishing=0;s->busy=outer;return 1;
}

int pt_editor_mixed_prepare_session_close(struct pt_editor_mixed_prepare_session *s)
{
    struct pt_editor_mixed *o;int complete;
    if(!s)return 0;
    if(s->busy){fail(s,PT_EDITOR_MIXED_SESSION_FAULT);return 0;}
    if(!s->phase||s->phase==PT_EDITOR_MIXED_SESSION_FINISHED)return 1;
    o=s->saved.binding;
    if(!no_other_owner(s)||o->transport)return 0;
    if(s->bridges_bound&&(o->owner_finish!=finish_session||o->owner_finish_context!=s))return 0;
    /* No source-array traversal or restarting. The existing barrier remains
     * responsible for a cancelled establishment job and checked owner. */
    s->busy=s->close_call=1;complete=pt_editor_mixed_stop(o);
    s->close_call=s->busy=0;
    if(!complete||s->bridges_bound)return 0;
    s->phase=PT_EDITOR_MIXED_SESSION_FINISHED;
    if(!s->first_error)s->result=PT_EDITOR_MIXED_SESSION_CLOSED;
    return 1;
}
