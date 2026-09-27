#ifndef PT_VOICE_INTERNAL_H
#define PT_VOICE_INTERNAL_H
#include "voice.h"
/* Immutable renderer/prepared Studio entry points AFTER full source-value validation and while all
 * source values remain immutable. Descriptor shape/capacity, bounds, format and
 * alias checks still run; only the value scan is omitted. No validation token,
 * ownership or pin is created. Never use for untrusted or mutable playback PCM.
 * Public voice APIs retain full validation. Same atomic failure semantics. */
enum pt_pcm_result pt_voice_init_validated(struct pt_voice *,const struct pt_pcm *,
    uint32_t,uint32_t,enum pt_voice_loop,uint32_t,uint32_t,uint64_t,unsigned);
enum pt_pcm_result pt_voice_init_segment_validated(struct pt_voice *,const struct pt_pcm *,
    uint32_t,uint32_t,uint32_t,uint32_t,uint64_t,unsigned);
enum pt_pcm_result pt_voice_set_repeat_source_validated(struct pt_voice *,const struct pt_pcm *,uint32_t,uint32_t);
#endif
