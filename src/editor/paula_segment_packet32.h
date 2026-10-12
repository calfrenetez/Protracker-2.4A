#ifndef PT_PAULA_SEGMENT_PACKET32_H
#define PT_PAULA_SEGMENT_PACKET32_H
#include "paula_segment_future.h"
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The geometry packet32 converter is an inhibited HOST address model"
#endif
#define PT_PAULA_PACKET32_VERSION 1U
#define PT_PAULA_PACKET32_HEADER_WORDS 32U
#define PT_PAULA_PACKET32_SOURCE_WORDS 21U
#define PT_PAULA_PACKET32_WORDS 74U
#define PT_PAULA_PACKET32_BYTES 296U
/* Explicit big-endian bytes; the union only supplies uint32_t object alignment.
 * No C struct layout, HOST pointer width, native address or device byte order is
 * inferred. All 64-bit values occupy high/low words without narrowing. */
union pt_paula_packet32_record {
    uint32_t alignment;
    uint8_t bytes[PT_PAULA_PACKET32_BYTES];
};
/* Pointer anchors are compared only, never followed. The caller supplies typed
 * equality labels and cache_base32 as an explicit HOST address-model number;
 * none is obtained by converting/truncating a HOST pointer. Matching a label
 * with the prepared source is NOT authentication, currentness or a real Chip
 * address. Corresponding labels in two rows preserve equality/inequality.
 * Only cache_base32 is offset numerically; complete model extents must fit and
 * be even, and different retained caches have disjoint model extents. */
struct pt_paula_packet32_source_map {
    const struct pt_pcm *descriptor;
    const int32_t *master_data;
    const struct pt_sample_version *version;
    const void *sampler_owner,*project_identity,*cache_owner;
    const uint8_t *cache_data;
    uint32_t descriptor_tag,master_tag,version_tag,sampler_tag,project_tag,
        cache_owner_tag,cache_base32;
};
struct pt_paula_packet32_mapping {
    unsigned source_count;
    struct pt_paula_packet32_source_map source[2];
};
enum pt_paula_packet32_result {
    PT_PAULA_PACKET32_OK,PT_PAULA_PACKET32_INVALID,PT_PAULA_PACKET32_ALIAS,
    PT_PAULA_PACKET32_UNSUPPORTED,PT_PAULA_PACKET32_MAPPING
};
/* Pure bounded TASK-side metadata conversion. Full live input objects and
 * independent immutable prepared-packet/source/cache lifetimes are caller
 * obligations. Fixed numeric extent/alignment/disjoint checks precede typed
 * input reads. No former descriptor, PCM value, cache byte or capacity padding
 * is read. Full declared int32-element capacity, complete even-padded cache,
 * descriptor and declared guard spans protect output before the only write.
 * Refusal preserves output. Unknown opaque control/context extents additionally
 * remain caller-disjoint. No callbacks, allocation, timer, queue, currentness,
 * ownership, activation, quiet, fetch boundary, MMIO, DMA or IRQ authority.
 * INITIAL retains its exact phase and ranges. REPROGRAM never emits an initial
 * address/write and preserves the current fetch's Q32 phase; its repeat still
 * becomes effective only at the independently established fetch boundary.
 * STOP retains every selected identity, emits no range/address and grants no
 * cancellation/retirement. Original ticket/[first,last)/frame are copied exactly.
 * Word offsets0..31: magic/version/bytes/count/kind/track/slot/writeflags,
 * period/volume,ticket(2),first(2),last(2),frame(2),phase(2),initialaddr,
 * repeataddr,initial(offset,length,byteoffset,bytelength,words),repeat(same5).
 * Each source row: descriptor/master/version/sampler/project/cache-owner tags,
 * cachebase/cachebytes/capacity-elements/frames/rate/bits/channels/generation/
 * sample,cacheversion(2),lease-slot,lease-serial(2),guardcount. Unused row is zero.
 */
enum pt_paula_packet32_result pt_paula_segment_packet32_build(
    const struct pt_paula_future_packet *,const struct pt_paula_packet32_mapping *,
    union pt_paula_packet32_record *);
#endif
