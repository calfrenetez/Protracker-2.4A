#ifndef PT_WAVETABLE_DISPATCH_H
#define PT_WAVETABLE_DISPATCH_H
#include "wavetable_voices.h"
#include "../core/render_commands.h"
enum pt_wavetable_capability {
    PT_WAVETABLE_COMPATIBLE, PT_WAVETABLE_INVALID, PT_WAVETABLE_RENDER,
    PT_WAVETABLE_MEMORY, PT_WAVETABLE_FORMAT, PT_WAVETABLE_CHANNEL,
    PT_WAVETABLE_SOURCE, PT_WAVETABLE_GEOMETRY, PT_WAVETABLE_CONTROL,
    PT_WAVETABLE_OPERATION
};
struct pt_wavetable_preflight_report {
    enum pt_wavetable_capability result;
    enum pt_render_result render_result;
    uint64_t intervals,frames; /* All consumed spans, including silent pre-roll. */
    unsigned action,channel; /* Zero-based action; UINT_MAX means no action. */
    enum pt_render_action_kind kind;
    uint8_t samples[PT_PROJECT_SAMPLES]; /* Triggered slots; valid on COMPATIBLE. */
};
/* Silent full-sequence capability analysis with the SAME rules as dispatch.
 * Initial voices are idle. Includes pre-roll, late rows and retained loops;
 * advances audited renderer phases in <=256-frame blocks. Existing required
 * tick/frame limits bound both measurement and traversal. Two caller allocations
 * (plan and sequence), always released, with no master writes, cache
 * allocation, reservation or device callbacks. Renderer validation may inspect
 * source PCM (e.g. classic silent-repeat words); no audio is mixed. controls
 * declares whether the eventual driver supplies the phase-preserving control callback.
 * Project/options/format and source descriptors must remain immutable throughout;
 * allocator callbacks must not edit/reenter. Report is always written when non-NULL.
 * Success is capability evidence for these exact inputs, NOT an owned playback
 * session, capacity guarantee, hardware acceptance or scheduling policy. Edits,
 * route/format/options changes require revalidation; playback still needs source
 * ownership, current bridge revision and per-batch/address checks. In particular
 * this does not authorize audible pre-roll or mid-voice range starts. */
enum pt_wavetable_capability pt_wavetable_preflight(const struct pt_project *,
    const struct pt_render_options *,const struct pt_playback_format *,unsigned controls,
    const struct pt_allocator *,struct pt_wavetable_preflight_report *);
/* Apply one successful audited renderer plan after the preceding interval.
 * Capture bridge.version AFTER sync when constructing plan. Project/master
 * descriptors must remain immutable/current; exact descriptor identity is
 * required (foreign/private EFx sources refuse without dereference).
 * All actions preflight before callbacks. Mono TRIGGER/CONTROL/STOP only;
 * segment/repeat/stereo/unsupported geometry refuse unchanged. All slots share
 * explicit8/16-bit output format. CONTROL does not retrigger or change phase.
 * Returns1 applied,0 preflight refusal (no device callbacks),-1 runtime failure.
 * Runtime failure blocks new triggers, attempts each held voice's stop ONCE,
 * retains every unconfirmed lease. Discard sequence/plan; close owner, never retry
 * a partially applied plan. No scheduling, hardware or real stop proof here. */
int pt_wavetable_dispatch(struct pt_wavetable_voices *,uint64_t version,unsigned output_rate,
    const struct pt_render_plan *,const struct pt_playback_format *,uint8_t *staging,size_t capacity);
#endif
