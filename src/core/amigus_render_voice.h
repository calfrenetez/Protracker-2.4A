#ifndef PT_AMIGUS_RENDER_VOICE_H
#define PT_AMIGUS_RENDER_VOICE_H
#include "render_commands.h"
#include "amigus_voice_plan.h"
/* Translate resolved renderer Q32 step and per-side Q16 gains directly. No
 * repeat pan law/finetune/velocity application. Output unchanged on refusal. */
int pt_amigus_render_control(uint64_t step,unsigned output_rate,const uint32_t gains[2],
    uint32_t *rate,uint16_t *left,uint16_t *right);
/* Ordinary initialized mono trigger only. Caller has resolved pcm identity to
 * a validated immutable source and pinned cache. Segments, pending handoffs,
 * pingpong and stereo refuse. No source reads, allocation, callbacks or I/O.
 * End remains a half-open software bound, not a verified hardware register. */
int pt_amigus_render_voice(const struct pt_voice *,unsigned output_rate,const uint32_t gains[2],
    const struct pt_playback_format *,uint32_t address,uint32_t bytes,struct pt_amigus_voice_plan *);
/* Exact restore description, NOT a register image or ordinary trigger. Cursor
 * is absolute CACHE BYTE position in unsigned Q32 (including fractional byte).
 * Bounds retain the original start, forward loop and half-open end; cursor may
 * be odd/fractional even when bounds are aligned. A future driver must explicitly
 * support exact restore or refuse BEFORE output. Never pass bounds alone to start.
 * Current MMIO/voice APIs do not consume this plan. No PCM pointers are retained. */
struct pt_amigus_restore_plan {
    struct pt_amigus_voice_plan bounds;
    uint64_t cursor_q32;
};
/* Active ordinary mono one-shot/forward voice only. Validates exact phase/cycle;
 * rejects segments, handoffs, pingpong, stereo and out-of-range state. Source
 * metadata/storage already validated by owner. No allocation/PCM reads/pinning;
 * cache must remain leased through any future restore and confirmed stop.
 * Source24-bit masters are unchanged; format selects only the8/16-bit cache.
 * Output is untouched on refusal. Does not claim native register semantics. */
int pt_amigus_render_restore(const struct pt_voice *,unsigned output_rate,const uint32_t gains[2],
    const struct pt_playback_format *,uint32_t address,uint32_t bytes,struct pt_amigus_restore_plan *);
#endif
