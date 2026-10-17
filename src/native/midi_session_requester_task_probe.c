/* Explicit ordinary-Task GUI experiment; not application enrollment or MIDI I/O.
 * Exact original owners stay stationary. Any unexpected result retains the
 * complete process/Task/context; the probe has no general recovery authority. */
#if !defined(PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY) || PT_MIDI_SESSION_REQUESTER_NATIVE_GUI_QUALIFICATION_ONLY != 1
#error "Select the exact MIDI session requester GUI Task qualification"
#endif
#if !defined(__m68k__) || !defined(__amigaos__)
#error "This experiment requires the genuine classic Amiga m68k target"
#endif
#include "midi_session_requester.h"
#include "../editor/sampler_lifetime_internal.h"
#include "../core/mod_project.h"
#include "native_checked_memory.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define PASS_MARKER "MIDI SESSION GUI TASK PASS:real parent/chooser/apply/cancel/refresh;master/save/undo;attached refusal;detach;owned close calls;zero private Fast ledgers;NO MIDI INPUT/CAMD/TIMING/OUTPUT"
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
/* Original separately placed GUI controls are HUNK/BSS storage, not allocator
 * context or additional private heap allocations. These and the caller stack
 * remain alive on every unexpected result. The exact original context stays
 * &memory; no domain/progress object is embedded in it. */
struct gui_progress_slot {
    struct pt_midi_session_requester_progress value;
    unsigned char tail[TAIL];
};
struct gui_state {
    struct pt_midi_session_requester_domain domain;
    struct gui_progress_slot progress[3];
    struct Screen *screen;struct Window *parent;
    struct MsgPort *port;struct RastPort *rport;struct TextFont *font;
    struct Message *pending;
    unsigned signal_bit,window_close_attempted,screen_close_attempted;
    unsigned final_get_calls,final_replied,refresh_active;
};
struct repaint_state {struct Window *parent;unsigned guard,phase,repaints;};
static struct gui_state gui;
static struct repaint_state repaint_state;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
#define PARENT_MASK (IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW)
static void gui_refused(void);


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
    gui_refused();master_unchanged();tails();
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
/* Genuine private screen/parent only. No shared port, parent dispatcher,
 * timer/device, fabricated message or application display cache is installed. */
static int gui_normal(void)
{
    return normal() && IntuitionBase && GfxBase &&
        ((struct Library *)IntuitionBase)->lib_Version>=37 &&
        ((struct Library *)GfxBase)->lib_Version>=37;
}
static int parent_current(ULONG mask)
{
    return gui_normal() && gui.parent && gui.screen && gui.port && gui.rport &&
        gui.parent->UserPort==gui.port && gui.parent->WScreen==gui.screen &&
        gui.parent->RPort==gui.rport && gui.parent->IDCMPFlags==mask &&
        gui.port->mp_SigTask==state.task &&
        (unsigned)(unsigned char)gui.port->mp_SigBit==gui.signal_bit;
}
static void parent_draw(struct Window *w)
{
    static const char *const instructions[4]={
        "GUI 1: click Cancel in the real chooser.",
        "GUI 2: Input, Ctrl-U, type Native Session GUI, click Apply.",
        "GUI 3: drag child; Input, edit draft, click Cancel.",
        "GUI complete: release every key/button; preparing owned close."};
    const char *text;
    REQUIRE(gui_normal() && w==gui.parent && w->RPort==gui.rport &&
        repaint_state.parent==w && repaint_state.phase<4);
    text=instructions[repaint_state.phase];
    SetAPen(w->RPort,0);RectFill(w->RPort,0,0,639,511);
    SetAPen(w->RPort,1);SetBPen(w->RPort,0);SetDrMd(w->RPort,JAM2);
    Move(w->RPort,12,24);Text(w->RPort,(STRPTR)text,(ULONG)strlen(text));
    Move(w->RPort,12,44);Text(w->RPort,(STRPTR)"Saved metadata only; no MIDI device or timing authority.",
        (ULONG)strlen("Saved metadata only; no MIDI device or timing authority."));
}
static void repaint(struct Window *w,void *context)
{
    REQUIRE(context==&repaint_state && repaint_state.guard && w==gui.parent);
    parent_draw(w);++repaint_state.repaints;
}
static void gui_tails(void)
{
    unsigned i,j;
    for(i=0;i<3;++i)for(j=0;j<TAIL;++j)
        REQUIRE(gui.progress[i].tail[j]==(unsigned char)(0xb1+i));
}
static int progress_zero(const struct pt_midi_session_requester_progress *p)
{
    const unsigned char *b=(const unsigned char *)p;size_t i;
    for(i=0;i<sizeof(*p);++i)if(b[i])return 0;
    return 1;
}
static void gui_open(void)
{
    static const UWORD colours[16]={0x000,0xfff,0x555,0xff0,0x333,0x777,0xaaa,0xccc,
        0x00f,0x0f0,0xf00,0x0ff,0xf0f,0x880,0x088,0x808};
    struct TextAttr attr={(STRPTR)"topaz.font",8,0,0};unsigned i;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library",37);
    GfxBase=(struct GfxBase *)OpenLibrary((STRPTR)"graphics.library",37);
    REQUIRE(gui_normal());
    gui.font=OpenFont(&attr);REQUIRE(gui.font && gui.font->tf_XSize==8 && gui.font->tf_YSize==8);
    gui.screen=OpenScreenTags(NULL,SA_Width,640,SA_Height,512,SA_Depth,4,
        SA_DisplayID,PAL_MONITOR_ID|HIRESLACE_KEY,SA_Type,CUSTOMSCREEN,
        SA_Quiet,TRUE,SA_ShowTitle,FALSE,SA_Title,(ULONG)"MIDI session GUI qualification",TAG_DONE);
    REQUIRE(gui.screen && gui_normal());LoadRGB4(&gui.screen->ViewPort,(UWORD *)colours,16);
    gui.parent=OpenWindowTags(NULL,WA_CustomScreen,(ULONG)gui.screen,WA_Left,0,WA_Top,0,
        WA_Width,640,WA_Height,512,WA_Borderless,TRUE,WA_Backdrop,TRUE,WA_Activate,TRUE,
        WA_RMBTrap,TRUE,WA_SimpleRefresh,TRUE,WA_IDCMP,PARENT_MASK,TAG_DONE);
    REQUIRE(gui.parent && gui.parent->UserPort && gui.parent->RPort && gui_normal());
    gui.port=gui.parent->UserPort;gui.rport=gui.parent->RPort;
    gui.signal_bit=(unsigned)(unsigned char)gui.port->mp_SigBit;
    REQUIRE(gui.signal_bit<32 && parent_current(PARENT_MASK));
    SetFont(gui.rport,gui.font);repaint_state.parent=gui.parent;
    gui.domain=(struct pt_midi_session_requester_domain){
        state.session,state.session_capacity,state.frontend,state.frontend_capacity,
        state.owner,state.owner_capacity,state.take,state.take_capacity,state.editor,
        {&memory,sizeof(memory)}};
    for(i=0;i<3;++i)memset(gui.progress[i].tail,(int)(0xb1+i),TAIL);
    parent_draw(gui.parent);WaitBlit();gui_tails();
}
static enum pt_midi_session_requester_result call_gui(unsigned which)
{
    REQUIRE(which<3 && parent_current(PARENT_MASK) && repaint_state.guard);
    return pt_native_midi_session_requester(&gui.progress[which].value,sizeof(gui.progress[which]),
        &gui.domain,gui.parent,sizeof(*gui.parent),TARGET,repaint,&repaint_state,sizeof(repaint_state));
}
static void gui_refused(void)
{
    struct pt_private_memory_stats a=stats(),b;
    REQUIRE(parent_current(PARENT_MASK) && !repaint_state.guard && progress_zero(&gui.progress[0].value));
    repaint_state.guard=1;
    REQUIRE(call_gui(0)==PT_MIDI_SESSION_REQUESTER_REFUSED);
    REQUIRE(progress_zero(&gui.progress[0].value) && parent_current(PARENT_MASK) &&
        pt_midi_frontend_context_state(state.frontend)==PT_MIDI_FRONTEND_LIVE);
    b=stats();REQUIRE(a.allocations==b.allocations && a.releases==b.releases && a.live==b.live && a.used==b.used);
    repaint_state.guard=0;gui_tails();
}
static void real_modal(unsigned which,enum pt_midi_requester_result expected)
{
    struct pt_midi_session_requester_progress *p;
    REQUIRE(which<3);p=&gui.progress[which].value;
    REQUIRE(!repaint_state.guard && parent_current(PARENT_MASK) && progress_zero(p));
    repaint_state.phase=which;parent_draw(gui.parent);WaitBlit();repaint_state.guard=1;
    REQUIRE(call_gui(which)==PT_MIDI_SESSION_REQUESTER_COMPLETE && gui_normal());
    /* These are actual public progress/state witnesses, not independent native
     * counters proving the number of lower API invocations. */
    REQUIRE(p->state==PT_MIDI_SESSION_REQUESTER_COMPLETE && !p->busy &&
        p->begin_result==PT_MIDI_SETUP_OK && p->end_result==PT_MIDI_SETUP_OK &&
        p->parent_progress.state==PT_MIDI_PARENT_RESTORED &&
        p->parent_progress.window_result==expected &&
        !p->parent_progress.pending_message && !p->parent_progress.refresh_active &&
        parent_current(PARENT_MASK) && pt_midi_recording_session_state(state.session)==PT_MIDI_SESSION_LIVE);
    REQUIRE(pt_native_midi_requester_parent_restored(&p->parent_progress));
    parent_draw(gui.parent);WaitBlit();repaint_state.guard=0;
    master_unchanged();tails();gui_tails();
}
static void gui_settings(void)
{
    const char expected[]="Native Session GUI";
    size_t count=state.editor->history.count,cursor=state.editor->history.cursor;
    uint32_t revision=state.editor->history.revision;unsigned calls=memory.allocations;
    encode(state.before);real_modal(0,PT_MIDI_REQUESTER_CANCELLED);encode(state.scratch);
    REQUIRE(!memcmp(state.before,state.scratch,state.encoded_bytes) &&
        state.editor->history.count==count && state.editor->history.cursor==cursor &&
        state.editor->history.revision==revision && memory.allocations==calls);
    real_modal(1,PT_MIDI_REQUESTER_ACCEPTED);
    REQUIRE(!strcmp(state.document.project.midi_input,expected) &&
        state.editor->history.count==count+1 && state.editor->history.cursor==cursor+1 &&
        memory.allocations==calls+1 && stats().live==MAX_LIVE);
    encode(state.recorded);pt_editor_saved(state.editor);revision=state.editor->history.revision;
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,-1)==PT_EDIT_OK);
    encode(state.scratch);REQUIRE(!memcmp(state.before,state.scratch,state.encoded_bytes));
    count=state.editor->history.count;cursor=state.editor->history.cursor;
    {uint32_t undone=state.editor->history.revision;
        real_modal(2,PT_MIDI_REQUESTER_CANCELLED);encode(state.scratch);
        REQUIRE(!memcmp(state.before,state.scratch,state.encoded_bytes) &&
            state.editor->history.count==count && state.editor->history.cursor==cursor &&
            state.editor->history.revision==undone && memory.allocations==calls+1);
    }
    REQUIRE(gui.progress[2].value.parent_progress.repainted[1]>0);
    REQUIRE(pt_pattern_undo(&state.document.project,&state.editor->history,1)==PT_EDIT_OK);
    encode(state.scratch);REQUIRE(!memcmp(state.recorded,state.scratch,state.encoded_bytes) &&
        state.editor->history.revision==revision && !pt_editor_dirty(state.editor));
    master_unchanged();tails();gui_tails();
}
/* Auto-created own IDCMP port only. Root-supplied primary Intuition contract:
 * consuming CloseWindow frees this port and pending messages. Drain/reply first;
 * never GetMsg/ReplyMsg/dereference this port/window after the consuming call. */
static void gui_close(void)
{
    unsigned i;struct Message *raw;
    REQUIRE(!repaint_state.guard && parent_current(PARENT_MASK) &&
        pt_midi_recording_session_state(state.session)==PT_MIDI_SESSION_LIVE);
    for(i=0;i<3;++i)REQUIRE(gui.progress[i].value.state==PT_MIDI_SESSION_REQUESTER_COMPLETE);
    repaint_state.phase=3;parent_draw(gui.parent);WaitBlit();repaint_state.guard=1;
    REQUIRE(ModifyIDCMP(gui.parent,IDCMP_REFRESHWINDOW) && parent_current(IDCMP_REFRESHWINDOW));
    for(i=0;i<PT_MIDI_PARENT_DRAIN_LIMIT;++i){
        struct IntuiMessage *message;ULONG kind;
        REQUIRE(parent_current(IDCMP_REFRESHWINDOW));++gui.final_get_calls;
        raw=GetMsg(gui.port);gui.pending=raw;
        REQUIRE(parent_current(IDCMP_REFRESHWINDOW));
        if(!raw)break;
        REQUIRE(raw->mn_Node.ln_Type==NT_MESSAGE && raw->mn_Length>=sizeof(*message) && raw->mn_ReplyPort);
        message=(struct IntuiMessage *)raw;REQUIRE(message->IDCMPWindow==gui.parent);kind=message->Class;
        ReplyMsg(raw);gui.pending=NULL;++gui.final_replied;
        REQUIRE(parent_current(IDCMP_REFRESHWINDOW));
        if(kind==IDCMP_REFRESHWINDOW){
            gui.refresh_active=1;BeginRefresh(gui.parent);REQUIRE(parent_current(IDCMP_REFRESHWINDOW));
            repaint(gui.parent,&repaint_state);REQUIRE(parent_current(IDCMP_REFRESHWINDOW));
            EndRefresh(gui.parent,TRUE);gui.refresh_active=0;REQUIRE(parent_current(IDCMP_REFRESHWINDOW));
        }
    }
    REQUIRE(i<PT_MIDI_PARENT_DRAIN_LIMIT && !gui.pending && !gui.refresh_active &&
        gui.screen->FirstWindow==gui.parent && !gui.parent->NextWindow && parent_current(IDCMP_REFRESHWINDOW));
    WaitBlit();gui.window_close_attempted=1;CloseWindow(gui.parent);
    gui.parent=NULL;gui.port=NULL;gui.rport=NULL;repaint_state.parent=NULL;
    /* Screen is still live here; the consumed parent/port is never followed. */
    REQUIRE(gui_normal() && !gui.screen->FirstWindow);
    /* Genuine classic prototype is BOOL CloseScreen(struct Screen *).
     * FALSE leaves this original screen/font/libraries/Task retained. No retry. */
    gui.screen_close_attempted=1;REQUIRE(CloseScreen(gui.screen));gui.screen=NULL;
    CloseFont(gui.font);gui.font=NULL;
    CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;
    CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;
    /* Nulls record issued consuming calls, not OS deallocation receipts. */
    repaint_state.guard=0;gui_tails();REQUIRE(normal());
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
    struct pt_private_memory_span guards[3];
    state.exec=SysBase;
    REQUIRE(state.exec && (state.task=FindTask(NULL)) && state.exec->ThisTask==state.task);
    state.idnest=(int)(signed char)state.exec->IDNestCnt;state.tdnest=(int)(signed char)state.exec->TDNestCnt;
    REQUIRE(state.idnest==-1 && state.tdnest==-1 && normal() && stack_headroom());
    REQUIRE(fast(&state,sizeof(state)) && fast(&memory,sizeof(memory)) && normal());plan();
    REQUIRE(fast(&gui,sizeof(gui)) && fast(&repaint_state,sizeof(repaint_state)));
    guards[0]=(struct pt_private_memory_span){&state,sizeof(state)};
    guards[1]=(struct pt_private_memory_span){&gui,sizeof(gui)};
    guards[2]=(struct pt_private_memory_span){&repaint_state,sizeof(repaint_state)};
    REQUIRE(pt_private_native_memory_init(&memory,state.byte_ceiling,guards,3)==PT_PRIVATE_MEMORY_OK && normal());
    state.allocator=(struct pt_allocator){&memory,allocate,release};initialize();
    gui_open();ordinary_take();failed_stop_local_cancel();gui_settings();gui_close();finish();puts(PASS_MARKER);return 0;
}
