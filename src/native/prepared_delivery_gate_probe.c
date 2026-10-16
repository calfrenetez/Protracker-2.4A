/* Normal original-TASK probe only. No IRQ/Cause/exception entry, fake Exec,
 * artificial nesting/IPL, timer, device, source owner or filesystem sidecar. */
#include "prepared_delivery_gate.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <assert.h>
#include <stdio.h>

#define PROBE_STORAGE 2048U
#define PROBE_TAIL 32U
#define PASS_MARKER "PREPARED DELIVERY GATE PASS:4 staged,4 local records,4 once-only observations;normal TASK Exec balance;no IRQ/output/source/quiet authority"
union gate_storage {uint64_t alignment;void *pointer;unsigned char bytes[PROBE_STORAGE];};
static union gate_storage storage,stop_storage;

static int held(void) __attribute__((noreturn));
static int held(void)
{
    /* An uncertain pair must not print, Wait, exit or force Enable. Keep this
     * process, gate and tokens alive for separately authorized own recovery. */
    for(;;)__asm__ volatile ("" ::: "memory");
    __builtin_unreachable();
}
static int same_task(struct ExecBase *base,struct Task *task,int idnest,int tdnest)
{
    return SysBase==base && base->ThisTask==task &&
        (int)(signed char)base->IDNestCnt==idnest &&
        (int)(signed char)base->TDNestCnt==tdnest;
}
static int failed(void)
{puts("PREPARED DELIVERY GATE FAIL:normal TASK metadata check");return 20;}

int main(void)
{
    struct ExecBase *base=SysBase;struct Task *task;
    struct pt_native_delivery_gate *gate=(void *)storage.bytes,*stop_gate=(void *)stop_storage.bytes;
    struct pt_native_delivery_gate_token token={0},copy={0},spare={0};
    struct pt_native_delivery_gate_record records[4],observed,unchanged;
    enum pt_native_delivery_gate_result result;
    size_t size=pt_native_delivery_gate_size(),alignment=pt_native_delivery_gate_alignment(),i;
    unsigned bad=0;int idnest,tdnest;
    if(!base || !(task=FindTask(NULL)) || base->ThisTask!=task)return held();
    idnest=(int)(signed char)base->IDNestCnt;tdnest=(int)(signed char)base->TDNestCnt;
    if(idnest!=-1 || tdnest!=-1)return held();
    assert(sizeof(uint64_t)==8);
    assert(PT_NATIVE_DELIVERY_GATE_SLOTS==4);
    if(!alignment || size>PROBE_STORAGE-PROBE_TAIL ||
       (uintptr_t)gate%alignment || (uintptr_t)stop_gate%alignment)return failed();
    for(i=0;i<PROBE_TAIL;++i){storage.bytes[size+i]=0xa5;stop_storage.bytes[size+i]=0x5a;}
    for(i=0;i<4;++i)records[i]=(struct pt_native_delivery_gate_record){
        UINT64_C(0x100000001)+i,UINT64_C(0x200000003)+i,
        UINT64_C(0x300000005)+i,(int32_t)(17+i),(uint32_t)(0x40+i)};
    unchanged=(struct pt_native_delivery_gate_record){99,98,97,-9,96};

#define CLOSED() do{if(!same_task(base,task,idnest,tdnest))return held();}while(0)
#define MASKED() do{if(!same_task(base,task,idnest+1,tdnest))return held();}while(0)
#define OUTSIDE(call,want) do{result=(call);if(result==PT_NATIVE_DELIVERY_GATE_FAULT)return held();CLOSED();if(result!=(want))return failed();}while(0)
#define INSIDE(call,want) do{result=(call);if(result==PT_NATIVE_DELIVERY_GATE_FAULT)return held();MASKED();if(result!=(want))bad=1;}while(0)
#define TASK_ENTER(which) do{result=pt_native_delivery_gate_task_enter((which),&token);if(result==PT_NATIVE_DELIVERY_GATE_FAULT)return held();if(result!=PT_NATIVE_DELIVERY_GATE_OK){CLOSED();return failed();}MASKED();}while(0)
#define TASK_LEAVE(which) do{result=pt_native_delivery_gate_task_leave((which),&token);if(result!=PT_NATIVE_DELIVERY_GATE_OK)return held();CLOSED();if(bad)return failed();}while(0)

    OUTSIDE(pt_native_delivery_gate_init(gate,size+PROBE_TAIL,UINT64_C(0x100000007)),PT_NATIVE_DELIVERY_GATE_OK);
    OUTSIDE(pt_native_delivery_gate_init(stop_gate,size+PROBE_TAIL,UINT64_C(0x100000009)),PT_NATIVE_DELIVERY_GATE_OK);
    TASK_ENTER(gate);
    for(i=0;i<4;++i)INSIDE(pt_native_delivery_gate_stage(gate,&token,(unsigned)i,records[i].ticket),PT_NATIVE_DELIVERY_GATE_OK);
    INSIDE(pt_native_delivery_gate_task_enter(gate,&spare),PT_NATIVE_DELIVERY_GATE_REFUSED);
    INSIDE(pt_native_delivery_gate_local_enter(gate,0,records[0].ticket,&spare),PT_NATIVE_DELIVERY_GATE_REFUSED);
    copy=token;
    INSIDE(pt_native_delivery_gate_task_leave(gate,&copy),PT_NATIVE_DELIVERY_GATE_REFUSED);
    TASK_LEAVE(gate);
    OUTSIDE(pt_native_delivery_gate_task_leave(gate,&token),PT_NATIVE_DELIVERY_GATE_REFUSED);

    for(i=0;i<4;++i){
        result=pt_native_delivery_gate_local_enter(gate,(unsigned)i,records[i].ticket,&token);
        if(result==PT_NATIVE_DELIVERY_GATE_FAULT)return held();
        if(result!=PT_NATIVE_DELIVERY_GATE_OK){CLOSED();return failed();}
        MASKED();copy=token;
        INSIDE(pt_native_delivery_gate_task_enter(gate,&spare),PT_NATIVE_DELIVERY_GATE_REFUSED);
        INSIDE(pt_native_delivery_gate_local_enter(gate,(unsigned)i,records[i].ticket,&spare),PT_NATIVE_DELIVERY_GATE_REFUSED);
        INSIDE(pt_native_delivery_gate_local_record(gate,&token,records+i),PT_NATIVE_DELIVERY_GATE_OK);
        INSIDE(pt_native_delivery_gate_local_record(gate,&token,records+i),PT_NATIVE_DELIVERY_GATE_REFUSED);
        INSIDE(pt_native_delivery_gate_local_leave(gate,&copy),PT_NATIVE_DELIVERY_GATE_REFUSED);
        result=pt_native_delivery_gate_local_leave(gate,&token);
        if(result!=PT_NATIVE_DELIVERY_GATE_OK)return held();
        CLOSED();if(bad)return failed();
        OUTSIDE(pt_native_delivery_gate_local_leave(gate,&token),PT_NATIVE_DELIVERY_GATE_REFUSED);
        OUTSIDE(pt_native_delivery_gate_local_enter(gate,(unsigned)i,records[i].ticket,&spare),PT_NATIVE_DELIVERY_GATE_REFUSED);
    }
    TASK_ENTER(gate);
    for(i=0;i<4;++i){
        observed=unchanged;
        INSIDE(pt_native_delivery_gate_observe(gate,&token,(unsigned)i,&observed),PT_NATIVE_DELIVERY_GATE_OK);
        if(observed.ticket!=records[i].ticket || observed.observed!=records[i].observed ||
           observed.issued!=records[i].issued || observed.result!=records[i].result ||
           observed.detail!=records[i].detail)bad=1;
        observed=unchanged;
        INSIDE(pt_native_delivery_gate_observe(gate,&token,(unsigned)i,&observed),PT_NATIVE_DELIVERY_GATE_REFUSED);
        if(observed.ticket!=unchanged.ticket || observed.observed!=unchanged.observed ||
           observed.issued!=unchanged.issued || observed.result!=unchanged.result ||
           observed.detail!=unchanged.detail)bad=1;
    }
    INSIDE(pt_native_delivery_gate_stop_admission(gate,&token),PT_NATIVE_DELIVERY_GATE_OK);
    TASK_LEAVE(gate);

    /* Separate empty gate proves stop refuses a NEW, otherwise eligible slot;
     * the four populated slots above are never reset or reused. */
    TASK_ENTER(stop_gate);
    INSIDE(pt_native_delivery_gate_stop_admission(stop_gate,&token),PT_NATIVE_DELIVERY_GATE_OK);
    INSIDE(pt_native_delivery_gate_stop_admission(stop_gate,&token),PT_NATIVE_DELIVERY_GATE_OK);
    INSIDE(pt_native_delivery_gate_stage(stop_gate,&token,0,77),PT_NATIVE_DELIVERY_GATE_REFUSED);
    TASK_LEAVE(stop_gate);
    for(i=0;i<PROBE_TAIL;++i)
        if(storage.bytes[size+i]!=0xa5 || stop_storage.bytes[size+i]!=0x5a)return failed();
    CLOSED();puts(PASS_MARKER);return 0;
#undef CLOSED
#undef MASKED
#undef OUTSIDE
#undef INSIDE
#undef TASK_ENTER
#undef TASK_LEAVE
}
