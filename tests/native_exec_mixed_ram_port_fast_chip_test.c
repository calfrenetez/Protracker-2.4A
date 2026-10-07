/* Optional distinct Fast/Chip native Exec wrapper: SOURCE_ONLY_NOT_RUN.
 * Link native_mixed_ram_port.c separately. This file includes only the genuine
 * test fixture/resources and calls its explicit named entry once.
 * Masters/control/fake card use bounded Fast RAM; selective Paula copies use
 * actual Chip RAM. Scripted adapter only: no CIA/IRQ/DMA/device/audio access.
 */
#ifdef NDEBUG
#error The genuine native RAM fixture requires assertions enabled.
#endif
#include "native_exec_memory.h"
#include <exec/tasks.h>
#include <limits.h>
#include <string.h>
#include "../src/native/paula_memory.h"
/* Preinclude the genuine declaration before the fixture-only bind macro. */
#include "../src/editor/sampler_paula.h"

#define NM_STACK_REQUIRED 65536UL
/* Both inspected pinned SDK Task declarations document upper bound + 2.
 * This is a conservative admission interpretation, not runtime stack proof.
 * Native build review must rebind the actual selected SDK declaration.
 */
#define NM_STACK_UPPER_BIAS 2UL
struct nm_exec_context {
    struct Task *task;
    uintptr_t lower,reported_upper,guard_upper;
    BYTE priority;
};
static unsigned nm_calls,nm_releases,nm_live,nm_extent_checks;

static uintptr_t nm_stack_pointer(void)
{
    uintptr_t value;
    __asm__ volatile ("move.l %%sp,%0" : "=r" (value) : : "cc");
    return value;
}
static int nm_context_observe(struct nm_exec_context *out,unsigned first)
{
    struct Task *task=FindTask(NULL);
    uintptr_t lower,reported,guard,sp;
    if(!task)return 0;
    lower=(uintptr_t)task->tc_SPLower;
    reported=(uintptr_t)task->tc_SPUpper;
    if(!lower || reported<=NM_STACK_UPPER_BIAS)return 0;
    guard=reported-NM_STACK_UPPER_BIAS;
    if(guard<=lower || guard-lower<NM_STACK_REQUIRED ||
       (lower&1U) || (guard&1U))return 0;
    sp=nm_stack_pointer();
    if((sp&1U) || sp<lower || sp>=guard)return 0;
    if(first){
        out->task=task;out->lower=lower;out->reported_upper=reported;
        out->guard_upper=guard;out->priority=task->tc_Node.ln_Pri;
        return 1;
    }
    return task==out->task && lower==out->lower &&
        reported==out->reported_upper && guard==out->guard_upper &&
        task->tc_Node.ln_Pri==out->priority;
}
static void *nm_allocate(size_t bytes)
{
    void *p;uintptr_t address,last;
    if(!bytes)return NULL;
    assert(nm_calls<UINT_MAX && nm_live<UINT_MAX && nm_extent_checks<UINT_MAX);
    p=native_allocate(bytes);
    address=(uintptr_t)p;
    assert(address && bytes-1U<=UINTPTR_MAX-address);
    last=address+bytes-1U;
    assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST);
    assert((TypeOfMem((APTR)last)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST);
    ++nm_calls;++nm_live;++nm_extent_checks;
    return p;
}
static void *nm_callocate(size_t count,size_t bytes)
{
    void *p;size_t total;
    if(!count || !bytes || count>SIZE_MAX/bytes)return NULL;
    total=count*bytes;p=nm_allocate(total);
    if(p)memset(p,0,total);
    return p;
}
static void nm_release(void *p)
{
    if(!p)return;
    assert(nm_live && nm_releases<UINT_MAX);
    --nm_live;++nm_releases;native_release(p);
}

/* Chip buffers have exact pointer/context/size records allocated through the
 * same bounded Fast pool. No fixed record cap is substituted for real memory
 * admission. The 4096 limit remains the inherited software cache budget per
 * binding; it is not a global Chip capacity, DMA or reservation claim.
 */
struct nm_chip_record {
    struct nm_chip_record *next;
    void *data;void *context;size_t bytes;
};
static struct nm_chip_record *nm_chip_records;
static size_t nm_chip_live_bytes;
static unsigned nm_chip_calls,nm_chip_releases,nm_chip_live;
static unsigned nm_chip_extent_checks,nm_chip_bindings;
static void *nm_chip_allocate(void *context,size_t bytes)
{
    struct nm_chip_record *record;void *p;uintptr_t address,last;
    unsigned *count=context;
    if(!count || !bytes || bytes>UINT32_MAX ||
       bytes>pt_paula_chip_available())return NULL;
    assert(*count<UINT_MAX && nm_chip_calls<UINT_MAX &&
        nm_chip_live<UINT_MAX && nm_chip_extent_checks<UINT_MAX &&
        bytes<=SIZE_MAX-nm_chip_live_bytes);
    record=nm_allocate(sizeof(*record));
    p=AllocMem((ULONG)bytes,MEMF_CHIP|MEMF_PUBLIC);
    if(!p){nm_release(record);return NULL;}
    address=(uintptr_t)p;
    assert(address && bytes-1U<=UINTPTR_MAX-address);
    last=address+bytes-1U;
    assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
    assert((TypeOfMem((APTR)last)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
    record->data=p;record->context=context;record->bytes=bytes;
    record->next=nm_chip_records;nm_chip_records=record;
    ++*count;++nm_chip_calls;++nm_chip_live;++nm_chip_extent_checks;
    nm_chip_live_bytes+=bytes;
    return p;
}
static void nm_chip_release(void *context,void *p,size_t bytes)
{
    struct nm_chip_record **link=&nm_chip_records,*record;
    unsigned *count=context;uintptr_t address,last;
    assert(count && p && bytes && nm_chip_live && *count &&
        bytes<=nm_chip_live_bytes && nm_chip_releases<UINT_MAX);
    while(*link && (*link)->data!=p)link=&(*link)->next;
    assert(*link);record=*link;
    assert(record->context==context && record->bytes==bytes);
    address=(uintptr_t)p;
    assert(bytes-1U<=UINTPTR_MAX-address);last=address+bytes-1U;
    assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
    assert((TypeOfMem((APTR)last)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
    *link=record->next;
    --*count;--nm_chip_live;++nm_chip_releases;nm_chip_live_bytes-=bytes;
    FreeMem(p,(ULONG)bytes);nm_release(record);
}
static int nm_bind_paula(struct pt_sampler_paula *,struct pt_sampler *,struct pt_project *,
    void *,void *(*)(void *,size_t),void (*)(void *,void *,size_t),size_t);

/* An explicit entry hook is needed: nested inherited helper includes define
 * and undefine main. All fixture malloc/calloc/free use the bounded Fast pool.
 * Only the verified inherited Paula bind call is intercepted. Separately
 * compiled production C and its real declarations are not macro-transformed.
 */
#define PT_NATIVE_MIXED_RAM_PORT_TEST_ENTRY nm_fixture_once
#define malloc nm_allocate
#define calloc nm_callocate
#define free nm_release
#define pt_sampler_paula_bind nm_bind_paula
#include "native_mixed_ram_port_test.c"
#undef pt_sampler_paula_bind
#undef free
#undef calloc
#undef malloc
#undef PT_NATIVE_MIXED_RAM_PORT_TEST_ENTRY
#ifndef PT_NATIVE_MIXED_RAM_PORT_TEST_VERSION
#error The genuine mixed RAM fixture lacks the reviewed explicit entry version.
#elif PT_NATIVE_MIXED_RAM_PORT_TEST_VERSION != 1
#error Unreviewed mixed RAM fixture entry version.
#endif

/* This depends deliberately on the frozen inherited fixture's exact resources
 * layout and callbacks. An unexpected binding fails its existing assert; it
 * does not silently turn another software allocator into a Chip allocator.
 * There is no copy, close/rebind, or bypass of production bind validation.
 */
static int nm_bind_paula(struct pt_sampler_paula *paula,struct pt_sampler *sampler,
    struct pt_project *project,void *context,void *(*allocate)(void *,size_t),
    void (*release)(void *,void *,size_t),size_t budget)
{
    struct resources *r;uintptr_t address=(uintptr_t)paula;int result;
    if(!paula || !sampler || !project || !context ||
       address<offsetof(struct resources,paula) ||
       allocate!=chip_new || release!=chip_drop || budget!=4096)return 0;
    r=(struct resources *)(address-offsetof(struct resources,paula));
    if(paula!=&r->paula || sampler!=&r->sampler ||
       project!=&r->document.project || context!=&r->chips || r->chips)return 0;
    assert(nm_chip_bindings<UINT_MAX);
    result=pt_sampler_paula_bind(paula,sampler,project,context,
        nm_chip_allocate,nm_chip_release,budget);
    if(result)++nm_chip_bindings;
    return result;
}

static int nm_failure(const char *message)
{
    fprintf(stderr,"EXEC MIXED FAST CHIP FAIL: %s\n",message);
    return 20;
}
int main(void)
{
    struct nm_exec_context context;int result;
    memset(&context,0,sizeof(context));
    if(!nm_context_observe(&context,1))return nm_failure("original task/stack admission");
    puts("EXEC MIXED FAST CHIP BEGIN: genuine fixture; bounded Fast masters/control; selective Chip copies; scripted clock; no IRQ/DMA/device/audio");
    printf("EXEC MIXED RAM CONTEXT: reported_stack_bytes=%lu guard_stack_bytes=%lu priority=%d\n",
        (unsigned long)(context.reported_upper-context.lower),
        (unsigned long)(context.guard_upper-context.lower),(int)context.priority);
    if(fflush(stdout))return nm_failure("BEGIN output persistence");
    native_memory_start();
    if(fflush(stdout))return nm_failure("memory-start output persistence");
    result=nm_fixture_once();
    if(result)return nm_failure("genuine fixture returned failure");
    if(!nm_context_observe(&context,0))return nm_failure("original task/stack/priority changed");
    if(!nm_calls || nm_live || nm_releases!=nm_calls ||
       nm_extent_checks!=nm_calls || native_allocations!=nm_calls || native_pool.used)
        return nm_failure("Fast allocation/release census mismatch");
    if(!nm_chip_bindings || !nm_chip_calls || nm_chip_live || nm_chip_live_bytes ||
       nm_chip_records || nm_chip_releases!=nm_chip_calls ||
       nm_chip_extent_checks!=nm_chip_calls)
        return nm_failure("Chip allocation/release census mismatch");
    printf("EXEC MIXED CHIP MEMORY PASS: %u bindings, %u Chip allocations, first/final-byte checks, zero owned Chip bytes; no DMA\n",
        nm_chip_bindings,nm_chip_calls);
    native_memory_finish();
    puts("EXEC MIXED FAST CHIP PASS: genuine fixture once; Fast/Chip extents checked; zero owned bytes; original task/stack/priority; RAM_ONLY");
    if(fflush(stdout))return nm_failure("PASS output persistence");
    return 0;
}
