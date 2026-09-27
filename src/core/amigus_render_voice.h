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
#endif
