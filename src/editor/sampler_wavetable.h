#ifndef PT_SAMPLER_WAVETABLE_H
#define PT_SAMPLER_WAVETABLE_H
#include "sampler.h"
#include "../core/amigus_wavetable_cache.h"
/* Serial control-thread bridge for one sampler/project and a dedicated empty
 * attached wavetable cache. Zero-initialize; keep all contexts alive until close.
 * Do not mutate masters outside sampler APIs, reinitialize the sampler, replace
 * the document or use the backend cache directly while bound. Use close/rebind
 * for another document; ordinary sampler table growth/undo is supported.
 * Backend callbacks are synchronous and must not reenter the sampler/editor. */
struct pt_sampler_wavetable {
    struct pt_sampler *sampler;
    struct pt_project *project;
    struct pt_amigus_wavetable_cache *backend;
    struct pt_sample *table;
    unsigned generation,count;
    uint64_t version;
};
int pt_sampler_wavetable_bind(struct pt_sampler_wavetable *,struct pt_sampler *,
    struct pt_project *,struct pt_amigus_wavetable_cache *);
/* Retire all representations when sampler generation or table identity changes.
 * Active device leases retain their old bytes. Called automatically before every
 * acquire/location; caller may also call after edits for immediate retirement.
 * Conservative global invalidation includes metadata-only edits and undo/redo. */
int pt_sampler_wavetable_sync(struct pt_sampler_wavetable *);
/* Pins an immutable master for the entire synchronous upload, including failure.
 * Releases the source pin on return; device cache leases remain independently
 * pinned until explicitly unpinned after confirmed voice/transfer completion.
 * Output lease changes only on success. Caller supplies bounded Fast staging. */
enum pt_cache_result pt_sampler_wavetable_acquire(struct pt_sampler_wavetable *,unsigned slot,
    const struct pt_playback_format *,uint8_t *,size_t,struct pt_cache_lease *);
/* Current generation only: a held retired lease cannot authorize a new trigger.
 * A voice already playing retired data keeps its previously obtained address. */
int pt_sampler_wavetable_location(struct pt_sampler_wavetable *,struct pt_cache_lease,
    uint32_t *address,uint32_t *bytes);
int pt_sampler_wavetable_unpin(struct pt_sampler_wavetable *,struct pt_cache_lease);
/* Detaches backend (ending reservation access) only after all device pins end.
 * Busy close blocks new acquisitions; stop voices, unpin and retry close before
 * releasing/replacing sampler/project storage. Reservation itself stays open. */
int pt_sampler_wavetable_close(struct pt_sampler_wavetable *);
#endif
