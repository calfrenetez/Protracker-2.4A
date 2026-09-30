#ifndef PT_PAULA_VOICES_H
#define PT_PAULA_VOICES_H
#include "sampler_paula.h"
#define PT_PAULA_VOICES 4
/* Explicit signed8 segment geometry. Offset and length are bytes/frames of the
 * selected mono cache, both even. Length is 2..131070 bytes (1..65535 words).
 * A final silent padding byte may be included. No implicit alignment, loop,
 * resampling, zero-length-register encoding or fractional cursor conversion. */
struct pt_paula_voice_request {uint32_t offset,length;uint16_t period;uint8_t volume;};
struct pt_paula_voice_plan {const uint8_t *data;uint16_t words,period;uint8_t volume;};
/* Injected driver seam. Slots0..3 must be exclusively owned and initially
 * stopped. start/control return exactly1 only for confirmed success; any other
 * result is uncertain and retains the lease until confirmed stop. stop returns
 * exactly1 only when DMA and all callbacks can no longer read this slot's data,
 * 0 pending, negative failure. Issuing a stop is not confirmation.
 * start consumes/copies the plan synchronously. control preserves sample phase.
 * The adapter must validate its actual clock/period/device capabilities.
 * One serialized control thread; callbacks must not reenter or edit the project.
 * API/context/allocators/project/sampler outlive successful close. */
struct pt_paula_voice_api {
    void *context;
    int (*start)(void *,unsigned,const struct pt_paula_voice_plan *);
    int (*stop)(void *,unsigned);
    int (*control)(void *,unsigned,uint16_t period,uint8_t volume);
};
struct pt_paula_voice {struct pt_cache_lease lease;unsigned held,uncertain;int8_t track;};
struct pt_paula_voices {
    struct pt_sampler_paula *bridge;struct pt_paula_voice_api api;
    struct pt_paula_voice voice[PT_PAULA_VOICES];int8_t map[PT_CHANNEL_LIMIT];
    void *song_owner; /* Exclusive session; public direct operations refuse while set. */
    unsigned closing,started;int (*quiesce)(void *);void *quiesce_context;
};
enum pt_paula_voice_result {
    PT_PAULA_VOICE_REFUSED=-2,PT_PAULA_VOICE_STOP_FAILED=-1,
    PT_PAULA_VOICE_STOP_PENDING=0,PT_PAULA_VOICE_ACTIVE=1,PT_PAULA_VOICE_UNCERTAIN=2
};
/* Zero-init once. Dedicated bridge with no outstanding leases. While bound,
 * access/close/unpin the bridge only through this owner. No allocation at bind. */
int pt_paula_voices_bind(struct pt_paula_voices *,struct pt_sampler_paula *,const struct pt_paula_voice_api *);
/* Required if interrupts/callbacks retain owner/cache contexts after per-slot
 * stops; optional for synchronous drivers. Bind before any trigger. Exactly1
 * confirms no retained contexts. Close retries without waiting or force release. */
int pt_paula_voices_bind_quiesce(struct pt_paula_voices *,int (*)(void *),void *);
/* Preserve continuing track slots. Stop removed/reassigned voices before
 * committing a new map. On pending/error, old map stays and triggers refuse.
 * Stops already confirmed in the attempt remain stopped. No trigger queued.
 * Returns1 ready,0 pending,-1 stop failure,-2 invalid/closing state. */
int pt_paula_voices_sync(struct pt_paula_voices *);
/* Validate range before allocation. Acquire candidate before stopping old voice:
 * capacity refusal preserves old playback. Pending/failed stop retains old lease
 * and drops unstarted candidate. Invoked start always retains candidate, even
 * if callback fails. Current route/revision is rechecked before invoking start.
 * Explicit source_channel selection; neither mute nor solo changes assignment. */
enum pt_paula_voice_result pt_paula_voices_trigger(struct pt_paula_voices *,unsigned track,
    unsigned sample,unsigned source_channel,const struct pt_paula_voice_request *);
/* Invalid/unsupported control refuses without change. Uncertain control blocks
 * subsequent controls until confirmed stop or replacement. */
enum pt_paula_voice_result pt_paula_voices_control(struct pt_paula_voices *,unsigned track,uint16_t,uint8_t);
/* One attempt on the previously assigned track; works during close/route edits.
 * Returns1 idle/confirmed,0 pending,-1 failed/invalid. */
int pt_paula_voices_stop(struct pt_paula_voices *,unsigned track);
/* Block triggers, stop every held slot once, then confirm quiescence and close
 * cache. No forced release. Close before project/sampler disposal/reinit.
 * No DMA programming, timer, audio.device reservation or song dispatch here. */
int pt_paula_voices_close(struct pt_paula_voices *);
#endif
