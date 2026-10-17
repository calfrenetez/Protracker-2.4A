/* Explicit ordinary-Task RAM experiment; not application enrollment or MIDI I/O.
 * Exact original owners stay stationary. Any unexpected result retains the
 * complete process/Task/context; the probe has no general recovery authority. */
#if !defined(PT_MIDI_FRONTEND_RECORDING_SESSION_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_FRONTEND_RECORDING_SESSION_NATIVE_TASK_PROBE_ONLY != 1
#error "Select the exact MIDI frontend recording session Task RAM probe"
#endif
#if !defined(__m68k__) || !defined(__amigaos__)
#error "This experiment requires the genuine classic Amiga m68k target"
#endif
#include "midi_frontend_recording_session.h"
#include "../editor/sampler_lifetime_internal.h"
#include "../core/mod_project.h"
#include "native_checked_memory.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define PASS_MARKER "MIDI SESSION TASK RAM PASS:real setup/take/master/save/undo;attached guards;detach reuse;dispose/close;zero Fast ledgers;NO WINDOW/INPUT/CAMD/TIMING/OUTPUT"
#define ROW(n) ((uint64_t)(n) << 16)
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
#define SAMPLER_ALLOWANCE (1024UL * 1024UL)
enum { TRACKS=4, TARGET=3, SAMPLES=31, TAIL=16,
    DOCUMENT_BLOCKS=3, SAMPLER_BLOCKS=3, EDITOR_BLOCKS=1, CONTROL_BLOCKS=4,
    WORKSPACE_BLOCKS=1, RECORD_BLOCKS=1, SAVE_BLOCKS=3,
    MAX_LIVE=DOCUMENT_BLOCKS+SAMPLER_BLOCKS+EDITOR_BLOCKS+CONTROL_BLOCKS+WORKSPACE_BLOCKS+RECORD_BLOCKS+SAVE_BLOCKS };
typedef char genuine_classic_geometry[(CHAR_BIT==8 && sizeof(void *)==4 &&
    sizeof(size_t)==4 && sizeof(uintptr_t)==4 && sizeof(uint32_t)==4 &&
    sizeof(uint64_t)==8)?1:-1];

struct probe_state {
    struct ExecBase *exec;
    struct Task *task;
    int idnest,tdnest;
    struct pt_allocator allocator;
    struct pt_document document;
    struct pt_editor *editor;
    struct pt_midi_recording_session *session;struct pt_midi_frontend_context *frontend;
    struct pt_midi_setup_owner *owner;struct pt_editor_midi_recording *take;
    size_t session_capacity,frontend_capacity,owner_capacity,take_capacity;
    size_t document_bytes,encoded_bytes,byte_ceiling;
    unsigned char *before,*recorded,*scratch;
    struct pt_sample_version *pin;
    struct pt_pcm master;
    struct pt_sample sample;
    unsigned generation;
    size_t master_bytes;
    unsigned phase;
    /* Public, unowned shape for a checked encoded-size calculation only.
     * It is never passed to a release API or treated as a copied live owner. */
    struct pt_project planned;
    uint16_t order;
    struct pt_event events[PT_PROJECT_ROWS*TRACKS];
    struct pt_sample samples[SAMPLES];
    int32_t values[4];
};
static struct probe_state state;
static struct pt_private_native_memory memory;

static int held(void) __attribute__((noreturn));
static int held(void)
{
    /* Original HUNK, Task stack, controls, allocator and surviving resources
     * remain live. No unwinding/free/retry/Break or quiet inference. */
    for (;;) __asm__ volatile ("" ::: "memory");
    __builtin_unreachable();
}
#define REQUIRE(x) do { if (!(x)) held(); } while (0)
static int normal(void)
{
    return state.exec && SysBase==state.exec && state.exec->ThisTask==state.task &&
        (int)(signed char)state.exec->IDNestCnt==state.idnest &&
        (int)(signed char)state.exec->TDNestCnt==state.tdnest;
}
static int stack_headroom(void)
{
    const volatile struct Task *task;
    uintptr_t lower,upper,current;
    if (!normal()) return 0;
    task=state.task;lower=(uintptr_t)task->tc_SPLower;upper=(uintptr_t)task->tc_SPUpper;
    __asm__ volatile ("move.l %%sp,%0" : "=d" (current) : : "memory");
    return lower && lower<upper && current>=lower && current<upper && !(current&1u) &&
        current-lower>=1024UL*1024UL && task==state.task && normal() &&
        (uintptr_t)task->tc_SPLower==lower && (uintptr_t)task->tc_SPUpper==upper;
}
static int fast(const void *p,size_t bytes)
{
    uintptr_t a=(uintptr_t)p;
    return p && bytes && bytes<=UINTPTR_MAX-a &&
        (TypeOfMem((APTR)p)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST &&
        (TypeOfMem((APTR)(a+bytes-1))&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST;
}
static size_t add(size_t a,size_t b) { REQUIRE(b<=SIZE_MAX-a);return a+b; }
static size_t mul(size_t a,size_t b) { REQUIRE(!a || b<=SIZE_MAX/a);return a*b; }
static int allocator_same(struct pt_allocator a,struct pt_allocator b)
{ return a.context==b.context && a.allocate==b.allocate && a.release==b.release; }
static void *allocate(void *context,size_t bytes)
{
    void *p;
    REQUIRE(context==&memory && normal() && !memory.failed);
    p=pt_private_native_allocate(context,bytes);
    REQUIRE(normal() && !memory.failed);
    if (p) REQUIRE(fast(p,bytes));
    return p;
}
static void release(void *context,void *p)
{
    REQUIRE(context==&memory && normal() && !memory.failed);
    REQUIRE(pt_private_native_release_checked(&memory,p)==PT_PRIVATE_MEMORY_OK);
    REQUIRE(normal() && !memory.failed);
}
static void *new_zero(size_t bytes,size_t alignment)
{
    void *p=allocate(&memory,bytes);
    REQUIRE(p && alignment && (uintptr_t)p%alignment==0);
    memset(p,0,bytes);return p;
}
static struct pt_private_memory_stats stats(void)
{
    struct pt_private_memory_stats out;
    REQUIRE(normal() && pt_private_native_memory_stats(&memory,&out)==PT_PRIVATE_MEMORY_OK && !out.failed);
    return out;
}
static void plan(void)
{
    unsigned i;
    size_t bytes;
    state.values[0]=8388607;state.values[1]=1;state.values[2]=-1;state.values[3]=-8388608;
    pt_channels_init(&state.planned.channels);
    REQUIRE(pt_channels_resize(&state.planned.channels,TRACKS)==PT_CHANNEL_OK);
    state.planned.orders=&state.order;state.planned.events=state.events;state.planned.samples=state.samples;
    state.planned.order_count=1;state.planned.pattern_count=1;state.planned.sample_count=SAMPLES;
    state.planned.bpm=125;state.planned.speed=6;
    for (i=0;i<SAMPLES;++i) {
        state.samples[i].pcm.bits=8;state.samples[i].pcm.channels=1;state.samples[i].pcm.rate=PT_CLASSIC_RATE;
    }
    state.samples[0].pcm=(struct pt_pcm){state.values,4,4,48000,1,24};
    state.samples[0].volume=64;
    REQUIRE(pt_project_size(&state.planned,&state.encoded_bytes)==PT_PROJECT_OK && state.encoded_bytes);
    state.document_bytes=add(add(sizeof(uint16_t),mul(PT_PROJECT_ROWS*TRACKS,sizeof(struct pt_event))),
        mul(SAMPLES,sizeof(struct pt_sample)));
    state.session_capacity=add(pt_midi_recording_session_size(),TAIL);
    state.frontend_capacity=add(pt_midi_frontend_context_size(),TAIL);
    state.owner_capacity=add(pt_midi_setup_owner_size(),TAIL);
    state.take_capacity=add(pt_editor_midi_recording_size(),TAIL);
    bytes=add(state.document_bytes,sizeof(struct pt_editor));
    bytes=add(bytes,add(add(state.session_capacity,state.frontend_capacity),add(state.owner_capacity,state.take_capacity)));
    bytes=add(bytes,pt_midi_setup_size());
    bytes=add(bytes,mul(RECORD_BLOCKS,pt_midi_setup_command_bytes()));
    bytes=add(bytes,mul(SAVE_BLOCKS,state.encoded_bytes));
    /* All three opaque sampler allocations are charged by the genuine sampler.
     * This explicit allowance bounds them without copying its private layout. */
    state.byte_ceiling=add(bytes,SAMPLER_ALLOWANCE);
    REQUIRE(MAX_LIVE<=PT_PRIVATE_MEMORY_ALLOCATIONS && normal());
}
static void initialize(void)
{
    static const uint8_t raw[12]={0x7f,0xff,0xff,0,0,1,0xff,0xff,0xff,0x80,0,0};
    const struct pt_raw_format format={48000,24,1,0,0};
    struct pt_channel channel;
    struct pt_event_update update;
    struct pt_editor *e;
    unsigned calls;
    size_t bytes;
    pt_document_init(&state.document,&state.allocator);
    REQUIRE(pt_document_new(&state.document,TRACKS,state.document_bytes)==PT_PROJECT_OK &&
        state.document.allocated_bytes==state.document_bytes && normal());
    state.editor=new_zero(sizeof(*state.editor),ALIGN_OF(struct pt_editor));e=state.editor;
    REQUIRE(pt_editor_init(e,&state.document.project) && normal());
    REQUIRE(!e->sampler.bytes && !e->sampler.live_count && !e->sampler.pending_count &&
        !e->sample_source.loaded && !e->sample_source.allocated_bytes && !e->song.current && !e->song.bytes);
    /* Only still-empty owners are reinitialized, before any retained master. */
    pt_sampler_init(&e->sampler,&state.allocator,SAMPLER_ALLOWANCE);
    pt_document_init(&e->sample_source,&state.allocator);
    pt_song_init(&e->song,&state.allocator,state.document_bytes);
    REQUIRE(allocator_same(e->sampler.allocator,state.allocator) &&
        allocator_same(e->sample_source.allocator,state.allocator) && allocator_same(e->song.allocator,state.allocator) &&
        e->sampler.budget==SAMPLER_ALLOWANCE && e->song.budget==state.document_bytes &&
        !e->sampler.bytes && !e->sampler.live_count && !e->sampler.pending_count &&
        !e->sample_source.loaded && !e->sample_source.allocated_bytes && !e->song.current && !e->song.bytes && normal());
    REQUIRE(pt_sampler_import_raw(&e->sampler,&state.document.project,&e->history,0,
        raw,sizeof(raw),"Original24",&format)==PT_EDIT_OK && normal());
    REQUIRE(stats().live==DOCUMENT_BLOCKS+EDITOR_BLOCKS+SAMPLER_BLOCKS && e->sampler.bytes<=SAMPLER_ALLOWANCE);
    channel=state.document.project.channels.track[TARGET];channel.route=PT_MIDI;channel.midi_channel=3;
    REQUIRE(pt_pattern_channel_apply(&state.document.project,&e->history,TARGET,&channel)==PT_EDIT_OK);
    update.index=TARGET;update.event=state.document.project.events[TARGET];
    update.event.instrument=1;update.event.effect=14;update.event.parameter=0xc3;
    REQUIRE(pt_pattern_apply(&state.document.project,&e->history,&update,1)==PT_EDIT_OK);
    calls=memory.allocations;
    REQUIRE(pt_sampler_pin(&e->sampler,&state.document.project,0,e->sampler.generation,
        &state.master,&state.pin)==PT_EDIT_OK && state.pin && memory.allocations==calls && normal());
    state.sample=state.document.project.samples[0];state.generation=e->sampler.generation;state.master_bytes=e->sampler.bytes;
    REQUIRE(pt_project_size(&state.document.project,&bytes)==PT_PROJECT_OK && bytes==state.encoded_bytes);
    state.before=new_zero(bytes,1);state.recorded=new_zero(bytes,1);state.scratch=new_zero(bytes,1);
    state.session=new_zero(state.session_capacity,pt_midi_recording_session_alignment());
    state.frontend=new_zero(state.frontend_capacity,pt_midi_frontend_context_alignment());
    state.owner=new_zero(state.owner_capacity,pt_midi_setup_owner_alignment());
    state.take=new_zero(state.take_capacity,pt_editor_midi_recording_alignment());
    memset((unsigned char *)state.session+pt_midi_recording_session_size(),0xa1,TAIL);
    memset((unsigned char *)state.frontend+pt_midi_frontend_context_size(),0xa2,TAIL);
    memset((unsigned char *)state.owner+pt_midi_setup_owner_size(),0xa3,TAIL);
    memset((unsigned char *)state.take+pt_editor_midi_recording_size(),0xa4,TAIL);
    REQUIRE(stats().live==MAX_LIVE-WORKSPACE_BLOCKS-RECORD_BLOCKS && normal());
    REQUIRE(pt_midi_recording_session_construct(state.session,state.session_capacity,
        state.frontend,state.frontend_capacity,state.owner,state.owner_capacity,
        state.take,state.take_capacity,e,(struct pt_midi_setup_owner_span){&memory,sizeof(memory)},
        RECORD_BLOCKS*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK && normal());
    REQUIRE(stats().live==MAX_LIVE-RECORD_BLOCKS &&
        pt_midi_recording_session_state(state.session)==PT_MIDI_SESSION_LIVE);
    pt_editor_saved(e);
}
static void master_unchanged(void)
{
    const struct pt_sample *s=&state.document.project.samples[0];
    REQUIRE(state.editor->sampler.current[0]==state.pin &&
        state.editor->sampler.generation==state.generation && state.editor->sampler.bytes==state.master_bytes &&
        state.editor->sampler.budget==SAMPLER_ALLOWANCE && !memcmp(s,&state.sample,sizeof(*s)) &&
        s->pcm.data==state.master.data && s->pcm.capacity==4 && s->pcm.frames==4 &&
        s->pcm.rate==48000 && s->pcm.channels==1 && s->pcm.bits==24 &&
        !memcmp(s->pcm.data,state.values,sizeof(state.values)) && normal());
}
static void tails(void)
{
    unsigned i;
    for(i=0;i<TAIL;++i) {
        REQUIRE(((unsigned char *)state.session)[pt_midi_recording_session_size()+i]==0xa1);
        REQUIRE(((unsigned char *)state.frontend)[pt_midi_frontend_context_size()+i]==0xa2);
        REQUIRE(((unsigned char *)state.owner)[pt_midi_setup_owner_size()+i]==0xa3);
        REQUIRE(((unsigned char *)state.take)[pt_editor_midi_recording_size()+i]==0xa4);
    }
}
static void settings(void)
{
    const char name[]="Native Session Input";size_t i;
    struct pt_midi_requester_model *model;unsigned calls=memory.allocations;
    REQUIRE(pt_midi_recording_session_setup_begin(state.session,TARGET)==PT_MIDI_SETUP_OK);
    model=pt_midi_recording_session_model(state.session);REQUIRE(model);
    REQUIRE(pt_midi_requester_model_select(model,PT_MIDI_SETUP_INPUT)==PT_MIDI_SETUP_OK);
    REQUIRE(pt_midi_requester_model_key(model,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
    for(i=0;i<sizeof(name)-1;++i) REQUIRE(pt_midi_requester_model_key(model,
        PT_MIDI_KEY_CHARACTER,(unsigned char)name[i])==PT_MIDI_EVENT_CHANGED);
    REQUIRE(pt_midi_requester_model_apply(model)==PT_MIDI_SETUP_OK && !model->self && normal());
    REQUIRE(pt_midi_recording_session_setup_end(state.session)==PT_MIDI_SETUP_OK &&
        !strcmp(state.document.project.midi_input,name) && memory.allocations==calls+RECORD_BLOCKS &&
        stats().live==MAX_LIVE && normal());
    REQUIRE(pt_midi_recording_session_setup_begin(state.session,TARGET)==PT_MIDI_SETUP_OK);
    model=pt_midi_recording_session_model(state.session);REQUIRE(model);
    REQUIRE(pt_midi_requester_model_select(model,PT_MIDI_SETUP_INPUT)==PT_MIDI_SETUP_OK);
    REQUIRE(pt_midi_requester_model_key(model,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
    REQUIRE(pt_midi_recording_session_setup_end(state.session)==PT_MIDI_SETUP_OK &&
        !strcmp(state.document.project.midi_input,name) && memory.allocations==calls+RECORD_BLOCKS);
    master_unchanged();tails();
}
static void encode(unsigned char *out)
{
    size_t bytes,written;
    REQUIRE(pt_project_size(&state.document.project,&bytes)==PT_PROJECT_OK && bytes==state.encoded_bytes &&
        pt_project_encode(&state.document.project,out,bytes,&written)==PT_PROJECT_OK && written==bytes && normal());
    master_unchanged();
}
static struct pt_editor_midi_record_status take_status(void)
{
    struct pt_editor_midi_record_status out;
    REQUIRE(pt_midi_recording_session_record_status(state.session,&out)==PT_EDITOR_RECORD_OK && normal());
    return out;
}
static void begin(uint32_t first,uint32_t limit)
{
    const struct pt_record_settings options={first,limit,1,1,1,0};
    REQUIRE(pt_midi_recording_session_record_begin(state.session,&options,ROW(first),TARGET,16)==PT_EDITOR_RECORD_OK && normal());
    REQUIRE(take_status().attached && take_status().armed && !take_status().started && !take_status().active);
    tails();
}
static void veto(void)
{
    struct pt_private_memory_stats before=stats(),after;size_t cursor=state.editor->history.cursor;
    REQUIRE(take_status().attached && !pt_editor_prepare_change(state.editor) && !pt_editor_dispose(state.editor));
    REQUIRE(pt_midi_recording_session_setup_begin(state.session,TARGET)==PT_MIDI_SETUP_ACTIVE &&
        pt_midi_recording_session_dispose_close(state.session)==PT_MIDI_SETUP_ACTIVE &&
        pt_midi_frontend_context_state(state.frontend)==PT_MIDI_FRONTEND_LIVE);
    after=stats();REQUIRE(before.allocations==after.allocations && before.releases==after.releases &&
        before.live==after.live && before.used==after.used && state.editor->history.cursor==cursor && normal());
    master_unchanged();tails();
}
static void message(uint64_t position,unsigned status,unsigned note,unsigned velocity,
    enum pt_editor_midi_record_result expected)
{
    const uint8_t bytes[3]={(uint8_t)status,(uint8_t)note,(uint8_t)velocity};
    REQUIRE(pt_midi_recording_session_message(state.session,position,bytes,sizeof(bytes))==expected && normal());
    master_unchanged();tails();
}
static void ordinary_take(void)
{
    size_t cursor=state.editor->history.cursor;
    unsigned calls=memory.allocations;
    uint32_t revision;
    const struct pt_event *events=state.document.project.events;
    encode(state.before);begin(0,64);veto();encode(state.scratch);
    REQUIRE(!memcmp(state.before,state.scratch,state.encoded_bytes));
    message(ROW(0),0x90,60,91,PT_EDITOR_RECORD_IGNORED);
    REQUIRE(state.editor->history.cursor==cursor);
    message(ROW(0),0x9f,60,91,PT_EDITOR_RECORD_OK);
    REQUIRE(take_status().started && take_status().active==1);
    message(ROW(1),0x9f,60,0,PT_EDITOR_RECORD_OK);
    REQUIRE(pt_midi_recording_session_stop(state.session,ROW(2))==PT_EDITOR_RECORD_OK && normal());
    REQUIRE(!take_status().armed && !take_status().active);veto();
    REQUIRE(pt_midi_recording_session_detach(state.session)==PT_EDITOR_RECORD_OK &&
        pt_midi_recording_session_state(state.session)==PT_MIDI_SESSION_LIVE && pt_editor_prepare_change(state.editor) && normal());
    REQUIRE(state.editor->history.cursor==cursor+2 && events[TARGET].kind==PT_NOTE_MIDI &&
        events[TARGET].pitch==60 && events[TARGET].velocity==91 && events[TARGET].instrument==1 &&
        events[TARGET].effect==14 && events[TARGET].parameter==0xc3 && events[TRACKS+TARGET].kind==PT_NOTE_OFF);
    encode(state.recorded);REQUIRE(memcmp(state.before,state.recorded,state.encoded_bytes));
    pt_editor_saved(state.editor);revision=state.editor->history.revision;
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,-1)==PT_EDIT_OK &&
        pt_pattern_undo(&state.document.project,&state.editor->history,-1)==PT_EDIT_OK);
    encode(state.scratch);REQUIRE(!memcmp(state.before,state.scratch,state.encoded_bytes));
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,1)==PT_EDIT_OK &&
        pt_pattern_undo(&state.document.project,&state.editor->history,1)==PT_EDIT_OK);
    encode(state.scratch);
    REQUIRE(!memcmp(state.recorded,state.scratch,state.encoded_bytes) &&
        state.editor->history.revision==revision && !pt_editor_dirty(state.editor) && memory.allocations==calls);
    tails();state.phase=1;
}
static void failed_stop_local_cancel(void)
{
    size_t cursor=state.editor->history.cursor;
    unsigned calls=memory.allocations;
    begin(63,64);message(ROW(63),0x9f,61,100,PT_EDITOR_RECORD_OK);
    REQUIRE(pt_midi_recording_session_stop(state.session,ROW(64))==PT_EDITOR_RECORD_CAPACITY && normal());
    REQUIRE(take_status().armed && take_status().active==1 &&
        pt_midi_recording_session_detach(state.session)==PT_EDITOR_RECORD_CONFLICT);
    veto();
    REQUIRE(pt_midi_recording_session_cancel(state.session)==PT_EDITOR_RECORD_OK && normal());
    REQUIRE(!take_status().armed && !take_status().active &&
        state.editor->history.cursor==cursor+1 && state.document.project.events[63*TRACKS+TARGET].kind==PT_NOTE_MIDI);
    veto();
    REQUIRE(pt_midi_recording_session_detach(state.session)==PT_EDITOR_RECORD_OK &&
        pt_midi_recording_session_state(state.session)==PT_MIDI_SESSION_LIVE && pt_editor_prepare_change(state.editor) && normal());
    REQUIRE(memory.allocations==calls);master_unchanged();tails();state.phase=2;
}
static void finish(void)
{
    struct pt_private_memory_stats after;
    REQUIRE(pt_private_native_memory_seal(&memory)==PT_PRIVATE_MEMORY_OK && normal());
    REQUIRE(pt_midi_recording_session_state(state.session)==PT_MIDI_SESSION_LIVE);
    master_unchanged();pt_sampler_unpin(state.pin);state.pin=NULL;
    REQUIRE(pt_midi_recording_session_dispose_close(state.session)==PT_MIDI_SETUP_OK && normal());
    REQUIRE(pt_midi_recording_session_state(state.session)==PT_MIDI_SESSION_CLOSED &&
        pt_midi_frontend_context_state(state.frontend)==PT_MIDI_FRONTEND_CLOSED &&
        pt_sampler_lifetime_storage_retired(&state.editor->sampler) && !state.editor->history.count);
    release(&memory,state.editor);state.editor=NULL;
    REQUIRE(pt_document_release_checked(&state.document)==PT_DOCUMENT_RELEASED && normal());
    REQUIRE(pt_midi_recording_session_state(state.session)==PT_MIDI_SESSION_CLOSED &&
        !pt_midi_recording_session_model(state.session));tails();
    release(&memory,state.take);state.take=NULL;release(&memory,state.owner);state.owner=NULL;
    release(&memory,state.frontend);state.frontend=NULL;release(&memory,state.session);state.session=NULL;
    release(&memory,state.scratch);state.scratch=NULL;release(&memory,state.recorded);state.recorded=NULL;
    release(&memory,state.before);state.before=NULL;after=stats();
    REQUIRE(!after.live && !after.used && !after.failed && after.allocations==after.releases && after.peak_live==MAX_LIVE);
    REQUIRE(pt_private_native_memory_finish(&memory)==PT_PRIVATE_MEMORY_OK && normal());state.phase=3;
}
int main(void)
{
    struct pt_private_memory_span guard;
    state.exec=SysBase;
    REQUIRE(state.exec && (state.task=FindTask(NULL)) && state.exec->ThisTask==state.task);
    state.idnest=(int)(signed char)state.exec->IDNestCnt;state.tdnest=(int)(signed char)state.exec->TDNestCnt;
    REQUIRE(state.idnest==-1 && state.tdnest==-1 && normal() && stack_headroom());
    REQUIRE(fast(&state,sizeof(state)) && fast(&memory,sizeof(memory)) && normal());plan();
    guard=(struct pt_private_memory_span){&state,sizeof(state)};
    REQUIRE(pt_private_native_memory_init(&memory,state.byte_ceiling,&guard,1)==PT_PRIVATE_MEMORY_OK && normal());
    state.allocator=(struct pt_allocator){&memory,allocate,release};initialize();
    settings();ordinary_take();failed_stop_local_cancel();finish();puts(PASS_MARKER);return 0;
}
