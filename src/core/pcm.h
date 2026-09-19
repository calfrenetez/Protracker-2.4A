#ifndef PT_PCM_H
#define PT_PCM_H
#include <stddef.h>
#include <stdint.h>

/* Interleaved signed integer samples in their declared 8/16/24-bit range.
 * 24-bit values retain all eight low bits. No floating-point dependency. */
struct pt_pcm {
    int32_t *data;
    size_t capacity; /* Number of int32_t elements, not bytes or frames. */
    uint32_t frames, rate;
    uint8_t channels, bits;
};
enum pt_pcm_result { PT_PCM_OK, PT_PCM_INVALID, PT_PCM_CAPACITY, PT_PCM_ALIAS };
enum pt_pcm_edit { PT_PCM_REVERSE, PT_PCM_NORMALIZE, PT_PCM_GAIN,
                   PT_PCM_FADE_IN, PT_PCM_FADE_OUT, PT_PCM_REMOVE_DC };
enum pt_pcm_result pt_pcm_validate(const struct pt_pcm *);
/* Half-open frame selection. Gain is per mille, 0..8000, with saturation.
 * Silence normalizes to silence; single-frame fades produce zero. */
enum pt_pcm_result pt_pcm_edit(struct pt_pcm *, enum pt_pcm_edit, uint32_t, uint32_t, unsigned int);
/* Explicit bit conversion; reduction rounds nearest, ties away from zero. */
enum pt_pcm_result pt_pcm_convert(const struct pt_pcm *, struct pt_pcm *);
/* Offline linear resampling, ceil(frames * new_rate / old_rate).
 * Requires disjoint source/destination buffers; no antialias filter yet. */
enum pt_pcm_result pt_pcm_resampled_frames(const struct pt_pcm *, uint32_t, uint32_t *);
enum pt_pcm_result pt_pcm_resample(const struct pt_pcm *, struct pt_pcm *);
#endif
