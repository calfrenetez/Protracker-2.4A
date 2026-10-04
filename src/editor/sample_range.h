#ifndef PT_SAMPLE_RANGE_H
#define PT_SAMPLE_RANGE_H
#include "../core/project.h"
struct pt_sample_range { uint32_t start,end; }; /* Half-open frame boundaries. */
struct pt_sample_loop { uint32_t start,end,crossfade; uint8_t kind; };
enum pt_sample_range_result { PT_SAMPLE_RANGE_OK, PT_SAMPLE_RANGE_INVALID,
    PT_SAMPLE_RANGE_NO_LOOP, PT_SAMPLE_RANGE_CAPACITY, PT_SAMPLE_RANGE_ALIAS };
/* Metadata only; no allocation, PCM-value read or source mutation. Refusals
 * preserve outputs. Known source headers/full PCM capacity/markers are protected;
 * unrepresentable declared spans fail closed. Borrowed inputs remain alive. */
enum pt_sample_range_result pt_sample_range_validate(const struct pt_sample *,
    const struct pt_sample_range *);
enum pt_sample_range_result pt_sample_range_loop(const struct pt_sample *,
    struct pt_sample_range *);
/* Preserve the viewport span and clamp its centre around boundary, including
 * exclusive boundary==frames. Selection is independent and never changed.
 * Exact out==view is allowed; partial overlap is refused. */
enum pt_sample_range_result pt_sample_range_centre(uint32_t frames,
    const struct pt_sample_range *view,uint32_t boundary,struct pt_sample_range *out);
/* Preserve/translate a complete contained loop, otherwise publish NONE/zero.
 * Corrupt loop metadata is a refusal, not an implicit repair. */
enum pt_sample_range_result pt_sample_range_copy_loop(const struct pt_sample *,
    const struct pt_sample_range *,struct pt_sample_loop *);
/* Keep only existing strictly ascending markers in [start,end), translated by
 * start. At most4096 metadata entries; no invented zero marker. Output guard
 * covers actual emitted entries, not spare destination capacity. NULL markers
 * is legal when none are emitted; count remains protected. */
enum pt_sample_range_result pt_sample_range_copy_markers(const struct pt_sample *,
    const struct pt_sample_range *,uint32_t *markers,size_t capacity,size_t *count);
#endif
