#ifndef PT_NATIVE_ECLOCK_H
#define PT_NATIVE_ECLOCK_H
#include <exec/execbase.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <stdint.h>
/* OS2+ owner-thread read-only clock. Zero-initialize before first open; never
 * copy a live owner. Its private request is never submitted, so close has no
 * pending I/O to abort/wait. Keep open until every reader client is closed.
 * No global TimerBase, signal waits, CIA programming or playback side effects. */
struct pt_native_eclock {struct MsgPort *port;struct timerequest *request;unsigned opened;};
static inline void pt_native_eclock_close(struct pt_native_eclock *c)
{
    if(!c)return;
    if(c->opened)CloseDevice((struct IORequest *)c->request);
    if(c->request)DeleteIORequest((struct IORequest *)c->request);
    if(c->port)DeleteMsgPort(c->port);
    c->port=0;c->request=0;c->opened=0;
}
static inline int pt_native_eclock_open(struct pt_native_eclock *c)
{
    if(!c || c->port || c->request || c->opened || SysBase->LibNode.lib_Version<36)return 0;
    c->port=CreateMsgPort();if(!c->port)return 0;
    c->request=(struct timerequest *)CreateIORequest(c->port,sizeof(*c->request));
    if(!c->request || OpenDevice(TIMERNAME,UNIT_ECLOCK,(struct IORequest *)c->request,0))goto failed;
    c->opened=1;
    if(c->request->tr_node.io_Device->dd_Library.lib_Version<36)goto failed;
    return 1;
failed:pt_native_eclock_close(c);return 0;
}
static inline int pt_native_eclock_read(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_native_eclock *c=context;struct EClockVal value;ULONG rate;
    struct Device *TimerBase;
    uintptr_t a=(uintptr_t)c,b=(uintptr_t)ticks,d=(uintptr_t)frequency;
    if(!c || !ticks || !frequency || !c->opened || !c->request ||
       a>UINTPTR_MAX-sizeof(*c) || b>UINTPTR_MAX-sizeof(*ticks) || d>UINTPTR_MAX-sizeof(*frequency) ||
       (b<a+sizeof(*c) && a<b+sizeof(*ticks)) || (d<a+sizeof(*c) && a<d+sizeof(*frequency)) ||
       (b<d+sizeof(*frequency) && d<b+sizeof(*ticks)))return 0;
    TimerBase=c->request->tr_node.io_Device;rate=ReadEClock(&value);
    if(!rate)return 0;
    *ticks=((uint64_t)value.ev_hi<<32)|value.ev_lo;*frequency=rate;return 1;
}
#endif
