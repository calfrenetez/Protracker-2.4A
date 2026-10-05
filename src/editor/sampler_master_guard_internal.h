#ifndef PT_SAMPLER_MASTER_GUARD_INTERNAL_H
#define PT_SAMPLER_MASTER_GUARD_INTERNAL_H
#include "sampler_internal.h"
/* Private already-validated pin-job seam. Guard returns fresh ordinary storage
 * from EXACTLY the supplied sampler allocator, or NULL. It must classify whole
 * source/control/parent/current-version extents BEFORE initialization, dispose
 * only proved-fresh stale returns through that same base, and serialize callbacks.
 * Sampler budget still charges the complete version. Original sampler allocator
 * and future release binding never change. No validation certificate, replacement
 * allocator lifetime or duplicate master ledger. Caller owns the genuine source
 * validation/job lifetime; existing step/cancel and atomic publication apply. */
typedef void *(*pt_sampler_master_guard_allocate)(void *,const struct pt_allocator *,size_t);
enum pt_edit_result pt_sampler_pin_job_begin_guarded(struct pt_sampler_pin_job *,
    struct pt_sampler *,struct pt_project *,unsigned,unsigned,
    pt_sampler_master_guard_allocate,void *);
#endif
