#ifndef PT_FLOW_H
#define PT_FLOW_H
#include "project.h"

/* Control flow only, not an audio player. The validated project is borrowed and
 * must remain immutable/alive until this state is discarded. No allocation or
 * hardware access. Extended mode preserves ascending 1..16 track processing and
 * uses all 256 order positions; classic mode preserves the native 7-bit wrap. */
enum pt_flow_mode { PT_FLOW_CLASSIC128, PT_FLOW_EXTENDED256 };
enum pt_flow_result { PT_FLOW_TICK, PT_FLOW_STOPPED, PT_FLOW_LIMIT, PT_FLOW_INVALID };
struct pt_flow {
    const struct pt_project *project;
    /* positions counts order transitions; returns counts destinations <= the
       order at tick entry. E6 row loops do not count as order transitions. */
    uint32_t ticks,fetches,limit,positions,returns;
    uint16_t order,played_order,row,played_row,bpm;
    uint8_t counter,speed,active,fresh,delayed,pending_delay,delay;
    uint8_t break_row,jump,loop_break,mode;
    uint8_t loop_start[PT_CHANNEL_LIMIT],loop_count[PT_CHANNEL_LIMIT];
    uint8_t effect[PT_CHANNEL_LIMIT],parameter[PT_CHANNEL_LIMIT];
};
/* Failure leaves *out untouched. Classic requires 4 tracks, <=128 positions and
 * initial 6/125, as used by the pinned CIA ABI. The extended clock starts from
 * the project's speed/BPM. Both retain the native initial speed-tick lead-in. */
enum pt_flow_result pt_flow_init(struct pt_flow *out,const struct pt_project *,
                               enum pt_flow_mode,unsigned start_order,uint32_t tick_limit);
/* PT_FLOW_TICK publishes one completed tick, including the F00 tick. Inspect
 * fresh/delayed and played vs next cursor separately. The next call reports
 * STOPPED or LIMIT without changing state. No successful-song-end inference:
 * native songs wrap, so a looping song reaches its explicit tick budget. */
enum pt_flow_result pt_flow_tick(struct pt_flow *);
#endif
