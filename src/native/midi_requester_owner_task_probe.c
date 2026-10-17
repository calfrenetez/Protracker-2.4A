/* Separate ordinary-TASK RAM qualification only. No window, document
 * publication, native frontend, CAMD/library discovery, MIDI/audio or IRQ. */
#if !defined(PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_SETUP_OWNER_NATIVE_TASK_PROBE_ONLY != 1
#error "Select the exact native owner TASK RAM probe"
#endif
#if !defined(PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY) || PT_MIDI_REQUESTER_OWNER_NATIVE_TASK_PROBE_ONLY != 1
#error "Select the exact native requester TASK RAM probe"
#endif
#include "midi_requester.h"
#include "native_checked_memory.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define PASS_MARKER "MIDI OWNER TASK RAM PASS:real editor/owner/modal;apply/cancel/retained redo;dispose/close/final-address rebind;original Fast context;zero ledgers;NO WINDOW/CAMD/OUTPUT AUTHORITY"
enum {TAIL=16};
typedef char probe_real_classic_geometry[(CHAR_BIT==8&&sizeof(void *)==4&&
    sizeof(uintptr_t)==4&&sizeof(size_t)==4&&sizeof(uint32_t)==4&&
    sizeof(uint64_t)==8)?1:-1];
struct probe_state {
    struct ExecBase *exec;struct Task *task;int idnest,tdnest;
    struct pt_document document;
    struct pt_editor *editor;
    struct pt_midi_setup_owner *owner;size_t owner_capacity;
    struct pt_midi_requester_modal *modal;
    struct pt_allocator allocator;
    unsigned char *encoded,*scratch;size_t encoded_bytes;
    unsigned barriers,phase;
};
static struct probe_state state;
static struct pt_private_native_memory memory;

static int held(void) __attribute__((noreturn));
static int held(void)
{
    /* Retain this HUNK/Task and every original surviving context. A refusal
     * grants no rollback, retry, forced history disposal or process exit. */
    for(;;)__asm__ volatile ("" ::: "memory");
    __builtin_unreachable();
}
#define REQUIRE(condition) do {if(!(condition))held();} while(0)
static int normal(void)
{
    return state.exec&&SysBase==state.exec&&state.exec->ThisTask==state.task&&
        (int)(signed char)state.exec->IDNestCnt==state.idnest&&
        (int)(signed char)state.exec->TDNestCnt==state.tdnest;
}
static int fast_class(const void *p,size_t bytes)
{
    uintptr_t a=(uintptr_t)p;
    return p&&bytes&&bytes<=UINTPTR_MAX-a&&
        (TypeOfMem((APTR)p)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST&&
        (TypeOfMem((APTR)(a+bytes-1))&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST;
}
static int allocator_same(struct pt_allocator a,struct pt_allocator b)
{return a.context==b.context&&a.allocate==b.allocate&&a.release==b.release;}
static void *allocate(void *context,size_t bytes)
{
    void *p;REQUIRE(context==&memory&&normal()&&!memory.failed);
    p=pt_private_native_allocate(context,bytes);
    REQUIRE(normal()&&!memory.failed);if(p)REQUIRE(fast_class(p,bytes));return p;
}
static void release(void *context,void *p)
{
    REQUIRE(context==&memory&&normal()&&!memory.failed);
    REQUIRE(pt_private_native_release_checked(&memory,p)==PT_PRIVATE_MEMORY_OK);
    REQUIRE(normal()&&!memory.failed);
}
static int change_ready(void *context)
{
    REQUIRE(context==&state&&normal()&&!memory.failed);++state.barriers;return 1;
}
static void *new_zero(size_t bytes,size_t alignment)
{
    void *p=allocate(&memory,bytes);
    REQUIRE(p&&alignment&&(uintptr_t)p%alignment==0&&normal());
    memset(p,0,bytes);return p;
}
static struct pt_private_memory_stats memory_stats(void)
{
    struct pt_private_memory_stats s;
    REQUIRE(normal()&&pt_private_native_memory_stats(&memory,&s)==PT_PRIVATE_MEMORY_OK);
    REQUIRE(!s.failed);return s;
}
static void editor_init_at_final_address(void)
{
    struct pt_editor *e=state.editor;
    REQUIRE(normal()&&pt_editor_init(e,&state.document.project)&&normal());
    /* Only empty components are bound to the original checked allocator.
     * No live version, source, song, journal or controller is copied/reset. */
    REQUIRE(!e->sampler.bytes&&!e->sample_source.loaded&&
        !e->sample_source.allocated_bytes&&!e->song.current&&!e->song.bytes);
    pt_sampler_init(&e->sampler,&state.allocator,128UL*1024);
    pt_document_init(&e->sample_source,&state.allocator);
    pt_song_init(&e->song,&state.allocator,128UL*1024);
    REQUIRE(allocator_same(e->sampler.allocator,state.allocator)&&
        allocator_same(e->sample_source.allocator,state.allocator)&&
        allocator_same(e->song.allocator,state.allocator)&&
        e->sampler.budget==128UL*1024&&e->song.budget==128UL*1024&&
        !e->sampler.bytes&&!e->sample_source.loaded&&
        !e->sample_source.allocated_bytes&&!e->song.current&&!e->song.bytes&&normal());
    REQUIRE(pt_editor_change_barrier(e,change_ready,&state)&&normal());
}
static struct pt_midi_setup_owner_status owner_status(void)
{
    struct pt_midi_setup_owner_status s;
    REQUIRE(normal()&&pt_midi_setup_owner_get(state.owner,&s)==PT_MIDI_SETUP_OK&&normal());
    return s;
}
static void owner_tail(void)
{
    const unsigned char *p=(const unsigned char *)state.owner+pt_midi_setup_owner_size();unsigned i;
    for(i=0;i<TAIL;++i)REQUIRE(p[i]==0x7b);
}
static void saved_equal(void)
{
    size_t bytes,written;
    REQUIRE(normal()&&pt_project_size(&state.document.project,&bytes)==PT_PROJECT_OK&&bytes==state.encoded_bytes);
    REQUIRE(pt_project_encode(&state.document.project,state.scratch,bytes,&written)==PT_PROJECT_OK&&
        written==bytes&&!memcmp(state.scratch,state.encoded,bytes)&&normal());
}
static void modal_begin(enum pt_midi_setup_target target,unsigned track)
{
    unsigned calls=memory.allocations;
    REQUIRE(normal()&&pt_midi_requester_modal_begin(state.modal,state.owner,
        state.owner_capacity,state.editor,track)==PT_MIDI_SETUP_OK&&normal());
    REQUIRE(pt_midi_requester_model_select(&state.modal->model,target)==PT_MIDI_SETUP_OK&&normal());
    REQUIRE(memory.allocations==calls&&owner_status().modal==1);
    REQUIRE(pt_midi_setup_owner_close_controller(state.owner)==PT_MIDI_SETUP_BUSY&&
        pt_midi_setup_owner_release_workspace(state.owner)==PT_MIDI_SETUP_BUSY&&normal());
}
static void type_name(const char *name)
{
    size_t i;
    REQUIRE(pt_midi_requester_model_key(&state.modal->model,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
    for(i=0;i<PT_MIDI_ENDPOINT&&name[i];++i)
        REQUIRE(pt_midi_requester_model_key(&state.modal->model,
            PT_MIDI_KEY_CHARACTER,(unsigned char)name[i])==PT_MIDI_EVENT_CHANGED);
    REQUIRE(i<PT_MIDI_ENDPOINT&&normal());
}
static void modal_return(void)
{
    unsigned i;const unsigned char *p=(const unsigned char *)state.modal;
    REQUIRE(normal()&&pt_midi_requester_modal_return(state.modal)==PT_MIDI_SETUP_OK&&normal());
    for(i=0;i<sizeof(*state.modal);++i)REQUIRE(!p[i]);
    REQUIRE(!owner_status().modal);owner_tail();
}
static void commit_name(enum pt_midi_setup_target target,unsigned track,const char *name)
{
    modal_begin(target,track);type_name(name);
    REQUIRE(pt_midi_requester_model_apply(&state.modal->model)==PT_MIDI_SETUP_OK&&
        !state.modal->model.self&&normal());
    /* Accepted model consumption does not itself return the genuine ticket. */
    REQUIRE(owner_status().modal==1);modal_return();
}
static void dispose_and_close(void)
{
    struct pt_midi_setup_owner_status s;
    REQUIRE(normal()&&pt_editor_dispose(state.editor)&&normal());
    REQUIRE(!state.editor->history.count&&!state.editor->history.cursor&&
        !state.editor->sampler.bytes&&!state.editor->sample_source.loaded&&
        !state.editor->sample_source.allocated_bytes&&!state.editor->song.current&&!state.editor->song.bytes);
    REQUIRE(pt_midi_setup_owner_close_controller(state.owner)==PT_MIDI_SETUP_OK&&normal());
    s=owner_status();REQUIRE(s.state==PT_MIDI_OWNER_CLOSED&&!s.commands&&!s.metadata_bytes&&!s.modal);
    owner_tail();
}
int main(void)
{
    struct pt_private_memory_span guard;struct pt_private_memory_stats before,after;
    struct pt_midi_setup_owner_status status;size_t command_bytes,written;unsigned calls,releases,barriers;
    state.exec=SysBase;REQUIRE(state.exec&&(state.task=FindTask(NULL))&&state.exec->ThisTask==state.task);
    state.idnest=(int)(signed char)state.exec->IDNestCnt;
    state.tdnest=(int)(signed char)state.exec->TDNestCnt;
    REQUIRE(state.idnest==-1&&state.tdnest==-1&&normal());
    /* Static HUNK storage is observed, not relocated or forced into a class. */
    REQUIRE(fast_class(&state,sizeof(state))&&fast_class(&memory,sizeof(memory))&&normal());
    guard=(struct pt_private_memory_span){&state,sizeof(state)};
    REQUIRE(pt_private_native_memory_init(&memory,512UL*1024,&guard,1)==PT_PRIVATE_MEMORY_OK&&normal());
    state.allocator=(struct pt_allocator){&memory,allocate,release};
    pt_document_init(&state.document,&state.allocator);
    REQUIRE(allocator_same(state.document.allocator,state.allocator)&&normal());
    REQUIRE(pt_document_new(&state.document,4,128UL*1024)==PT_PROJECT_OK&&normal());
    state.editor=new_zero(sizeof(*state.editor),offsetof(struct {char c;struct pt_editor v;},v));
    REQUIRE(pt_midi_setup_owner_size()<=SIZE_MAX-TAIL);
    state.owner_capacity=pt_midi_setup_owner_size()+TAIL;
    state.owner=new_zero(state.owner_capacity,pt_midi_setup_owner_alignment());
    state.modal=new_zero(sizeof(*state.modal),offsetof(struct {char c;struct pt_midi_requester_modal v;},v));
    memset((unsigned char *)state.owner+pt_midi_setup_owner_size(),0x7b,TAIL);
    editor_init_at_final_address();command_bytes=pt_midi_setup_command_bytes();
    REQUIRE(command_bytes&&command_bytes<=SIZE_MAX/4);
    REQUIRE(pt_midi_setup_owner_init(state.owner,state.owner_capacity,state.editor,
        (struct pt_midi_setup_owner_span){&memory,sizeof(memory)},4*command_bytes)==PT_MIDI_SETUP_OK&&normal());
    status=owner_status();REQUIRE(status.state==PT_MIDI_OWNER_LIVE&&!status.commands&&
        !status.modal&&status.workspace_bytes==pt_midi_setup_size());owner_tail();state.phase=1;

    commit_name(PT_MIDI_SETUP_INPUT,0,"keys");
    commit_name(PT_MIDI_SETUP_OUTPUT,2,"synth");
    REQUIRE(!strcmp(state.document.project.midi_input,"keys")&&
        !strcmp(state.document.project.midi_output[2],"synth")&&
        state.editor->history.count==2&&state.editor->history.cursor==2);
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,-1)==PT_EDIT_OK&&normal());
    status=owner_status();REQUIRE(status.commands==2&&status.metadata_bytes==2*command_bytes&&
        state.editor->history.cursor==1&&!state.document.project.midi_output[2][0]);
    REQUIRE(pt_midi_setup_owner_close_controller(state.owner)==PT_MIDI_SETUP_IN_USE&&
        pt_midi_setup_owner_release_workspace(state.owner)==PT_MIDI_SETUP_IN_USE&&normal());
    REQUIRE(pt_project_size(&state.document.project,&state.encoded_bytes)==PT_PROJECT_OK&&state.encoded_bytes);
    state.encoded=new_zero(state.encoded_bytes,1);state.scratch=new_zero(state.encoded_bytes,1);
    REQUIRE(pt_project_encode(&state.document.project,state.encoded,state.encoded_bytes,&written)==PT_PROJECT_OK&&
        written==state.encoded_bytes&&normal());
    before=memory_stats();barriers=state.barriers;
    modal_begin(PT_MIDI_SETUP_INPUT,0);type_name("cancelled");modal_return();
    after=memory_stats();REQUIRE(before.allocations==after.allocations&&before.releases==after.releases&&
        before.live==after.live&&before.used==after.used&&state.barriers==barriers&&
        state.editor->history.count==2&&state.editor->history.cursor==1&&owner_status().commands==2);
    saved_equal();
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,1)==PT_EDIT_OK&&normal());
    REQUIRE(!strcmp(state.document.project.midi_output[2],"synth")&&owner_status().commands==2);
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,-1)==PT_EDIT_OK&&normal());
    saved_equal();state.phase=2;

    /* Actual journal disposal retires both committed and undone records before
     * positive controller close. It does not free the original workspace. */
    before=memory_stats();dispose_and_close();after=memory_stats();
    REQUIRE(after.releases==before.releases+2&&after.live+2==before.live);
    editor_init_at_final_address();calls=memory.allocations;releases=memory.releases;
    REQUIRE(pt_midi_setup_owner_rebind(state.owner,state.editor)==PT_MIDI_SETUP_OK&&normal());
    REQUIRE(memory.allocations==calls&&memory.releases==releases&&owner_status().state==PT_MIDI_OWNER_LIVE);
    modal_begin(PT_MIDI_SETUP_CLOCK,0);
    REQUIRE(pt_midi_requester_model_focus(&state.modal->model,PT_MIDI_FOCUS_SEND)&&
        pt_midi_requester_model_key(&state.modal->model,PT_MIDI_KEY_ACTIVATE,0)==PT_MIDI_EVENT_CHANGED);
    REQUIRE(pt_midi_requester_model_focus(&state.modal->model,PT_MIDI_FOCUS_RECEIVE)&&
        pt_midi_requester_model_key(&state.modal->model,PT_MIDI_KEY_ACTIVATE,0)==PT_MIDI_EVENT_CHANGED);
    REQUIRE(pt_midi_requester_model_apply(&state.modal->model)==PT_MIDI_SETUP_OK&&normal());
    modal_return();REQUIRE(state.document.project.midi_flags==3&&owner_status().commands==1);
    REQUIRE(pt_private_native_memory_seal(&memory)==PT_PRIVATE_MEMORY_OK&&normal());
    state.phase=3;dispose_and_close();
    REQUIRE(pt_midi_setup_owner_release_workspace(state.owner)==PT_MIDI_SETUP_OK&&normal());owner_tail();
    /* No callback retains model/owner/editor; all genuine controller records
     * and original workspace are now positively gone. End those exact blocks. */
    release(&memory,state.scratch);state.scratch=NULL;
    release(&memory,state.encoded);state.encoded=NULL;
    release(&memory,state.modal);state.modal=NULL;
    release(&memory,state.owner);state.owner=NULL;
    release(&memory,state.editor);state.editor=NULL;
    REQUIRE(pt_document_release_checked(&state.document)==PT_DOCUMENT_RELEASED&&normal());
    after=memory_stats();REQUIRE(!after.live&&!after.used&&!after.failed&&
        after.allocations==after.releases&&state.barriers>=3);
    REQUIRE(pt_private_native_memory_finish(&memory)==PT_PRIVATE_MEMORY_OK&&normal());
    state.phase=4;puts(PASS_MARKER);return 0;
}
