#include "elapsed_clock.h"
#include <stddef.h>
#include <string.h>
enum pt_elapsed_result pt_elapsed_clock_init(struct pt_elapsed_clock *c,uint32_t frequency,uint32_t rate,uint64_t ticks,uint64_t frames)
{
    struct pt_elapsed_clock next;
    if(!c || !frequency || !rate || rate>192000)return PT_ELAPSED_INVALID;
    memset(&next,0,sizeof(next));next.frequency=frequency;next.rate=rate;next.ticks=ticks;next.frames=frames;*c=next;
    return PT_ELAPSED_OK;
}
enum pt_elapsed_result pt_elapsed_clock_advance(struct pt_elapsed_clock *c,uint32_t frequency,uint64_t ticks,uint64_t *frames)
{
    uint64_t delta,whole,fraction,add,total;uintptr_t a=(uintptr_t)c,b=(uintptr_t)frames;
    if(!c || !frames || !c->frequency || !c->rate || c->rate>192000 || c->fraction>=c->frequency ||
       a>UINTPTR_MAX-sizeof(*c) || b>UINTPTR_MAX-sizeof(*frames) || (a<b+sizeof(*frames) && b<a+sizeof(*c)))
        return PT_ELAPSED_INVALID;
    if(c->failure)return c->failure;
    if(frequency!=c->frequency)return c->failure=PT_ELAPSED_FREQUENCY;
    if(ticks<c->ticks)return c->failure=PT_ELAPSED_REGRESSION;
    delta=ticks-c->ticks;whole=delta/c->frequency;
    /* Split before multiplying; delta*rate may overflow even when the resulting
       frame count fits. The remainder product is bounded by UINT32_MAX*192000. */
    if(whole>(UINT64_MAX-c->frames)/c->rate)return c->failure=PT_ELAPSED_OVERFLOW;
    add=whole*c->rate;fraction=(delta%c->frequency)*c->rate+c->fraction;
    total=fraction/c->frequency;
    if(total>UINT64_MAX-c->frames-add)return c->failure=PT_ELAPSED_OVERFLOW;
    c->frames+=add+total;c->fraction=(uint32_t)(fraction%c->frequency);c->ticks=ticks;
    *frames=c->frames;return PT_ELAPSED_OK;
}
