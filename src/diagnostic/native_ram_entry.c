/* RAM-only diagnostic entry. Portable builds and software models do not qualify
 * native execution, launch or timing.
 * This entry never touches Paula DMA, AmiGUS, audio.device or card sample RAM.
 * CIA timer ownership is the separately reviewed adapter's only register work.
 * Guard descriptors protect stable extents; controlled bytes follow their own
 * serialized owner APIs. Independent sample bytes remain immutable.
 */
#include "../native/readers_ram/native_ram_irq_layout.h"
#include "../native/readers_ram/native_cia_ram_adapter.h"
#include "../native/native_checked_memory.h"
#include "document.h"
#include "eclock_alarm.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <limits.h>
#include <string.h>

#define ENTRY_GENERATION UINT64_C(7)
#define ENTRY_FRAME UINT64_C(4096)
#define ENTRY_RATE 48000U
#define ENTRY_SAMPLE_BYTES 128U
#define ENTRY_HOLDERS 5U
#define ENTRY_EARLY 128U
#define ENTRY_RESIDENCY 256U
#define ENTRY_READS 64U
/* Diagnostic admission policy only; not a total task/IRQ stack bound. */
#define ENTRY_TASK_MIN_HEADROOM 32768U
#define ENTRY_TASK_STACK_BOOT 1U
#define ENTRY_TASK_STACK_BEFORE_SOURCE 2U
#define ENTRY_TASK_STACK_SOURCE_CLOSED 4U
#define ENTRY_TASK_STACK_TEARDOWN 8U
#define ENTRY_TASK_STACK_SAMPLE_LIMIT 4U

struct entry_watch {
    struct pt_private_native_memory *memory;
    unsigned current,terminal,released,failed;
    unsigned live[ENTRY_HOLDERS];
};
struct entry_holder {
    struct entry_watch *watch;
    uint8_t *sample;
    uint64_t token;
    unsigned index,terminal;
};
struct entry_scratch {
    struct pt_scheduled_batch batch;
    struct pt_readers_control command;
    struct pt_readers_owner reader[4];
    struct pt_scheduled_span span[4];
};
struct entry_state {
    struct entry_watch *watch;
    struct pt_private_ram_port *port;
    struct pt_private_ram_irq *irq;
    struct pt_private_cia_ram_adapter *cia;
    struct entry_scratch *scratch;
    struct entry_holder *holder[ENTRY_HOLDERS];
    struct pt_allocator allocator;
    struct pt_scheduled_grid grid;
    struct pt_readers_activation_port port_api;
    struct pt_readers_backend backend;
    struct pt_readers_activation *ledger;
    struct pt_readers_output *queue;
    struct pt_elapsed_clock clock_grid;
    struct pt_readers_reader_receipt receipt;
    struct pt_readers_command_receipt command_receipt;
    struct pt_readers_key key[4];
    uint64_t session,ticket,first,last,termination_deadline,observed,issued;
    unsigned transferred,acquired,publish_attempted,success,activation_current;
};
/* The control allocation is separate: this object contains only its pointer.
 * No encompassing guard overlaps the checked-memory control itself.
 * Normal-task/stack/context identity must remain original through the hold.
 */
struct entry_bootstrap {
    struct Task *task;
    struct ExecBase *exec;
    void *system_lower,*system_upper;
    void *stack_lower,*stack_upper; /* raw upper retained for Task identity */
    void *stack_usable_upper;
    uintptr_t task_entry_sp,task_sampled_low_sp;
    unsigned task_stack_status,task_stack_samples,task_stack_phases;
    ULONG original_signals,completion_mask,termination_mask,clock_mask;
    BYTE priority;
    LONG completion_signal;
    struct pt_native_eclock clock;
    struct pt_native_alarm termination;
    struct pt_private_native_memory *memory;
    void *original_control;
    size_t original_control_bytes;
    struct pt_private_memory_span guard[PT_PRIVATE_MEMORY_GUARDS];
    unsigned memory_initialized,source_closed,finished;
    unsigned clock_signal_owned,termination_signal_owned,completion_signal_owned;
};
static struct entry_bootstrap bootstrap;
struct entry_control_alignment {char byte;struct pt_private_native_memory value;};
typedef char entry_stack_guard_capacity[(PT_PRIVATE_MEMORY_GUARDS>=9)?1:-1];

static int entry_span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int entry_apart(const void *a,size_t n,const void *b,size_t m)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return entry_span(a,n)&&entry_span(b,m)&&(!n||!m||(x>=y?x-y>=m:y-x>=n));
}
static int entry_fast(const void *p,size_t n)
{
    ULONG first,last;
    if(!n||!entry_span(p,n))return 0;
    first=TypeOfMem((APTR)p);last=TypeOfMem((APTR)((uintptr_t)p+n-1));
    return (first&MEMF_FAST)&&!(first&MEMF_CHIP)&&(last&MEMF_FAST)&&!(last&MEMF_CHIP);
}
static void entry_write(const char *text)
{Write(Output(),(APTR)text,(LONG)strlen(text));}
static void entry_hex(uint64_t value)
{
    char b[17];unsigned i;
    for(i=0;i<16;++i){unsigned digit=(unsigned)(value&15U);b[15-i]=(char)(digit<10?'0'+digit:'a'+digit-10);value>>=4;}
    b[16]=0;entry_write(b);
}
static int entry_task_current(void);
static void entry_hold(const char *reason) __attribute__((noreturn));
static void entry_hold(const char *reason)
{
    /* Every caller has restored its own outer Enable. No close retry, owner
     * callback, forced release, Exit or return occurs from this passive hold.
     * The separately qualified launcher MUST NOT kill/unload on timeout.
     */
    /* Lost task provenance also forbids a mutating allocator API. The hold
     * then preserves the whole world without any further allocator operation.
     */
    if(bootstrap.memory_initialized&&!bootstrap.finished&&entry_task_current())
        pt_private_native_memory_retain(bootstrap.memory);
    entry_write("NATIVE RAM ENTRY HOLD: ");entry_write(reason);
    entry_write("; complete original Process/HUNK/stack/libraries/contexts retained\n");
    for(;;)Delay(50);
    __builtin_unreachable();
}
static int entry_task_current(void)
{
    return bootstrap.exec&&SysBase==bootstrap.exec&&
        SysBase->SysStkLower==bootstrap.system_lower&&SysBase->SysStkUpper==bootstrap.system_upper&&
        bootstrap.task&&FindTask(NULL)==bootstrap.task&&
        bootstrap.task->tc_Node.ln_Pri==bootstrap.priority&&
        bootstrap.task->tc_SPLower==bootstrap.stack_lower&&
        bootstrap.task->tc_SPUpper==bootstrap.stack_upper;
}
static void entry_require_task(const char *reason)
{if(!entry_task_current())entry_hold(reason);}
static int entry_memory_good(void)
{return bootstrap.memory_initialized&&!bootstrap.memory->failed&&!bootstrap.memory->busy;}
static int entry_session(const char *text,uint64_t *out)
{
    uint64_t n=0;unsigned i=0;
    if(!text||!out)return 0;
    for(;text[i];++i){unsigned digit;char c=text[i];
        if(i==16)return 0;
        if(c>='0'&&c<='9')digit=(unsigned)(c-'0');
        else if(c>='a'&&c<='f')digit=(unsigned)(c-'a'+10);
        else if(c>='A'&&c<='F')digit=(unsigned)(c-'A'+10);
        else return 0;
        n=(n<<4)|digit;
    }
    if(!i||!n)return 0;
    *out=n;return 1;
}
static int entry_key_same(const struct pt_readers_key *a,const struct pt_readers_key *b)
{
    return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&
        a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;
}
static int entry_source_same(const struct entry_holder *h)
{
    unsigned i;
    if(!h||h->index<1||h->index>4||!h->sample)return 0;
    for(i=0;i<ENTRY_SAMPLE_BYTES;++i)
        if(h->sample[i]!=(uint8_t)((int)(h->index-1)*17+(int)i-64))return 0;
    return 1;
}
static int entry_current(void *context,uint64_t token,uint64_t generation)
{
    struct entry_holder *h=context;
    if(!entry_task_current()||!h||!h->watch)return 0;
    ++h->watch->current;
    return h->index<ENTRY_HOLDERS&&h->watch->live[h->index]==1&&!h->watch->failed&&
        !h->terminal&&token==h->token&&generation==ENTRY_GENERATION&&
        (!h->index||entry_source_same(h));
}
static void entry_terminal(void *context,uint64_t token,int valid)
{
    struct entry_holder *h=context;
    entry_require_task("original task changed before holder terminal callback");
    if(!h||!h->watch)return;
    if(h->index>=ENTRY_HOLDERS||token!=h->token||h->terminal||!h->watch->live[h->index]){
        h->watch->failed=1;return;
    }
    h->terminal=1;++h->watch->terminal;
    if(!valid)h->watch->failed=1;
}
static void entry_release(void *context,uint64_t token)
{
    struct entry_holder *h=context;struct entry_watch *watch;struct pt_private_native_memory *memory;
    uint8_t *sample;unsigned index;
    entry_require_task("original task changed before holder release callback");
    if(!h||!h->watch)return;
    watch=h->watch;memory=watch->memory;index=h->index;sample=h->sample;
    if(index>=ENTRY_HOLDERS||token!=h->token||!h->terminal||watch->live[index]!=1){watch->failed=1;return;}
    /* Source and holder releases are permitted only by the calling entry's
     * actual unexposed-local or independently exact domain protocol. The
     * callback never interprets ACTIVE, source-close or timing flags itself.
     */
    if(sample){int result=pt_private_native_release_checked(memory,sample);
        entry_require_task("original task changed during sample release");
        if(result!=PT_PRIVATE_MEMORY_OK){watch->failed=1;return;}
    }
    h->sample=NULL;watch->live[index]=0;++watch->released;
    {int result=pt_private_native_release_checked(memory,h);
        entry_require_task("original task changed during holder release");
        if(result!=PT_PRIVATE_MEMORY_OK)watch->failed=1;
    }
}
static struct pt_readers_control entry_control(struct entry_holder *h)
{return (struct pt_readers_control){h,sizeof(*h),h->token,entry_current,entry_release,entry_terminal};}
static void *entry_allocate(size_t bytes)
{
    void *p;
    entry_require_task("original task changed before checked allocation");
    p=pt_private_native_allocate(bootstrap.memory,bytes);
    entry_require_task("original task changed during checked allocation");
    if(!entry_memory_good())entry_hold("checked allocation ambiguity");
    if(p)memset(p,0,bytes);
    return p;
}
static int entry_drop(void *p)
{
    int result;
    entry_require_task("original task changed before checked cleanup");
    if(!p)return 1;
    result=pt_private_native_release_checked(bootstrap.memory,p);
    entry_require_task("original task changed during checked cleanup");
    return result==PT_PRIVATE_MEMORY_OK;
}
static void *entry_allocator_allocate(void *context,size_t bytes)
{
    void *p;
    entry_require_task("original task changed before core allocator callback");
    if(context!=bootstrap.memory)entry_hold("core allocator context changed");
    p=pt_private_native_allocate(context,bytes);
    entry_require_task("original task changed during core allocator callback");
    return p;
}
static void entry_allocator_release(void *context,void *p)
{
    int result;
    entry_require_task("original task changed before core release callback");
    if(context!=bootstrap.memory)entry_hold("core release context changed");
    result=pt_private_native_release_checked(context,p);
    entry_require_task("original task changed during core release callback");
    if(result!=PT_PRIVATE_MEMORY_OK)entry_hold("core checked release refused");
}

/* Only the forced host-model build injects a synthetic pointer. Native code
 * observes the running m68k SP; tc_SPReg and addresses of locals are not SP.
 * CRT/main/getter frames already exist, so this cannot rescue a bad launcher.
 */
static uintptr_t entry_live_sp(void)
{
#ifdef PT_PRIVATE_NATIVE_ENTRY_HOST_MODEL
    return entry_model_live_sp();
#else
    uintptr_t pointer;
    __asm__ volatile ("move.l %%sp,%0" : "=r" (pointer) : : "memory");
    return pointer;
#endif
}
static int entry_task_stack_admitted(uintptr_t lower,uintptr_t raw_upper,uintptr_t pointer)
{
    uintptr_t upper;
    /* Pinned Task header describes tc_SPUpper as upper bound + 2. Exclude
     * those two bytes and use checked subtraction before any span arithmetic.
     */
    if(!lower||!raw_upper||!pointer||raw_upper<2U||((lower|raw_upper|pointer)&1U))return 0;
    upper=raw_upper-2U;
    if(upper<=lower||pointer<lower||pointer>=upper)return 0;
    return pointer-lower>=ENTRY_TASK_MIN_HEADROOM;
}
static int entry_system_span(void *lower,void *upper)
{
    uintptr_t first=(uintptr_t)lower,last=(uintptr_t)upper;
    return first&&last>first&&!((first|last)&1U)&&entry_span(lower,last-first);
}
/* Fixed task samples are observations of this getter's live-SP location.
 * Only initial admission applies the 32768-byte policy. Later samples neither
 * measure all callees nor prove remaining total-stack demand.
 */
static void entry_task_record(uintptr_t pointer,unsigned phase)
{
    uintptr_t lower=(uintptr_t)bootstrap.stack_lower,upper=(uintptr_t)bootstrap.stack_usable_upper;
    if(bootstrap.task_stack_samples>=ENTRY_TASK_STACK_SAMPLE_LIMIT)
        bootstrap.task_stack_status|=PT_PRIVATE_RAM_STACK_BAD_COUNT;
    else ++bootstrap.task_stack_samples;
    if(bootstrap.task_stack_phases&phase)bootstrap.task_stack_status|=PT_PRIVATE_RAM_STACK_BAD_PHASE;
    bootstrap.task_stack_phases|=phase;
    if(!pointer||(pointer&1U)||pointer<lower||pointer>=upper){
        bootstrap.task_stack_status|=PT_PRIVATE_RAM_STACK_BAD_SAMPLE;return;
    }
    if(!bootstrap.task_sampled_low_sp||pointer<bootstrap.task_sampled_low_sp)
        bootstrap.task_sampled_low_sp=pointer;
}
static void entry_task_sample(unsigned phase)
{
    uintptr_t pointer;
    entry_require_task("original Task/ExecBase stack identity changed before task sample");
#ifdef PT_PRIVATE_NATIVE_ENTRY_HOST_MODEL
    pointer=entry_model_task_sample_sp(phase);
#else
    pointer=entry_live_sp();
#endif
    entry_require_task("original Task/ExecBase stack identity changed during task sample");
    entry_task_record(pointer,phase);
    if(bootstrap.task_stack_status)entry_hold("sampled task stack record is invalid; no total-stack inference");
}
static int entry_irq_stack_good(const struct pt_private_ram_irq *irq)
{
    uintptr_t lower=(uintptr_t)bootstrap.system_lower,upper=(uintptr_t)bootstrap.system_upper;
    uintptr_t first=irq->entry_sp,low=irq->sampled_low_sp,last=irq->exit_sp;
    return irq->stack_version==PT_PRIVATE_RAM_STACK_VERSION&&
        irq->system_lower==lower&&irq->system_upper==upper&&
        irq->stack_status==PT_PRIVATE_RAM_STACK_PHASES&&irq->stack_samples==PT_PRIVATE_RAM_STACK_SAMPLE_LIMIT&&
        first>=lower&&first<upper&&last==first&&low>=lower&&low<upper&&
        !((first|low|last)&1U)&&first-lower>=48U&&low<=first-48U;
}
static void entry_irq_stack_report(const struct pt_private_ram_irq *irq)
{
    /* Called only after independently positive exact source close. These are
     * bounded samples, not an IRQ overflow guard or nested-call high-water.
     */
    entry_write("NATIVE RAM ENTRY SAMPLED IRQ STACK: lower=");entry_hex(irq->system_lower);
    entry_write(" upper=");entry_hex(irq->system_upper);entry_write(" entry=");entry_hex(irq->entry_sp);
    entry_write(" low=");entry_hex(irq->sampled_low_sp);entry_write(" exit=");entry_hex(irq->exit_sp);
    entry_write(" status=");entry_hex(irq->stack_status);entry_write(" samples=");entry_hex(irq->stack_samples);
    entry_write("; sampled workload only, total/system reserve/WCET UNKNOWN\n");
}
static void entry_task_stack_report(void)
{
    entry_write("NATIVE RAM ENTRY SAMPLED TASK STACK: lower=");entry_hex((uintptr_t)bootstrap.stack_lower);
    entry_write(" usable-upper=");entry_hex((uintptr_t)bootstrap.stack_usable_upper);
    entry_write(" entry=");entry_hex(bootstrap.task_entry_sp);entry_write(" low=");entry_hex(bootstrap.task_sampled_low_sp);
    entry_write(" downward-headroom=");entry_hex(bootstrap.task_sampled_low_sp-(uintptr_t)bootstrap.stack_lower);
    entry_write(" status=");entry_hex(bootstrap.task_stack_status);entry_write(" phases=");entry_hex(bootstrap.task_stack_phases);
    entry_write(" samples=");entry_hex(bootstrap.task_stack_samples);
    entry_write("; admission policy 32768, sampled workload only, aggregate stack UNKNOWN\n");
}

/* Independent bootstrap ownership has no prepended allocation header, copied
 * owner or self-allocation. Any ambiguous non-NULL return is retained in the
 * static bootstrap record without dereferencing/freeing the suspect control.
 */
static int entry_bootstrap_open(void)
{
    struct pt_master_memory policy;uintptr_t stack_pointer;void *p;size_t n=sizeof(struct pt_private_native_memory),stack_n;
    if(!SysBase||!entry_span(SysBase,sizeof(*SysBase))||SysBase->LibNode.lib_Version<36)return 0;
    bootstrap.exec=SysBase;bootstrap.system_lower=SysBase->SysStkLower;bootstrap.system_upper=SysBase->SysStkUpper;
    bootstrap.task=FindTask(NULL);
    if(!bootstrap.task||bootstrap.task->tc_Node.ln_Type!=NT_PROCESS)return 0;
    bootstrap.priority=bootstrap.task->tc_Node.ln_Pri;bootstrap.original_signals=bootstrap.task->tc_SigAlloc;
    bootstrap.stack_lower=bootstrap.task->tc_SPLower;bootstrap.stack_upper=bootstrap.task->tc_SPUpper;
    stack_pointer=entry_live_sp();
    if(!entry_task_stack_admitted((uintptr_t)bootstrap.stack_lower,
         (uintptr_t)bootstrap.stack_upper,stack_pointer))return 0;
    bootstrap.stack_usable_upper=(void *)((uintptr_t)bootstrap.stack_upper-2U);
    stack_n=(uintptr_t)bootstrap.stack_usable_upper-(uintptr_t)bootstrap.stack_lower;
    if(!entry_span(bootstrap.stack_lower,stack_n)||
       !entry_system_span(bootstrap.system_lower,bootstrap.system_upper))return 0;
    bootstrap.task_entry_sp=stack_pointer;entry_task_record(stack_pointer,ENTRY_TASK_STACK_BOOT);
    pt_master_memory_init(&policy);
    entry_require_task("original task changed during bootstrap availability queries");
    if(policy.flags!=MEMF_FAST||policy.limit<n)return 0;
    p=AllocMem((ULONG)n,MEMF_FAST|MEMF_PUBLIC);
    bootstrap.original_control=p;bootstrap.original_control_bytes=n;
    entry_require_task("original task changed during bootstrap allocation");
    if(!p)return 0;
    if(!entry_span(p,n)||(uintptr_t)p%offsetof(struct entry_control_alignment,value)||
       !entry_apart(p,n,&bootstrap,sizeof(bootstrap))||
       !entry_apart(p,n,bootstrap.exec,sizeof(*bootstrap.exec))||
       !entry_apart(p,n,bootstrap.system_lower,(uintptr_t)bootstrap.system_upper-(uintptr_t)bootstrap.system_lower)||
       !entry_apart(p,n,bootstrap.task,sizeof(struct Process))||
       !entry_apart(p,n,bootstrap.stack_lower,stack_n)||!entry_fast(p,n))entry_hold("bootstrap control return ambiguous");
    entry_require_task("original task changed during bootstrap placement checks");
    memset(p,0,n);bootstrap.memory=p;bootstrap.completion_signal=-1;
    return 1;
}
/* Entry-owned construction performs no implicit cleanup before validating the
 * actual returned port/signal identity. A NULL/negative acquisition is ordinary
 * refusal; a nonnegative malformed or overlapping identity is uncertainty.
 * Existing inline eclock/alarm implementations remain unchanged in their files.
 */
static ULONG entry_accept_port(struct MsgPort *port,ULONG other)
{
    ULONG mask;
    entry_require_task("original task changed before accepting message port");
    if(!port||port->mp_SigBit>=32||port->mp_SigTask!=bootstrap.task)
        entry_hold("message port signal identity is unproven");
    mask=1UL<<port->mp_SigBit;
    if((mask&(bootstrap.original_signals|other|SIGBREAKF_CTRL_C))||
       !(bootstrap.task->tc_SigAlloc&mask))entry_hold("message port signal is not fresh and independently owned");
    return mask;
}
static int entry_clock_open(void)
{
    LONG error;
    entry_require_task("original task changed before clock port creation");
    bootstrap.clock.port=CreateMsgPort();
    entry_require_task("original task changed during clock port creation");
    if(!bootstrap.clock.port)return 0;
    bootstrap.clock_mask=entry_accept_port(bootstrap.clock.port,0);bootstrap.clock_signal_owned=1;
    entry_require_task("original task changed before clock request creation");
    bootstrap.clock.request=(struct timerequest *)CreateIORequest(bootstrap.clock.port,sizeof(*bootstrap.clock.request));
    entry_require_task("original task changed during clock request creation");
    if(!bootstrap.clock.request)return 0;
    entry_require_task("original task changed before clock device open");
    error=OpenDevice(TIMERNAME,UNIT_ECLOCK,(struct IORequest *)bootstrap.clock.request,0);
    entry_require_task("original task changed during clock device open");
    if(error)return 0;
    bootstrap.clock.opened=1;
    if(!bootstrap.clock.request->tr_node.io_Device)entry_hold("successful clock open lacks device identity");
    return bootstrap.clock.request->tr_node.io_Device->dd_Library.lib_Version>=36;
}
static int entry_termination_open(void)
{
    LONG error;
    entry_require_task("original task changed before termination port creation");
    bootstrap.termination.port=CreateMsgPort();
    entry_require_task("original task changed during termination port creation");
    if(!bootstrap.termination.port)return 0;
    bootstrap.termination_mask=entry_accept_port(bootstrap.termination.port,bootstrap.clock_mask);
    bootstrap.termination_signal_owned=1;
    entry_require_task("original task changed before termination request creation");
    bootstrap.termination.request=(struct timerequest *)CreateIORequest(bootstrap.termination.port,sizeof(*bootstrap.termination.request));
    entry_require_task("original task changed during termination request creation");
    if(!bootstrap.termination.request)return 0;
    entry_require_task("original task changed before termination device open");
    error=OpenDevice(TIMERNAME,UNIT_WAITECLOCK,(struct IORequest *)bootstrap.termination.request,0);
    entry_require_task("original task changed during termination device open");
    if(error)return 0;
    bootstrap.termination.opened=1;
    if(!bootstrap.termination.request->tr_node.io_Device)entry_hold("successful termination open lacks device identity");
    return bootstrap.termination.request->tr_node.io_Device->dd_Library.lib_Version>=36;
}
static int entry_os_open(void)
{
    ULONG mask;
    entry_require_task("original task changed before OS construction");
    if(!entry_clock_open()||!entry_termination_open())return 0;
    entry_require_task("original task changed before completion signal allocation");
    bootstrap.completion_signal=AllocSignal(-1);
    entry_require_task("original task changed during completion signal allocation");
    if(bootstrap.completion_signal<0)return 0;
    if(bootstrap.completion_signal>=32)entry_hold("nonnegative completion signal is out of range");
    mask=1UL<<bootstrap.completion_signal;bootstrap.completion_mask=mask;
    if((mask&(bootstrap.original_signals|bootstrap.clock_mask|bootstrap.termination_mask|SIGBREAKF_CTRL_C))||
       !(bootstrap.task->tc_SigAlloc&mask))entry_hold("completion signal identity is not fresh and independently owned");
    bootstrap.completion_signal_owned=1;
    return 1;
}
static int entry_memory_open(void)
{
    size_t stack_n=(uintptr_t)bootstrap.stack_usable_upper-(uintptr_t)bootstrap.stack_lower;
    unsigned i=0;int result;
    entry_require_task("original task changed before checked-memory initialization");
    bootstrap.guard[i++]=(struct pt_private_memory_span){&bootstrap,sizeof(bootstrap)};
    bootstrap.guard[i++]=(struct pt_private_memory_span){bootstrap.task,sizeof(struct Process)};
    bootstrap.guard[i++]=(struct pt_private_memory_span){bootstrap.stack_lower,stack_n};
    bootstrap.guard[i++]=(struct pt_private_memory_span){bootstrap.exec,sizeof(*bootstrap.exec)};
    bootstrap.guard[i++]=(struct pt_private_memory_span){bootstrap.system_lower,
        (uintptr_t)bootstrap.system_upper-(uintptr_t)bootstrap.system_lower};
    bootstrap.guard[i++]=(struct pt_private_memory_span){bootstrap.clock.port,sizeof(*bootstrap.clock.port)};
    bootstrap.guard[i++]=(struct pt_private_memory_span){bootstrap.clock.request,sizeof(*bootstrap.clock.request)};
    bootstrap.guard[i++]=(struct pt_private_memory_span){bootstrap.termination.port,sizeof(*bootstrap.termination.port)};
    bootstrap.guard[i++]=(struct pt_private_memory_span){bootstrap.termination.request,sizeof(*bootstrap.termination.request)};
    if(i>PT_PRIVATE_MEMORY_GUARDS)return 0;
    result=pt_private_native_memory_init(bootstrap.memory,UINT32_MAX,bootstrap.guard,i);
    entry_require_task("original task changed during checked-memory initialization");
    if(result!=PT_PRIVATE_MEMORY_OK)return 0;
    bootstrap.memory_initialized=1;return 1;
}

/* Every context and source is a separate genuine allocation in the checked
 * registry. The construction declarations are ended after enqueue; no live
 * registered event/domain/span/control storage is poisoned or copied here.
 */
static struct entry_state *entry_storage(uint64_t session)
{
    struct entry_state *s=entry_allocate(sizeof(*s));unsigned i;
    if(!s)return NULL;
    s->session=session;
    s->watch=entry_allocate(sizeof(*s->watch));
    if(!s->watch)return s;
    s->watch->memory=bootstrap.memory;
    s->port=entry_allocate(sizeof(*s->port));
    s->irq=entry_allocate(sizeof(*s->irq));
    s->cia=entry_allocate(sizeof(*s->cia));
    s->scratch=entry_allocate(sizeof(*s->scratch));
    if(!s->port||!s->irq||!s->cia||!s->scratch)return s;
    for(i=0;i<ENTRY_HOLDERS;++i){unsigned j;struct entry_holder *h;
        h=entry_allocate(sizeof(*h));if(!h)return s;
        s->holder[i]=h;h->watch=s->watch;h->index=i;h->token=(uint64_t)i+1;
        s->watch->live[i]=1;
        if(!i)continue;
        h->sample=entry_allocate(ENTRY_SAMPLE_BYTES);if(!h->sample)return s;
        for(j=0;j<ENTRY_SAMPLE_BYTES;++j)h->sample[j]=(uint8_t)((int)(i-1)*17+(int)j-64);
    }
    return s;
}
static int entry_storage_complete(const struct entry_state *s)
{
    unsigned i;
    if(!s||!s->watch||!s->port||!s->irq||!s->cia||!s->scratch)return 0;
    for(i=0;i<ENTRY_HOLDERS;++i)
        if(!s->holder[i]||!s->watch->live[i]||(i&&!entry_source_same(s->holder[i])))return 0;
    return 1;
}
static int entry_deadlines(struct entry_state *s)
{
    uint64_t epoch,offset,last_offset;uint32_t frequency;
    entry_require_task("original task changed before original clock epoch");
    if(!pt_native_eclock_read(&bootstrap.clock,&epoch,&frequency)||
       (frequency!=709379&&frequency!=715909))return 0;
    entry_require_task("original task changed during original clock read");
    s->grid=(struct pt_scheduled_grid){epoch,ENTRY_GENERATION,frequency,ENTRY_RATE};
    if(pt_elapsed_clock_init(&s->clock_grid,frequency,ENTRY_RATE,epoch,0)!=PT_ELAPSED_OK||
       pt_elapsed_clock_deadline(&s->clock_grid,ENTRY_FRAME,&s->first)!=PT_ELAPSED_OK||
       pt_elapsed_clock_deadline(&s->clock_grid,ENTRY_FRAME+1,&s->last)!=PT_ELAPSED_OK||
       pt_elapsed_clock_deadline(&s->clock_grid,60000,&s->termination_deadline)!=PT_ELAPSED_OK)return 0;
    /* Independent ceil division checks the unshifted frame interval. These
     * are threshold oracles, never substituted for an actual clock reading.
     * PAL:60534/60549, NTSC:61091/61106 offset ticks at 48000 frames/sec.
     */
    offset=(ENTRY_FRAME*frequency+ENTRY_RATE-1)/ENTRY_RATE;
    last_offset=((ENTRY_FRAME+1)*frequency+ENTRY_RATE-1)/ENTRY_RATE;
    return s->first>=epoch&&s->last>=epoch&&s->first-epoch==offset&&s->last-epoch==last_offset&&
        offset==(frequency==709379?60534U:61091U)&&
        last_offset==(frequency==709379?60549U:61106U)&&s->first<s->last&&s->first>=ENTRY_EARLY;
}
static int entry_core_open(struct entry_state *s)
{
    struct pt_private_ram_adapter adapter;unsigned i;enum pt_scheduled_result result;
    entry_require_task("original task changed before genuine core construction");
    /* These actual owners are initialized before any constructor clock call.
     * No forged seed/validated flag, predicted ACTIVE or artificial key exists.
     */
    s->cia->timer=bootstrap.clock.request->tr_node.io_Device;
    s->cia->frequency=s->grid.frequency;
    s->irq->timer=s->cia->timer;s->irq->task=bootstrap.task;
    s->irq->signal=bootstrap.completion_mask;s->irq->port=s->port;
    s->irq->stack_version=PT_PRIVATE_RAM_STACK_VERSION;
    s->irq->system_lower=(uintptr_t)bootstrap.system_lower;s->irq->system_upper=(uintptr_t)bootstrap.system_upper;
    adapter=pt_private_cia_ram_api(s->cia);
    if(!pt_private_ram_init(s->port,s->session,ENTRY_GENERATION,s->grid.frequency,
       &adapter,ENTRY_EARLY,ENTRY_RESIDENCY,ENTRY_READS))return 0;
    s->allocator=(struct pt_allocator){bootstrap.memory,entry_allocator_allocate,entry_allocator_release};
    s->port_api=pt_private_ram_api(s->port);
    result=pt_readers_activation_open(&s->allocator,&s->grid,s->session,&s->port_api,&s->ledger);
    if(!entry_memory_good())entry_hold("ledger constructor allocation ambiguity");
    if(result!=PT_SCHEDULED_OK)return 0;
    if(pt_readers_activation_api(s->ledger,&s->backend)!=PT_SCHEDULED_OK)return 0;
    result=pt_readers_open(&s->allocator,&s->grid,s->session,&s->backend,2,8,&s->queue);
    if(!entry_memory_good())entry_hold("queue constructor allocation ambiguity");
    if(result!=PT_SCHEDULED_OK)return 0;
    s->scratch->batch.generation=ENTRY_GENERATION;
    s->scratch->batch.frame=ENTRY_FRAME;s->scratch->batch.count=4;
    s->scratch->command=entry_control(s->holder[0]);
    for(i=0;i<4;++i){
        s->scratch->span[i]=(struct pt_scheduled_span){s->holder[i+1]->sample,ENTRY_SAMPLE_BYTES};
        s->scratch->reader[i]=(struct pt_readers_owner){entry_control(s->holder[i+1]),s->scratch->span+i,1};
        s->scratch->batch.action[i]=(struct pt_scheduled_action){PT_SCHEDULED_TRIGGER,i,
            s->holder[i+1]->sample,64,428,32};
    }
    result=pt_readers_enqueue(s->queue,&s->scratch->batch,NULL,&s->scratch->command,s->scratch->reader,&s->ticket);
    if(result!=PT_SCHEDULED_OK)return 0;
    s->transferred=1;
    /* Only ended caller declaration storage is erased/freed. Genuine original
     * registered descriptors/holders/sources remain immutable and live.
     */
    memset(s->scratch,0,sizeof(*s->scratch));
    if(!entry_drop(s->scratch))entry_hold("ended construction scratch release refused");
    s->scratch=NULL;
    return s->ticket&&pt_readers_commands_held(s->queue)==1&&pt_readers_readers_held(s->queue)==4;
}
static void entry_source_end(struct entry_state *s)
{
    int result;
    entry_require_task("original task changed before source shutdown");
    if(bootstrap.source_closed||!s||!s->acquired)return;
    /* Normal-task return provenance and whole task entry exclusion are source
     * obligations, not checked by a completed flag. No Wait/print/hold inside.
     * Adapter performs at most one exact owned close attempt on uncertainty.
     */
    Disable();result=pt_private_ram_source_close(s->port);Enable();
    entry_require_task("original task changed during source shutdown");
    if(result!=1)entry_hold("owned CIA source shutdown uncertain");
    bootstrap.source_closed=1;
    if(s->cia->phase!=PT_PRIVATE_CIA_CLOSED||s->cia->vector_owned||s->irq->armed||
       !s->port->source_closed||s->port->dispatching)entry_hold("positive source-close invariants changed");
    entry_task_sample(ENTRY_TASK_STACK_SOURCE_CLOSED);
    if(s->irq->calls||s->irq->stack_samples){
        entry_irq_stack_report(s->irq);
        if(s->irq->calls!=1||!entry_irq_stack_good(s->irq))
            entry_hold("sampled IRQ stack record invalid after source quiet; complete world retained");
    }
}
static int entry_arm_publish(struct entry_state *s)
{
    enum pt_alarm_result alarm;enum pt_scheduled_result result;uint64_t now;uint32_t frequency;int acquired;
    entry_require_task("original task changed before termination arm");
    alarm=pt_native_alarm_arm(&bootstrap.termination,s->termination_deadline);
    entry_require_task("original task changed during termination arm");
    if(alarm!=PT_ALARM_WAITING)return 0; /* termination notification ONLY */
    {int sealed=pt_private_native_memory_seal(bootstrap.memory);
        entry_require_task("original task changed during allocation seal");
        if(sealed!=PT_PRIVATE_MEMORY_OK)return 0;
    }
    /* The epoch is never reconstructed after a preparation delay. Refuse late
     * preparation in its original interval; there is no immediate-start path.
     */
    if(!pt_native_eclock_read(&bootstrap.clock,&now,&frequency)||frequency!=s->grid.frequency||
       now<s->grid.epoch||now>=s->first-ENTRY_EARLY)return 0;
    entry_require_task("original task changed before source acquisition");
    entry_task_sample(ENTRY_TASK_STACK_BEFORE_SOURCE);
    acquired=pt_private_cia_ram_acquire(s->cia,bootstrap.clock.request->tr_node.io_Device,s->irq,s->grid.frequency);
    entry_require_task("original task changed during source acquisition");
    if(acquired==0)return 0;
    s->acquired=1;
    if(acquired!=1){entry_source_end(s);entry_hold("CIA acquisition was uncertain; retain complete original world");}
    Disable();
    SetSignal(0,bootstrap.completion_mask); /* fresh independently owned bit */
    s->publish_attempted=1;result=pt_readers_publish(s->queue,s->ticket);
    s->activation_current=s->watch->current;
    Enable();
    entry_require_task("original task changed during publication");
    if(!entry_memory_good()||s->watch->failed){entry_source_end(s);entry_hold("publication owner/allocator protocol failed");}
    if(result==PT_SCHEDULED_OK)return 1;
    entry_source_end(s);
    if(result==PT_SCHEDULED_PENDING||result==PT_SCHEDULED_LATE||result==PT_SCHEDULED_CLOCK)return 0;
    entry_hold("publication uncertainty; no automatic repeat or forced cleanup");
}
static int entry_wait(struct entry_state *s)
{
    ULONG signals;
    entry_require_task("original task changed before notification wait");
    signals=Wait(bootstrap.completion_mask|bootstrap.termination_mask|SIGBREAKF_CTRL_C);
    entry_require_task("original task changed during notification wait");
    /* Waking up is not evidence of activation. Stop the exact source first,
     * while all IRQ/code/timer/ledger/context lifetimes remain retained.
     */
    entry_source_end(s);
    return (signals&bootstrap.completion_mask)&&!(signals&SIGBREAKF_CTRL_C);
}
static int entry_actual(struct entry_state *s)
{
    struct pt_private_ram_port *p=s->port;struct pt_private_ram_irq *irq=s->irq;unsigned i,readers=0;
    uint64_t before=((uint64_t)irq->before.hi<<32)|irq->before.lo;
    uint64_t after=((uint64_t)irq->after.hi<<32)|irq->after.lo;
    if(!entry_task_current()||!bootstrap.source_closed||irq->calls!=1||!entry_irq_stack_good(irq)||
       irq->dispatch_result!=PT_READERS_ACTIVATION_COMMITTED||p->dispatch_result!=PT_READERS_ACTIVATION_COMMITTED||
       p->ledger_result!=PT_READERS_ACTIVATION_COMMITTED||p->entries!=1||p->fires!=1||p->commits!=1||p->effects!=4||
       p->mask!=15U||p->trace_count<3||p->trace_count>ENTRY_READS||
       irq->before_frequency!=s->grid.frequency||irq->after_frequency!=s->grid.frequency||
       before>=s->last||before<s->first-ENTRY_EARLY||after<before||
       s->watch->current!=s->activation_current||s->watch->terminal||s->watch->released||s->watch->failed)return 0;
    for(i=0;i<p->trace_count;++i)
        if(p->trace[i].frequency!=s->grid.frequency||(i&&p->trace[i].ticks<p->trace[i-1].ticks))return 0;
    s->observed=p->trace[p->trace_count-2].ticks;s->issued=p->trace[p->trace_count-1].ticks;
    if(p->trace[0].ticks<before||p->trace[p->trace_count-1].ticks>after||
       p->trace[p->trace_count-1].ticks-p->trace[0].ticks>ENTRY_RESIDENCY||
       s->observed<s->first||s->observed>s->issued||s->issued>=s->last)return 0;
    for(i=0;i<PT_PRIVATE_RAM_COMMANDS;++i){unsigned j;
        if(!p->command[i].live)continue;
        if(p->command[i].packet.ticket!=s->ticket||p->command[i].packet.frame!=ENTRY_FRAME||
           p->command[i].packet.first!=s->first||p->command[i].packet.last!=s->last||
           !p->command[i].finished||p->command[i].armed||p->command[i].uncertain)return 0;
        for(j=0;j<p->command[i].packet.count;++j)
            if(p->command[i].packet.action[j].data||p->command[i].packet.action[j].words)return 0;
    }
    for(i=0;i<PT_PRIVATE_RAM_READERS;++i){struct pt_private_ram_reader *r=p->reader+i;unsigned slot;
        if(!r->live)continue;
        ++readers;slot=r->key.slot;
        if(slot>=4||!r->active||!entry_key_same(&r->key,p->slot+slot)||
           r->key.queue!=s->queue||r->key.session!=s->session||r->key.generation!=ENTRY_GENERATION||
           r->key.trigger!=s->ticket||r->key.owner!=(uint64_t)slot+2||!r->key.serial||r->key.action!=slot||
           r->data!=s->holder[slot+1]->sample||r->bytes!=ENTRY_SAMPLE_BYTES||r->period!=428||r->volume!=32||
           !entry_source_same(s->holder[slot+1]))return 0;
    }
    return readers==4&&pt_readers_commands_held(s->queue)==1&&pt_readers_readers_held(s->queue)==4;
}
static int entry_observe_and_detach(struct entry_state *s)
{
    unsigned i,j;enum pt_scheduled_result result;
    entry_require_task("original task changed before genuine domain observations");
    /* This sequence deliberately gets original ACTIVE observation BEFORE any
     * getter. PENDING alone is insufficient: exact public receipt fields below
     * prove a changed, positive genuine observation, not submission acceptance.
     */
    for(i=0;i<4;++i){
        memset(&s->receipt,0,sizeof(s->receipt));
        result=pt_readers_poll_reader(s->queue,s->ticket,i,&s->receipt);
        entry_require_task("original task changed during reader observation");
        if(result!=PT_SCHEDULED_PENDING||s->receipt.domain!=PT_READERS_READER_DOMAIN||
           !s->receipt.reference||s->receipt.context!=s->holder[i+1]||
           s->receipt.context_bytes!=sizeof(*s->holder[i+1])||
           !entry_key_same(&s->receipt.key,s->port->slot+i)||s->receipt.state!=PT_READERS_ACTIVE||
           s->receipt.adoption!=PT_READERS_ADOPTED||s->receipt.observed!=s->observed||
           s->receipt.issued!=s->issued)return 0;
        if(pt_readers_reader_key(s->queue,s->ticket,i,s->key+i)!=PT_SCHEDULED_OK||
           !entry_key_same(s->key+i,&s->receipt.key))return 0;
    }
    if(s->watch->terminal||s->watch->released||s->watch->failed||!entry_memory_good())return 0;
    result=pt_readers_poll_command(s->queue,s->ticket,&s->command_receipt);
    entry_require_task("original task changed during command detach");
    if(result!=PT_SCHEDULED_OK||s->command_receipt.domain!=PT_READERS_COMMAND_DOMAIN||
       s->command_receipt.origin!=PT_READERS_BACKEND_ACTUAL||s->command_receipt.queue!=s->queue||
       s->command_receipt.session!=s->session||s->command_receipt.generation!=ENTRY_GENERATION||
       s->command_receipt.ticket!=s->ticket||s->command_receipt.owner!=1||s->command_receipt.count!=4||
       !s->command_receipt.event||s->command_receipt.context!=s->holder[0]||
       s->command_receipt.context_bytes!=sizeof(*s->holder[0]))return 0;
    s->holder[0]=NULL; /* Released identity is compared only as an opaque value. */
    if(s->watch->released!=1||s->watch->terminal!=1||s->watch->live[0]||s->watch->failed||
       !entry_memory_good()||pt_readers_commands_held(s->queue)||pt_readers_readers_held(s->queue)!=4||
       s->port->mask!=15U)return 0;
    for(i=0;i<4;++i){const struct pt_readers_action_receipt *a=s->command_receipt.action+i;
        if(a->command!=PT_READERS_ISSUED||a->reader!=PT_READERS_ACTIVE||a->adoption!=PT_READERS_ADOPTED||
           !entry_key_same(&a->key,s->key+i)||a->observed!=s->observed||a->issued!=s->issued||
           !entry_source_same(s->holder[i+1]))return 0;
    }
    for(j=0;j<PT_PRIVATE_RAM_COMMANDS;++j)
        if(s->port->command[j].live||s->port->command[j].ledger)return 0;
    return 1; /* Detachment has not retired any persistent reader. */
}
static int entry_retire(struct entry_state *s)
{
    unsigned i,j;enum pt_scheduled_result result;
    entry_require_task("original task changed before reader retirement");
    for(i=0;i<4;++i){
        result=pt_readers_cancel_reader(s->queue,s->ticket,i,&s->receipt);
        entry_require_task("original task changed during reader retirement");
        if(result!=PT_SCHEDULED_OK||s->receipt.domain!=PT_READERS_READER_DOMAIN||
           !s->receipt.reference||s->receipt.context!=s->holder[i+1]||
           s->receipt.context_bytes!=sizeof(*s->holder[i+1])||
           s->receipt.state!=PT_READERS_RETIRED||s->receipt.adoption!=PT_READERS_ADOPTED||
           !entry_key_same(&s->receipt.key,s->key+i)||s->receipt.observed!=s->observed||
           s->receipt.issued!=s->issued)return 0;
        s->holder[i+1]=NULL;
        if(!entry_memory_good()||s->watch->failed||s->watch->live[i+1]||
           s->watch->released!=i+2||s->watch->terminal!=i+2||
           pt_readers_readers_held(s->queue)!=3-i||s->port->mask!=(15U&~((1U<<(i+1))-1)))return 0;
        for(j=0;j<PT_PRIVATE_RAM_READERS;++j)
            if(s->port->reader[j].live&&entry_key_same(&s->port->reader[j].key,s->key+i))return 0;
    }
    return !pt_readers_commands_held(s->queue)&&!pt_readers_readers_held(s->queue)&&!s->port->mask;
}
static int entry_core_end(struct entry_state *s)
{
    unsigned i;
    entry_require_task("original task changed before core teardown");
    if(s->queue){
        if(pt_readers_stop(s->queue)!=PT_SCHEDULED_OK||!entry_memory_good())return 0;
        if(!pt_readers_close(s->queue)||!entry_memory_good())return 0;
        s->queue=NULL;
    }
    /* ABI2 has no queue-close notification: this explicit order ends the only
     * queue holding the copied API AND the autonomous source before ledger.
     */
    if(s->ledger){
        if(s->acquired&&!bootstrap.source_closed)return 0;
        if(!pt_readers_activation_close(&s->ledger)||s->ledger||!entry_memory_good())return 0;
    }
    if(s->transferred){
        if(!s->watch||s->watch->failed||s->watch->released!=ENTRY_HOLDERS||s->watch->terminal!=ENTRY_HOLDERS)return 0;
        for(i=0;i<ENTRY_HOLDERS;++i){if(s->watch->live[i])return 0;s->holder[i]=NULL;}
    }else{
        for(i=0;i<ENTRY_HOLDERS;++i)if(s->holder[i]){
            entry_terminal(s->holder[i],s->holder[i]->token,1);
            entry_release(s->holder[i],s->holder[i]->token);s->holder[i]=NULL;
            if(!entry_memory_good()||s->watch->failed)return 0;
        }
    }
    return 1;
}
static int entry_storage_end(struct entry_state *s)
{
    entry_require_task("original task changed before any tracked teardown");
    if(!s)return 1;
    if(!entry_core_end(s))return 0;
    if(!entry_drop(s->scratch)||!entry_drop(s->port)||!entry_drop(s->irq)||!entry_drop(s->cia)||
       !entry_drop(s->watch)||!entry_drop(s))return 0;
    return entry_memory_good();
}
static void entry_require_port(const struct MsgPort *port,ULONG mask,unsigned owned)
{
    entry_require_task("original task changed before task-relative port operation");
    if(!port||!owned||port->mp_SigTask!=bootstrap.task||port->mp_SigBit>=32||
       (1UL<<port->mp_SigBit)!=mask||!(bootstrap.task->tc_SigAlloc&mask))
        entry_hold("previously accepted message-port signal identity changed");
}
/* The termination teardown below retains the pinned owner's actual
 * CheckIO-before-WaitIO protocol, with provenance checks around EACH OS call.
 * One cancel/AbortIO request is followed by finite observation only.
 */
static enum pt_alarm_result entry_termination_poll(void)
{
    struct pt_native_alarm *a=&bootstrap.termination;struct IORequest *completed;LONG error;
    entry_require_task("original task changed before termination poll");
    if(!a->pending)return a->failed?PT_ALARM_ERROR:PT_ALARM_IDLE;
    if(!a->opened||!a->request)entry_hold("pending termination request lacks its original owner");
    entry_require_port(a->port,bootstrap.termination_mask,bootstrap.termination_signal_owned);
    completed=CheckIO((struct IORequest *)a->request);
    entry_require_task("original task changed during termination CheckIO");
    if(!completed)return PT_ALARM_WAITING;
    if(completed!=(struct IORequest *)a->request)entry_hold("termination CheckIO returned a foreign completed request");
    entry_require_task("original task changed before completed termination WaitIO");
    error=WaitIO((struct IORequest *)a->request);
    entry_require_task("original task changed during completed termination WaitIO");
    a->pending=0;
    if(error&&!(a->cancelling&&error==IOERR_ABORTED)){a->failed=1;return PT_ALARM_ERROR;}
    return a->cancelling?PT_ALARM_CANCELLED:PT_ALARM_READY;
}
static enum pt_alarm_result entry_termination_cancel(void)
{
    struct pt_native_alarm *a=&bootstrap.termination;struct IORequest *completed;
    entry_require_task("original task changed before termination cancel");
    if(!a->pending)return a->failed?PT_ALARM_ERROR:PT_ALARM_IDLE;
    if(!a->opened||!a->request)entry_hold("pending termination cancel lacks its original owner");
    entry_require_port(a->port,bootstrap.termination_mask,bootstrap.termination_signal_owned);
    if(!a->cancelling){
        a->cancelling=1;
        completed=CheckIO((struct IORequest *)a->request);
        entry_require_task("original task changed during cancellation CheckIO");
        if(completed&&completed!=(struct IORequest *)a->request)entry_hold("cancellation CheckIO returned a foreign completed request");
        if(!completed){
            entry_require_task("original task changed before termination AbortIO");
            AbortIO((struct IORequest *)a->request);
            entry_require_task("original task changed during termination AbortIO");
        }
    }
    return entry_termination_poll();
}
static void entry_termination_close(void)
{
    struct pt_native_alarm *a=&bootstrap.termination;
    entry_require_task("original task changed before termination owner deletion");
    if(a->pending)entry_hold("unfinished termination owner cannot be deleted");
    if(a->port)entry_require_port(a->port,bootstrap.termination_mask,bootstrap.termination_signal_owned);
    else if(a->request||a->opened||bootstrap.termination_signal_owned)entry_hold("termination port ownership changed");
    if(a->opened){
        if(!a->request)entry_hold("opened termination owner lacks request");
        entry_require_task("original task changed before termination CloseDevice");
        CloseDevice((struct IORequest *)a->request);
        entry_require_task("original task changed during termination CloseDevice");a->opened=0;
    }
    if(a->request){
        entry_require_port(a->port,bootstrap.termination_mask,bootstrap.termination_signal_owned);
        DeleteIORequest((struct IORequest *)a->request);
        entry_require_task("original task changed during termination request deletion");a->request=NULL;
    }
    if(a->port){
        entry_require_port(a->port,bootstrap.termination_mask,bootstrap.termination_signal_owned);
        DeleteMsgPort(a->port);
        entry_require_task("original task changed during termination port deletion");
        a->port=NULL;bootstrap.termination_signal_owned=0;
        if(bootstrap.task->tc_SigAlloc&bootstrap.termination_mask)entry_hold("termination signal ownership did not restore");
    }
}
static void entry_clock_close(void)
{
    struct pt_native_eclock *c=&bootstrap.clock;
    entry_require_task("original task changed before clock owner deletion");
    if(c->port)entry_require_port(c->port,bootstrap.clock_mask,bootstrap.clock_signal_owned);
    else if(c->request||c->opened||bootstrap.clock_signal_owned)entry_hold("clock port ownership changed");
    if(c->opened){
        if(!c->request)entry_hold("opened clock owner lacks request");
        entry_require_task("original task changed before clock CloseDevice");
        CloseDevice((struct IORequest *)c->request);
        entry_require_task("original task changed during clock CloseDevice");c->opened=0;
    }
    if(c->request){
        entry_require_port(c->port,bootstrap.clock_mask,bootstrap.clock_signal_owned);
        DeleteIORequest((struct IORequest *)c->request);
        entry_require_task("original task changed during clock request deletion");c->request=NULL;
    }
    if(c->port){
        entry_require_port(c->port,bootstrap.clock_mask,bootstrap.clock_signal_owned);
        DeleteMsgPort(c->port);
        entry_require_task("original task changed during clock port deletion");
        c->port=NULL;bootstrap.clock_signal_owned=0;
        if(bootstrap.task->tc_SigAlloc&bootstrap.clock_mask)entry_hold("clock signal ownership did not restore");
    }
}
static void entry_os_end(void)
{
    unsigned n=0;enum pt_alarm_result result;
    entry_require_task("original task changed before allocator or OS teardown");
    entry_task_sample(ENTRY_TASK_STACK_TEARDOWN);
    entry_task_stack_report();
    /* Finish precedes deletion of guarded OS extents. It does not certify
     * source quiet or OS restoration; any later uncertain close holds the
     * complete still-live original process/bootstrap/library world.
     */
    if(bootstrap.memory_initialized){
        int finished=pt_private_native_memory_finish(bootstrap.memory);
        entry_require_task("original task changed during checked finish");
        if(finished!=PT_PRIVATE_MEMORY_OK)
            entry_hold("checked registry cannot finish with exact zero ownership");
        bootstrap.finished=1;
        entry_write("NATIVE RAM ENTRY CHECKED FINISH: live=");entry_hex(bootstrap.memory->live);
        entry_write(" used=");entry_hex(bootstrap.memory->pool.used);
        entry_write(" allocations=");entry_hex(bootstrap.memory->allocations);
        entry_write(" releases=");entry_hex(bootstrap.memory->releases);entry_write("\n");
    }
    if(bootstrap.termination.pending){
        result=entry_termination_cancel(); /* AbortIO at most once */
        while(result==PT_ALARM_WAITING&&n++<50){
            entry_require_task("original task changed before termination service delay");
            Delay(1);entry_require_task("original task changed during termination service delay");
            result=entry_termination_poll();
        }
        if(result==PT_ALARM_WAITING||result==PT_ALARM_ERROR||bootstrap.termination.pending)
            entry_hold("termination request did not positively return; no retry-close");
    }
    entry_termination_close();entry_clock_close();
    if(bootstrap.completion_signal>=0){
        entry_require_task("original task changed before completion signal release");
        if(!bootstrap.completion_signal_owned||bootstrap.completion_signal>=32||
           (1UL<<bootstrap.completion_signal)!=bootstrap.completion_mask||
           !(bootstrap.task->tc_SigAlloc&bootstrap.completion_mask))entry_hold("completion signal ownership is unproven");
        SetSignal(0,bootstrap.completion_mask);
        entry_require_task("original task changed during completion signal clear");
        FreeSignal(bootstrap.completion_signal);
        entry_require_task("original task changed during completion signal free");
        bootstrap.completion_signal=-1;bootstrap.completion_signal_owned=0;
        if(bootstrap.task->tc_SigAlloc&bootstrap.completion_mask)entry_hold("completion signal ownership did not restore");
    }else if(bootstrap.completion_signal_owned){
        entry_hold("accepted completion signal identity was lost");
    }
    if(!entry_task_current()||bootstrap.task->tc_SigAlloc!=bootstrap.original_signals)
        entry_hold("original task/stack/signal ownership did not restore");
    if(!bootstrap.original_control||bootstrap.original_control!=bootstrap.memory||
       !entry_fast(bootstrap.original_control,bootstrap.original_control_bytes))entry_hold("bootstrap final ownership changed");
    /* Separate bootstrap owner, never helper self-release or invented header. */
    entry_require_task("original task changed before bootstrap control release");
    FreeMem(bootstrap.original_control,(ULONG)bootstrap.original_control_bytes);
    entry_require_task("original task changed during bootstrap control release");
    bootstrap.original_control=NULL;bootstrap.memory=NULL;
}
static void entry_report(const struct entry_state *s)
{
    unsigned i;
    entry_write("NATIVE RAM ENTRY OBSERVATION: session=");entry_hex(s->session);
    entry_write(" generation=");entry_hex(ENTRY_GENERATION);entry_write(" frame=");entry_hex(ENTRY_FRAME);
    entry_write(" epoch=");entry_hex(s->grid.epoch);entry_write(" first=");entry_hex(s->first);
    entry_write(" last=");entry_hex(s->last);entry_write(" observed=");entry_hex(s->observed);
    entry_write(" issued=");entry_hex(s->issued);entry_write(" frequency=");entry_hex(s->grid.frequency);entry_write("\n");
    for(i=0;i<4;++i){const struct pt_readers_key *k=s->key+i;
        entry_write("NATIVE RAM ENTRY FULL KEY: queue=");entry_hex((uintptr_t)k->queue);
        entry_write(" session=");entry_hex(k->session);entry_write(" generation=");entry_hex(k->generation);
        entry_write(" trigger=");entry_hex(k->trigger);entry_write(" owner=");entry_hex(k->owner);
        entry_write(" serial=");entry_hex(k->serial);entry_write(" action=");entry_hex(k->action);
        entry_write(" slot=");entry_hex(k->slot);entry_write("\n");
    }
    entry_write("NATIVE RAM ENTRY RAW BRACKET: before=");entry_hex(((uint64_t)s->irq->before.hi<<32)|s->irq->before.lo);
    entry_write(" after=");entry_hex(((uint64_t)s->irq->after.hi<<32)|s->irq->after.lo);
    entry_write(" dispatch-reads=");entry_hex(s->port->trace_count);entry_write("\n");
}
int main(int argc,char **argv)
{
    struct entry_state *s=NULL;uint64_t session;int success=0;
    if(argc!=2||!entry_session(argv[1],&session)){
        entry_write("NATIVE RAM ENTRY REFUSED: exactly one fresh nonzero hexadecimal session required\n");return 20;
    }
    if(!entry_bootstrap_open())return 20;
    if(!entry_os_open()||!entry_memory_open())goto clean;
    s=entry_storage(session);
    if(!entry_storage_complete(s)||!entry_deadlines(s)||!entry_core_open(s)||!entry_arm_publish(s))goto clean;
    if(!entry_wait(s)||!entry_actual(s))entry_hold("actual IRQ/window/whole-batch observation failed");
    if(!entry_observe_and_detach(s)||!entry_retire(s))entry_hold("independent actual domain proof refused");
    s->success=1;entry_report(s);success=1;
clean:
    entry_require_task("original task changed before any cleanup branch");
    if(s&&s->acquired&&!bootstrap.source_closed)entry_source_end(s);
    if(!entry_storage_end(s))entry_hold("ordered source/queue/ledger/holder teardown uncertain");
    entry_os_end();
    if(success)entry_write("NATIVE RAM ENTRY SOFTWARE OWNERSHIP PASS: original exact window, genuine four-reader adoption, independent detachment/retirement and zero checked ownership; no Paula DMA/audio/WCET/stack qualification\n");
    else entry_write("NATIVE RAM ENTRY CLEAN REFUSAL: no rebased/immediate/catch-up activation\n");
    return success?0:20;
}
