#ifndef PT_SAMPLER_PAULA_H
#define PT_SAMPLER_PAULA_H
#include "sampler.h"
#include "../core/playback_pcm.h"
/* Dedicated optional Paula representation pool. Zero-init once; callbacks must
 * allocate/free CPU-writable Chip RAM on Amiga (host tests may inject storage).
 * No allocation at bind. Only explicit acquisitions create signed8, even-byte
 * padded copies. Any of 16 tracks may acquire if routed to Paula; at most four
 * routes are accepted by project validation. Stereo requires explicit selection
 * of one source channel, never implicit downmix. No rate/loop conversion.
 * Serialized control-thread API, no interrupts or callback reentry. Keep all
 * contexts alive until close succeeds. Close before sampler/document release or
 * reinitialization. Masters must change through sampler APIs;
 * source project/PCM are immutable during each call. Outputs must be disjoint
 * from owner/project/cache storage. No DMA, voice allocation,
 * scheduling, mixed-backend dispatch or hardware ownership is performed here. */
struct pt_sampler_paula {
    struct pt_sampler *sampler;struct pt_project *project;
    struct pt_sample_cache cache;struct pt_sample *table;
    unsigned generation,count,channels,closing;uint64_t version;
    uint8_t routes[PT_CHANNEL_LIMIT];
};
int pt_sampler_paula_bind(struct pt_sampler_paula *,struct pt_sampler *,struct pt_project *,
    void *,void *(*)(void *,size_t),void (*)(void *,void *,size_t),size_t budget);
/* Generation/table/count/route edits retire copies. Active leases retain old
 * bytes; retired copies cannot authorize a new trigger. Mute/solo/selection do
 * not evict representations. No caller may use the embedded cache directly. */
int pt_sampler_paula_sync(struct pt_sampler_paula *);
/* Pin exact master through conversion, release source pin on return. Derived
 * lease remains pinned until caller has confirmed every DMA reader stopped.
 * Failure preserves *out; source promotion may remain as an unchanged master. */
enum pt_cache_result pt_sampler_paula_acquire(struct pt_sampler_paula *,unsigned track,
    unsigned sample,unsigned source_channel,struct pt_cache_lease *out);
/* Current revision/route only; outputs unchanged on refusal. An already-active
 * voice may retain a previously obtained pointer until it stops and unpins. */
int pt_sampler_paula_location(struct pt_sampler_paula *,unsigned track,struct pt_cache_lease,
    const uint8_t **data,size_t *bytes);
int pt_sampler_paula_unpin(struct pt_sampler_paula *,struct pt_cache_lease);
/* Busy close blocks further acquisitions; stop readers, unpin, then retry.
 * Never releases active data to force a successful close. */
int pt_sampler_paula_close(struct pt_sampler_paula *);
#endif
