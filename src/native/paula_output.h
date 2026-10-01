#ifndef PT_NATIVE_PAULA_OUTPUT_H
#define PT_NATIVE_PAULA_OUTPUT_H
#include <exec/execbase.h>
#include <exec/memory.h>
#include "paula_reservation.h"
#include "../editor/paula_voices.h"
#include "../core/paula_render_voice.h"
/* OS2+ private audio.device output, serialized owner thread. Zero-init once;
 * noncopyable. Eight fixed requests and two private ports, no custom IRQ/MMIO writes.
 * Caller supplies contiguous, immutable, valid Chip cache storage, held until
 * stop==1. Start==1 requires an owned channel's stopped-to-enabled DMA
 * transition with DMA master enabled and an error-free asynchronous WRITE.
 * Start messages can arrive later and remain owned through shutdown. Missing
 * activation returns0 (uncertain); no asynchronous success upgrade.
 * Control==1 requires command completion. Stop aborts once, polls completion and
 * consumes start notifications and proves channel DMA off before releasing data.
 * No unfinished WaitIO.
 * Close voice/cache owner before closing this adapter; contexts outlive both.
 * quiesce proves reader closure but does not release channel reservation.
 * Capability clock uses OS2+ Exec's documented E-clock frequency, PAL/NTSC only;
 * 5 E clocks per Paula tick. Unsupported clocks refuse without reservation. */
struct pt_native_paula_output {
    struct pt_native_paula_reservation reservation;
    struct MsgPort *starts;
    struct IOAudio *write[4],*control[4];
    unsigned held[4],pending[4],cancelling[4],notified[4],control_pending[4];
    unsigned activated[4];
    unsigned ready,closing,failed;
    struct pt_paula_render_caps caps;
};
/* Read only. Tests substitute the register read; production never writes DMA. */
static inline UWORD pt_native_paula_output_dma(void)
{
#ifdef PT_PAULA_DMA_READ
    return PT_PAULA_DMA_READ();
#else
    return *(volatile UWORD *)0xdff002;
#endif
}
static inline int pt_native_paula_output_notifications(struct pt_native_paula_output *o)
{
    struct Message *m;unsigned n,i;
    if(!o->starts)return 1;
    for(n=0;n<4;++n) {
        m=GetMsg(o->starts);if(!m)return 1;
        for(i=0;i<4;++i)if(o->write[i] && m==&o->write[i]->ioa_WriteMsg)break;
        if(i==4){o->failed=1;return 0;}o->notified[i]=1;
    }
    return 1; /* At most one start message per fixed outstanding write. */
}
static inline int pt_native_paula_output_completed(struct IOAudio *a,unsigned *pending)
{
    if(!*pending)return 1;
    if(!(a->ioa_Request.io_Flags&IOF_QUICK)) {
        if(!CheckIO((struct IORequest *)a))return 0;
        WaitIO((struct IORequest *)a);
    }
    *pending=0;return 1;
}
static inline int pt_native_paula_output_stop(void *context,unsigned slot)
{
    struct pt_native_paula_output *o=context;int done=1;
    if(!o || slot>=4)return -1;
    if(o->pending[slot]) {
        if(!o->cancelling[slot]) {
            o->cancelling[slot]=1;
            if(!(o->write[slot]->ioa_Request.io_Flags&IOF_QUICK) &&
                !CheckIO((struct IORequest *)o->write[slot]))AbortIO((struct IORequest *)o->write[slot]);
        }
        if(!pt_native_paula_output_completed(o->write[slot],&o->pending[slot]))done=0;
    }
    if(o->control_pending[slot] && !pt_native_paula_output_completed(o->control[slot],&o->control_pending[slot]))done=0;
    if(!pt_native_paula_output_notifications(o))return -1;
    if(!done || (o->held[slot] && (pt_native_paula_output_dma()&(1U<<slot))))return 0;
    o->held[slot]=o->cancelling[slot]=o->notified[slot]=o->activated[slot]=0;
    if(o->write[slot]){o->write[slot]->ioa_Data=0;o->write[slot]->ioa_Length=0;}
    return 1;
}
static inline int pt_native_paula_output_quiesce(void *context)
{
    struct pt_native_paula_output *o=context;unsigned i;
    if(!o || o->failed || !pt_native_paula_output_notifications(o))return 0;
    for(i=0;i<4;++i)if(o->held[i] || o->pending[i] || o->control_pending[i])return 0;
    return !o->reservation.mask || !(pt_native_paula_output_dma()&o->reservation.mask);
}
static inline int pt_native_paula_output_close(struct pt_native_paula_output *o)
{
    unsigned i;int done=1;
    if(!o)return 0;
    o->closing=1;
    for(i=0;i<4;++i)if(pt_native_paula_output_stop(o,i)!=1)done=0;
    if(!done || !pt_native_paula_output_quiesce(o))return 0;
    if(!pt_native_paula_reservation_close(&o->reservation))return 0;
    for(i=0;i<4;++i) {
        if(o->write[i])DeleteIORequest((struct IORequest *)o->write[i]);
        if(o->control[i])DeleteIORequest((struct IORequest *)o->control[i]);
        o->write[i]=o->control[i]=0;
    }
    if(o->starts)DeleteMsgPort(o->starts);
    o->starts=0;
    o->ready=o->closing=0;o->caps=(struct pt_paula_render_caps){0};return 1;
}
static inline int pt_native_paula_output_open(struct pt_native_paula_output *o)
{
    unsigned i;ULONG rate;
    if(!o || o->starts || o->reservation.port || o->ready || o->closing || o->failed || SysBase->LibNode.lib_Version<36)return 0;
    for(i=0;i<4;++i)if(o->write[i] || o->control[i] || o->held[i])return 0;
    rate=SysBase->ex_EClockFrequency;if(rate!=709379UL && rate!=715909UL)return 0;
    o->caps.clock_hz=rate*5;o->caps.minimum_period=124;o->caps.maximum_period=65535;
    if(!pt_native_paula_reservation_open(&o->reservation))goto failed;
    o->starts=CreateMsgPort();if(!o->starts)goto failed;
    for(i=0;i<4;++i) {
        o->write[i]=(struct IOAudio *)CreateIORequest(o->reservation.port,sizeof(struct IOAudio));
        o->control[i]=(struct IOAudio *)CreateIORequest(o->reservation.port,sizeof(struct IOAudio));
        if(!o->write[i] || !o->control[i])goto failed;
    }
    return 1;
failed:pt_native_paula_output_close(o);return 0;
}
static inline int pt_native_paula_output_advance(struct pt_native_paula_output *o)
{
    unsigned i;int result;
    if(!o || o->closing || o->failed)return -1;
    result=pt_native_paula_reservation_advance(&o->reservation);
    if(result!=1)return result;
    if(!o->ready)for(i=0;i<4;++i) {
        o->write[i]->ioa_Request.io_Device=o->control[i]->ioa_Request.io_Device=o->reservation.command->ioa_Request.io_Device;
        o->write[i]->ioa_Request.io_Unit=o->control[i]->ioa_Request.io_Unit=(struct Unit *)(uintptr_t)(1U<<i);
        o->write[i]->ioa_AllocKey=o->control[i]->ioa_AllocKey=o->reservation.key;
    }
    o->ready=1;return 1;
}
static inline int pt_native_paula_output_valid(struct pt_native_paula_output *o,uint16_t period,uint8_t volume)
{return o && o->ready && !o->closing && !o->failed && period>=o->caps.minimum_period && period<=o->caps.maximum_period && volume<=64 &&
    SysBase->LibNode.lib_Version>=36 && SysBase->ex_EClockFrequency==o->caps.clock_hz/5 && pt_native_paula_reservation_advance(&o->reservation)==1;}
static inline int pt_native_paula_output_start(void *context,unsigned slot,const struct pt_paula_voice_plan *plan)
{
    struct pt_native_paula_output *o=context;struct IOAudio *a;uintptr_t p;size_t bytes;UWORD dma;
    if(slot>=4 || !plan || !pt_native_paula_output_valid(o,plan->period,plan->volume) ||
        o->held[slot] || o->pending[slot] || o->control_pending[slot] || !plan->data || !plan->words)return 0;
    p=(uintptr_t)plan->data;bytes=(size_t)plan->words*2;
    if((p&1) || p>UINTPTR_MAX-(bytes-1) || !(TypeOfMem((void *)p)&MEMF_CHIP) || !(TypeOfMem((void *)(p+bytes-1))&MEMF_CHIP))return 0;
    dma=pt_native_paula_output_dma();
    if(!(dma&0x200U) || (dma&(1U<<slot)))return 0;
    a=o->write[slot];a->ioa_Request.io_Unit=(struct Unit *)(uintptr_t)(1U<<slot);a->ioa_AllocKey=o->reservation.key;a->ioa_Data=(UBYTE *)plan->data;a->ioa_Length=(ULONG)bytes;
    a->ioa_Period=plan->period;a->ioa_Volume=plan->volume;a->ioa_Cycles=0;
    a->ioa_WriteMsg.mn_ReplyPort=o->starts;a->ioa_WriteMsg.mn_Length=sizeof(struct Message);
    a->ioa_Request.io_Command=CMD_WRITE;a->ioa_Request.io_Flags=IOF_QUICK|ADIOF_PERVOL|ADIOF_WRITEMESSAGE;a->ioa_Request.io_Error=0;
    o->held[slot]=o->pending[slot]=1;o->cancelling[slot]=o->notified[slot]=0;
    BeginIO((struct IORequest *)a);
    if(!pt_native_paula_output_notifications(o))return 0;
    dma=pt_native_paula_output_dma();
    o->activated[slot]=!a->ioa_Request.io_Error && !(a->ioa_Request.io_Flags&IOF_QUICK) &&
        (uintptr_t)a->ioa_Request.io_Unit==(1U<<slot) && (dma&(0x200U|(1U<<slot)))==(0x200U|(1U<<slot));
    return o->activated[slot]?1:0;
}
static inline int pt_native_paula_output_control(void *context,unsigned slot,uint16_t period,uint8_t volume)
{
    struct pt_native_paula_output *o=context;struct IOAudio *a;
    if(slot>=4 || !pt_native_paula_output_valid(o,period,volume) || !o->held[slot] || o->cancelling[slot] || !o->activated[slot] || o->control_pending[slot])return 0;
    if((pt_native_paula_output_dma()&(0x200U|(1U<<slot)))!=(0x200U|(1U<<slot)))return 0;
    a=o->control[slot];a->ioa_Request.io_Unit=(struct Unit *)(uintptr_t)(1U<<slot);a->ioa_AllocKey=o->reservation.key;a->ioa_Period=period;a->ioa_Volume=volume;
    a->ioa_Request.io_Command=ADCMD_PERVOL;a->ioa_Request.io_Flags=IOF_QUICK;a->ioa_Request.io_Error=0;
    o->control_pending[slot]=1;BeginIO((struct IORequest *)a);
    if(!pt_native_paula_output_completed(a,&o->control_pending[slot]))return 0;
    return !a->ioa_Request.io_Error && (uintptr_t)a->ioa_Request.io_Unit==(1U<<slot)?1:0;
}
static inline int pt_native_paula_output_api(struct pt_native_paula_output *o,struct pt_paula_voice_api *api)
{
    if(!o || !api || !o->ready || o->closing || o->failed)return 0;
    api->context=o;api->start=pt_native_paula_output_start;api->stop=pt_native_paula_output_stop;api->control=pt_native_paula_output_control;return 1;
}
#endif
