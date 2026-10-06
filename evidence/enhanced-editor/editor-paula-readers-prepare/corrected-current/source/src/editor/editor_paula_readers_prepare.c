#include "editor_paula_readers_prepare.h"
#include "editor_mixed_internal.h"
#include "project_snapshot.h"
#include "../core/render_storage_internal.h"
#include <string.h>
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);}
static int inside(struct pt_sampler_storage_span s,const void *p,size_t n)
{return n&&span(s.data,s.bytes)&&span(p,n)&&s.bytes>=n&&(uintptr_t)p>=(uintptr_t)s.data&&
    (uintptr_t)p-(uintptr_t)s.data<=s.bytes-n;}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static enum pt_editor_readers_result fail(struct pt_editor_paula_readers_prepare *s,enum pt_editor_readers_result r)
{
    if(!s->first_error)s->first_error=r;
    s->result=s->first_error;
    if(s->phase!=PT_EDITOR_READERS_FINISHED)s->phase=PT_EDITOR_READERS_FAILED;
    return s->result;
}
static int reentry(struct pt_editor_paula_readers_prepare *s)
{if(!s->busy)return 0;++s->reentries;fail(s,PT_EDITOR_READERS_FAULT);return 1;}
static int idle(const struct pt_editor_mixed *o)
{return !o->owner&&!o->transport&&!o->preparation_close&&!o->preparation_context&&!o->owner_finish&&!o->owner_finish_context;}
static int source_apart(struct pt_editor_mixed *o,const void *p,size_t n)
{return apart(p,n,o,sizeof(*o))&&apart(p,n,o->editor,sizeof(*o->editor))&&
    pt_sampler_output_disjoint(&o->editor->sampler,p,n)&&pt_render_project_storage_output_disjoint(o->editor->project,p,n);}
static int fixed_apart(struct pt_editor_paula_readers_prepare *s,const void *p,size_t n)
{unsigned i;for(i=0;i<PT_EDITOR_READERS_ALLOCATIONS;++i)
    if(!apart(p,n,s->allocation[i].data,s->allocation[i].bytes))return 0;
 return span(p,n)&&apart(p,n,s->saved.contexts.data,s->saved.contexts.bytes)&&
    apart(p,n,s->inputs,sizeof(*s->inputs))&&apart(p,n,s->saved.establish,sizeof(*s->saved.establish))&&
    apart(p,n,s->saved.workspace,s->saved.workspace_capacity)&&
    apart(p,n,s->saved.binding,sizeof(*s->saved.binding))&&
    apart(p,n,s->editor,sizeof(*s->editor));}
static int callback_current(struct pt_editor_paula_readers_prepare *s)
{struct pt_editor_mixed *o=s->saved.binding;struct pt_project header;
 if(o->editor!=s->editor||s->editor->project!=s->project||!pt_editor_mixed_attached(o))return 0;
 if(s->producer_adopted){header=s->project_header;header.channels.selected=o->editor->project->channels.selected;
    if(memcmp(&o->editor->sampler,&s->sampler_header,sizeof(s->sampler_header))||
       !pt_project_snapshot_equal(o->editor->project,&header))return 0;}
 return pt_editor_mixed_attached(o)&&!o->owner&&!o->transport&&!o->owner_finish&&!o->owner_finish_context&&
    !memcmp(s->inputs,&s->saved,sizeof(s->saved))&&
    !memcmp(&o->editor->sampler.allocator,&s->original_allocator,sizeof(s->original_allocator))&&
    o->editor->history.revision==s->revision&&o->editor->sampler.generation==s->generation;}
static int remember(struct pt_editor_paula_readers_prepare *s,void *p,size_t n)
{
    unsigned i;
    if(!p)return 1;
    for(i=0;i<PT_EDITOR_READERS_ALLOCATIONS;++i){
        if(!s->allocation[i].data){s->allocation[i]=(struct pt_sampler_storage_span){p,n};return 1;}
    }
    return 0;
}
static void forget(struct pt_editor_paula_readers_prepare *s,void *p)
{unsigned i;for(i=0;i<PT_EDITOR_READERS_ALLOCATIONS;++i)if(s->allocation[i].data==p)
    s->allocation[i]=(struct pt_sampler_storage_span){NULL,0};}
static void *guard_allocate(void *context,size_t n)
{struct pt_editor_paula_readers_prepare *s=context;void *p;
 if(!s->busy||s->closing||s->first_error)return NULL;
 p=s->original_allocator.allocate(s->original_allocator.context,n);
 if(p&&!fixed_apart(s,p,n)){fail(s,PT_EDITOR_READERS_FAULT);return NULL;}
 /* The genuine producer owns numeric source snapshots. If a callback changes
  * fixed headers, return its arena there for classification without walking
  * former tables or guessing whether release is safe. */
 if(!callback_current(s)){fail(s,PT_EDITOR_READERS_FAULT);return p;}
 if(p&&!source_apart(s->saved.binding,p,n)){fail(s,PT_EDITOR_READERS_FAULT);return NULL;}
 if(s->first_error&&p){s->original_allocator.release(s->original_allocator.context,p);return NULL;}
 if(!remember(s,p,n)){fail(s,PT_EDITOR_READERS_FAULT);s->original_allocator.release(s->original_allocator.context,p);return NULL;}
 return p;}
static void guard_release(void *context,void *p)
{struct pt_editor_paula_readers_prepare *s=context;
 if(!s->busy){fail(s,PT_EDITOR_READERS_FAULT);return;}
 s->original_allocator.release(s->original_allocator.context,p);
 forget(s,p);
 if(!callback_current(s))fail(s,PT_EDITOR_READERS_FAULT);}
static void *guard_chip_allocate(void *context,size_t n)
{struct pt_editor_paula_readers_prepare *s=context;void *p;
 if(!s->busy||s->closing||s->first_error)return NULL;
 p=s->saved.config.readers.chip_allocate(s->saved.config.readers.chip_context,n);
 if(p&&!fixed_apart(s,p,n)){fail(s,PT_EDITOR_READERS_FAULT);return NULL;}
 if(!callback_current(s)){fail(s,PT_EDITOR_READERS_FAULT);return p;}
 if(p&&!source_apart(s->saved.binding,p,n)){fail(s,PT_EDITOR_READERS_FAULT);return NULL;}
 if(s->first_error&&p){s->saved.config.readers.chip_release(s->saved.config.readers.chip_context,p,n);return NULL;}
 if(!remember(s,p,n)){fail(s,PT_EDITOR_READERS_FAULT);s->saved.config.readers.chip_release(s->saved.config.readers.chip_context,p,n);return NULL;}
 return p;}
static void guard_chip_release(void *context,void *p,size_t n)
{struct pt_editor_paula_readers_prepare *s=context;
 if(!s->busy){fail(s,PT_EDITOR_READERS_FAULT);return;}
 s->saved.config.readers.chip_release(s->saved.config.readers.chip_context,p,n);
 forget(s,p);
 if(!callback_current(s))fail(s,PT_EDITOR_READERS_FAULT);}
static int guard_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_editor_paula_readers_prepare *s=((struct pt_editor_readers_callback *)context)->owner;int r;
    if(!s->busy||s->closing||s->first_error)return 0;
    r=s->saved.config.backend.read_clock(s->saved.config.backend.context,ticks,frequency);
    if(!callback_current(s))fail(s,PT_EDITOR_READERS_FAULT);
    return s->first_error?0:r;
}
static int guard_submit(void *context,const struct pt_readers_event *event)
{
    struct pt_editor_paula_readers_prepare *s=((struct pt_editor_readers_callback *)context)->owner;int r;
    if(!s->busy||s->closing||s->first_error)return 0;
    r=s->saved.config.backend.submit(s->saved.config.backend.context,event);
    if(!callback_current(s))fail(s,PT_EDITOR_READERS_FAULT);
    return r;
}
static enum pt_readers_reply guard_command(void *context,uint64_t ticket,struct pt_readers_command_receipt *out,unsigned cancel)
{
    struct pt_editor_paula_readers_prepare *s=((struct pt_editor_readers_callback *)context)->owner;enum pt_readers_reply r;
    if(!s->busy||s->closing)return PT_READERS_UNCERTAIN;
    r=cancel?s->saved.config.backend.cancel_command(s->saved.config.backend.context,ticket,out):
        s->saved.config.backend.poll_command(s->saved.config.backend.context,ticket,out);
    if(!callback_current(s))fail(s,PT_EDITOR_READERS_FAULT);
    return r;
}
static enum pt_readers_reply guard_poll_command(void *c,uint64_t t,struct pt_readers_command_receipt *out)
{return guard_command(c,t,out,0);}
static enum pt_readers_reply guard_cancel_command(void *c,uint64_t t,struct pt_readers_command_receipt *out)
{return guard_command(c,t,out,1);}
static enum pt_readers_reply guard_reader(void *context,const struct pt_readers_domain *domain,struct pt_readers_reader_receipt *out,unsigned cancel)
{
    struct pt_editor_paula_readers_prepare *s=((struct pt_editor_readers_callback *)context)->owner;enum pt_readers_reply r;
    if(!s->busy||s->closing)return PT_READERS_UNCERTAIN;
    r=cancel?s->saved.config.backend.cancel_reader(s->saved.config.backend.context,domain,out):
        s->saved.config.backend.poll_reader(s->saved.config.backend.context,domain,out);
    if(!callback_current(s))fail(s,PT_EDITOR_READERS_FAULT);
    return r;
}
static enum pt_readers_reply guard_poll_reader(void *c,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{return guard_reader(c,d,out,0);}
static enum pt_readers_reply guard_cancel_reader(void *c,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{return guard_reader(c,d,out,1);}
static int close_producer(void *context)
{
    struct pt_editor_paula_readers_prepare *s=context;unsigned outer=s->busy,before=s->reentries;int complete=1;
    if(s->closing||(s->busy&&!s->close_call)){++s->reentries;fail(s,PT_EDITOR_READERS_FAULT);return 0;}
    if(!s->close_call)fail(s,PT_EDITOR_READERS_CANCELLED);
    s->busy=s->closing=1;
    if(s->producer){s->producer_result=pt_paula_readers_song_cancel(s->producer);
        complete=pt_paula_readers_song_close(&s->producer);}
    s->closing=0;s->busy=outer;
    if(!complete||s->producer||before!=s->reentries)return 0;
    s->phase=PT_EDITOR_READERS_FINISHED;if(!s->first_error)s->result=PT_EDITOR_READERS_CLOSED;return 1;
}
enum pt_editor_readers_result pt_editor_paula_readers_prepare_begin(
    struct pt_editor_paula_readers_prepare *s,const struct pt_editor_paula_readers_prepare_inputs *in)
{
    struct pt_editor_paula_readers_prepare_inputs v;struct pt_editor_mixed *o;
    struct pt_sampler_storage_span controls[4];const void *contexts[3];unsigned i,j;
    if(!span(s,sizeof(*s))||!span(in,sizeof(*in))||(uintptr_t)s%ALIGN_OF(struct pt_editor_paula_readers_prepare)||
       !apart(s,sizeof(*s),in,sizeof(*in)))return PT_EDITOR_READERS_INVALID;
    memcpy(&v,in,sizeof(v));o=v.binding;
    if(!span(o,sizeof(*o))||!o||(uintptr_t)o%ALIGN_OF(struct pt_editor_mixed)||
       !span(o->editor,sizeof(*o->editor))||(uintptr_t)o->editor%ALIGN_OF(struct pt_editor)||!pt_editor_mixed_attached(o)||
       !span(v.establish,sizeof(*v.establish))||!v.establish||(uintptr_t)v.establish%ALIGN_OF(struct pt_editor_mixed_establish)||
       !inside(v.contexts,s,sizeof(*s))||!source_apart(o,v.contexts.data,v.contexts.bytes)||
       !source_apart(o,s,sizeof(*s))||!span(v.workspace,v.workspace_capacity)||!v.workspace||
       v.workspace_capacity<pt_paula_readers_song_begin_workspace_size()||
       (uintptr_t)v.workspace%pt_paula_readers_song_begin_workspace_alignment())return PT_EDITOR_READERS_INVALID;
    controls[0]=(struct pt_sampler_storage_span){v.establish,sizeof(*v.establish)};
    controls[1]=(struct pt_sampler_storage_span){o,sizeof(*o)};
    controls[2]=(struct pt_sampler_storage_span){o->editor,sizeof(*o->editor)};
    controls[3]=(struct pt_sampler_storage_span){in,sizeof(*in)};
    if(!source_apart(o,v.workspace,v.workspace_capacity)||!source_apart(o,in,sizeof(*in))||
       !source_apart(o,v.establish,sizeof(*v.establish))||
       !apart(v.workspace,v.workspace_capacity,v.contexts.data,v.contexts.bytes))return PT_EDITOR_READERS_INVALID;
    for(i=0;i<4;++i){
        if(!apart(v.workspace,v.workspace_capacity,controls[i].data,controls[i].bytes))return PT_EDITOR_READERS_INVALID;
        if(i<3&&!apart(v.contexts.data,v.contexts.bytes,controls[i].data,controls[i].bytes))return PT_EDITOR_READERS_INVALID;
        for(j=i+1;j<4;++j)if(!apart(controls[i].data,controls[i].bytes,controls[j].data,controls[j].bytes))return PT_EDITOR_READERS_INVALID;
    }
    contexts[0]=o->editor->sampler.allocator.context;contexts[1]=v.config.readers.chip_context;contexts[2]=v.config.backend.context;
    for(i=0;i<3;++i)if(contexts[i]&&(!inside(v.contexts,contexts[i],i==2?v.config.backend.context_bytes:1)||
        !apart(contexts[i],i==2?v.config.backend.context_bytes:1,s,sizeof(*s))||
        !apart(contexts[i],i==2?v.config.backend.context_bytes:1,in,sizeof(*in))))return PT_EDITOR_READERS_INVALID;
    if(!v.config.backend.context||!v.config.backend.context_bytes||!v.config.readers.chip_allocate||!v.config.readers.chip_release||
       !v.config.backend.read_clock||!v.config.backend.submit||!v.config.backend.poll_command||!v.config.backend.cancel_command||
       !v.config.backend.poll_reader||!v.config.backend.cancel_reader||!o->editor->sampler.allocator.allocate||
       !o->editor->sampler.allocator.release)return PT_EDITOR_READERS_INVALID;
    if(s->busy){reentry(s);return s->result;}
    if(!idle(o)||!zero(s,sizeof(*s))||!zero(v.establish,sizeof(*v.establish)))return PT_EDITOR_READERS_INVALID;
    s->inputs=in;s->saved=v;s->original_allocator=o->editor->sampler.allocator;
    s->editor=o->editor;s->project=o->editor->project;
    s->allocator=(struct pt_allocator){s,guard_allocate,guard_release};s->producer_config=v.config;
    s->producer_config.readers.chip_context=s;s->producer_config.readers.chip_allocate=guard_chip_allocate;
    s->producer_config.readers.chip_release=guard_chip_release;
    s->callback.owner=s;s->producer_config.backend.context=&s->callback;s->producer_config.backend.context_bytes=sizeof(s->callback);
    s->producer_config.backend.read_clock=guard_clock;s->producer_config.backend.submit=guard_submit;
    s->producer_config.backend.poll_command=guard_poll_command;s->producer_config.backend.cancel_command=guard_cancel_command;
    s->producer_config.backend.poll_reader=guard_poll_reader;s->producer_config.backend.cancel_reader=guard_cancel_reader;
    s->phase=PT_EDITOR_READERS_MASTERS;s->result=PT_EDITOR_READERS_PENDING;
    s->revision=o->editor->history.revision;s->generation=o->editor->sampler.generation;
    s->parents[0]=v.contexts;s->parents[1]=(struct pt_sampler_storage_span){v.workspace,v.workspace_capacity};s->parents[2]=controls[3];
    s->busy=1;s->establish_result=pt_editor_mixed_establish_begin(v.establish,o,s->parents,3);s->busy=0;
    if(s->first_error)return s->result;
    if(s->establish_result!=PT_ESTABLISH_PENDING)return fail(s,PT_EDITOR_READERS_ESTABLISH_ERROR);
    return s->result;
}
enum pt_editor_readers_result pt_editor_paula_readers_prepare_get(struct pt_editor_paula_readers_prepare *s)
{
    struct pt_editor_mixed *o;
    if(!s||!s->phase)return PT_EDITOR_READERS_INVALID;
    if(reentry(s))return s->result;
    if(s->phase==PT_EDITOR_READERS_FINISHED||s->phase==PT_EDITOR_READERS_FAILED)return s->result;
    o=s->saved.binding;
    if(!callback_current(s)||o->owner||o->transport||o->owner_finish||o->owner_finish_context)
        return fail(s,PT_EDITOR_READERS_CANCELLED);
    if(s->producer_adopted){if(o->preparation_close!=close_producer||o->preparation_context!=s)
            return fail(s,PT_EDITOR_READERS_CANCELLED);}
    else if(o->preparation_context!=s->saved.establish||s->saved.establish->binding!=o||!s->saved.establish->job.active)
        return fail(s,PT_EDITOR_READERS_CANCELLED);
    return s->result;
}
static enum pt_editor_readers_result record(struct pt_editor_paula_readers_prepare *s,enum pt_paula_readers_song_result r)
{
    s->producer_result=r;if(s->first_error)return s->result;
    switch(r){case PT_PAULA_READERS_SONG_OK:case PT_PAULA_READERS_SONG_PENDING:s->result=PT_EDITOR_READERS_PENDING;break;
    case PT_PAULA_READERS_SONG_DONE:s->result=PT_EDITOR_READERS_DONE;break;
    case PT_PAULA_READERS_SONG_WAIT_ACTIVE:s->result=PT_EDITOR_READERS_WAIT_ACTIVE;break;
    case PT_PAULA_READERS_SONG_WAIT_PRESSURE:s->result=PT_EDITOR_READERS_WAIT_PRESSURE;break;
    default:return fail(s,PT_EDITOR_READERS_PRODUCER_ERROR);}return s->result;
}
enum pt_editor_readers_result pt_editor_paula_readers_prepare_step(
    struct pt_editor_paula_readers_prepare *s,unsigned work,struct pt_paula_readers_song_status *out)
{
    enum pt_editor_readers_result r;struct pt_editor_mixed *o;struct pt_paula_readers_song_status status;
    if(!s||!s->phase||!work||work>4096)return PT_EDITOR_READERS_INVALID;
    if(s->phase==PT_EDITOR_READERS_FINISHED)return pt_editor_paula_readers_prepare_get(s);
    r=pt_editor_paula_readers_prepare_get(s);if(s->first_error||r==PT_EDITOR_READERS_INVALID)return r;
    if(out&&(!fixed_apart(s,out,sizeof(*out))||!source_apart(s->saved.binding,out,sizeof(*out))))return PT_EDITOR_READERS_INVALID;
    o=s->saved.binding;s->busy=1;
    if(s->phase==PT_EDITOR_READERS_MASTERS){s->establish_result=pt_editor_mixed_establish_step(s->saved.establish,work);
        if(!s->first_error){if(s->establish_result==PT_ESTABLISH_READY)s->phase=PT_EDITOR_READERS_HANDOFF;
            else if(s->establish_result!=PT_ESTABLISH_PENDING)fail(s,PT_EDITOR_READERS_ESTABLISH_ERROR);}}
    else if(s->phase==PT_EDITOR_READERS_HANDOFF){
        s->establish_result=pt_sampler_establish_get(&s->saved.establish->job,s->revision,s->generation);
        if(s->establish_result!=PT_ESTABLISH_READY)fail(s,PT_EDITOR_READERS_ESTABLISH_ERROR);
        else if(!pt_editor_mixed_stop(o))fail(s,PT_EDITOR_READERS_FAULT);
        else if(!s->first_error){
            if(!idle(o)||!callback_current(s))fail(s,PT_EDITOR_READERS_CANCELLED);
            else{s->project_header=*o->editor->project;s->sampler_header=o->editor->sampler;
                o->preparation_context=s;o->preparation_close=close_producer;s->producer_adopted=1;
                s->producer_result=pt_paula_readers_song_begin_in_workspace(&s->allocator,&o->editor->sampler,o->editor->project,
                    &s->producer_config,s->revision,s->saved.workspace,s->saved.workspace_capacity,&s->producer);
                if(!s->first_error){if(s->producer_result==PT_PAULA_READERS_SONG_PENDING)s->phase=PT_EDITOR_READERS_PRODUCER;
                    else fail(s,PT_EDITOR_READERS_PRODUCER_ERROR);}}}}
    else if(s->phase==PT_EDITOR_READERS_PRODUCER){memset(&status,0,sizeof(status));
        record(s,pt_paula_readers_song_step(s->producer,o->editor->history.revision,work,out?&status:NULL));
        if(out&&!s->first_error&&callback_current(s)&&fixed_apart(s,out,sizeof(*out))&&source_apart(o,out,sizeof(*out)))
            *out=status;}
    else fail(s,PT_EDITOR_READERS_FAULT);
    s->busy=0;return s->result;
}
enum pt_scheduled_result pt_editor_paula_readers_prepare_publish(struct pt_editor_paula_readers_prepare *s)
{
    enum pt_scheduled_result r;
    if(!s)return PT_SCHEDULED_INVALID;
    (void)pt_editor_paula_readers_prepare_get(s);
    if(s->first_error||s->phase!=PT_EDITOR_READERS_PRODUCER||!s->producer)return PT_SCHEDULED_INVALID;
    s->busy=1;r=pt_paula_readers_song_publish_next(s->producer,s->saved.binding->editor->history.revision);s->busy=0;
    if(r!=PT_SCHEDULED_OK&&r!=PT_SCHEDULED_PENDING&&r!=PT_SCHEDULED_INVALID)fail(s,PT_EDITOR_READERS_PRODUCER_ERROR);
    return s->first_error?PT_SCHEDULED_BACKEND:r;
}
static int service_ready(struct pt_editor_paula_readers_prepare *s,const void *out,size_t n)
{
    if(!s||!s->phase||s->phase==PT_EDITOR_READERS_FINISHED||!s->producer)return 0;
    if(reentry(s))return 0;
    if(out&&(!fixed_apart(s,out,n)||s->first_error||!callback_current(s)||!source_apart(s->saved.binding,out,n)))return 0;
    if(!s->producer_adopted||s->saved.binding->preparation_close!=close_producer||
       s->saved.binding->preparation_context!=s)return 0;
    return 1;
}
enum pt_scheduled_result pt_editor_paula_readers_prepare_service_command(
    struct pt_editor_paula_readers_prepare *s,unsigned index,unsigned cancel,struct pt_readers_command_receipt *out)
{enum pt_scheduled_result r;struct pt_readers_command_receipt receipt;
 if(index>=PT_PAULA_READERS_SONG_COMMANDS||cancel>1||!service_ready(s,out,sizeof(*out)))return PT_SCHEDULED_INVALID;
 memset(&receipt,0,sizeof(receipt));
 s->busy=1;r=pt_paula_readers_song_service_command(s->producer,index,cancel,out&&!s->first_error?&receipt:NULL);
 if(cancel||r==PT_SCHEDULED_BACKEND||r==PT_SCHEDULED_LATE||r==PT_SCHEDULED_CLOCK)fail(s,PT_EDITOR_READERS_PRODUCER_ERROR);
 if(out&&!s->first_error&&receipt.domain==PT_READERS_COMMAND_DOMAIN&&callback_current(s)&&fixed_apart(s,out,sizeof(*out))&&source_apart(s->saved.binding,out,sizeof(*out)))
     *out=receipt;
 s->busy=0;return s->first_error?PT_SCHEDULED_BACKEND:r;}
enum pt_scheduled_result pt_editor_paula_readers_prepare_service_reader(
    struct pt_editor_paula_readers_prepare *s,unsigned index,unsigned cancel,struct pt_readers_reader_receipt *out)
{enum pt_scheduled_result r;struct pt_readers_reader_receipt receipt;
 if(index>=PT_PAULA_READERS_SONG_READERS||cancel>1||!service_ready(s,out,sizeof(*out)))return PT_SCHEDULED_INVALID;
 memset(&receipt,0,sizeof(receipt));
 s->busy=1;r=pt_paula_readers_song_service_reader(s->producer,index,cancel,out&&!s->first_error?&receipt:NULL);
 if(r==PT_SCHEDULED_BACKEND||r==PT_SCHEDULED_LATE||r==PT_SCHEDULED_CLOCK)fail(s,PT_EDITOR_READERS_PRODUCER_ERROR);
 if(out&&!s->first_error&&receipt.domain==PT_READERS_READER_DOMAIN&&callback_current(s)&&fixed_apart(s,out,sizeof(*out))&&source_apart(s->saved.binding,out,sizeof(*out)))
     *out=receipt;
 s->busy=0;return s->first_error?PT_SCHEDULED_BACKEND:r;}
enum pt_editor_readers_result pt_editor_paula_readers_prepare_terminal_stop(struct pt_editor_paula_readers_prepare *s)
{enum pt_editor_readers_result r=pt_editor_paula_readers_prepare_get(s);
 if(!s||s->first_error||r!=PT_EDITOR_READERS_DONE||!s->producer)return r;
 s->busy=1;r=record(s,pt_paula_readers_song_terminal_stop(s->producer,s->saved.binding->editor->history.revision));s->busy=0;return r;}
int pt_editor_paula_readers_prepare_close(struct pt_editor_paula_readers_prepare *s)
{
    struct pt_editor_mixed *o;unsigned before;int complete;
    if(!s)return 0;
    if(reentry(s))return 0;
    if(!s->phase||s->phase==PT_EDITOR_READERS_FINISHED)return 1;
    o=s->saved.binding;
    if(o->owner||o->transport||o->owner_finish||o->owner_finish_context)return 0;
    if(s->producer_adopted){if(o->preparation_close!=close_producer||o->preparation_context!=s)return 0;}
    else if(o->preparation_context&&o->preparation_context!=s->saved.establish)return 0;
    before=s->reentries;s->busy=s->close_call=1;complete=pt_editor_mixed_stop(o);s->close_call=s->busy=0;
    if(!complete||s->producer||before!=s->reentries)return 0;
    s->phase=PT_EDITOR_READERS_FINISHED;if(!s->first_error)s->result=PT_EDITOR_READERS_CLOSED;return 1;
}
