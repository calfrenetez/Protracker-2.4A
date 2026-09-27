#ifndef PT_AMIGUS_SAMPLE_RAM_H
#define PT_AMIGUS_SAMPLE_RAM_H
#include "sample_cache.h"
#define PT_AMIGUS_RAM_WRITE_MAX 256
/* Caller-owned address arena, not a master store or CPU PCM buffer. Region and
 * capacity must be verified by the device owner; no assumed card size. Keep the
 * arena at a stable address for the lifetime of its cache. Single control owner,
 * no reentry/interrupt use. Retain an exclusive WAVETABLE reservation for the
 * ENTIRE cache lifetime. On ownership loss, stop all voices and clear all cached
 * handles before reopening; reacquiring ownership cannot validate old RAM.
 * write32 is synchronous: even failure must leave no deferred host-memory or
 * DMA reference. Native MMIO, bus atomicity and reservation binding are absent.
 */
struct pt_amigus_ram_block {
    uint32_t address,bytes,reserved,written,word;
    unsigned fill,failed;
};
struct pt_amigus_sample_ram {
    struct pt_amigus_ram_block block[PT_CACHE_SLOTS];
    uint32_t base,capacity;
    void *context;
    int (*owned)(void *);
    int (*write32)(void *,unsigned,uint32_t);
};
/* Four-byte aligned region; refuses wrap, missing callbacks or empty region.
 * Initialization performs no I/O. Never reinitialize a live arena. */
int pt_amigus_sample_ram_init(struct pt_amigus_sample_ram *,uint32_t base,
    uint32_t capacity,void *,int (*owned)(void *),int (*write32)(void *,unsigned,uint32_t));
/* pt_sample_cache allocator callbacks. Padding counts against the physical
 * arena capacity, independently of the cache's logical byte budget. Never move
 * allocations to compact memory: a pinned voice may still reference them.
 * Release only after all device voices/transfers have stopped using the block;
 * the cache lease enforces this when used correctly. */
void *pt_amigus_sample_ram_allocate(void *,size_t);
void pt_amigus_sample_ram_release(void *,void *,size_t);
/* pt_playback_pcm_upload_chunks callback. Ordered chunks of 1..256 bytes;
 * signed mono8 or explicitly endian-packed mono16 bytes from that converter.
 * A new LOAD starts at offset zero, permitting an unpinned cache allocation to
 * be refilled. Final padding is silent and belongs only to the reserved block.
 * Failures poison this upload until a fresh offset-zero LOAD. No publication
 * before the complete logical byte count. */
int pt_amigus_sample_ram_write(void *,void *,size_t,const uint8_t *,size_t);
/* Returns byte address and logical length only for a completely uploaded block.
 * Does not grant a voice lease: caller must keep the cache lease pinned. */
int pt_amigus_sample_ram_location(const struct pt_amigus_sample_ram *,const void *,uint32_t *,uint32_t *);
#endif
