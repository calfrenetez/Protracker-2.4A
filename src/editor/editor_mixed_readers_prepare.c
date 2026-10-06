#include "editor_mixed_readers_prepare.h"
#include "editor_mixed_internal.h"
#include "project_snapshot.h"
#include "sampler_internal.h"
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
static int activation_live(const struct pt_editor_mixed_readers_prepare *s)
{unsigned i;if(!s->activation)return 0;
 for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)
    if(s->ordinary[i].data==s->activation&&s->ordinary[i].bytes==pt_mixed_activation_control_size())return 1;
 return 0;}
static enum pt_editor_mixed_readers_result fail(struct pt_editor_mixed_readers_prepare *s,
 enum pt_editor_mixed_readers_result r)
{if(!s->first_error)s->first_error=r;s->result=s->first_error;
 if((r==PT_EDITOR_MIXED_READERS_FAULT||r==PT_EDITOR_MIXED_READERS_STALE)&&activation_live(s))
    pt_mixed_activation_fail_closed(s->activation);
 if(s->phase!=PT_EDITOR_MIXED_READERS_FINISHED)s->phase=PT_EDITOR_MIXED_READERS_FAILED;
 return s->result;}
static int reentry(struct pt_editor_mixed_readers_prepare *s)
{if(!s->busy)return 0;++s->reentries;fail(s,PT_EDITOR_MIXED_READERS_FAULT);return 1;}
static int idle(const struct pt_editor_mixed *o)
{return !o->owner&&!o->transport&&!o->preparation_close&&!o->preparation_context&&
 !o->owner_finish&&!o->owner_finish_context;}
/* Fixed readable controls only; no former source table or master traversal. */
static int fixed_current(struct pt_editor_mixed_readers_prepare *s)
{
 struct pt_editor_mixed *o=s->saved.binding;struct pt_project h;
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
{if(!span(p,n)||s->guard_count>=PT_EDITOR_MIXED_READERS_GUARDS)return 0;
 if(n)s->guards[s->guard_count++]=(struct pt_sampler_storage_span){p,n};
 return 1;}
static int capture(struct pt_editor_mixed_readers_prepare *s)
{
 struct pt_project *p=s->project;struct pt_sampler *m=&s->editor->sampler;
 struct pt_sampler_storage_span v[PT_SAMPLER_VERSION_SPANS];unsigned i,j,count;
 if(!add(s,p,sizeof(*p))||!add(s,s->editor,sizeof(*s->editor))||
    !add(s,s->saved.binding,sizeof(*s->saved.binding))||
    !add(s,s->inputs,sizeof(*s->inputs))||!add(s,s->saved.contexts.data,s->saved.contexts.bytes)||
    !add(s,s->saved.activation_workspace,s->saved.activation_capacity)||
    !add(s,s->saved.factory_workspace,s->saved.factory_capacity)||
    !add(s,s->saved.backend,sizeof(*s->saved.backend))||
    !add(s,s->saved.backend->reservation,sizeof(*s->saved.backend->reservation))||
    !add(s,p->samples,(size_t)p->sample_count*sizeof(*p->samples))||
    !add(s,p->orders,(size_t)p->order_count*sizeof(*p->orders))||
    !add(s,p->events,(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count*sizeof(*p->events))||
    !add(s,p->extensions,(size_t)p->extension_count*sizeof(*p->extensions))||
    !add(s,m->table,m->table_bytes)||
    !add(s,m->table_original,m->table_original?(size_t)p->sample_count*sizeof(*p->samples):0))return 0;
 for(i=0;i<p->sample_count;++i){const struct pt_sample *a=p->samples+i;
    if(!m->current[i]||a->pcm.capacity>SIZE_MAX/sizeof(int32_t)||
       !add(s,a->pcm.data,a->pcm.capacity*sizeof(int32_t))||
       !add(s,a->slices,(size_t)a->slice_count*sizeof(uint32_t)))return 0;}
 for(i=0;i<p->extension_count;++i)if(!add(s,p->extensions[i].data,p->extensions[i].length))return 0;
 for(i=0;i<PT_PROJECT_SAMPLES;++i)if(m->current[i]){
    if(!pt_sampler_version_spans(m->current[i],v,PT_SAMPLER_VERSION_SPANS,&count))return 0;
    for(j=0;j<count;++j)if(!add(s,v[j].data,v[j].bytes))return 0;}
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
 if(!output_apart(s,p,n)){fail(s,PT_EDITOR_MIXED_READERS_FAULT);return NULL;}
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
 if(!output_apart(s,p,n)){fail(s,PT_EDITOR_MIXED_READERS_FAULT);return NULL;}
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
 return o->editor==s->editor&&pt_editor_mixed_attached(o)&&!o->owner&&!o->transport&&
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

enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_begin(
 struct pt_editor_mixed_readers_prepare *s,const struct pt_editor_mixed_readers_prepare_inputs *in)
{
 struct pt_editor_mixed_readers_prepare_inputs v;struct pt_editor_mixed *o;
 struct pt_sampler_storage_span control[5];const void *opaque[7];size_t sizes[7];unsigned i,j;
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
    !zero(v.factory_workspace,pt_sampler_mixed_workspace_size()))return PT_EDITOR_MIXED_READERS_INVALID;
 control[0]=(struct pt_sampler_storage_span){o,sizeof(*o)};
 control[1]=(struct pt_sampler_storage_span){o->editor,sizeof(*o->editor)};
 control[2]=(struct pt_sampler_storage_span){v.backend,sizeof(*v.backend)};
 control[3]=(struct pt_sampler_storage_span){v.backend->reservation,sizeof(*v.backend->reservation)};
 control[4]=(struct pt_sampler_storage_span){in,sizeof(*in)};
 for(i=0;i<5;++i){
    if(i<4&&!apart(v.contexts.data,v.contexts.bytes,control[i].data,control[i].bytes))return PT_EDITOR_MIXED_READERS_INVALID;
    if(!apart(v.activation_workspace,v.activation_capacity,control[i].data,control[i].bytes)||
       !apart(v.factory_workspace,v.factory_capacity,control[i].data,control[i].bytes))return PT_EDITOR_MIXED_READERS_INVALID;
    for(j=i+1;j<5;++j)if(!apart(control[i].data,control[i].bytes,control[j].data,control[j].bytes))return PT_EDITOR_MIXED_READERS_INVALID;}
 opaque[0]=v.activation.allocator.context;opaque[1]=o->editor->sampler.allocator.context;
 opaque[2]=o->editor->sampler.progress_context;opaque[3]=v.chip_context;
 opaque[4]=v.activation.port.context;opaque[5]=v.backend->context;opaque[6]=v.backend->reservation->api.context;
 for(i=0;i<7;++i){sizes[i]=i==4?v.activation.port.context_bytes:1;
    if(opaque[i]&&(!(i>=5?callback_covered(&v,opaque[i],sizes[i]):inside(v.contexts,opaque[i],sizes[i]))||
       !apart(opaque[i],sizes[i],s,sizeof(*s))||!apart(opaque[i],sizes[i],in,sizeof(*in))))return PT_EDITOR_MIXED_READERS_INVALID;}
 if(v.backend->arena.context&&(!callback_covered(&v,v.backend->arena.context,1)||
    !apart(v.backend->arena.context,1,s,sizeof(*s))||!apart(v.backend->arena.context,1,in,sizeof(*in))))return PT_EDITOR_MIXED_READERS_INVALID;
 if(v.backend->cache.context&&(!callback_covered(&v,v.backend->cache.context,1)||
    !apart(v.backend->cache.context,1,s,sizeof(*s))||!apart(v.backend->cache.context,1,in,sizeof(*in))))return PT_EDITOR_MIXED_READERS_INVALID;
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
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_batch_begin(
 struct pt_editor_mixed_readers_prepare *s,uint64_t frame,const struct pt_editor_mixed_readers_request *in,
 unsigned count,struct pt_editor_mixed_command_ref *out)
{
 struct pt_sampler_mixed_request x[PT_SAMPLER_MIXED_ACTIONS];struct pt_sampler_mixed_command_handle h={NULL,0};
 struct pt_sampler_mixed_reader_handle handles[PT_SAMPLER_MIXED_ACTIONS];
 struct pt_editor_mixed_command_record *c;struct pt_editor_mixed_reader_record *rr;
 struct pt_mixed_readers_key key;enum pt_sampler_mixed_result fr;enum pt_editor_mixed_readers_result r;
 unsigned i,ci,free_readers=0,needed=0;size_t bytes;
 if(!s||!s->phase||s->phase==PT_EDITOR_MIXED_READERS_FINISHED||!count||count>PT_SAMPLER_MIXED_ACTIONS)return PT_EDITOR_MIXED_READERS_INVALID;
 bytes=(size_t)count*sizeof(*in);
 if(!output_apart(s,in,bytes)||!output_apart(s,out,sizeof(*out))||!apart(in,bytes,out,sizeof(*out))||
    !output_apart(s,x,sizeof(x))||!output_apart(s,&h,sizeof(h))||
    !output_apart(s,handles,sizeof(handles))||!local_apart(s,(uintptr_t)&key,sizeof(key)))return PT_EDITOR_MIXED_READERS_INVALID;
 r=pt_editor_mixed_readers_prepare_get(s);if(r!=PT_EDITOR_MIXED_READERS_OPEN||s->first_error)return r;
 for(ci=0;ci<PT_SAMPLER_MIXED_COMMANDS&&s->command[ci].handle.address;++ci){}
 for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(!s->reader[i].handle.address)++free_readers;
 for(i=0;i<count;++i)if(in[i].kind==PT_MIXED_READERS_TRIGGER)++needed;
 if(ci==PT_SAMPLER_MIXED_COMMANDS||needed>free_readers||s->serial>UINT64_MAX-needed-1)return PT_EDITOR_MIXED_READERS_CAPACITY;
 memset(x,0,sizeof(x));memset(handles,0,sizeof(handles));
 for(i=0;i<count;++i){x[i].kind=in[i].kind;x[i].track=in[i].track;x[i].sample=in[i].sample;x[i].channel=in[i].channel;
    x[i].expected=in[i].expected;memcpy(&x[i].geometry,&in[i].geometry,sizeof(x[i].geometry));
    if(in[i].kind==PT_MIXED_READERS_TRIGGER){if(in[i].reader.serial||in[i].reader.slot)return PT_EDITOR_MIXED_READERS_INVALID;}
    else if(in[i].expected||!(rr=reader(s,in[i].reader))||rr->track!=in[i].track||rr->sample!=in[i].sample||
       rr->channel!=in[i].channel||!rr->ticket)return PT_EDITOR_MIXED_READERS_INVALID;
 }
 s->busy=1;
 /* Positive current ACTIVE keys are obtained here, never accepted externally. */
 for(i=0;i<count;++i)if(x[i].kind!=PT_MIXED_READERS_TRIGGER){
    rr=reader(s,in[i].reader);
    if(pt_sampler_mixed_reader_key(s->pool,rr->handle,&key)!=PT_MIXED_READERS_OK||!post(s)){
       s->busy=0;return s->first_error?s->result:PT_EDITOR_MIXED_READERS_INVALID;}
    memcpy(&x[i].key,&key,sizeof(key));}
 fr=pt_sampler_mixed_begin(s->pool,s->revision,frame,x,count,&h);
 /* An admitted genuine handle is recorded even after outer callback fault. */
 if(h.address){c=s->command+ci;memset(c,0,sizeof(*c));c->handle=h;c->serial=++s->serial;c->count=count;
    for(i=0;i<count;++i)c->reader[i]=NO_READER;
    for(i=0;i<count;++i)if(x[i].kind==PT_MIXED_READERS_TRIGGER){unsigned ri;
       if(pt_sampler_mixed_reader(s->pool,h,i,handles+i)!=PT_SAMPLER_MIXED_OK){fail(s,PT_EDITOR_MIXED_READERS_FAULT);break;}
       for(ri=0;ri<PT_SAMPLER_MIXED_READERS&&s->reader[ri].handle.address;++ri){}
       if(ri==PT_SAMPLER_MIXED_READERS){fail(s,PT_EDITOR_MIXED_READERS_FAULT);break;}
       rr=s->reader+ri;memset(rr,0,sizeof(*rr));rr->handle=handles[i];rr->serial=++s->serial;
       rr->action=i;rr->track=x[i].track;rr->sample=x[i].sample;rr->channel=x[i].channel;c->reader[i]=ri;
    }
 }
 if(!post(s))r=s->result;else r=factory_result(s,fr);
 if(h.address&&!s->first_error&&r==PT_EDITOR_MIXED_READERS_PENDING&&output_apart(s,out,sizeof(*out)))
    *out=(struct pt_editor_mixed_command_ref){ci,s->command[ci].serial};
 s->busy=0;return r;
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
 if(!s||!s->phase||s->phase==PT_EDITOR_MIXED_READERS_FINISHED||!s->pool)return 0;
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
 if(s->closing||(s->busy&&!s->close_call)){++s->reentries;fail(s,PT_EDITOR_MIXED_READERS_FAULT);return 0;}
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
