/* PRIVATE SOURCE PROPOSAL: real acquisition/retirement primitive boundaries.
 * Based on the existing opt-in native_cia_ram_adapter protocol. No vendor input
 * is edited. No call, build, host test or native qualification has been run.
 */
#include "native_replay_cia_stop.h"
#ifndef PT_REPLAY_STOP_STUB
#if !defined(__m68k__) || !defined(__amigaos__)
#error "Native primitive requires the real Amiga compiler; model requires explicit STUB"
#endif
typedef char exact_native_geometry[(CHAR_BIT==8 && sizeof(void *)==4 && sizeof(uintptr_t)==4 &&
    sizeof(unsigned)==4 && sizeof(UBYTE)==1 && sizeof(WORD)==2)?1:-1];
#endif

#ifndef PT_REPLAY_STOP_HARDWARE
#define PT_REPLAY_STOP_HARDWARE(chip) ((volatile struct CIA *)(chip?0xbfe001UL:0xbfd000UL))
#endif
#ifndef PT_REPLAY_STOP_CONTROL_WRITE
#define PT_REPLAY_STOP_CONTROL_WRITE(control,value) (*(control)=(value))
#endif
enum cia_stop_phase {CIA_STOP_NEW,CIA_STOP_OWNED,CIA_STOP_RETIRING,
    CIA_STOP_RETIRED,CIA_STOP_HELD};
static struct {
    struct ExecBase *exec;
    struct Task *task;
    uintptr_t stack_low,stack_high;
    struct Library *resource;
    struct Interrupt *server,server_snapshot;
    volatile struct CIA *hardware;
    volatile UBYTE *control;
    unsigned bit,chip,add_issued,remove_issued;
    WORD mask_before,mask_after,pending_after;
    enum cia_stop_phase phase;
} cia_owner;

/* These aliases must be added by the PRIVATE generated ABI fragment. They
 * name actual original replay state; no getter fabricates a quiet flag. */
extern volatile UBYTE pt_replay_enabled;
extern struct Library * volatile pt_replay_cia_base;

static uintptr_t actual_sp(void)
{
#ifdef PT_REPLAY_STOP_STUB
    return pt_replay_stop_stub_sp;
#else
    uintptr_t value;
    __asm__ volatile ("move.l %%sp,%0" : "=d" (value) : : "memory");
    return value;
#endif
}
static int ordinary(unsigned disabled)
{
    uintptr_t sp=actual_sp();
    return cia_owner.exec && SysBase==cia_owner.exec && cia_owner.task &&
        SysBase->ThisTask==cia_owner.task &&
        cia_owner.task->tc_Node.ln_Type==NT_PROCESS &&
        (int)(signed char)SysBase->IDNestCnt==(disabled?0:-1) &&
        (int)(signed char)SysBase->TDNestCnt==-1 &&
        (uintptr_t)cia_owner.task->tc_SPLower==cia_owner.stack_low &&
        (uintptr_t)cia_owner.task->tc_SPUpper==cia_owner.stack_high &&
        cia_owner.stack_low && cia_owner.stack_low<cia_owner.stack_high &&
        sp>=cia_owner.stack_low && sp<cia_owner.stack_high && !(sp&1u);
}
static int current_locked(void)
{
    return ordinary(1) && cia_owner.resource && cia_owner.server &&
        cia_owner.resource->lib_Version>=36 && cia_owner.bit<2 && cia_owner.chip<2 &&
        cia_owner.hardware==PT_REPLAY_STOP_HARDWARE(cia_owner.chip) &&
        cia_owner.control==(cia_owner.bit?&cia_owner.hardware->ciacrb:&cia_owner.hardware->ciacra) &&
        cia_owner.server->is_Code==cia_owner.server_snapshot.is_Code &&
        cia_owner.server->is_Data==cia_owner.server_snapshot.is_Data;
}
static void hold_original(void) __attribute__((noreturn));
static void hold_original(void)
{
    cia_owner.phase=CIA_STOP_HELD;
    /* A genuine successful add cannot be undone or returned as a refusal when
     * subsequent custody is uncertain. Retain the original Process/HUNK/stack.
     * No automatic signal, forced cleanup, retry, unwind or Exit. */
    for(;;) {
        if(ordinary(0))Wait(0);
        __asm__ volatile ("" ::: "memory");
    }
}

uintptr_t pt_native_replay_add_cia_vector(struct Library *cia_resource,
    unsigned bit,struct Interrupt *server,volatile struct CIA *hardware)
{
    struct Interrupt *previous;
    struct Task *task;
    unsigned chip;
    /* Non-NULL sentinel follows the original AddICRVector refusal branch.
     * Its exact generated call-site branch must be checked before integration. */
    uintptr_t refused=~(uintptr_t)0;
    if(cia_owner.phase!=CIA_STOP_NEW || cia_owner.add_issued || !SysBase ||
       SysBase->LibNode.lib_Version<36 ||
       (int)(signed char)SysBase->IDNestCnt!=-1 ||
       (int)(signed char)SysBase->TDNestCnt!=-1 ||
       !cia_resource || cia_resource->lib_Version<36 || !server || !server->is_Code ||
       pt_replay_enabled || bit>=2)return refused;
    if(hardware==PT_REPLAY_STOP_HARDWARE(0))chip=0;
    else if(hardware==PT_REPLAY_STOP_HARDWARE(1))chip=1;
    else return refused;
    if(OpenResource(chip?(CONST_STRPTR)"ciaa.resource":(CONST_STRPTR)"ciab.resource")!=cia_resource)return refused;
    task=FindTask(NULL);
    if(!task || task->tc_Node.ln_Type!=NT_PROCESS)return refused;
    cia_owner.exec=SysBase;cia_owner.task=task;
    cia_owner.stack_low=(uintptr_t)task->tc_SPLower;
    cia_owner.stack_high=(uintptr_t)task->tc_SPUpper;
    if(!ordinary(0))return refused;
    Disable();
    if(!ordinary(1) || pt_replay_enabled ||
       (*(bit?&hardware->ciacrb:&hardware->ciacra)&1u)) {
        Enable();return refused; /* occupied/running timer is never modified */
    }
    cia_owner.add_issued=1;
    previous=AddICRVector(cia_resource,bit,server);
    if(previous) {
        /* Actual failed acquisition grants no vector ownership. This one-shot
         * proposal deliberately does not retry another chip/timer. */
        Enable();return (uintptr_t)previous;
    }
    cia_owner.resource=cia_resource;cia_owner.server=server;
    cia_owner.server_snapshot=*server;cia_owner.hardware=hardware;
    cia_owner.chip=chip;cia_owner.bit=bit;
    cia_owner.control=bit?&hardware->ciacrb:&hardware->ciacra;
    cia_owner.phase=CIA_STOP_OWNED;
    if(!current_locked()) {Enable();hold_original();}
    Enable();return 0;
}

int pt_native_replay_cia_retire(void)
{
    struct Library *cia_resource=cia_owner.resource;
    unsigned own;
    int result=-1;
    if(cia_owner.phase==CIA_STOP_RETIRED) {
        if(ordinary(0))return 1;
        cia_owner.phase=CIA_STOP_HELD;return -1;
    }
    if(cia_owner.phase==CIA_STOP_RETIRING) {cia_owner.phase=CIA_STOP_HELD;return -1;}
    if(cia_owner.phase!=CIA_STOP_OWNED)return -1;
    if(!ordinary(0)) {cia_owner.phase=CIA_STOP_HELD;return -1;}
    Disable();
    if(!current_locked() || pt_replay_cia_base!=cia_resource)goto held;
    cia_owner.phase=CIA_STOP_RETIRING;own=1u<<cia_owner.bit;
    /* Actual normal task resumption is after any earlier maskable ISR returns.
     * This exclusion remains uninterrupted until the own vector is detached.
     * No raw CIA ICR or shared level2/6 interrupt acknowledgement is used. */
    pt_replay_enabled=0;
    cia_owner.mask_before=AbleICR(cia_resource,0);
    AbleICR(cia_resource,(WORD)own);              /* disable own bit only */
    PT_REPLAY_STOP_CONTROL_WRITE(cia_owner.control,(UBYTE)(*cia_owner.control&~1u));
    if(*cia_owner.control&1u)goto held;
    SetICR(cia_resource,(WORD)own);              /* clear own pending bit */
    cia_owner.mask_after=AbleICR(cia_resource,0);
    cia_owner.pending_after=SetICR(cia_resource,0); /* OS2+ status query */
    if(cia_owner.phase!=CIA_STOP_RETIRING || pt_replay_enabled ||
       ((unsigned)(UWORD)cia_owner.mask_after&own) ||
       ((unsigned)(UWORD)cia_owner.pending_after&own) ||
       (((unsigned)(UWORD)cia_owner.mask_before^(unsigned)(UWORD)cia_owner.mask_after)&~own&0x1fu))goto held;
    /* Positive deallocation is this exact resource/bit/server RemICRVector API
     * completion, together with the still-held exclusion and pre/post samples.
     * It is not an arbitrary boolean after the old void replay-stop function.
     * The API's synchronous removal semantics must be pinned/qualified. */
    if(cia_owner.remove_issued)goto held;
    cia_owner.remove_issued=1;
    RemICRVector(cia_resource,cia_owner.bit,cia_owner.server);
    cia_owner.mask_after=AbleICR(cia_resource,0);
    cia_owner.pending_after=SetICR(cia_resource,0);
    if(cia_owner.phase!=CIA_STOP_RETIRING || !current_locked() ||
       pt_replay_enabled || (*cia_owner.control&1u) ||
       ((unsigned)(UWORD)cia_owner.mask_after&own) ||
       ((unsigned)(UWORD)cia_owner.pending_after&own) ||
       (((unsigned)(UWORD)cia_owner.mask_before^(unsigned)(UWORD)cia_owner.mask_after)&~own&0x1fu))goto held;
    /* Clear original alias only AFTER genuine removal and positive samples. */
    pt_replay_cia_base=NULL;
    cia_owner.phase=CIA_STOP_RETIRED;result=1;goto done;
held:cia_owner.phase=CIA_STOP_HELD;
done:Enable();return result;
}
