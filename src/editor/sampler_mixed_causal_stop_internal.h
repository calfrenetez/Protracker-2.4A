#ifndef PT_SAMPLER_MIXED_CAUSAL_STOP_INTERNAL_H
#define PT_SAMPLER_MIXED_CAUSAL_STOP_INTERNAL_H
#include "sampler_mixed_causal_internal.h"
/* Separate original noncopyable binding. The WHOLE slot is zero and retained
 * inside the complete ordinary context aggregate. Genuine empty STOP owner
 * capability is checked under exclusion both at bind and open (including any
 * original ownership/exclusion callback); fields below
 * are identity beforeimages, never a public STOP/READY authorization flag.
 * One constructor attempt; no reset/rebind after refusal or pool consumption. */
struct pt_sampler_mixed_causal_stop_binding {
    struct pt_sampler_mixed_causal_binding original;
    struct pt_sampler_mixed_causal_stop_binding *self;
};
int pt_sampler_mixed_causal_stop_bind(struct pt_sampler_mixed_causal_stop_binding *,
    struct pt_mixed_causal_owner *,struct pt_mixed_readers_output *,
    uint64_t session,uint64_t generation,struct pt_mixed_readers_span contexts);
enum pt_sampler_mixed_result pt_sampler_mixed_causal_stop_open(
    const struct pt_sampler_mixed_config *,uint32_t,void *,size_t,
    struct pt_sampler_mixed_causal_stop_binding *,struct pt_sampler_mixed_pool **);
/* Finite alternative second command: absolute frame and genuine ORIGINAL
 * first reader handles only, unique route/slot, zero unused tail. No caller
 * numeric key, source/master/cache geometry, new reader, persistent pin/lease acquisition
 * or upload. Preparation derives actual keys after genuine completed first and
 * independently observed ACTIVE queue readers. One new genuine C holder is
 * prepared; enqueue/publish use the separate core STOP contract. Actual enqueue
 * OK alone transfers it even if an outer callback faults the factory. Original
 * readers retain cache/master resources until independent R/C/source proofs.
 * After-first preparation aperture is HOST-only; no native exact-schedule,
 * controller/producer, third lineage, device stop, IRQ or listening acceptance. */
struct pt_sampler_mixed_causal_stop_batch {
    uint64_t frame;
    unsigned count;
    struct pt_sampler_mixed_reader_handle reader[PT_SAMPLER_MIXED_ACTIONS];
};
enum pt_sampler_mixed_result pt_sampler_mixed_causal_stop_begin(
    struct pt_sampler_mixed_pool *,uint32_t,
    const struct pt_sampler_mixed_causal_stop_batch *,
    struct pt_sampler_mixed_command_handle *);
#endif
