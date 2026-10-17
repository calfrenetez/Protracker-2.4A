#include "midi_setup.h"
#include "sampler_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
struct midi_change {
    struct midi_change *self,*next;
    struct pt_midi_setup *owner;
    struct pt_project *project;
    enum pt_midi_setup_target kind;unsigned track;
    char name[2][PT_MIDI_ENDPOINT];uint32_t flags[2];
};
struct pt_midi_setup {
    struct pt_midi_setup *self;size_t capacity,budget,bytes;
    struct pt_editor *editor;struct pt_project *project;
    struct pt_event *events;unsigned channels,patterns;
    struct pt_pattern_command *journal_commands;struct pt_event_change *journal_changes;
    size_t command_capacity,change_capacity;
    struct pt_allocator allocator;
    struct midi_change *head,*committing;
    unsigned commands,busy,held,draft,track,generation;
    enum pt_midi_setup_target target;
    uint32_t revision,old_flags,new_flags;
    char old_name[PT_MIDI_ENDPOINT],new_name[PT_MIDI_ENDPOINT];
    void *unresolved;size_t unresolved_bytes;
};
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,an)&&span(b,bn)&&
 (!an||!bn||(x<=y?an<=y-x:bn<=x-y));}
static int identity(const struct pt_midi_setup *s)
{return s&&s->self==s&&s->capacity>=sizeof(*s)&&span(s,s->capacity);}
static int current(const struct pt_midi_setup *s)
{return identity(s)&&!s->held&&s->editor&&s->editor->project==s->project&&
 s->project&&s->project->events==s->events&&s->project->channels.count==s->channels&&
 s->project->pattern_count==s->patterns&&s->editor->history.bound_events==s->events&&
 s->editor->history.bound_channels==s->channels&&s->editor->history.bound_patterns==s->patterns&&
 s->editor->history.commands==s->journal_commands&&s->editor->history.changes==s->journal_changes&&
 s->editor->history.command_capacity==s->command_capacity&&s->editor->history.change_capacity==s->change_capacity&&
 s->allocator.context==s->editor->sampler.allocator.context&&
 s->allocator.allocate==s->editor->sampler.allocator.allocate&&
 s->allocator.release==s->editor->sampler.allocator.release;}
static int operation_active(const struct pt_editor *e)
{return e->playback.active||e->workflow.busy||e->workflow.scanning||e->workflow.transaction||
 e->workflow.pending_apply||e->workflow.resolving;}
static int outputs_apart(const struct pt_midi_setup *s,const void *p,size_t n)
{
 const struct midi_change *c;unsigned count=0;
 if(!current(s)||!p||!n||!apart(p,n,s,s->capacity)||
    !apart(p,n,s->editor,sizeof(*s->editor))||
    !pt_sampler_output_disjoint(&s->editor->sampler,p,n)||
    !pt_render_project_storage_output_disjoint(s->project,p,n))return 0;
 for(c=s->head;c;c=c->next){
    if(count++>=s->commands||c->self!=c||c->owner!=s||!apart(p,n,c,sizeof(*c)))return 0;
 }
 return count==s->commands;
}
static char *field(struct pt_project *p,enum pt_midi_setup_target kind,unsigned track)
{return kind==PT_MIDI_SETUP_INPUT?p->midi_input:
 kind==PT_MIDI_SETUP_OUTPUT&&track<p->channels.count?p->midi_output[track]:NULL;}
static int terminated(const char *name)
{return memchr(name,0,PT_MIDI_ENDPOINT)!=NULL;}
static int draft_current(const struct pt_midi_setup *s)
{
 const char *name;
 if(!current(s)||!s->draft||s->editor->history.revision!=s->revision||
    s->editor->sampler.generation!=s->generation)return 0;
 if(s->target==PT_MIDI_SETUP_CLOCK)return s->project->midi_flags==s->old_flags;
 name=field(s->project,s->target,s->track);
 return name&&terminated(name)&&!memcmp(name,s->old_name,PT_MIDI_ENDPOINT);
}
/* Runs under the public owner's busy guard. A recording query may inspect
 * external capture ownership, but must not change/reinitialize any live source.
 * Recheck both callback identity and all draft sources after it returns. */
static enum pt_midi_setup_result quiet(struct pt_midi_setup *s,
 int (*recording)(void *),void *context)
{
 int active;
 if(!draft_current(s)||s->editor->workflow.recording_busy!=recording||
    s->editor->workflow.recording_context!=context)return PT_MIDI_SETUP_STALE;
 if(operation_active(s->editor))return PT_MIDI_SETUP_ACTIVE;
 active=recording?recording(context):0;
 if(!draft_current(s)||s->editor->workflow.recording_busy!=recording||
    s->editor->workflow.recording_context!=context)return PT_MIDI_SETUP_STALE;
 return active||operation_active(s->editor)?PT_MIDI_SETUP_ACTIVE:PT_MIDI_SETUP_OK;
}
static int record_apply(void *context,struct pt_project *p,int direction)
{
 struct midi_change *c=context;struct pt_midi_setup *s=c->owner;
 unsigned expected=direction>0?0:1,replacement=1-expected,was_busy,generation;char *name;
 struct pt_pattern_history *h;uint32_t revision,next_revision,saved_revision;
 size_t cursor,count,used;int active;int (*recording)(void *);void *recording_context;
 if(!identity(s)||s->held||c->self!=c||p!=c->project||
    !current(s)||operation_active(s->editor)||(s->busy&&s->committing!=c)||p->channels.count!=s->channels)return 0;
 was_busy=s->busy;s->busy=1;
 h=&s->editor->history;revision=h->revision;next_revision=h->next_revision;saved_revision=h->saved_revision;
 cursor=h->cursor;count=h->count;used=h->used;generation=s->editor->sampler.generation;
 recording=s->editor->workflow.recording_busy;recording_context=s->editor->workflow.recording_context;
 active=recording?recording(recording_context):0;
 if(active||!current(s)||operation_active(s->editor)||s->editor->sampler.generation!=generation||
    s->editor->workflow.recording_busy!=recording||s->editor->workflow.recording_context!=recording_context||
    h->revision!=revision||h->next_revision!=next_revision||h->saved_revision!=saved_revision||
    h->cursor!=cursor||h->count!=count||h->used!=used){s->busy=was_busy;return 0;}
 if(c->kind==PT_MIDI_SETUP_CLOCK){
    if(p->midi_flags!=c->flags[expected]){s->busy=was_busy;return 0;}
    p->midi_flags=c->flags[replacement];
 }else{
    name=field(p,c->kind,c->track);
    if(!name||memcmp(name,c->name[expected],PT_MIDI_ENDPOINT)){s->busy=was_busy;return 0;}
    memcpy(name,c->name[replacement],PT_MIDI_ENDPOINT);
 }
 s->busy=was_busy;return 1;
}
static void record_discard(void *context)
{
 struct midi_change *c=context,**link;struct pt_midi_setup *s=c->owner;
 struct pt_allocator a;unsigned was_busy,count=0;
 if(!identity(s)||c->self!=c||s->bytes<sizeof(*c)||!s->commands)return;
 link=&s->head;
 while(*link&&*link!=c){if(count++>=s->commands){s->held=1;return;}link=&(*link)->next;}
 if(*link!=c){s->held=1;return;}
 was_busy=s->busy;s->busy=1;*link=c->next;--s->commands;s->bytes-=sizeof(*c);
 a=s->allocator;c->self=NULL;a.release(a.context,c);s->busy=was_busy;
}
size_t pt_midi_setup_size(void){return sizeof(struct pt_midi_setup);}
size_t pt_midi_setup_alignment(void){return ALIGN_OF(struct pt_midi_setup);}
size_t pt_midi_setup_command_bytes(void){return sizeof(struct midi_change);}
enum pt_midi_setup_result pt_midi_setup_init(struct pt_midi_setup *s,size_t capacity,
 struct pt_editor *e,size_t budget)
{
 const unsigned char *zero=(const unsigned char *)s;size_t i;
 if(!s||capacity<sizeof(*s)||!span(s,capacity)||(uintptr_t)s%ALIGN_OF(struct pt_midi_setup)||
    !e||!e->project||!e->sampler.allocator.allocate||!e->sampler.allocator.release||
    pt_channels_validate(&e->project->channels)!=PT_CHANNEL_OK||
    e->history.bound_events!=e->project->events||e->history.bound_channels!=e->project->channels.count||
    e->history.bound_patterns!=e->project->pattern_count||!apart(s,capacity,e,sizeof(*e))||
    !pt_sampler_output_disjoint(&e->sampler,s,capacity)||
    !pt_render_project_storage_output_disjoint(e->project,s,capacity))return PT_MIDI_SETUP_INVALID;
 for(i=0;i<sizeof(*s);++i)if(zero[i])return PT_MIDI_SETUP_INVALID;
 s->self=s;s->capacity=capacity;s->editor=e;s->project=e->project;s->events=e->project->events;
 s->channels=e->project->channels.count;s->patterns=e->project->pattern_count;
 s->journal_commands=e->history.commands;s->journal_changes=e->history.changes;
 s->command_capacity=e->history.command_capacity;s->change_capacity=e->history.change_capacity;
 s->allocator=e->sampler.allocator;s->budget=budget;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_begin(struct pt_midi_setup *s,enum pt_midi_setup_target target,unsigned track)
{
 const char *name;
 if(!identity(s))return PT_MIDI_SETUP_INVALID;
 if(s->busy)return PT_MIDI_SETUP_BUSY;
 if(!current(s))return PT_MIDI_SETUP_STALE;
 if(s->draft)return PT_MIDI_SETUP_IN_USE;
 if(target!=PT_MIDI_SETUP_INPUT&&target!=PT_MIDI_SETUP_OUTPUT&&target!=PT_MIDI_SETUP_CLOCK)return PT_MIDI_SETUP_INVALID;
 if(target==PT_MIDI_SETUP_OUTPUT&&track>=s->channels)return PT_MIDI_SETUP_INVALID;
 if(target==PT_MIDI_SETUP_CLOCK){
    if(s->project->midi_flags>3)return PT_MIDI_SETUP_INVALID;
    s->old_flags=s->new_flags=s->project->midi_flags;
 }else{
    name=field(s->project,target,track);if(!name||!terminated(name))return PT_MIDI_SETUP_INVALID;
    memcpy(s->old_name,name,PT_MIDI_ENDPOINT);memcpy(s->new_name,name,PT_MIDI_ENDPOINT);
 }
 s->target=target;s->track=track;s->revision=s->editor->history.revision;
 s->generation=s->editor->sampler.generation;s->draft=1;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_name(struct pt_midi_setup *s,const char *name,size_t bytes)
{
 char value[PT_MIDI_ENDPOINT];
 if(!identity(s))return PT_MIDI_SETUP_INVALID;
 if(s->busy)return PT_MIDI_SETUP_BUSY;
 if(!draft_current(s))return PT_MIDI_SETUP_STALE;
 if(s->target==PT_MIDI_SETUP_CLOCK||!name||!bytes||bytes>PT_MIDI_ENDPOINT||
    !span(name,bytes)||name[bytes-1]||memchr(name,0,bytes-1))return PT_MIDI_SETUP_INVALID;
 memset(value,0,sizeof(value));memcpy(value,name,bytes);memcpy(s->new_name,value,sizeof(value));return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_flags(struct pt_midi_setup *s,uint32_t flags)
{
 if(!identity(s))return PT_MIDI_SETUP_INVALID;
 if(s->busy)return PT_MIDI_SETUP_BUSY;
 if(!draft_current(s))return PT_MIDI_SETUP_STALE;
 if(s->target!=PT_MIDI_SETUP_CLOCK||flags>3)return PT_MIDI_SETUP_INVALID;
 s->new_flags=flags;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_cancel(struct pt_midi_setup *s)
{if(!identity(s))return PT_MIDI_SETUP_INVALID;if(s->busy)return PT_MIDI_SETUP_BUSY;s->draft=0;return PT_MIDI_SETUP_OK;}
enum pt_midi_setup_result pt_midi_setup_commit(struct pt_midi_setup *s)
{
 struct midi_change *c;struct pt_edit_resource resource;enum pt_edit_result result;
 enum pt_midi_setup_result admitted;int (*recording)(void *);void *recording_context;
 if(!identity(s))return PT_MIDI_SETUP_INVALID;
 if(s->busy)return PT_MIDI_SETUP_BUSY;
 if(!draft_current(s))return PT_MIDI_SETUP_STALE;
 if((s->target==PT_MIDI_SETUP_CLOCK&&s->old_flags==s->new_flags)||
    (s->target!=PT_MIDI_SETUP_CLOCK&&!strcmp(s->old_name,s->new_name))){s->draft=0;return PT_MIDI_SETUP_OK;}
 if(operation_active(s->editor))return PT_MIDI_SETUP_ACTIVE;
 if(s->bytes>s->budget||sizeof(*c)>s->budget-s->bytes)return PT_MIDI_SETUP_CAPACITY;
 s->busy=1;
 recording=s->editor->workflow.recording_busy;recording_context=s->editor->workflow.recording_context;
 admitted=quiet(s,recording,recording_context);
 if(admitted!=PT_MIDI_SETUP_OK){s->busy=0;return admitted;}
 if(!pt_editor_prepare_change(s->editor)){s->busy=0;return PT_MIDI_SETUP_VETO;}
 admitted=quiet(s,recording,recording_context);
 if(admitted!=PT_MIDI_SETUP_OK){s->busy=0;return admitted;}
 c=s->allocator.allocate(s->allocator.context,sizeof(*c));
 if(!c){s->busy=0;return PT_MIDI_SETUP_CAPACITY;}
 if((uintptr_t)c%ALIGN_OF(struct midi_change)||!outputs_apart(s,c,sizeof(*c))){
    s->unresolved=c;s->unresolved_bytes=sizeof(*c);s->held=1;s->busy=0;return PT_MIDI_SETUP_HELD;
 }
 memset(c,0,sizeof(*c));c->self=c;c->owner=s;c->project=s->project;c->kind=s->target;c->track=s->track;
 memcpy(c->name[0],s->old_name,PT_MIDI_ENDPOINT);memcpy(c->name[1],s->new_name,PT_MIDI_ENDPOINT);
 c->flags[0]=s->old_flags;c->flags[1]=s->new_flags;c->next=s->head;s->head=c;++s->commands;s->bytes+=sizeof(*c);
 admitted=quiet(s,recording,recording_context);
 if(admitted!=PT_MIDI_SETUP_OK){record_discard(c);s->busy=0;return admitted;}
 resource=(struct pt_edit_resource){c,record_apply,record_discard};s->committing=c;
 result=pt_pattern_resource_apply(s->project,&s->editor->history,&resource);s->committing=NULL;
 if(result!=PT_EDIT_OK)record_discard(c);else s->draft=0;
 s->busy=0;
 return result==PT_EDIT_OK?PT_MIDI_SETUP_OK:result==PT_EDIT_CAPACITY?PT_MIDI_SETUP_CAPACITY:PT_MIDI_SETUP_CONFLICT;
}
enum pt_midi_setup_result pt_midi_setup_get(struct pt_midi_setup *s,struct pt_midi_setup_status *out)
{
 struct pt_midi_setup_status value;
 if(!identity(s)||!out)return PT_MIDI_SETUP_INVALID;
 if(s->busy)return PT_MIDI_SETUP_BUSY;
 if(s->held)return PT_MIDI_SETUP_HELD;
 if(!outputs_apart(s,out,sizeof(*out)))return PT_MIDI_SETUP_INVALID;
 value=(struct pt_midi_setup_status){s->bytes,s->budget,s->commands,s->draft};*out=value;return PT_MIDI_SETUP_OK;
}
static int list_valid(const struct pt_midi_endpoint_name *names,unsigned count,unsigned known)
{
 unsigned i,j;
 if(known>1||count>PT_MIDI_PORTS||(!known&&count))return 0;
 for(i=0;i<count;++i){
    if(!names[i].identity[0]||!terminated(names[i].identity))return 0;
    for(j=0;j<i;++j)if(!strcmp(names[i].identity,names[j].identity))return 0;
 }
 return 1;
}
static enum pt_midi_setup_presence presence(const char *name,const struct pt_midi_endpoint_name *names,unsigned count,unsigned known)
{unsigned i;if(!name[0])return PT_MIDI_SETUP_UNCONFIGURED;if(!known)return PT_MIDI_SETUP_UNKNOWN;for(i=0;i<count;++i)if(!strcmp(name,names[i].identity))return PT_MIDI_SETUP_PRESENT;return PT_MIDI_SETUP_MISSING;}
enum pt_midi_setup_result pt_midi_setup_view(struct pt_midi_setup *s,unsigned track,
 const struct pt_midi_setup_discovery *d,struct pt_midi_setup_view *out)
{
 struct pt_midi_setup_view value;
 if(!identity(s)||!d||!out)return PT_MIDI_SETUP_INVALID;
 if(s->busy)return PT_MIDI_SETUP_BUSY;
 if(!current(s))return PT_MIDI_SETUP_STALE;
 if(track>=s->channels||!terminated(s->project->midi_input)||!terminated(s->project->midi_output[track])||
    s->project->midi_flags>3||!list_valid(d->input,d->input_count,d->input_known)||
    !list_valid(d->output,d->output_count,d->output_known)||!outputs_apart(s,out,sizeof(*out))||
    !apart(out,sizeof(*out),d,sizeof(*d)))return PT_MIDI_SETUP_INVALID;
 memset(&value,0,sizeof(value));memcpy(value.input,s->project->midi_input,PT_MIDI_ENDPOINT);
 memcpy(value.output,s->project->midi_output[track],PT_MIDI_ENDPOINT);value.flags=s->project->midi_flags;
 value.track=track;value.route=s->project->channels.track[track].route;value.midi_channel=s->project->channels.track[track].midi_channel;
 value.input_presence=presence(value.input,d->input,d->input_count,d->input_known);
 value.output_presence=presence(value.output,d->output,d->output_count,d->output_known);*out=value;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_setup_close(struct pt_midi_setup *s)
{
 if(!identity(s))return PT_MIDI_SETUP_INVALID;
 if(s->busy)return PT_MIDI_SETUP_BUSY;
 if(s->held)return PT_MIDI_SETUP_HELD;
 if(s->commands||s->head||s->bytes)return PT_MIDI_SETUP_IN_USE;
 s->self=NULL;return PT_MIDI_SETUP_OK;
}
