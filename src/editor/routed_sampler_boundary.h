#ifndef PT_ROUTED_SAMPLER_BOUNDARY_H
#define PT_ROUTED_SAMPLER_BOUNDARY_H

#include "../core/routed_playback.h"
#include "sampler_mixed_readers.h"

enum pt_routed_sampler_result {
    PT_ROUTED_SAMPLER_INVALID, PT_ROUTED_SAMPLER_PENDING, PT_ROUTED_SAMPLER_READY,
    PT_ROUTED_SAMPLER_REFUSED, PT_ROUTED_SAMPLER_STALE
};

struct pt_routed_sampler_config {
    struct pt_sampler_mixed_pool *pool;
    const struct pt_project *project;
    struct pt_sample_version *const *masters; /* Project sample_count held pointer slots. */
    struct pt_mixed_plan_origin *origins; /* PT_MIXED_PLAN_RECORDS entries. */
    struct pt_paula_render_caps caps;
    struct pt_playback_format format;
    void *workspace;
    size_t workspace_bytes;
    uint32_t rate, revision, generation;
};

/* Fresh zeroed, disjoint task storage. Keep the config, origins, held masters,
 * normalizer workspace and original renderer/route owners live and immutable
 * while this boundary is pending. All calls are serialized, with no callback
 * reentry. Acquire trigger reader handles from command before publishing.
 * The caller services the exact ticket and keeps every adopted reader alive
 * through its own independent retirement. */
struct pt_routed_sampler_boundary {
    struct pt_routed_playback_frame playback;
    struct pt_routed_render_plan plan;
    struct pt_mixed_plan_inputs inputs;
    struct pt_mixed_plan_normalizer *normalizer;
    struct pt_sampler_mixed_normalized_batch batch;
    struct pt_mixed_readers_stop_keys stop;
    struct pt_sampler_mixed_command_handle command;
    const struct pt_routed_sampler_config *config;
    uint64_t ticket;
    enum pt_routed_sampler_result phase;
    unsigned publish_attempted, published;
};

/* Bind the genuinely ready lookahead. A refusal may leave a private binding;
 * abandon it before starting a separate recovery or playback operation. */
enum pt_routed_sampler_result pt_routed_sampler_begin(struct pt_routed_sampler_boundary *,
    const struct pt_routed_sampler_config *, struct pt_render_lookahead *, struct pt_routed_midi_schedule *);
/* At most one bounded normalizer step or factory transition. */
enum pt_routed_sampler_result pt_routed_sampler_step(struct pt_routed_sampler_boundary *,
    uint32_t revision, uint32_t generation, unsigned work);
/* Enqueue and attempt the exact common publication once. On any uncertain
 * result, retain the original command, ticket and schedule for settlement. */
int pt_routed_sampler_publish(struct pt_routed_sampler_boundary *, uint32_t revision, uint32_t generation);
/* Requires source consumption, command detach and the exact combined receipt.
 * Only a successful commit updates origins. Between boundaries the caller
 * removes origins for readers independently proved retired. */
enum pt_render_result pt_routed_sampler_commit(struct pt_routed_sampler_boundary *, uint32_t revision, uint32_t generation);
/* Discard private preparation before publication or retire this local binding
 * after externally verified command detach and schedule settlement. A failed
 * boundary is not reusable as a new playback attempt on the same forecast. */
int pt_routed_sampler_abandon(struct pt_routed_sampler_boundary *);

#endif
