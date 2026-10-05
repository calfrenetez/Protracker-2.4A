#ifndef PT_SAMPLER_PREPARE_PROJECT_H
#define PT_SAMPLER_PREPARE_PROJECT_H
#include "sampler_prepare_memory.h"
/* Private opt-in layer over the genuine sampler child allocator. Zero-initialize
 * this control; use memory.allocator only after successful begin. Never copy or
 * edit an active control. Copies base/parent descriptors; they may then expire.
 * Protects whole control, sampler storage and the complete borrowed project,
 * including unpromoted/unused PCM capacities and extension payloads, BEFORE
 * sampler bookkeeping can claim a returned child. Existing guard owns each
 * exact child once; no duplicate ownership registry or new promotion/retain.
 * Project/header/tables/storage and genuine sampler versions stay alive until
 * zero-child finish. In-place descriptor/payload edits are forbidden and are
 * not detected; cancel before editing. A valid channels.selected cursor change
 * alone is permitted. Header replacement faults allocation. Both captured and
 * currently named live project spans are classified before stale cleanup.
 * Known aliases or unclassifiable spans are refused without initialization or
 * release; malformed callback source metadata can retain an ambiguous result.
 * Safe fresh returns after observable changes are released once by the original
 * guard. Exact previous children remain releasable without traversing sources.
 * No destructive callbacks, hardware calls, semantic validation or certificates.
 * Parent descriptors count <=15: one of the original sixteen slots protects
 * this complete control. Base supplies alignment, byte budgeting and reserve;
 * eight concurrent children is not an aggregate byte budget or Fast placement.
 * Existing sampler-only helper and every PLAY/mixed-owner path are unchanged.
 */
struct pt_sampler_prepare_project {
    struct pt_sampler_prepare_memory memory;
    struct pt_allocator base;
    const struct pt_project *project;
    struct pt_project snapshot;
};
/* No callbacks. Refusal preserves the entire control. */
int pt_sampler_prepare_project_begin(struct pt_sampler_prepare_project *,
    const struct pt_sampler *,const struct pt_project *,const struct pt_allocator *,
    const struct pt_sampler_storage_span *,unsigned);
/* Never frees children; ends borrowing only once all exact children retired. */
int pt_sampler_prepare_project_finish(struct pt_sampler_prepare_project *);
#endif
