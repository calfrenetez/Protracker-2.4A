/* Explicit ordinary-TASK RAM qualification of the genuine durable context.
 * No parent/window/frontend enrollment, output, device, CAMD or publication. */
#if !defined(PT_MIDI_FRONTEND_CONTEXT_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_FRONTEND_CONTEXT_NATIVE_TASK_PROBE_ONLY != 1
#error "Select the exact native frontend-context TASK RAM probe"
#endif
#include "midi_frontend_context.h"
#include "native_checked_memory.h"
#include "../core/mod_project.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define PASS_MARKER "MIDI FRONTEND CONTEXT TASK RAM PASS:original constructor/modal/journal;apply/cancel/retained redo;dispose/close/workspace;zero Fast ledgers;NO WINDOW/FRONTEND/DEVICE/OUTPUT AUTHORITY"
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
enum {TAIL=16,TRACKS=4,SAMPLES=31,DOCUMENT_BLOCKS=3,FIXED_BLOCKS=5,
    WORKSPACE_BLOCKS=1,RECORD_BLOCKS=2,MAX_LIVE=DOCUMENT_BLOCKS+FIXED_BLOCKS+WORKSPACE_BLOCKS+RECORD_BLOCKS};
typedef char genuine_classic_geometry[(CHAR_BIT==8&&sizeof(void *)==4&&sizeof(size_t)==4&&
    sizeof(uintptr_t)==4&&sizeof(uint32_t)==4&&sizeof(uint64_t)==8)?1:-1];
struct probe_state {
    struct ExecBase *exec;struct Task *task;int idnest,tdnest;
    struct pt_document document;struct pt_editor *editor;
    struct pt_midi_frontend_context *frontend;size_t frontend_capacity;
    struct pt_midi_setup_owner *owner;size_t owner_capacity;
    struct pt_allocator allocator;
    unsigned char *encoded,*scratch;size_t encoded_bytes,document_bytes,byte_ceiling;
    struct pt_project planned;uint16_t order;
    struct pt_event events[PT_PROJECT_ROWS*TRACKS];struct pt_sample samples[SAMPLES];
    unsigned barriers,phase,callback_mode,callback_hits;
};
static struct probe_state state;
static struct pt_private_native_memory memory;
static int held(void) __attribute__((noreturn));
static int held(void)
{
    /* Keep this original HUNK/Task/stack and every surviving owner/context.
     * No unwinding, forced free, retry, quiet inference or Ctrl-C recovery. */
    for(;;)__asm__ volatile ("" ::: "memory");
    __builtin_unreachable();
}
#define REQUIRE(x) do {if(!(x))held();} while(0)
static int normal(void)
{return state.exec&&SysBase==state.exec&&state.exec->ThisTask==state.task&&
    (int)(signed char)state.exec->IDNestCnt==state.idnest&&
    (int)(signed char)state.exec->TDNestCnt==state.tdnest;}
static int stack_headroom(void)
{
    const volatile struct Task *task;uintptr_t lower,upper,current;
    if(!normal())return 0;
    task=state.task;lower=(uintptr_t)task->tc_SPLower;upper=(uintptr_t)task->tc_SPUpper;
    __asm__ volatile ("move.l %%sp,%0" : "=d" (current) : : "memory");
    return lower&&lower<upper&&current>=lower&&current<upper&&!(current&1u)&&
        current-lower>=1024UL*1024UL&&task==state.task&&normal()&&
        (uintptr_t)task->tc_SPLower==lower&&(uintptr_t)task->tc_SPUpper==upper;
}
static int fast(const void *p,size_t bytes)
{
    uintptr_t a=(uintptr_t)p;
    return p&&bytes&&bytes<=UINTPTR_MAX-a&&
        (TypeOfMem((APTR)p)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST&&
        (TypeOfMem((APTR)(a+bytes-1))&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST;
}
static size_t add(size_t a,size_t b){REQUIRE(b<=SIZE_MAX-a);return a+b;}
static size_t mul(size_t a,size_t b){REQUIRE(!a||b<=SIZE_MAX/a);return a*b;}
static int allocator_same(struct pt_allocator a,struct pt_allocator b)
{return a.context==b.context&&a.allocate==b.allocate&&a.release==b.release;}
static void callback_reentry(void)
{
    enum pt_midi_setup_result a,b,c;
    if(!state.callback_mode)return;
    a=pt_midi_frontend_context_begin(state.frontend,0);
    b=pt_midi_frontend_context_end(state.frontend);
    c=pt_midi_frontend_context_dispose_close(state.frontend);
    if(state.callback_mode==1)REQUIRE(a==PT_MIDI_SETUP_BUSY&&b==PT_MIDI_SETUP_BUSY&&c==PT_MIDI_SETUP_BUSY);
    else REQUIRE(state.callback_mode==2&&a==PT_MIDI_SETUP_IN_USE&&b==PT_MIDI_SETUP_BUSY&&c==PT_MIDI_SETUP_IN_USE);
    REQUIRE(!pt_midi_frontend_context_model(state.frontend)&&normal());++state.callback_hits;
}
static void *allocate(void *context,size_t bytes)
{
    void *p;REQUIRE(context==&memory&&normal()&&!memory.failed);callback_reentry();
    p=pt_private_native_allocate(context,bytes);
    REQUIRE(normal()&&!memory.failed);if(p)REQUIRE(fast(p,bytes));return p;
}
static void release(void *context,void *p)
{
    REQUIRE(context==&memory&&normal()&&!memory.failed);callback_reentry();
    REQUIRE(pt_private_native_release_checked(&memory,p)==PT_PRIVATE_MEMORY_OK);
    REQUIRE(normal()&&!memory.failed);
}
static void *new_zero(size_t bytes,size_t alignment)
{
    void *p=allocate(&memory,bytes);
    REQUIRE(p&&alignment&&(uintptr_t)p%alignment==0);memset(p,0,bytes);return p;
}
static struct pt_private_memory_stats stats(void)
{
    struct pt_private_memory_stats out;
    REQUIRE(normal()&&pt_private_native_memory_stats(&memory,&out)==PT_PRIVATE_MEMORY_OK&&!out.failed);
    return out;
}
static int change_ready(void *context)
{REQUIRE(context==&state&&normal()&&!memory.failed);++state.barriers;return 1;}
static void plan(void)
{
    unsigned i;size_t bytes=0;
    /* Exact empty new-document shape, only borrowed to compute encoded size.
     * These public descriptors are not a copied/live owner or a release token. */
    pt_channels_init(&state.planned.channels);
    REQUIRE(pt_channels_resize(&state.planned.channels,TRACKS)==PT_CHANNEL_OK);
    state.planned.orders=&state.order;state.planned.events=state.events;state.planned.samples=state.samples;
    state.planned.order_count=1;state.planned.pattern_count=1;state.planned.sample_count=SAMPLES;
    state.planned.bpm=125;state.planned.speed=6;
    for(i=0;i<SAMPLES;++i){state.samples[i].pcm.bits=8;state.samples[i].pcm.channels=1;state.samples[i].pcm.rate=PT_CLASSIC_RATE;}
    REQUIRE(pt_project_size(&state.planned,&state.encoded_bytes)==PT_PROJECT_OK&&state.encoded_bytes);
    state.document_bytes=add(add(sizeof(uint16_t),mul(PT_PROJECT_ROWS*TRACKS,sizeof(struct pt_event))),mul(SAMPLES,sizeof(struct pt_sample)));
    state.frontend_capacity=add(pt_midi_frontend_context_size(),TAIL);
    state.owner_capacity=add(pt_midi_setup_owner_size(),TAIL);
    bytes=add(bytes,state.document_bytes);bytes=add(bytes,sizeof(struct pt_editor));
    bytes=add(bytes,state.frontend_capacity);bytes=add(bytes,state.owner_capacity);
    bytes=add(bytes,pt_midi_setup_size());bytes=add(bytes,mul(RECORD_BLOCKS,pt_midi_setup_command_bytes()));
    bytes=add(bytes,mul(2,state.encoded_bytes));state.byte_ceiling=bytes;
    REQUIRE(MAX_LIVE<=PT_PRIVATE_MEMORY_ALLOCATIONS&&pt_midi_setup_command_bytes()&&normal());
}
static void initialize_editor(void)
{
    struct pt_editor *e=state.editor;
    REQUIRE(pt_editor_init(e,&state.document.project)&&normal());
    REQUIRE(!e->sampler.bytes&&!e->sampler.live_count&&!e->sampler.pending_count&&
        !e->sample_source.loaded&&!e->sample_source.allocated_bytes&&!e->song.current&&!e->song.bytes);
    pt_sampler_init(&e->sampler,&state.allocator,state.document_bytes);
    pt_document_init(&e->sample_source,&state.allocator);
    pt_song_init(&e->song,&state.allocator,state.document_bytes);
    REQUIRE(allocator_same(e->sampler.allocator,state.allocator)&&
        allocator_same(e->sample_source.allocator,state.allocator)&&allocator_same(e->song.allocator,state.allocator)&&
        e->sampler.budget==state.document_bytes&&e->song.budget==state.document_bytes&&
        !e->sampler.bytes&&!e->sampler.live_count&&!e->sampler.pending_count&&
        !e->sample_source.loaded&&!e->sample_source.allocated_bytes&&!e->song.current&&!e->song.bytes&&normal());
    REQUIRE(pt_editor_change_barrier(e,change_ready,&state));
}
static struct pt_midi_setup_owner_status owner_status(void)
{
    struct pt_midi_setup_owner_status out;
    REQUIRE(pt_midi_setup_owner_get(state.owner,&out)==PT_MIDI_SETUP_OK&&normal());return out;
}
static void tails(void)
{
    const unsigned char *a=(const unsigned char *)state.frontend+pt_midi_frontend_context_size();
    const unsigned char *b=(const unsigned char *)state.owner+pt_midi_setup_owner_size();unsigned i;
    for(i=0;i<TAIL;++i)REQUIRE(a[i]==0x6d&&b[i]==0x7b);
}
static struct pt_midi_requester_model *begin(enum pt_midi_setup_target target,unsigned track)
{
    struct pt_midi_requester_model *model;unsigned calls=memory.allocations;
    REQUIRE(pt_midi_frontend_context_begin(state.frontend,track)==PT_MIDI_SETUP_OK&&normal());
    model=pt_midi_frontend_context_model(state.frontend);REQUIRE(model&&owner_status().modal==1);
    REQUIRE(pt_midi_requester_model_select(model,target)==PT_MIDI_SETUP_OK&&normal());
    REQUIRE(memory.allocations==calls&&pt_midi_frontend_context_dispose_close(state.frontend)==PT_MIDI_SETUP_IN_USE);
    return model;
}
static void type_name(struct pt_midi_requester_model *model,const char *text)
{
    size_t i;REQUIRE(pt_midi_requester_model_key(model,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
    for(i=0;i<PT_MIDI_ENDPOINT&&text[i];++i)
        REQUIRE(pt_midi_requester_model_key(model,PT_MIDI_KEY_CHARACTER,(unsigned char)text[i])==PT_MIDI_EVENT_CHANGED);
    REQUIRE(i<PT_MIDI_ENDPOINT&&normal());
}
static void end(void)
{
    REQUIRE(pt_midi_frontend_context_end(state.frontend)==PT_MIDI_SETUP_OK&&normal());
    REQUIRE(pt_midi_frontend_context_state(state.frontend)==PT_MIDI_FRONTEND_LIVE&&!owner_status().modal);
    REQUIRE(pt_midi_frontend_context_end(state.frontend)==PT_MIDI_SETUP_IN_USE);tails();
}
static void commit(enum pt_midi_setup_target target,unsigned track,const char *text)
{
    struct pt_midi_requester_model *model=begin(target,track);unsigned hits=state.callback_hits;type_name(model,text);
    state.callback_mode=2;
    REQUIRE(pt_midi_requester_model_apply(model)==PT_MIDI_SETUP_OK&&!model->self&&normal());
    REQUIRE(state.callback_hits==hits+1);state.callback_mode=0;
    REQUIRE(!pt_midi_frontend_context_model(state.frontend)&&owner_status().modal==1);end();
}
static void saved_equal(void)
{
    size_t bytes,written;REQUIRE(pt_project_size(&state.document.project,&bytes)==PT_PROJECT_OK&&bytes==state.encoded_bytes);
    REQUIRE(pt_project_encode(&state.document.project,state.scratch,bytes,&written)==PT_PROJECT_OK&&
        written==bytes&&!memcmp(state.scratch,state.encoded,bytes)&&normal());
}
int main(void)
{
    struct pt_private_memory_span guard;struct pt_private_memory_stats before,after;
    struct pt_midi_requester_model *model;struct pt_midi_setup_owner_status status;
    size_t written;unsigned calls,barriers;uint32_t revision;
    state.exec=SysBase;REQUIRE(state.exec&&(state.task=FindTask(NULL))&&state.exec->ThisTask==state.task);
    state.idnest=(int)(signed char)state.exec->IDNestCnt;state.tdnest=(int)(signed char)state.exec->TDNestCnt;
    REQUIRE(state.idnest==-1&&state.tdnest==-1&&normal()&&stack_headroom());
    REQUIRE(fast(&state,sizeof(state))&&fast(&memory,sizeof(memory))&&normal());plan();
    guard=(struct pt_private_memory_span){&state,sizeof(state)};
    REQUIRE(pt_private_native_memory_init(&memory,state.byte_ceiling,&guard,1)==PT_PRIVATE_MEMORY_OK&&normal());
    state.allocator=(struct pt_allocator){&memory,allocate,release};pt_document_init(&state.document,&state.allocator);
    REQUIRE(pt_document_new(&state.document,TRACKS,state.document_bytes)==PT_PROJECT_OK&&
        state.document.allocated_bytes==state.document_bytes&&normal());
    state.editor=new_zero(sizeof(*state.editor),ALIGN_OF(struct pt_editor));initialize_editor();
    state.frontend=new_zero(state.frontend_capacity,pt_midi_frontend_context_alignment());
    state.owner=new_zero(state.owner_capacity,pt_midi_setup_owner_alignment());
    memset((unsigned char *)state.frontend+pt_midi_frontend_context_size(),0x6d,TAIL);
    memset((unsigned char *)state.owner+pt_midi_setup_owner_size(),0x7b,TAIL);
    calls=memory.allocations;state.callback_mode=1;
    REQUIRE(pt_midi_frontend_context_construct(state.frontend,state.frontend_capacity,state.owner,state.owner_capacity,
        state.editor,(struct pt_midi_setup_owner_span){&memory,sizeof(memory)},mul(RECORD_BLOCKS,pt_midi_setup_command_bytes()))==PT_MIDI_SETUP_OK&&normal());
    REQUIRE(state.callback_hits==1);state.callback_mode=0;
    REQUIRE(memory.allocations==calls+1&&pt_midi_frontend_context_state(state.frontend)==PT_MIDI_FRONTEND_LIVE);
    status=owner_status();REQUIRE(!status.modal&&!status.commands&&status.workspace_bytes==pt_midi_setup_size());tails();state.phase=1;
    calls=memory.allocations;barriers=state.barriers;
    REQUIRE(pt_midi_frontend_context_begin(state.frontend,99)==PT_MIDI_SETUP_INVALID&&normal());
    REQUIRE(pt_midi_frontend_context_state(state.frontend)==PT_MIDI_FRONTEND_LIVE&&!owner_status().modal&&
        memory.allocations==calls&&state.barriers==barriers);
    commit(PT_MIDI_SETUP_INPUT,0,"keys");commit(PT_MIDI_SETUP_OUTPUT,2,"synth");
    REQUIRE(!strcmp(state.document.project.midi_input,"keys")&&!strcmp(state.document.project.midi_output[2],"synth"));
    pt_editor_saved(state.editor);revision=state.editor->history.revision;
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,-1)==PT_EDIT_OK&&normal());
    status=owner_status();REQUIRE(status.commands==RECORD_BLOCKS&&status.metadata_bytes==mul(RECORD_BLOCKS,pt_midi_setup_command_bytes())&&
        state.editor->history.count==2&&state.editor->history.cursor==1&&!state.document.project.midi_output[2][0]);
    state.encoded=new_zero(state.encoded_bytes,1);state.scratch=new_zero(state.encoded_bytes,1);
    REQUIRE(pt_project_encode(&state.document.project,state.encoded,state.encoded_bytes,&written)==PT_PROJECT_OK&&written==state.encoded_bytes);
    before=stats();REQUIRE(before.live==MAX_LIVE&&before.used==state.byte_ceiling);barriers=state.barriers;
    model=begin(PT_MIDI_SETUP_INPUT,0);type_name(model,"cancelled");end();after=stats();
    REQUIRE(before.allocations==after.allocations&&before.releases==after.releases&&before.live==after.live&&
        before.used==after.used&&state.barriers==barriers&&state.editor->history.count==2&&state.editor->history.cursor==1);saved_equal();
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,1)==PT_EDIT_OK&&normal());
    REQUIRE(state.editor->history.revision==revision&&!pt_editor_dirty(state.editor)&&!strcmp(state.document.project.midi_output[2],"synth"));
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,-1)==PT_EDIT_OK&&normal());saved_equal();state.phase=2;
    REQUIRE(pt_private_native_memory_seal(&memory)==PT_PRIVATE_MEMORY_OK&&normal());before=stats();
    calls=state.callback_hits;state.callback_mode=1;
    REQUIRE(pt_midi_frontend_context_dispose_close(state.frontend)==PT_MIDI_SETUP_OK&&normal());
    REQUIRE(state.callback_hits==calls+RECORD_BLOCKS+WORKSPACE_BLOCKS);state.callback_mode=0;
    after=stats();REQUIRE(after.releases==before.releases+RECORD_BLOCKS+WORKSPACE_BLOCKS&&
        after.live+RECORD_BLOCKS+WORKSPACE_BLOCKS==before.live&&pt_midi_frontend_context_state(state.frontend)==PT_MIDI_FRONTEND_CLOSED);tails();
    /* Only the genuine original owner of these unexposed objects retires them.
     * CLOSED is not a grant for arbitrary document/master/device lifetimes. */
    release(&memory,state.scratch);state.scratch=NULL;release(&memory,state.encoded);state.encoded=NULL;
    release(&memory,state.owner);state.owner=NULL;release(&memory,state.editor);state.editor=NULL;
    REQUIRE(pt_document_release_checked(&state.document)==PT_DOCUMENT_RELEASED&&normal());
    calls=memory.releases;
    REQUIRE(pt_midi_frontend_context_state(state.frontend)==PT_MIDI_FRONTEND_CLOSED&&
        pt_midi_frontend_context_begin(state.frontend,0)==PT_MIDI_SETUP_IN_USE&&
        pt_midi_frontend_context_end(state.frontend)==PT_MIDI_SETUP_IN_USE&&
        pt_midi_frontend_context_dispose_close(state.frontend)==PT_MIDI_SETUP_IN_USE&&
        !pt_midi_frontend_context_model(state.frontend)&&memory.releases==calls);
    release(&memory,state.frontend);state.frontend=NULL;after=stats();
    REQUIRE(!after.live&&!after.used&&!after.failed&&after.allocations==after.releases&&after.peak_live==MAX_LIVE&&state.barriers==3);
    REQUIRE(pt_private_native_memory_finish(&memory)==PT_PRIVATE_MEMORY_OK&&normal());state.phase=3;puts(PASS_MARKER);return 0;
}
