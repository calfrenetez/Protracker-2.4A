#ifndef PT_TIMELINE_H
#define PT_TIMELINE_H
#include "flow.h"
#include "frame_clock.h"
/* Ideal-BPM offline reference timeline, not a measured native/CIA clock. */
struct pt_timeline { struct pt_flow flow;struct pt_frame_clock clock; };
struct pt_tick_span { uint64_t start,end;uint32_t frames; };
enum pt_timeline_result { PT_TIMELINE_TICK, PT_TIMELINE_STOPPED,
                         PT_TIMELINE_TICK_LIMIT, PT_TIMELINE_FRAME_LIMIT,
                         PT_TIMELINE_INVALID };
enum pt_timeline_result pt_timeline_init(struct pt_timeline *,const struct pt_project *,
                                       enum pt_flow_mode,unsigned start,uint32_t rate,
                                       uint32_t tick_limit,uint64_t frame_limit);
/* Publishes the span BEFORE the completed tick. Render that span with the old
 * voice state, then apply this tick's new flow/effect commands at span.end. In
 * particular a new Fxx tempo changes the next interval, not the elapsed one.
 * Every non-TICK outcome leaves both timeline and output span unchanged. */
enum pt_timeline_result pt_timeline_next(struct pt_timeline *,struct pt_tick_span *);
#endif
