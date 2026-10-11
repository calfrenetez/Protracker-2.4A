#ifndef PT_MIXED_READERS_CAUSAL_CONTROL16_INTERNAL_H
#define PT_MIXED_READERS_CAUSAL_CONTROL16_INTERNAL_H
#include "mixed_readers_causal_control_internal.h"
/* PRIVATE additive numeric request for final register-width CONTROL levels.
 * No Q16 gain, scaling, clipping or cast from a caller source/key certificate.
 * Existing uint8 request/entry layouts and behavior are unchanged. This entry
 * uses the same installed CONTROL or composite capability, genuine completed
 * first post-state/ACTIVE getters and copied publication; no new bind or mode.
 * Full original request/output/holder/local capacities remain guarded. Exact
 * frame and once-only successor admission, zero new R/resources and ownership
 * on actual lower OK under outer faults remain the original contract.
 * SOURCE prototype only; no host/native/physical acceptance is asserted. */
struct pt_mixed_causal_control16_action {
    unsigned route,slot;
    uint16_t period;uint8_t volume;
    uint32_t rate;uint16_t left,right;
};
struct pt_mixed_causal_control16_request {
    uint64_t predecessor,frame;
    unsigned count;
    struct pt_mixed_readers_control command;
    struct pt_mixed_causal_control16_action action[PT_MIXED_READERS_ACTIONS];
};
enum pt_mixed_readers_result pt_mixed_causal_control16_enqueue(
    struct pt_mixed_causal_owner *,const struct pt_mixed_causal_control16_request *,uint64_t *);
/* Reuse the genuine existing control_publish entry after actual enqueue OK. */
#endif
