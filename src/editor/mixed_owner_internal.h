#ifndef PT_MIXED_OWNER_INTERNAL_H
#define PT_MIXED_OWNER_INTERNAL_H
#include "mixed_owner.h"
/* Private future sequence-dispatch preparation. Only resolved immutable plans
 * from the owner's audited sequence; never an arbitrary provider/trust bypass.
 * Begin copies/partitions and checks both backend rules plus genuine union pins
 * before reserving any derived representation. Other staging refuses until
 * cancel. Step performs ONE Paula cache operation (<=256 bytes), OR one AmiGUS
 * reserve/hit/upload (<=256 bytes), OR one command conversion. Duplicate triggers
 * retain independent leases. Ready means BOTH caches and commands ready, no
 * voice output. Captured reader states must remain unchanged through staging.
 * Cancellation releases only unstarted leases, preserves active readers/masters;
 * completed cache warming may remain. Close/failure cancels partial jobs first.
 * Commit checks BOTH routes before output then emits in original global order.
 * No elapsed time, scheduler or native output.
 * AmiGUS bus upload and ownership predicates are preparation callbacks; adapter
 * must uphold synchronous ownership. API/context/arrays immutable/non-reentrant. */
enum pt_mixed_owner_result pt_mixed_stage_begin(struct pt_mixed_owner *,const struct pt_render_plan *);
enum pt_mixed_owner_result pt_mixed_stage_step(struct pt_mixed_owner *);
/* Ready batch only. No allocation/upload/conversion. Refusal cancels unstarted
 * candidates without output. Partial failure poisons both engines, tries each
 * retained reader stop once and retains master pins/tokens until confirmed close.
 * Never retry partial output; caller must close. */
enum pt_mixed_owner_result pt_mixed_stage_commit(struct pt_mixed_owner *);
void pt_mixed_stage_cancel(struct pt_mixed_owner *);
#endif
