#include "editor_midi_recording.h"
#include "../core/record_pattern.h"
#include "../core/render_storage_internal.h"
#include "sampler_internal.h"
#include <string.h>
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)

struct pt_editor_midi_recording {
    struct pt_editor_midi_recording *self;
    size_t capacity,context_bytes;
    struct pt_editor *editor;
    struct pt_project *project;
    struct pt_event *events;
    uint16_t *orders;
    unsigned channels,patterns,order_count,generation,song_generation;
    struct pt_pattern_command *commands;
    struct pt_event_change *changes;
    size_t command_capacity,change_capacity,count,cursor,used;
    uint32_t revision,next_revision,saved_revision;
    struct pt_allocator allocator;
    struct pt_record_pattern store;
    struct pt_recorder recorder;
    unsigned track,channel,attached,busy,held;
};
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return span(a,an)&&span(b,bn)&&(!an||!bn||(x<=y?an<=y-x:bn<=x-y));
}
static int identity(const struct pt_editor_midi_recording *o)
{
    return span(o,sizeof(*o))&&!((uintptr_t)o%ALIGN_OF(struct pt_editor_midi_recording))&&
        o->self==o&&o->capacity>=sizeof(*o)&&span(o,o->capacity);
}
/* These two callbacks are deliberately inert, including on source drift. The
 * original owner stays alive; an enrolled take never admits a generic mutation. */
static int barrier(void *context)
{(void)context;return 0;}
static int recording_busy(void *context)
{(void)context;return 1;}
static void remember_journal(struct pt_editor_midi_recording *o)
{
    const struct pt_pattern_history *h=&o->editor->history;
    o->count=h->count;o->cursor=h->cursor;o->used=h->used;o->revision=h->revision;
    o->next_revision=h->next_revision;o->saved_revision=h->saved_revision;
}
static int current(const struct pt_editor_midi_recording *o)
{
    const struct pt_editor *e=o->editor;
    const struct pt_project *p=o->project;
    const struct pt_pattern_history *h;
    if(!e||e->playback.active||e->workflow.busy||e->workflow.transaction||
       e->project!=p||!p||p->events!=o->events||p->orders!=o->orders||
       p->channels.count!=o->channels||p->pattern_count!=o->patterns||
       p->order_count!=o->order_count||o->track>=o->channels||
       p->channels.track[o->track].route!=PT_MIDI||
       e->sampler.generation!=o->generation||e->song.generation!=o->song_generation||
       e->sampler.allocator.context!=o->allocator.context||
       e->sampler.allocator.allocate!=o->allocator.allocate||
       e->sampler.allocator.release!=o->allocator.release)return 0;
    h=&e->history;
    return h->bound_events==o->events&&h->bound_channels==o->channels&&
        h->bound_patterns==o->patterns&&h->commands==o->commands&&h->changes==o->changes&&
        h->command_capacity==o->command_capacity&&h->change_capacity==o->change_capacity&&
        h->count==o->count&&h->cursor==o->cursor&&h->used==o->used&&
        h->revision==o->revision&&h->next_revision==o->next_revision&&h->saved_revision==o->saved_revision&&
        !e->before_change&&e->change_ready==barrier&&e->before_change_context==o&&
        e->workflow.recording_busy==recording_busy&&e->workflow.recording_context==o;
}
static int geometry(const struct pt_editor_midi_recording *o)
{
    const struct pt_editor *e=o->editor;const struct pt_pattern_history *h=&e->history;
    const struct pt_project *p=o->project;
    return !e->playback.active&&!e->workflow.busy&&!e->workflow.transaction&&
        e->project==p&&p&&p->events==o->events&&p->orders==o->orders&&
        p->channels.count==o->channels&&p->pattern_count==o->patterns&&p->order_count==o->order_count&&
        p->channels.track[o->track].route==PT_MIDI&&
        h->bound_events==o->events&&h->bound_channels==o->channels&&h->bound_patterns==o->patterns&&
        h->commands==o->commands&&h->changes==o->changes&&h->command_capacity==o->command_capacity&&
        h->change_capacity==o->change_capacity&&e->sampler.generation==o->generation&&
        e->song.generation==o->song_generation&&e->sampler.allocator.context==o->allocator.context&&
        e->sampler.allocator.allocate==o->allocator.allocate&&e->sampler.allocator.release==o->allocator.release&&
        !e->before_change&&e->change_ready==barrier&&e->before_change_context==o&&
        e->workflow.recording_busy==recording_busy&&e->workflow.recording_context==o;
}
static enum pt_editor_midi_record_result admission(struct pt_editor_midi_recording *o)
{
    if(!identity(o))return PT_EDITOR_RECORD_INVALID;
    if(o->busy)return PT_EDITOR_RECORD_BUSY;
    if(o->held)return PT_EDITOR_RECORD_HELD;
    if(!o->attached)return PT_EDITOR_RECORD_CONFLICT;
    if(!current(o)){o->held=1;return PT_EDITOR_RECORD_HELD;}
    return PT_EDITOR_RECORD_OK;
}
static int output_apart(const struct pt_editor_midi_recording *o,const void *p,size_t n)
{
    return apart(p,n,o,o->capacity)&&apart(p,n,o->editor,sizeof(*o->editor))&&
        apart(p,n,o->allocator.context,o->context_bytes)&&
        apart(p,n,o->commands,o->command_capacity*sizeof(*o->commands))&&
        apart(p,n,o->changes,o->change_capacity*sizeof(*o->changes))&&
        pt_sampler_output_disjoint(&o->editor->sampler,p,n)&&
        pt_render_project_storage_output_disjoint(o->project,p,n);
}
static enum pt_editor_midi_record_result translated(enum pt_record_result r)
{
    switch(r){
    case PT_RECORD_OK:return PT_EDITOR_RECORD_OK;
    case PT_RECORD_IGNORED:return PT_EDITOR_RECORD_IGNORED;
    case PT_RECORD_INVALID:return PT_EDITOR_RECORD_INVALID;
    case PT_RECORD_TIME:return PT_EDITOR_RECORD_TIME;
    case PT_RECORD_CAPACITY:return PT_EDITOR_RECORD_CAPACITY;
    case PT_RECORD_COLLISION:return PT_EDITOR_RECORD_COLLISION;
    default:return PT_EDITOR_RECORD_STORE_FAILED;
    }
}
static enum pt_editor_midi_record_result completed(struct pt_editor_midi_recording *o,enum pt_record_result r)
{
    if(!geometry(o)){o->held=1;o->busy=0;return PT_EDITOR_RECORD_HELD;}
    remember_journal(o);o->busy=0;return translated(r);
}
size_t pt_editor_midi_recording_size(void){return sizeof(struct pt_editor_midi_recording);}
size_t pt_editor_midi_recording_alignment(void){return ALIGN_OF(struct pt_editor_midi_recording);}
enum pt_editor_midi_record_result pt_editor_midi_recording_begin(
    struct pt_editor_midi_recording *o,size_t capacity,struct pt_editor *e,
    size_t context_bytes,const struct pt_record_settings *settings,
    uint64_t position,unsigned track,unsigned channel)
{
    struct pt_record_settings copy;struct pt_project *p;struct pt_pattern_history *h;size_t i;
    const unsigned char *prefix=(const unsigned char *)o;
    if(!span(o,capacity)||capacity<sizeof(*o)||(uintptr_t)o%ALIGN_OF(struct pt_editor_midi_recording)||
       !e||!settings||!span(settings,sizeof(*settings))||!span(e,sizeof(*e)))return PT_EDITOR_RECORD_INVALID;
    p=e->project;h=&e->history;
    if(!p||!p->events||!p->orders||!h->commands||!h->changes||!h->command_capacity||!h->change_capacity||
       h->command_capacity>SIZE_MAX/sizeof(*h->commands)||h->change_capacity>SIZE_MAX/sizeof(*h->changes)||
       !span(e->sampler.allocator.context,context_bytes)||
       (e->sampler.allocator.context&&!context_bytes)||(!e->sampler.allocator.context&&context_bytes))return PT_EDITOR_RECORD_INVALID;
    if(!apart(o,capacity,e,sizeof(*e))||!apart(o,capacity,settings,sizeof(*settings))||
       !apart(o,capacity,e->sampler.allocator.context,context_bytes)||
       !apart(o,capacity,h->commands,h->command_capacity*sizeof(*h->commands))||
       !apart(o,capacity,h->changes,h->change_capacity*sizeof(*h->changes))||
       !pt_sampler_output_disjoint(&e->sampler,o,capacity)||
       !pt_render_project_storage_output_disjoint(p,o,capacity))return PT_EDITOR_RECORD_ALIAS;
    for(i=0;i<sizeof(*o);++i)if(prefix[i])return PT_EDITOR_RECORD_BUSY;
    copy=*settings;
    if(channel<1||channel>16||track>=p->channels.count||track>=16||
       p->channels.track[track].route!=PT_MIDI||!copy.grid_rows||copy.grid_rows>64||
       copy.first_row>=copy.row_limit||copy.row_limit>(uint32_t)p->order_count*64U||
       copy.hold>1||copy.velocity>1||copy.instrument>p->sample_count||
       pt_project_validate(p,NULL)!=PT_PROJECT_OK||
       h->bound_events!=p->events||h->bound_channels!=p->channels.count||h->bound_patterns!=p->pattern_count)return PT_EDITOR_RECORD_INVALID;
    if(e->before_change||e->change_ready||e->before_change_context||
       e->workflow.recording_busy||e->workflow.recording_context||e->playback.active||
       e->workflow.busy||e->workflow.scanning||e->workflow.transaction||e->workflow.pending_apply||e->workflow.resolving)return PT_EDITOR_RECORD_CONFLICT;
    memset(o,0,sizeof(*o));o->self=o;o->capacity=capacity;o->editor=e;o->project=p;
    o->events=p->events;o->orders=p->orders;o->channels=p->channels.count;
    o->patterns=p->pattern_count;o->order_count=p->order_count;
    o->generation=e->sampler.generation;o->song_generation=e->song.generation;
    o->commands=h->commands;o->changes=h->changes;o->command_capacity=h->command_capacity;o->change_capacity=h->change_capacity;
    o->allocator=e->sampler.allocator;o->context_bytes=context_bytes;o->track=track;o->channel=channel;
    o->store.project=p;o->store.history=h;remember_journal(o);
    if(!pt_record_arm(&o->recorder,&copy,position,&o->store,pt_record_pattern_store)||
       !pt_editor_change_barrier(e,barrier,o)){o->held=1;return PT_EDITOR_RECORD_HELD;}
    e->workflow.recording_busy=recording_busy;e->workflow.recording_context=o;o->attached=1;
    return PT_EDITOR_RECORD_OK;
}
enum pt_editor_midi_record_result pt_editor_midi_recording_message(
    struct pt_editor_midi_recording *o,uint64_t position,const uint8_t *message,size_t bytes)
{
    uint8_t copy[3];enum pt_editor_midi_record_result a=admission(o);
    if(a!=PT_EDITOR_RECORD_OK)return a;
    if((bytes!=2&&bytes!=3)||!span(message,bytes))return PT_EDITOR_RECORD_INVALID;
    memcpy(copy,message,bytes);o->busy=1;
    return completed(o,pt_record_channel_message(&o->recorder,position,o->track,o->channel,copy,bytes));
}
enum pt_editor_midi_record_result pt_editor_midi_recording_stop(struct pt_editor_midi_recording *o,uint64_t position)
{
    enum pt_editor_midi_record_result a=admission(o);
    if(a!=PT_EDITOR_RECORD_OK)return a;
    o->busy=1;return completed(o,pt_record_stop(&o->recorder,position));
}
enum pt_editor_midi_record_result pt_editor_midi_recording_cancel(struct pt_editor_midi_recording *o)
{
    enum pt_editor_midi_record_result a=admission(o);
    if(a!=PT_EDITOR_RECORD_OK)return a;
    o->busy=1;pt_record_cancel(&o->recorder);return completed(o,PT_RECORD_OK);
}
enum pt_editor_midi_record_result pt_editor_midi_recording_detach(struct pt_editor_midi_recording *o)
{
    unsigned i;enum pt_editor_midi_record_result a=admission(o);
    if(a!=PT_EDITOR_RECORD_OK)return a;
    if(o->recorder.armed)return PT_EDITOR_RECORD_CONFLICT;
    for(i=0;i<16;++i)if(o->recorder.voices[i].active)return PT_EDITOR_RECORD_CONFLICT;
    o->busy=1;
    if(!pt_editor_change_barrier(o->editor,NULL,NULL)){o->held=1;o->busy=0;return PT_EDITOR_RECORD_HELD;}
    o->editor->workflow.recording_busy=NULL;o->editor->workflow.recording_context=NULL;
    o->attached=0;o->busy=0;return PT_EDITOR_RECORD_OK;
}
enum pt_editor_midi_record_result pt_editor_midi_recording_get(
    struct pt_editor_midi_recording *o,struct pt_editor_midi_record_status *out)
{
    struct pt_editor_midi_record_status value;unsigned i;
    if(!identity(o)||!out||!span(out,sizeof(*out)))return PT_EDITOR_RECORD_INVALID;
    if(!apart(o,o->capacity,out,sizeof(*out)))return PT_EDITOR_RECORD_ALIAS;
    if(o->busy)return PT_EDITOR_RECORD_BUSY;
    if(o->held)return PT_EDITOR_RECORD_HELD;
    if(o->attached&&!current(o)){o->held=1;return PT_EDITOR_RECORD_HELD;}
    if(o->attached&&!output_apart(o,out,sizeof(*out)))return PT_EDITOR_RECORD_ALIAS;
    value=(struct pt_editor_midi_record_status){o->attached,o->recorder.armed,o->recorder.started,0,o->track,o->channel,o->recorder.origin,o->recorder.last_position};
    for(i=0;i<16;++i)value.active+=o->recorder.voices[i].active!=0;
    *out=value;return PT_EDITOR_RECORD_OK;
}
