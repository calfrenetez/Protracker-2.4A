#ifndef PT_AMIGUS_TRIGGER_LEVELS_H
#define PT_AMIGUS_TRIGGER_LEVELS_H
#include "amigus_voice_plan.h"

/* Explicit already-quantized register levels, not Q16 gains. The inactive
 * geometry.volume/pan MUST be zero. Every uint16 pair is meaningful, including
 * silence and independent maxima; no defaults, clamping or second gain law.
 * This additive request does not change the legacy request/helper ABI. */
struct pt_amigus_trigger_levels_request {
    struct pt_amigus_voice_request geometry;
    uint16_t left, right;
};
/* Metadata-only numeric plan. All five non-level fields and rational/cache
 * geometry are validated by the unchanged legacy helper. The sample descriptor,
 * selected format, whole request, complete declared int32 PCM capacity, declared
 * slice storage and output must be valid ordinary disjoint spans. Positive
 * frames require sufficient declared PCM capacity, but no PCM/slice values are
 * read or validated. Unknown enclosing caller contexts remain its obligation.
 * Whole local child request/plan spans are guarded before scratch initialization;
 * any refusal preserves the external output byte-for-byte. Success publishes
 * all seven fields once. No allocation, callbacks, PCM reads or device access.
 * Numeric address/alignment limits are not physical capacity or stop semantics;
 * the genuine factory still owns the allocation and stronger upload alignment.
 * Source8/16/24 and mono/stereo selected-channel caches8/16 retain legacy meaning.
 * Canonical renderer parity is a mono oracle only. This D1 helper alone does not
 * change factory/controller/normalizer admission, scheduling or authority. */
int pt_amigus_trigger_levels_prepare(const struct pt_sample *,
    const struct pt_playback_format *,
    const struct pt_amigus_trigger_levels_request *, uint32_t address,
    uint32_t logical_bytes, struct pt_amigus_voice_plan *);
#endif
