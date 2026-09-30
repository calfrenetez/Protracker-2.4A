#ifndef PT_MIXED_PREFLIGHT_H
#define PT_MIXED_PREFLIGHT_H
#include "paula_preflight.h"
#include "wavetable_dispatch.h"
enum pt_mixed_result {PT_MIXED_OK,PT_MIXED_INVALID,PT_MIXED_RENDER,PT_MIXED_MEMORY,PT_MIXED_RANGE,PT_MIXED_ROUTE,PT_MIXED_PAULA,PT_MIXED_AMIGUS};
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
#endif
