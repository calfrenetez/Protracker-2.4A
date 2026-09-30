#ifndef PT_PAULA_DISPATCH_H
#define PT_PAULA_DISPATCH_H
#include "paula_voices.h"
#include "paula_preflight.h"
#include "sampler_paula_internal.h"
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
    struct pt_cache_lease lease;struct pt_paula_voice_plan plan;struct pt_pcm source;
    unsigned held,sample;uint32_t offset,length;
};
struct pt_paula_batch {struct pt_paula_batch_entry entry[PT_RENDER_ACTIONS];};
int pt_paula_dispatch(struct pt_paula_voices *,uint64_t version,unsigned rate,
    const struct pt_render_plan *,const struct pt_paula_render_caps *,struct pt_paula_batch *);
/* Zero-init once, do not copy/reinitialize while preparing/ready. Preparation may allocate
 * and convert; it emits no callbacks, retains every candidate lease and takes
 * exclusive voice ownership. Other public operations refuse until apply/cancel.
 * Keep project/master descriptors immutable and all contexts alive. Apply checks
 * identities again and emits without cache acquisition/allocation/conversion.
 * Returns1 applied,0 refusal without output,-1 poisoned partial output as above.
 * All outcomes retire the prepared batch; cancel releases only unstarted leases.
 * Public preparation is synchronous; private validated-session staging uses bounded
 * conversion steps. No deadlines, native scheduling or output provided. */
struct pt_paula_prepared {
    struct pt_paula_batch batch;struct pt_render_plan plan;struct pt_paula_render_caps caps;
    struct pt_paula_voices *voices;struct pt_sampler_paula *bridge;struct pt_project *project,header;
    struct pt_paula_voice_api api;struct pt_paula_voice voice[PT_PAULA_VOICES];
    int8_t map[PT_CHANNEL_LIMIT];int (*quiesce)(void *);void *quiesce_context,*owner;
    struct pt_sampler_paula_job job;struct pt_sample_version *master[PT_RENDER_ACTIONS];
    uint64_t version;unsigned rate,ready,claims,preparing,incremental,index;
};
int pt_paula_prepare(struct pt_paula_prepared *,struct pt_paula_voices *,uint64_t,unsigned,
    const struct pt_render_plan *,const struct pt_paula_render_caps *);
int pt_paula_apply(struct pt_paula_prepared *);
void pt_paula_cancel(struct pt_paula_prepared *);

#endif
