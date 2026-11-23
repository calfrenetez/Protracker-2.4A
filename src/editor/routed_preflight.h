#ifndef PT_ROUTED_PREFLIGHT_H
#define PT_ROUTED_PREFLIGHT_H

#include "../core/document.h"
#include "mixed_readers_plan_normalize.h"

struct pt_routed_preflight;

enum pt_routed_preflight_result {
    PT_ROUTED_PREFLIGHT_INVALID, PT_ROUTED_PREFLIGHT_PENDING, PT_ROUTED_PREFLIGHT_READY,
    PT_ROUTED_PREFLIGHT_REFUSED, PT_ROUTED_PREFLIGHT_STALE
};

struct pt_routed_preflight_inputs {
    const struct pt_project *project;
    struct pt_render_options options;
    struct pt_paula_render_caps caps;
    struct pt_playback_format format;
    struct pt_allocator allocator;
    void *normalizer_workspace;
    size_t normalizer_capacity;
    uint32_t revision, generation;
};

struct pt_routed_preflight_report {
    uint64_t frame, audio_boundaries;
    enum pt_render_result render;
    struct pt_mixed_plan_report plan;
};

/* Audit an entire routed sequence with the playback quantizer before handing
 * it to a session. Provide separate, zeroed, aligned caller workspaces for
 * this control and the normalizer. The allocator supplies a fresh renderer.
 * Project storage stays immutable through the transferred sequence's lifetime.
 * Preserve the owner slot and both workspaces through close. Serialize calls.
 * Begin validates renderer inputs synchronously. No audio or MIDI output,
 * sampler/cache operations or device callbacks occur during the audit. */
size_t pt_routed_preflight_workspace_size(void);
size_t pt_routed_preflight_workspace_alignment(void);
enum pt_routed_preflight_result pt_routed_preflight_begin(void *, size_t,
    const struct pt_routed_preflight_inputs *, struct pt_routed_preflight **);

/* One renderer or normalizer phase per call. Work 1..256 limits ticks, frames
 * or normalizer items. Metadata checks and one plan completion are separate
 * from that budget. A stale tag refuses before walking former source tables.
 * Report may be NULL, and is unchanged on stale or invalid calls. */
enum pt_routed_preflight_result pt_routed_preflight_step(struct pt_routed_preflight *,
    uint32_t revision, uint32_t generation, unsigned work, struct pt_routed_preflight_report *);

/* Rewind and transfer the same validated sequence once. Refusal preserves
 * the caller's output. Close cancels the audit or disposes its empty control. */
int pt_routed_preflight_take(struct pt_routed_preflight *, uint32_t revision, uint32_t generation,
    struct pt_render_sequence **);
int pt_routed_preflight_close(struct pt_routed_preflight **);

#endif
