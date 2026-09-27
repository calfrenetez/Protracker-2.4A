#ifndef PT_AMIGUS_VOICE_PLAN_H
#define PT_AMIGUS_VOICE_PLAN_H
#include "project.h"
#include "playback_pcm.h"
/* Register-width limits, NOT a detected card capacity. Caller must independently
 * verify its exclusive RAM region. Both pinned Hagen and SHIVA maps expose
 * address bits24..0; upload addresses are4-aligned, voice pointers2-aligned. */
#define PT_AMIGUS_RAM_ADDRESS_SPACE 0x02000000UL
struct pt_amigus_voice_request {
    uint32_t rate_numerator,rate_denominator; /* resolved frames/sec incl. pitch */
    uint32_t offset; /* source frame, NOT an interleaved element/byte */
    unsigned volume,pan; /* resolved0..64; linear pan0(left)..256(right) */
};
/* Software command plan only. End is a HALF-OPEN byte bound. Native register
 * lowering must verify endpoint/loop semantics and stop completion separately.
 * No register writes, source reads, pointers into master or dynamic allocation. */
struct pt_amigus_voice_plan {
    uint32_t start,loop,end_exclusive,rate;
    uint16_t control,left,right;
};
/* Takes one selected-channel8/16-bit cache with exact logical byte count (no
 * Paula word padding). Supports no loop/forward only; refuses odd pointers,
 * truncation, address overflow, zero/unsupported rates and unsupported loops.
 * Caller resolves period/MIDI/finetune to rational rate, volume/velocity and pan.
 * Does not apply those twice. Output remains unchanged on any refusal.
 * Metadata/PCM contents must already be validated by the project/cache owner. */
int pt_amigus_voice_plan_prepare(const struct pt_sample *,const struct pt_playback_format *,
    const struct pt_amigus_voice_request *,uint32_t address,uint32_t bytes,
    struct pt_amigus_voice_plan *);
#endif
