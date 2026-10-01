#include "../src/native/paula_output.h"
#include <proto/dos.h>
#include <stdio.h>
/* Silence at volume0: actual device WRITE/DMA readers, no listening claim.
 * One bounded attempt, never retry/reset. Keep Chip storage on uncertain stop. */
static int close_bounded(struct pt_native_paula_output *o)
{unsigned n;for(n=0;n<50;++n){if(pt_native_paula_output_close(o))return 1;Delay(1);}return 0;}
int main(void)
{
    struct pt_native_paula_output o={0};struct pt_paula_voice_api api;
    struct pt_paula_voice_plan plan;UBYTE *data;unsigned i,n;int result=0,ready=0;
    if(!pt_native_paula_output_open(&o) || !close_bounded(&o))return 10;
    if(!pt_native_paula_output_open(&o))return 11;
    for(n=0;n<50;++n){ready=pt_native_paula_output_advance(&o);if(ready)break;Delay(1);}
    if(ready!=1 || !pt_native_paula_output_api(&o,&api)){close_bounded(&o);return 12;}
    data=AllocMem(32,MEMF_CHIP|MEMF_PUBLIC|MEMF_CLEAR);if(!data){close_bounded(&o);return 13;}
    plan.data=data;plan.words=16;plan.period=400;plan.volume=0;
    for(i=0;i<4;++i)if(api.start(api.context,i,&plan)!=1){result=14;break;}
    if(!result && (*(volatile UWORD *)0xdff002&15)!=15)result=15;
    if(!result)for(i=0;i<4;++i)if(api.control(api.context,i,500,0)!=1){result=16;break;}
    /* Delay outside callbacks; confirms retained readers survive a task turn. */
    if(!result)Delay(1);
    if(!close_bounded(&o))return 17; /* Intentionally retain data if uncertain. */
    if((*(volatile UWORD *)0xdff002&15) || o.starts || o.reservation.port)return 18;
    FreeMem(data,32);if(result)return result;
    printf("PAULA OUTPUT PASS: idle close and four acknowledged silent DMA voices; control, confirmed stop, Chip storage and all device resources released; clock=%lu\n",(unsigned long)SysBase->ex_EClockFrequency*5);
    return 0;
}
