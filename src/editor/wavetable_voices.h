#ifndef PT_WAVETABLE_VOICES_H
#define PT_WAVETABLE_VOICES_H
#include "sampler_wavetable.h"
#include "../core/amigus_voice_plan.h"
#define PT_WAVETABLE_VOICES 16
struct pt_amigus_restore_plan;
/* Injected driver contract, not hardware dispatch. start returns 1 only when
 * started; every other result is uncertain and requires a confirmed stop.
 * stop returns 1 only when this voice can no longer read its sample RAM, 0 for
 * pending, negative for failure. Merely issuing a stop is NOT confirmation.
 * Callbacks must not reenter or edit the sampler. One serial control thread.
 * Voice IDs must be exclusively owned and initially stopped. */
struct pt_wavetable_voice_api {
    void *context;
    int (*start)(void *,unsigned,const struct pt_amigus_voice_plan *);
    int (*stop)(void *,unsigned);
    /* Optional synchronous phase-preserving rate/gain update for sequence
     * dispatch. Return1 confirmed; all other results require confirmed stop. */
    int (*control)(void *,unsigned,uint32_t rate,uint16_t left,uint16_t right);
    /* Optional EXACT cursor restore, distinct from start. Must synchronously
     * consume/copy both bounds and Q32 cursor; never round/drop phase. Return1
     * confirmed, anything else uncertain until stop. Same context/lifetime. */
    int (*restore)(void *,unsigned,const struct pt_amigus_restore_plan *);
};
struct pt_wavetable_voice {
    struct pt_cache_lease lease;
    unsigned held,uncertain;
};
struct pt_wavetable_voices {
    struct pt_sampler_wavetable *bridge;
    struct pt_wavetable_voice_api api;
    struct pt_wavetable_voice voice[PT_WAVETABLE_VOICES];
    unsigned closing;
    const void *song_owner; /* Exclusive sequence session, managed by wavetable_song. */
    int (*quiesce)(void *);
    void *quiesce_context;
    unsigned quiesced;
};
enum pt_voice_result {
    PT_VOICE_REFUSED=-2, PT_VOICE_STOP_FAILED=-1, PT_VOICE_STOP_PENDING=0,
    PT_VOICE_ACTIVE=1, PT_VOICE_UNCERTAIN=2
};
/* Zero-initialize, bind once to a dedicated bridge with no outstanding leases.
 * Do not directly close/unpin/use the bridge while this owner is bound. */
int pt_wavetable_voices_bind(struct pt_wavetable_voices *,struct pt_sampler_wavetable *,
    const struct pt_wavetable_voice_api *);
/* Optional for synchronous injected drivers; REQUIRED when backend callbacks or
 * interrupts can retain voice/cache contexts. Bind before any voice/song starts.
 * After all voice stops, close calls this once per attempt until exactly1 means
 * no callbacks can reference these contexts and the interrupt guard is clear.
 * Pending/error/unknown results retain bridge, cache and reservation. Callback
 * and its context outlive successful close; no reentrancy or blocking waits. */
int pt_wavetable_voices_bind_quiesce(struct pt_wavetable_voices *,int (*)(void *),void *);
/* Validate metadata/rate/range before acquiring; resolve a bounded plan before
 * stopping old voice. Callback must consume/copy the plan synchronously.
 * Acquire before stopping old voice: acquisition failure preserves old playback.
 * A pending/failed stop keeps the old lease and drops the unstarted candidate.
 * No trigger is queued: retry explicitly with the current master after stop.
 * Any invoked start owns its lease until confirmed stop, even on start failure. */
enum pt_voice_result pt_wavetable_voices_trigger(struct pt_wavetable_voices *,unsigned voice,
    unsigned sample,const struct pt_playback_format *,const struct pt_amigus_voice_request *,
    uint8_t *staging,size_t capacity);
/* One stop attempt; returns 1 confirmed/idle, 0 pending, -1 failure/invalid. */
int pt_wavetable_voices_stop(struct pt_wavetable_voices *,unsigned voice);
/* Blocks new triggers, attempts each held voice once, detaches bridge only after
 * every stop and any bound quiescence barrier are confirmed. Retry pending or
 * failed stops/barriers; never forcibly free.
 * No wait loop, allocation or real device I/O beyond injected callbacks. */
int pt_wavetable_voices_close(struct pt_wavetable_voices *);
#endif
