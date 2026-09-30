#ifndef PT_PAULA_INTERNAL_H
#define PT_PAULA_INTERNAL_H
#include "paula_dispatch.h"
/* Private exclusive-session entry points. Token must match song_owner exactly;
 * NULL is reserved for public direct calls when no song owns the controller. */
int pt_paula_stop_owned(struct pt_paula_voices *,unsigned,void *);
/* Confirm readers/barrier while retaining cache and exclusive token. */
int pt_paula_drain_owned(struct pt_paula_voices *,void *);
int pt_paula_close_owned(struct pt_paula_voices *,void *);
int pt_paula_dispatch_owned(struct pt_paula_voices *,uint64_t,unsigned,
    const struct pt_render_plan *,const struct pt_paula_render_caps *,struct pt_paula_batch *,void *);
int pt_paula_prepare_owned(struct pt_paula_prepared *,struct pt_paula_voices *,uint64_t,unsigned,
    const struct pt_render_plan *,const struct pt_paula_render_caps *,void *);
/* Already-validated immutable session/project, exclusive matching non-NULL
 * owner, all genuine current master pins held by caller through apply/cancel.
 * Begin copies/checks the whole plan without conversion. Each step reserves one
 * cache candidate OR copies <=256 bytes OR advances one action; no callbacks.
 * PENDING blocks apply; LOAD means ready; refusal cancels unstarted leases.
 * Do not call the public validating bridge path per chunk. */
int pt_paula_prepare_begin_owned(struct pt_paula_prepared *,struct pt_paula_voices *,uint64_t,unsigned,
    const struct pt_render_plan *,const struct pt_paula_render_caps *,struct pt_sample_version *const *,void *);
enum pt_cache_result pt_paula_prepare_step_owned(struct pt_paula_prepared *);
#endif
