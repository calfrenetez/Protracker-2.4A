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
 * validation and static scans remain synchronous, outside playback deadlines.
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
#endif
