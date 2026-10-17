#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/native/midi_frontend_recording_session.h"
#include "mod_project.h"
#define BUDGET (1024U*1024U)
#define ROW(n) ((uint64_t)(n)<<16)
struct entry {void *pointer;size_t bytes;unsigned workspace;};
struct ledger {
 struct entry entries[128];size_t bytes;unsigned live,calls,frees,workspace_frees,record_frees;
 unsigned expect_workspace,fail,alias,reentry,probes;void *workspace;
 struct pt_midi_recording_session *session;struct pt_editor *editor;
};
struct rig {
 struct ledger *ledger;struct pt_document *document;struct pt_editor *editor;
 struct pt_midi_recording_session *session;struct pt_midi_frontend_context *frontend;
 struct pt_midi_setup_owner *owner;struct pt_editor_midi_recording *take;
 size_t sc,fc,oc,tc;struct ledger *foreign;struct pt_midi_setup_owner_borrow *foreign_ticket;
 struct pt_midi_setup *foreign_setup;struct pt_sample_version *pin;struct pt_pcm master;
 struct pt_sample sample;int32_t pcm[4];size_t sampler_bytes;uint64_t generation;
};
static struct rig *retained[5];static unsigned completed;
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static void reentry(struct ledger *l)
{
 struct pt_editor_midi_record_status value,before;const uint8_t b[3]={0x9f,60,100};
 if(!l->reentry)return;
 memset(&value,0x35,sizeof(value));before=value;
 assert(pt_midi_recording_session_setup_begin(l->session,15)==PT_MIDI_SETUP_BUSY);
 assert(pt_midi_recording_session_setup_end(l->session)==PT_MIDI_SETUP_BUSY);
 assert(pt_midi_recording_session_dispose_close(l->session)==PT_MIDI_SETUP_BUSY);
 assert(!pt_midi_recording_session_model(l->session));
 assert(pt_midi_recording_session_message(l->session,0,b,3)==PT_EDITOR_RECORD_BUSY);
 assert(pt_midi_recording_session_stop(l->session,0)==PT_EDITOR_RECORD_BUSY);
 assert(pt_midi_recording_session_cancel(l->session)==PT_EDITOR_RECORD_BUSY);
 assert(pt_midi_recording_session_detach(l->session)==PT_EDITOR_RECORD_BUSY);
 assert(pt_midi_recording_session_record_status(l->session,&value)==PT_EDITOR_RECORD_BUSY);
 assert(pt_midi_recording_session_record_status(l->session,(void *)l->session)==PT_EDITOR_RECORD_ALIAS);
 assert(!memcmp(&value,&before,sizeof(value)));++l->probes;
}
static void *allocate(void *context,size_t bytes)
{
 struct ledger *l=context;unsigned i;void *p;++l->calls;reentry(l);
 if(l->alias)return l->editor;
 if(l->fail||bytes>BUDGET-l->bytes)return NULL;
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
static struct rig *make(void)
{
 static const uint8_t raw[12]={0x7f,0xff,0xff,0,0,1,0xff,0xff,0xff,0x80,0,0};
 const struct pt_raw_format format={48000,24,1,0,0};struct pt_allocator a;struct pt_channel ch;
 struct rig *r=calloc(1,sizeof(*r));struct pt_event_update u;assert(r);
 r->ledger=calloc(1,sizeof(*r->ledger));r->document=calloc(1,sizeof(*r->document));r->editor=calloc(1,sizeof(*r->editor));
 r->sc=pt_midi_recording_session_size()+32;r->fc=pt_midi_frontend_context_size()+32;
 r->oc=pt_midi_setup_owner_size()+32;r->tc=pt_editor_midi_recording_size()+32;
 r->session=calloc(1,r->sc);r->frontend=calloc(1,r->fc);r->owner=calloc(1,r->oc);r->take=calloc(1,r->tc);
 assert(r->ledger&&r->document&&r->editor&&r->session&&r->frontend&&r->owner&&r->take);
 r->ledger->session=r->session;r->ledger->editor=r->editor;
 a=(struct pt_allocator){r->ledger,allocate,release};pt_document_init(r->document,&a);
 assert(pt_document_new(r->document,16,BUDGET)==PT_PROJECT_OK&&pt_editor_init(r->editor,&r->document->project));
 /* Only genuinely empty owners are rebound, before first retained allocation. */
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
 pt_editor_saved(r->editor);return r;
}
static void tails(const struct rig *r)
{
 unsigned i;for(i=0;i<32;++i){
  assert(((unsigned char *)r->session)[pt_midi_recording_session_size()+i]==0xa1);
  assert(((unsigned char *)r->frontend)[pt_midi_frontend_context_size()+i]==0xa2);
  assert(((unsigned char *)r->owner)[pt_midi_setup_owner_size()+i]==0xa3);
  assert(((unsigned char *)r->take)[pt_editor_midi_recording_size()+i]==0xa4);
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
static enum pt_midi_setup_result construct(struct rig *r)
{
 enum pt_midi_setup_result x;r->ledger->expect_workspace=1;
 x=pt_midi_recording_session_construct(r->session,r->sc,r->frontend,r->fc,r->owner,r->oc,r->take,r->tc,r->editor,
  (struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},8*pt_midi_setup_command_bytes());
 r->ledger->expect_workspace=0;tails(r);return x;
}
static enum pt_editor_midi_record_result begin(struct rig *r,uint32_t first,uint32_t limit,unsigned hold)
{
 const struct pt_record_settings options={first,limit,1,(uint8_t)hold,1,0};
 return pt_midi_recording_session_record_begin(r->session,&options,0,15,16);
}
static enum pt_editor_midi_record_result send(struct rig *r,uint64_t position,unsigned note,unsigned velocity)
{const uint8_t b[3]={0x9f,(uint8_t)note,(uint8_t)velocity};return pt_midi_recording_session_message(r->session,position,b,3);}
static struct pt_editor_midi_record_status status(struct rig *r)
{struct pt_editor_midi_record_status s;assert(pt_midi_recording_session_record_status(r->session,&s)==PT_EDITOR_RECORD_OK);return s;}
static void text(struct pt_midi_requester_model *m,const char *s)
{
 assert(m&&pt_midi_requester_model_select(m,PT_MIDI_SETUP_INPUT)==PT_MIDI_SETUP_OK);
 assert(pt_midi_requester_model_key(m,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
 for(;*s;++s)assert(pt_midi_requester_model_key(m,PT_MIDI_KEY_CHARACTER,(unsigned char)*s)==PT_MIDI_EVENT_CHANGED);
}
static void apply(struct rig *r,const char *name)
{
 struct pt_midi_requester_model *m;
 assert(pt_midi_recording_session_setup_begin(r->session,15)==PT_MIDI_SETUP_OK);
 m=pt_midi_recording_session_model(r->session);text(m,name);
 assert(pt_midi_requester_model_apply(m)==PT_MIDI_SETUP_OK&&!m->self);
 assert(!pt_midi_recording_session_model(r->session));
 assert(pt_midi_recording_session_setup_end(r->session)==PT_MIDI_SETUP_OK);
}
static void take_refuses_setup_and_shutdown(struct rig *r)
{
 void *s=malloc(r->sc),*f=malloc(r->fc),*o=malloc(r->oc),*t=malloc(r->tc),*w=malloc(pt_midi_setup_size());assert(s&&f&&o&&t&&w);
 memcpy(s,r->session,r->sc);memcpy(f,r->frontend,r->fc);memcpy(o,r->owner,r->oc);memcpy(t,r->take,r->tc);memcpy(w,r->ledger->workspace,pt_midi_setup_size());
 assert(pt_midi_recording_session_setup_begin(r->session,15)==PT_MIDI_SETUP_ACTIVE);
 assert(pt_midi_recording_session_dispose_close(r->session)==PT_MIDI_SETUP_ACTIVE);
 assert(!pt_midi_recording_session_model(r->session));
 assert(pt_midi_frontend_context_state(r->frontend)==PT_MIDI_FRONTEND_LIVE);
 assert(!memcmp(s,r->session,r->sc)&&!memcmp(f,r->frontend,r->fc)&&!memcmp(o,r->owner,r->oc)&&!memcmp(t,r->take,r->tc)&&!memcmp(w,r->ledger->workspace,pt_midi_setup_size()));
 free(s);free(f);free(o);free(t);free(w);tails(r);
}
static void finish(struct rig *r,unsigned constructed)
{
 struct pt_midi_recording_session *s=r->session;struct pt_editor_midi_record_status out,before;
 unsigned probes=r->ledger->probes,frees=r->ledger->frees;struct pt_midi_setup_owner_status own;
 unsigned expected=0;if(constructed&&r->ledger->reentry){assert(pt_midi_setup_owner_get(r->owner,&own)==PT_MIDI_SETUP_OK);expected=own.commands+1;}
 master(r);pt_sampler_unpin(r->pin);r->pin=NULL;
 if(constructed){assert(pt_midi_recording_session_dispose_close(s)==PT_MIDI_SETUP_OK);
  assert(pt_midi_recording_session_state(s)==PT_MIDI_SESSION_CLOSED&&r->ledger->workspace_frees==1);
  if(expected){assert(r->ledger->probes-probes==r->ledger->frees-frees&&r->ledger->probes-probes>=expected);r->ledger->reentry=0;}}
 else assert(pt_editor_dispose(r->editor));
 tails(r);free(r->frontend);free(r->owner);free(r->take);free(r->editor);
 assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);free(r->document);
 assert(!r->ledger->live&&!r->ledger->bytes);free(r->ledger);free(r);
 if(constructed){
  memset(&out,0x47,sizeof(out));before=out;
  assert(pt_midi_recording_session_state(s)==PT_MIDI_SESSION_CLOSED);
  assert(pt_midi_recording_session_setup_begin(s,0)==PT_MIDI_SETUP_IN_USE);
  assert(pt_midi_recording_session_setup_end(s)==PT_MIDI_SETUP_IN_USE);
  assert(pt_midi_recording_session_dispose_close(s)==PT_MIDI_SETUP_IN_USE&&!pt_midi_recording_session_model(s));
  assert(pt_midi_recording_session_record_status(s,&out)==PT_EDITOR_RECORD_CONFLICT&&!memcmp(&out,&before,sizeof(out)));
  assert(pt_midi_recording_session_stop(s,0)==PT_EDITOR_RECORD_CONFLICT);
  assert(pt_midi_recording_session_cancel(s)==PT_EDITOR_RECORD_CONFLICT);
  assert(pt_midi_recording_session_detach(s)==PT_EDITOR_RECORD_CONFLICT);
 }
 free(s);++completed;
}
static void preflight_and_null(void)
{
 struct rig *r=make();unsigned calls=r->ledger->calls;const struct pt_record_settings options={0,128,1,1,1,0};
 size_t n;uint8_t *gold=save_project(r,&n);size_t cap=pt_midi_recording_session_size()+pt_midi_frontend_context_size()+64;
 unsigned char *arena=calloc(1,cap);struct pt_midi_frontend_context *nested;assert(arena);
 nested=(void *)(arena+pt_midi_recording_session_size());
 assert((uintptr_t)nested%pt_midi_frontend_context_alignment()==0);
 assert(pt_midi_recording_session_construct((void *)arena,cap,nested,pt_midi_frontend_context_size(),r->owner,r->oc,r->take,r->tc,r->editor,
  (struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},1000)==PT_MIDI_SETUP_INVALID);
 assert(zero(arena,cap));free(arena);
 assert(pt_midi_recording_session_construct(r->session,pt_midi_recording_session_size()-1,r->frontend,r->fc,r->owner,r->oc,r->take,r->tc,r->editor,
  (struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},1000)==PT_MIDI_SETUP_INVALID);
 assert(r->ledger->calls==calls&&zero(r->session,pt_midi_recording_session_size()));
 r->ledger->fail=1;assert(construct(r)==PT_MIDI_SETUP_CAPACITY);r->ledger->fail=0;
 assert(zero(r->session,pt_midi_recording_session_size())&&zero(r->frontend,pt_midi_frontend_context_size())&&zero(r->owner,pt_midi_setup_owner_size())&&zero(r->take,pt_editor_midi_recording_size()));
 same_project(r,gold,n);free(gold);
 assert(construct(r)==PT_MIDI_SETUP_OK);
 assert(pt_midi_recording_session_record_begin(r->session,(void *)r->take,0,15,16)==PT_EDITOR_RECORD_ALIAS);
 assert(pt_midi_recording_session_record_begin(r->session,&options,0,15,0)==PT_EDITOR_RECORD_INVALID);
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_LIVE&&zero(r->take,pt_editor_midi_recording_size()));finish(r,1);
}
static void modal_failure_and_constructor_reentry(void)
{
 struct rig *r=make();struct pt_midi_requester_model *m;size_t n;uint8_t *gold;
 r->ledger->reentry=1;assert(construct(r)==PT_MIDI_SETUP_OK&&r->ledger->probes==1);r->ledger->reentry=0;
 gold=save_project(r,&n);assert(pt_midi_recording_session_setup_begin(r->session,15)==PT_MIDI_SETUP_OK);
 assert(begin(r,0,128,1)==PT_EDITOR_RECORD_CONFLICT&&zero(r->take,pt_editor_midi_recording_size()));
 assert(pt_midi_recording_session_dispose_close(r->session)==PT_MIDI_SETUP_IN_USE);
 m=pt_midi_recording_session_model(r->session);text(m,"Will fail");r->ledger->fail=1;
 assert(pt_midi_requester_model_apply(m)==PT_MIDI_SETUP_CAPACITY&&m->self);r->ledger->fail=0;
 assert(pt_midi_recording_session_setup_end(r->session)==PT_MIDI_SETUP_OK);same_project(r,gold,n);free(gold);
 assert(pt_midi_recording_session_setup_begin(r->session,15)==PT_MIDI_SETUP_OK);
 m=pt_midi_recording_session_model(r->session);text(m,"Canceled");
 assert(pt_midi_requester_model_cancel(m)==PT_MIDI_SETUP_OK&&pt_midi_recording_session_setup_end(r->session)==PT_MIDI_SETUP_OK);finish(r,1);
}
static void real_recording_reuse_and_save(void)
{
 struct rig *r=make();size_t start,n;uint8_t *gold;struct pt_editor_midi_record_status st;
 const uint8_t other[3]={0x90,60,100};struct pt_document decoded;struct pt_allocator a={r->ledger,allocate,release};
 assert(construct(r)==PT_MIDI_SETUP_OK);start=r->editor->history.cursor;assert(begin(r,63,128,1)==PT_EDITOR_RECORD_OK);
 take_refuses_setup_and_shutdown(r);assert(pt_midi_recording_session_message(r->session,UINT64_MAX,other,3)==PT_EDITOR_RECORD_IGNORED);
 r->document->project.channels.selected=0;assert(send(r,ROW(100),60,91)==PT_EDITOR_RECORD_OK);assert(send(r,ROW(100)+1,60,0)==PT_EDITOR_RECORD_OK);
 assert(r->document->project.events[63*16+15].kind==PT_NOTE_MIDI&&r->document->project.events[64*16+15].kind==PT_NOTE_OFF);
 assert(r->document->project.events[63*16+15].effect==14&&r->document->project.events[63*16+15].parameter==0xc3);
 assert(pt_midi_recording_session_stop(r->session,ROW(101))==PT_EDITOR_RECORD_OK);st=status(r);assert(!st.armed&&!st.active&&st.attached);take_refuses_setup_and_shutdown(r);
 assert(pt_midi_recording_session_detach(r->session)==PT_EDITOR_RECORD_OK&&zero(r->take,pt_editor_midi_recording_size()));tails(r);
 assert(begin(r,0,128,0)==PT_EDITOR_RECORD_OK&&send(r,ROW(2),70,88)==PT_EDITOR_RECORD_OK&&send(r,ROW(3),70,0)==PT_EDITOR_RECORD_OK);
 assert(pt_midi_recording_session_stop(r->session,ROW(4))==PT_EDITOR_RECORD_OK&&pt_midi_recording_session_detach(r->session)==PT_EDITOR_RECORD_OK);
 assert(zero(r->take,pt_editor_midi_recording_size())&&r->editor->history.cursor==start+4);tails(r);master(r);
 gold=save_project(r,&n);pt_document_init(&decoded,&a);assert(pt_document_load(&decoded,gold,n,BUDGET)==PT_PROJECT_OK);
 assert(decoded.project.samples[0].pcm.bits==24&&!memcmp(decoded.project.samples[0].pcm.data,r->pcm,sizeof(r->pcm)));assert(pt_document_release_checked(&decoded)==PT_DOCUMENT_RELEASED);
 pt_editor_saved(r->editor);assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
 assert(pt_pattern_undo(&r->document->project,&r->editor->history,1)==PT_EDIT_OK&&!pt_editor_dirty(r->editor));same_project(r,gold,n);free(gold);
 apply(r,"After takes");assert(!strcmp(r->document->project.midi_input,"After takes"));finish(r,1);
}
static void failed_stop_cancel_protection(void)
{
 struct rig *r=make();assert(construct(r)==PT_MIDI_SETUP_OK&&begin(r,127,128,0)==PT_EDITOR_RECORD_OK);
 assert(send(r,0,127,100)==PT_EDITOR_RECORD_OK&&pt_midi_recording_session_stop(r->session,ROW(1))==PT_EDITOR_RECORD_CAPACITY);
 assert(status(r).armed&&status(r).active);take_refuses_setup_and_shutdown(r);
 assert(pt_midi_recording_session_detach(r->session)==PT_EDITOR_RECORD_CONFLICT);assert(!zero(r->take,pt_editor_midi_recording_size()));
 assert(pt_midi_recording_session_cancel(r->session)==PT_EDITOR_RECORD_OK&&!status(r).armed);take_refuses_setup_and_shutdown(r);
 assert(pt_midi_recording_session_detach(r->session)==PT_EDITOR_RECORD_OK&&zero(r->take,pt_editor_midi_recording_size()));
 assert(r->document->project.events[127*16+15].kind==PT_NOTE_MIDI);finish(r,1);
}
static void real_redo_discard_reentry_and_aliases(void)
{
 struct rig *r=make();unsigned before;struct pt_editor_midi_record_status st;size_t n;uint8_t *copy;
 assert(construct(r)==PT_MIDI_SETUP_OK);apply(r,"Retained redo");
 assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
 assert(begin(r,0,128,0)==PT_EDITOR_RECORD_OK);before=r->ledger->record_frees;r->ledger->reentry=1;
 assert(send(r,ROW(5),60,99)==PT_EDITOR_RECORD_OK&&r->ledger->record_frees==before+1&&r->ledger->probes==1);r->ledger->reentry=0;
 n=sizeof(st);copy=malloc(n);assert(copy&&pt_midi_setup_size()>=n);memcpy(copy,r->ledger->workspace,n);
 assert(pt_midi_recording_session_record_status(r->session,r->ledger->workspace)==PT_EDITOR_RECORD_ALIAS&&!memcmp(copy,r->ledger->workspace,n));
 memcpy(copy,r->session,n);assert(pt_midi_recording_session_record_status(r->session,(void *)r->session)==PT_EDITOR_RECORD_ALIAS&&!memcmp(copy,r->session,n));free(copy);
 assert(pt_midi_recording_session_record_status(r->session,(void *)r->take)==PT_EDITOR_RECORD_ALIAS);
 assert(pt_midi_recording_session_stop(r->session,ROW(6))==PT_EDITOR_RECORD_OK&&pt_midi_recording_session_detach(r->session)==PT_EDITOR_RECORD_OK);finish(r,1);
}
static void final_journal_disposal_reentry(void)
{
 struct rig *r=make();assert(construct(r)==PT_MIDI_SETUP_OK);apply(r,"Committed");
 assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
 r->ledger->reentry=1;finish(r,1);
}
static void retained_hook_drift(void)
{
 struct rig *r=make();unsigned char *take;assert(construct(r)==PT_MIDI_SETUP_OK&&begin(r,0,128,1)==PT_EDITOR_RECORD_OK);
 r->editor->workflow.recording_context=r->ledger;take=malloc(r->tc);assert(take);memcpy(take,r->take,r->tc);
 assert(send(r,0,60,100)==PT_EDITOR_RECORD_HELD&&pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_HELD);
 assert(memcmp(take,r->take,r->tc)!=0); /* Child genuinely latches its HELD state. */
 free(take);assert(pt_midi_recording_session_dispose_close(r->session)==PT_MIDI_SETUP_HELD);
 assert(!zero(r->take,pt_editor_midi_recording_size()));master(r);tails(r);retained[0]=r;
}
static void retained_allocator_alias(void)
{
 struct rig *r=make();r->ledger->alias=1;assert(construct(r)==PT_MIDI_SETUP_HELD);
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_HELD&&r->ledger->workspace_frees==0);
 assert(pt_midi_recording_session_setup_begin(r->session,15)==PT_MIDI_SETUP_HELD);
 master(r);tails(r);retained[1]=r;
}
static void retained_allocator_before_first_take(void)
{
 struct rig *r=make();unsigned calls;struct pt_record_settings options={0,128,1,1,1,0};
 assert(construct(r)==PT_MIDI_SETUP_OK);calls=r->ledger->calls;
 r->foreign=calloc(1,sizeof(*r->foreign));assert(r->foreign);
 /* Deliberate forbidden allocator drift; original resources/context stay alive. */
 r->editor->sampler.allocator.context=r->foreign;
 assert(pt_midi_recording_session_record_begin(r->session,&options,0,15,16)==PT_EDITOR_RECORD_HELD);
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_HELD&&zero(r->take,pt_editor_midi_recording_size()));
 assert(!r->editor->change_ready&&!r->editor->workflow.recording_busy&&!r->editor->workflow.recording_context);
 assert(r->ledger->calls==calls&&!r->foreign->calls);master(r);tails(r);retained[2]=r;
}
static void retained_outside_modal(void)
{
 struct rig *r=make();unsigned calls;assert(construct(r)==PT_MIDI_SETUP_OK);calls=r->ledger->calls;
 r->foreign_ticket=calloc(1,sizeof(*r->foreign_ticket));assert(r->foreign_ticket);
 /* A genuine borrowed ticket is retained in heap storage, never a returned stack. */
 assert(pt_midi_setup_owner_borrow(r->owner,r->foreign_ticket)==PT_MIDI_SETUP_OK);
 assert(begin(r,0,128,1)==PT_EDITOR_RECORD_HELD&&zero(r->take,pt_editor_midi_recording_size()));
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_HELD&&r->ledger->calls==calls);
 master(r);tails(r);retained[3]=r;
}
static void retained_outside_draft(void)
{
 struct rig *r=make();struct pt_midi_requester_model *model;unsigned calls;
 assert(construct(r)==PT_MIDI_SETUP_OK&&pt_midi_recording_session_setup_begin(r->session,15)==PT_MIDI_SETUP_OK);
 model=pt_midi_recording_session_model(r->session);assert(model);r->foreign_setup=model->setup;
 assert(pt_midi_recording_session_setup_end(r->session)==PT_MIDI_SETUP_OK);calls=r->ledger->calls;
 /* Deliberate raw-controller bypass of this session's exclusive aliases. */
 assert(pt_midi_setup_begin(r->foreign_setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
 assert(pt_midi_setup_name(r->foreign_setup,"Outside draft",14)==PT_MIDI_SETUP_OK);
 assert(begin(r,0,128,1)==PT_EDITOR_RECORD_HELD&&zero(r->take,pt_editor_midi_recording_size()));
 assert(pt_midi_recording_session_state(r->session)==PT_MIDI_SESSION_HELD&&r->ledger->calls==calls);
 master(r);tails(r);retained[4]=r;
}
int main(void)
{
 preflight_and_null();modal_failure_and_constructor_reentry();real_recording_reuse_and_save();failed_stop_cancel_protection();
 real_redo_discard_reentry_and_aliases();final_journal_disposal_reentry();retained_hook_drift();retained_allocator_alias();
 retained_allocator_before_first_take();retained_outside_modal();retained_outside_draft();
 assert(completed==6&&retained[0]&&retained[1]&&retained[2]&&retained[3]&&retained[4]);
 puts("MIDI SESSION HOST PASS:11 groups;6 complete;5 retained;real modal/takes/reuse/shutdown;master/save/undo;reentry;NO INPUT/CAMD/TIMING/OUTPUT");return 0;
}
