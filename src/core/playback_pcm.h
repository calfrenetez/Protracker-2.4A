#ifndef PT_PLAYBACK_PCM_H
#define PT_PLAYBACK_PCM_H
#include "pcm.h"
#include "sample_cache.h"
/* One selected source channel per hardware voice. No implicit stereo downmix,
 * sample-rate conversion, loop relocation, dither or master modification.
 * Signed 8/16-bit output, explicit 16-bit byte order; optional silent even-byte
 * padding for Paula DMA. Conversion rounds nearest, ties away from zero. */
struct pt_playback_format {unsigned bits,channel,little_endian,word_pad;};
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
void pt_playback_pcm_invalidate(struct pt_sample_cache *,uint32_t identity);
#endif
