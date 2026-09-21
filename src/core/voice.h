#ifndef PT_VOICE_H
#define PT_VOICE_H
#include "pcm.h"
enum pt_voice_loop { PT_VOICE_ONCE, PT_VOICE_FORWARD, PT_VOICE_PINGPONG };
struct pt_voice {
    const struct pt_pcm *pcm,*repeat_pcm;
    uint64_t phase,step,cycle;
    uint32_t start,end,loop_start,loop_end;
    uint8_t loop,looped,linear,active,segment;
};
/* Borrowed immutable PCM; no allocation or sample writes. Step is positive Q32
 * source frames per output frame. Bounds are half-open; loops must be contained
 * in the selected range. Pingpong has no duplicated endpoints and at most
 * 2^31 source frames per loop. Only a completely empty PCM permits start=end.
 * Init validates all PCM; subsequent calls require that PCM/state remain valid.
 * Zeroed, uninitialized voices may be supplied to the mixer as silent slots. */
enum pt_pcm_result pt_voice_init(struct pt_voice *,const struct pt_pcm *,
                                uint32_t start,uint32_t end,enum pt_voice_loop,
                                uint32_t loop_start,uint32_t loop_end,
                                uint64_t step,unsigned linear);
/* Play a nonempty initial segment once, then repeat an independently bounded
 * forward-loop range in the same PCM. The two ranges may overlap or be disjoint.
 * Interpolation crosses from the initial end to repeat_start. Other validation,
 * lifetime, alias and no-mutation-on-error rules match pt_voice_init. */
enum pt_pcm_result pt_voice_init_segment(struct pt_voice *,const struct pt_pcm *,
                                        uint32_t start,uint32_t end,
                                        uint32_t repeat_start,uint32_t repeat_end,
                                        uint64_t step,unsigned linear);
/* Reprogram a live forward/one-shot voice's next repeat in the same PCM.
 * The current segment/iteration finishes at its existing end; position, step,
 * interpolation and source remain unchanged. Next boundary enters [start,end).
 * Repeated calls replace the pending range. Inactive/pingpong voices and invalid
 * ranges are refused without mutation. No allocation or sample writes. */
enum pt_pcm_result pt_voice_set_repeat(struct pt_voice *,uint32_t start,uint32_t end);
/* As set_repeat, but borrow another PCM for the next boundary. Both sources
 * must remain immutable/alive until handoff; format, channels and rate must
 * match. Output alias checks protect both. No cross-format conversion. */
enum pt_pcm_result pt_voice_set_repeat_source(struct pt_voice *,const struct pt_pcm *,uint32_t start,uint32_t end);
/* Returns normalized signed 24-bit stereo; mono is duplicated. Nearest uses the
 * current frame; linear uses the next frame with loop-aware endpoint mapping.
 * Outputs and state are unchanged on error. An inactive voice returns silence. */
enum pt_pcm_result pt_voice_frame(struct pt_voice *,int32_t stereo[2]);
/* Offline mix: count<=16, Q16 per-side gains <=65536. Caller chooses pan law,
 * mute/solo/group selection via gains; zero gain still advances the voice.
 * Output must be stereo16/24 with a disjoint buffer. Accumulate in signed64,
 * then round once (ties away from zero) and saturate. No automatic normalization
 * or dither. Clipped counts output values, not source voices. Preflight failure
 * preserves output, voices and clipped. Zero count produces silence. */
enum pt_pcm_result pt_voice_mix(struct pt_voice *,unsigned count,
                               const uint32_t (*gains)[2],struct pt_pcm *output,
                               uint64_t *clipped);
#endif
