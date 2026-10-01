#ifndef PT_NATIVE_ECLOCK_ALARM_H
#define PT_NATIVE_ECLOCK_ALARM_H
#include "eclock.h"
#include <exec/errors.h>
enum pt_alarm_result {PT_ALARM_ERROR=-2,PT_ALARM_INVALID=-1,PT_ALARM_WAITING=0,PT_ALARM_READY=1,PT_ALARM_CANCELLED=2,PT_ALARM_IDLE=3,PT_ALARM_LATE=4};
/* Zero-initialized, noncopyable, owner-thread private WAITECLOCK request.
 * No public method waits for unfinished IO. A pending close returns0 and keeps
 * EVERY resource alive; poll/close again later. Never dispose such an owner.
 * Wakeups are notifications only: clients must resample/check actual deadlines. */
struct pt_native_alarm {struct MsgPort *port;struct timerequest *request;unsigned opened,pending,cancelling,closing,failed;
    /* Last arm observation/target retained on close, reset on successful open.
     * Diagnostic only; never a replacement for a new actual clock read. */
    uint64_t observed_ticks,attempted_deadline;};
static inline enum pt_alarm_result pt_native_alarm_poll(struct pt_native_alarm *a)
{
    LONG error;
    if(!a || !a->opened)return PT_ALARM_INVALID;
    if(!a->pending)return a->failed?PT_ALARM_ERROR:PT_ALARM_IDLE;
    if(!CheckIO((struct IORequest *)a->request))return PT_ALARM_WAITING;
    error=WaitIO((struct IORequest *)a->request);a->pending=0;
    if(error && !(a->cancelling && error==IOERR_ABORTED)){a->failed=1;return PT_ALARM_ERROR;}
    return a->cancelling?PT_ALARM_CANCELLED:PT_ALARM_READY;
}
static inline enum pt_alarm_result pt_native_alarm_cancel(struct pt_native_alarm *a)
{
    if(!a || !a->opened)return PT_ALARM_INVALID;
    if(!a->pending)return a->failed?PT_ALARM_ERROR:PT_ALARM_IDLE;
    if(!a->cancelling) {
        a->cancelling=1;
        if(!CheckIO((struct IORequest *)a->request))AbortIO((struct IORequest *)a->request);
    }
    return pt_native_alarm_poll(a);
}
static inline int pt_native_alarm_close(struct pt_native_alarm *a)
{
    if(!a)return 0;
    a->closing=1;
    if(a->pending){pt_native_alarm_cancel(a);if(a->pending)return 0;}
    if(a->opened)CloseDevice((struct IORequest *)a->request);
    if(a->request)DeleteIORequest((struct IORequest *)a->request);
    if(a->port)DeleteMsgPort(a->port);
    a->port=0;a->request=0;a->opened=a->pending=a->cancelling=a->closing=a->failed=0;return 1;
}
static inline int pt_native_alarm_open(struct pt_native_alarm *a)
{
    if(!a || a->port || a->request || a->opened || a->pending || SysBase->LibNode.lib_Version<36)return 0;
    a->port=CreateMsgPort();if(!a->port)return 0;
    a->request=(struct timerequest *)CreateIORequest(a->port,sizeof(*a->request));
    if(!a->request || OpenDevice(TIMERNAME,UNIT_WAITECLOCK,(struct IORequest *)a->request,0))goto failed;
    a->opened=1;
    if(a->request->tr_node.io_Device->dd_Library.lib_Version<36)goto failed;
    a->cancelling=a->closing=a->failed=0;a->observed_ticks=a->attempted_deadline=0;return 1;
failed:pt_native_alarm_close(a);return 0;
}
static inline enum pt_alarm_result pt_native_alarm_arm(struct pt_native_alarm *a,uint64_t deadline)
{
    struct EClockVal now;struct Device *TimerBase;
    if(!a || !a->opened || a->pending || a->closing || a->failed)return PT_ALARM_INVALID;
    TimerBase=a->request->tr_node.io_Device;
    if(!ReadEClock(&now)){a->failed=1;return PT_ALARM_ERROR;}
    a->observed_ticks=((uint64_t)now.ev_hi<<32)|now.ev_lo;a->attempted_deadline=deadline;
    if(deadline<=a->observed_ticks)return PT_ALARM_LATE;
    a->request->tr_node.io_Command=TR_ADDREQUEST;a->request->tr_node.io_Flags=0;a->request->tr_node.io_Error=0;
    a->request->tr_time.tv_secs=(ULONG)(deadline>>32);a->request->tr_time.tv_micro=(ULONG)deadline;
    a->cancelling=0;a->pending=1;SendIO((struct IORequest *)a->request);return PT_ALARM_WAITING;
}
static inline ULONG pt_native_alarm_signal(const struct pt_native_alarm *a)
{return a && a->pending && a->port && a->port->mp_SigBit<32?1UL<<a->port->mp_SigBit:0;}
#endif
