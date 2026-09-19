#ifndef PT_SLICES_H
#define PT_SLICES_H
#include "pcm.h"
enum pt_slice_result {PT_SLICE_OK,PT_SLICE_INVALID,PT_SLICE_CAPACITY,PT_SLICE_ALIAS};
struct pt_slice_options {
    uint32_t minimum_spacing,zero_radius;
    uint16_t threshold_per_mille; /* 1..1000 of source peak; lower is more sensitive. */
    uint8_t envelope_shift; /* 1..12, integer exponential envelope. */
};
int pt_slices_valid(uint32_t,const uint32_t *,size_t);
enum pt_slice_result pt_slice_insert(uint32_t,uint32_t *,size_t *,size_t,uint32_t);
enum pt_slice_result pt_slice_remove(uint32_t,uint32_t *,size_t *,size_t);
/* Offline proposals, source untouched. Zero first marker for nonempty input.
 * Two passes preserve output on capacity failure. Output is at most 4096 markers.
 * Refinement chooses the nearest crossing shared by all channels, else keeps
 * the detected attack. Marker spacing is respected after refinement.
 */
enum pt_slice_result pt_auto_slice(const struct pt_pcm *,const struct pt_slice_options *,uint32_t *,size_t,size_t *);
/* Destructive offline crossfade; head and tail spans cannot overlap. Resulting
 * forward-loop start skips the head frames incorporated into the blended tail.
 * Caller supplies undo/ownership and commits loop metadata only after success.
 */
enum pt_pcm_result pt_pcm_crossfade_loop(struct pt_pcm *,uint32_t,uint32_t,uint32_t,uint32_t *);
#endif
