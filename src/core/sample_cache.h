#ifndef PT_SAMPLE_CACHE_H
#define PT_SAMPLE_CACHE_H
#include <stddef.h>
#include <stdint.h>
#define PT_CACHE_SLOTS 32
struct pt_cache_lease {unsigned slot;uint64_t serial;};
struct pt_cache_entry {
    void *data;size_t bytes;uint64_t key,version,serial,touched;
    unsigned pins,valid;
};
struct pt_sample_cache {
    struct pt_cache_entry entry[PT_CACHE_SLOTS];
    void *context;void *(*allocate)(void *,size_t);void (*release)(void *,void *,size_t);
    size_t bytes,budget;uint64_t clock;
};
enum pt_cache_result {PT_CACHE_HIT,PT_CACHE_LOAD,PT_CACHE_CAPACITY,PT_CACHE_BUSY,PT_CACHE_INVALID,PT_CACHE_TRANSFER};
/* Single-owner control API; no allocation, eviction or release in an interrupt.
 * key identifies sample plus representation settings. version identifies master
 * revision. LOAD storage is unpublished and pinned: fill/upload, then publish.
 * Never publish before conversion/upload succeeds. Release without publishing
 * discards staging. Backend mutation requires a new version or invalidation.
 * Cache resources are owned derived data, never the master. */
void pt_cache_init(struct pt_sample_cache *,void *,void *(*)(void *,size_t),void (*)(void *,void *,size_t),size_t);
enum pt_cache_result pt_cache_take(struct pt_sample_cache *,uint64_t,uint64_t,size_t,struct pt_cache_lease *);
void *pt_cache_data(struct pt_sample_cache *,struct pt_cache_lease);
int pt_cache_publish(struct pt_sample_cache *,struct pt_cache_lease);
int pt_cache_unpin(struct pt_sample_cache *,struct pt_cache_lease);
void pt_cache_invalidate(struct pt_sample_cache *,uint64_t);
/* Evict oldest unpinned resources until at least wanted bytes are freed.
 * Return actual freed bytes. Pinned resources are never touched. */
size_t pt_cache_trim(struct pt_sample_cache *,size_t wanted);
/* Invalidates all; returns false while leases still pin retired resources.
 * Keep the allocator/context alive until the final lease is released. */
int pt_cache_clear(struct pt_sample_cache *);
#endif
