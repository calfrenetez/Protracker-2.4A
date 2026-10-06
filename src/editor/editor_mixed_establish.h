#ifndef PT_EDITOR_MIXED_ESTABLISH_H
#define PT_EDITOR_MIXED_ESTABLISH_H
#include "editor_mixed.h"
#include "sampler_establish.h"
#define PT_EDITOR_ESTABLISH_EXTRA 10U
/* Opt-in, genuine zero-init caller control. Serialized editor thread only;
 * never copy/edit an adopted control or call its underlying job directly.
 * Attach the ordinary mixed binding first. Begin uses THIS editor's sampler
 * allocator and adopts cancellation before its first allocator callback.
 * Complete controller/binding/editor and up to10 caller spans are guarded
 * before allocation; original extra vector and named contexts stay alive until
 * confirmed stop. Opaque allocator contexts must be included or independently
 * disjoint. Source descriptors, PCM capacity and spare storage stay immutable
 * until stop except this job's atomic completed-master publication/selection.
 * All begin results after adoption retain the control until stop: callers inspect
 * binding.preparation_context to distinguish admission refusal from adoption.
 * Step is bounded by the existing establishment contract. READY still holds its
 * source borrow; call mixed_stop BEFORE bridge binding/any checked owner begin.
 * Actual editor change/dispose also cancels it. Partial unpublished copies are
 * discarded; completed masters remain sampler-owned. Reentry vetoes mutation,
 * latches fault and retains adoption until an explicit later stop.
 * No checked owner/transport adoption, UI/native PLAY wiring, backend/cache work,
 * exact schedule changes or device/latency/native placement acceptance here. */
struct pt_editor_mixed_establish {
    struct pt_sampler_establish job;
    struct pt_editor_mixed *binding;
    struct pt_sampler_storage_span parents[PT_ESTABLISH_PARENTS-2];
};
enum pt_establish_result pt_editor_mixed_establish_begin(struct pt_editor_mixed_establish *,
    struct pt_editor_mixed *,const struct pt_sampler_storage_span *,unsigned);
enum pt_establish_result pt_editor_mixed_establish_step(struct pt_editor_mixed_establish *,unsigned);
#endif
