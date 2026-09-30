#ifndef PT_PAULA_RENDER_VOICE_H
#define PT_PAULA_RENDER_VOICE_H
#include "voice.h"
/* Caller-declared actual target clock and safe period range. No implicit PAL,
 * NTSC, DMA mode or device capability. A supported period rounds the requested
 * Q32 source frequency to the nearest register value (ties upward); it never
 * clamps an unsupported frequency. This is pitch quantization, not resampling. */
struct pt_paula_render_caps {uint32_t clock_hz;uint16_t minimum_period,maximum_period;};
struct pt_paula_render_plan {uint32_t offset,length;uint16_t period;uint8_t volume;};
int pt_paula_render_caps_valid(const struct pt_paula_render_caps *);
/* Q16 gains must match physical stereo: slots0/3 left,1/2 right. Opposite gain
 * must be zero; selected gain must be exactly representable by volume0..64.
 * No panning/downmix approximation. Silence remains representable on any slot.
 * All outputs unchanged on refusal; no PCM reads, allocation or callbacks. */
int pt_paula_render_control(uint64_t step,unsigned output_rate,const uint32_t gains[2],unsigned slot,
    const struct pt_paula_render_caps *,uint16_t *period,uint8_t *volume);
/* Exact initial mono8/16/24 one-shot geometry. Signed8 cache is required later.
 * Even offset and length2..131070; no padding inference, loops, pending repeats,
 * interpolation, fractional phase or mid-voice restore. Caller owns immutable
 * validated PCM descriptors. No sample values are read or changed. */
int pt_paula_render_voice(const struct pt_voice *,unsigned output_rate,const uint32_t gains[2],unsigned slot,
    const struct pt_paula_render_caps *,struct pt_paula_render_plan *);
#endif
