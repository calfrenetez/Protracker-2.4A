#ifndef PT_MIXED_OWNER_ESTABLISHED_H
#define PT_MIXED_OWNER_ESTABLISHED_H
#include "mixed_owner.h"
#include "sampler_prepare_project.h"
/* Private opt-in established-master path. Genuine caller control, zero-init,
 * never copy/edit while active. Begin protects complete source, both engines,
 * bridges, backend/reservation, input controls, publisher and this entire control
 * before claiming an ordinary child. Up to three additional opaque-context spans
 * must be named by caller. Input controls and all named spans stay alive/stable
 * through close AND zero-child finish. No source promotion is permitted during
 * borrowing. All required masters must already be genuine sampler-owned current
 * versions; missing required versions refuse after the actual audit.
 * Begin uses metadata only: one fixed owner allocation, no full validation, pins,
 * cache, live backend callback or device output. Prepare uses genuine cancellable
 * INITIAL validation then the full mixed audit; only complete successful masks
 * can retain one existing master per call. The SAME checked sequence transfers.
 * Work1..4096 bounds startup items; the full audit keeps existing finite steps.
 * No byte-budget/placement/latency/stack/deadline guarantee. Base allocator supplies
 * alignment, byte budget/reserve; eight child slots do not bound aggregate bytes.
 * Serialize, forbid callback edits/destruction. Observable allocation/preparation/
 * close reentry faults the owner without freeing a live call; it does not make
 * arbitrary callbacks safe. In-place PCM/table writes forbidden, not detected.
 * Existing ownership predicate stays separate from source validation; native
 * timing, device capacity/order/stop/audio/listening remain unqualified.
 * Cancellation uses ordinary pt_mixed_owner_close, retaining all source pins and
 * both tokens until BOTH drains confirm. Then finish this zero-child control.
 * Existing legacy begin, promotions and editor PLAY are unchanged.
 */
struct pt_mixed_established {
    struct pt_sampler_prepare_project storage;
    struct pt_mixed_preflight_setup *startup;
    struct pt_mixed_owner **publisher;
    uint32_t revision;
    unsigned work,started,starting,faulted;
};
enum pt_mixed_owner_result pt_mixed_owner_established_begin(struct pt_mixed_established *,
    struct pt_paula_voices *,struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_paula_render_caps *,const struct pt_playback_format *,
    const struct pt_allocator *,const struct pt_sampler_storage_span *,unsigned,
    uint32_t revision,unsigned work,struct pt_mixed_owner **);
int pt_mixed_owner_established_finish(struct pt_mixed_established *);
#endif
