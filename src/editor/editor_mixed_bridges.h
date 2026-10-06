#ifndef PT_EDITOR_MIXED_BRIDGES_H
#define PT_EDITOR_MIXED_BRIDGES_H
#include "editor_mixed.h"
#include "sampler_internal.h"
#define PT_EDITOR_BRIDGE_CONTEXTS 8U
struct pt_editor_mixed_bridges {
    struct pt_sampler_paula paula_cache;
    struct pt_sampler_wavetable amigus_cache;
    struct pt_paula_voices paula;
    struct pt_wavetable_voices amigus;
};
struct pt_editor_mixed_bridge_inputs {
    struct pt_amigus_wavetable_cache *backend;
    void *chip_context;
    void *(*chip_allocate)(void *,size_t);
    void (*chip_release)(void *,void *,size_t);
    size_t chip_budget;
    struct pt_paula_voice_api paula;
    struct pt_wavetable_voice_api amigus;
    int (*paula_quiesce)(void *);void *paula_quiesce_context;
    int (*amigus_quiesce)(void *);void *amigus_quiesce_context;
};
/* Optional metadata-only admission for THIS attached editor's idle engines.
 * Genuine stable, completely zeroed output; no copying/reinitialization while
 * bound. Stop pre-borrow establishment BEFORE this call. Caller owns an attached,
 * dedicated empty WAVETABLE backend and exclusively idle device voice slots.
 * Protects complete output against sampler/project/backend/reservation/editor/
 * binding/input-vector/named context extents BEFORE any initialization. Up to8
 * named context spans; unlisted opaque contexts must independently be disjoint.
 * Even unlisted context start pointers inside output are refused. Supported ABI
 * layout must let the checked owner's four control spans cover this whole object.
 * All descriptors are genuine readable controls; no pointer-residency proof.
 * Refusal preserves output and every input. Successful bind invokes no callback,
 * allocator, sync, PCM/event/order/marker scan, master pin/promotion or bus I/O.
 * This is NOT a semantic/backend ready certificate: use ONLY the checked editor
 * owner and its complete cancellable INITIAL validation/two-route audit before
 * any playback. Ordinary public binding/validation paths remain unchanged.
 * Stop/close both engines through their owner, confirm quiescence and detach the
 * backend before output/context disposal. Input argument/vector may expire after
 * success; output, editor/source, backend/reservation and actual callback/context
 * storage survive until confirmed closure. Chip callbacks must supply Chip RAM
 * on Amiga; live ownership/completion and exact timing need separate proof.
 * No native PLAY/UI wiring, timer/schedule change or hardware acceptance here.
 */
int pt_editor_mixed_bridges_bind(struct pt_editor_mixed_bridges *,struct pt_editor_mixed *,
    const struct pt_editor_mixed_bridge_inputs *,const struct pt_sampler_storage_span *,unsigned);
#endif
