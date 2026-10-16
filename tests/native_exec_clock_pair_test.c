#include "native_exec_memory.h"
#include "../src/native/eclock.h"
#include "clock_pair.h"
#include <proto/dos.h>
#include <stddef.h>
#include <string.h>
#define PT_CLOCK_PAIR_EMBEDDED
#include "clock_pair_test.c"
struct clock_pair_payload {struct Device *timer;struct pt_clock_pair pair;};
#define OFFSET(type,field,n) typedef char offset_##type##_##field[(offsetof(struct type,field)==n)?1:-1]
OFFSET(clock_pair_payload,timer,0);OFFSET(clock_pair_payload,pair,4);
OFFSET(pt_clock_pair,first,0);OFFSET(pt_clock_pair,second,12);
OFFSET(pt_clock_sample,hi,0);OFFSET(pt_clock_sample,lo,4);OFFSET(pt_clock_sample,frequency,8);
typedef char sample_size[(sizeof(struct pt_clock_sample)==12)?1:-1];
typedef char pair_size[(sizeof(struct pt_clock_pair)==24)?1:-1];
typedef char payload_size[(sizeof(struct clock_pair_payload)==28)?1:-1];
extern void pt_diagnostic_clock_pair(struct clock_pair_payload *);
static int clock_pair_hold(void) __attribute__((noreturn));
static int clock_pair_hold(void)
{puts("CLOCK PAIR HOLD: task/clock/memory restoration unresolved");fflush(stdout);for(;;)Delay(50);__builtin_unreachable();}
int main(void)
{
    struct Task *task=FindTask(NULL);BYTE priority=task?task->tc_Node.ln_Pri:0;
    BYTE idnest=SysBase->IDNestCnt,tdnest=SysBase->TDNestCnt;ULONG signals=task?task->tc_SigAlloc:0;
    struct pt_native_eclock clock={0};struct clock_pair_payload *trace=NULL;
    struct pt_clock_pair_result results[16];unsigned i,attempted=0,completed=0,valid=0,failures=0;int rc=0;
#define CHECK(test) do{if(!(test)){printf("CLOCK PAIR FAIL line=%u attempted=%u completed=%u\n",(unsigned)__LINE__,attempted,completed);rc=20;goto done;}}while(0)
    CHECK(task && idnest==-1 && tdnest==-1);clock_pair_fixture();native_memory_start();
    trace=native_allocate(16*sizeof(*trace));memset(trace,0,16*sizeof(*trace));memset(results,0,sizeof(results));
    CHECK(pt_native_eclock_open(&clock) && clock.port->mp_SigTask==task);
    for(i=0;i<16;++i){
        CHECK(!(SetSignal(0,0)&SIGBREAKF_CTRL_C));
        trace[i].timer=clock.request->tr_node.io_Device;attempted=i+1;
        Disable();pt_diagnostic_clock_pair(trace+i);Enable();
        CHECK(FindTask(NULL)==task && task->tc_Node.ln_Pri==priority && SysBase->IDNestCnt==idnest && SysBase->TDNestCnt==tdnest);
        CHECK(pt_clock_pair_analyse(&trace[i].pair,results+i));++completed;
        if(!results[i].status && (trace[i].pair.first.frequency==709379 || trace[i].pair.first.frequency==715909))++valid;
        else ++failures;
    }
    if(valid!=16 || failures)rc=20;
done:
    pt_native_eclock_close(&clock);
    if(!task || FindTask(NULL)!=task || task->tc_Node.ln_Pri!=priority || task->tc_SigAlloc!=signals ||
       SysBase->IDNestCnt!=idnest || SysBase->TDNestCnt!=tdnest || clock.port || clock.request || clock.opened)return clock_pair_hold();
    for(i=0;i<completed;++i){const struct pt_clock_pair *p=&trace[i].pair;const struct pt_clock_pair_result *r=results+i;
        printf("CLOCK PAIR OBS index=%u first=%lu:%lu:%lu second=%lu:%lu:%lu status=%lu delta=%lu:%lu\n",i,
            (unsigned long)p->first.hi,(unsigned long)p->first.lo,(unsigned long)p->first.frequency,
            (unsigned long)p->second.hi,(unsigned long)p->second.lo,(unsigned long)p->second.frequency,
            (unsigned long)r->status,(unsigned long)r->delta_hi,(unsigned long)r->delta_lo);}
    printf("CLOCK PAIR CLOSED attempted=%u completed=%u valid=%u failures=%u task_identity=1 priority=%d unchanged=1 signals_restored=1 clock_closed=1 interrupt_nesting_restored=1 task_nesting_restored=1\n",attempted,completed,valid,failures,(int)priority);
    native_release(trace);if(native_started)native_memory_finish();
    if(!rc)puts("CLOCK PAIR COMPLETE: sixteen adjacent direct clock observations; no scheduling, IRQ WCET, audio or hardware timing acceptance");
    return rc;
#undef CHECK
}
