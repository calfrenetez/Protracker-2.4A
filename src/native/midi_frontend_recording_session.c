#include "midi_frontend_recording_session.h"
#include "../editor/sampler_lifetime_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
struct pt_midi_recording_session {
 struct pt_midi_recording_session *self;size_t capacity;
 struct pt_midi_frontend_context *frontend;size_t frontend_capacity;
 struct pt_midi_setup_owner *owner;size_t owner_capacity;
 struct pt_editor_midi_recording *take;size_t take_capacity;
 struct pt_editor *editor;struct pt_midi_setup_owner_span context;
 enum pt_midi_recording_session_state state;unsigned busy;
 struct pt_editor_midi_record_status ended_status;
};
static int span(const void *p,size_t n){return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *p,size_t n,const void *q,size_t z)
{uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return span(p,n)&&span(q,z)&&(!n||!z||a>=b+z||b>=a+n);}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static int original_apart(const struct pt_editor *e,const void *p,size_t n)
{
 const struct pt_pattern_history *h=&e->history;
 return e->project&&h->command_capacity<=SIZE_MAX/sizeof(*h->commands)&&
  h->change_capacity<=SIZE_MAX/sizeof(*h->changes)&&apart(p,n,e,sizeof(*e))&&
  apart(p,n,e->project,sizeof(*e->project))&&
  apart(p,n,h->commands,h->command_capacity*sizeof(*h->commands))&&
  apart(p,n,h->changes,h->change_capacity*sizeof(*h->changes))&&
  pt_sampler_lifetime_output_disjoint(&e->sampler,p,n)&&
  pt_render_project_storage_output_disjoint(e->project,p,n);
}
static int identity(const struct pt_midi_recording_session *s)
{return s&&s->self==s&&s->capacity>=sizeof(*s)&&span(s,s->capacity);}
static int known_apart(const struct pt_midi_recording_session *s,const void *p,size_t n)
{return apart(p,n,s,s->capacity)&&apart(p,n,s->frontend,s->frontend_capacity)&&
 apart(p,n,s->owner,s->owner_capacity)&&apart(p,n,s->take,s->take_capacity)&&
 apart(p,n,s->editor,sizeof(*s->editor))&&apart(p,n,s->context.data,s->context.bytes);}
static void hold(struct pt_midi_recording_session *s){s->state=PT_MIDI_SESSION_HELD;s->busy=0;}
static enum pt_midi_setup_result setup_admit(const struct pt_midi_recording_session *s)
{return !identity(s)?PT_MIDI_SETUP_INVALID:s->busy?PT_MIDI_SETUP_BUSY:
 s->state==PT_MIDI_SESSION_HELD?PT_MIDI_SETUP_HELD:PT_MIDI_SETUP_OK;}
static enum pt_editor_midi_record_result record_admit(const struct pt_midi_recording_session *s)
{return !identity(s)?PT_EDITOR_RECORD_INVALID:s->busy?PT_EDITOR_RECORD_BUSY:
 s->state==PT_MIDI_SESSION_HELD?PT_EDITOR_RECORD_HELD:PT_EDITOR_RECORD_OK;}
static int frontend_is(struct pt_midi_recording_session *s,enum pt_midi_frontend_state expected)
{if(pt_midi_frontend_context_state(s->frontend)==expected)return 1;hold(s);return 0;}
static int original(struct pt_midi_recording_session *s)
{
 const struct pt_midi_setup_owner_transition_span spans[4]={
  {s,s->capacity,PT_MIDI_OWNER_TRANSITION_STORAGE},
  {s->frontend,s->frontend_capacity,PT_MIDI_OWNER_TRANSITION_STORAGE},
  {s->take,s->take_capacity,PT_MIDI_OWNER_TRANSITION_STORAGE},
  {s->context.data,s->context.bytes,PT_MIDI_OWNER_TRANSITION_CONTEXT_INPUT}};
 if(pt_midi_setup_owner_transition_admit(s->owner,s->owner_capacity,s->editor,spans,4)==PT_MIDI_SETUP_OK)return 1;
 hold(s);return 0;
}
static int same(const struct pt_midi_recording_session *s,const struct pt_midi_recording_session *b)
{return !memcmp(s,b,sizeof(*s));}
size_t pt_midi_recording_session_size(void){return sizeof(struct pt_midi_recording_session);}
size_t pt_midi_recording_session_alignment(void){return ALIGN_OF(struct pt_midi_recording_session);}
enum pt_midi_setup_result pt_midi_recording_session_construct(
 struct pt_midi_recording_session *s,size_t capacity,struct pt_midi_frontend_context *f,size_t fc,
 struct pt_midi_setup_owner *o,size_t oc,struct pt_editor_midi_recording *t,size_t tc,
 struct pt_editor *e,struct pt_midi_setup_owner_span context,size_t budget)
{
 const void *objects[4]={s,f,o,t};size_t sizes[4]={capacity,fc,oc,tc};unsigned i,j;
 struct pt_midi_recording_session before;enum pt_midi_setup_result r;
 if(!s||capacity<sizeof(*s)||(uintptr_t)s%ALIGN_OF(struct pt_midi_recording_session)||
  !f||fc<pt_midi_frontend_context_size()||(uintptr_t)f%pt_midi_frontend_context_alignment()||
  !o||oc<pt_midi_setup_owner_size()||(uintptr_t)o%pt_midi_setup_owner_alignment()||
  !t||tc<pt_editor_midi_recording_size()||(uintptr_t)t%pt_editor_midi_recording_alignment()||
  !e||!span(e,sizeof(*e))||!span(context.data,context.bytes)||
  context.data!=e->sampler.allocator.context)return PT_MIDI_SETUP_INVALID;
 for(i=0;i<4;++i){
  if(!span(objects[i],sizes[i])||!apart(objects[i],sizes[i],context.data,context.bytes)||
     !original_apart(e,objects[i],sizes[i]))return PT_MIDI_SETUP_INVALID;
  for(j=0;j<i;++j)if(!apart(objects[i],sizes[i],objects[j],sizes[j]))return PT_MIDI_SETUP_INVALID;
 }
 if(!zero(s,sizeof(*s))||!zero(f,pt_midi_frontend_context_size())||
    !zero(o,pt_midi_setup_owner_size())||!zero(t,pt_editor_midi_recording_size()))return PT_MIDI_SETUP_BUSY;
 s->self=s;s->capacity=capacity;s->frontend=f;s->frontend_capacity=fc;s->owner=o;s->owner_capacity=oc;
 s->take=t;s->take_capacity=tc;s->editor=e;s->context=context;s->state=PT_MIDI_SESSION_CONSTRUCTING;s->busy=1;
 memcpy(&before,s,sizeof(before));r=pt_midi_frontend_context_construct(f,fc,o,oc,e,context,budget);
 if(!same(s,&before)){hold(s);return PT_MIDI_SETUP_HELD;}
 if(r==PT_MIDI_SETUP_CAPACITY||r==PT_MIDI_SETUP_INVALID||r==PT_MIDI_SETUP_BUSY){
  if(!zero(f,pt_midi_frontend_context_size())||!zero(o,pt_midi_setup_owner_size())||
     !zero(t,pt_editor_midi_recording_size())){hold(s);return PT_MIDI_SETUP_HELD;}
  memset(s,0,sizeof(*s));return r;
 }
 if(r!=PT_MIDI_SETUP_OK||!frontend_is(s,PT_MIDI_FRONTEND_LIVE)||
    !pt_midi_setup_owner_storage_disjoint(o,oc,s,capacity)||
    !pt_midi_setup_owner_storage_disjoint(o,oc,t,tc)){hold(s);return PT_MIDI_SETUP_HELD;}
 s->state=PT_MIDI_SESSION_LIVE;s->busy=0;return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_recording_session_setup_begin(struct pt_midi_recording_session *s,unsigned track)
{
 enum pt_midi_setup_result r=setup_admit(s);struct pt_midi_recording_session before;
 if(r!=PT_MIDI_SETUP_OK)return r;
 if(s->state==PT_MIDI_SESSION_TAKE)return PT_MIDI_SETUP_ACTIVE;
 if(s->state!=PT_MIDI_SESSION_LIVE)return PT_MIDI_SETUP_IN_USE;
 if(!frontend_is(s,PT_MIDI_FRONTEND_LIVE))return PT_MIDI_SETUP_HELD;
 s->busy=1;memcpy(&before,s,sizeof(before));r=pt_midi_frontend_context_begin(s->frontend,track);
 if(!same(s,&before)){hold(s);return PT_MIDI_SETUP_HELD;}
 if(r==PT_MIDI_SETUP_HELD||!frontend_is(s,r==PT_MIDI_SETUP_OK?PT_MIDI_FRONTEND_MODAL:PT_MIDI_FRONTEND_LIVE)){
  hold(s);return PT_MIDI_SETUP_HELD;}
 s->state=r==PT_MIDI_SETUP_OK?PT_MIDI_SESSION_MODAL:PT_MIDI_SESSION_LIVE;s->busy=0;return r;
}
struct pt_midi_requester_model *pt_midi_recording_session_model(struct pt_midi_recording_session *s)
{
 if(setup_admit(s)!=PT_MIDI_SETUP_OK||s->state!=PT_MIDI_SESSION_MODAL)return NULL;
 if(!frontend_is(s,PT_MIDI_FRONTEND_MODAL))return NULL;
 return pt_midi_frontend_context_model(s->frontend);
}
enum pt_midi_setup_result pt_midi_recording_session_setup_end(struct pt_midi_recording_session *s)
{
 enum pt_midi_setup_result r=setup_admit(s);struct pt_midi_recording_session before;
 if(r!=PT_MIDI_SETUP_OK)return r;
 if(s->state!=PT_MIDI_SESSION_MODAL)return PT_MIDI_SETUP_IN_USE;
 if(!frontend_is(s,PT_MIDI_FRONTEND_MODAL))return PT_MIDI_SETUP_HELD;
 s->busy=1;memcpy(&before,s,sizeof(before));r=pt_midi_frontend_context_end(s->frontend);
 if(!same(s,&before)){hold(s);return PT_MIDI_SETUP_HELD;}
 if(r==PT_MIDI_SETUP_BUSY){s->busy=0;return r;}
 if(r!=PT_MIDI_SETUP_OK||!frontend_is(s,PT_MIDI_FRONTEND_LIVE)){hold(s);return PT_MIDI_SETUP_HELD;}
 s->state=PT_MIDI_SESSION_LIVE;s->busy=0;return PT_MIDI_SETUP_OK;
}
static enum pt_editor_midi_record_result record_done(struct pt_midi_recording_session *s,
 const struct pt_midi_recording_session *before,enum pt_editor_midi_record_result r)
{
 if(!same(s,before)||r==PT_EDITOR_RECORD_HELD||!frontend_is(s,PT_MIDI_FRONTEND_LIVE)||!original(s)){
  hold(s);return PT_EDITOR_RECORD_HELD;}
 s->busy=0;return r;
}
enum pt_editor_midi_record_result pt_midi_recording_session_record_begin(
 struct pt_midi_recording_session *s,const struct pt_record_settings *settings,uint64_t position,unsigned track,unsigned channel)
{
 struct pt_record_settings copy;struct pt_midi_recording_session before;
 enum pt_editor_midi_record_result r=record_admit(s);
 if(r!=PT_EDITOR_RECORD_OK)return r;
 if(s->state!=PT_MIDI_SESSION_LIVE)return PT_EDITOR_RECORD_CONFLICT;
 if(!settings||!span(settings,sizeof(*settings))||!known_apart(s,settings,sizeof(*settings)))return PT_EDITOR_RECORD_ALIAS;
 if(!frontend_is(s,PT_MIDI_FRONTEND_LIVE)||!original(s))return PT_EDITOR_RECORD_HELD;
 copy=*settings;s->busy=1;memcpy(&before,s,sizeof(before));
 r=pt_editor_midi_recording_begin(s->take,s->take_capacity,s->editor,s->context.bytes,&copy,position,track,channel);
 r=record_done(s,&before,r);if(r==PT_EDITOR_RECORD_OK)s->state=PT_MIDI_SESSION_TAKE;return r;
}
enum pt_editor_midi_record_result pt_midi_recording_session_message(
 struct pt_midi_recording_session *s,uint64_t position,const uint8_t *message,size_t bytes)
{
 uint8_t copy[3];struct pt_midi_recording_session before;enum pt_editor_midi_record_result r=record_admit(s);
 if(r!=PT_EDITOR_RECORD_OK)return r;
 if(s->state!=PT_MIDI_SESSION_TAKE)return PT_EDITOR_RECORD_CONFLICT;
 if(!frontend_is(s,PT_MIDI_FRONTEND_LIVE)||!original(s))return PT_EDITOR_RECORD_HELD;
 if((bytes!=2&&bytes!=3)||!span(message,bytes))return PT_EDITOR_RECORD_INVALID;
 memcpy(copy,message,bytes);s->busy=1;memcpy(&before,s,sizeof(before));
 r=pt_editor_midi_recording_message(s->take,position,copy,bytes);return record_done(s,&before,r);
}
enum pt_editor_midi_record_result pt_midi_recording_session_stop(struct pt_midi_recording_session *s,uint64_t position)
{
 struct pt_midi_recording_session before;enum pt_editor_midi_record_result r=record_admit(s);
 if(r!=PT_EDITOR_RECORD_OK)return r;
 if(s->state!=PT_MIDI_SESSION_TAKE)return PT_EDITOR_RECORD_CONFLICT;
 if(!frontend_is(s,PT_MIDI_FRONTEND_LIVE)||!original(s))return PT_EDITOR_RECORD_HELD;
 s->busy=1;memcpy(&before,s,sizeof(before));r=pt_editor_midi_recording_stop(s->take,position);return record_done(s,&before,r);
}
enum pt_editor_midi_record_result pt_midi_recording_session_cancel(struct pt_midi_recording_session *s)
{
 struct pt_midi_recording_session before;enum pt_editor_midi_record_result r=record_admit(s);
 if(r!=PT_EDITOR_RECORD_OK)return r;
 if(s->state!=PT_MIDI_SESSION_TAKE)return PT_EDITOR_RECORD_CONFLICT;
 if(!frontend_is(s,PT_MIDI_FRONTEND_LIVE)||!original(s))return PT_EDITOR_RECORD_HELD;
 s->busy=1;memcpy(&before,s,sizeof(before));r=pt_editor_midi_recording_cancel(s->take);return record_done(s,&before,r);
}
enum pt_editor_midi_record_result pt_midi_recording_session_detach(struct pt_midi_recording_session *s)
{
 struct pt_midi_recording_session before;struct pt_editor_midi_record_status ended;
 enum pt_editor_midi_record_result r=record_admit(s);
 if(r!=PT_EDITOR_RECORD_OK)return r;
 if(s->state!=PT_MIDI_SESSION_TAKE)return PT_EDITOR_RECORD_CONFLICT;
 if(!frontend_is(s,PT_MIDI_FRONTEND_LIVE)||!original(s))return PT_EDITOR_RECORD_HELD;
 s->busy=1;memcpy(&before,s,sizeof(before));r=pt_editor_midi_recording_detach(s->take);
 r=record_done(s,&before,r);if(r!=PT_EDITOR_RECORD_OK)return r;
 s->busy=1;r=pt_editor_midi_recording_get(s->take,&ended);
 if(r!=PT_EDITOR_RECORD_OK||ended.attached||ended.armed||ended.active){hold(s);return PT_EDITOR_RECORD_HELD;}
 s->ended_status=ended;memset(s->take,0,pt_editor_midi_recording_size());s->state=PT_MIDI_SESSION_LIVE;s->busy=0;return PT_EDITOR_RECORD_OK;
}
enum pt_editor_midi_record_result pt_midi_recording_session_record_status(
 struct pt_midi_recording_session *s,struct pt_editor_midi_record_status *out)
{
 struct pt_editor_midi_record_status value;struct pt_midi_recording_session before;
 enum pt_editor_midi_record_result r;
 if(!identity(s)||!out||!span(out,sizeof(*out)))return PT_EDITOR_RECORD_INVALID;
 if(!known_apart(s,out,sizeof(*out)))return PT_EDITOR_RECORD_ALIAS;
 r=record_admit(s);if(r!=PT_EDITOR_RECORD_OK)return r;
 if(s->state!=PT_MIDI_SESSION_TAKE)return PT_EDITOR_RECORD_CONFLICT;
 if(!frontend_is(s,PT_MIDI_FRONTEND_LIVE)||!original(s))return PT_EDITOR_RECORD_HELD;
 if(!original_apart(s->editor,out,sizeof(*out))||
    !pt_midi_setup_owner_storage_disjoint(s->owner,s->owner_capacity,out,sizeof(*out)))return PT_EDITOR_RECORD_ALIAS;
 s->busy=1;memcpy(&before,s,sizeof(before));r=pt_editor_midi_recording_get(s->take,&value);
 r=record_done(s,&before,r);if(r==PT_EDITOR_RECORD_OK)*out=value;return r;
}
enum pt_midi_setup_result pt_midi_recording_session_dispose_close(struct pt_midi_recording_session *s)
{
 struct pt_midi_recording_session before;enum pt_midi_setup_result r=setup_admit(s);
 if(r!=PT_MIDI_SETUP_OK)return r;
 if(s->state==PT_MIDI_SESSION_TAKE)return PT_MIDI_SETUP_ACTIVE;
 if(s->state!=PT_MIDI_SESSION_LIVE)return PT_MIDI_SETUP_IN_USE;
 if(!frontend_is(s,PT_MIDI_FRONTEND_LIVE))return PT_MIDI_SETUP_HELD;
 s->busy=1;memcpy(&before,s,sizeof(before));r=pt_midi_frontend_context_dispose_close(s->frontend);
 if(!same(s,&before)||r!=PT_MIDI_SETUP_OK||!frontend_is(s,PT_MIDI_FRONTEND_CLOSED)){
  hold(s);return PT_MIDI_SETUP_HELD;}
 s->state=PT_MIDI_SESSION_CLOSED;s->busy=0;return PT_MIDI_SETUP_OK;
}
enum pt_midi_recording_session_state pt_midi_recording_session_state(const struct pt_midi_recording_session *s)
{return identity(s)?s->state:PT_MIDI_SESSION_INVALID;}
