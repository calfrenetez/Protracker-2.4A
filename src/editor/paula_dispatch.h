#ifndef PT_PAULA_DISPATCH_H
#define PT_PAULA_DISPATCH_H
#include "paula_voices.h"
#include "paula_preflight.h"
/* Apply one successful shared-sequence plan. Caller captures bridge.version
 * after sync and owns immutable project/master descriptors throughout the call.
 * Whole plan, stable map, current revision and held readers are checked before
 * any device callback. Every trigger cache lease/address is prepared first.
 * Return1 applied,0 preparation refusal (no callbacks, old readers preserved),
 * -1 runtime failure: owner is closing, each held slot gets one bounded stop
 * attempt and unconfirmed leases remain owned. Discard plan/sequence; never
 * retry partial output. Normal close must confirm stops/quiescence afterwards.
 * Unchanged master promotion/cache warming may remain after refusal. Serialized
 * owner thread, no callback/allocator reentry or edits. Only exact one-shot
 * mono TRIGGER/CONTROL/STOP; no repeat, restore, scheduling or native DMA here.
 * Caller-owned bounded workspace, disjoint from inputs; zero-init not required. */
struct pt_paula_batch_entry {
    struct pt_cache_lease lease;struct pt_paula_voice_plan plan;
    unsigned held,sample;
};
struct pt_paula_batch {struct pt_paula_batch_entry entry[PT_RENDER_ACTIONS];};
int pt_paula_dispatch(struct pt_paula_voices *,uint64_t version,unsigned rate,
    const struct pt_render_plan *,const struct pt_paula_render_caps *,struct pt_paula_batch *);
#endif
