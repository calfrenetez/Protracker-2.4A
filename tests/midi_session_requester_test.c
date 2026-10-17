/* HOST protocol model only. The real editor/session/modal/owner/model/take,
 * allocator, master and journal are compiled unchanged. Only the two parent
 * protocol functions below are modelled; no OS structure or OS C is supplied.
 * Every retained fault remains rooted with its entire original stationary rig. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/native/midi_session_requester.h"
#include "mod_project.h"
#define BUDGET (1024U*1024U)
#define ROW(n) ((uint64_t)(n)<<16)
#define PARENT_BYTES 64U
#define PENDING_BYTES 16U
struct rig;
struct entry {void *pointer;size_t bytes;unsigned workspace;};
struct ledger {
 struct entry entries[128];size_t bytes;unsigned live,calls,frees,workspace_frees,record_frees;
 unsigned expect_workspace,reentry,probes;void *workspace;struct rig *rig;
};
enum model_mode {MODEL_APPLY,MODEL_CANCEL,MODEL_ESCAPE,MODEL_UNAVAILABLE,
 MODEL_DRAIN_HELD,MODEL_PENDING_HELD,MODEL_RESTORE_HELD,MODEL_END_DRIFT,MODEL_POST_END_DRIFT};
struct parent_model {
 struct rig *rig;enum model_mode mode;const char *name;
 struct pt_midi_requester_model *observed_model;
 unsigned calls,restored_calls,repaints,dispatch_guard,prepared_suspended,mask_restored;
 void *pending_identity;
};
struct rig {
 struct ledger *ledger;struct pt_document *document;struct pt_editor *editor;
 struct pt_midi_recording_session *session;struct pt_midi_frontend_context *frontend;
 struct pt_midi_setup_owner *owner;struct pt_editor_midi_recording *take;
 size_t sc,fc,oc,tc,pc;struct pt_midi_session_requester_progress *progress;
 struct pt_midi_session_requester_domain domain;struct parent_model *gui;
 struct Window *parent_identity;
 struct pt_sample_version *pin;struct pt_pcm master;struct pt_sample sample;
 int32_t pcm[4];size_t sampler_bytes;uint64_t generation;
};
static struct rig *retained[5];static unsigned completed;
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static void repaint(struct Window *parent,void *context)
{
 struct parent_model *g=context;
 assert(g&&g->rig&&parent==g->rig->parent_identity&&g->dispatch_guard&&g->prepared_suspended);
 ++g->repaints; /* A protocol callback count, never observed Intuition/pixel evidence. */
}
static enum pt_midi_session_requester_result invoke(struct rig *r)
{return pt_native_midi_session_requester(r->progress,r->pc,&r->domain,r->parent_identity,
 PARENT_BYTES,15,repaint,r->gui,sizeof(*r->gui));}
static void reentry(struct ledger *l)
{
 struct rig *r=l->rig;
 if(!l->reentry)return;
 assert(r&&r->progress->busy&&r->progress->begin_attempted&&r->progress->parent_attempted&&!r->progress->end_attempted);
 /* During genuine model allocation/discard, local BUSY is decided before any
  * caller descriptor or former source is followed. No second end is attempted. */
 assert(pt_native_midi_session_requester(r->progress,r->pc,NULL,NULL,0,0,NULL,NULL,0)==PT_MIDI_SESSION_REQUESTER_BUSY);
 assert(r->gui->observed_model&&pt_midi_requester_model_cancel(r->gui->observed_model)==PT_MIDI_SETUP_BUSY);
 ++l->probes;
}
static void *allocate(void *context,size_t bytes)
{
 struct ledger *l=context;unsigned i;void *p;++l->calls;reentry(l);
 if(bytes>BUDGET-l->bytes)return NULL;
 p=malloc(bytes);assert(p);for(i=0;i<128&&l->entries[i].pointer;++i){}
 assert(i<128);l->entries[i]=(struct entry){p,bytes,l->expect_workspace};
 if(l->expect_workspace){assert(!l->workspace);l->workspace=p;l->expect_workspace=0;}
 ++l->live;l->bytes+=bytes;return p;
}
static void release(void *context,void *p)
{
 struct ledger *l=context;unsigned i;reentry(l);
 for(i=0;i<128&&l->entries[i].pointer!=p;++i){}
 assert(i<128&&l->live&&l->bytes>=l->entries[i].bytes);
 if(l->entries[i].workspace){++l->workspace_frees;l->workspace=NULL;}
 else if(l->entries[i].bytes==pt_midi_setup_command_bytes())++l->record_frees;
 l->bytes-=l->entries[i].bytes;--l->live;++l->frees;l->entries[i]=(struct entry){0};free(p);
}
static struct rig *make(enum model_mode mode)
{
 static const uint8_t raw[12]={0x7f,0xff,0xff,0,0,1,0xff,0xff,0xff,0x80,0,0};
 const struct pt_raw_format format={48000,24,1,0,0};struct pt_allocator a;struct pt_channel ch;
 struct rig *r=calloc(1,sizeof(*r));struct pt_event_update u;assert(r);
 r->ledger=calloc(1,sizeof(*r->ledger));r->document=calloc(1,sizeof(*r->document));r->editor=calloc(1,sizeof(*r->editor));
 r->sc=pt_midi_recording_session_size()+32;r->fc=pt_midi_frontend_context_size()+32;
 r->oc=pt_midi_setup_owner_size()+32;r->tc=pt_editor_midi_recording_size()+32;
 r->pc=sizeof(*r->progress)+32;
 r->session=calloc(1,r->sc);r->frontend=calloc(1,r->fc);r->owner=calloc(1,r->oc);r->take=calloc(1,r->tc);
 r->progress=calloc(1,r->pc);r->gui=calloc(1,sizeof(*r->gui));
 /* These bytes have no Window/Message layout or OS fields. The HOST model uses
  * them only as original, separately allocated opaque identity/span tokens. */
 r->parent_identity=calloc(1,PARENT_BYTES);
 assert(r->ledger&&r->document&&r->editor&&r->session&&r->frontend&&r->owner&&r->take&&r->progress&&r->gui&&r->parent_identity);
 r->gui->pending_identity=malloc(PENDING_BYTES);assert(r->gui->pending_identity);
 memset(r->gui->pending_identity,0x73,PENDING_BYTES);
 r->ledger->rig=r;r->gui->rig=r;r->gui->mode=mode;r->gui->name="Composed input";
 r->gui->dispatch_guard=1;r->gui->prepared_suspended=1;
 a=(struct pt_allocator){r->ledger,allocate,release};pt_document_init(r->document,&a);
 assert(pt_document_new(r->document,16,BUDGET)==PT_PROJECT_OK&&pt_editor_init(r->editor,&r->document->project));
 /* The same empty-owner setup as the existing genuine session fixture, before
  * its first retained allocation. No live owner/controller is copied. */
 pt_sampler_init(&r->editor->sampler,&a,BUDGET);pt_song_init(&r->editor->song,&a,BUDGET);pt_document_init(&r->editor->sample_source,&a);
 assert(pt_sampler_import_raw(&r->editor->sampler,&r->document->project,&r->editor->history,0,raw,sizeof(raw),"Original24",&format)==PT_EDIT_OK);
 assert(pt_song_append(&r->editor->song,&r->document->project,&r->editor->history,0,1)==PT_EDIT_OK);
 ch=r->document->project.channels.track[15];ch.route=PT_MIDI;ch.midi_channel=3;
 assert(pt_pattern_channel_apply(&r->document->project,&r->editor->history,15,&ch)==PT_EDIT_OK);
 u.index=63*16+15;u.event=r->document->project.events[u.index];u.event.instrument=1;u.event.effect=14;u.event.parameter=0xc3;
 assert(pt_pattern_apply(&r->document->project,&r->editor->history,&u,1)==PT_EDIT_OK);
 assert(pt_sampler_pin(&r->editor->sampler,&r->document->project,0,r->editor->sampler.generation,&r->master,&r->pin)==PT_EDIT_OK);
 assert(r->master.bits==24&&r->master.channels==1&&r->master.frames==4);
 memcpy(r->pcm,r->master.data,sizeof(r->pcm));r->sample=r->document->project.samples[0];
 r->generation=r->editor->sampler.generation;r->sampler_bytes=r->editor->sampler.bytes;
 memset((char *)r->session+pt_midi_recording_session_size(),0xa1,32);
 memset((char *)r->frontend+pt_midi_frontend_context_size(),0xa2,32);
 memset((char *)r->owner+pt_midi_setup_owner_size(),0xa3,32);
 memset((char *)r->take+pt_editor_midi_recording_size(),0xa4,32);
 memset((char *)r->progress+sizeof(*r->progress),0xa5,32);
 r->domain=(struct pt_midi_session_requester_domain){r->session,r->sc,r->frontend,r->fc,r->owner,r->oc,r->take,r->tc,r->editor,
  {r->ledger,sizeof(*r->ledger)}};
 pt_editor_saved(r->editor);return r;
}
static void tails(const struct rig *r)
{
 unsigned i;for(i=0;i<32;++i){
  assert(((unsigned char *)r->session)[pt_midi_recording_session_size()+i]==0xa1);
  assert(((unsigned char *)r->frontend)[pt_midi_frontend_context_size()+i]==0xa2);
  assert(((unsigned char *)r->owner)[pt_midi_setup_owner_size()+i]==0xa3);
  assert(((unsigned char *)r->take)[pt_editor_midi_recording_size()+i]==0xa4);
  assert(((unsigned char *)r->progress)[sizeof(*r->progress)+i]==0xa5);
 }
}
static void master(const struct rig *r)
{
 const struct pt_sample *s=&r->document->project.samples[0];
 assert(r->editor->sampler.current[0]==r->pin&&r->editor->sampler.generation==r->generation&&r->editor->sampler.bytes==r->sampler_bytes);
 assert(!memcmp(s,&r->sample,sizeof(*s))&&s->pcm.data==r->master.data&&!memcmp(s->pcm.data,r->pcm,sizeof(r->pcm)));
}
static uint8_t *save_project(struct rig *r,size_t *n)
{
 size_t written;uint8_t *b;assert(pt_project_size(&r->document->project,n)==PT_PROJECT_OK);b=malloc(*n);assert(b);
 assert(pt_project_encode(&r->document->project,b,*n,&written)==PT_PROJECT_OK&&written==*n);return b;
}
static void same_project(struct rig *r,const uint8_t *gold,size_t n)
{size_t z;uint8_t *b=save_project(r,&z);assert(n==z&&!memcmp(b,gold,n));free(b);master(r);}
static void construct(struct rig *r)
{
 r->ledger->expect_workspace=1;
 assert(pt_midi_recording_session_construct(r->session,r->sc,r->frontend,r->fc,r->owner,r->oc,r->take,r->tc,r->editor,
  (struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},8*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK);
 r->ledger->expect_workspace=0;tails(r);
}
static void text(struct pt_midi_requester_model *m,const char *s)
{
 assert(m&&pt_midi_requester_model_select(m,PT_MIDI_SETUP_INPUT)==PT_MIDI_SETUP_OK);
 assert(pt_midi_requester_model_key(m,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
 for(;*s;++s)assert(pt_midi_requester_model_key(m,PT_MIDI_KEY_CHARACTER,(unsigned char)*s)==PT_MIDI_EVENT_CHANGED);
}
static void explicit_apply(struct rig *r,const char *name)
{
 struct pt_midi_requester_model *m;
 assert(pt_midi_recording_session_setup_begin(r->session,15)==PT_MIDI_SETUP_OK);
 m=pt_midi_recording_session_model(r->session);text(m,name);
 assert(pt_midi_requester_model_apply(m)==PT_MIDI_SETUP_OK&&!m->self);
 assert(pt_midi_recording_session_setup_end(r->session)==PT_MIDI_SETUP_OK);
}
static void later_explicit_setup(struct rig *r)
{
 struct pt_midi_requester_model *m;
 assert(pt_midi_recording_session_setup_begin(r->session,15)==PT_MIDI_SETUP_OK);
 m=pt_midi_recording_session_model(r->session);assert(m&&pt_midi_requester_model_cancel(m)==PT_MIDI_SETUP_OK);
 assert(pt_midi_recording_session_setup_end(r->session)==PT_MIDI_SETUP_OK);
}
/* GUI-MODEL IMPLEMENTATION ONLY. This replaces exactly the parent protocol
 * entry/observation, not session begin/model/end, modal return or model editing.
 * It never calls raw owner return, setup open, disposal, Stop or an OS function. */
enum pt_midi_parent_result pt_native_midi_requester_parent_window(
 struct pt_midi_requester_parent_progress *p,struct Window *parent,
 struct pt_midi_requester_model *m,pt_midi_parent_repaint paint,void *context,size_t bytes)
{
 struct parent_model *g=context;struct rig *r;
 assert(g&&bytes==sizeof(*g));r=g->rig;
 assert(r&&p==&r->progress->parent_progress&&parent==r->parent_identity&&paint==repaint);
 assert(g->dispatch_guard&&g->prepared_suspended&&!g->calls&&zero(p,sizeof(*p)));
 assert(r->progress->busy&&r->progress->begin_attempted&&r->progress->model_captured&&r->progress->parent_attempted&&!r->progress->end_attempted);
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_MODAL);
 assert(m==pt_midi_recording_session_model(r->session)&&m==r->progress->captured_model&&m->self==m);
 ++g->calls;g->observed_model=m;
 p->self=p;p->parent=parent;p->model=m;p->repaint=paint;p->repaint_context=context;p->repaint_bytes=bytes;
 p->state=PT_MIDI_PARENT_HELD;p->phase=PT_MIDI_PARENT_PRE_DRAIN;
 p->isolation_attempted=1;p->isolation_confirmed=1;
 if(g->mode==MODEL_DRAIN_HELD){
  p->get_calls[0]=PT_MIDI_PARENT_DRAIN_LIMIT;p->replied[0]=PT_MIDI_PARENT_DRAIN_LIMIT;return p->state;
 }
 if(g->mode==MODEL_PENDING_HELD){
  p->get_calls[0]=1;p->pending_message=(struct Message *)g->pending_identity;return p->state;
 }
 p->phase=PT_MIDI_PARENT_CHILD;p->window_called=1;
 if(g->mode==MODEL_APPLY){
  text(m,g->name);assert(pt_midi_requester_model_apply(m)==PT_MIDI_SETUP_OK&&!m->self);
  p->window_result=PT_MIDI_REQUESTER_ACCEPTED;
 }else if(g->mode==MODEL_UNAVAILABLE){
  assert(m->self==m&&m->stage==PT_MIDI_REQUESTER_CHOOSE);
  p->window_result=PT_MIDI_REQUESTER_UNAVAILABLE;
 }else {
  text(m,"Discarded draft");
  if(g->mode==MODEL_ESCAPE)assert(pt_midi_requester_model_key(m,PT_MIDI_KEY_ESCAPE,0)==PT_MIDI_EVENT_CANCEL);
  assert(pt_midi_requester_model_cancel(m)==PT_MIDI_SETUP_OK&&!m->self);
  p->window_result=PT_MIDI_REQUESTER_CANCELLED;
  /* A labelled genuine-model identity fault, not a stubbed end result. The
   * actual modal return must reject this unexpected controller mismatch. */
  if(g->mode==MODEL_END_DRIFT)m->setup=NULL;
 }
 if(g->mode==MODEL_RESTORE_HELD){
  /* The genuine Cancel has consumed model.self; the original modal ticket is
   * still borrowed. Parent uncertainty forbids inferring a ticket return. */
  p->phase=PT_MIDI_PARENT_RESTORE;p->restore_attempted=1;return p->state;
 }
 p->phase=PT_MIDI_PARENT_COMPLETE;p->restore_attempted=1;p->restore_confirmed=1;
 p->state=PT_MIDI_PARENT_RESTORED;g->mask_restored=1;
 return p->state;
}
int pt_native_midi_requester_parent_restored(const struct pt_midi_requester_parent_progress *p)
{
 struct parent_model *g=p->repaint_context;struct rig *r=g->rig;
 assert(p==&r->progress->parent_progress&&p->self==p&&p->parent==r->parent_identity);
 assert(g->calls==1&&!g->restored_calls&&r->progress->busy&&r->progress->end_attempted&&r->progress->restored_checked);
 assert(r->progress->end_result==PT_MIDI_SETUP_OK&&pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_LIVE);
 assert(g->dispatch_guard&&g->prepared_suspended);
 ++g->restored_calls;
 /* Inject modelled identity/mask uncertainty only AFTER the actual matched end.
  * This is a HOST protocol fault, never an observed native GUI mutation. */
 if(g->mode==MODEL_POST_END_DRIFT)g->mask_restored=0;
 return g->mask_restored&&p->state==PT_MIDI_PARENT_RESTORED&&p->restore_confirmed;
}
static void complete(struct rig *r)
{
 assert(r->progress->state==PT_MIDI_SESSION_REQUESTER_COMPLETE&&!r->progress->busy);
 assert(r->progress->begin_attempted==1&&r->progress->model_captured==1&&r->progress->parent_attempted==1&&
  r->progress->end_attempted==1&&r->progress->restored_checked==1);
 assert(r->progress->begin_result==PT_MIDI_SETUP_OK&&r->progress->end_result==PT_MIDI_SETUP_OK);
 assert(r->gui->calls==1&&r->gui->restored_calls==1&&r->gui->dispatch_guard&&r->gui->prepared_suspended);
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_LIVE);
 assert(!pt_midi_recording_session_model(r->session));master(r);tails(r);
}
static void finish(struct rig *r)
{
 struct pt_midi_session_requester_progress *p=r->progress;size_t pc=r->pc;
 complete(r);pt_sampler_unpin(r->pin);r->pin=NULL;
 assert(pt_midi_recording_session_dispose_close(r->session)==PT_MIDI_SETUP_OK);
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_CLOSED&&r->ledger->workspace_frees==1);
 tails(r);free(r->frontend);free(r->owner);free(r->take);free(r->editor);free(r->session);
 assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);free(r->document);
 assert(!r->ledger->live&&!r->ledger->bytes);free(r->ledger);
 free(r->gui->pending_identity);free(r->gui);free(r->parent_identity);free(r);
 /* Completed one-shot control observes only itself after all former sources,
  * descriptor, callbacks and opaque parent/model context have ended. */
 assert(pt_native_midi_session_requester(p,pc,NULL,NULL,0,0,NULL,NULL,0)==PT_MIDI_SESSION_REQUESTER_REFUSED);
 free(p);++completed;
}
/* Inert byte beforeimages only, never operational owner/controller clones.
 * The ledger enumerates this fixture's actual owned resource allocations. */
struct take_before {
 void *session,*frontend,*owner,*take,*editor,*document;struct ledger ledger;void *owned[128];
};
static void *copy_bytes(const void *p,size_t n)
{void *b=malloc(n);assert(b);memcpy(b,p,n);return b;}
static void take_before(struct rig *r,struct take_before *b)
{
 unsigned i;memset(b,0,sizeof(*b));b->session=copy_bytes(r->session,r->sc);b->frontend=copy_bytes(r->frontend,r->fc);
 b->owner=copy_bytes(r->owner,r->oc);b->take=copy_bytes(r->take,r->tc);b->editor=copy_bytes(r->editor,sizeof(*r->editor));
 b->document=copy_bytes(r->document,sizeof(*r->document));b->ledger=*r->ledger;
 for(i=0;i<128;++i)if(r->ledger->entries[i].pointer)b->owned[i]=copy_bytes(r->ledger->entries[i].pointer,r->ledger->entries[i].bytes);
}
static void take_unchanged(struct rig *r,struct take_before *b)
{
 unsigned i;assert(!memcmp(b->session,r->session,r->sc)&&!memcmp(b->frontend,r->frontend,r->fc)&&
  !memcmp(b->owner,r->owner,r->oc)&&!memcmp(b->take,r->take,r->tc)&&!memcmp(b->editor,r->editor,sizeof(*r->editor))&&
  !memcmp(b->document,r->document,sizeof(*r->document))&&!memcmp(&b->ledger,r->ledger,sizeof(*r->ledger)));
 for(i=0;i<128;++i)if(b->owned[i]){
  assert(!memcmp(b->owned[i],r->ledger->entries[i].pointer,r->ledger->entries[i].bytes));free(b->owned[i]);
 }
 free(b->session);free(b->frontend);free(b->owner);free(b->take);free(b->editor);free(b->document);
}
static void preflight(void)
{
 struct rig *r=make(MODEL_UNAVAILABLE);struct pt_midi_session_requester_domain d;unsigned calls;
 construct(r);calls=r->ledger->calls;
 assert(pt_native_midi_session_requester(r->progress,sizeof(*r->progress)-1,&r->domain,r->parent_identity,PARENT_BYTES,15,repaint,r->gui,sizeof(*r->gui))==PT_MIDI_SESSION_REQUESTER_REFUSED);
 d=r->domain;d.session_capacity=pt_midi_recording_session_size()-1;
 assert(pt_native_midi_session_requester(r->progress,r->pc,&d,r->parent_identity,PARENT_BYTES,15,repaint,r->gui,sizeof(*r->gui))==PT_MIDI_SESSION_REQUESTER_REFUSED);
 assert(pt_native_midi_session_requester(r->progress,r->pc,&r->domain,r->parent_identity,PARENT_BYTES,15,repaint,r->progress,sizeof(*r->progress))==PT_MIDI_SESSION_REQUESTER_REFUSED);
 assert(pt_native_midi_session_requester(r->progress,r->pc,&r->domain,(struct Window *)r->progress,sizeof(*r->progress),15,repaint,r->gui,sizeof(*r->gui))==PT_MIDI_SESSION_REQUESTER_REFUSED);
 assert(zero(r->progress,sizeof(*r->progress))&&!r->gui->calls&&r->ledger->calls==calls);
 assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_COMPLETE);finish(r);
}
static void applied_record_survives_return(void)
{
 struct rig *r=make(MODEL_APPLY);size_t n,z,start;uint8_t *before,*after;struct pt_document decoded;
 struct pt_allocator a={r->ledger,allocate,release};
 construct(r);explicit_apply(r,"Old redo");assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
 before=save_project(r,&n);start=r->editor->history.cursor;
 assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_COMPLETE);complete(r);
 assert(!strcmp(r->document->project.midi_input,"Composed input")&&r->editor->history.cursor==start+1);
 after=save_project(r,&z);pt_document_init(&decoded,&a);assert(pt_document_load(&decoded,after,z,BUDGET)==PT_PROJECT_OK);
 assert(!strcmp(decoded.project.midi_input,"Composed input")&&decoded.project.samples[0].pcm.bits==24&&
  !memcmp(decoded.project.samples[0].pcm.data,r->pcm,sizeof(r->pcm)));
 assert(pt_document_release_checked(&decoded)==PT_DOCUMENT_RELEASED);
 assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);same_project(r,before,n);
 assert(pt_pattern_undo(&r->document->project,&r->editor->history,1)==PT_EDIT_OK);same_project(r,after,z);
 pt_editor_saved(r->editor);assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
 assert(pt_pattern_undo(&r->document->project,&r->editor->history,1)==PT_EDIT_OK&&!pt_editor_dirty(r->editor));
 free(before);free(after);later_explicit_setup(r);finish(r);
}
static void unchanged_choice(enum model_mode mode)
{
 struct rig *r=make(mode);size_t n,cursor,count;uint8_t *gold;unsigned calls,frees;int dirty;
 construct(r);explicit_apply(r,"Retained redo");assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
 gold=save_project(r,&n);cursor=r->editor->history.cursor;count=r->editor->history.count;
 calls=r->ledger->calls;frees=r->ledger->frees;dirty=pt_editor_dirty(r->editor);
 assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_COMPLETE);complete(r);same_project(r,gold,n);
 assert(pt_editor_dirty(r->editor)==dirty);
 assert(r->editor->history.cursor==cursor&&r->editor->history.count==count&&r->ledger->calls==calls&&r->ledger->frees==frees);
 assert(pt_pattern_undo(&r->document->project,&r->editor->history,1)==PT_EDIT_OK&&
  !strcmp(r->document->project.midi_input,"Retained redo"));
 assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);same_project(r,gold,n);
 later_explicit_setup(r);same_project(r,gold,n);free(gold);finish(r);
}
static void attached_take_precedes_ui(void)
{
 struct rig *r=make(MODEL_UNAVAILABLE);struct take_before b;struct pt_editor_midi_record_status s;
 const struct pt_record_settings options={0,128,1,1,1,0};const uint8_t note[3]={0x9f,60,100};
 construct(r);assert(pt_midi_recording_session_record_begin(r->session,&options,0,15,16)==PT_EDITOR_RECORD_OK);
 assert(pt_midi_recording_session_record_status(r->session,&s)==PT_EDITOR_RECORD_OK&&s.armed&&s.attached);
 take_before(r,&b);assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_REFUSED);take_unchanged(r,&b);
 assert(zero(r->progress,sizeof(*r->progress))&&!r->gui->calls);
 assert(pt_midi_recording_session_message(r->session,ROW(1),note,3)==PT_EDITOR_RECORD_OK);
 assert(pt_midi_recording_session_stop(r->session,ROW(2))==PT_EDITOR_RECORD_OK);
 assert(pt_midi_recording_session_record_status(r->session,&s)==PT_EDITOR_RECORD_OK&&!s.armed&&!s.active&&s.attached);
 take_before(r,&b);assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_REFUSED);take_unchanged(r,&b);
 assert(zero(r->progress,sizeof(*r->progress))&&!r->gui->calls);
 assert(pt_midi_recording_session_detach(r->session)==PT_EDITOR_RECORD_OK&&zero(r->take,pt_editor_midi_recording_size()));
 assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_COMPLETE);finish(r);
}
static void genuine_allocator_callback_busy(void)
{
 struct rig *r=make(MODEL_APPLY);unsigned probes;
 construct(r);explicit_apply(r,"Discard this redo");assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
 probes=r->ledger->probes;r->ledger->reentry=1;
 assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_COMPLETE);r->ledger->reentry=0;
 assert(r->ledger->probes>=probes+2&&r->ledger->record_frees>=1);complete(r);finish(r);
}
static void retain_parent(enum model_mode mode,unsigned index)
{
 struct rig *r=make(mode);unsigned frees;construct(r);frees=r->ledger->frees;
 assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_HELD);
 assert(r->progress->begin_attempted==1&&r->progress->model_captured==1&&r->progress->parent_attempted==1&&
  !r->progress->end_attempted&&!r->progress->restored_checked&&!r->progress->busy);
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_MODAL);
 if(mode==MODEL_RESTORE_HELD)assert(!r->gui->observed_model->self&&!pt_midi_recording_session_model(r->session));
 else assert(pt_midi_recording_session_model(r->session)==r->gui->observed_model);
 assert(r->gui->calls==1&&!r->gui->restored_calls&&r->ledger->frees==frees&&!r->ledger->workspace_frees);
 assert(pt_native_midi_session_requester(r->progress,r->pc,NULL,NULL,0,0,NULL,NULL,0)==PT_MIDI_SESSION_REQUESTER_HELD);
 assert(r->gui->calls==1&&!r->progress->end_attempted&&r->gui->dispatch_guard&&r->gui->prepared_suspended);
 if(mode==MODEL_PENDING_HELD){unsigned i;assert(r->progress->parent_progress.pending_message==(struct Message *)r->gui->pending_identity);
  for(i=0;i<PENDING_BYTES;++i)assert(((unsigned char *)r->gui->pending_identity)[i]==0x73);}
 master(r);tails(r);retained[index]=r;
}
static void retain_failed_actual_end(void)
{
 struct rig *r=make(MODEL_END_DRIFT);unsigned frees;construct(r);frees=r->ledger->frees;
 assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_HELD);
 assert(r->progress->end_attempted==1&&r->progress->end_result==PT_MIDI_SETUP_HELD&&!r->progress->restored_checked);
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_HELD);
 assert(r->gui->calls==1&&!r->gui->restored_calls&&r->ledger->frees==frees&&!r->ledger->workspace_frees);
 assert(pt_native_midi_session_requester(r->progress,r->pc,NULL,NULL,0,0,NULL,NULL,0)==PT_MIDI_SESSION_REQUESTER_HELD);
 assert(r->progress->end_attempted==1&&r->gui->calls==1&&r->gui->dispatch_guard&&r->gui->prepared_suspended);
 master(r);tails(r);retained[3]=r;
}
static void retain_after_consumed_end(void)
{
 struct rig *r=make(MODEL_POST_END_DRIFT);unsigned frees;construct(r);frees=r->ledger->frees;
 assert(invoke(r)==PT_MIDI_SESSION_REQUESTER_HELD);
 assert(r->progress->end_attempted==1&&r->progress->end_result==PT_MIDI_SETUP_OK&&r->progress->restored_checked==1);
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_LIVE&&!pt_midi_recording_session_model(r->session));
 assert(r->gui->calls==1&&r->gui->restored_calls==1&&r->ledger->frees==frees&&!r->ledger->workspace_frees);
 assert(pt_native_midi_session_requester(r->progress,r->pc,NULL,NULL,0,0,NULL,NULL,0)==PT_MIDI_SESSION_REQUESTER_HELD);
 assert(r->progress->end_attempted==1&&r->gui->calls==1&&r->gui->dispatch_guard&&r->gui->prepared_suspended);
 master(r);tails(r);retained[4]=r;
}
int main(void)
{
 preflight();applied_record_survives_return();unchanged_choice(MODEL_CANCEL);unchanged_choice(MODEL_ESCAPE);
 unchanged_choice(MODEL_UNAVAILABLE);attached_take_precedes_ui();genuine_allocator_callback_busy();
 retain_parent(MODEL_DRAIN_HELD,0);retain_parent(MODEL_PENDING_HELD,1);retain_parent(MODEL_RESTORE_HELD,2);
 retain_failed_actual_end();retain_after_consumed_end();
 assert(completed==7&&retained[0]&&retained[1]&&retained[2]&&retained[3]&&retained[4]);
 puts("MIDI SESSION REQUESTER HOST MODEL PASS:12 cases;7 complete;5 retained;genuine session/modal/master/save/undo/take;parent protocol MODEL ONLY;NO OS/INPUT/CAMD/TIMING/OUTPUT");return 0;
}
