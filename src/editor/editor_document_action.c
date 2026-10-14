#include "editor_document_action.h"
#if !defined(PT_DOCUMENT_TYPED_ACTION) || !defined(PT_DOCUMENT_TYPED_REPLACEMENT)
#error Document action requires the explicit typed action/replacement composition
#endif
#include "sampler_lifetime_internal.h"
#include "../core/document_retirement_internal.h"
#include "../core/render_storage_internal.h"
#include <stddef.h>
#include <string.h>
static struct pt_editor_document_action *active;
static struct pt_editor_document_action expected;
enum { ACTION_PREPARING=1,ACTION_PREPARED,ACTION_CLEANUP,ACTION_DELEGATED };
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *p,size_t n,const void *q,size_t m)
{uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return span(p,n)&&span(q,m)&&(!n||!m||a>=b+m||b>=a+n);}
static int contains(struct pt_sampler_storage_span s,const void *p,size_t n)
{return span(s.data,s.bytes)&&span(p,n)&&(uintptr_t)p>=(uintptr_t)s.data&&n<=s.bytes&&
    (uintptr_t)p-(uintptr_t)s.data<=s.bytes-n;}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
struct action_admission {
    struct pt_sampler_storage_span protected[PT_DOCUMENT_ACTION_SPANS];
    unsigned protected_count;
};
static int add(struct action_admission *c,const void *p,size_t n)
{
    if(!span(p,n)||c->protected_count==PT_DOCUMENT_ACTION_SPANS)return 0;
    c->protected[c->protected_count++]=(struct pt_sampler_storage_span){p,n};return 1;
}
static int metadata_current(const struct pt_editor_document_action *c)
{
    size_t start=offsetof(struct pt_editor_document_action,replacement);
    size_t end=start+sizeof(c->replacement);
    if(!c||active!=c||c->self!=c||c->fault)return 0;
    if(expected.phase!=ACTION_DELEGATED)return !memcmp(c,&expected,sizeof(*c));
    return !memcmp(c,&expected,start)&&!memcmp((const unsigned char *)c+end,
        (const unsigned char *)&expected+end,sizeof(*c)-end);
}
static int source_current(const struct pt_editor_document_action *c)
{
    const struct pt_editor_document_action_inputs *i=&expected.inputs;
    if(!metadata_current(c))return 0;
    if(expected.phase==ACTION_DELEGATED)return 1;
    return !memcmp(i->document,&c->old_document,sizeof(c->old_document))&&
        !memcmp(i->binding,&c->old_binding,sizeof(c->old_binding))&&
        !memcmp(i->binding->editor,&c->old_editor,sizeof(c->old_editor))&&
        !memcmp(i->old_control,&c->old_control,sizeof(c->old_control))&&
        !memcmp(i->old_paula,&c->old_paula,sizeof(c->old_paula))&&
        !memcmp(i->old_amigus,&c->old_amigus,sizeof(c->old_amigus))&&
        !memcmp(i->old_paula->bridge,&c->old_paula_cache,sizeof(c->old_paula_cache))&&
        !memcmp(i->old_amigus->bridge,&c->old_amigus_cache,sizeof(c->old_amigus_cache))&&
        !memcmp(i->binding->owner,&c->old_owner,sizeof(c->old_owner))&&
        !memcmp(i->old_amigus->bridge->backend,&c->old_backend,sizeof(c->old_backend))&&
        !memcmp(i->old_amigus->bridge->backend->reservation,&c->old_reservation,sizeof(c->old_reservation))&&
        pt_editor_mixed_checked_binding_current(i->binding,i->old_control,i->old_paula,i->old_amigus);
}
static enum pt_document_action_result hold(struct pt_editor_document_action *c)
{c->fault=1;c->busy=0;c->result=PT_DOCUMENT_ACTION_HOLD;return c->result;}
static int protection_apart(const struct pt_sampler_storage_span *protected,unsigned count,
    const struct pt_editor_document_action_inputs *i,const struct pt_project *project,const void *p,size_t n)
{
    unsigned j;
    if(!span(p,n)||!p||!n)return 0;
    for(j=0;j<count;++j)
        if(!apart(p,n,protected[j].data,protected[j].bytes))return 0;
    return pt_sampler_lifetime_output_disjoint(&i->binding->editor->sampler,p,n)&&
        pt_render_project_storage_output_disjoint(project,p,n);
}
static int protected_apart(const struct pt_editor_document_action *c,const void *p,size_t n)
{return protection_apart(c->protected,c->protected_count,&c->inputs,&c->old_document.project,p,n);}
static int admit(struct pt_editor_document_action *out,const struct pt_editor_document_action_inputs *i,
    const uint8_t *source,size_t bytes)
{
    /* Admission is callback-free. Keep only the span inventory on the stack;
     * snapshot the large editor/owner state directly after every prewrite check. */
    struct action_admission c;struct pt_document empty;void *owned[7];size_t sizes[7];unsigned j;
    struct pt_editor *e;struct pt_sampler_prepare_memory *m;
    if(active){if(active->busy)active->fault=1;return 0;}
    if(!out||!i||!span(out,sizeof(*out))||!apart(out,sizeof(*out),i,sizeof(*i))||
       !span(source,bytes)||!apart(out,sizeof(*out),source,bytes)||
       !i->document||!i->candidate||!i->binding||!i->old_control||!i->new_control||
       !i->old_paula||!i->old_amigus||!i->establish||!i->bridges||
       !i->allocator.allocate||!i->allocator.release||!i->work||i->work>256||
       i->context_count>PT_DOCUMENT_ACTION_CONTEXTS||
       !pt_editor_mixed_checked_binding_current(i->binding,i->old_control,i->old_paula,i->old_amigus)||
       !(e=i->binding->editor)||e->project!=&i->document->project||
       !i->bridge_inputs.backend||!i->bridge_inputs.backend->reservation||
       i->bridge_inputs.backend==i->old_amigus->bridge->backend)return 0;
    memset(&c,0,sizeof(c));
    if(!add(&c,&expected,sizeof(expected))||!add(&c,&active,sizeof(active)))return 0;
#define KEEP(p) if(!add(&c,(p),sizeof(*(p))))return 0
    KEEP(i->document);KEEP(e);KEEP(i->binding);KEEP(i->old_control);
    KEEP(i->old_paula);KEEP(i->old_amigus);KEEP(i->old_paula->bridge);KEEP(i->old_amigus->bridge);
    KEEP(i->old_amigus->bridge->backend);KEEP(i->old_amigus->bridge->backend->reservation);
    KEEP(i->binding->owner);KEEP(i->establish);KEEP(i->new_control);KEEP(i->bridges);
    KEEP(i->bridge_inputs.backend);KEEP(i->bridge_inputs.backend->reservation);
#undef KEEP
    if(!pt_document_owned_storage_spans(i->document,owned,sizes))return 0;
    for(j=0;j<7;++j)if(!add(&c,owned[j],sizes[j]))return 0;
    m=&i->old_control->storage.memory;
    if(m->parent_count>PT_SAMPLER_PREPARE_PARENTS)return 0;
    for(j=0;j<m->parent_count;++j)if(!add(&c,m->parents[j].data,m->parents[j].bytes))return 0;
    for(j=0;j<PT_SAMPLER_PREPARE_BLOCKS;++j)if(!add(&c,m->block[j].data,m->block[j].bytes))return 0;
    if(!protection_apart(c.protected,c.protected_count,i,&i->document->project,out,sizeof(*out))||
       !protection_apart(c.protected,c.protected_count,i,&i->document->project,i->candidate,sizeof(*i->candidate))||
       !apart(out,sizeof(*out),i->candidate,sizeof(*i->candidate))||
       !apart(out,sizeof(*out),i->allocator.context,i->allocator.context?1:0))return 0;
    for(j=0;j<i->context_count;++j){
        if(!apart(i->candidate,sizeof(*i->candidate),i->contexts[j].data,i->contexts[j].bytes)||
           (!apart(out,sizeof(*out),i->contexts[j].data,i->contexts[j].bytes)&&
            !contains(i->contexts[j],out,sizeof(*out)))||
           !add(&c,i->contexts[j].data,i->contexts[j].bytes))return 0;
    }
    if(i->allocator.context){
        uintptr_t p=(uintptr_t)i->allocator.context;int contained=0;
        for(j=0;j<i->context_count;++j){uintptr_t q=(uintptr_t)i->contexts[j].data;
            if(p>=q&&p-q<i->contexts[j].bytes)contained=1;}
        if(!contained)return 0;
    }
    memset(&empty,0,sizeof(empty));empty.allocator=i->candidate->allocator;
    if(!zero(out,sizeof(*out))||memcmp(i->candidate,&empty,sizeof(empty))||
       !pt_document_replacement_allowed(i->candidate)||
       !apart(i->candidate,sizeof(*i->candidate),i,sizeof(*i))||
       !apart(i->candidate,sizeof(*i->candidate),source,bytes)||
       !add(&c,out,sizeof(*out))||!add(&c,source,bytes))return 0;
    out->self=out;out->inputs=*i;out->source=source;out->source_bytes=bytes;
    out->old_document=*i->document;out->old_editor=*e;out->old_binding=*i->binding;out->old_control=*i->old_control;
    out->old_paula=*i->old_paula;out->old_amigus=*i->old_amigus;
    out->old_paula_cache=*i->old_paula->bridge;out->old_amigus_cache=*i->old_amigus->bridge;
    out->old_owner=*i->binding->owner;out->old_backend=*i->old_amigus->bridge->backend;
    out->old_reservation=*i->old_amigus->bridge->backend->reservation;
    memcpy(out->protected,c.protected,sizeof(c.protected));out->protected_count=c.protected_count;
    out->phase=ACTION_PREPARING;out->busy=1;out->result=PT_DOCUMENT_ACTION_PREPARED;
    active=out;expected=*out;return 1;
}
static enum pt_document_action_result cleanup(struct pt_editor_document_action *c,enum pt_document_action_result done)
{
    enum pt_document_release_result r;enum pt_project_result reason=c->preparation_result;
    if(c->cleanup_attempted)return hold(c);
    c->phase=ACTION_CLEANUP;c->cleanup_attempted=1;c->busy=1;expected=*c;
    r=pt_document_release_checked(c->inputs.candidate);
    if(r!=PT_DOCUMENT_RELEASED||!source_current(c))return hold(c);
    active=NULL;memset(c,0,sizeof(*c));c->result=done;c->preparation_result=reason;
    memset(&expected,0,sizeof(expected));return done;
}
static enum pt_document_action_result prepared(struct pt_editor_document_action *c,enum pt_project_result r)
{
    enum pt_document_replace_result next;
    if(!source_current(c))return hold(c);
    c->preparation_result=r;
    if(r!=PT_PROJECT_OK)return cleanup(c,PT_DOCUMENT_ACTION_FAILED);
    /* The typed admission is callback-free. Suspend this additional veto only
     * while its own exact source/ownership helpers enroll the detached candidate.
     * The existing typed guard remains the sole retirement authority. */
    active=NULL;
    next=pt_editor_document_replacement_begin(&c->replacement,c->inputs.document,c->inputs.candidate,
        c->inputs.binding,c->inputs.old_control,c->inputs.old_paula,c->inputs.old_amigus,
        c->inputs.establish,c->inputs.new_control,c->inputs.bridges,&c->inputs.bridge_inputs,
        &c->inputs.options,&c->inputs.caps,&c->inputs.format,
        (struct pt_sampler_storage_span){c,sizeof(*c)},c->inputs.work);
    active=c;
    if(next!=PT_DOCUMENT_REPLACE_PREPARED)return cleanup(c,PT_DOCUMENT_ACTION_FAILED);
    c->phase=ACTION_PREPARED;c->busy=0;expected=*c;return c->result;
}
enum pt_document_action_result pt_editor_document_action_new(struct pt_editor_document_action *c,
    const struct pt_editor_document_action_inputs *i,unsigned channels,size_t budget)
{
    enum pt_project_result r;
    if(!admit(c,i,NULL,0))return PT_DOCUMENT_ACTION_INVALID;
    pt_document_init(i->candidate,&c->inputs.allocator);
    r=pt_document_new(i->candidate,channels,budget);return prepared(c,r);
}
enum pt_document_action_result pt_editor_document_action_load(struct pt_editor_document_action *c,
    const struct pt_editor_document_action_inputs *i,const uint8_t *bytes,size_t count,size_t budget)
{
    enum pt_project_result r;
    if(!admit(c,i,bytes,count))return PT_DOCUMENT_ACTION_INVALID;
    pt_document_init(i->candidate,&c->inputs.allocator);
    r=pt_document_load(i->candidate,bytes,count,budget);return prepared(c,r);
}
static enum pt_document_action_result translate(enum pt_document_replace_result r)
{
    switch(r){
    case PT_DOCUMENT_REPLACE_WAITING:return PT_DOCUMENT_ACTION_WAITING;
    case PT_DOCUMENT_REPLACE_RETIRING:return PT_DOCUMENT_ACTION_RETIRING;
    case PT_DOCUMENT_REPLACE_PUBLISHED:return PT_DOCUMENT_ACTION_PUBLISHED;
    case PT_DOCUMENT_REPLACE_COMMITTED:return PT_DOCUMENT_ACTION_COMMITTED;
    case PT_DOCUMENT_REPLACE_COMMITTED_NO_OWNER:return PT_DOCUMENT_ACTION_COMMITTED_NO_OWNER;
    default:return PT_DOCUMENT_ACTION_HOLD;
    }
}
enum pt_document_action_result pt_editor_document_action_step(struct pt_editor_document_action *c)
{
    enum pt_document_replace_result r;
    if(!c||active!=c||c->self!=c)return PT_DOCUMENT_ACTION_INVALID;
    if(c->busy)return hold(c);
    if(c->result==PT_DOCUMENT_ACTION_HOLD)return c->result;
    if(!source_current(c))return hold(c);
    c->phase=ACTION_DELEGATED;c->busy=1;expected=*c;
    r=pt_editor_document_replacement_step(&c->replacement);
    if(!metadata_current(c))return hold(c);
    c->busy=0;c->result=translate(r);expected=*c;return c->result;
}
enum pt_document_action_result pt_editor_document_action_cancel(struct pt_editor_document_action *c)
{
    int done;
    if(!c||active!=c||c->self!=c)return PT_DOCUMENT_ACTION_INVALID;
    if(c->busy)return hold(c);
    if(c->result==PT_DOCUMENT_ACTION_HOLD)return c->result;
    if(!source_current(c))return hold(c);
    if(c->phase!=ACTION_PREPARED)return PT_DOCUMENT_ACTION_INVALID;
    active=NULL;done=pt_editor_document_replacement_finish(&c->replacement);active=c;
    if(!done)return hold(c);
    return cleanup(c,PT_DOCUMENT_ACTION_CANCELED);
}
int pt_editor_document_action_finish(struct pt_editor_document_action *c)
{
    if(!c)return 1;
    if(!c->self)return active!=c;
    if(active!=c||c->self!=c)return 0;
    if(c->busy){(void)hold(c);return 0;}
    if(c->result!=PT_DOCUMENT_ACTION_COMMITTED&&c->result!=PT_DOCUMENT_ACTION_COMMITTED_NO_OWNER)return 0;
    if(!metadata_current(c)||!pt_editor_document_replacement_finish(&c->replacement))return 0;
    active=NULL;memset(c,0,sizeof(*c));memset(&expected,0,sizeof(expected));return 1;
}
int pt_editor_document_action_reset(struct pt_editor_document_action *c)
{
    if(!c||!apart(c,sizeof(*c),&expected,sizeof(expected))||
       !apart(c,sizeof(*c),&active,sizeof(active)))return 0;
    if(active==c){if(c->busy)(void)hold(c);return 0;}
    if(c->self||(active&&!pt_document_typed_action_output_disjoint(c,sizeof(*c))))return 0;
    memset(c,0,sizeof(*c));return 1;
}
int pt_document_typed_action_project_allowed(const struct pt_project *p)
{
    const struct pt_editor_document_action *c=active;
    if(!c)return 1;
    if(!source_current(c))return 0;
    if(expected.phase==ACTION_DELEGATED)return 1;
    if(p==&expected.inputs.document->project)return 0;
    if(p==&expected.inputs.candidate->project)return c->busy&&
        (c->phase==ACTION_PREPARING||c->phase==ACTION_CLEANUP);
    return 1;
}
int pt_document_typed_action_output_disjoint(const void *p,size_t bytes)
{
    const struct pt_editor_document_action *c=active;
    if(!apart(p,bytes,&expected,sizeof(expected))||!apart(p,bytes,&active,sizeof(active)))return 0;
    if(!c)return span(p,bytes);
    if(!source_current(c))return 0;
    if(expected.phase==ACTION_DELEGATED)return apart(p,bytes,c,sizeof(*c))&&
        apart(p,bytes,c->source,c->source_bytes);
    if(p==expected.inputs.candidate&&bytes==sizeof(*expected.inputs.candidate))return c->busy&&
        (c->phase==ACTION_PREPARING||c->phase==ACTION_CLEANUP);
    if(!apart(p,bytes,expected.inputs.candidate,sizeof(*expected.inputs.candidate)))return 0;
    return protected_apart(c,p,bytes);
}
