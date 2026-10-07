#include "editor_mixed_readers_prepare.h"
#include "editor_mixed_readers_source_internal.h"
#include "editor_mixed_internal.h"
#include "project_snapshot.h"
#include "sampler_internal.h"
#include "sampler_mixed_readers_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>
#include <limits.h>
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
#define NO_READER PT_SAMPLER_MIXED_READERS

static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int inside(struct pt_sampler_storage_span s,const void *p,size_t n)
{return n&&span(s.data,s.bytes)&&span(p,n)&&s.bytes>=n&&
 (uintptr_t)p>=(uintptr_t)s.data&&(uintptr_t)p-(uintptr_t)s.data<=s.bytes-n;}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static int finish(void *);
static int source_identity(struct pt_editor_mixed_readers_prepare *);
static int source_descriptor(struct pt_editor_mixed_readers_prepare *);
static int source_fixed(struct pt_editor_mixed_readers_prepare *);
static int activation_live(const struct pt_editor_mixed_readers_prepare *s)
{unsigned i;if(!s->activation)return 0;
 for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)
    if(s->ordinary[i].data==s->activation&&s->ordinary[i].bytes==pt_mixed_activation_control_size())return 1;
 return 0;}
static enum pt_editor_mixed_readers_result fail(struct pt_editor_mixed_readers_prepare *s,
 enum pt_editor_mixed_readers_result r)
{if(s->source_mode)s->source_cancel_requested=1;
 if(!s->first_error)s->first_error=r;s->result=s->first_error;
 if((r==PT_EDITOR_MIXED_READERS_FAULT||r==PT_EDITOR_MIXED_READERS_STALE)&&activation_live(s))
    pt_mixed_activation_fail_closed(s->activation);
 if(s->phase!=PT_EDITOR_MIXED_READERS_FINISHED)s->phase=PT_EDITOR_MIXED_READERS_FAILED;
 return s->result;}
static int reentry(struct pt_editor_mixed_readers_prepare *s)
{if(!s->busy&&!s->source_busy)return 0;++s->reentries;fail(s,PT_EDITOR_MIXED_READERS_FAULT);return 1;}
static int idle(const struct pt_editor_mixed *o)
{return !o->owner&&!o->transport&&!o->preparation_close&&!o->preparation_context&&
 !o->owner_finish&&!o->owner_finish_context;}
/* Fixed readable controls only; no former source table or master traversal. */
static int fixed_current(struct pt_editor_mixed_readers_prepare *s)
{
 struct pt_editor_mixed *o=s->saved.binding;struct pt_project h;
 if(s->source_mode&&(!source_descriptor(s)||!s->source_activated))return 0;
 if(o->editor!=s->editor||s->editor->project!=s->project||!pt_editor_mixed_attached(o)||
    o->owner||o->transport||o->owner_finish||o->owner_finish_context||
    o->preparation_close!=finish||o->preparation_context!=s||
    s->editor->history.revision!=s->revision||s->editor->sampler.generation!=s->generation||
    memcmp(s->inputs,&s->saved,sizeof(s->saved))||
    memcmp(&s->editor->sampler,&s->sampler_header,sizeof(s->sampler_header)))return 0;
 memcpy(&h,&s->project_header,sizeof(h));h.channels.selected=s->project->channels.selected;
 return pt_project_snapshot_equal(s->project,&h);
}
static int source_apart(const struct pt_editor_mixed *o,const void *p,size_t n)
{return apart(p,n,o,sizeof(*o))&&apart(p,n,o->editor,sizeof(*o->editor))&&
 pt_sampler_output_disjoint(&o->editor->sampler,p,n)&&
 pt_render_project_storage_output_disjoint(o->editor->project,p,n);}
static int callback_covered(const struct pt_editor_mixed_readers_prepare_inputs *v,const void *p,size_t n)
{return inside(v->contexts,p,n)||inside((struct pt_sampler_storage_span){v->backend,sizeof(*v->backend)},p,n)||
 inside((struct pt_sampler_storage_span){v->backend->reservation,sizeof(*v->backend->reservation)},p,n);}
static int add(struct pt_editor_mixed_readers_prepare *s,const void *p,size_t n)
{unsigned i;if(!span(p,n))return 0;
 /* Early source capture survives promotion. Retain every old complete extent
  * while avoiding duplicate numeric recapture of large extension/context sets.
  * This changes no standalone capture or temporary batch-pair placement. */
 if(n&&s->source_mode)for(i=0;i<s->guard_count;++i)
    if(s->guards[i].data==p&&s->guards[i].bytes==n)return 1;
 if(s->guard_count>=PT_EDITOR_MIXED_READERS_GUARDS)return 0;
 if(n)s->guards[s->guard_count++]=(struct pt_sampler_storage_span){p,n};
 return 1;}
static int capture_sources(struct pt_editor_mixed_readers_prepare *s,unsigned require_current)
{
 struct pt_project *p=s->project;struct pt_sampler *m=&s->editor->sampler;
 struct pt_sampler_storage_span v[PT_SAMPLER_VERSION_SPANS];unsigned i,j,count;
 if(!add(s,p->samples,(size_t)p->sample_count*sizeof(*p->samples))||
    !add(s,p->orders,(size_t)p->order_count*sizeof(*p->orders))||
    !add(s,p->events,(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count*sizeof(*p->events))||
    !add(s,p->extensions,(size_t)p->extension_count*sizeof(*p->extensions))||
    !add(s,m->table,m->table_bytes)||
    !add(s,m->table_original,m->table_original?(size_t)p->sample_count*sizeof(*p->samples):0))return 0;
 for(i=0;i<p->sample_count;++i){const struct pt_sample *a=p->samples+i;
    if((require_current&&!m->current[i])||a->pcm.capacity>SIZE_MAX/sizeof(int32_t)||
       !add(s,a->pcm.data,a->pcm.capacity*sizeof(int32_t))||
       !add(s,a->slices,(size_t)a->slice_count*sizeof(uint32_t)))return 0;}
 for(i=0;i<p->extension_count;++i)if(!add(s,p->extensions[i].data,p->extensions[i].length))return 0;
 for(i=0;i<PT_PROJECT_SAMPLES;++i)if(m->current[i]){
    if(!pt_sampler_version_spans(m->current[i],v,PT_SAMPLER_VERSION_SPANS,&count))return 0;
    for(j=0;j<count;++j)if(!add(s,v[j].data,v[j].bytes))return 0;}
 return 1;
}
static int capture(struct pt_editor_mixed_readers_prepare *s)
{
 struct pt_project *p=s->project;
 if(!add(s,p,sizeof(*p))||!add(s,s->editor,sizeof(*s->editor))||
    !add(s,s->saved.binding,sizeof(*s->saved.binding))||
    !add(s,s->inputs,sizeof(*s->inputs))||!add(s,s->saved.contexts.data,s->saved.contexts.bytes)||
    !add(s,s->saved.activation_workspace,s->saved.activation_capacity)||
    !add(s,s->saved.factory_workspace,s->saved.factory_capacity)||
    !add(s,s->saved.backend,sizeof(*s->saved.backend))||
    !add(s,s->saved.backend->reservation,sizeof(*s->saved.backend->reservation))||!capture_sources(s,1))return 0;
 return 1;
}
/* Numeric captured guards remain readable after source staleness. No master,
 * project table, cache lease or unpublished holder needs dereferencing here. */
static int output_apart(struct pt_editor_mixed_readers_prepare *s,const void *p,size_t n)
{
 unsigned i;if(!n||!span(p,n)||!apart(p,n,s,sizeof(*s)))return 0;
 for(i=0;i<s->guard_count;++i)if(!apart(p,n,s->guards[i].data,s->guards[i].bytes))return 0;
 for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)
    if(!apart(p,n,s->ordinary[i].data,s->ordinary[i].bytes))return 0;
 for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP;++i)
    if(!apart(p,n,s->chip[i].data,s->chip[i].bytes))return 0;
 return !s->queue||pt_mixed_readers_output_disjoint(s->queue,p,n);
}
static int allocation_apart(struct pt_editor_mixed_readers_prepare *s,const void *p,size_t n)
{
 unsigned i;if(!output_apart(s,p,n))return 0;
 for(i=0;i<s->source_mutable_count;++i)
    if(!apart(p,n,s->source_mutable[i].data,s->source_mutable[i].bytes))return 0;
 return 1;
}
/* Local scratch is not initialized before its whole span is admitted. Accept
 * only the numeric address value, then use the identical full-width readonly
 * output guard. Neither helper reads or writes the pointed-to scratch bytes. */
static int local_apart(struct pt_editor_mixed_readers_prepare *s,uintptr_t address,size_t n)
{return output_apart(s,(const void *)address,n);}
static int remember(struct pt_sampler_storage_span *v,unsigned count,void *p,size_t n)
{unsigned i;for(i=0;i<count;++i)if(!v[i].data){v[i]=(struct pt_sampler_storage_span){p,n};return 1;}return 0;}
static int retire(struct pt_sampler_storage_span *v,unsigned count,void *p,size_t n,unsigned exact)
{unsigned i;
 for(i=0;i<count;++i)if(v[i].data==p&&(!exact||v[i].bytes==n)){
    v[i]=(struct pt_sampler_storage_span){NULL,0};return 1;
 }
 return 0;}
static void *guard_allocate(void *context,size_t n)
{
 struct pt_editor_mixed_readers_prepare *s=context;void *p;unsigned before=s->reentries;
 if(!s->busy||s->closing||s->first_error||!fixed_current(s))return NULL;
 p=s->saved.activation.allocator.allocate(s->saved.activation.allocator.context,n);
 if(!p)return NULL;
 if(!allocation_apart(s,p,n)){fail(s,PT_EDITOR_MIXED_READERS_FAULT);return NULL;}
 if(!remember(s->ordinary,PT_EDITOR_MIXED_READERS_ORDINARY,p,n)){
    fail(s,PT_EDITOR_MIXED_READERS_FAULT);
    s->saved.activation.allocator.release(s->saved.activation.allocator.context,p);return NULL;}
 if(before!=s->reentries||s->first_error||!fixed_current(s)){
    fail(s,PT_EDITOR_MIXED_READERS_FAULT);
    (void)retire(s->ordinary,PT_EDITOR_MIXED_READERS_ORDINARY,p,0,0);
    s->saved.activation.allocator.release(s->saved.activation.allocator.context,p);return NULL;}
 return p;
}
static void guard_release(void *context,void *p)
{
 struct pt_editor_mixed_readers_prepare *s=context;
 if(!s->busy||!retire(s->ordinary,PT_EDITOR_MIXED_READERS_ORDINARY,p,0,0)){
    fail(s,PT_EDITOR_MIXED_READERS_FAULT);return;}
 /* Child close consumes its actual local slot before this callback. Publish
  * that consumption in the controller BEFORE user release/reentry/free, too. */
 if(p==s->activation)s->activation=NULL;
 if(p==s->queue)s->queue=NULL;
 if(p==s->pool)s->pool=NULL;
 /* Retire before the callback, so no reentrant observer sees fresh ownership. */
 s->saved.activation.allocator.release(s->saved.activation.allocator.context,p);
 if(!fixed_current(s))fail(s,PT_EDITOR_MIXED_READERS_STALE);
}
static void *guard_chip_allocate(void *context,size_t n)
{
 struct pt_editor_mixed_readers_prepare *s=context;void *p;unsigned before=s->reentries;
 if(!s->busy||s->closing||s->first_error||!fixed_current(s))return NULL;
 p=s->saved.chip_allocate(s->saved.chip_context,n);if(!p)return NULL;
 if(!allocation_apart(s,p,n)){fail(s,PT_EDITOR_MIXED_READERS_FAULT);return NULL;}
 if(!remember(s->chip,PT_EDITOR_MIXED_READERS_CHIP,p,n)){
    fail(s,PT_EDITOR_MIXED_READERS_FAULT);s->saved.chip_release(s->saved.chip_context,p,n);return NULL;}
 if(before!=s->reentries||s->first_error||!fixed_current(s)){
    fail(s,PT_EDITOR_MIXED_READERS_FAULT);
    (void)retire(s->chip,PT_EDITOR_MIXED_READERS_CHIP,p,n,1);
    s->saved.chip_release(s->saved.chip_context,p,n);return NULL;}
 return p;
}
static void guard_chip_release(void *context,void *p,size_t n)
{
 struct pt_editor_mixed_readers_prepare *s=context;
 if(!s->busy||!retire(s->chip,PT_EDITOR_MIXED_READERS_CHIP,p,n,1)){
    fail(s,PT_EDITOR_MIXED_READERS_FAULT);return;}
 s->saved.chip_release(s->saved.chip_context,p,n);
 if(!fixed_current(s))fail(s,PT_EDITOR_MIXED_READERS_STALE);
}
static int hook_owned(struct pt_editor_mixed_readers_prepare *s)
{struct pt_editor_mixed *o=s->saved.binding;
 return (!s->source_mode||source_identity(s))&&o->editor==s->editor&&pt_editor_mixed_attached(o)&&!o->owner&&!o->transport&&
 !o->owner_finish&&!o->owner_finish_context&&o->preparation_close==finish&&o->preparation_context==s;}
static int post(struct pt_editor_mixed_readers_prepare *s)
{if(!fixed_current(s)){fail(s,PT_EDITOR_MIXED_READERS_STALE);return 0;}return !s->first_error;}
struct callback_snapshot {
 struct pt_editor_mixed binding;struct pt_sampler sampler;struct pt_project project;
 struct pt_editor_mixed_readers_prepare_inputs inputs;const struct pt_project *identity;
 uint32_t revision;unsigned project_present,reentries;
};
/* Task-only callback comparison uses fixed headers, never former tables. It
 * compares the actual state at this call, allowing unchanged captured stale
 * owners to drain through later independent exact proofs. */
static void snapshot(struct pt_editor_mixed_readers_prepare *s,struct callback_snapshot *v)
{
 memset(v,0,sizeof(*v));memcpy(&v->binding,s->saved.binding,sizeof(v->binding));
 memcpy(&v->sampler,&s->editor->sampler,sizeof(v->sampler));memcpy(&v->inputs,s->inputs,sizeof(v->inputs));
 v->identity=s->editor->project;v->revision=s->editor->history.revision;v->reentries=s->reentries;
 if(v->identity==s->project){memcpy(&v->project,s->project,sizeof(v->project));v->project_present=1;}
}
static void callback_fault(struct pt_editor_mixed_readers_prepare *s,
 const struct callback_snapshot *before)
{
 struct callback_snapshot after;snapshot(s,&after);
 if(memcmp(before,&after,sizeof(after))){
    fail(s,PT_EDITOR_MIXED_READERS_FAULT);
    /* Preserve the callback's actual raw outcome. fail() uses only the explicit
     * live-owner fault latch/serial seam; no cancellation/effects/proof. */
 }
}
/* These two forwarders are also called by fire. They read copied callback
 * identities only: no editor/project/holder/domain/span traversal. */
static int guard_port_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
 struct pt_editor_mixed_readers_prepare *s=((struct pt_editor_mixed_readers_callback *)context)->owner;
 return s->saved.activation.port.read_clock(s->saved.activation.port.context,ticks,frequency);
}
static int guard_port_commit(void *context,const struct pt_mixed_activation_packet *packet,struct pt_mixed_activation_actual *actual)
{
 struct pt_editor_mixed_readers_prepare *s=((struct pt_editor_mixed_readers_callback *)context)->owner;
 return s->saved.activation.port.commit(s->saved.activation.port.context,packet,actual);
}
static int guard_port_publish(void *context,struct pt_mixed_readers_activation *owner,const struct pt_mixed_activation_packet *packet)
{
 struct pt_editor_mixed_readers_prepare *s=((struct pt_editor_mixed_readers_callback *)context)->owner;
 struct callback_snapshot before;int r;
 if(s->first_error||!fixed_current(s)){
    fail(s,PT_EDITOR_MIXED_READERS_STALE);return 0;}
 snapshot(s,&before);r=s->saved.activation.port.publish(s->saved.activation.port.context,owner,packet);
 callback_fault(s,&before);return r;
}
static int guard_port_command(void *context,const struct pt_mixed_activation_command_identity *identity,unsigned cancel)
{
 struct pt_editor_mixed_readers_prepare *s=((struct pt_editor_mixed_readers_callback *)context)->owner;
 struct callback_snapshot before;int r;snapshot(s,&before);
 r=s->saved.activation.port.command_quiet(s->saved.activation.port.context,identity,cancel);
 callback_fault(s,&before);return r;
}
static int guard_port_reader(void *context,const struct pt_mixed_activation_reader_identity *identity,unsigned cancel)
{
 struct pt_editor_mixed_readers_prepare *s=((struct pt_editor_mixed_readers_callback *)context)->owner;
 struct callback_snapshot before;int r;snapshot(s,&before);
 r=s->saved.activation.port.reader_quiet(s->saved.activation.port.context,identity,cancel);
 callback_fault(s,&before);return r;
}
static int guard_port_source_close(void *context,const struct pt_mixed_activation_registration *identity)
{
 struct pt_editor_mixed_readers_prepare *s=((struct pt_editor_mixed_readers_callback *)context)->owner;
 struct callback_snapshot before;int r;snapshot(s,&before);
 r=s->saved.activation.port.source_close(s->saved.activation.port.context,identity);
 callback_fault(s,&before);return r;
}
static int guard_port_source_quiet(void *context,const struct pt_mixed_activation_registration *identity)
{
 struct pt_editor_mixed_readers_prepare *s=((struct pt_editor_mixed_readers_callback *)context)->owner;
 struct callback_snapshot before;int r;snapshot(s,&before);
 r=s->saved.activation.port.source_quiet(s->saved.activation.port.context,identity);
 callback_fault(s,&before);return r;
}
static enum pt_editor_mixed_readers_result factory_result(struct pt_editor_mixed_readers_prepare *s,
 enum pt_sampler_mixed_result r)
{switch(r){case PT_SAMPLER_MIXED_OK:return PT_EDITOR_MIXED_READERS_OPEN;
 case PT_SAMPLER_MIXED_PENDING:return PT_EDITOR_MIXED_READERS_PENDING;
 case PT_SAMPLER_MIXED_CAPACITY:return PT_EDITOR_MIXED_READERS_CAPACITY;
 case PT_SAMPLER_MIXED_INVALID:return PT_EDITOR_MIXED_READERS_INVALID;
 case PT_SAMPLER_MIXED_STALE:return fail(s,PT_EDITOR_MIXED_READERS_STALE);
 default:return fail(s,PT_EDITOR_MIXED_READERS_FAULT);}}

static int prepare_admit(struct pt_editor_mixed_readers_prepare *s,
 const struct pt_editor_mixed_readers_prepare_inputs *in)
{
 struct pt_editor_mixed_readers_prepare_inputs v;struct pt_editor_mixed *o;
 struct pt_sampler_storage_span control[5];const void *opaque[7];size_t sizes[7];unsigned i,j;
 if(!span(s,sizeof(*s))||!s||(uintptr_t)s%ALIGN_OF(struct pt_editor_mixed_readers_prepare)||
    !span(in,sizeof(*in))||!in||!apart(s,sizeof(*s),in,sizeof(*in)))return 0;
 memcpy(&v,in,sizeof(v));o=v.binding;
 if(!span(o,sizeof(*o))||!o||(uintptr_t)o%ALIGN_OF(struct pt_editor_mixed)||
    !span(o->editor,sizeof(*o->editor))||!o->editor||(uintptr_t)o->editor%ALIGN_OF(struct pt_editor)||
    !pt_editor_mixed_attached(o)||!span(v.backend,sizeof(*v.backend))||!v.backend||
    !span(v.backend->reservation,sizeof(*v.backend->reservation))||!v.backend->reservation||
    !inside(v.contexts,s,sizeof(*s))||!source_apart(o,v.contexts.data,v.contexts.bytes)||
    !source_apart(o,in,sizeof(*in))||!source_apart(o,v.backend,sizeof(*v.backend))||
    !source_apart(o,v.backend->reservation,sizeof(*v.backend->reservation))||
    !v.activation.allocator.allocate||!v.activation.allocator.release||
    !inside(v.contexts,v.activation.allocator_context.data,v.activation.allocator_context.bytes)||
    (v.activation.allocator.context&&!inside((struct pt_sampler_storage_span){v.activation.allocator_context.data,
       v.activation.allocator_context.bytes},v.activation.allocator.context,1))||
    !v.chip_allocate||!v.chip_release||!v.activation.port.context||!v.activation.port.context_bytes||
    !v.activation.port.read_clock||!v.activation.port.publish||!v.activation.port.commit||
    !v.activation.port.command_quiet||!v.activation.port.reader_quiet||
    !v.activation.port.source_close||!v.activation.port.source_quiet||
    v.activation.port.version!=PT_MIXED_ACTIVATION_PORT_VERSION||
    v.activation.port.flags!=PT_MIXED_ACTIVATION_PORT_REQUIRED||
    !v.activation.session||!v.activation.grid.generation||
    v.factory_budget<pt_sampler_mixed_pool_size()||!v.chip_budget||
    !span(v.activation_workspace,v.activation_capacity)||!v.activation_workspace||
    v.activation_capacity<pt_mixed_activation_workspace_size()||
    (uintptr_t)v.activation_workspace%pt_mixed_activation_workspace_alignment()||
    !span(v.factory_workspace,v.factory_capacity)||!v.factory_workspace||
    v.factory_capacity<pt_sampler_mixed_workspace_size()||
    (uintptr_t)v.factory_workspace%pt_sampler_mixed_workspace_alignment()||
    !source_apart(o,v.activation_workspace,v.activation_capacity)||
    !source_apart(o,v.factory_workspace,v.factory_capacity)||
    !apart(v.activation_workspace,v.activation_capacity,v.factory_workspace,v.factory_capacity)||
    !apart(v.activation_workspace,v.activation_capacity,v.contexts.data,v.contexts.bytes)||
    !apart(v.factory_workspace,v.factory_capacity,v.contexts.data,v.contexts.bytes)||
    !zero(v.activation_workspace,pt_mixed_activation_workspace_size())||
    !zero(v.factory_workspace,pt_sampler_mixed_workspace_size()))return 0;
 control[0]=(struct pt_sampler_storage_span){o,sizeof(*o)};
 control[1]=(struct pt_sampler_storage_span){o->editor,sizeof(*o->editor)};
 control[2]=(struct pt_sampler_storage_span){v.backend,sizeof(*v.backend)};
 control[3]=(struct pt_sampler_storage_span){v.backend->reservation,sizeof(*v.backend->reservation)};
 control[4]=(struct pt_sampler_storage_span){in,sizeof(*in)};
 for(i=0;i<5;++i){
    if(i<4&&!apart(v.contexts.data,v.contexts.bytes,control[i].data,control[i].bytes))return 0;
    if(!apart(v.activation_workspace,v.activation_capacity,control[i].data,control[i].bytes)||
       !apart(v.factory_workspace,v.factory_capacity,control[i].data,control[i].bytes))return 0;
    for(j=i+1;j<5;++j)if(!apart(control[i].data,control[i].bytes,control[j].data,control[j].bytes))return 0;}
 opaque[0]=v.activation.allocator.context;opaque[1]=o->editor->sampler.allocator.context;
 opaque[2]=o->editor->sampler.progress_context;opaque[3]=v.chip_context;
 opaque[4]=v.activation.port.context;opaque[5]=v.backend->context;opaque[6]=v.backend->reservation->api.context;
 for(i=0;i<7;++i){sizes[i]=i==4?v.activation.port.context_bytes:1;
    if(opaque[i]&&(!(i>=5?callback_covered(&v,opaque[i],sizes[i]):inside(v.contexts,opaque[i],sizes[i]))||
       !apart(opaque[i],sizes[i],s,sizeof(*s))||!apart(opaque[i],sizes[i],in,sizeof(*in))))return 0;}
 if(v.backend->arena.context&&(!callback_covered(&v,v.backend->arena.context,1)||
    !apart(v.backend->arena.context,1,s,sizeof(*s))||!apart(v.backend->arena.context,1,in,sizeof(*in))))return 0;
 if(v.backend->cache.context&&(!callback_covered(&v,v.backend->cache.context,1)||
    !apart(v.backend->cache.context,1,s,sizeof(*s))||!apart(v.backend->cache.context,1,in,sizeof(*in))))return 0;
 return 1;
}

/* Fixed controller-issued source identity. No outer callback or external
 * establishment/audit/producer symbol is linked by this controller. */
static uint64_t source_next_serial=1;
static int source_identity(struct pt_editor_mixed_readers_prepare *s)
{
 const struct pt_editor_mixed_source_borrow *b=s->source_publisher;
 if(!s->source_mode||!s->source_serial||!b||s->saved.binding!=s->source_binding)return 0;
 if(s->source_held)return !s->source_released&&b->address==s&&b->serial==s->source_serial;
 return s->source_released&&!b->address&&!b->serial;
}
static int source_original(struct pt_editor_mixed_readers_prepare *s,
 const struct pt_editor_mixed_source_borrow *b)
{return s&&b&&b==s->source_publisher&&s->source_held&&source_identity(s)&&hook_owned(s);}
static int source_descriptor(struct pt_editor_mixed_readers_prepare *s)
{
 const struct pt_editor_mixed_source_inputs *v=s->source_inputs;
 if(!source_identity(s)||!v||v->binding!=s->source_binding||
    v->preparation!=s->source_preparation||v->contexts.data!=s->source_contexts.data||
    v->contexts.bytes!=s->source_contexts.bytes||
    v->activation_workspace.data!=s->source_activation_workspace.data||
    v->activation_workspace.bytes!=s->source_activation_workspace.bytes||
    v->factory_workspace.data!=s->source_factory_workspace.data||
    v->factory_workspace.bytes!=s->source_factory_workspace.bytes||
    v->backend_parent.data!=s->source_backend_parent.data||
    v->backend_parent.bytes!=s->source_backend_parent.bytes||
    v->immutable_count!=s->source_immutable_count||
    v->mutable_count!=s->source_mutable_count||
    memcmp(v->immutable,s->source_immutable,sizeof(s->source_immutable))||
    memcmp(v->mutable,s->source_mutable,sizeof(s->source_mutable)))return 0;
 return 1;
}
static int source_fixed(struct pt_editor_mixed_readers_prepare *s)
{
 struct pt_editor_mixed *o=s->source_binding;struct pt_sampler m;struct pt_project h;unsigned i;
 if(!source_descriptor(s)||!o||!hook_owned(s)||o->editor!=s->editor||
    s->editor->project!=s->project||s->editor->history.revision!=s->revision||
    s->editor->sampler.generation!=s->generation)return 0;
 if(s->source_activated)return fixed_current(s);
 /* Only genuine producer calls may establish previously absent current slots.
  * The seam permits their header changes; it does not certify their semantics.
  * Existing current identities and all fixed allocator/table/budget fields stay.
  * No former descriptor table or value is traversed by this fixed check. */
 memcpy(&m,&s->editor->sampler,sizeof(m));
 if(m.bytes<s->sampler_header.bytes||m.bytes>m.budget)return 0;
 for(i=0;i<PT_PROJECT_SAMPLES;++i){
    if(s->sampler_header.current[i]&&m.current[i]!=s->sampler_header.current[i])return 0;
    m.current[i]=s->sampler_header.current[i];}
 m.bytes=s->sampler_header.bytes;
 if(memcmp(&m,&s->sampler_header,sizeof(m)))return 0;
 memcpy(&h,&s->project_header,sizeof(h));h.channels.selected=s->project->channels.selected;
 return pt_project_snapshot_equal(s->project,&h);
}
static int source_empty(struct pt_editor_mixed_readers_prepare *s)
{
 unsigned i;if(s->activation||s->queue||s->pool)return 0;
 for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(s->command[i].handle.address||s->command[i].handle.token)return 0;
 for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(s->reader[i].handle.address||s->reader[i].handle.token)return 0;
 for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)if(s->ordinary[i].data||s->ordinary[i].bytes)return 0;
 for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP;++i)if(s->chip[i].data||s->chip[i].bytes)return 0;
 return 1;
}
/* Named full immutable roles are the only construction-storage exemptions.
 * No future preparation fields or pointed-to backend/reservation bytes are read
 * at SOURCE begin. A full parent is never replaced by a typed child extent. */
static int source_role_declared(const struct pt_editor_mixed_source_inputs *v,
 struct pt_sampler_storage_span role)
{
 unsigned i;if(!role.bytes||!span(role.data,role.bytes))return 0;
 for(i=0;i<v->immutable_count;++i)if(v->immutable[i].data==role.data&&v->immutable[i].bytes==role.bytes)return 1;
 return 0;
}
static int source_role_apart(struct pt_editor_mixed_readers_prepare *s,const void *p,size_t bytes,
 struct pt_sampler_storage_span permitted)
{
 unsigned i;if(s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS||
    !inside(permitted,p,bytes)||!apart(p,bytes,s,sizeof(*s)))return 0;
 for(i=0;i<s->guard_count;++i){const struct pt_sampler_storage_span *g=s->guards+i;
    if(g->data==permitted.data&&g->bytes==permitted.bytes)continue;
    if(!apart(p,bytes,g->data,g->bytes))return 0;}
 for(i=0;i<s->source_mutable_count;++i)
    if(!apart(p,bytes,s->source_mutable[i].data,s->source_mutable[i].bytes))return 0;
 return 1;
}
static int source_roles_current(struct pt_editor_mixed_readers_prepare *s,
 const struct pt_editor_mixed_readers_prepare_inputs *in)
{
 struct pt_amigus_wavetable_cache *backend=in->backend;
 struct pt_amigus_reservation *reservation;
 /* Exact whole workspace identities precede the old zero-read admission.
  * Admit the backend object numerically before its reservation pointer read. */
 if(in->activation_workspace!=s->source_activation_workspace.data||
    in->activation_capacity!=s->source_activation_workspace.bytes||
    in->factory_workspace!=s->source_factory_workspace.data||
    in->factory_capacity!=s->source_factory_workspace.bytes||
    !source_role_apart(s,in->activation_workspace,in->activation_capacity,s->source_activation_workspace)||
    !source_role_apart(s,in->factory_workspace,in->factory_capacity,s->source_factory_workspace)||
    !backend||(uintptr_t)backend%ALIGN_OF(struct pt_amigus_wavetable_cache)||
    !source_role_apart(s,backend,sizeof(*backend),s->source_backend_parent))return 0;
 reservation=backend->reservation;
 return reservation&&!((uintptr_t)reservation%ALIGN_OF(struct pt_amigus_reservation))&&
    source_role_apart(s,reservation,sizeof(*reservation),s->source_backend_parent);
}
enum pt_editor_mixed_readers_result pt_editor_mixed_source_begin(
 struct pt_editor_mixed_readers_prepare *s,const struct pt_editor_mixed_source_inputs *in,
 struct pt_editor_mixed_source_borrow *out)
{
 struct pt_editor_mixed_source_inputs v;struct pt_editor_mixed *o;
 struct pt_sampler_storage_span declared[PT_EDITOR_MIXED_SOURCE_SPANS];unsigned i,j,n;
 if(!s||!in||!out||!span(s,sizeof(*s))||!span(in,sizeof(*in))||!span(out,sizeof(*out))||
    (uintptr_t)s%ALIGN_OF(struct pt_editor_mixed_readers_prepare)||
    (uintptr_t)in%ALIGN_OF(struct pt_editor_mixed_source_inputs)||
    (uintptr_t)out%ALIGN_OF(struct pt_editor_mixed_source_borrow)||
    !apart(s,sizeof(*s),in,sizeof(*in))||!apart(s,sizeof(*s),out,sizeof(*out))||
    !apart(in,sizeof(*in),out,sizeof(*out))||!zero(s,sizeof(*s))||out->address||out->serial||
    !source_next_serial||source_next_serial==UINT64_MAX)return PT_EDITOR_MIXED_READERS_INVALID;
 memcpy(&v,in,sizeof(v));o=v.binding;
 if(!o||!span(o,sizeof(*o))||(uintptr_t)o%ALIGN_OF(struct pt_editor_mixed)||!o->editor||
    !span(o->editor,sizeof(*o->editor))||(uintptr_t)o->editor%ALIGN_OF(struct pt_editor)||
    !pt_editor_mixed_attached(o)||!idle(o)||!v.preparation||!span(v.preparation,sizeof(*v.preparation))||
    (uintptr_t)v.preparation%ALIGN_OF(struct pt_editor_mixed_readers_prepare_inputs)||
    !inside(v.contexts,s,sizeof(*s))||!source_apart(o,v.contexts.data,v.contexts.bytes)||
    !source_apart(o,in,sizeof(*in))||!source_apart(o,out,sizeof(*out))||
    !source_apart(o,v.preparation,sizeof(*v.preparation))||
    !apart(v.contexts.data,v.contexts.bytes,in,sizeof(*in))||
    !apart(v.contexts.data,v.contexts.bytes,out,sizeof(*out))||
    !apart(v.preparation,sizeof(*v.preparation),s,sizeof(*s))||
    !apart(v.preparation,sizeof(*v.preparation),in,sizeof(*in))||
    !apart(v.preparation,sizeof(*v.preparation),out,sizeof(*out))||
    (!inside(v.contexts,v.preparation,sizeof(*v.preparation))&&
      !apart(v.contexts.data,v.contexts.bytes,v.preparation,sizeof(*v.preparation)))||
    v.immutable_count>PT_EDITOR_MIXED_SOURCE_SPANS||
    v.mutable_count>PT_EDITOR_MIXED_SOURCE_SPANS-v.immutable_count)return PT_EDITOR_MIXED_READERS_INVALID;
 if(o->editor->sampler.allocator.context&&
    (!inside(v.contexts,o->editor->sampler.allocator.context,1)||
     !apart(o->editor->sampler.allocator.context,1,s,sizeof(*s))))return PT_EDITOR_MIXED_READERS_INVALID;
 if(o->editor->sampler.progress_context&&
    (!inside(v.contexts,o->editor->sampler.progress_context,1)||
     !apart(o->editor->sampler.progress_context,1,s,sizeof(*s))))return PT_EDITOR_MIXED_READERS_INVALID;
 n=v.immutable_count+v.mutable_count;
 for(i=0;i<PT_EDITOR_MIXED_SOURCE_SPANS;++i){
    if(i>=v.immutable_count&&(v.immutable[i].data||v.immutable[i].bytes))return PT_EDITOR_MIXED_READERS_INVALID;
    if(i>=v.mutable_count&&(v.mutable[i].data||v.mutable[i].bytes))return PT_EDITOR_MIXED_READERS_INVALID;}
 for(i=0;i<n;++i){declared[i]=i<v.immutable_count?v.immutable[i]:v.mutable[i-v.immutable_count];
    if(!declared[i].bytes||!source_apart(o,declared[i].data,declared[i].bytes)||
       !apart(declared[i].data,declared[i].bytes,v.contexts.data,v.contexts.bytes)||
       !apart(declared[i].data,declared[i].bytes,in,sizeof(*in))||
       !apart(declared[i].data,declared[i].bytes,out,sizeof(*out))||
       !apart(declared[i].data,declared[i].bytes,v.preparation,sizeof(*v.preparation)))return PT_EDITOR_MIXED_READERS_INVALID;
    for(j=0;j<i;++j)if(!apart(declared[i].data,declared[i].bytes,declared[j].data,declared[j].bytes))return PT_EDITOR_MIXED_READERS_INVALID;}
 if(!source_role_declared(&v,v.activation_workspace)||!source_role_declared(&v,v.factory_workspace)||
    !source_role_declared(&v,v.backend_parent)||
    !apart(v.activation_workspace.data,v.activation_workspace.bytes,v.factory_workspace.data,v.factory_workspace.bytes)||
    !apart(v.activation_workspace.data,v.activation_workspace.bytes,v.backend_parent.data,v.backend_parent.bytes)||
    !apart(v.factory_workspace.data,v.factory_workspace.bytes,v.backend_parent.data,v.backend_parent.bytes))return PT_EDITOR_MIXED_READERS_INVALID;
 s->source_mode=s->source_held=1;s->source_inputs=in;s->source_preparation=v.preparation;
 s->source_publisher=out;s->source_binding=o;s->source_contexts=v.contexts;
 s->source_activation_workspace=v.activation_workspace;s->source_factory_workspace=v.factory_workspace;
 s->source_backend_parent=v.backend_parent;
 s->source_immutable_count=v.immutable_count;s->source_mutable_count=v.mutable_count;
 memcpy(s->source_immutable,v.immutable,sizeof(v.immutable));memcpy(s->source_mutable,v.mutable,sizeof(v.mutable));
 s->saved.binding=o;s->editor=o->editor;s->project=o->editor->project;
 memcpy(&s->project_header,s->project,sizeof(s->project_header));
 memcpy(&s->sampler_header,&s->editor->sampler,sizeof(s->sampler_header));
 s->revision=s->editor->history.revision;s->generation=s->editor->sampler.generation;
 if(!add(s,s->project,sizeof(*s->project))||!add(s,s->editor,sizeof(*s->editor))||
    !add(s,o,sizeof(*o))||!add(s,in,sizeof(*in))||!add(s,v.contexts.data,v.contexts.bytes)||
    !add(s,v.preparation,sizeof(*v.preparation))||!capture_sources(s,0))goto refused;
 for(i=0;i<v.immutable_count;++i)if(!add(s,v.immutable[i].data,v.immutable[i].bytes))goto refused;
 s->source_publisher_guard=s->guard_count;if(!add(s,out,sizeof(*out)))goto refused;
 s->source_serial=source_next_serial++;s->phase=PT_EDITOR_MIXED_READERS_SOURCE;
 s->result=PT_EDITOR_MIXED_READERS_PENDING;s->hook=1;
 o->preparation_close=finish;o->preparation_context=s;
 out->address=s;out->serial=s->source_serial;
 return s->result;
refused:
 memset(s,0,sizeof(*s));return PT_EDITOR_MIXED_READERS_INVALID;
}
enum pt_editor_mixed_readers_result pt_editor_mixed_source_activate(
 struct pt_editor_mixed_readers_prepare *s,const struct pt_editor_mixed_source_borrow *b,
 const struct pt_editor_mixed_readers_prepare_inputs *in)
{
 struct pt_editor_mixed_readers_prepare_inputs old;unsigned count;
 if(!source_original(s,b)||in!=s->source_preparation)return PT_EDITOR_MIXED_READERS_INVALID;
 if(reentry(s))return s->result;
 if(s->source_activated||s->source_cancel_requested||s->first_error||
    !source_fixed(s)||!source_roles_current(s,in)||!prepare_admit(s,in)||in->binding!=s->source_binding||
    in->contexts.data!=s->source_contexts.data||in->contexts.bytes!=s->source_contexts.bytes)
    return PT_EDITOR_MIXED_READERS_INVALID;
 /* Earlier immutable/mutable buffer guards must not be weakened by a later
  * constructor. Workspaces/card/reservation are admitted full original spans. */
 if(!inside(s->source_contexts,in,sizeof(*in))&&
    !apart(in,sizeof(*in),s->source_contexts.data,s->source_contexts.bytes))return PT_EDITOR_MIXED_READERS_INVALID;
 count=s->guard_count;memcpy(&old,&s->saved,sizeof(old));s->inputs=in;memcpy(&s->saved,in,sizeof(s->saved));
 if(!capture(s)){s->guard_count=count;s->inputs=NULL;memcpy(&s->saved,&old,sizeof(old));return PT_EDITOR_MIXED_READERS_INVALID;}
 memcpy(&s->project_header,s->project,sizeof(s->project_header));
 memcpy(&s->sampler_header,&s->editor->sampler,sizeof(s->sampler_header));
 s->allocator=(struct pt_allocator){s,guard_allocate,guard_release};s->callback.owner=s;
 s->source_activated=1;s->source_drained=0;s->phase=PT_EDITOR_MIXED_READERS_ACTIVATION;
 s->result=PT_EDITOR_MIXED_READERS_PENDING;return s->result;
}
int pt_editor_mixed_source_children_closed(struct pt_editor_mixed_readers_prepare *s,
 const struct pt_editor_mixed_source_borrow *b)
{return source_original(s,b)&&!s->busy&&!s->source_busy&&!s->closing&&source_empty(s);}
int pt_editor_mixed_source_enter(const struct pt_editor_mixed_source_borrow *b)
{
 struct pt_editor_mixed_readers_prepare *s;
 if(!b||!b->address||!(s=b->address)||!source_original(s,b))return 0;
 if(reentry(s))return 0;
 if(!source_fixed(s))fail(s,PT_EDITOR_MIXED_READERS_STALE);
 s->source_busy=1;return 1;
}
int pt_editor_mixed_source_leave(const struct pt_editor_mixed_source_borrow *b)
{
 struct pt_editor_mixed_readers_prepare *s;
 if(!b||!b->address||!(s=b->address)||!source_original(s,b)||!s->source_busy)return 0;
 /* Clear only the exact original active latch. Actual external outcomes must
  * already be retained; no callback/child cleanup/source traversal occurs. */
 s->source_busy=0;if(!source_fixed(s))fail(s,PT_EDITOR_MIXED_READERS_STALE);
 return 1;
}
int pt_editor_mixed_source_borrow_close(struct pt_editor_mixed_source_borrow *b)
{
 struct pt_editor_mixed_readers_prepare *s;unsigned i;
 if(!b||!span(b,sizeof(*b))||(uintptr_t)b%ALIGN_OF(struct pt_editor_mixed_source_borrow))return 0;
 if(!b->address&&!b->serial)return 1;
 s=b->address;if(!source_original(s,b)||!pt_editor_mixed_source_children_closed(s,b)||
    s->source_publisher_guard>=s->guard_count)return 0;
 /* Exempt exactly the original separately admitted publisher, never its parent
  * or another guard. Stale cleanup uses only the captured numeric extents. */
 if(s->guards[s->source_publisher_guard].data!=b||s->guards[s->source_publisher_guard].bytes!=sizeof(*b)||
    !apart(b,sizeof(*b),s,sizeof(*s)))return 0;
 for(i=0;i<s->guard_count;++i)if(i!=s->source_publisher_guard&&
    !apart(b,sizeof(*b),s->guards[i].data,s->guards[i].bytes))return 0;
 for(i=0;i<s->source_mutable_count;++i)if(!apart(b,sizeof(*b),s->source_mutable[i].data,s->source_mutable[i].bytes))return 0;
 /* Closure cannot reopen preparation after the producer releases its pins.
  * Actual children are already empty: this is a local cancellation latch only. */
 fail(s,PT_EDITOR_MIXED_READERS_CANCELLED);
 s->source_held=0;s->source_released=s->source_drained=1;
 b->address=NULL;b->serial=0;return 1;
}

/* No mutation/reentry/fault path belongs in these owner-thread queries. First
 * admit the readable original publisher. A zero pair never follows controller,
 * including after its actual terminal lifetime; zero is not issuer proof. */
static enum pt_editor_mixed_source_scope_observation source_query_scope(
 struct pt_editor_mixed_readers_prepare *s,const struct pt_editor_mixed_source_borrow *b)
{
 if(!b||!span(b,sizeof(*b))||(uintptr_t)b%ALIGN_OF(struct pt_editor_mixed_source_borrow))
    return PT_EDITOR_MIXED_SOURCE_SCOPE_INVALID;
 if(!b->address&&!b->serial)return PT_EDITOR_MIXED_SOURCE_SCOPE_ZERO;
 if(!b->address||!b->serial||b->address!=s||!s||!span(s,sizeof(*s))||
    (uintptr_t)s%ALIGN_OF(struct pt_editor_mixed_readers_prepare))return PT_EDITOR_MIXED_SOURCE_SCOPE_INVALID;
 /* Exact original pointer/serial and copied numeric identities precede binding
  * or descriptor checks. A copied publisher/controller cannot touch the latch. */
 if(s->source_publisher!=b||s->source_mode!=1||s->source_held!=1||s->source_released||
    !s->source_serial||s->source_serial!=b->serial||!s->hook||
    !s->source_binding||s->saved.binding!=s->source_binding||!hook_owned(s))
    return PT_EDITOR_MIXED_SOURCE_SCOPE_INVALID;
 return PT_EDITOR_MIXED_SOURCE_SCOPE_HELD;
}
struct pt_editor_mixed_source_observation pt_editor_mixed_source_observe(
 struct pt_editor_mixed_readers_prepare *s,const struct pt_editor_mixed_source_borrow *b)
{
 struct pt_editor_mixed_source_observation out={0};
 out.scope=source_query_scope(s,b);
 if(out.scope==PT_EDITOR_MIXED_SOURCE_SCOPE_HELD){
    out.result=s->result;out.first_error=s->first_error;
    out.cancel_requested=s->source_cancel_requested;out.source_busy=s->source_busy;
    out.controller_busy=s->busy;out.fixed_tags_current=source_fixed(s)?1U:0U;
 }
 return out;
}
static int source_query_extent(const void *p,size_t n,const struct pt_sampler_storage_span *g,
 unsigned empty_allowed)
{
 if(!g->data&&!g->bytes)return empty_allowed!=0;
 return g->data&&g->bytes&&span(g->data,g->bytes)&&apart(p,n,g->data,g->bytes);
}
int pt_editor_mixed_source_allocation_disjoint(struct pt_editor_mixed_readers_prepare *s,
 const struct pt_editor_mixed_source_borrow *b,const void *p,size_t n)
{
 unsigned i;
 if(source_query_scope(s,b)!=PT_EDITOR_MIXED_SOURCE_SCOPE_HELD||!p||!n||!span(p,n)||
    !apart(p,n,s,sizeof(*s))||!s->guard_count||s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS||
    s->source_immutable_count>PT_EDITOR_MIXED_SOURCE_SPANS||
    s->source_mutable_count>PT_EDITOR_MIXED_SOURCE_SPANS-s->source_immutable_count||
    s->source_publisher_guard>=s->guard_count||
    s->guards[s->source_publisher_guard].data!=b||
    s->guards[s->source_publisher_guard].bytes!=sizeof(*b))return 0;
 /* Numeric extents already captured at begin/activate survive tag staleness.
  * Do not call output_apart/allocation_apart: their queue query is wider than
  * this private captured-only predicate. No full vector/control stack copy. */
 for(i=0;i<s->guard_count;++i)if(!source_query_extent(p,n,s->guards+i,0))return 0;
 for(i=0;i<s->source_mutable_count;++i)if(!source_query_extent(p,n,s->source_mutable+i,0))return 0;
 for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)if(!source_query_extent(p,n,s->ordinary+i,1))return 0;
 for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP;++i)if(!source_query_extent(p,n,s->chip+i,1))return 0;
 return 1;
}
static int source_query_command_zero(const struct pt_editor_mixed_command_record *c)
{
 unsigned i;if(c->handle.address||c->handle.token||c->serial||c->ticket||c->count||c->transferred)return 0;
 for(i=0;i<PT_SAMPLER_MIXED_ACTIONS;++i)if(c->reader[i])return 0;
 return 1;
}
static int source_query_reader_zero(const struct pt_editor_mixed_reader_record *r)
{return !r->handle.address&&!r->handle.token&&!r->serial&&!r->ticket&&
 !r->action&&!r->track&&!r->sample&&!r->channel;}
static enum pt_editor_mixed_source_registration_observation source_query_registration(
 uint64_t ref,uint64_t maximum,uint64_t actual,const void *address,uint64_t token,int empty)
{
 if(!ref||ref>maximum)return PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID;
 if(empty)return PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT;
 if(!address||!token||!actual||actual>maximum||actual<ref)
    return PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID;
 return actual==ref?PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT:PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT;
}
enum pt_editor_mixed_source_registration_observation pt_editor_mixed_source_command_registration(
 struct pt_editor_mixed_readers_prepare *s,const struct pt_editor_mixed_source_borrow *b,
 struct pt_editor_mixed_command_ref ref)
{
 const struct pt_editor_mixed_command_record *c;unsigned i;int empty;
 if(source_query_scope(s,b)!=PT_EDITOR_MIXED_SOURCE_SCOPE_HELD||
    ref.slot>=PT_SAMPLER_MIXED_COMMANDS||!ref.serial||ref.serial>s->serial)
    return PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID;
 c=s->command+ref.slot;empty=source_query_command_zero(c);
 if(!empty){
    if(!c->count||c->count>PT_SAMPLER_MIXED_ACTIONS||c->transferred>1||
       (c->transferred?!c->ticket:c->ticket!=0))return PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID;
    for(i=0;i<PT_SAMPLER_MIXED_ACTIONS;++i)if(c->reader[i]>NO_READER)
       return PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID;
 }
 return source_query_registration(ref.serial,s->serial,c->serial,c->handle.address,c->handle.token,empty);
}
enum pt_editor_mixed_source_registration_observation pt_editor_mixed_source_reader_registration(
 struct pt_editor_mixed_readers_prepare *s,const struct pt_editor_mixed_source_borrow *b,
 struct pt_editor_mixed_reader_ref ref)
{
 const struct pt_editor_mixed_reader_record *r;int empty;
 if(source_query_scope(s,b)!=PT_EDITOR_MIXED_SOURCE_SCOPE_HELD||
    ref.slot>=PT_SAMPLER_MIXED_READERS||!ref.serial||ref.serial>s->serial)
    return PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID;
 r=s->reader+ref.slot;empty=source_query_reader_zero(r);
 if(!empty&&(r->action>=PT_SAMPLER_MIXED_ACTIONS||r->track>=PT_SAMPLER_MIXED_ACTIONS||
    r->sample>=PT_PROJECT_SAMPLES||r->channel>1))return PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID;
 return source_query_registration(ref.serial,s->serial,r->serial,r->handle.address,r->handle.token,empty);
}

enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_begin(
 struct pt_editor_mixed_readers_prepare *s,const struct pt_editor_mixed_readers_prepare_inputs *in)
{
 struct pt_editor_mixed_readers_prepare_inputs v;struct pt_editor_mixed *o;
 unsigned i;
 if(!span(s,sizeof(*s))||!s||(uintptr_t)s%ALIGN_OF(struct pt_editor_mixed_readers_prepare)||
    !span(in,sizeof(*in))||!in||!apart(s,sizeof(*s),in,sizeof(*in)))return PT_EDITOR_MIXED_READERS_INVALID;
 /* An adopted genuine controller has already captured its complete input
  * extent. Refuse reuse without touching expired source arrays or workspaces.
  * Only the admitted original input may enter the busy reentry fault path,
  * after its exact captured numeric extent and live fixed hook are checked. */
 if(s->phase){unsigned captured=0;
    if(!s->busy||in!=s->inputs||!s->hook||s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS)
       return PT_EDITOR_MIXED_READERS_INVALID;
    for(i=0;i<s->guard_count;++i)if(s->guards[i].data==in&&s->guards[i].bytes==sizeof(*in))captured=1;
    if(!captured||!hook_owned(s))return PT_EDITOR_MIXED_READERS_INVALID;
    (void)reentry(s);return s->result;
 }
 if(!prepare_admit(s,in))return PT_EDITOR_MIXED_READERS_INVALID;
 memcpy(&v,in,sizeof(v));o=v.binding;
 if(!idle(o)||!zero(s,sizeof(*s)))return PT_EDITOR_MIXED_READERS_INVALID;
 s->inputs=in;memcpy(&s->saved,&v,sizeof(v));s->editor=o->editor;s->project=o->editor->project;
 memcpy(&s->project_header,s->project,sizeof(s->project_header));
 memcpy(&s->sampler_header,&s->editor->sampler,sizeof(s->sampler_header));
 s->revision=s->editor->history.revision;s->generation=s->editor->sampler.generation;
 if(!capture(s)){memset(s,0,sizeof(*s));return PT_EDITOR_MIXED_READERS_INVALID;}
 s->allocator=(struct pt_allocator){s,guard_allocate,guard_release};
 s->callback.owner=s;
 s->phase=PT_EDITOR_MIXED_READERS_ACTIVATION;s->result=PT_EDITOR_MIXED_READERS_PENDING;
 /* No callback or allocation has happened: the existing editor barrier owns
  * cancellation before either child constructor can be entered. */
 o->preparation_close=finish;o->preparation_context=s;s->hook=1;
 return s->result;
}
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_get(struct pt_editor_mixed_readers_prepare *s)
{if(!s||!s->phase)return PT_EDITOR_MIXED_READERS_INVALID;
 if(reentry(s))return s->result;
 if(s->phase==PT_EDITOR_MIXED_READERS_FINISHED)return s->result;
 if(s->source_mode&&!s->source_activated)return PT_EDITOR_MIXED_READERS_INVALID;
 if(!fixed_current(s))return fail(s,PT_EDITOR_MIXED_READERS_STALE);
 return s->first_error?s->result:(s->phase==PT_EDITOR_MIXED_READERS_REQUESTS?PT_EDITOR_MIXED_READERS_OPEN:PT_EDITOR_MIXED_READERS_PENDING);}

enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_advance_validation(
 struct pt_editor_mixed_readers_prepare *s,unsigned work)
{
 struct pt_mixed_activation_config a;struct pt_sampler_mixed_config f;
 struct pt_mixed_readers_activation *activation=NULL;struct pt_mixed_readers_output *queue=NULL;
 struct pt_sampler_mixed_pool *pool=NULL;enum pt_mixed_readers_result ar;enum pt_sampler_mixed_result fr;
 enum pt_editor_mixed_readers_result r;
 if(!work||work>PT_PROJECT_VALIDATION_WORK_MAX)return PT_EDITOR_MIXED_READERS_INVALID;
 r=pt_editor_mixed_readers_prepare_get(s);if(!s||s->first_error||r!=PT_EDITOR_MIXED_READERS_PENDING)return r;
 if(!local_apart(s,(uintptr_t)&a,sizeof(a))||!local_apart(s,(uintptr_t)&f,sizeof(f))||
    !output_apart(s,&activation,sizeof(activation))||!output_apart(s,&queue,sizeof(queue))||
    !output_apart(s,&pool,sizeof(pool)))return PT_EDITOR_MIXED_READERS_INVALID;
 s->busy=1;
 if(s->phase==PT_EDITOR_MIXED_READERS_ACTIVATION){
    memcpy(&a,&s->saved.activation,sizeof(a));a.allocator=s->allocator;
    a.allocator_context=(struct pt_mixed_readers_span){s,sizeof(*s)};
    a.port=(struct pt_mixed_activation_port){&s->callback,sizeof(s->callback),PT_MIXED_ACTIVATION_PORT_VERSION,
       PT_MIXED_ACTIVATION_PORT_REQUIRED,guard_port_clock,guard_port_publish,guard_port_commit,
       guard_port_command,guard_port_reader,guard_port_source_close,guard_port_source_quiet};
    ar=pt_mixed_activation_open(&a,s->saved.activation_workspace,s->saved.activation_capacity,&activation);
    /* BACKEND may publish a retained recovery owner. Actual slot consumption
     * is authoritative even if the callback also faulted this controller. */
    s->activation=activation;
    if(activation){ar=pt_mixed_activation_borrow_queue(activation,&queue);if(ar==PT_MIXED_READERS_OK)s->queue=queue;}
    if(!post(s))r=s->result;
    else if(ar!=PT_MIXED_READERS_OK||!activation||!queue)r=fail(s,PT_EDITOR_MIXED_READERS_FAULT);
    else{s->phase=PT_EDITOR_MIXED_READERS_FACTORY;r=PT_EDITOR_MIXED_READERS_PENDING;}
 }else if(s->phase==PT_EDITOR_MIXED_READERS_FACTORY){
    memset(&f,0,sizeof(f));f.allocator=s->allocator;f.sampler=&s->editor->sampler;f.project=s->project;
    f.queue=s->queue;f.backend=s->saved.backend;f.chip_context=s;
    f.chip_allocate=guard_chip_allocate;f.chip_release=guard_chip_release;
    f.control_budget=s->saved.factory_budget;f.chip_budget=s->saved.chip_budget;
    f.generation=s->saved.activation.grid.generation;
    f.maximum_commands=PT_SAMPLER_MIXED_COMMANDS;f.maximum_readers=PT_SAMPLER_MIXED_READERS;
    f.context_count=4;f.contexts[0]=(struct pt_mixed_readers_span){s->saved.contexts.data,s->saved.contexts.bytes};
    f.contexts[1]=(struct pt_mixed_readers_span){s->activation,pt_mixed_activation_control_size()};
    f.contexts[2]=(struct pt_mixed_readers_span){s->saved.backend,sizeof(*s->saved.backend)};
    f.contexts[3]=(struct pt_mixed_readers_span){s->saved.backend->reservation,sizeof(*s->saved.backend->reservation)};
    fr=pt_sampler_mixed_open(&f,s->revision,s->saved.factory_workspace,s->saved.factory_capacity,&pool);
    s->pool=pool;
    if(!post(s))r=s->result;
    else if(fr!=PT_SAMPLER_MIXED_PENDING||!pool)r=fail(s,PT_EDITOR_MIXED_READERS_FAULT);
    else{s->phase=PT_EDITOR_MIXED_READERS_VALIDATION;r=PT_EDITOR_MIXED_READERS_PENDING;}
 }else if(s->phase==PT_EDITOR_MIXED_READERS_VALIDATION){
    fr=pt_sampler_mixed_advance_validation(s->pool,s->revision,work);
    if(!post(s))r=s->result;
    else if(fr==PT_SAMPLER_MIXED_OK){s->phase=PT_EDITOR_MIXED_READERS_REQUESTS;s->result=PT_EDITOR_MIXED_READERS_OPEN;r=s->result;}
    else if(fr==PT_SAMPLER_MIXED_PENDING)r=PT_EDITOR_MIXED_READERS_PENDING;
    else r=factory_result(s,fr);
 }else r=fail(s,PT_EDITOR_MIXED_READERS_FAULT);
 s->busy=0;return r;
}
static struct pt_editor_mixed_command_record *command(struct pt_editor_mixed_readers_prepare *s,
 struct pt_editor_mixed_command_ref ref)
{if(ref.slot>=PT_SAMPLER_MIXED_COMMANDS||!ref.serial||s->command[ref.slot].serial!=ref.serial||
 !s->command[ref.slot].handle.address)return NULL;
 return s->command+ref.slot;}
static struct pt_editor_mixed_reader_record *reader(struct pt_editor_mixed_readers_prepare *s,
 struct pt_editor_mixed_reader_ref ref)
{if(ref.slot>=PT_SAMPLER_MIXED_READERS||!ref.serial||s->reader[ref.slot].serial!=ref.serial||
 !s->reader[ref.slot].handle.address)return NULL;
 return s->reader+ref.slot;}
enum pt_mixed_readers_result pt_editor_mixed_source_reader_readiness(
 struct pt_editor_mixed_readers_prepare *s,const struct pt_editor_mixed_source_borrow *b,
 struct pt_editor_mixed_reader_ref ref)
{
 struct pt_editor_mixed_reader_record *rr;enum pt_mixed_readers_result r;
 /* Original numeric scope/ref admission precedes any writable fault path.
  * A copied publisher or stale ref cannot fault or clear the genuine latch. */
 if(source_query_scope(s,b)!=PT_EDITOR_MIXED_SOURCE_SCOPE_HELD||
    pt_editor_mixed_source_reader_registration(s,b,ref)!=PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT)
    return PT_MIXED_READERS_INVALID;
 if(reentry(s))return PT_MIXED_READERS_BACKEND;
 if(s->first_error)return PT_MIXED_READERS_BACKEND;
 if(s->source_cancel_requested||s->closing||!s->source_activated||
    s->phase!=PT_EDITOR_MIXED_READERS_REQUESTS||!s->pool||!s->queue)
    return PT_MIXED_READERS_INVALID;
 if(!fixed_current(s)){fail(s,PT_EDITOR_MIXED_READERS_STALE);return PT_MIXED_READERS_STALE;}
 if(!(rr=reader(s,ref))||!rr->ticket)return PT_MIXED_READERS_INVALID;
 s->busy=1;r=pt_sampler_mixed_reader_readiness(s->pool,rr->handle);
 (void)post(s);s->busy=0;
 return s->first_error?PT_MIXED_READERS_BACKEND:r;
}
static int reader_close(struct pt_editor_mixed_readers_prepare *s,unsigned index)
{
 struct pt_sampler_mixed_reader_handle h=s->reader[index].handle;int r;
 if(!h.address)return 1;
 if(!output_apart(s,&h,sizeof(h)))return 0;
 r=pt_sampler_mixed_reader_close(s->pool,&h);
 memcpy(&s->reader[index].handle,&h,sizeof(h));
 if(!h.address&&!h.token)memset(s->reader+index,0,sizeof(s->reader[index]));
 return r&&!h.address&&!h.token;
}
static int command_close(struct pt_editor_mixed_readers_prepare *s,unsigned index)
{
 struct pt_sampler_mixed_command_handle h=s->command[index].handle;int r;
 if(!h.address)return 1;
 if(!output_apart(s,&h,sizeof(h)))return 0;
 r=pt_sampler_mixed_command_close(s->pool,&h);
 memcpy(&s->command[index].handle,&h,sizeof(h));
 if(!h.address&&!h.token)memset(s->command+index,0,sizeof(s->command[index]));
 return r&&!h.address&&!h.token;
}
/* The two original external spans survive all callbacks, not just admission.
 * A callback may append captured numeric guards behind this invocation's pair;
 * remove only our positively matched entries and preserve the whole suffix. */
struct batch_pair {unsigned index;struct pt_sampler_storage_span input,output;};
struct batch_scratch {
 struct pt_sampler_mixed_quantized_batch lower;
 struct pt_editor_mixed_reader_ref refs[PT_SAMPLER_MIXED_ACTIONS];
 struct pt_sampler_mixed_command_handle handle;
 struct pt_sampler_mixed_reader_handle readers[PT_SAMPLER_MIXED_ACTIONS];
 struct pt_mixed_readers_key key;struct batch_pair pair;
};
static int batch_spans(struct pt_editor_mixed_readers_prepare *s,const void *in,size_t bytes,
 struct pt_editor_mixed_command_ref *out,uintptr_t local,size_t local_bytes)
{
 if(s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS||
    (uintptr_t)out%ALIGN_OF(struct pt_editor_mixed_command_ref)||
    !output_apart(s,in,bytes)||!output_apart(s,out,sizeof(*out))||
    !apart(in,bytes,out,sizeof(*out))||!local_apart(s,local,local_bytes)||
    !apart((const void *)local,local_bytes,in,bytes)||
    !apart((const void *)local,local_bytes,out,sizeof(*out)))return 0;
 return 1;
}
static void batch_pair_begin(struct pt_editor_mixed_readers_prepare *s,
 const void *in,size_t bytes,struct pt_editor_mixed_command_ref *out,struct batch_pair *pair)
{
 pair->index=s->guard_count;
 pair->input=(struct pt_sampler_storage_span){in,bytes};
 pair->output=(struct pt_sampler_storage_span){out,sizeof(*out)};
 s->guards[s->guard_count++]=pair->input;
 s->guards[s->guard_count++]=pair->output;
}
static int batch_pair_end(struct pt_editor_mixed_readers_prepare *s,const struct batch_pair *pair)
{
 unsigned i,n=s->guard_count;
 if(n>PT_EDITOR_MIXED_READERS_GUARDS||pair->index>PT_EDITOR_MIXED_READERS_GUARDS-2||
    n<pair->index+2||s->guards[pair->index].data!=pair->input.data||
    s->guards[pair->index].bytes!=pair->input.bytes||
    s->guards[pair->index+1].data!=pair->output.data||
    s->guards[pair->index+1].bytes!=pair->output.bytes){
    fail(s,PT_EDITOR_MIXED_READERS_FAULT);return 0;
 }
 for(i=pair->index+2;i<n;++i)s->guards[i-2]=s->guards[i];
 s->guards[n-2]=(struct pt_sampler_storage_span){NULL,0};
 s->guards[n-1]=(struct pt_sampler_storage_span){NULL,0};
 s->guard_count=n-2;return 1;
}
static int batch_voice_zero(const struct pt_amigus_voice_request *v)
{return !v->rate_numerator&&!v->rate_denominator&&!v->offset&&!v->volume&&!v->pan;}
static int batch_request_zero(const struct pt_editor_mixed_readers_request *x)
{return !x->kind&&!x->track&&!x->sample&&!x->channel&&!x->expected&&
 !x->reader.slot&&!x->reader.serial&&!x->geometry.amigus.bits&&
 !x->geometry.amigus.little_endian&&batch_voice_zero(&x->geometry.amigus.trigger)&&
 !x->geometry.amigus.rate&&!x->geometry.amigus.left&&!x->geometry.amigus.right;}
/* Read only admitted input and fixed captured controller metadata. No former
 * sample table walks or positive ACTIVE getter callbacks happen here. */
static int batch_quantized_metadata(struct pt_editor_mixed_readers_prepare *s,
 const struct pt_editor_mixed_readers_request *in,unsigned count,
 const struct pt_sampler_mixed_trigger_levels *levels,uint64_t frame)
{
 unsigned i,seen=0;
 if(!count||count>PT_SAMPLER_MIXED_ACTIONS||frame==UINT64_MAX)return 0;
 for(i=count;i<PT_SAMPLER_MIXED_ACTIONS;++i)
    if(!batch_request_zero(in+i)||levels[i].mode!=PT_SAMPLER_MIXED_TRIGGER_LEGACY||
       levels[i].left||levels[i].right)return 0;
 for(i=0;i<count;++i){const struct pt_editor_mixed_readers_request *x=in+i;
    const struct pt_sampler_mixed_trigger_levels *l=levels+i;
    struct pt_editor_mixed_reader_record *r;unsigned route;
    if(x->track>=s->project_header.channels.count||x->track>=PT_SAMPLER_MIXED_ACTIONS||
       x->sample>=s->project_header.sample_count||(seen&(1U<<x->track)))return 0;
    seen|=1U<<x->track;route=s->project_header.channels.track[x->track].route;
    if(route!=PT_PAULA&&route!=PT_AMIGUS)return 0;
    if(l->mode==PT_SAMPLER_MIXED_TRIGGER_LEGACY){if(l->left||l->right)return 0;}
    else if(l->mode!=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED||route!=PT_AMIGUS||
       x->kind!=PT_MIXED_READERS_TRIGGER||x->geometry.amigus.trigger.volume||
       x->geometry.amigus.trigger.pan)return 0;
    if(x->kind==PT_MIXED_READERS_TRIGGER){
       if(!x->expected||x->expected!=s->sampler_header.current[x->sample]||x->reader.slot||x->reader.serial)return 0;
       if(route==PT_PAULA){if(!x->geometry.paula.period||x->geometry.paula.volume>64)return 0;}
       else if((x->geometry.amigus.bits!=8&&x->geometry.amigus.bits!=16)||x->geometry.amigus.little_endian>1||
          !x->geometry.amigus.trigger.rate_numerator||!x->geometry.amigus.trigger.rate_denominator||
          x->geometry.amigus.trigger.volume>64||x->geometry.amigus.trigger.pan>256||
          x->geometry.amigus.rate||x->geometry.amigus.left||x->geometry.amigus.right)return 0;
    }else{
       if((x->kind!=PT_MIXED_READERS_CONTROL&&x->kind!=PT_MIXED_READERS_STOP)||x->expected||
          !(r=reader(s,x->reader))||r->track!=x->track||r->sample!=x->sample||r->channel!=x->channel||!r->ticket)return 0;
       if(route==PT_AMIGUS&&(x->geometry.amigus.bits||x->geometry.amigus.little_endian||
          !batch_voice_zero(&x->geometry.amigus.trigger)))return 0;
       if(x->kind==PT_MIXED_READERS_STOP){
          if(route==PT_PAULA){if(x->geometry.paula.period||x->geometry.paula.volume)return 0;}
          else if(x->geometry.amigus.rate||x->geometry.amigus.left||x->geometry.amigus.right)return 0;
       }else if(route==PT_PAULA){if(!x->geometry.paula.period||x->geometry.paula.volume>64)return 0;}
       else if(!x->geometry.amigus.rate||x->geometry.amigus.rate>0x40000000UL)return 0;
    }
 }
 return 1;
}
/* Called only AFTER fixed_current positively verified the fixed controls. */
static int batch_quantized_sources(struct pt_editor_mixed_readers_prepare *s,
 const struct pt_editor_mixed_readers_request *in,unsigned count)
{
 unsigned i;
 for(i=0;i<count;++i)if(in[i].kind==PT_MIXED_READERS_TRIGGER){
    const struct pt_sample *a=s->project_header.samples+in[i].sample;
    if(in[i].channel>=a->pcm.channels)return 0;
    if(s->project_header.channels.track[in[i].track].route==PT_PAULA&&
       (a->loop!=PT_LOOP_NONE||!a->pcm.frames||a->pcm.frames>131070))return 0;
 }
 return 1;
}
static enum pt_editor_mixed_readers_result batch_begin_private(
 struct pt_editor_mixed_readers_prepare *s,uint64_t frame,const struct pt_editor_mixed_readers_request *in,
 unsigned count,const struct pt_sampler_mixed_trigger_levels *levels,const void *original,size_t bytes,
 struct pt_editor_mixed_command_ref *out)
{
 struct batch_scratch b;struct pt_editor_mixed_command_record *c;
 struct pt_editor_mixed_reader_record *rr;enum pt_sampler_mixed_result fr;
 enum pt_editor_mixed_readers_result r;unsigned i,ci,free_readers=0,needed=0;
 if(!s||!s->phase||s->phase==PT_EDITOR_MIXED_READERS_FINISHED||
    (s->source_mode&&!s->source_activated)||!count||count>PT_SAMPLER_MIXED_ACTIONS||
    !batch_spans(s,original,bytes,out,(uintptr_t)&b,sizeof(b)))return PT_EDITOR_MIXED_READERS_INVALID;
 /* Reserve BEFORE scratch writes, writable reentry/fault or callback entry. */
 if(s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS-2)return PT_EDITOR_MIXED_READERS_CAPACITY;
 if(levels){
    if(!batch_quantized_metadata(s,in,count,levels,frame))return PT_EDITOR_MIXED_READERS_INVALID;
    if(!fixed_current(s)){if(reentry(s))return s->result;return fail(s,PT_EDITOR_MIXED_READERS_STALE);}
    if(!batch_quantized_sources(s,in,count))return PT_EDITOR_MIXED_READERS_INVALID;
 }
 r=pt_editor_mixed_readers_prepare_get(s);
 if(r!=PT_EDITOR_MIXED_READERS_OPEN||s->first_error)return r;
 for(ci=0;ci<PT_SAMPLER_MIXED_COMMANDS&&s->command[ci].handle.address;++ci){}
 for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(!s->reader[i].handle.address)++free_readers;
 for(i=0;i<count;++i)if(in[i].kind==PT_MIXED_READERS_TRIGGER)++needed;
 if(ci==PT_SAMPLER_MIXED_COMMANDS||needed>free_readers||s->serial>UINT64_MAX-needed-1)return PT_EDITOR_MIXED_READERS_CAPACITY;
 memset(&b,0,sizeof(b));b.lower.count=count;
 for(i=0;i<count;++i){b.lower.action[i].kind=in[i].kind;b.lower.action[i].track=in[i].track;
    b.lower.action[i].sample=in[i].sample;b.lower.action[i].channel=in[i].channel;
    b.lower.action[i].expected=in[i].expected;
    memcpy(&b.lower.action[i].geometry,&in[i].geometry,sizeof(b.lower.action[i].geometry));
    b.refs[i]=in[i].reader;
    if(in[i].kind==PT_MIXED_READERS_TRIGGER){if(b.refs[i].serial||b.refs[i].slot)return PT_EDITOR_MIXED_READERS_INVALID;}
    else if(in[i].expected||!(rr=reader(s,b.refs[i]))||rr->track!=in[i].track||rr->sample!=in[i].sample||
       rr->channel!=in[i].channel||!rr->ticket)return PT_EDITOR_MIXED_READERS_INVALID;
 }
 if(levels)memcpy(b.lower.levels,levels,sizeof(b.lower.levels));
 s->busy=1;batch_pair_begin(s,original,bytes,out,&b.pair);
 /* Every ref was copied before the first genuine getter/current callback. */
 for(i=0;i<count;++i)if(b.lower.action[i].kind!=PT_MIXED_READERS_TRIGGER){
    rr=reader(s,b.refs[i]);
    if(!rr){r=PT_EDITOR_MIXED_READERS_INVALID;goto complete;}
    {enum pt_mixed_readers_result key_result=pt_sampler_mixed_reader_key(s->pool,rr->handle,&b.key);
     int current=post(s);
     if(key_result!=PT_MIXED_READERS_OK||!current){
       r=s->first_error?s->result:PT_EDITOR_MIXED_READERS_INVALID;goto complete;
     }
    }
    memcpy(&b.lower.action[i].key,&b.key,sizeof(b.key));
 }
 if(levels)fr=pt_sampler_mixed_begin_quantized(s->pool,s->revision,frame,&b.lower,&b.handle);
 else fr=pt_sampler_mixed_begin(s->pool,s->revision,frame,b.lower.action,count,&b.handle);
 /* Actual admitted handles remain registered after an outer callback fault. */
 if(b.handle.address){c=s->command+ci;memset(c,0,sizeof(*c));c->handle=b.handle;c->serial=++s->serial;c->count=count;
    for(i=0;i<count;++i)c->reader[i]=NO_READER;
    for(i=0;i<count;++i)if(b.lower.action[i].kind==PT_MIXED_READERS_TRIGGER){unsigned ri;
       if(pt_sampler_mixed_reader(s->pool,b.handle,i,b.readers+i)!=PT_SAMPLER_MIXED_OK){fail(s,PT_EDITOR_MIXED_READERS_FAULT);break;}
       for(ri=0;ri<PT_SAMPLER_MIXED_READERS&&s->reader[ri].handle.address;++ri){}
       if(ri==PT_SAMPLER_MIXED_READERS){fail(s,PT_EDITOR_MIXED_READERS_FAULT);break;}
       rr=s->reader+ri;memset(rr,0,sizeof(*rr));rr->handle=b.readers[i];rr->serial=++s->serial;
       rr->action=i;rr->track=b.lower.action[i].track;rr->sample=b.lower.action[i].sample;
       rr->channel=b.lower.action[i].channel;c->reader[i]=ri;
    }
 }
 if(!post(s))r=s->result;else r=factory_result(s,fr);
complete:
 if(!batch_pair_end(s,&b.pair))r=s->result;
 if(b.handle.address&&!s->first_error&&r==PT_EDITOR_MIXED_READERS_PENDING){
    if(output_apart(s,out,sizeof(*out)))*out=(struct pt_editor_mixed_command_ref){ci,s->command[ci].serial};
    else r=fail(s,PT_EDITOR_MIXED_READERS_FAULT);
 }
 s->busy=0;return r;
}
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_batch_begin(
 struct pt_editor_mixed_readers_prepare *s,uint64_t frame,const struct pt_editor_mixed_readers_request *in,
 unsigned count,struct pt_editor_mixed_command_ref *out)
{
 if(!count||count>PT_SAMPLER_MIXED_ACTIONS||(uintptr_t)in%ALIGN_OF(struct pt_editor_mixed_readers_request))
    return PT_EDITOR_MIXED_READERS_INVALID;
 return batch_begin_private(s,frame,in,count,NULL,in,(size_t)count*sizeof(*in),out);
}
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_batch_begin_quantized(
 struct pt_editor_mixed_readers_prepare *s,uint64_t frame,
 const struct pt_editor_mixed_readers_quantized_batch *batch,struct pt_editor_mixed_command_ref *out)
{
 /* No batch field is read before its actual full original extent is guarded. */
 if(!s||!s->phase||s->phase==PT_EDITOR_MIXED_READERS_FINISHED||
    s->guard_count>PT_EDITOR_MIXED_READERS_GUARDS||
    (uintptr_t)batch%ALIGN_OF(struct pt_editor_mixed_readers_quantized_batch)||
    (uintptr_t)out%ALIGN_OF(struct pt_editor_mixed_command_ref)||
    !output_apart(s,batch,sizeof(*batch))||!output_apart(s,out,sizeof(*out))||
    !apart(batch,sizeof(*batch),out,sizeof(*out)))return PT_EDITOR_MIXED_READERS_INVALID;
 return batch_begin_private(s,frame,batch->action,batch->count,batch->levels,batch,sizeof(*batch),out);
}
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_batch_advance(
 struct pt_editor_mixed_readers_prepare *s,struct pt_editor_mixed_command_ref ref)
{
 struct pt_editor_mixed_command_record *c;enum pt_sampler_mixed_result fr;enum pt_editor_mixed_readers_result r;
 r=pt_editor_mixed_readers_prepare_get(s);if(!s||r!=PT_EDITOR_MIXED_READERS_OPEN||s->first_error)return r;
 if(!(c=command(s,ref))||c->transferred)return PT_EDITOR_MIXED_READERS_INVALID;
 s->busy=1;fr=pt_sampler_mixed_advance(s->pool,s->revision,c->handle,NULL);
 r=post(s)?factory_result(s,fr):s->result;s->busy=0;return r;
}
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_reader_reference(
 struct pt_editor_mixed_readers_prepare *s,struct pt_editor_mixed_command_ref ref,unsigned action,
 struct pt_editor_mixed_reader_ref *out)
{
 struct pt_editor_mixed_command_record *c;unsigned i;enum pt_editor_mixed_readers_result r;
 if(!s||!s->phase||s->phase==PT_EDITOR_MIXED_READERS_FINISHED||!output_apart(s,out,sizeof(*out)))return PT_EDITOR_MIXED_READERS_INVALID;
 r=pt_editor_mixed_readers_prepare_get(s);if(r!=PT_EDITOR_MIXED_READERS_OPEN||s->first_error)return r;
 if(!(c=command(s,ref))||action>=c->count||(i=c->reader[action])>=PT_SAMPLER_MIXED_READERS||!s->reader[i].handle.address)return PT_EDITOR_MIXED_READERS_INVALID;
 *out=(struct pt_editor_mixed_reader_ref){i,s->reader[i].serial};return PT_EDITOR_MIXED_READERS_OPEN;
}
enum pt_mixed_readers_result pt_editor_mixed_readers_prepare_enqueue(
 struct pt_editor_mixed_readers_prepare *s,struct pt_editor_mixed_command_ref ref,uint64_t *out)
{
 struct pt_editor_mixed_command_record *c;enum pt_mixed_readers_result r;uint64_t ticket=0;unsigned i;
 if(!s||!s->phase||s->phase==PT_EDITOR_MIXED_READERS_FINISHED||!output_apart(s,out,sizeof(*out))||
    !output_apart(s,&ticket,sizeof(ticket)))return PT_MIXED_READERS_INVALID;
 if(pt_editor_mixed_readers_prepare_get(s)!=PT_EDITOR_MIXED_READERS_OPEN||s->first_error||
    !(c=command(s,ref))||c->transferred)return PT_MIXED_READERS_INVALID;
 s->busy=1;r=pt_sampler_mixed_enqueue(s->pool,s->revision,c->handle,&ticket);
 if(r==PT_MIXED_READERS_OK){c->ticket=ticket;c->transferred=1;
    for(i=0;i<c->count;++i)if(c->reader[i]<PT_SAMPLER_MIXED_READERS)s->reader[c->reader[i]].ticket=ticket;}
 (void)post(s);
 /* Exact actual enqueue success/ticket survives a simultaneous outer fault;
  * the complete preflight was performed on the ACTUAL external output. */
 if(r==PT_MIXED_READERS_OK&&output_apart(s,out,sizeof(*out)))*out=ticket;
 s->busy=0;return r==PT_MIXED_READERS_OK?r:(s->first_error?PT_MIXED_READERS_BACKEND:r);
}
enum pt_mixed_readers_result pt_editor_mixed_readers_prepare_publish(
 struct pt_editor_mixed_readers_prepare *s,struct pt_editor_mixed_command_ref ref)
{
 struct pt_editor_mixed_command_record *c;enum pt_mixed_readers_result r;
 if(!s||pt_editor_mixed_readers_prepare_get(s)!=PT_EDITOR_MIXED_READERS_OPEN||s->first_error||
    !(c=command(s,ref))||!c->transferred)return PT_MIXED_READERS_INVALID;
 s->busy=1;r=pt_sampler_mixed_publish(s->pool,c->ticket);(void)post(s);s->busy=0;
 return s->first_error?PT_MIXED_READERS_BACKEND:r;
}
static int service_enter(struct pt_editor_mixed_readers_prepare *s,const void *out,size_t n)
{
 if(!s||!s->phase||s->phase==PT_EDITOR_MIXED_READERS_FINISHED||
    (s->source_mode&&!s->source_activated)||!s->pool)return 0;
 if(out&&!output_apart(s,out,n))return 0;
 if(reentry(s)||!hook_owned(s))return 0;
 if(!fixed_current(s))fail(s,PT_EDITOR_MIXED_READERS_STALE);
 if(out&&s->first_error)return 0;
 s->busy=1;return 1;
}
enum pt_mixed_readers_result pt_editor_mixed_readers_prepare_service_command(
 struct pt_editor_mixed_readers_prepare *s,struct pt_editor_mixed_command_ref ref,unsigned cancel,
 struct pt_mixed_readers_command_receipt *out)
{
 struct pt_editor_mixed_command_record *c;struct pt_mixed_readers_command_receipt receipt;
 enum pt_mixed_readers_result r;unsigned index;
 if(cancel>1||!s||!local_apart(s,(uintptr_t)&receipt,sizeof(receipt))||!service_enter(s,out,sizeof(*out)))return PT_MIXED_READERS_INVALID;
 if(!(c=command(s,ref))||!c->transferred){s->busy=0;return PT_MIXED_READERS_INVALID;}
 index=ref.slot;memset(&receipt,0,sizeof(receipt));
 r=pt_sampler_mixed_service_command(s->pool,c->ticket,cancel,out?&receipt:NULL);
 (void)post(s);
 if(out&&!s->first_error&&r==PT_MIXED_READERS_OK&&output_apart(s,out,sizeof(*out)))memcpy(out,&receipt,sizeof(receipt));
 /* Close may refuse LIVE without callbacks. Consume only actual NULL slots. */
 (void)command_close(s,index);s->busy=0;
 return s->first_error?PT_MIXED_READERS_BACKEND:r;
}
enum pt_mixed_readers_result pt_editor_mixed_readers_prepare_service_reader(
 struct pt_editor_mixed_readers_prepare *s,struct pt_editor_mixed_reader_ref ref,unsigned cancel,
 struct pt_mixed_readers_reader_receipt *out)
{
 struct pt_editor_mixed_reader_record *rr;struct pt_mixed_readers_reader_receipt receipt;enum pt_mixed_readers_result r;
 if(cancel>1||!s||!local_apart(s,(uintptr_t)&receipt,sizeof(receipt))||!service_enter(s,out,sizeof(*out)))return PT_MIXED_READERS_INVALID;
 if(!(rr=reader(s,ref))||!rr->ticket){s->busy=0;return PT_MIXED_READERS_INVALID;}
 memset(&receipt,0,sizeof(receipt));r=pt_sampler_mixed_service_reader(s->pool,rr->ticket,rr->action,cancel,out?&receipt:NULL);
 (void)post(s);
 if(out&&!s->first_error&&r==PT_MIXED_READERS_OK&&output_apart(s,out,sizeof(*out)))memcpy(out,&receipt,sizeof(receipt));
 (void)reader_close(s,ref.slot);s->busy=0;return s->first_error?PT_MIXED_READERS_BACKEND:r;
}
static int local_cancel(struct pt_editor_mixed_readers_prepare *s)
{
 enum pt_mixed_readers_result qr=PT_MIXED_READERS_OK;
 if(s->pool)(void)pt_sampler_mixed_stop(s->pool);
 if(s->queue)qr=pt_mixed_activation_stop(s->activation);
 return qr==PT_MIXED_READERS_OK;
}
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_cancel(struct pt_editor_mixed_readers_prepare *s)
{
 if(!s||!s->phase)return PT_EDITOR_MIXED_READERS_INVALID;
 if(reentry(s))return s->result;
 if(s->phase==PT_EDITOR_MIXED_READERS_FINISHED)return s->result;
 if(!hook_owned(s))return PT_EDITOR_MIXED_READERS_INVALID;
 fail(s,PT_EDITOR_MIXED_READERS_CANCELLED);s->busy=1;
 if(!local_cancel(s))fail(s,PT_EDITOR_MIXED_READERS_FAULT);
 s->busy=0;return s->result;
}
static int finish(void *context)
{
 struct pt_editor_mixed_readers_prepare *s=context;struct pt_sampler_mixed_pool *pool;
 struct pt_mixed_readers_activation *activation;unsigned outer=s->busy,before=s->reentries,i;int all=1,r;
 if(s->source_mode)s->source_cancel_requested=1;
 if(s->source_busy||s->closing||(s->busy&&!s->close_call)){++s->reentries;fail(s,PT_EDITOR_MIXED_READERS_FAULT);return 0;}
 if(!hook_owned(s))return 0;
 if(!local_apart(s,(uintptr_t)&pool,sizeof(pool))||!local_apart(s,(uintptr_t)&activation,sizeof(activation)))return 0;
 if(!s->close_call)fail(s,PT_EDITOR_MIXED_READERS_CANCELLED);
 s->busy=s->closing=1;
 if(!local_cancel(s))all=0;
 /* Cancelled untransferred readers first: command close would otherwise free
  * them and leave external stale reader handles. Retired LIVE readers likewise
  * remain independent of command identity and can outlive that registration. */
 if(s->pool){
    for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(s->reader[i].handle.address&&!reader_close(s,i))all=0;
    for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(s->command[i].handle.address){unsigned j,retained=0;
       if(!s->command[i].transferred)for(j=0;j<s->command[i].count;++j){unsigned ri=s->command[i].reader[j];
          if(ri<PT_SAMPLER_MIXED_READERS&&s->reader[ri].handle.address)retained=1;}
       if(retained||!command_close(s,i))all=0;}
    pool=s->pool;r=pt_sampler_mixed_close(&pool);s->pool=pool;
    if(!r||pool)all=0;
 }
 if(!s->pool){activation=s->activation;r=pt_mixed_activation_close(&activation);
    s->activation=activation;if(!activation)s->queue=NULL;
    if(!r||activation)all=0;}
 if(before!=s->reentries)all=0;
 s->closing=0;s->busy=outer;
 if(!all||s->pool||s->activation)return 0;
 if(s->source_mode){s->source_drained=1;if(s->source_held)return 0;}
 s->phase=PT_EDITOR_MIXED_READERS_FINISHED;s->hook=0;
 if(!s->first_error)s->result=PT_EDITOR_MIXED_READERS_CLOSED;
 return 1;
}
int pt_editor_mixed_readers_prepare_close(struct pt_editor_mixed_readers_prepare *s)
{
 int r;unsigned before;
 if(!s)return 0;
 if(reentry(s))return 0;
 if(!s->phase||s->phase==PT_EDITOR_MIXED_READERS_FINISHED)return 1;
 if(!hook_owned(s))return 0;
 before=s->reentries;s->busy=s->close_call=1;r=pt_editor_mixed_stop(s->saved.binding);
 s->close_call=s->busy=0;
 return r&&s->phase==PT_EDITOR_MIXED_READERS_FINISHED&&!s->hook&&before==s->reentries;
}
