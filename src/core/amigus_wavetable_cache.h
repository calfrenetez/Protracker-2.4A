#ifndef PT_AMIGUS_WAVETABLE_CACHE_H
#define PT_AMIGUS_WAVETABLE_CACHE_H
#include "amigus_reservation.h"
#include "amigus_sample_ram.h"
#include "playback_pcm.h"
/* Stable, zero-initialized caller-owned Fast-RAM metadata. Single owner thread,
 * no copying, reentry or interrupt calls. The library/card reservation is borrowed.
 * This adapter holds its access lease until every cached resource is released. */
struct pt_amigus_wavetable_cache {
    struct pt_amigus_reservation *reservation;
    struct pt_amigus_sample_ram arena;
    struct pt_sample_cache cache;
    void *context;
    int (*owned)(void *);
    int (*write32)(void *,unsigned,uint32_t);
    unsigned closing,faulted;
};
/* Requires an open WAVETABLE reservation with no existing access lease.
 * Region/capacity and synchronous bus semantics require independent verification.
 * Attach performs no bus I/O; failures leave reservation access unchanged. */
int pt_amigus_wavetable_cache_attach(struct pt_amigus_wavetable_cache *,
    struct pt_amigus_reservation *,uint32_t base,uint32_t capacity,size_t budget,
    void *,int (*owned)(void *),int (*write32)(void *,unsigned,uint32_t));
enum pt_cache_result pt_amigus_wavetable_cache_acquire(struct pt_amigus_wavetable_cache *,
    const struct pt_pcm *,uint32_t identity,uint64_t version,const struct pt_playback_format *,
    uint8_t *staging,size_t capacity,struct pt_cache_lease *);
int pt_amigus_wavetable_cache_location(struct pt_amigus_wavetable_cache *,struct pt_cache_lease,
    uint32_t *address,uint32_t *bytes);
/* Unpin only after the corresponding voice/transfer has definitively stopped.
 * This software adapter does not dispatch or stop hardware voices. */
int pt_amigus_wavetable_cache_unpin(struct pt_amigus_wavetable_cache *,struct pt_cache_lease);
void pt_amigus_wavetable_cache_invalidate(struct pt_amigus_wavetable_cache *,uint32_t identity);
/* Retires all cached blocks and refuses new acquire/location operations.
 * Returns 0 while pinned: retain owner, library and bus contexts, stop voices,
 * unpin their leases and retry. Success releases access but leaves the borrowed
 * reservation open for its owner to close. Ownership loss latches refusal until
 * detach/reacquire, so a new reservation cannot silently reuse stale card data. */
int pt_amigus_wavetable_cache_detach(struct pt_amigus_wavetable_cache *);
#endif
