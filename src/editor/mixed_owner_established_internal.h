#ifndef PT_MIXED_OWNER_ESTABLISHED_INTERNAL_H
#define PT_MIXED_OWNER_ESTABLISHED_INTERNAL_H
#include "mixed_owner_established.h"
/* Private explicit publisher-container admission. The complete genuine container
 * must contain the exact publisher slot and be disjoint from source, controller
 * and all other input controls. Protects container before claiming any child.
 * Up to2 additional parent spans; original public begin retains its stricter
 * no-publisher-overlap extra-span contract and unchanged three-span capacity.
 * Container may be changed only through its serialized owner API. No copying,
 * arbitrary output aliases or extra external publisher accepted by this seam. */
enum pt_mixed_owner_result pt_mixed_owner_established_begin_bound(struct pt_mixed_established *,
    struct pt_paula_voices *,struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_paula_render_caps *,const struct pt_playback_format *,
    const struct pt_allocator *,const void *,size_t,const struct pt_sampler_storage_span *,unsigned,
    uint32_t,unsigned,struct pt_mixed_owner **);
#endif
