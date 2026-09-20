#include "frame_clock.h"
#include <stddef.h>

enum pt_clock_result pt_frame_clock_init(struct pt_frame_clock *c,uint32_t rate,uint64_t limit)
{
    struct pt_frame_clock next;
    if(!c || !rate || rate>192000 || !limit)return PT_CLOCK_INVALID;
    next.frames=0;next.limit=limit;next.rate=rate;next.fraction=0;*c=next;
    return PT_CLOCK_OK;
}

enum pt_clock_result pt_frame_clock_tick(struct pt_frame_clock *c,unsigned bpm,uint32_t *frames)
{
    uint64_t fixed,carry;uint32_t whole;
    if(!c || !frames || !c->rate || c->rate>192000 || !c->limit ||
       c->frames>c->limit || bpm<32 || bpm>255)return PT_CLOCK_INVALID;
    /* Public result must not alias live clock state. */
    if((uintptr_t)frames>UINTPTR_MAX-sizeof(*frames) ||
       (uintptr_t)c>UINTPTR_MAX-sizeof(*c) ||
       ((uintptr_t)frames<(uintptr_t)c+sizeof(*c) &&
        (uintptr_t)c<(uintptr_t)frames+sizeof(*frames)))return PT_CLOCK_INVALID;
    fixed=((uint64_t)c->rate*5<<31)/bpm;
    carry=(uint64_t)c->fraction+(uint32_t)fixed;
    whole=(uint32_t)(fixed>>32)+(uint32_t)(carry>>32);
    if(whole>c->limit-c->frames)return PT_CLOCK_LIMIT;
    c->frames+=whole;c->fraction=(uint32_t)carry;*frames=whole;
    return PT_CLOCK_OK;
}
