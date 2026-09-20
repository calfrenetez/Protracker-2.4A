#include "timeline.h"
#include <string.h>
enum pt_timeline_result pt_timeline_init(struct pt_timeline *out,const struct pt_project *p,
                                       enum pt_flow_mode mode,unsigned start,uint32_t rate,
                                       uint32_t ticks,uint64_t frames)
{
    struct pt_timeline next;
    if(!out)return PT_TIMELINE_INVALID;
    memset(&next,0,sizeof(next));
    if(pt_flow_init(&next.flow,p,mode,start,ticks)!=PT_FLOW_TICK ||
       pt_frame_clock_init(&next.clock,rate,frames)!=PT_CLOCK_OK)return PT_TIMELINE_INVALID;
    *out=next;return PT_TIMELINE_TICK;
}
enum pt_timeline_result pt_timeline_next(struct pt_timeline *out,struct pt_tick_span *span)
{
    struct pt_timeline next;struct pt_tick_span result;enum pt_flow_result flow;enum pt_clock_result clock;
    if(!out || !span)return PT_TIMELINE_INVALID;
    if((uintptr_t)span>UINTPTR_MAX-sizeof(*span) || (uintptr_t)out>UINTPTR_MAX-sizeof(*out) ||
       ((uintptr_t)span<(uintptr_t)out+sizeof(*out) && (uintptr_t)out<(uintptr_t)span+sizeof(*span)))return PT_TIMELINE_INVALID;
    next=*out;flow=pt_flow_tick(&next.flow);
    if(flow!=PT_FLOW_TICK)return flow==PT_FLOW_STOPPED?PT_TIMELINE_STOPPED:
        flow==PT_FLOW_LIMIT?PT_TIMELINE_TICK_LIMIT:PT_TIMELINE_INVALID;
    memset(&result,0,sizeof(result));result.start=next.clock.frames;
    clock=pt_frame_clock_tick(&next.clock,out->flow.bpm,&result.frames);
    if(clock!=PT_CLOCK_OK)return clock==PT_CLOCK_LIMIT?PT_TIMELINE_FRAME_LIMIT:PT_TIMELINE_INVALID;
    result.end=next.clock.frames;*out=next;*span=result;return PT_TIMELINE_TICK;
}
