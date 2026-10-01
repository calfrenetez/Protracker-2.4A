#ifndef PT_NATIVE_PAULA_RESERVATION_H
#define PT_NATIVE_PAULA_RESERVATION_H
#include <stdint.h>
#include <devices/audio.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>
/* Private, zero-initialized, noncopyable, owner-thread OS2+ reservation.
 * Open performs only resource setup; advance submits/polls at most one command.
 * No WRITE, DMA programming, sample ownership or playback here. Reserve at -128
 * without stealing; raise precedence only after exact four-channel acquisition.
 * LOCK observes theft, it does not prevent it. A completed LOCK fails readiness.
 * Close only after clients confirm every voice/reader stopped. Pending/error
 * close retains all resources; never dispose or reset an unclosed reservation.
 * Signal is a notification, not completion proof. No unfinished WaitIO calls. */
struct pt_native_paula_reservation {
    struct MsgPort *port;
    struct IOAudio *command,*lock;
    unsigned opened,pending,locked,closing,failed,phase,mask;
    WORD key;
    UBYTE channels;
};
enum pt_paula_reservation_result {PT_PAULA_RESERVATION_ERROR=-1,
    PT_PAULA_RESERVATION_PENDING=0,PT_PAULA_RESERVATION_READY=1};
static inline int pt_native_paula_command_done(struct pt_native_paula_reservation *r)
{
    if(!r->pending)return 1;
    if(!(r->command->ioa_Request.io_Flags&IOF_QUICK)) {
        if(!CheckIO((struct IORequest *)r->command))return 0;
        WaitIO((struct IORequest *)r->command);
    }
    r->pending=0;return 1;
}
static inline void pt_native_paula_submit(struct pt_native_paula_reservation *r,UWORD command)
{
    r->command->ioa_Request.io_Command=command;
    r->command->ioa_Request.io_Flags=IOF_QUICK;
    r->command->ioa_Request.io_Error=0;
    r->pending=1;BeginIO((struct IORequest *)r->command);
}
static inline int pt_native_paula_reservation_close(struct pt_native_paula_reservation *r)
{
    if(!r)return 0;
    r->closing=1;
    if(!pt_native_paula_command_done(r))return 0;
    /* A close during allocation must capture the returned ownership first. */
    if(r->opened && r->phase==1) {
        r->mask=(unsigned)(uintptr_t)r->command->ioa_Request.io_Unit;
        r->key=r->command->ioa_AllocKey;r->phase=2;
    }
    if(r->mask) {
        if(r->mask>15 || !r->key){r->failed=1;return 0;}
        if(r->phase!=5) {
            r->command->ioa_Request.io_Unit=(struct Unit *)(uintptr_t)r->mask;
            r->command->ioa_AllocKey=r->key;r->phase=5;
            pt_native_paula_submit(r,ADCMD_FREE);return 0;
        }
        if(r->command->ioa_Request.io_Error &&
            r->command->ioa_Request.io_Error!=ADIOERR_NOALLOCATION){r->failed=1;return 0;}
        r->mask=0;
    }
    if(r->locked) {
        /* FREE completes LOCK. Poll only: never force close a retained request. */
        if(!CheckIO((struct IORequest *)r->lock))return 0;
        WaitIO((struct IORequest *)r->lock);r->locked=0;
    }
    if(r->opened) {
        r->command->ioa_Request.io_Unit=0;r->command->ioa_AllocKey=0;
        CloseDevice((struct IORequest *)r->command);r->opened=0;
    }
    if(r->lock)DeleteIORequest((struct IORequest *)r->lock);
    if(r->command)DeleteIORequest((struct IORequest *)r->command);
    if(r->port)DeleteMsgPort(r->port);
    r->lock=r->command=0;r->port=0;r->pending=r->phase=r->closing=r->failed=0;r->key=0;
    return 1;
}
static inline int pt_native_paula_reservation_open(struct pt_native_paula_reservation *r)
{
    if(!r || r->port || r->command || r->lock || r->opened || r->pending || r->mask || r->locked)return 0;
    r->port=CreateMsgPort();if(!r->port)return 0;
    r->command=(struct IOAudio *)CreateIORequest(r->port,sizeof(*r->command));
    r->lock=(struct IOAudio *)CreateIORequest(r->port,sizeof(*r->lock));
    if(!r->command || !r->lock)goto failed;
    r->command->ioa_Length=0;r->command->ioa_AllocKey=0;
    if(OpenDevice(AUDIONAME,0,(struct IORequest *)r->command,0))goto failed;
    r->opened=1;r->channels=15;r->phase=r->closing=r->failed=0;return 1;
failed:pt_native_paula_reservation_close(r);return 0;
}
static inline enum pt_paula_reservation_result pt_native_paula_reservation_advance(struct pt_native_paula_reservation *r)
{
    if(!r || !r->opened || r->closing || r->failed)return PT_PAULA_RESERVATION_ERROR;
    if(!pt_native_paula_command_done(r))return PT_PAULA_RESERVATION_PENDING;
    if(r->phase==0) {
        r->command->ioa_Data=&r->channels;r->command->ioa_Length=1;
        r->command->ioa_Request.io_Message.mn_Node.ln_Pri=ADALLOC_MINPREC;
        r->command->ioa_Request.io_Command=ADCMD_ALLOCATE;
        r->command->ioa_Request.io_Flags=IOF_QUICK|ADIOF_NOWAIT;
        r->command->ioa_Request.io_Error=0;r->pending=1;r->phase=1;
        BeginIO((struct IORequest *)r->command);return PT_PAULA_RESERVATION_PENDING;
    }
    if(r->phase==1) {
        r->mask=(unsigned)(uintptr_t)r->command->ioa_Request.io_Unit;
        r->key=r->command->ioa_AllocKey;r->phase=2;
        if(r->command->ioa_Request.io_Error || r->mask!=15 || !r->key)goto failed;
        r->command->ioa_Request.io_Message.mn_Node.ln_Pri=ADALLOC_MAXPREC;
        pt_native_paula_submit(r,ADCMD_SETPREC);return PT_PAULA_RESERVATION_PENDING;
    }
    if(r->phase==2) {
        if(r->command->ioa_Request.io_Error || (uintptr_t)r->command->ioa_Request.io_Unit!=15 || r->command->ioa_AllocKey!=r->key)goto failed;
        r->lock->ioa_Request.io_Device=r->command->ioa_Request.io_Device;
        r->lock->ioa_Request.io_Unit=(struct Unit *)(uintptr_t)r->mask;
        r->lock->ioa_AllocKey=r->key;r->lock->ioa_Request.io_Command=ADCMD_LOCK;
        r->lock->ioa_Request.io_Flags=0;r->lock->ioa_Request.io_Error=0;
        r->locked=1;r->phase=3;BeginIO((struct IORequest *)r->lock);
    }
    if(r->phase!=3 || CheckIO((struct IORequest *)r->lock))goto failed;
    return PT_PAULA_RESERVATION_READY;
failed:r->failed=1;return PT_PAULA_RESERVATION_ERROR;
}
static inline ULONG pt_native_paula_reservation_signal(const struct pt_native_paula_reservation *r)
{return r && r->port && (r->pending || r->locked) && r->port->mp_SigBit<32?1UL<<r->port->mp_SigBit:0;}
#endif
