/* Ordinary Task RAM-only document admission/cancel qualification candidate.
 * No publication step, playback preparation/start, timer, window or real device.
 * RAM reservation callbacks own only their original in-process token objects. */
#if !defined(PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY) || PT_DOCUMENT_MIDI_SETUP_NATIVE_TASK_PROBE_ONLY != 1
#error "Select the exact native MIDI document TASK RAM probe"
#endif
#include "midi_requester.h"
#include "native_checked_memory.h"
#include "../editor/editor_document_action.h"
#include "../editor/sampler_internal.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define PASS_MARKER "MIDI DOCUMENT TASK RAM PASS:modal veto;New/Load prepare-cancel;original records/redo/master/save;genuine RAM tokens;zero Fast ledgers;NO PUBLICATION/WINDOW/DEVICE/OUTPUT AUTHORITY"
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
enum {TRACKS=4,FRAMES=2,TAIL=16,FIXED_BLOCKS=6,DOCUMENT_BLOCKS=3,
      MASTER_BLOCKS=1,WORKSPACE_BLOCKS=1,RECORD_BLOCKS=2,OWNER_BLOCKS=1,
      CANDIDATE_BLOCKS=4,MAX_LIVE=FIXED_BLOCKS+DOCUMENT_BLOCKS+MASTER_BLOCKS+
      WORKSPACE_BLOCKS+RECORD_BLOCKS+OWNER_BLOCKS+CANDIDATE_BLOCKS};
typedef char classic_geometry[(CHAR_BIT==8&&sizeof(void *)==4&&sizeof(uintptr_t)==4&&
    sizeof(size_t)==4&&sizeof(uint32_t)==4&&sizeof(uint64_t)==8)?1:-1];
typedef char unchanged_registry_suffices[(MAX_LIVE<=PT_PRIVATE_MEMORY_ALLOCATIONS)?1:-1];

struct ram_token {unsigned identity;};
struct ram_model {
    struct ram_model *self;struct pt_amigus_reservation *reservation;
    struct ram_token token;void *original_owner;
    unsigned opened,opens,reserves,releases,closes;
};
struct ram_bus {struct pt_amigus_reservation reservation;struct pt_amigus_wavetable_cache cache;struct ram_model model;};
/* This public-control byte image is observation only: never passed as a live
 * owner, restored, or used to authorize release. The full rig snapshot is likewise
 * observation only; opaque MIDI owner and modal model are separate, not copied. */
struct original_domain {
    struct pt_document document;struct pt_editor editor;
    struct pt_editor_mixed binding;struct pt_mixed_established checked;
    struct pt_sampler_paula paula_cache;struct pt_sampler_wavetable wave_cache;
    struct pt_paula_voices paula;struct pt_wavetable_voices wave;
    struct ram_bus bus;
};
struct rig {
    struct original_domain original;struct ram_bus future_bus;
    /* Original retained parent objects live through genuine checked-owner close. */
    struct pt_render_options options;struct pt_paula_render_caps caps;
    struct pt_playback_format format;
    struct pt_document candidate;
    struct pt_editor_document_action action;
    struct pt_editor_document_action_inputs inputs;
    struct pt_mixed_established future;
    struct pt_editor_mixed_establish establish;
    struct pt_editor_mixed_bridges bridges;
};
struct task_state {
    struct ExecBase *exec;struct Task *task;int idnest,tdnest;
    struct rig *rig;unsigned char *before;
    struct pt_midi_setup_owner *owner;size_t owner_capacity;
    struct pt_midi_requester_modal *modal;
    struct pt_sample_version *pin;struct pt_pcm pcm;
    struct pt_allocator allocator;size_t request_cap,byte_ceiling,master_cap,document_cap;
    unsigned char *encoded,*scratch;size_t encoded_bytes;
    unsigned phase,device_calls;
    /* Planning input has no ownership. Actual factories/size/probe must confirm
     * this exact finite geometry before any tested transaction. */
    struct pt_project planned;uint16_t order;
    struct pt_event events[PT_PROJECT_ROWS*TRACKS];struct pt_sample sample;
    int32_t values[FRAMES];
};
static struct task_state state;
static struct pt_private_native_memory memory;
static int held(void) __attribute__((noreturn));
static int held(void)
{
    /* Never unwind, retry/reset or free a partial/uncertain domain. */
    for(;;)__asm__ volatile ("" ::: "memory");
    __builtin_unreachable();
}
#define REQUIRE(c) do {if(!(c))held();} while(0)
static int normal(void)
{return state.exec&&SysBase==state.exec&&state.exec->ThisTask==state.task&&
    (int)(signed char)state.exec->IDNestCnt==state.idnest&&
    (int)(signed char)state.exec->TDNestCnt==state.tdnest;}
static int fast(const void *p,size_t n)
{uintptr_t a=(uintptr_t)p;return p&&n&&n<=UINTPTR_MAX-a&&
    (TypeOfMem((APTR)p)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST&&
    (TypeOfMem((APTR)(a+n-1))&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST;}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static size_t add(size_t a,size_t b){REQUIRE(b<=SIZE_MAX-a);return a+b;}
static size_t mul(size_t a,size_t b){REQUIRE(!a||b<=SIZE_MAX/a);return a*b;}
static int same_allocator(struct pt_allocator a,struct pt_allocator b)
{return a.context==b.context&&a.allocate==b.allocate&&a.release==b.release;}
static void *allocate(void *context,size_t bytes)
{
    void *p;REQUIRE(context==&memory&&normal()&&!memory.failed&&bytes&&bytes<=state.request_cap);
    p=pt_private_native_allocate(context,bytes);
    REQUIRE(normal()&&!memory.failed&&memory.live<=MAX_LIVE);
    if(p)REQUIRE(fast(p,bytes));return p;
}
static void release(void *context,void *p)
{REQUIRE(context==&memory&&normal()&&!memory.failed);
 REQUIRE(pt_private_native_release_checked(&memory,p)==PT_PRIVATE_MEMORY_OK&&normal()&&!memory.failed);}
static void *fixed(size_t bytes,size_t alignment)
{
    void *p;unsigned calls=memory.allocations;REQUIRE(!state.request_cap&&alignment);
    state.request_cap=bytes;p=allocate(&memory,bytes);state.request_cap=0;
    REQUIRE(p&&(uintptr_t)p%alignment==0&&memory.allocations==calls+1);memset(p,0,bytes);return p;
}
static struct pt_private_memory_stats stats(void)
{struct pt_private_memory_stats s;REQUIRE(normal()&&pt_private_native_memory_stats(&memory,&s)==PT_PRIVATE_MEMORY_OK&&!s.failed);return s;}
static void owner_tail(void)
{const unsigned char *p=(const unsigned char *)state.owner+pt_midi_setup_owner_size();unsigned i;for(i=0;i<TAIL;++i)REQUIRE(p[i]==0x7b);}
static struct pt_midi_setup_owner_status owner_status(void)
{struct pt_midi_setup_owner_status s;REQUIRE(pt_midi_setup_owner_get(state.owner,&s)==PT_MIDI_SETUP_OK&&normal());return s;}

/* Real synchronous ownership of THESE original RAM model tokens only. None of
 * these callbacks discovers/opens/reserves a physical library/card/device. */
static struct ram_model *model(void *context)
{struct ram_model *m=context;REQUIRE(m&&m->self==m&&normal()&&m->reservation);return m;}
static int ram_open(void *context)
{struct ram_model *m=model(context);REQUIRE(!m->opened&&!m->original_owner&&!m->opens);m->opened=1;++m->opens;return 1;}
static void ram_close(void *context)
{struct ram_model *m=model(context);REQUIRE(m->opened&&!m->original_owner&&m->reserves==m->releases);m->opened=0;++m->closes;}
static void *ram_find(void *context,void *previous)
{struct ram_model *m=model(context);REQUIRE(m->opened&&(!previous||previous==&m->token));return previous?NULL:&m->token;}
static int ram_supported(void *context,void *card,enum pt_amigus_resource resource)
{struct ram_model *m=model(context);REQUIRE(m->opened&&card==&m->token);return resource==PT_AMIGUS_WAVETABLE;}
static unsigned long ram_reserve(void *context,void *card,enum pt_amigus_resource resource,void *owner)
{struct ram_model *m=model(context);REQUIRE(m->opened&&card==&m->token&&resource==PT_AMIGUS_WAVETABLE&&owner==m->reservation&&!m->original_owner&&!m->reserves);m->original_owner=owner;++m->reserves;return 0;}
static void ram_release(void *context,void *card,enum pt_amigus_resource resource,void *owner)
{struct ram_model *m=model(context);REQUIRE(m->opened&&card==&m->token&&resource==PT_AMIGUS_WAVETABLE&&owner==m->reservation&&owner==m->original_owner&&!m->releases);m->original_owner=NULL;++m->releases;}
static int ram_owned(void *context)
{struct ram_model *m=model(context);return m->opened&&m->original_owner==m->reservation&&
    m->reservation->opened&&m->reservation->reserved&&m->reservation->card==&m->token&&
    m->reservation->resource==PT_AMIGUS_WAVETABLE&&!m->reservation->interrupt;}
static int reject_write(void *context,unsigned reg,uint32_t word)
{(void)context;(void)reg;(void)word;++state.device_calls;return held();}
static void *reject_chip(void *context,size_t bytes)
{(void)context;(void)bytes;++state.device_calls;held();}
static void reject_chip_free(void *context,void *p,size_t bytes)
{(void)context;(void)p;(void)bytes;++state.device_calls;held();}
static int reject_paula_start(void *context,unsigned voice,const struct pt_paula_voice_plan *p)
{(void)context;(void)voice;(void)p;++state.device_calls;return held();}
static int reject_wave_start(void *context,unsigned voice,const struct pt_amigus_voice_plan *p)
{(void)context;(void)voice;(void)p;++state.device_calls;return held();}
static int reject_stop(void *context,unsigned voice)
{(void)context;(void)voice;++state.device_calls;return held();}
static int reject_quiesce(void *context)
{(void)context;++state.device_calls;return held();}
static void bus_open(struct ram_bus *b,unsigned identity)
{
    struct pt_amigus_reservation_api api;
    b->model.self=&b->model;b->model.reservation=&b->reservation;b->model.token.identity=identity;
    api=(struct pt_amigus_reservation_api){&b->model,ram_open,ram_close,ram_find,ram_supported,ram_reserve,ram_release};
    REQUIRE(pt_amigus_reservation_open_resource(&b->reservation,&api,0,PT_AMIGUS_WAVETABLE)==PT_AMIGUS_RESERVED&&normal());
    REQUIRE(pt_amigus_wavetable_cache_attach(&b->cache,&b->reservation,0,4,4,&b->model,ram_owned,reject_write)&&normal());
}
static void bus_close(struct ram_bus *b)
{
    REQUIRE(!b->cache.cache.bytes&&pt_amigus_wavetable_cache_detach(&b->cache)&&normal());
    REQUIRE(pt_amigus_reservation_close(&b->reservation)&&normal());
    REQUIRE(!b->model.opened&&!b->model.original_owner&&b->model.opens==1&&b->model.reserves==1&&b->model.releases==1&&b->model.closes==1);
}
static void plan(void)
{
    size_t bytes,fixed_bytes,base,peak,command=pt_midi_setup_command_bytes();
    struct pt_project *p=&state.planned;
    state.values[0]=-1200;state.values[1]=1200;
    pt_channels_init(&p->channels);REQUIRE(pt_channels_resize(&p->channels,TRACKS)==PT_CHANNEL_OK);
    state.sample.pcm=(struct pt_pcm){state.values,FRAMES,FRAMES,8000,1,16};state.sample.volume=64;
    p->orders=&state.order;p->events=state.events;p->samples=&state.sample;
    p->order_count=p->pattern_count=p->sample_count=1;p->bpm=125;p->speed=6;
    strcpy(p->midi_input,"source-one");
    REQUIRE(pt_project_size(p,&state.encoded_bytes)==PT_PROJECT_OK&&state.encoded_bytes);
    REQUIRE(pt_midi_setup_owner_size()<=SIZE_MAX-TAIL&&command);
    state.owner_capacity=pt_midi_setup_owner_size()+TAIL;
    bytes=add(sizeof(uint16_t),mul(PT_PROJECT_ROWS*TRACKS,sizeof(struct pt_event)));
    state.document_cap=add(bytes,mul(31,sizeof(struct pt_sample)));
    /* Public sizeof-based conservative master request cap, enforced on the
     * actual one-version allocation. No duplicate private version layout. */
    state.master_cap=add(add(sizeof(struct pt_sampler),sizeof(struct pt_sample)),mul(FRAMES,sizeof(int32_t)));
    fixed_bytes=add(mul(2,sizeof(struct rig)),add(state.owner_capacity,sizeof(struct pt_midi_requester_modal)));
    fixed_bytes=add(fixed_bytes,mul(2,state.encoded_bytes));
    base=add(fixed_bytes,state.document_cap);base=add(base,state.master_cap);
    base=add(base,pt_midi_setup_size());base=add(base,mul(RECORD_BLOCKS,command));base=add(base,sizeof(struct pt_mixed_owner));
    /* At most four candidate allocations: order,event,sample,PCM. NEW's31
     * sample table is a conservative bound for encoded LOAD's one sample. */
    peak=add(state.document_cap,mul(FRAMES,sizeof(int32_t)));state.byte_ceiling=add(base,peak);
}
static void policies(struct pt_editor *e)
{
    REQUIRE(pt_editor_init(e,&state.rig->original.document.project)&&normal());
    REQUIRE(!e->sampler.bytes&&!e->sample_source.loaded&&!e->sample_source.allocated_bytes&&!e->song.current&&!e->song.bytes);
    pt_sampler_init(&e->sampler,&state.allocator,state.master_cap);
    pt_document_init(&e->sample_source,&state.allocator);pt_song_init(&e->song,&state.allocator,state.document_cap);
    REQUIRE(same_allocator(e->sampler.allocator,state.allocator)&&same_allocator(e->sample_source.allocator,state.allocator)&&
        same_allocator(e->song.allocator,state.allocator)&&e->sampler.budget==state.master_cap&&e->song.budget==state.document_cap&&
        !e->sampler.bytes&&!e->sample_source.loaded&&!e->sample_source.allocated_bytes&&!e->song.current&&!e->song.bytes&&normal());
}
static void put_name(const char *name)
{
    struct pt_midi_setup_owner_borrow ticket;unsigned calls=memory.allocations;
    REQUIRE(pt_midi_setup_owner_borrow(state.owner,&ticket)==PT_MIDI_SETUP_OK);
    REQUIRE(pt_midi_setup_begin(ticket.setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
    REQUIRE(pt_midi_setup_name(ticket.setup,name,strlen(name)+1)==PT_MIDI_SETUP_OK);
    state.request_cap=pt_midi_setup_command_bytes();REQUIRE(pt_midi_setup_commit(ticket.setup)==PT_MIDI_SETUP_OK&&normal());state.request_cap=0;
    REQUIRE(memory.allocations==calls+1&&pt_midi_setup_owner_return(state.owner,&ticket)==PT_MIDI_SETUP_OK);owner_tail();
}
static void save_equal(void)
{
    struct original_domain *o=&state.rig->original;size_t bytes,written;
    REQUIRE(pt_project_size(&o->document.project,&bytes)==PT_PROJECT_OK&&bytes==state.encoded_bytes);
    REQUIRE(pt_project_encode(&o->document.project,state.scratch,bytes,&written)==PT_PROJECT_OK&&written==bytes&&!memcmp(state.scratch,state.encoded,bytes));
    REQUIRE(normal()&&!state.device_calls&&owner_status().commands==RECORD_BLOCKS);
    REQUIRE(state.pcm.frames==FRAMES&&state.pcm.bits==16&&state.pcm.data[0]==-1200&&state.pcm.data[1]==1200);
}
static void source_equal(void)
{REQUIRE(!memcmp(state.before,&state.rig->original,sizeof(struct original_domain)));save_equal();owner_tail();}
static void construct(void)
{
    struct rig *r=state.rig;struct original_domain *o=&r->original;
    struct pt_paula_voice_api pa;struct pt_wavetable_voice_api wa;unsigned calls;
    r->options=(struct pt_render_options){.frame_limit=100000,.tick_limit=100,.rate=48000,.gain_q16=65536,.bits=24};
    r->caps=(struct pt_paula_render_caps){3546895,124,65535};r->format=(struct pt_playback_format){8,0,0,0};
    pt_document_init(&o->document,&state.allocator);pt_document_init(&r->candidate,&state.allocator);
    state.request_cap=state.document_cap;REQUIRE(pt_document_new(&o->document,TRACKS,state.document_cap)==PT_PROJECT_OK);state.request_cap=0;
    o->document.project.sample_count=1;o->document.project.samples[0]=state.sample;
    policies(&o->editor);calls=memory.allocations;
    state.request_cap=state.master_cap;
    REQUIRE(pt_sampler_pin(&o->editor.sampler,&o->document.project,0,o->editor.sampler.generation,&state.pcm,&state.pin)==PT_EDIT_OK&&normal());state.request_cap=0;
    REQUIRE(state.pin&&memory.allocations==calls+1&&o->editor.sampler.bytes<=state.master_cap);
    REQUIRE(pt_editor_mixed_attach(&o->binding,&o->editor));bus_open(&o->bus,1);bus_open(&r->future_bus,2);
    REQUIRE(pt_sampler_paula_bind(&o->paula_cache,&o->editor.sampler,&o->document.project,&state,reject_chip,reject_chip_free,1));
    REQUIRE(pt_sampler_wavetable_bind(&o->wave_cache,&o->editor.sampler,&o->document.project,&o->bus.cache));
    pa=(struct pt_paula_voice_api){&state,reject_paula_start,reject_stop,NULL};
    wa=(struct pt_wavetable_voice_api){&state,reject_wave_start,reject_stop,NULL,NULL};
    REQUIRE(pt_paula_voices_bind(&o->paula,&o->paula_cache,&pa)&&pt_wavetable_voices_bind(&o->wave,&o->wave_cache,&wa));
    REQUIRE(!o->paula.quiesce&&!o->wave.quiesce);
    calls=memory.allocations;state.request_cap=pt_midi_setup_size();
    REQUIRE(pt_midi_setup_owner_init(state.owner,state.owner_capacity,&o->editor,
        (struct pt_midi_setup_owner_span){&memory,sizeof(memory)},mul(RECORD_BLOCKS,pt_midi_setup_command_bytes()))==PT_MIDI_SETUP_OK&&normal());state.request_cap=0;
    REQUIRE(memory.allocations==calls+1);put_name("source-one");put_name("source-two");
    REQUIRE(pt_pattern_undo(&o->document.project,&o->editor.history,-1)==PT_EDIT_OK);
    REQUIRE(o->editor.history.count==2&&o->editor.history.cursor==1&&!strcmp(o->document.project.midi_input,"source-one"));
    REQUIRE(owner_status().commands==2);
    calls=memory.allocations;state.request_cap=sizeof(struct pt_mixed_owner);
    REQUIRE(pt_editor_mixed_checked_begin(&o->binding,&o->checked,&o->paula,&o->wave,&r->options,&r->caps,&r->format,
        (struct pt_sampler_storage_span){&state,sizeof(state)},64)==PT_MIXED_OWNER_PREPARING&&normal());state.request_cap=0;
    REQUIRE(memory.allocations==calls+1&&pt_editor_mixed_checked_binding_current(&o->binding,&o->checked,&o->paula,&o->wave));
    r->inputs=(struct pt_editor_document_action_inputs){
        .document=&o->document,.candidate=&r->candidate,.binding=&o->binding,.old_control=&o->checked,.new_control=&r->future,
        .old_paula=&o->paula,.old_amigus=&o->wave,.establish=&r->establish,.bridges=&r->bridges,
        .bridge_inputs={&r->future_bus.cache,&state,reject_chip,reject_chip_free,1,pa,wa,reject_quiesce,&state,reject_quiesce,&state},
        .options=r->options,.caps=r->caps,.format=r->format,.allocator=state.allocator,
        .contexts={{&memory,sizeof(memory)},{&state,sizeof(state)}},.context_count=2,.work=64,.midi_setup=state.owner,.midi_capacity=state.owner_capacity};
}
int main(void)
{
    struct pt_private_memory_span guard;struct pt_private_memory_stats before,after;
    struct pt_project_requirements need;struct rig *r;struct original_domain *o;
    unsigned calls,releases;size_t written;unsigned i;
    state.exec=SysBase;REQUIRE(state.exec&&(state.task=FindTask(NULL))&&state.exec->ThisTask==state.task);
    state.idnest=(int)(signed char)state.exec->IDNestCnt;state.tdnest=(int)(signed char)state.exec->TDNestCnt;
    REQUIRE(state.idnest==-1&&state.tdnest==-1&&normal()&&fast(&state,sizeof(state))&&fast(&memory,sizeof(memory)));
    plan();guard=(struct pt_private_memory_span){&state,sizeof(state)};
    REQUIRE(pt_private_native_memory_init(&memory,state.byte_ceiling,&guard,1)==PT_PRIVATE_MEMORY_OK&&normal());
    state.allocator=(struct pt_allocator){&memory,allocate,release};
    state.rig=r=fixed(sizeof(*r),ALIGN_OF(struct rig));o=&r->original;
    state.before=fixed(sizeof(*r),1);state.owner=fixed(state.owner_capacity,pt_midi_setup_owner_alignment());
    state.modal=fixed(sizeof(*state.modal),ALIGN_OF(struct pt_midi_requester_modal));
    state.encoded=fixed(state.encoded_bytes,1);state.scratch=fixed(state.encoded_bytes,1);
    memset((unsigned char *)state.owner+pt_midi_setup_owner_size(),0x7b,TAIL);
    construct();REQUIRE(memory.live==MAX_LIVE-CANDIDATE_BLOCKS);
    REQUIRE(pt_project_encode(&o->document.project,state.encoded,state.encoded_bytes,&written)==PT_PROJECT_OK&&written==state.encoded_bytes);
    REQUIRE(pt_project_probe(state.encoded,state.encoded_bytes,&need)==PT_PROJECT_OK&&need.orders==1&&need.events==PT_PROJECT_ROWS*TRACKS&&
        need.samples==1&&need.pcm_values==FRAMES&&!need.slices&&!need.extensions&&!need.extension_bytes);
    memcpy(state.before,r,sizeof(*r));save_equal();state.phase=1;
    REQUIRE(pt_midi_requester_modal_begin(state.modal,state.owner,state.owner_capacity,&o->editor,0)==PT_MIDI_SETUP_OK);
    REQUIRE(pt_midi_requester_model_select(&state.modal->model,PT_MIDI_SETUP_INPUT)==PT_MIDI_SETUP_OK);
    REQUIRE(pt_midi_requester_model_key(&state.modal->model,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED&&
        pt_midi_requester_model_key(&state.modal->model,PT_MIDI_KEY_CHARACTER,'X')==PT_MIDI_EVENT_CHANGED);
    calls=memory.allocations;releases=memory.releases;
    REQUIRE(pt_editor_mixed_checked_binding_current(&o->binding,&o->checked,&o->paula,&o->wave));
    REQUIRE(pt_editor_document_action_new(&r->action,&r->inputs,TRACKS,state.document_cap)==PT_DOCUMENT_ACTION_INVALID);
    REQUIRE(!memcmp(state.before,r,sizeof(*r)));
    REQUIRE(zero(&r->action,sizeof(r->action))&&!r->candidate.loaded&&memory.allocations==calls&&memory.releases==releases);source_equal();
    REQUIRE(pt_midi_requester_modal_return(state.modal)==PT_MIDI_SETUP_OK&&zero(state.modal,sizeof(*state.modal)));source_equal();
    state.phase=2;before=stats();state.request_cap=state.document_cap;
    REQUIRE(pt_editor_document_action_new(&r->action,&r->inputs,TRACKS,state.document_cap)==PT_DOCUMENT_ACTION_PREPARED);state.request_cap=0;
    REQUIRE(r->action.replacement.midi_setup==state.owner&&r->candidate.loaded&&memory.live==before.live+3);
    REQUIRE(pt_editor_document_action_cancel(&r->action)==PT_DOCUMENT_ACTION_CANCELED&&!r->action.self);
    after=stats();REQUIRE(after.live==before.live&&after.used==before.used&&after.allocations==before.allocations+3&&after.releases==before.releases+3);source_equal();
    REQUIRE(pt_editor_document_action_reset(&r->action)&&zero(&r->action,sizeof(r->action)));
    state.phase=3;before=stats();state.request_cap=add(state.document_cap,mul(FRAMES,sizeof(int32_t)));
    REQUIRE(pt_editor_document_action_load(&r->action,&r->inputs,state.encoded,state.encoded_bytes,state.request_cap)==PT_DOCUMENT_ACTION_PREPARED);state.request_cap=0;
    REQUIRE(r->action.replacement.midi_setup==state.owner&&r->candidate.loaded&&memory.live==before.live+CANDIDATE_BLOCKS);
    REQUIRE(pt_editor_document_action_cancel(&r->action)==PT_DOCUMENT_ACTION_CANCELED&&!r->action.self&&pt_editor_document_action_finish(&r->action));
    after=stats();REQUIRE(after.live==before.live&&after.used==before.used&&after.allocations==before.allocations+CANDIDATE_BLOCKS&&after.releases==before.releases+CANDIDATE_BLOCKS);source_equal();
    /* End only the unstarted genuine checked software owner. No transport,
     * per-voice device call or quiesce callback has ever been installed/issued. */
    REQUIRE(pt_editor_mixed_stop(&o->binding)&&normal()&&!state.device_calls);
    REQUIRE(pt_pattern_undo(&o->document.project,&o->editor.history,1)==PT_EDIT_OK&&!strcmp(o->document.project.midi_input,"source-two"));
    REQUIRE(pt_pattern_undo(&o->document.project,&o->editor.history,-1)==PT_EDIT_OK);save_equal();
    REQUIRE(pt_paula_voices_close(&o->paula)&&pt_wavetable_voices_close(&o->wave)&&!state.device_calls);
    bus_close(&o->bus);bus_close(&r->future_bus);REQUIRE(pt_editor_mixed_detach(&o->binding));
    pt_sampler_unpin(state.pin);state.pin=NULL;state.pcm=(struct pt_pcm){0};
    REQUIRE(pt_private_native_memory_seal(&memory)==PT_PRIVATE_MEMORY_OK);
    REQUIRE(pt_editor_dispose(&o->editor)&&normal());
    REQUIRE(!o->editor.history.count&&!o->editor.history.cursor&&!o->editor.sampler.bytes&&!o->editor.sampler.live_count&&!o->editor.sampler.pending_count);
    REQUIRE(pt_midi_setup_owner_close_controller(state.owner)==PT_MIDI_SETUP_OK&&owner_status().state==PT_MIDI_OWNER_CLOSED);
    REQUIRE(!owner_status().commands&&!owner_status().metadata_bytes&&!owner_status().modal);
    REQUIRE(pt_midi_setup_owner_release_workspace(state.owner)==PT_MIDI_SETUP_OK);owner_tail();
    REQUIRE(pt_document_release_checked(&r->candidate)==PT_DOCUMENT_RELEASED&&pt_document_release_checked(&o->document)==PT_DOCUMENT_RELEASED&&normal());
    release(&memory,state.scratch);state.scratch=NULL;release(&memory,state.encoded);state.encoded=NULL;
    release(&memory,state.modal);state.modal=NULL;release(&memory,state.owner);state.owner=NULL;
    release(&memory,state.before);state.before=NULL;release(&memory,r);state.rig=NULL;
    after=stats();REQUIRE(!after.live&&!after.used&&!after.failed&&after.allocations==after.releases&&after.peak_live==MAX_LIVE&&!state.device_calls);
    REQUIRE(pt_private_native_memory_finish(&memory)==PT_PRIVATE_MEMORY_OK&&normal());
    for(i=0;i<FRAMES;++i)REQUIRE(state.values[i]==(i?1200:-1200));
    state.phase=4;puts(PASS_MARKER);return 0;
}
