#include "midi_frontend_context.h"
#include "../editor/sampler_lifetime_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
struct pt_midi_frontend_context {
    struct pt_midi_frontend_context *self;
    size_t capacity,owner_capacity;
    struct pt_midi_setup_owner *owner;
    struct pt_editor *editor;
    struct pt_midi_requester_modal modal;
    enum pt_midi_frontend_state state;
    unsigned busy,dispose_attempted,close_attempted,release_attempted;
};
static int span(const void *p,size_t bytes)
{return !bytes||(p&&bytes<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *p,size_t bytes,const void *q,size_t other)
{
    uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return span(p,bytes)&&span(q,other)&&(!bytes||!other||a>=b+other||b>=a+bytes);
}
static int zero(const void *p,size_t bytes)
{const unsigned char *b=p;size_t i;for(i=0;i<bytes;++i)if(b[i])return 0;return 1;}
static int original_apart(const struct pt_editor *e,const void *p,size_t bytes)
{
    const struct pt_pattern_history *h=&e->history;
    if(!e->project||h->command_capacity>SIZE_MAX/sizeof(*h->commands)||
       h->change_capacity>SIZE_MAX/sizeof(*h->changes))return 0;
    return apart(p,bytes,e,sizeof(*e))&&apart(p,bytes,e->project,sizeof(*e->project))&&
        apart(p,bytes,h->commands,h->command_capacity*sizeof(*h->commands))&&
        apart(p,bytes,h->changes,h->change_capacity*sizeof(*h->changes))&&
        pt_sampler_lifetime_output_disjoint(&e->sampler,p,bytes)&&
        pt_render_project_storage_output_disjoint(e->project,p,bytes);
}
static int identity(const struct pt_midi_frontend_context *c)
{return c&&c->self==c&&c->capacity>=sizeof(*c)&&span(c,c->capacity);}
static enum pt_midi_setup_result admission(const struct pt_midi_frontend_context *c)
{
    return !identity(c)?PT_MIDI_SETUP_INVALID:c->busy?PT_MIDI_SETUP_BUSY:
        c->state==PT_MIDI_FRONTEND_HELD?PT_MIDI_SETUP_HELD:PT_MIDI_SETUP_OK;
}
static enum pt_midi_setup_result hold(struct pt_midi_frontend_context *c)
{c->state=PT_MIDI_FRONTEND_HELD;c->busy=0;return PT_MIDI_SETUP_HELD;}
static enum pt_midi_setup_result original(struct pt_midi_frontend_context *c)
{
    const struct pt_midi_setup_owner_transition_span storage={
        c,c->capacity,PT_MIDI_OWNER_TRANSITION_STORAGE};
    return pt_midi_setup_owner_transition_admit(c->owner,c->owner_capacity,
        c->editor,&storage,1);
}
/* Exact copies here are only transient observations of this control, not cloned
 * owner/model/ticket ownership. No saved image is ever restored. */
static int same(const struct pt_midi_frontend_context *c,
    const struct pt_midi_frontend_context *before)
{return !memcmp(c,before,sizeof(*c));}
size_t pt_midi_frontend_context_size(void){return sizeof(struct pt_midi_frontend_context);}
size_t pt_midi_frontend_context_alignment(void){return ALIGN_OF(struct pt_midi_frontend_context);}
enum pt_midi_setup_result pt_midi_frontend_context_construct(
    struct pt_midi_frontend_context *c,size_t capacity,
    struct pt_midi_setup_owner *owner,size_t owner_capacity,struct pt_editor *e,
    struct pt_midi_setup_owner_span context,size_t budget)
{
    struct pt_midi_frontend_context before;enum pt_midi_setup_result r;
    size_t owner_bytes=pt_midi_setup_owner_size();
    if(!c||capacity<sizeof(*c)||!span(c,capacity)||(uintptr_t)c%ALIGN_OF(struct pt_midi_frontend_context)||
       !owner||owner_capacity<owner_bytes||!span(owner,owner_capacity)||
       (uintptr_t)owner%pt_midi_setup_owner_alignment()||!e||
       !apart(c,capacity,owner,owner_capacity)||!span(context.data,context.bytes)||
       !apart(c,capacity,context.data,context.bytes)||!apart(owner,owner_capacity,context.data,context.bytes)||
       !original_apart(e,c,capacity)||!original_apart(e,owner,owner_capacity))return PT_MIDI_SETUP_INVALID;
    if(!zero(c,sizeof(*c))||!zero(owner,owner_bytes))return PT_MIDI_SETUP_BUSY;
    /* The real constructor performs its own initialized-source/context checks.
     * Its INVALID/BUSY paths have not called the allocator; NULL CAPACITY has
     * restored only the unadopted owner prefix to zero. */
    c->self=c;c->capacity=capacity;c->owner=owner;c->owner_capacity=owner_capacity;
    c->editor=e;c->state=PT_MIDI_FRONTEND_CONSTRUCTING;c->busy=1;
    memcpy(&before,c,sizeof(before));
    r=pt_midi_setup_owner_init(owner,owner_capacity,e,context,budget);
    if(!same(c,&before))return hold(c);
    if(r==PT_MIDI_SETUP_CAPACITY||r==PT_MIDI_SETUP_INVALID||r==PT_MIDI_SETUP_BUSY){
        memset(c,0,sizeof(*c));return r;
    }
    if(r!=PT_MIDI_SETUP_OK)return hold(c);
    r=original(c);if(r!=PT_MIDI_SETUP_OK)return hold(c);
    c->state=PT_MIDI_FRONTEND_LIVE;c->busy=0;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_frontend_context_begin(
    struct pt_midi_frontend_context *c,unsigned track)
{
    enum pt_midi_setup_result r=admission(c);
    if(r!=PT_MIDI_SETUP_OK)return r;
    if(c->state!=PT_MIDI_FRONTEND_LIVE)return PT_MIDI_SETUP_IN_USE;
    r=original(c);if(r==PT_MIDI_SETUP_HELD)return hold(c);
    if(r!=PT_MIDI_SETUP_OK)return r;
    c->busy=1;r=pt_midi_requester_modal_begin(&c->modal,c->owner,c->owner_capacity,c->editor,track);
    if(r==PT_MIDI_SETUP_HELD||c->modal.held)return hold(c);
    if(r!=PT_MIDI_SETUP_OK){
        if(c->modal.self)return hold(c);
        c->busy=0;return r;
    }
    c->state=PT_MIDI_FRONTEND_MODAL;c->busy=0;return PT_MIDI_SETUP_OK;
}
struct pt_midi_requester_model *pt_midi_frontend_context_model(struct pt_midi_frontend_context *c)
{
    if(admission(c)!=PT_MIDI_SETUP_OK||c->state!=PT_MIDI_FRONTEND_MODAL||
       c->modal.self!=&c->modal||c->modal.busy||c->modal.held||c->modal.model.busy||
       c->modal.model.self!=&c->modal.model)return NULL;
    return &c->modal.model;
}
enum pt_midi_setup_result pt_midi_frontend_context_end(struct pt_midi_frontend_context *c)
{
    enum pt_midi_setup_result r=admission(c);
    if(r!=PT_MIDI_SETUP_OK)return r;
    if(c->state!=PT_MIDI_FRONTEND_MODAL)return PT_MIDI_SETUP_IN_USE;
    /* Reentry from the existing real model callback is a negative refusal,
     * before any matched-return attempt. The outer callback still owns it. */
    if(c->modal.busy||c->modal.model.busy)return PT_MIDI_SETUP_BUSY;
    c->busy=1;r=pt_midi_requester_modal_return(&c->modal);
    if(r!=PT_MIDI_SETUP_OK)return hold(c);
    c->state=PT_MIDI_FRONTEND_LIVE;c->busy=0;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_frontend_context_dispose_close(struct pt_midi_frontend_context *c)
{
    struct pt_midi_frontend_context before;enum pt_midi_setup_result r=admission(c);
    int disposed;
    if(r!=PT_MIDI_SETUP_OK)return r;
    if(c->state!=PT_MIDI_FRONTEND_LIVE)return PT_MIDI_SETUP_IN_USE;
    r=original(c);if(r==PT_MIDI_SETUP_HELD)return hold(c);
    if(r!=PT_MIDI_SETUP_OK)return r;
    c->busy=1;if(c->dispose_attempted)return hold(c);c->dispose_attempted=1;
    memcpy(&before,c,sizeof(before));disposed=pt_editor_dispose(c->editor);
    if(!same(c,&before)||!disposed)return hold(c);
    /* No former editor/project/history reads after this genuine dispose. */
    if(c->close_attempted)return hold(c);
    c->close_attempted=1;
    memcpy(&before,c,sizeof(before));r=pt_midi_setup_owner_close_controller(c->owner);
    if(!same(c,&before)||r!=PT_MIDI_SETUP_OK)return hold(c);
    if(c->release_attempted)return hold(c);
    c->release_attempted=1;
    memcpy(&before,c,sizeof(before));r=pt_midi_setup_owner_release_workspace(c->owner);
    if(!same(c,&before)||r!=PT_MIDI_SETUP_OK)return hold(c);
    c->state=PT_MIDI_FRONTEND_CLOSED;c->busy=0;return PT_MIDI_SETUP_OK;
}
enum pt_midi_frontend_state pt_midi_frontend_context_state(const struct pt_midi_frontend_context *c)
{return identity(c)?c->state:PT_MIDI_FRONTEND_INVALID;}
