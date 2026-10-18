/* Private genuine audio.device reservation increment, SOURCE ONLY.
 * No default enrollment, classic scheduling/layout change, playback, CIA,
 * cache release or document publication. See bound NDK terminal contracts. */
#include "native_audio_reservation.h"
#if !defined(__m68k__) || !defined(__amigaos__)
#error "The production translation unit requires the genuine Amiga compiler"
#endif
#include <devices/audio.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <limits.h>

typedef char private_native_geometry[(CHAR_BIT==8 && sizeof(BYTE)==1 &&
    sizeof(ULONG)==4 && sizeof(void *)==4 && sizeof(uintptr_t)==4)?1:-1];

struct private_audio_owner {
    struct ExecBase *exec;
    struct Task *task;
    uintptr_t stack_lower,stack_upper;
    struct MsgPort *port;
    struct IOAudio *audio,*lock;
    struct Device *device;
    WORD allocation_key;
    UBYTE channels;
    unsigned opened,free_issued;
    struct pt_private_audio_observation observed;
};
static struct private_audio_owner owner;

static uintptr_t current_stack_pointer(void)
{
    uintptr_t sp;
    __asm__ volatile ("move.l %%sp,%0" : "=d" (sp) : : "memory");
    return sp;
}
static int ordinary(void)
{
    uintptr_t sp;
    if(!owner.exec || SysBase!=owner.exec || owner.exec->ThisTask!=owner.task ||
       !owner.task || owner.task->tc_Node.ln_Type!=NT_PROCESS ||
       (int)(signed char)owner.exec->IDNestCnt!=-1 ||
       (int)(signed char)owner.exec->TDNestCnt!=-1 ||
       (uintptr_t)owner.task->tc_SPLower!=owner.stack_lower ||
       (uintptr_t)owner.task->tc_SPUpper!=owner.stack_upper)return 0;
    sp=current_stack_pointer();
    return owner.stack_lower && owner.stack_lower<owner.stack_upper &&
        sp>=owner.stack_lower && sp<owner.stack_upper && !(sp&1u);
}

static void held(void) __attribute__((noreturn));
static void held(void)
{
    owner.observed.result=PT_PRIVATE_AUDIO_HELD;
    owner.observed.stage=PT_PRIVATE_AUDIO_QUARANTINED;
    /* No abort, retry, forced cleanup, return, unwinding or caller waiver.
     * Wait(0) only in the genuinely unchanged ordinary owner context; any
     * context uncertainty retains the frame without making another OS call. */
    for(;;) {
        if(ordinary())Wait(0);
        __asm__ volatile ("" ::: "memory");
    }
    __builtin_unreachable();
}
#define REQUIRE_OWNED(x) do {if(!(x))held();} while(0)

static int request_storage(void)
{
    return owner.port && owner.audio && owner.lock &&
        owner.audio!=owner.lock && owner.port->mp_SigTask==owner.task &&
        owner.port->mp_SigBit<32 &&
        owner.audio->ioa_Request.io_Message.mn_ReplyPort==owner.port &&
        owner.lock->ioa_Request.io_Message.mn_ReplyPort==owner.port &&
        owner.audio->ioa_Request.io_Message.mn_Length==sizeof(*owner.audio) &&
        owner.lock->ioa_Request.io_Message.mn_Length==sizeof(*owner.lock);
}
static int keyed_requests(void)
{
    return request_storage() && owner.opened && owner.device &&
        (uintptr_t)owner.device!=UINTPTR_MAX && owner.allocation_key &&
        owner.audio->ioa_Request.io_Device==owner.device &&
        owner.lock->ioa_Request.io_Device==owner.device &&
        owner.audio->ioa_AllocKey==owner.allocation_key &&
        owner.lock->ioa_AllocKey==owner.allocation_key;
}

static enum pt_private_audio_result refuse_unissued(void)
{
    /* Only actual Create* successes, before any successful open or SendIO.
     * Failed audio OpenDevice closes its own unsuccessful open (audio.doc). */
    REQUIRE_OWNED(ordinary() && !owner.opened && !owner.observed.lock_issued);
    if(owner.lock) {
        REQUIRE_OWNED(owner.lock->ioa_Request.io_Message.mn_Length==sizeof(*owner.lock));
        DeleteIORequest((struct IORequest *)owner.lock);owner.lock=NULL;
        REQUIRE_OWNED(ordinary());
    }
    if(owner.audio) {
        REQUIRE_OWNED(owner.audio->ioa_Request.io_Message.mn_Length==sizeof(*owner.audio));
        DeleteIORequest((struct IORequest *)owner.audio);owner.audio=NULL;
        REQUIRE_OWNED(ordinary());
    }
    if(owner.port) {DeleteMsgPort(owner.port);owner.port=NULL;REQUIRE_OWNED(ordinary());}
    owner.observed.stage=PT_PRIVATE_AUDIO_CLOSED;
    owner.observed.result=PT_PRIVATE_AUDIO_REFUSED_CLOSED;
    return owner.observed.result;
}

enum pt_private_audio_result pt_private_audio_enroll(void)
{
    struct ExecBase *exec=SysBase;
    struct Task *task;
    BYTE result;
    /* One-shot module-owned state; no caller can supply an ownership record. */
    if(owner.observed.result!=PT_PRIVATE_AUDIO_NEW)return PT_PRIVATE_AUDIO_INVALID;
    if(!exec || (uintptr_t)exec==UINTPTR_MAX || exec->LibNode.lib_Version<36 ||
       (int)(signed char)exec->IDNestCnt!=-1 ||
       (int)(signed char)exec->TDNestCnt!=-1)return PT_PRIVATE_AUDIO_INVALID;
    task=FindTask(NULL);
    if(!task || exec!=SysBase || exec->ThisTask!=task ||
       task->tc_Node.ln_Type!=NT_PROCESS)return PT_PRIVATE_AUDIO_INVALID;
    owner.exec=exec;owner.task=task;
    owner.stack_lower=(uintptr_t)task->tc_SPLower;
    owner.stack_upper=(uintptr_t)task->tc_SPUpper;
    if(!ordinary())return PT_PRIVATE_AUDIO_INVALID;
    owner.observed.result=PT_PRIVATE_AUDIO_ENROLLING;
    owner.observed.stage=PT_PRIVATE_AUDIO_ALLOCATING;
    owner.channels=15;
    owner.port=CreateMsgPort();REQUIRE_OWNED(ordinary());
    if(!owner.port)return refuse_unissued();
    owner.audio=(struct IOAudio *)CreateIORequest(owner.port,sizeof(*owner.audio));
    REQUIRE_OWNED(ordinary());
    if(!owner.audio)return refuse_unissued();
    owner.lock=(struct IOAudio *)CreateIORequest(owner.port,sizeof(*owner.lock));
    REQUIRE_OWNED(ordinary());
    if(!owner.lock)return refuse_unissued();
    REQUIRE_OWNED(request_storage());
    owner.audio->ioa_Request.io_Message.mn_Node.ln_Pri=-128;
    owner.audio->ioa_AllocKey=0;
    owner.audio->ioa_Data=&owner.channels;owner.audio->ioa_Length=1;
    owner.observed.stage=PT_PRIVATE_AUDIO_OPENING;
    result=OpenDevice((CONST_STRPTR)"audio.device",0,(struct IORequest *)owner.audio,0);
    owner.observed.open_return=(int8_t)result;
    owner.observed.open_error=(int8_t)owner.audio->ioa_Request.io_Error;
    owner.observed.open_completed=1;
    REQUIRE_OWNED(ordinary() && result==owner.audio->ioa_Request.io_Error);
    if(result) {
        REQUIRE_OWNED((uintptr_t)owner.audio->ioa_Request.io_Device==UINTPTR_MAX);
        return refuse_unissued();
    }
    owner.opened=1;
    owner.device=owner.audio->ioa_Request.io_Device;
    owner.allocation_key=owner.audio->ioa_AllocKey;
    REQUIRE_OWNED((uintptr_t)owner.audio->ioa_Request.io_Unit==15);
    owner.observed.allocated_mask=(unsigned)(uintptr_t)owner.audio->ioa_Request.io_Unit;
    REQUIRE_OWNED(owner.device && (uintptr_t)owner.device!=UINTPTR_MAX &&
        owner.allocation_key && owner.observed.allocated_mask==15);
    owner.observed.stage=PT_PRIVATE_AUDIO_PRECEDENCE;
    owner.audio->ioa_Request.io_Message.mn_Node.ln_Pri=127;
    owner.audio->ioa_Request.io_Command=ADCMD_SETPREC;
    owner.audio->ioa_Request.io_Flags=0;
    result=DoIO((struct IORequest *)owner.audio);
    owner.observed.precedence_return=(int8_t)result;
    owner.observed.precedence_error=(int8_t)owner.audio->ioa_Request.io_Error;
    owner.observed.precedence_completed=1;
    REQUIRE_OWNED(ordinary() && request_storage() && result==0 &&
        owner.audio->ioa_Request.io_Error==0 &&
        owner.audio->ioa_Request.io_Device==owner.device &&
        owner.audio->ioa_AllocKey==owner.allocation_key &&
        (uintptr_t)owner.audio->ioa_Request.io_Unit==15);
    owner.lock->ioa_Request.io_Device=owner.device;
    owner.lock->ioa_Request.io_Unit=owner.audio->ioa_Request.io_Unit;
    owner.lock->ioa_AllocKey=owner.allocation_key;
    owner.lock->ioa_Request.io_Command=ADCMD_LOCK;
    owner.lock->ioa_Request.io_Flags=0;
    owner.observed.stage=PT_PRIVATE_AUDIO_LOCKING;
    owner.observed.lock_issued=1;
    SendIO((struct IORequest *)owner.lock);
    REQUIRE_OWNED(ordinary() && keyed_requests());
    /* No WaitIO on pending work; never CheckIO an unissued request. An early
     * completion is acquisition uncertainty, not an invented unlock proof. */
    REQUIRE_OWNED(CheckIO((struct IORequest *)owner.lock)==NULL);
    REQUIRE_OWNED(ordinary() && keyed_requests() &&
        !(owner.lock->ioa_Request.io_Flags&IOF_QUICK) &&
        (uintptr_t)owner.lock->ioa_Request.io_Unit==15);
    owner.observed.result=PT_PRIVATE_AUDIO_RESERVED;
    return owner.observed.result;
}

enum pt_private_audio_result pt_private_audio_retire(void)
{
    BYTE result;
    struct IORequest *completed;
    if(owner.observed.result==PT_PRIVATE_AUDIO_RETIRED)return ordinary()?
        PT_PRIVATE_AUDIO_RETIRED:PT_PRIVATE_AUDIO_INVALID;
    if(owner.observed.result!=PT_PRIVATE_AUDIO_RESERVED)return PT_PRIVATE_AUDIO_INVALID;
    if(!ordinary())return PT_PRIVATE_AUDIO_INVALID;
    REQUIRE_OWNED(keyed_requests() && owner.observed.lock_issued &&
        !owner.observed.lock_completed && !owner.free_issued &&
        (uintptr_t)owner.audio->ioa_Request.io_Unit==15);
    owner.observed.result=PT_PRIVATE_AUDIO_RETIRING;
    owner.observed.stage=PT_PRIVATE_AUDIO_FREEING;
    owner.audio->ioa_Request.io_Command=ADCMD_FREE;
    owner.audio->ioa_Request.io_Flags=0;owner.free_issued=1;
    result=DoIO((struct IORequest *)owner.audio);
    owner.observed.free_return=(int8_t)result;
    owner.observed.free_error=(int8_t)owner.audio->ioa_Request.io_Error;
    owner.observed.free_completed=1;
    REQUIRE_OWNED((uintptr_t)owner.audio->ioa_Request.io_Unit==15);
    owner.observed.freed_mask=(unsigned)(uintptr_t)owner.audio->ioa_Request.io_Unit;
    /* NOALLOCATION, partial masks and every unknown error are HELD. */
    REQUIRE_OWNED(ordinary() && keyed_requests() && result==0 &&
        owner.audio->ioa_Request.io_Error==0 && owner.observed.freed_mask==15);
    owner.observed.stage=PT_PRIVATE_AUDIO_CONSUMING_LOCK;
    completed=CheckIO((struct IORequest *)owner.lock);
    REQUIRE_OWNED(ordinary() && completed==(struct IORequest *)owner.lock);
    result=WaitIO((struct IORequest *)owner.lock);
    owner.observed.lock_return=(int8_t)result;
    owner.observed.lock_error=(int8_t)owner.lock->ioa_Request.io_Error;
    owner.observed.lock_completed=1;
    REQUIRE_OWNED(!((uintptr_t)owner.lock->ioa_Request.io_Unit&~(uintptr_t)15) &&
        (result!=0 || (uintptr_t)owner.lock->ioa_Request.io_Unit==0));
    owner.observed.lock_returned_mask=(unsigned)(uintptr_t)owner.lock->ioa_Request.io_Unit;
    /* CHANNELSTOLEN is a documented completed LOCK reply, not a blanket
     * FREE success. Full successful FREE above remains mandatory. */
    REQUIRE_OWNED(ordinary() && keyed_requests() &&
        result==owner.lock->ioa_Request.io_Error &&
        (result==0 || result==ADIOERR_CHANNELSTOLEN) &&
        !(owner.observed.lock_returned_mask&~15u) &&
        (result!=0 || owner.observed.lock_returned_mask==0));
    owner.observed.stage=PT_PRIVATE_AUDIO_CLOSING;
    CloseDevice((struct IORequest *)owner.audio);owner.opened=0;
    REQUIRE_OWNED(ordinary() && request_storage());
    DeleteIORequest((struct IORequest *)owner.lock);owner.lock=NULL;
    REQUIRE_OWNED(ordinary());
    DeleteIORequest((struct IORequest *)owner.audio);owner.audio=NULL;
    REQUIRE_OWNED(ordinary());
    DeleteMsgPort(owner.port);owner.port=NULL;
    REQUIRE_OWNED(ordinary());
    owner.observed.stage=PT_PRIVATE_AUDIO_CLOSED;
    owner.observed.result=PT_PRIVATE_AUDIO_RETIRED;
    return owner.observed.result;
}

static int overlaps(const void *p,size_t bytes,const void *q,size_t length)
{
    uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    if(!p || bytes>UINTPTR_MAX-a)return 1;
    if(!q || !length)return 0;
    if(length>UINTPTR_MAX-b)return 1;
    return a<b+length && b<a+bytes;
}
int pt_private_audio_observe(struct pt_private_audio_observation *out,size_t capacity)
{
    if(!ordinary() || !out || capacity<sizeof(*out) ||
       (uintptr_t)out%offsetof(struct {char prefix;struct pt_private_audio_observation value;},value) ||
       overlaps(out,capacity,&owner,sizeof(owner)) ||
       overlaps(out,capacity,owner.port,owner.port?sizeof(*owner.port):0) ||
       overlaps(out,capacity,owner.audio,owner.audio?sizeof(*owner.audio):0) ||
       overlaps(out,capacity,owner.lock,owner.lock?sizeof(*owner.lock):0))return 0;
    *out=owner.observed;return 1;
}
