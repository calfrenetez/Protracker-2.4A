#ifndef PT_PLAYBACK_PCM_H
#define PT_PLAYBACK_PCM_H
#include "pcm.h"
#include "sample_cache.h"
/* One selected source channel per hardware voice. No implicit stereo downmix,
 * sample-rate conversion, loop relocation, dither or master modification.
 * Signed 8/16-bit output, explicit 16-bit byte order; optional silent even-byte
 * padding for Paula DMA. Conversion rounds nearest, ties away from zero. */
struct pt_playback_format {unsigned bits,channel,little_endian,word_pad;};
/* Scalar and packed outputs must be disjoint from PCM/format descriptors and
 * the full declared master capacity, including unused storage. Checked span
 * overflow or missing nonzero-capacity storage returns PT_PCM_ALIAS without
 * publication. Guards read metadata only, never capacity padding. Existing
 * shape/value/format/count and pack capacity/NULL refusals precede alias checks.
 * Empty zero-capacity sources still publish size zero. Any valid empty shape
 * packs zero bytes without inspecting an unused byte pointer/storage extent,
 * including an empty NULL source with retained nonzero capacity. Scalar size
 * publication refuses that missing declared storage as PT_PCM_ALIAS. */
enum pt_pcm_result pt_playback_pcm_size(const struct pt_pcm *,const struct pt_playback_format *,size_t *);
enum pt_pcm_result pt_playback_pcm_pack(const struct pt_pcm *,const struct pt_playback_format *,uint8_t *,size_t);
/* Use a dedicated representation pool per backend/memory domain. Identity is
 * stable within that pool; version must change for ANY master/format metadata
 * edit (including undo). Format fields are encoded in the cache key. */
enum pt_cache_result pt_playback_pcm_acquire(struct pt_sample_cache *,const struct pt_pcm *,
    uint32_t identity,uint64_t version,const struct pt_playback_format *,struct pt_cache_lease *);
/* Device-backed pool: allocate/release manage opaque non-NULL resource handles,
 * not CPU-writable PCM. upload must synchronously consume staging and return 1
 * only after completion. No callback reentry; never call from an interrupt.
 * Caller supplies bounded Fast-RAM staging, disjoint from master and resources.
 * HIT requires no staging/upload. Failed LOAD releases unpublished resources;
 * eviction may already have occurred. Output lease changes only on success.
 * Keep the lease pinned until every device voice using the resource has stopped.
 * This is a driver seam, not an AmiGUS register or library implementation. */
enum pt_cache_result pt_playback_pcm_upload(struct pt_sample_cache *,const struct pt_pcm *,
    uint32_t identity,uint64_t version,const struct pt_playback_format *,
    uint8_t *staging,size_t capacity,void *context,
    int (*upload)(void *,void *resource,const uint8_t *,size_t),struct pt_cache_lease *);
/* Bounded staging variant: ordered byte offsets, complete signed frames per
 * write (plus optional final 8-bit pad). Minimum staging is one output frame.
 * Driver write consumes each chunk synchronously; nonzero means completed.
 * No publication until ALL chunks finish. Failure frees the partial resource.
 * The master must remain immutable throughout; no callback reentry. */
enum pt_cache_result pt_playback_pcm_upload_chunks(struct pt_sample_cache *,const struct pt_pcm *,
    uint32_t identity,uint64_t version,const struct pt_playback_format *,
    uint8_t *staging,size_t capacity,void *context,
    int (*write)(void *,void *resource,size_t offset,const uint8_t *,size_t),struct pt_cache_lease *);
#define PT_PLAYBACK_UPLOAD_CHUNK 256
/* Incremental upload. Zero-initialize job; no copying/reinitializing while active.
 * Begin validates complete PCM synchronously, copies descriptors and reserves a
 * cache lease. LOAD returns PENDING with no packing/write/publication; HIT writes
 * *out immediately and leaves job idle. Allocation/eviction remains synchronous.
 * Step packs/writes at most min(capacity,256) bytes, rounded to whole output
 * frames. PENDING leaves *out untouched; LOAD publishes/transfers the lease only
 * after the final write. Failure cancels/releases the unpublished lease; another
 * step then returns INVALID. Cancel is idempotent and never releases a previously
 * transferred result lease. Neither begin nor step changes *out on failure.
 * Source/format/lease-output overlap refuses before cache reservation in begin.
 * Step protects staging and lease output from the retained source descriptor and
 * full capacity, and staging from job/output metadata. Other failures cancel as
 * above. If *out overlaps the job, INVALID leaves the unpublished job unchanged:
 * retry with a disjoint output or explicitly cancel; cancellation would otherwise
 * overwrite the refused caller output.
 * Caller MUST retain immutable master data AND its descriptor, cache and callback
 * contexts through completion/cancel; this low-level job does not pin a master.
 * Descriptor changes/invalidation refuse. Value edits are forbidden, not scanned
 * every step. Staging must be disjoint from master, job and all metadata/outputs.
 * Job fields are private to these functions; never publish/use its partial lease.
 * Serialize operations; callbacks consume synchronously and must not reenter.
 */
struct pt_playback_upload_job {
    struct pt_sample_cache *cache;const struct pt_pcm *source;struct pt_pcm pcm;
    struct pt_playback_format format;struct pt_cache_lease lease;
    size_t bytes,offset;void *context;
    int (*write)(void *,void *,size_t,const uint8_t *,size_t);
};
enum pt_cache_result pt_playback_upload_begin(struct pt_playback_upload_job *,struct pt_sample_cache *,
    const struct pt_pcm *,uint32_t identity,uint64_t version,const struct pt_playback_format *,
    void *,int (*)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *);
enum pt_cache_result pt_playback_upload_step(struct pt_playback_upload_job *,uint8_t *,size_t,struct pt_cache_lease *);
void pt_playback_upload_cancel(struct pt_playback_upload_job *);
void pt_playback_pcm_invalidate(struct pt_sample_cache *,uint32_t identity);
#endif
