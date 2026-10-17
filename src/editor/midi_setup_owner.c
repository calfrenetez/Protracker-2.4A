#include "midi_setup_owner.h"
#include "sampler_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
struct source {
    struct pt_editor *editor;struct pt_project *project;struct pt_event *events;
    struct pt_pattern_command *commands;struct pt_event_change *changes;
    struct pt_allocator allocator;
    size_t command_capacity,change_capacity,count,cursor,used,sampler_bytes,sampler_budget;
    unsigned channels,patterns,generation;uint32_t revision,next_revision;
};
struct pt_midi_setup_owner {
    struct pt_midi_setup_owner *self;size_t capacity,workspace_bytes,metadata_budget;
    struct pt_midi_setup_owner_span context;
    struct source source;struct pt_allocator allocator;
    struct pt_midi_setup *workspace;void *unresolved;
    enum pt_midi_setup_owner_state state;
    uint32_t serial;unsigned busy,modal,exclusive;
};
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *p,size_t n,const void *q,size_t m)
{uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return span(p,n)&&span(q,m)&&(!n||!m||a>=b+m||b>=a+n);}
static int allocator_same(struct pt_allocator a,struct pt_allocator b)
{return a.context==b.context&&a.allocate==b.allocate&&a.release==b.release;}
static int capture(struct source *s,struct pt_editor *e)
{
    struct pt_pattern_history *h;
    if(!e||!e->project||!e->sampler.allocator.allocate||!e->sampler.allocator.release||
       pt_channels_validate(&e->project->channels)!=PT_CHANNEL_OK)return 0;
    h=&e->history;
    if(h->bound_events!=e->project->events||h->bound_channels!=e->project->channels.count||
       h->bound_patterns!=e->project->pattern_count||!h->commands||!h->changes||
       h->command_capacity>SIZE_MAX/sizeof(*h->commands)||
       h->change_capacity>SIZE_MAX/sizeof(*h->changes))return 0;
    memset(s,0,sizeof(*s));s->editor=e;s->project=e->project;s->events=e->project->events;
    s->commands=h->commands;s->changes=h->changes;s->allocator=e->sampler.allocator;
    s->command_capacity=h->command_capacity;s->change_capacity=h->change_capacity;
    s->count=h->count;s->cursor=h->cursor;s->used=h->used;s->revision=h->revision;s->next_revision=h->next_revision;
    s->channels=e->project->channels.count;s->patterns=e->project->pattern_count;
    s->generation=e->sampler.generation;s->sampler_bytes=e->sampler.bytes;s->sampler_budget=e->sampler.budget;return 1;
}
static int source_same(const struct source *a,const struct source *b)
{
#define EQ(f) a->f==b->f
    return EQ(editor)&&EQ(project)&&EQ(events)&&EQ(commands)&&EQ(changes)&&
        allocator_same(a->allocator,b->allocator)&&EQ(command_capacity)&&EQ(change_capacity)&&
        EQ(count)&&EQ(cursor)&&EQ(used)&&EQ(revision)&&EQ(next_revision)&&EQ(channels)&&EQ(patterns)&&
        EQ(generation)&&EQ(sampler_bytes)&&EQ(sampler_budget);
#undef EQ
}
static int fixed_apart(const struct source *s,const void *p,size_t n)
{
    return apart(p,n,s->editor,sizeof(*s->editor))&&
        apart(p,n,s->project,sizeof(*s->project))&&
        apart(p,n,s->commands,s->command_capacity*sizeof(*s->commands))&&
        apart(p,n,s->changes,s->change_capacity*sizeof(*s->changes));
}
static int source_apart(const struct source *s,const void *p,size_t n)
{
    return fixed_apart(s,p,n)&&
        pt_sampler_output_disjoint(&s->editor->sampler,p,n)&&
        pt_render_project_storage_output_disjoint(s->project,p,n);
}
static int identity(const struct pt_midi_setup_owner *o)
{return o&&o->self==o&&o->capacity>=sizeof(*o)&&span(o,o->capacity);}
static enum pt_midi_setup_result admission(const struct pt_midi_setup_owner *o)
{return !identity(o)?PT_MIDI_SETUP_INVALID:o->busy?PT_MIDI_SETUP_BUSY:
    o->state==PT_MIDI_OWNER_HELD?PT_MIDI_SETUP_HELD:PT_MIDI_SETUP_OK;}
static int known_apart(const struct pt_midi_setup_owner *o,const void *p,size_t n)
{
    return p&&n&&apart(p,n,o,o->capacity)&&apart(p,n,o->context.data,o->context.bytes)&&
        apart(p,n,o->workspace,o->workspace?o->workspace_bytes:0)&&
        apart(p,n,o->unresolved,o->unresolved?o->workspace_bytes:0)&&fixed_apart(&o->source,p,n)&&
        (o->state!=PT_MIDI_OWNER_LIVE||source_apart(&o->source,p,n));
}
static enum pt_midi_setup_result uncertain(struct pt_midi_setup_owner *o,void *raw)
{o->unresolved=raw;o->state=PT_MIDI_OWNER_HELD;o->busy=0;return PT_MIDI_SETUP_HELD;}
static enum pt_midi_setup_result observe(struct pt_midi_setup_owner *o,struct pt_midi_setup_status *s)
{
    enum pt_midi_setup_result r=pt_midi_setup_get(o->workspace,s);
    if(r==PT_MIDI_SETUP_HELD)o->state=PT_MIDI_OWNER_HELD;
    return r;
}
size_t pt_midi_setup_owner_size(void){return sizeof(struct pt_midi_setup_owner);}
size_t pt_midi_setup_owner_alignment(void){return ALIGN_OF(struct pt_midi_setup_owner);}
int pt_midi_setup_owner_storage_disjoint(const struct pt_midi_setup_owner *o,size_t capacity,
    const void *p,size_t bytes)
{
    if(admission(o)!=PT_MIDI_SETUP_OK||capacity!=o->capacity||!span(p,bytes))return 0;
    return apart(p,bytes,o,o->capacity)&&apart(p,bytes,o->context.data,o->context.bytes)&&
        apart(p,bytes,o->workspace,o->workspace?o->workspace_bytes:0)&&
        apart(p,bytes,o->unresolved,o->unresolved?o->workspace_bytes:0);
}
enum pt_midi_setup_result pt_midi_setup_owner_transition_admit(
    const struct pt_midi_setup_owner *o,size_t capacity,const struct pt_editor *e,
    const struct pt_midi_setup_owner_transition_span *spans,unsigned count)
{
    struct pt_midi_setup_status s;enum pt_midi_setup_result r=admission(o);unsigned n;
    if(r!=PT_MIDI_SETUP_OK)return r;
    if(capacity!=o->capacity||e!=o->source.editor||count>64||(!spans&&count)||
       !pt_midi_setup_owner_storage_disjoint(o,capacity,spans,count*sizeof(*spans)))return PT_MIDI_SETUP_INVALID;
    if(o->state!=PT_MIDI_OWNER_LIVE)return PT_MIDI_SETUP_STALE;
    if(o->modal)return PT_MIDI_SETUP_BUSY;
    /* This read-only genuine-child query validates the original project/history/
     * allocator binding and all retained records. It invokes no callback. */
    r=pt_midi_setup_get(o->workspace,&s);if(r!=PT_MIDI_SETUP_OK)return r;
    if(s.draft)return PT_MIDI_SETUP_IN_USE;
    for(n=0;n<count;++n){
        if(spans[n].role!=PT_MIDI_OWNER_TRANSITION_STORAGE&&
           spans[n].role!=PT_MIDI_OWNER_TRANSITION_CONTEXT_INPUT)return PT_MIDI_SETUP_INVALID;
        if(spans[n].role==PT_MIDI_OWNER_TRANSITION_CONTEXT_INPUT&&o->context.bytes&&
           spans[n].data==o->context.data&&spans[n].bytes==o->context.bytes)continue;
        if(!pt_midi_setup_owner_storage_disjoint(o,capacity,spans[n].data,spans[n].bytes))return PT_MIDI_SETUP_INVALID;
    }
    return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_owner_init(struct pt_midi_setup_owner *o,size_t capacity,
    struct pt_editor *e,struct pt_midi_setup_owner_span context,size_t budget)
{
    struct source first,last;const unsigned char *z=(const unsigned char *)o;size_t i,bytes;
    void *raw;enum pt_midi_setup_result r;
    if(!o||capacity<sizeof(*o)||!span(o,capacity)||(uintptr_t)o%ALIGN_OF(struct pt_midi_setup_owner)||
       !span(context.data,context.bytes)||!e||!apart(o,capacity,e,sizeof(*e))||
       !apart(o,capacity,context.data,context.bytes))return PT_MIDI_SETUP_INVALID;
    for(i=0;i<sizeof(*o);++i)if(z[i])return PT_MIDI_SETUP_BUSY;
    if(!capture(&first,e)||!source_apart(&first,o,capacity))return PT_MIDI_SETUP_INVALID;
    if(first.allocator.context){uintptr_t a=(uintptr_t)first.allocator.context,b=(uintptr_t)context.data;
        if(!context.data||!context.bytes||a<b||a-b>=context.bytes||!source_apart(&first,context.data,context.bytes))
            return PT_MIDI_SETUP_INVALID;
    }else if(context.data||context.bytes)return PT_MIDI_SETUP_INVALID;
    bytes=pt_midi_setup_size();if(!bytes)return PT_MIDI_SETUP_INVALID;
    o->self=o;o->capacity=capacity;o->context=context;o->source=first;o->allocator=first.allocator;
    o->workspace_bytes=bytes;o->metadata_budget=budget;o->state=PT_MIDI_OWNER_LIVE;o->busy=1;
    raw=o->allocator.allocate(o->allocator.context,bytes);
    if(!capture(&last,e)||!source_same(&first,&last))return uncertain(o,raw);
    if(!raw){memset(o,0,sizeof(*o));return PT_MIDI_SETUP_CAPACITY;}
    if((uintptr_t)raw%pt_midi_setup_alignment()||!known_apart(o,raw,bytes))return uncertain(o,raw);
    o->workspace=raw;o->exclusive=1;memset(raw,0,bytes);
    r=pt_midi_setup_init(o->workspace,bytes,e,budget);
    if(r!=PT_MIDI_SETUP_OK)return uncertain(o,NULL);
    o->busy=0;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_owner_borrow(struct pt_midi_setup_owner *o,
    struct pt_midi_setup_owner_borrow *out)
{
    struct pt_midi_setup_status s;enum pt_midi_setup_result r=admission(o);
    if(r!=PT_MIDI_SETUP_OK)return r;
    if(o->state!=PT_MIDI_OWNER_LIVE)return PT_MIDI_SETUP_STALE;
    if(o->modal)return PT_MIDI_SETUP_BUSY;
    if(!known_apart(o,out,sizeof(*out)))return PT_MIDI_SETUP_INVALID;
    if(o->serial==UINT32_MAX)return PT_MIDI_SETUP_CAPACITY;
    o->busy=1;r=observe(o,&s);if(r!=PT_MIDI_SETUP_OK){o->busy=0;return r;}
    if(s.draft){o->busy=0;return PT_MIDI_SETUP_IN_USE;}
    ++o->serial;o->modal=1;
    *out=(struct pt_midi_setup_owner_borrow){o,o->workspace,o->serial};o->busy=0;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_owner_return(struct pt_midi_setup_owner *o,
    const struct pt_midi_setup_owner_borrow *ticket)
{
    struct pt_midi_setup_status s;struct pt_midi_setup_owner_borrow value;
    enum pt_midi_setup_result r=admission(o);
    if(r==PT_MIDI_SETUP_INVALID||r==PT_MIDI_SETUP_BUSY)return r;
    if(!known_apart(o,ticket,sizeof(*ticket)))return PT_MIDI_SETUP_INVALID;
    value=*ticket;
    if(!o->modal||value.owner!=o||value.setup!=o->workspace||value.serial!=o->serial)return PT_MIDI_SETUP_INVALID;
    if(r==PT_MIDI_SETUP_HELD){o->modal=0;return r;}
    o->busy=1;r=observe(o,&s);
    if(r==PT_MIDI_SETUP_OK)r=pt_midi_setup_cancel(o->workspace);
    if(r==PT_MIDI_SETUP_OK||r==PT_MIDI_SETUP_HELD)o->modal=0;
    o->busy=0;return r;
}
enum pt_midi_setup_result pt_midi_setup_owner_cancel_draft(struct pt_midi_setup_owner *o)
{
    struct pt_midi_setup_status s;enum pt_midi_setup_result r=admission(o);
    if(r!=PT_MIDI_SETUP_OK)return r;
    if(o->modal)return PT_MIDI_SETUP_BUSY;
    if(o->state!=PT_MIDI_OWNER_LIVE)return PT_MIDI_SETUP_STALE;
    o->busy=1;r=observe(o,&s);if(r==PT_MIDI_SETUP_OK)r=pt_midi_setup_cancel(o->workspace);
    o->busy=0;return r;
}
enum pt_midi_setup_result pt_midi_setup_owner_close_controller(struct pt_midi_setup_owner *o)
{
    enum pt_midi_setup_result r=admission(o);if(r!=PT_MIDI_SETUP_OK)return r;
    if(o->modal)return PT_MIDI_SETUP_BUSY;
    if(o->state==PT_MIDI_OWNER_CLOSED)return PT_MIDI_SETUP_OK;
    o->busy=1;r=pt_midi_setup_close(o->workspace);
    if(r==PT_MIDI_SETUP_OK)o->state=PT_MIDI_OWNER_CLOSED;
    else if(r==PT_MIDI_SETUP_HELD)o->state=PT_MIDI_OWNER_HELD;
    o->busy=0;return r;
}
enum pt_midi_setup_result pt_midi_setup_owner_rebind(struct pt_midi_setup_owner *o,struct pt_editor *e)
{
    struct source fresh;enum pt_midi_setup_result r=admission(o);if(r!=PT_MIDI_SETUP_OK)return r;
    if(o->modal)return PT_MIDI_SETUP_BUSY;
    if(o->state!=PT_MIDI_OWNER_CLOSED)return PT_MIDI_SETUP_IN_USE;
    if(!capture(&fresh,e)||!allocator_same(o->allocator,fresh.allocator)||
       !source_apart(&fresh,o,o->capacity)||!source_apart(&fresh,o->workspace,o->workspace_bytes)||
       (o->context.bytes&&!source_apart(&fresh,o->context.data,o->context.bytes)))return PT_MIDI_SETUP_INVALID;
    o->busy=1;memset(o->workspace,0,o->workspace_bytes);
    r=pt_midi_setup_init(o->workspace,o->workspace_bytes,e,o->metadata_budget);
    if(r!=PT_MIDI_SETUP_OK)return uncertain(o,NULL);
    o->source=fresh;o->state=PT_MIDI_OWNER_LIVE;o->busy=0;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_owner_release_workspace(struct pt_midi_setup_owner *o)
{
    struct pt_allocator a;void *raw;enum pt_midi_setup_result r=admission(o);if(r!=PT_MIDI_SETUP_OK)return r;
    if(o->modal)return PT_MIDI_SETUP_BUSY;
    if(o->state!=PT_MIDI_OWNER_CLOSED)return PT_MIDI_SETUP_IN_USE;
    if(!o->exclusive||!o->workspace)return uncertain(o,NULL);
    o->busy=1;a=o->allocator;raw=o->workspace;o->workspace=NULL;o->exclusive=0;
    a.release(a.context,raw);o->self=NULL;o->busy=0;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_owner_get(struct pt_midi_setup_owner *o,
    struct pt_midi_setup_owner_status *out)
{
    struct pt_midi_setup_status s={0};struct pt_midi_setup_owner_status value;
    enum pt_midi_setup_result r=admission(o);if(r!=PT_MIDI_SETUP_OK)return r;
    if(!known_apart(o,out,sizeof(*out)))return PT_MIDI_SETUP_INVALID;
    if(o->state==PT_MIDI_OWNER_LIVE){o->busy=1;r=observe(o,&s);o->busy=0;if(r!=PT_MIDI_SETUP_OK)return r;}
    value=(struct pt_midi_setup_owner_status){o->workspace_bytes,o->metadata_budget,s.owned_bytes,
        s.commands,o->modal,o->state};*out=value;return PT_MIDI_SETUP_OK;
}
