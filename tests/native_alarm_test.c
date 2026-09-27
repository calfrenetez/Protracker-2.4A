#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../src/native/eclock_alarm.h"
static struct ExecBase base={{36}};struct ExecBase *SysBase=&base;
static struct Device device={{36}};static struct MsgPort port={5};static struct timerequest request;
static unsigned fail_at,ports,requests,opens,sends,aborts,waits,active,ready,complete_on_abort;static int error;static ULONG rate=700001;
struct MsgPort *CreateMsgPort(void){if(fail_at==1)return 0;++ports;return &port;}
void DeleteMsgPort(struct MsgPort *p){assert(p==&port && !requests && ports==1);--ports;}
struct IORequest *CreateIORequest(struct MsgPort *p,unsigned long n)
{assert(p==&port && n==sizeof(request));if(fail_at==2)return 0;++requests;return &request.tr_node;}
void DeleteIORequest(struct IORequest *p){assert(p==&request.tr_node && requests==1 && !opens && !active);--requests;}
int OpenDevice(const char *name,unsigned unit,struct IORequest *p,unsigned flags)
{assert(!strcmp(name,TIMERNAME) && unit==UNIT_WAITECLOCK && p==&request.tr_node && !flags);if(fail_at==3)return 1;p->io_Device=&device;++opens;return 0;}
void CloseDevice(struct IORequest *p){assert(p==&request.tr_node && opens==1 && !active);--opens;}
ULONG fake_read(struct Device *p,struct EClockVal *v){assert(p==&device && opens==1);v->ev_hi=1;v->ev_lo=100;return rate;}
void SendIO(struct IORequest *p){assert(p==&request.tr_node && !active && opens && p->io_Command==TR_ADDREQUEST && !p->io_Flags && !p->io_Error);active=1;ready=0;++sends;}
struct IORequest *CheckIO(struct IORequest *p){assert(p==&request.tr_node && active);return ready?p:NULL;}
LONG WaitIO(struct IORequest *p){assert(p==&request.tr_node && active && ready);active=0;++waits;return error;}
void AbortIO(struct IORequest *p){assert(p==&request.tr_node && active && !ready);++aborts;if(complete_on_abort){ready=1;error=0;}/* Otherwise completion stays delayed. */}
int main(void)
{
    struct pt_native_alarm a={0};unsigned i,n;uint64_t deadline=((uint64_t)2<<32)+123;
    base.LibNode.lib_Version=35;assert(!pt_native_alarm_open(&a) && !ports);base.LibNode.lib_Version=36;
    for(i=1;i<=4;++i){fail_at=i;device.dd_Library.lib_Version=i==4?35:36;
        assert(!pt_native_alarm_open(&a) && !ports && !requests && !opens);assert(pt_native_alarm_close(&a));}
    fail_at=0;device.dd_Library.lib_Version=36;assert(pt_native_alarm_open(&a));
    assert(!pt_native_alarm_open(&a) && !pt_native_alarm_signal(&a));
    assert(pt_native_alarm_arm(&a,((uint64_t)1<<32)+100)==PT_ALARM_LATE && !sends);
    assert(pt_native_alarm_arm(&a,deadline)==PT_ALARM_WAITING);
    assert(request.tr_time.tv_secs==2 && request.tr_time.tv_micro==123 && pt_native_alarm_signal(&a)==32);
    assert(pt_native_alarm_arm(&a,deadline)==PT_ALARM_INVALID && sends==1);
    for(i=0;i<3;++i)assert(pt_native_alarm_poll(&a)==PT_ALARM_WAITING && !waits);
    ready=1;error=0;assert(pt_native_alarm_poll(&a)==PT_ALARM_READY && waits==1 && !active);
    assert(pt_native_alarm_poll(&a)==PT_ALARM_IDLE && !pt_native_alarm_signal(&a));
    assert(pt_native_alarm_arm(&a,deadline)==PT_ALARM_WAITING);n=waits;
    assert(!pt_native_alarm_close(&a) && a.pending && a.closing && ports && requests && opens && aborts==1 && waits==n);
    assert(!pt_native_alarm_close(&a) && aborts==1 && waits==n);
    assert(pt_native_alarm_arm(&a,deadline)==PT_ALARM_INVALID);
    ready=1;error=IOERR_ABORTED;assert(pt_native_alarm_close(&a) && waits==n+1 && !ports && !requests && !opens);
    assert(pt_native_alarm_close(&a));
    /* Cancel a completed request: collect it once without abort. Then rearm. */
    assert(pt_native_alarm_open(&a));assert(pt_native_alarm_arm(&a,deadline)==PT_ALARM_WAITING);
    ready=1;error=0;n=aborts;assert(pt_native_alarm_cancel(&a)==PT_ALARM_CANCELLED && aborts==n);
    /* Completion can race the abort request and return success, not ABORTED. */
    assert(pt_native_alarm_arm(&a,deadline)==PT_ALARM_WAITING);complete_on_abort=1;n=waits;
    assert(pt_native_alarm_cancel(&a)==PT_ALARM_CANCELLED && waits==n+1 && !active);complete_on_abort=0;
    assert(pt_native_alarm_arm(&a,deadline)==PT_ALARM_WAITING);ready=1;error=7;
    assert(pt_native_alarm_poll(&a)==PT_ALARM_ERROR && !active);
    assert(pt_native_alarm_arm(&a,deadline)==PT_ALARM_INVALID);assert(pt_native_alarm_close(&a));
    assert(pt_native_alarm_open(&a));rate=0;n=sends;
    assert(pt_native_alarm_arm(&a,deadline)==PT_ALARM_ERROR && sends==n);assert(pt_native_alarm_close(&a));
    assert(!ports && !requests && !opens && !active);
    puts("NATIVE ALARM OWNER PASS: partial-open cleanup, absolute tick packing, busy/late refusal, bounded pending polls, delayed abort retains all resources, completed cancellation and device error poison; fake Exec only");return 0;
}
