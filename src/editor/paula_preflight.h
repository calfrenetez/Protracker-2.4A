#ifndef PT_PAULA_PREFLIGHT_H
#define PT_PAULA_PREFLIGHT_H
#include "../core/render_commands.h"
#include "../core/paula_render_voice.h"
#include "../core/document.h"
enum pt_paula_capability {PT_PAULA_COMPATIBLE,PT_PAULA_INVALID,PT_PAULA_RENDER,
    PT_PAULA_MEMORY,PT_PAULA_RANGE,PT_PAULA_CHANNEL,PT_PAULA_SOURCE,
    PT_PAULA_GEOMETRY,PT_PAULA_CONTROL,PT_PAULA_OPERATION,PT_PAULA_PENDING};
struct pt_paula_preflight_report {
    enum pt_paula_capability result;enum pt_render_result render_result;
    uint64_t intervals,frames;unsigned action,channel;enum pt_render_action_kind kind;
    int8_t map[PT_CHANNEL_LIMIT];uint8_t samples[PT_PROJECT_SAMPLES];
};
/* Pure batch gate shared with future dispatch. Uses current stable map (must
 * equal pt_channels_paula_map(project,map)); examines only Paula actions but
 * refuses any out-of-project channel/unknown kind. Exact source descriptor
 * identity is resolved before dereference. TRIGGER/CONTROL/STOP only; unsupported
 * segment/repeat operations refuse. Tentative held state and sample mask commit
 * only on whole-batch success; report always locates first refusal. Plan/source
 * immutable, validated project borrowed; no cache/pins/device effects. */
enum pt_paula_capability pt_paula_check_plan(const struct pt_project *,unsigned rate,
    const int8_t map[PT_CHANNEL_LIMIT],const struct pt_render_plan *,
    const struct pt_paula_render_caps *,unsigned controls,uint16_t *held,
    struct pt_paula_preflight_report *);
/* Silent full16-track audited sequence traversal; it does not project a song to
 * four tracks. Non-Paula selected tracks/global flow remain in the shared
 * renderer; this gate only qualifies Paula operations, not other backends.
 * Entire timeline (including late rows) precedes any future playback permission.
 * Requires ordinary renderer-valid options/route selection and finite budgets.
 * Row-range restoration is unsupported and refuses before allocation.
 * Previous map may be NULL; report.map is the resulting stable assignment.
 * Exactly two bounded caller allocations (workspace+sequence), released on all paths.
 * No PCM mixing, source pins, cache allocation, hardware or driver callbacks.
 * Inputs/arrays/PCM are borrowed immutable through return, including allocator
 * callbacks; output disjoint. Success is capability evidence only, not a session,
 * memory guarantee or hardware/audio qualification. Revalidate after any edit.
 */
enum pt_paula_capability pt_paula_preflight(const struct pt_project *,const struct pt_render_options *,
    const int8_t *previous,const struct pt_paula_render_caps *,unsigned controls,
    const struct pt_allocator *,struct pt_paula_preflight_report *);
/* Transfer the SAME completely validated sequence, rewound without allocation
 * or remeasurement. Success publishes *sequence; refusal leaves it unchanged.
 * Recipient owns sequence allocation and must separately pin all source masters
 * before playback; patterns/metadata remain borrowed immutable. Report samples
 * covers Paula sources only. Other backends are not qualified or owned here. */
enum pt_paula_capability pt_paula_preflight_take(const struct pt_project *,const struct pt_render_options *,
    const int8_t *previous,const struct pt_paula_render_caps *,unsigned controls,
    const struct pt_allocator *,struct pt_paula_preflight_report *,struct pt_render_sequence **);
/* Cancellable alternative to the synchronous wrappers. Begin requires a NULL
 * work handle and copies options/caps/map. Project/PCM validation and static
 * metadata scans remain synchronous, before any playback deadline. Two bounded
 * allocations own workspace and sequence. Each later step performs at most256
 * measurement ticks, one next, <=256 consumed frames, or one complete/plan check.
 * PENDING and refusal expose no source masks; full late-row success precedes
 * pins/cache/output permission. Borrowed project/source storage stays immutable
 * until close or transferred sequence close; allocator callbacks must not edit.
 * Transfer returns the SAME successful sequence rewound once, no allocation or
 * remeasurement. Refusal preserves output; repeated transfer refuses. Close is
 * idempotent and cancels any phase. Handle is an owner, not copyable state.
 * Operation bounds are not wall-clock, native timing or hardware guarantees. */
struct pt_paula_preflight;
enum pt_paula_capability pt_paula_preflight_begin(const struct pt_project *,const struct pt_render_options *,
    const int8_t *,const struct pt_paula_render_caps *,unsigned,const struct pt_allocator *,
    struct pt_paula_preflight_report *,struct pt_paula_preflight **);
enum pt_paula_capability pt_paula_preflight_step(struct pt_paula_preflight *,struct pt_paula_preflight_report *);
int pt_paula_preflight_transfer(struct pt_paula_preflight *,struct pt_render_sequence **);
void pt_paula_preflight_close(struct pt_paula_preflight **);
/* Optional opaque INITIAL setup, distinct from the complete capability audit.
 * Begin is metadata-only and owns exactly two fixed ordinary task controls (analysis plus
 * renderer startup); no pins/cache/Chip/backend. Original p/o/caps/a/previous-map
 * storage and callback contexts remain immutable/alive through transfer/cancel.
 * Source revision/generation change for every in-place edit between calls; source
 * and metadata remain immutable DURING calls, including allocator callbacks.
 * Known full declared capacities are protected; opaque callback context extents
 * remain a caller disjointness obligation. Normal valid selected cursor is allowed.
 */
struct pt_paula_preflight_setup;
enum pt_render_setup_result pt_paula_preflight_setup_begin(const struct pt_project *,
    const struct pt_render_options *, const int8_t *previous,
    const struct pt_paula_render_caps *, unsigned controls,
    const struct pt_allocator *, uint32_t revision, uint32_t generation,
    struct pt_paula_preflight_setup **);
enum pt_render_setup_result pt_paula_preflight_setup_step(struct pt_paula_preflight_setup *,
    uint32_t revision, uint32_t generation, unsigned work);
enum pt_render_setup_result pt_paula_preflight_setup_get(struct pt_paula_preflight_setup *,
    uint32_t revision, uint32_t generation, struct pt_render_setup_report *);
/* READY is initial validation/static readiness only, NEVER COMPATIBLE or a used
 * source mask. Transfer consumes actual setup and publishes actual preflight work
 * still PENDING. Existing step audits the entire timeline; existing transfer then
 * moves the SAME audited sequence. New reset/restart/rewind uses private checked
 * templates without semantic rescans. Old synchronous/mutating paths stay old.
 * Transfer makes one fixed allocation for the actual sequence. CAPACITY preserves
 * READY setup for explicit retry. Across success there are three total allocation
 * calls/transient three live controls; renderer startup is then released, leaving
 * the usual analysis+sequence owners. Known returned aliases are refused without
 * release (not fresh owned memory). Observable callback failure may consume the
 * inner startup, leaving this outer handle failed and cancellable; no partial
 * preflight owner is published. Begin callbacks must be serialized; no owner-busy
 * detection is claimed before publication. No automatic retry or rebase.
 * After successful transfer, the actual audit owns option/cap/map/allocator copies;
 * original setup argument controls are no longer borrowed. Project/source storage
 * remains borrowed immutable through actual audit/sequence close. After the audited
 * sequence is transferred, closing the now-empty actual audit still releases it.
 * Copied allocator callback functions/context remain callable/alive until ALL
 * actual audit/sequence owners close, even after original argument structs expire.
 * Checked actual-audit step/transfer/close and ordinary sequence entrypoints
 * reject closing reentry. Existing lookahead workspaces still require their
 * sequence/project/source to outlive cancellation/commit; no broader lookahead
 * reentry guard is claimed. Callbacks must not free live controls before callback.
 */
enum pt_render_setup_result pt_paula_preflight_setup_transfer(struct pt_paula_preflight_setup **,
    uint32_t revision, uint32_t generation, struct pt_paula_preflight **);
enum pt_render_setup_result pt_paula_preflight_setup_cancel(struct pt_paula_preflight_setup **);
#endif
