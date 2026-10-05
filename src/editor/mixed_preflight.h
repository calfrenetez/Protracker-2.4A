#ifndef PT_MIXED_PREFLIGHT_H
#define PT_MIXED_PREFLIGHT_H
#include "paula_preflight.h"
#include "wavetable_dispatch.h"
enum pt_mixed_result {PT_MIXED_OK,PT_MIXED_INVALID,PT_MIXED_RENDER,PT_MIXED_MEMORY,PT_MIXED_RANGE,PT_MIXED_ROUTE,PT_MIXED_PAULA,PT_MIXED_AMIGUS,PT_MIXED_PENDING};
struct pt_mixed_report {
    enum pt_mixed_result result;enum pt_render_result render_result;
    enum pt_paula_capability paula;enum pt_wavetable_capability amigus;
    uint64_t intervals,frames;unsigned action,channel;enum pt_render_action_kind kind;
    int8_t map[PT_CHANNEL_LIMIT];uint8_t samples[2][PT_PROJECT_SAMPLES];
};
/* Silent ONE full16-track sequence traversal, ordinary whole-song44.1/48kHz.
 * Selected routes must be Paula/AmiGUS; MIDI and range restore refuse. Both
 * existing backend capability rules apply to route-partitioned plans retaining
 * global channel/action identity. Global tempo/delay/end flow stays intact.
 * Full late-row validation precedes success; per-backend used-source masks valid
 * only on OK. Two bounded allocations (workspace+sequence), no pins/cache/device
 * effects or PCM mixing. Borrowed source/project arrays immutable until return
 * and through any transferred playback sequence; allocator callbacks no edits.
 * Failure leaves *sequence unchanged and clears both source masks. Success may
 * transfer SAME sequence rewound once without remeasurement; recipient must own
 * sources and close sequence. Passing gate is NOT mixed playback ownership,
 * memory/capacity guarantee, native timing, dispatch or hardware acceptance. */
enum pt_mixed_result pt_mixed_preflight(const struct pt_project *,const struct pt_render_options *,
    const int8_t *,const struct pt_paula_render_caps *,const struct pt_playback_format *,
    unsigned,unsigned,const struct pt_allocator *,struct pt_mixed_report *,struct pt_render_sequence **);
/* Cancellable version of the same gate. Begin requires a NULL work handle and
 * captures options/caps/format/map, with TWO bounded allocations. Initial project
 * validation and static scans of this legacy begin remain synchronous, outside
 * playback deadlines.
 * Each step performs at most256 measurement ticks, one next, <=256 consumed
 * frames, or one complete/backend-plan check. No pins/cache/upload/output.
 * Pending and failure reports expose no source masks. Inputs remain immutable
 * through close or transferred playback; serialize calls without callback edits.
 * Take transfers the SAME successful sequence, rewound once, without allocation
 * or remeasurement. Refusal preserves output; a second take refuses. Close is
 * idempotent and cancels any phase, including completed/failed untaken work.
 * The handle is an owner, not copyable state. Progress bounds are not a timing
 * guarantee and do not qualify device capacity, scheduling or hardware output. */
struct pt_mixed_preflight;
enum pt_mixed_result pt_mixed_preflight_begin(const struct pt_project *,const struct pt_render_options *,
    const int8_t *,const struct pt_paula_render_caps *,const struct pt_playback_format *,
    unsigned,unsigned,const struct pt_allocator *,struct pt_mixed_report *,struct pt_mixed_preflight **);
enum pt_mixed_result pt_mixed_preflight_step(struct pt_mixed_preflight *,struct pt_mixed_report *);
int pt_mixed_preflight_take(struct pt_mixed_preflight *,struct pt_render_sequence **);
void pt_mixed_preflight_close(struct pt_mixed_preflight **);
/* Optional opaque INITIAL mixed setup. This owner embeds the future actual
 * audit and owns the genuine renderer startup; no public validator/flow state,
 * certificate or ready flag is consumed. Begin is metadata-only, with two fixed
 * ordinary task controls; no PCM/order/event/slice value reads, master pins,
 * cache allocation/upload, bus/voice/backend calls or device output. Original p/o/caps/
 * format/a/previous-map storage remains immutable/alive through transfer/cancel,
 * including allocation/release callbacks. Known full source capacities and all
 * named controls are guarded; unenumerated opaque callback-context extents stay
 * caller-disjoint. Begin callback reentry must be serialized before an owner
 * exists. After publication, callback reentry latches failure rather than freeing
 * a live owner. Valid selection-cursor movement is permitted; every other edit
 * between calls changes revision/generation, and all source edits DURING calls
 * are forbidden. These are source tags, not backend timestamp generation.
 */
struct pt_mixed_preflight_setup;
enum pt_render_setup_result pt_mixed_preflight_setup_begin(const struct pt_project *,
    const struct pt_render_options *,const int8_t *previous,
    const struct pt_paula_render_caps *,const struct pt_playback_format *,
    unsigned paula_controls,unsigned amigus_controls,const struct pt_allocator *,
    uint32_t revision,uint32_t generation,struct pt_mixed_preflight_setup **);
/* One genuine renderer phase per call; work1..4096 bounds charged semantic or
 * metadata items. Fixed header/alias/metadata guards are separately finite,
 * never a wall-clock/IRQ guarantee. READY is initial validation/static readiness
 * only, never PT_MIXED_OK or a source mask. NULL get report is a checked query;
 * stale/alias refusal preserves a non-NULL report.
 */
enum pt_render_setup_result pt_mixed_preflight_setup_step(struct pt_mixed_preflight_setup *,
    uint32_t revision,uint32_t generation,unsigned work);
enum pt_render_setup_result pt_mixed_preflight_setup_get(struct pt_mixed_preflight_setup *,
    uint32_t revision,uint32_t generation,struct pt_render_setup_report *);
/* Consume actual completed startup, publishing the same actual mixed audit
 * still PT_MIXED_PENDING. Existing step then performs the full two-route timeline
 * audit; existing take moves the SAME successful sequence rewound once. Private
 * checked reset/measurement restart/rewind avoids semantic rescans. Transfer
 * makes one fixed sequence allocation: three calls/transient three live controls
 * in total, then analysis+sequence. CAPACITY preserves READY for explicit retry.
 * Returned aliases to finite recognized guarded extents are refused without
 * release as fresh ownership; allocators must supply fresh disjoint storage. An
 * observable callback fault may consume inner startup and leave the failed outer
 * owner cancellable, with no partial audit publication. Callback-mutated outputs
 * are not overwritten. Normal pending/alias/capacity refusals preserve handles.
 * After successful transfer, actual audit owns caps/format/map/options/allocator
 * copies; original argument structs cease borrowed. Copied callback functions/
 * contexts remain callable until ALL audit/sequence owners close. Project and
 * all tables/PCM/markers/extensions stay immutable/alive through transferred
 * sequence close. Pin promotion that replaces source descriptors must occur
 * before this borrowed lifetime, not during it.
 * Checked audit step/take/close and ordinary sequence entrypoints reject closing
 * reentry. Existing lookahead contract still requires its sequence/source to
 * outlive cancel/commit; no broader lookahead guarantee is added.
 * Genuine handle slots must be disjoint from borrowed source/control storage.
 * Close the empty audit after taking its sequence; cancellation reads no former
 * source arrays and is idempotent. Failure after release may consume to NULL.
 * Legacy begin/preflight and mixed_owner are unchanged. In particular mixed_owner
 * initial sync scans and promotion policy remain separate integration work.
 * Native/emulator/physical execution, placement, timing and hardware are NOT
 * qualified; no scheduling, activation or editor PLAY path is installed here.
 */
enum pt_render_setup_result pt_mixed_preflight_setup_transfer(struct pt_mixed_preflight_setup **,
    uint32_t revision,uint32_t generation,struct pt_mixed_preflight **);
enum pt_render_setup_result pt_mixed_preflight_setup_cancel(struct pt_mixed_preflight_setup **);
#endif
