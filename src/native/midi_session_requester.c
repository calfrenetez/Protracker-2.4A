#include "midi_session_requester.h"
#include "../editor/sampler_lifetime_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>
#if defined(PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY)
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#endif
#define ALIGN_OF(t) offsetof(struct {char c;t value;},value)
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
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
static int domain_admit(const struct pt_midi_session_requester_domain *d,
 const struct pt_midi_session_requester_progress *p,size_t capacity,
 const void *descriptor,void *repaint_context,size_t repaint_bytes)
{
 const void *objects[4]={p,d->session,d->frontend,d->take};
 size_t sizes[4]={capacity,d->session_capacity,d->frontend_capacity,d->take_capacity};
 struct pt_midi_setup_owner_transition_span checks[7];unsigned i,j;
 if(!d->session||d->session_capacity<pt_midi_recording_session_size()||
    (uintptr_t)d->session%pt_midi_recording_session_alignment()||
    !d->frontend||d->frontend_capacity<pt_midi_frontend_context_size()||
    (uintptr_t)d->frontend%pt_midi_frontend_context_alignment()||
    !d->owner||d->owner_capacity<pt_midi_setup_owner_size()||
    (uintptr_t)d->owner%pt_midi_setup_owner_alignment()||
    !d->take||d->take_capacity<pt_editor_midi_recording_size()||
    (uintptr_t)d->take%pt_editor_midi_recording_alignment()||
    !d->editor||!span(d->editor,sizeof(*d->editor))||
    !span(d->allocator_context.data,d->allocator_context.bytes)||
    d->allocator_context.data!=d->editor->sampler.allocator.context)return 0;
 for(i=0;i<4;++i){
  if(!span(objects[i],sizes[i])||!apart(objects[i],sizes[i],d->allocator_context.data,d->allocator_context.bytes))return 0;
  for(j=0;j<i;++j)if(!apart(objects[i],sizes[i],objects[j],sizes[j]))return 0;
 }
 if(!original_apart(d->editor,p,capacity)||
    !apart(p,capacity,descriptor,sizeof(*d))||
    !apart(p,capacity,repaint_context,repaint_bytes))return 0;
 checks[0]=(struct pt_midi_setup_owner_transition_span){p,capacity,PT_MIDI_OWNER_TRANSITION_STORAGE};
 checks[1]=(struct pt_midi_setup_owner_transition_span){d->session,d->session_capacity,PT_MIDI_OWNER_TRANSITION_STORAGE};
 checks[2]=(struct pt_midi_setup_owner_transition_span){d->frontend,d->frontend_capacity,PT_MIDI_OWNER_TRANSITION_STORAGE};
 checks[3]=(struct pt_midi_setup_owner_transition_span){d->take,d->take_capacity,PT_MIDI_OWNER_TRANSITION_STORAGE};
 checks[4]=(struct pt_midi_setup_owner_transition_span){d->allocator_context.data,d->allocator_context.bytes,PT_MIDI_OWNER_TRANSITION_CONTEXT_INPUT};
 checks[5]=(struct pt_midi_setup_owner_transition_span){repaint_context,repaint_bytes,PT_MIDI_OWNER_TRANSITION_CONTEXT_INPUT};
 checks[6]=(struct pt_midi_setup_owner_transition_span){descriptor,sizeof(*d),PT_MIDI_OWNER_TRANSITION_STORAGE};
 return pt_midi_setup_owner_transition_admit(d->owner,d->owner_capacity,d->editor,checks,7)==PT_MIDI_SETUP_OK;
}
#if defined(PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY)
static int capture_task(struct ExecBase **exec,struct Task **task,struct Library **intuition)
{
 struct ExecBase *e=SysBase;struct Library *i=(struct Library *)IntuitionBase;struct Task *t;
 if(!e||!i||i->lib_Version<37||(int)(signed char)e->IDNestCnt!=-1||
    (int)(signed char)e->TDNestCnt!=-1)return 0;
 t=FindTask(NULL);
 if(!t||e!=SysBase||e->ThisTask!=t||i!=(struct Library *)IntuitionBase||
    i->lib_Version<37||(int)(signed char)e->IDNestCnt!=-1||
    (int)(signed char)e->TDNestCnt!=-1)return 0;
 *exec=e;*task=t;*intuition=i;return 1;
}
static int task_current(const struct pt_midi_session_requester_progress *p)
{return SysBase==p->exec&&p->exec->ThisTask==p->task&&
 (int)(signed char)p->exec->IDNestCnt==-1&&(int)(signed char)p->exec->TDNestCnt==-1&&
 (struct Library *)IntuitionBase==p->intuition&&p->intuition->lib_Version>=37;}
static int gui_apart(const void *p,size_t n,struct Window *w,size_t bytes,
 struct ExecBase *e,struct Task *t,struct Library *i)
{
 return bytes>=sizeof(*w)&&w->UserPort&&w->WScreen&&w->RPort&&
  apart(p,n,w,bytes)&&apart(p,n,w->UserPort,sizeof(*w->UserPort))&&
  apart(p,n,w->WScreen,sizeof(*w->WScreen))&&apart(p,n,w->RPort,sizeof(*w->RPort))&&
  apart(p,n,e,sizeof(*e))&&apart(p,n,t,sizeof(*t))&&apart(p,n,i,sizeof(*i));
}
#else
/* HOST models supply protocol-only opaque GUI identities, never fake OS types.
 * These branches assert no native Task/GUI admission or scheduling behavior. */
static int capture_task(struct ExecBase **exec,struct Task **task,struct Library **intuition)
{*exec=NULL;*task=NULL;*intuition=NULL;return 1;}
static int task_current(const struct pt_midi_session_requester_progress *p)
{(void)p;return 1;}
static int gui_apart(const void *p,size_t n,struct Window *w,size_t bytes,
 struct ExecBase *e,struct Task *t,struct Library *i)
{(void)e;(void)t;(void)i;return apart(p,n,w,bytes);}
#endif
static enum pt_midi_session_requester_result held(struct pt_midi_session_requester_progress *p)
{p->state=PT_MIDI_SESSION_REQUESTER_HELD;p->busy=0;return p->state;}
static int same_control(const struct pt_midi_session_requester_progress *p,
 const struct pt_midi_session_requester_progress *b)
{return !memcmp(p,b,offsetof(struct pt_midi_session_requester_progress,parent_progress));}
enum pt_midi_session_requester_result pt_native_midi_session_requester(
 struct pt_midi_session_requester_progress *p,size_t capacity,
 const struct pt_midi_session_requester_domain *domain,struct Window *parent,size_t parent_bytes,
 unsigned track,pt_midi_parent_repaint repaint,void *repaint_context,size_t repaint_bytes)
{
 struct pt_midi_session_requester_domain d;struct pt_midi_session_requester_progress before;
 struct ExecBase *exec;struct Task *task;struct Library *intuition;
 enum pt_midi_setup_result r;enum pt_midi_parent_result parent_result;
 if(!p||capacity<sizeof(*p)||!span(p,capacity)||
    (uintptr_t)p%ALIGN_OF(struct pt_midi_session_requester_progress))return PT_MIDI_SESSION_REQUESTER_REFUSED;
 if(p->self==p){
  if(p->capacity!=capacity)return PT_MIDI_SESSION_REQUESTER_REFUSED;
  if(p->busy)return PT_MIDI_SESSION_REQUESTER_BUSY;
  return p->state==PT_MIDI_SESSION_REQUESTER_HELD?p->state:PT_MIDI_SESSION_REQUESTER_REFUSED;
 }
 if(!zero(p,sizeof(*p))||!domain||!span(domain,sizeof(*domain))||
    !apart(p,capacity,domain,sizeof(*domain))||!parent||!parent_bytes||!span(parent,parent_bytes)||
    !repaint||!span(repaint_context,repaint_bytes)||!apart(p,capacity,parent,parent_bytes)||
    !apart(p,capacity,repaint_context,repaint_bytes))return PT_MIDI_SESSION_REQUESTER_REFUSED;
 d=*domain;
 /* Reject non-LIVE sessions before source traversal. Genuine setup_begin
  * independently retains its attached-TAKE refusal before any UI call. */
 if(pt_midi_recording_session_state(d.session)!=PT_MIDI_SESSION_LIVE)return PT_MIDI_SESSION_REQUESTER_REFUSED;
 if(!domain_admit(&d,p,capacity,domain,repaint_context,repaint_bytes)||
    !capture_task(&exec,&task,&intuition)||!gui_apart(p,capacity,parent,parent_bytes,exec,task,intuition))
  return PT_MIDI_SESSION_REQUESTER_REFUSED;
 p->self=p;p->capacity=capacity;p->domain=d;p->original_parent=parent;p->parent_bytes=parent_bytes;
 p->track=track;p->repaint=repaint;p->repaint_context=repaint_context;p->repaint_bytes=repaint_bytes;
 p->exec=exec;p->task=task;p->intuition=intuition;p->busy=1;p->state=PT_MIDI_SESSION_REQUESTER_HELD;
 p->begin_attempted=1;memcpy(&before,p,sizeof(before));
 r=pt_midi_recording_session_setup_begin(d.session,track);
 if(!same_control(p,&before))return held(p);
 p->begin_result=r;
 if(r!=PT_MIDI_SETUP_OK){
  if(r==PT_MIDI_SETUP_HELD||pt_midi_recording_session_state(d.session)!=PT_MIDI_SESSION_LIVE)return held(p);
  p->state=PT_MIDI_SESSION_REQUESTER_REFUSED;p->busy=0;return p->state;
 }
 if(!task_current(p))return held(p);
 /* Capture the actual model ONCE. No later model.self test/re-query is a
  * completion certificate: genuine Apply may consume it before ticket return. */
 p->captured_model=pt_midi_recording_session_model(d.session);p->model_captured=1;
 if(!p->captured_model||!task_current(p))return held(p);
 p->parent_attempted=1;memcpy(&before,p,sizeof(before));
 parent_result=pt_native_midi_requester_parent_window(&p->parent_progress,parent,
  p->captured_model,repaint,repaint_context,repaint_bytes);
 if(!same_control(p,&before)||parent_result!=PT_MIDI_PARENT_RESTORED||!task_current(p))return held(p);
 p->end_attempted=1;memcpy(&before,p,sizeof(before));
 r=pt_midi_recording_session_setup_end(d.session);
 if(!same_control(p,&before))return held(p);
 p->end_result=r;
 if(r!=PT_MIDI_SETUP_OK||pt_midi_recording_session_state(d.session)!=PT_MIDI_SESSION_LIVE||
    !task_current(p)||!domain_admit(&d,p,capacity,domain,repaint_context,repaint_bytes))return held(p);
 p->restored_checked=1;memcpy(&before,p,sizeof(before));
 if(!pt_native_midi_requester_parent_restored(&p->parent_progress)||
    !same_control(p,&before)||!task_current(p))return held(p);
 p->state=PT_MIDI_SESSION_REQUESTER_COMPLETE;p->busy=0;return p->state;
}
