/* Distinct optional Exec memory/layout wrapper for the genuine34 causal pair.
 * Source only. No native execution, compiler result or target admission here.
 * Scripted clocks/card RAM remain models; no CIA/timer/IRQ/MMIO/DMA/audio. */
#ifdef NDEBUG
#error The genuine native hook composition requires assertions enabled.
#endif
#include "native_exec_memory.h"
#include <exec/tasks.h>
#include <limits.h>
#include <string.h>
#include "../src/native/paula_memory.h"
#define NC_STACK_REQUIRED 65536UL
#define NC_STACK_UPPER_BIAS 2UL
struct nc_context {
    struct Task *task;uintptr_t lower,reported,guard;BYTE priority;
};
static size_t nc_chip_used;
static unsigned nc_chip_allocations,nc_chip_releases;
static uintptr_t nc_sp(void)
{
    uintptr_t value;__asm__ volatile ("move.l %%sp,%0" : "=r" (value) : : "cc");return value;
}
static int nc_observe(struct nc_context *out,unsigned first)
{
    struct Task *task=FindTask(NULL);uintptr_t lower,reported,guard,sp;
    if(!task)return 0;
    lower=(uintptr_t)task->tc_SPLower;reported=(uintptr_t)task->tc_SPUpper;
    if(!lower || reported<=NC_STACK_UPPER_BIAS)return 0;
    guard=reported-NC_STACK_UPPER_BIAS;
    if(guard<=lower || guard-lower<NC_STACK_REQUIRED || (lower&1U) || (guard&1U))return 0;
    sp=nc_sp();if((sp&1U) || sp<lower || sp>=guard)return 0;
    if(first){out->task=task;out->lower=lower;out->reported=reported;
        out->guard=guard;out->priority=task->tc_Node.ln_Pri;return 1;}
    return task==out->task && lower==out->lower && reported==out->reported &&
        guard==out->guard && task->tc_Node.ln_Pri==out->priority;
}
static void *nch_platform_allocate(size_t bytes,unsigned kind)
{
    void *p;
    if(kind==1U)return native_allocate(bytes);
    assert(kind==2U);
    if(!bytes || bytes>UINT32_MAX || bytes>pt_paula_chip_available())return NULL;
    assert(bytes<=SIZE_MAX-nc_chip_used && nc_chip_allocations<UINT_MAX);
    p=AllocMem((ULONG)bytes,MEMF_CHIP|MEMF_PUBLIC);if(!p)return NULL;
    nc_chip_used+=bytes;++nc_chip_allocations;return p;
}
static int nch_platform_kind(void *p,size_t bytes,unsigned kind)
{
    uintptr_t address=(uintptr_t)p,last;ULONG flags;
    if(!address || !bytes || bytes-1U>UINTPTR_MAX-address || (kind!=1U && kind!=2U))return 0;
    last=address+bytes-1U;flags=kind==1U?MEMF_FAST:MEMF_CHIP;
    return (TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==flags &&
        (TypeOfMem((APTR)last)&(MEMF_FAST|MEMF_CHIP))==flags;
}
static void nch_platform_release(void *p,size_t bytes,unsigned kind)
{
    if(kind==1U){native_release(p);return;}
    assert(kind==2U && p && bytes && bytes<=UINT32_MAX && bytes<=nc_chip_used &&
        nc_chip_releases<UINT_MAX);
    nc_chip_used-=bytes;++nc_chip_releases;FreeMem(p,(ULONG)bytes);
}
static size_t nch_platform_chip_available(void)
{return pt_paula_chip_available();}
static int nch_platform_empty(void)
{return !native_pool.used && !nc_chip_used;}
struct cp_trial;
static void *nch_allocate(size_t);
static void *nch_callocate(size_t,size_t);
static void nch_release(void *);
static void nch_configure_chip(struct cp_trial *);
static void nch_case_begin(unsigned,unsigned,unsigned,unsigned,unsigned);
static void nch_case_end(unsigned);
#define NCH_TRACE_PREFIX "EXEC MIXED CAUSAL RAM"
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN nc_fixture_once
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_CALLOC nch_callocate
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CONFIGURE_CHIP nch_configure_chip
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CASE_BEGIN nch_case_begin
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CASE_END nch_case_end
#define malloc nch_allocate
#define free nch_release
#include "native_mixed_causal_ram_port_test.c"
#undef free
#undef malloc
#undef calloc
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CASE_END
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CASE_BEGIN
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CONFIGURE_CHIP
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_CALLOC
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN
#ifndef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_VERSION
#error Missing reviewed genuine34 fixture hook version.
#elif PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_VERSION != 1U
#error Unreviewed genuine34 fixture hook version.
#endif
#include "native_mixed_causal_ram_hooks_internal.h"
static int nc_failure(const char *message)
{fprintf(stderr,"EXEC MIXED CAUSAL FAST CHIP FAIL: %s\n",message);return 20;}
int main(void)
{
    struct nc_context context;int result;
    memset(&context,0,sizeof(context));
    if(!nc_observe(&context,1))return nc_failure("original task/stack admission");
    assert(sizeof(void *)==4U && sizeof(size_t)==4U && sizeof(unsigned)==4U &&
        sizeof(uint64_t)==8U && offsetof(struct cr_carrier,trial)==0 &&
        sizeof(cr_current->port.command)/sizeof(cr_current->port.command[0])==2U &&
        sizeof(cr_current->port.reader)/sizeof(cr_current->port.reader[0])==32U &&
        sizeof(cr_current->port.slot)/sizeof(cr_current->port.slot[0])==20U &&
        sizeof(cr_current->port.command[0].packet.action)/sizeof(cr_current->port.command[0].packet.action[0])==16U &&
        sizeof(cr_current->port.trace)/sizeof(cr_current->port.trace[0])==64U);
    puts("EXEC MIXED CAUSAL FAST CHIP BEGIN: genuine34 once;bounded Fast masters/control;selective Chip caches;scripted clocks/card;no IRQ/DMA/device/audio");
    printf("EXEC MIXED CAUSAL RAM CONTEXT: reported_stack_bytes=%lu guard_stack_bytes=%lu priority=%d\n",
        (unsigned long)(context.reported-context.lower),(unsigned long)(context.guard-context.lower),(int)context.priority);
    printf("EXEC MIXED CAUSAL RAM LAYOUT: carrier=%lu port=%lu packet=%lu key=%lu control=%lu carrier_alignment=%lu\n",
        (unsigned long)sizeof(struct cr_carrier),(unsigned long)sizeof(struct pt_private_mixed_causal_ram_port),
        (unsigned long)sizeof(struct pt_mixed_causal_packet),(unsigned long)sizeof(struct pt_mixed_readers_key),
        (unsigned long)sizeof(struct pt_editor_mixed_causal_prepare),(unsigned long)offsetof(struct cr_carrier_alignment,value));
    if(fflush(stdout))return nc_failure("BEGIN/layout output persistence");
    native_memory_start();if(fflush(stdout))return nc_failure("memory-start output persistence");
    result=nc_fixture_once();if(result)return nc_failure("genuine34 fixture returned failure");
    nch_final();
    if(!nc_observe(&context,0))return nc_failure("original task/stack/priority changed");
    assert(native_allocations==nch_fast_calls && !native_pool.used &&
        nc_chip_allocations==52U && nc_chip_releases==52U && !nc_chip_used);
    puts("EXEC MIXED CAUSAL CHIP MEMORY PASS:34 exact pre-begin bindings;52 selective Chip allocations;first/final-byte checks;zero owned Chip bytes;no DMA");
    native_memory_finish();
    puts("EXEC MIXED CAUSAL FAST CHIP PASS:genuine34 once;all allocation hooks;actual Fast/Chip extents and zero owned bytes;original task/stack/priority;RAM_ONLY");
    if(fflush(stdout))return nc_failure("PASS output persistence");
    return 0;
}
