/* Actual selected production lifecycle C, with explicitly fake HOST API edges.
 * No production main, native guard bypass, fake target macro, or native result.
 * Each invocation is one fresh process; no production owner-reset API exists. */
#if defined(__m68k__) || defined(__amigaos__)
#error "This HOST fixture must never become a native qualification or target"
#endif
#define PT_NATIVE_AUDIO_RESERVATION_PRIVATE_SOURCE 1
#include "native_audio_reservation.h"
#include "host_exec_model.h"
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum call {
    FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,OPEN_DEVICE,SET_PREC,
    SEND_LOCK,CHECK_LOCK,FREE_CHANNELS,WAIT_LOCK,CLOSE_DEVICE,
    DELETE_LOCK,DELETE_AUDIO,DELETE_PORT,RETAIN_WAIT
};
enum scenario {
    CONTEXT_REFUSAL=1,PORT_REFUSAL,AUDIO_REFUSAL,LOCK_REFUSAL,
    OPEN_REFUSAL,ALLOC_REFUSAL,OPEN_MISMATCH,OPEN_BAD_DEVICE,
    OPEN_PARTIAL,OPEN_ZERO_KEY,PREC_ERROR,PREC_PARTIAL,LOCK_EARLY,
    SUCCESS,STOLEN_REPLY,FREE_NOALLOCATION,FREE_PARTIAL,FREE_MISMATCH,
    LOCK_PENDING,CHECK_OTHER,LOCK_UNKNOWN,LOCK_MISMATCH,
    LOCK_ZERO_MASK_ERROR,OUTPUT_AND_ONCE,OPEN_WIDE_MASK,FREE_WIDE_MASK,LOCK_WIDE_MASK
};
struct tracked {void *pointer;size_t bytes;unsigned live;};
struct boundary {
    unsigned scenario,requests,opened,sent,completed,consumed,freed,held;
    unsigned expect_hold;
    struct tracked blocks[3]; /* actual model heap: port,audio,lock */
    enum call calls[64];unsigned call_count;
    struct Task task,other_task;
    struct ExecBase exec;
    struct Device device;
    uintptr_t sp;
    jmp_buf retained;
};
static struct boundary model;
struct ExecBase *SysBase;
static void verify_held(void);

static void fail(const char *message) __attribute__((noreturn));
static void fail(const char *message)
{
    unsigned i;
    fprintf(stderr,"HOST24 first failure case=%u: %s; calls=",model.scenario,message);
    for(i=0;i<model.call_count;++i)fprintf(stderr,"%s%u",i?",":"",(unsigned)model.calls[i]);
    fputc('\n',stderr);fflush(stderr);exit(1);
}
#define CHECK(x) do {if(!(x))fail(#x);} while(0)
static void record(enum call call)
{
    CHECK(model.call_count<sizeof(model.calls)/sizeof(model.calls[0]));
    CHECK(!model.held);model.calls[model.call_count++]=call;
}
static unsigned live(void)
{
    return model.blocks[0].live+model.blocks[1].live+model.blocks[2].live;
}
static void *new_block(unsigned slot,size_t bytes)
{
    void *p;
    CHECK(slot<3 && !model.blocks[slot].pointer && !model.blocks[slot].live);
    p=calloc(1,bytes);CHECK(p);
    model.blocks[slot].pointer=p;model.blocks[slot].bytes=bytes;model.blocks[slot].live=1;
    return p;
}
static void drop_block(unsigned slot,void *pointer)
{
    CHECK(slot<3 && model.blocks[slot].live && model.blocks[slot].pointer==pointer);
    free(pointer);model.blocks[slot].pointer=NULL;model.blocks[slot].live=0;
}
static struct IOAudio *audio(void) {return model.blocks[1].pointer;}
static struct IOAudio *lock(void) {return model.blocks[2].pointer;}
static void mask(struct IORequest *request,unsigned value)
{
    request->io_Unit=(struct Unit *)(uintptr_t)value;
}
static void wide_mask(struct IORequest *request,unsigned low)
{
    uintptr_t high=(uintptr_t)UINT32_MAX+1u;
    CHECK(sizeof(uintptr_t)>sizeof(ULONG) && high && !(high&(high-1u)));
    request->io_Unit=(struct Unit *)(high|(uintptr_t)low);
    CHECK((uintptr_t)request->io_Unit>UINT32_MAX);
}
static void require_key(const struct IORequest *request)
{
    const struct IOAudio *a=(const struct IOAudio *)request;
    CHECK(request->io_Device==&model.device && a->ioa_AllocKey==1234);
}

struct Task *FindTask(CONST_STRPTR name)
{
    record(FIND_TASK);CHECK(name==NULL);return model.exec.ThisTask;
}
struct MsgPort *CreateMsgPort(void)
{
    struct MsgPort *port;
    record(CREATE_PORT);
    if(model.scenario==PORT_REFUSAL)return NULL;
    port=new_block(0,sizeof(*port));port->mp_SigTask=&model.task;port->mp_SigBit=7;
    return port;
}
struct IORequest *CreateIORequest(struct MsgPort *port,ULONG bytes)
{
    unsigned slot;
    struct IOAudio *request;
    CHECK(model.requests<2);slot=++model.requests;
    record(slot==1?CREATE_AUDIO:CREATE_LOCK);
    CHECK(model.blocks[0].live && port==model.blocks[0].pointer && bytes==sizeof(*request));
    if((slot==1 && model.scenario==AUDIO_REFUSAL) ||
       (slot==2 && model.scenario==LOCK_REFUSAL))return NULL;
    request=new_block(slot,sizeof(*request));
    request->ioa_Request.io_Message.mn_ReplyPort=port;
    request->ioa_Request.io_Message.mn_Length=(UWORD)sizeof(*request);
    return &request->ioa_Request;
}
BYTE OpenDevice(CONST_STRPTR name,ULONG unit,struct IORequest *request,ULONG flags)
{
    struct IOAudio *a=(struct IOAudio *)request;
    record(OPEN_DEVICE);
    CHECK(model.blocks[1].live && model.blocks[2].live && a==audio());
    CHECK(!strcmp((const char *)name,"audio.device") && unit==0 && flags==0);
    CHECK(request->io_Message.mn_Node.ln_Pri==-128 && a->ioa_AllocKey==0 &&
        a->ioa_Length==1 && a->ioa_Data && *a->ioa_Data==15);
    if(model.scenario==OPEN_REFUSAL || model.scenario==ALLOC_REFUSAL ||
       model.scenario==OPEN_MISMATCH) {
        request->io_Device=(struct Device *)UINTPTR_MAX;
        request->io_Error=model.scenario==OPEN_REFUSAL?-1:ADIOERR_ALLOCFAILED;
        return model.scenario==OPEN_MISMATCH?-1:request->io_Error;
    }
    model.opened=1;request->io_Device=&model.device;request->io_Error=0;
    mask(request,model.scenario==OPEN_PARTIAL?7:15);
    a->ioa_AllocKey=model.scenario==OPEN_ZERO_KEY?0:1234;
    if(model.scenario==OPEN_BAD_DEVICE)request->io_Device=NULL;
    if(model.scenario==OPEN_WIDE_MASK)wide_mask(request,15);
    return 0;
}
BYTE DoIO(struct IORequest *request)
{
    struct IOAudio *a=(struct IOAudio *)request;
    CHECK(a==audio() && model.blocks[1].live && model.opened);
    CHECK(request->io_Flags==0);require_key(request);
    CHECK((uintptr_t)request->io_Unit==15);
    if(request->io_Command==ADCMD_SETPREC) {
        record(SET_PREC);CHECK(request->io_Message.mn_Node.ln_Pri==127 && !model.sent);
        request->io_Flags=IOF_QUICK;
        request->io_Error=model.scenario==PREC_ERROR?ADIOERR_NOALLOCATION:0;
        if(model.scenario==PREC_PARTIAL)mask(request,7);
        return request->io_Error;
    }
    CHECK(request->io_Command==ADCMD_FREE);record(FREE_CHANNELS);
    CHECK(model.sent && !model.freed && !model.consumed);
    request->io_Flags=IOF_QUICK;
    if(model.scenario==FREE_NOALLOCATION) {
        mask(request,0);request->io_Error=ADIOERR_NOALLOCATION;return request->io_Error;
    }
    if(model.scenario==FREE_PARTIAL) {
        mask(request,7);request->io_Error=0;mask(&lock()->ioa_Request,8);return 0;
    }
    if(model.scenario==FREE_MISMATCH) {
        mask(request,15);request->io_Error=ADIOERR_NOALLOCATION;return 0;
    }
    model.freed=1;request->io_Error=0;mask(request,15);
    if(model.scenario==FREE_WIDE_MASK)wide_mask(request,15);
    if(model.scenario==STOLEN_REPLY) {
        CHECK(model.completed && lock()->ioa_Request.io_Error==ADIOERR_CHANNELSTOLEN &&
            (uintptr_t)lock()->ioa_Request.io_Unit==15);
    } else {
        model.completed=model.scenario!=LOCK_PENDING;
        lock()->ioa_Request.io_Error=0;mask(&lock()->ioa_Request,0);
    }
    if(model.scenario==LOCK_UNKNOWN)lock()->ioa_Request.io_Error=7;
    if(model.scenario==LOCK_ZERO_MASK_ERROR)mask(&lock()->ioa_Request,8);
    if(model.scenario==LOCK_WIDE_MASK)wide_mask(&lock()->ioa_Request,0);
    return 0;
}
void SendIO(struct IORequest *request)
{
    record(SEND_LOCK);CHECK(model.opened && !model.sent && !model.freed);
    CHECK(request==&lock()->ioa_Request && request->io_Command==ADCMD_LOCK &&
        request->io_Flags==0 && (uintptr_t)request->io_Unit==15);
    require_key(request);model.sent=1;request->io_Flags=0;
    if(model.scenario==LOCK_EARLY) {
        model.completed=1;request->io_Error=ADIOERR_NOALLOCATION;
    }
}
struct IORequest *CheckIO(const struct IORequest *request)
{
    record(CHECK_LOCK);
    CHECK(model.sent && model.blocks[2].live && request==&lock()->ioa_Request && !model.consumed);
    if(model.freed && model.scenario==CHECK_OTHER)return &audio()->ioa_Request;
    return model.completed?&lock()->ioa_Request:NULL;
}
BYTE WaitIO(struct IORequest *request)
{
    record(WAIT_LOCK);
    CHECK(model.sent && model.completed && model.freed && !model.consumed &&
        request==&lock()->ioa_Request);
    model.consumed=1;
    return model.scenario==LOCK_MISMATCH?ADIOERR_CHANNELSTOLEN:request->io_Error;
}
void CloseDevice(struct IORequest *request)
{
    record(CLOSE_DEVICE);
    CHECK(request==&audio()->ioa_Request && model.opened && model.freed && model.consumed);
    CHECK(model.blocks[1].live && model.blocks[2].live);model.opened=0;
}
void DeleteIORequest(struct IORequest *request)
{
    unsigned slot;
    CHECK(!model.opened && (!model.sent || model.consumed));
    slot=request==model.blocks[2].pointer?2:1;
    record(slot==2?DELETE_LOCK:DELETE_AUDIO);
    CHECK(request->io_Message.mn_Length==sizeof(struct IOAudio));
    drop_block(slot,request);
}
void DeleteMsgPort(struct MsgPort *port)
{
    record(DELETE_PORT);
    CHECK(!model.opened && !model.blocks[1].live && !model.blocks[2].live &&
        (!model.sent || model.consumed));drop_block(0,port);
}
ULONG Wait(ULONG signals)
{
    record(RETAIN_WAIT);CHECK(signals==0 && model.expect_hold);
    verify_held();model.held=1;longjmp(model.retained,1);
}
static uintptr_t current_stack_pointer(void) {return model.sp;}

/* Exact selected C begins at its private owner definition. Only the native
 * SP accessor body is externalized to the labelled boundary above. */
#include "selected_native_audio_algorithm.h"

static void verify_held(void)
{
    CHECK(owner.observed.result==PT_PRIVATE_AUDIO_HELD &&
        owner.observed.stage==PT_PRIVATE_AUDIO_QUARANTINED);
    CHECK(live()==3 && owner.port==model.blocks[0].pointer &&
        owner.audio==model.blocks[1].pointer && owner.lock==model.blocks[2].pointer);
    CHECK(model.blocks[0].live && model.blocks[1].live && model.blocks[2].live);
    CHECK(!model.consumed || model.scenario==LOCK_UNKNOWN ||
        model.scenario==LOCK_MISMATCH || model.scenario==LOCK_ZERO_MASK_ERROR ||
        model.scenario==LOCK_WIDE_MASK);
    CHECK(owner.task==&model.task && owner.exec==&model.exec);
    CHECK(owner.observed.open_completed==1);
    if(model.scenario==OPEN_MISMATCH) {
        CHECK(owner.observed.open_return==-1 && owner.observed.open_error==ADIOERR_ALLOCFAILED);
        CHECK(!owner.opened && !owner.observed.precedence_completed);
    } else {
        CHECK(owner.observed.open_return==0 && owner.observed.open_error==0 && owner.opened);
    }
    if(model.scenario==OPEN_WIDE_MASK) {
        CHECK(owner.observed.allocated_mask==0 && !owner.observed.precedence_completed &&
            !owner.observed.lock_issued && !owner.observed.free_completed && !owner.free_issued);
        CHECK((uintptr_t)owner.audio->ioa_Request.io_Unit>UINT32_MAX &&
            ((uintptr_t)owner.audio->ioa_Request.io_Unit&UINT32_MAX)==15);
        CHECK(!model.sent && !model.freed && !model.consumed);return;
    }
    if(model.scenario==FREE_WIDE_MASK) {
        CHECK(owner.observed.allocated_mask==15 && owner.observed.precedence_completed &&
            owner.observed.lock_issued && owner.observed.free_completed && owner.free_issued &&
            owner.observed.free_return==0 && owner.observed.free_error==0 &&
            owner.observed.freed_mask==0 && !owner.observed.lock_completed);
        CHECK((uintptr_t)owner.audio->ioa_Request.io_Unit>UINT32_MAX &&
            ((uintptr_t)owner.audio->ioa_Request.io_Unit&UINT32_MAX)==15);
        CHECK(model.sent && model.freed && model.completed && !model.consumed);return;
    }
    if(model.scenario==LOCK_WIDE_MASK) {
        CHECK(owner.observed.allocated_mask==15 && owner.observed.precedence_completed &&
            owner.observed.lock_issued && owner.observed.free_completed && owner.free_issued &&
            owner.observed.free_return==0 && owner.observed.free_error==0 &&
            owner.observed.freed_mask==15 && owner.observed.lock_completed &&
            owner.observed.lock_return==0 && owner.observed.lock_error==0 &&
            owner.observed.lock_returned_mask==0);
        CHECK((uintptr_t)owner.lock->ioa_Request.io_Unit>UINT32_MAX &&
            ((uintptr_t)owner.lock->ioa_Request.io_Unit&UINT32_MAX)==0);
        CHECK(model.sent && model.freed && model.completed && model.consumed);return;
    }
    if(model.scenario==OPEN_PARTIAL)CHECK(owner.observed.allocated_mask==0 &&
        (uintptr_t)owner.audio->ioa_Request.io_Unit==7);
    if(model.scenario==OPEN_BAD_DEVICE)CHECK(owner.device==NULL);
    if(model.scenario==OPEN_ZERO_KEY)CHECK(owner.allocation_key==0);
    if(model.scenario==PREC_ERROR)CHECK(owner.observed.precedence_completed==1 &&
        owner.observed.precedence_return==ADIOERR_NOALLOCATION &&
        owner.observed.precedence_error==ADIOERR_NOALLOCATION);
    if(model.scenario==PREC_PARTIAL)CHECK(owner.observed.precedence_completed==1 &&
        owner.observed.precedence_return==0 && owner.observed.precedence_error==0 &&
        (uintptr_t)owner.audio->ioa_Request.io_Unit==7);
    if(model.scenario<=LOCK_EARLY)CHECK(!owner.observed.free_completed && !owner.free_issued);
    else {
        CHECK(owner.observed.free_completed==1 && owner.free_issued==1);
        if(model.scenario==FREE_NOALLOCATION)CHECK(owner.observed.free_return==ADIOERR_NOALLOCATION &&
            owner.observed.free_error==ADIOERR_NOALLOCATION && owner.observed.freed_mask==0);
        else if(model.scenario==FREE_PARTIAL)CHECK(owner.observed.free_return==0 &&
            owner.observed.free_error==0 && owner.observed.freed_mask==0 &&
            (uintptr_t)owner.audio->ioa_Request.io_Unit==7);
        else if(model.scenario==FREE_MISMATCH)CHECK(owner.observed.free_return==0 &&
            owner.observed.free_error==ADIOERR_NOALLOCATION && owner.observed.freed_mask==15);
        else CHECK(owner.observed.free_return==0 && owner.observed.free_error==0 &&
            owner.observed.freed_mask==15);
    }
    CHECK(owner.observed.lock_completed==(unsigned)(model.scenario>=LOCK_UNKNOWN));
    if(model.scenario==LOCK_UNKNOWN)CHECK(owner.observed.lock_return==7 && owner.observed.lock_error==7);
    if(model.scenario==LOCK_MISMATCH)CHECK(owner.observed.lock_return==ADIOERR_CHANNELSTOLEN &&
        owner.observed.lock_error==0);
    if(model.scenario==LOCK_ZERO_MASK_ERROR)CHECK(owner.observed.lock_return==0 &&
        owner.observed.lock_error==0 && owner.observed.lock_returned_mask==0 &&
        (uintptr_t)owner.lock->ioa_Request.io_Unit==8);
}
static void trace(const enum call *expected,size_t count)
{
    CHECK(model.call_count==count && !memcmp(model.calls,expected,count*sizeof(*expected)));
}
static void prefix_trace(const enum call *prefix,size_t count)
{
    CHECK(model.call_count>=count && !memcmp(model.calls,prefix,count*sizeof(*prefix)));
}
static void no_cleanup_after_held(void)
{
    unsigned i;
    CHECK(model.held && live()==3 && model.calls[model.call_count-1]==RETAIN_WAIT);
    for(i=0;i<model.call_count;++i)CHECK(model.calls[i]!=CLOSE_DEVICE &&
        model.calls[i]!=DELETE_LOCK && model.calls[i]!=DELETE_AUDIO && model.calls[i]!=DELETE_PORT);
}
static void context_refusal(void)
{
    unsigned count;
    model.task.tc_Node.ln_Type=1;
    CHECK(pt_private_audio_enroll()==PT_PRIVATE_AUDIO_INVALID);
    model.task.tc_Node.ln_Type=NT_PROCESS;model.exec.IDNestCnt=0;
    count=model.call_count;CHECK(pt_private_audio_enroll()==PT_PRIVATE_AUDIO_INVALID && model.call_count==count);
    model.exec.IDNestCnt=-1;model.exec.TDNestCnt=0;
    CHECK(pt_private_audio_enroll()==PT_PRIVATE_AUDIO_INVALID && model.call_count==count);
    model.exec.TDNestCnt=-1;SysBase=NULL;
    CHECK(pt_private_audio_enroll()==PT_PRIVATE_AUDIO_INVALID && model.call_count==count);
    SysBase=&model.exec;CHECK(live()==0 && !model.requests && !model.opened && !model.sent);
    CHECK(model.call_count==1 && model.calls[0]==FIND_TASK);
}
static void output_and_once(void)
{
    union {long double alignment;unsigned char bytes[1024];} buffer;
    struct pt_private_audio_observation *out=(void *)buffer.bytes;
    unsigned char unchanged[sizeof(buffer.bytes)];unsigned count;
    size_t alignment=offsetof(struct {char prefix;struct pt_private_audio_observation value;},value);
    uintptr_t p,owner_start=(uintptr_t)&owner;size_t capacity;
    memset(buffer.bytes,0xa5,sizeof(buffer.bytes));memcpy(unchanged,buffer.bytes,sizeof(unchanged));
    CHECK(pt_private_audio_observe(out,sizeof(*out)-1)==0 &&
        !memcmp(buffer.bytes,unchanged,sizeof(unchanged)));
    CHECK(pt_private_audio_observe((void *)(buffer.bytes+1),sizeof(*out))==0 &&
        !memcmp(buffer.bytes,unchanged,sizeof(unchanged)));
    CHECK(pt_private_audio_observe((void *)&owner,sizeof(owner))==0);
    CHECK(pt_private_audio_observe((void *)owner.port,sizeof(*out))==0);
    CHECK(pt_private_audio_observe((void *)owner.audio,sizeof(*out))==0);
    CHECK(pt_private_audio_observe((void *)owner.lock,sizeof(*out))==0);
    CHECK(pt_private_audio_observe(out,SIZE_MAX)==0 &&
        !memcmp(buffer.bytes,unchanged,sizeof(unchanged)));
    CHECK(owner_start>sizeof(*out)+alignment);
    p=(owner_start-sizeof(*out)-alignment)/alignment*alignment;
    capacity=(size_t)(owner_start-p)+sizeof(owner);
    CHECK(p+sizeof(*out)<=owner_start && p+capacity>owner_start);
    CHECK(pt_private_audio_observe((void *)p,capacity)==0);
    CHECK(pt_private_audio_observe(out,sizeof(buffer.bytes))==1 &&
        out->result==PT_PRIVATE_AUDIO_RESERVED && out->allocated_mask==15 &&
        out->open_completed==1 && out->precedence_completed==1 &&
        out->free_completed==0 && out->lock_completed==0);
    CHECK(!memcmp(buffer.bytes+sizeof(*out),unchanged+sizeof(*out),sizeof(buffer.bytes)-sizeof(*out)));
    count=model.call_count;CHECK(pt_private_audio_enroll()==PT_PRIVATE_AUDIO_INVALID && model.call_count==count);
    model.exec.ThisTask=&model.other_task;
    CHECK(pt_private_audio_retire()==PT_PRIVATE_AUDIO_INVALID && model.call_count==count);
    CHECK(pt_private_audio_observe(out,sizeof(*out))==0 && model.call_count==count);
    model.exec.ThisTask=&model.task;
    CHECK(pt_private_audio_retire()==PT_PRIVATE_AUDIO_RETIRED);
    count=model.call_count;CHECK(pt_private_audio_retire()==PT_PRIVATE_AUDIO_RETIRED && model.call_count==count);
    CHECK(pt_private_audio_observe(out,sizeof(*out))==1 && out->result==PT_PRIVATE_AUDIO_RETIRED &&
        out->free_completed==1 && out->lock_completed==1 && out->freed_mask==15);
}

int main(int argc,char **argv)
{
    enum pt_private_audio_result result;
    static const enum call full[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,
        OPEN_DEVICE,SET_PREC,SEND_LOCK,CHECK_LOCK,FREE_CHANNELS,CHECK_LOCK,
        WAIT_LOCK,CLOSE_DEVICE,DELETE_LOCK,DELETE_AUDIO,DELETE_PORT};
    static const enum call port_refused[]={FIND_TASK,CREATE_PORT};
    static const enum call audio_refused[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,DELETE_PORT};
    static const enum call lock_refused[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,DELETE_AUDIO,DELETE_PORT};
    static const enum call open_refused[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,OPEN_DEVICE,DELETE_LOCK,DELETE_AUDIO,DELETE_PORT};
    static const enum call open_held[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,OPEN_DEVICE,RETAIN_WAIT};
    static const enum call prec_held[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,OPEN_DEVICE,SET_PREC,RETAIN_WAIT};
    static const enum call early_held[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,OPEN_DEVICE,SET_PREC,SEND_LOCK,CHECK_LOCK,RETAIN_WAIT};
    static const enum call free_held[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,OPEN_DEVICE,SET_PREC,SEND_LOCK,CHECK_LOCK,FREE_CHANNELS,RETAIN_WAIT};
    static const enum call check_held[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,OPEN_DEVICE,SET_PREC,SEND_LOCK,CHECK_LOCK,FREE_CHANNELS,CHECK_LOCK,RETAIN_WAIT};
    static const enum call wait_held[]={FIND_TASK,CREATE_PORT,CREATE_AUDIO,CREATE_LOCK,OPEN_DEVICE,SET_PREC,SEND_LOCK,CHECK_LOCK,FREE_CHANNELS,CHECK_LOCK,WAIT_LOCK,RETAIN_WAIT};
    CHECK(argc==2 && strlen(argv[1])==2 && argv[1][0]>='0' && argv[1][0]<='2' &&
        argv[1][1]>='0' && argv[1][1]<='9');
    model.scenario=(unsigned)(argv[1][0]-'0')*10u+(unsigned)(argv[1][1]-'0');
    CHECK(model.scenario>=1 && model.scenario<=27);
    model.task.tc_Node.ln_Type=NT_PROCESS;
    model.task.tc_SPLower=(void *)(uintptr_t)0x1000;
    model.task.tc_SPUpper=(void *)(uintptr_t)0x9000;
    model.sp=0x5000;model.exec.LibNode.lib_Version=47;
    model.exec.ThisTask=&model.task;model.exec.IDNestCnt=-1;model.exec.TDNestCnt=-1;
    SysBase=&model.exec;
    model.expect_hold=(model.scenario>=OPEN_MISMATCH && model.scenario<=LOCK_EARLY) ||
        (model.scenario>=FREE_NOALLOCATION && model.scenario<=LOCK_ZERO_MASK_ERROR) ||
        (model.scenario>=OPEN_WIDE_MASK && model.scenario<=LOCK_WIDE_MASK);
    if(!setjmp(model.retained)) {
        if(model.scenario==CONTEXT_REFUSAL)context_refusal();
        else {
            result=pt_private_audio_enroll();
            if(model.scenario<=ALLOC_REFUSAL) {
                CHECK(result==PT_PRIVATE_AUDIO_REFUSED_CLOSED && live()==0 && !model.opened && !model.sent);
            } else {
                CHECK(result==PT_PRIVATE_AUDIO_RESERVED && live()==3 && model.sent && !model.completed);
                CHECK(owner.observed.open_completed && owner.observed.precedence_completed &&
                    owner.observed.lock_issued && !owner.observed.lock_completed && !owner.free_issued);
                if(model.scenario==STOLEN_REPLY) {
                    /* Device-boundary event after genuine enrollment: the
                     * issued LOCK has replied before retirement begins. */
                    model.completed=1;
                    lock()->ioa_Request.io_Error=ADIOERR_CHANNELSTOLEN;
                    mask(&lock()->ioa_Request,15);
                }
                if(model.scenario==OUTPUT_AND_ONCE)output_and_once();
                else CHECK(pt_private_audio_retire()==PT_PRIVATE_AUDIO_RETIRED);
                CHECK(live()==0 && !model.opened && model.consumed);
                CHECK(owner.observed.result==PT_PRIVATE_AUDIO_RETIRED && owner.observed.free_completed &&
                    owner.observed.lock_completed && owner.observed.freed_mask==15);
                CHECK(owner.observed.lock_return==(model.scenario==STOLEN_REPLY?ADIOERR_CHANNELSTOLEN:0));
            }
        }
        CHECK(!model.expect_hold && !model.held);
    } else {CHECK(model.expect_hold);no_cleanup_after_held();}
    if(model.scenario==PORT_REFUSAL)trace(port_refused,sizeof(port_refused)/sizeof(*port_refused));
    else if(model.scenario==AUDIO_REFUSAL)trace(audio_refused,sizeof(audio_refused)/sizeof(*audio_refused));
    else if(model.scenario==LOCK_REFUSAL)trace(lock_refused,sizeof(lock_refused)/sizeof(*lock_refused));
    else if(model.scenario==OPEN_REFUSAL || model.scenario==ALLOC_REFUSAL)trace(open_refused,sizeof(open_refused)/sizeof(*open_refused));
    else if(model.scenario>=OPEN_MISMATCH && model.scenario<=OPEN_ZERO_KEY)trace(open_held,sizeof(open_held)/sizeof(*open_held));
    else if(model.scenario==PREC_ERROR || model.scenario==PREC_PARTIAL)trace(prec_held,sizeof(prec_held)/sizeof(*prec_held));
    else if(model.scenario==LOCK_EARLY)trace(early_held,sizeof(early_held)/sizeof(*early_held));
    else if(model.scenario>=FREE_NOALLOCATION && model.scenario<=FREE_MISMATCH)trace(free_held,sizeof(free_held)/sizeof(*free_held));
    else if(model.scenario==LOCK_PENDING || model.scenario==CHECK_OTHER)trace(check_held,sizeof(check_held)/sizeof(*check_held));
    else if(model.scenario>=LOCK_UNKNOWN && model.scenario<=LOCK_ZERO_MASK_ERROR)trace(wait_held,sizeof(wait_held)/sizeof(*wait_held));
    else if(model.scenario==OPEN_WIDE_MASK)trace(open_held,sizeof(open_held)/sizeof(*open_held));
    else if(model.scenario==FREE_WIDE_MASK)trace(free_held,sizeof(free_held)/sizeof(*free_held));
    else if(model.scenario==LOCK_WIDE_MASK)trace(wait_held,sizeof(wait_held)/sizeof(*wait_held));
    else if(model.scenario!=CONTEXT_REFUSAL)trace(full,sizeof(full)/sizeof(*full));
    if((model.scenario>=FREE_NOALLOCATION && model.scenario<=OUTPUT_AND_ONCE) ||
        model.scenario==FREE_WIDE_MASK || model.scenario==LOCK_WIDE_MASK)prefix_trace(full,8);
    if(model.scenario<=OUTPUT_AND_ONCE)
        CHECK(printf("AUDIO RESERVATION HOST24 case=%02u PASS; ACTUAL_SELECTED_C; HOST_MODEL_ONLY; native=NOT_RUN\n",model.scenario)>0);
    else CHECK(printf("AUDIO RESERVATION HOST27 case=%02u PASS; ACTUAL_SELECTED_C; HOST_MODEL_ONLY; native=NOT_RUN\n",model.scenario)>0);
    CHECK(fflush(stdout)==0);return 0;
}
