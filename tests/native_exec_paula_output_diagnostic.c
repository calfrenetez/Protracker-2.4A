#include "../src/native/paula_output.h"
#include <proto/dos.h>
#include <stdio.h>
/* One diagnostic voice only. Zero32byte Chip buffer, volume0. Observe immediate
 * and one-tick-later state; never retry a start or upgrade the synchronous result.
 * Retain storage until confirmed device/reader close; no MMIO writes or IRQ code. */
static int close_bounded(struct pt_native_paula_output *o)
{unsigned n;for(n=0;n<50;++n){if(pt_native_paula_output_close(o))return 1;Delay(1);}return 0;}
static void state(const char *when,struct pt_native_paula_output *o,int result)
{
    struct IOAudio *a=o->write[0];
    printf("PAULA DIAG %s result=%d notified=%u held=%u pending=%u error=%d flags=%u unit=%lu key=%d request_done=%u dma=%u\n",
        when,result,o->notified[0],o->held[0],o->pending[0],(int)a->ioa_Request.io_Error,
        (unsigned)a->ioa_Request.io_Flags,(unsigned long)(uintptr_t)a->ioa_Request.io_Unit,
        (int)a->ioa_AllocKey,CheckIO((struct IORequest *)a)?1U:0U,(unsigned)(*(volatile UWORD *)0xdff002&15));
}
int main(void)
{
    struct pt_native_paula_output o={0};struct pt_paula_voice_api api;
    struct pt_paula_voice_plan plan;UBYTE *data;unsigned n;int ready=0,result,drained;
    if(!pt_native_paula_output_open(&o))return 10;
    for(n=0;n<50;++n){ready=pt_native_paula_output_advance(&o);if(ready)break;Delay(1);}
    if(ready!=1 || !pt_native_paula_output_api(&o,&api)){close_bounded(&o);return 11;}
    data=AllocMem(32,MEMF_CHIP|MEMF_PUBLIC|MEMF_CLEAR);if(!data){close_bounded(&o);return 12;}
    plan.data=data;plan.words=16;plan.period=400;plan.volume=0;
    printf("PAULA DIAG validation ready=%u failed=%u closing=%u clock=%lu eclock=%lu minimum=%u maximum=%u chip_start=%lu chip_end=%lu aligned=%u mask=%u key=%d\n",
        o.ready,o.failed,o.closing,(unsigned long)o.caps.clock_hz,(unsigned long)SysBase->ex_EClockFrequency,
        (unsigned)o.caps.minimum_period,(unsigned)o.caps.maximum_period,
        (unsigned long)TypeOfMem(data),(unsigned long)TypeOfMem(data+31),((uintptr_t)data&1)?0U:1U,o.reservation.mask,(int)o.reservation.key);
    result=api.start(api.context,0,&plan);state("immediate",&o,result);
    if(o.pending[0]) {Delay(1);drained=pt_native_paula_output_notifications(&o);printf("PAULA DIAG one_tick_notification_drain=%d\n",drained);state("after_tick",&o,result);}
    if(!close_bounded(&o))return 13; /* Keep buffer if reader/IO uncertain. */
    if((*(volatile UWORD *)0xdff002&15) || o.starts || o.reservation.port)return 14;
    FreeMem(data,32);
    printf("PAULA OUTPUT DIAGNOSTIC PASS: original_start_result=%d; confirmed reader/WRITE/control/FREE/LOCK close, all requests/ports/device closed, Chip32 released; no playback acceptance\n",result);
    return 0;
}
