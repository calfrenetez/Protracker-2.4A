#ifndef PT_SAMPLER_PREPARE_MEMORY_H
#define PT_SAMPLER_PREPARE_MEMORY_H
#include "sampler_internal.h"
#define PT_SAMPLER_PREPARE_PARENTS 16U
#define PT_SAMPLER_PREPARE_BLOCKS 8U
/* Private ordinary-memory preparation allocator, not Chip/device memory.
 * Zero-initialize this genuine control; never copy or edit an active control.
 * Copies allocator/parent descriptors at begin (those arguments may then expire).
 * Sampler and current version storage stay alive with stable identity/metadata
 * and immutable PCM/markers through finish. Controlled reference counts may
 * change through serialized retain/release APIs while storage remains alive.
 * Parent span identities/extents stay stable; controlled parent contents may
 * change through their owner. All current masters,
 * including unused slots, are protected. No version is promoted/retained here.
 * A parent span may contain this control or legitimate publisher slots: parent
 * spans protect child initialization, not parent writes. The caller separately
 * guards external outputs and non-sampler project/document storage.
 * Serialized; no callback mutation/destruction. Opaque context extents cannot be
 * guessed. Known source/parent/control/live-child aliases are refused WITHOUT
 * release; overlapping storage never became a fresh owned block. A fresh return
 * after observable sampler change or allocator/finish reentry is released once and poisons this
 * lifetime. NULL capacity refusal alone does not poison. Faults block further
 * allocation; exact prior children remain releasable without source traversal.
 * Base allocator must provide fresh ordinary C alignment and its own byte
 * budget/reserve policy; eight child slots are not an aggregate byte budget.
 * At most eight concurrent children. No payload scans, hardware ownership,
 * semantic validation, placement, timing or quiet/retirement qualification.
 * Not wired to mixed_owner, editor PLAY or any existing allocator.
 */
struct pt_sampler_prepare_memory {
    struct pt_allocator base,allocator;
    const struct pt_sampler *sampler;
    struct pt_sampler snapshot;
    struct pt_sampler_storage_span parents[PT_SAMPLER_PREPARE_PARENTS];
    struct {void *data;size_t bytes;} block[PT_SAMPLER_PREPARE_BLOCKS];
    unsigned parent_count,active,busy,faulted;
};
/* Refusal leaves control and allocator output unchanged; no callbacks. */
int pt_sampler_prepare_memory_begin(struct pt_sampler_prepare_memory *,
    const struct pt_sampler *,const struct pt_allocator *,
    const struct pt_sampler_storage_span *,unsigned);
/* Caller uses memory.allocator only after successful begin. Finish never frees
 * children; refuses while busy or any remain. Zero-owner finish ends borrowing,
 * preserving the fault observation until an explicit new zero-owner begin. */
int pt_sampler_prepare_memory_finish(struct pt_sampler_prepare_memory *);
#endif
