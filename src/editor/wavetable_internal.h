#ifndef PT_WAVETABLE_INTERNAL_H
#define PT_WAVETABLE_INTERNAL_H
#include "wavetable_dispatch.h"
/* Private prepared-song callbacks, never an opt-in for arbitrary providers.
 * The song has validated immutable project/PCM, completed capability analysis,
 * and holds every required exact master pin. Callbacks check its captured
 * revision, identities and live device ownership on each operation; acquire
 * retains the exact validated master through synchronous upload. No callbacks may reenter
 * or mutate the owner. Dispatch stores no callback/trust state. Public dispatch
 * always uses the fully validating bridge. Prepared upload reuses the held master validation.
 * All cache leases, uncertain-stop ownership and rollback rules are unchanged. */
struct pt_wavetable_prepared {
    void *context;
    int (*current)(void *);
    enum pt_cache_result (*acquire)(void *,unsigned,const struct pt_playback_format *,uint8_t *,size_t,struct pt_cache_lease *);
    int (*location)(void *,struct pt_cache_lease,uint32_t *,uint32_t *);
};
int pt_wavetable_dispatch_prepared(struct pt_wavetable_voices *,uint64_t,unsigned,
    const struct pt_render_plan *,const struct pt_playback_format *,uint8_t *,size_t,const struct pt_wavetable_prepared *);
int pt_wavetable_restore_prepared(struct pt_wavetable_voices *,uint64_t,unsigned,
    const struct pt_render_snapshot *,const struct pt_playback_format *,uint8_t *,size_t,const struct pt_wavetable_prepared *);
#endif
