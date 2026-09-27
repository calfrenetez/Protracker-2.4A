#ifndef PT_WAVETABLE_DISPATCH_H
#define PT_WAVETABLE_DISPATCH_H
#include "wavetable_voices.h"
#include "../core/render_commands.h"
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
