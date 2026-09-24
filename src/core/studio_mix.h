#ifndef PT_STUDIO_MIX_H
#define PT_STUDIO_MIX_H
#include "document.h"
#include "voice.h"
/* Owner-thread PCM block mixer, not an interrupt routine or device transport.
 * acquire pins an immutable master version and returns its descriptor/token.
 * Failure must acquire nothing. release drops exactly that pin. Context and
 * allocator must outlive session. Callbacks must not reenter the session.
 * Editing may publish a new master while old pinned versions remain alive.
 * This API never converts or copies master PCM into a playback cache. */
struct pt_studio_source {
    void *context;
    int (*acquire)(void *,uint64_t key,uint64_t version,struct pt_pcm *,void **token);
    void (*release)(void *,void *token);
};
struct pt_studio_note {
    uint64_t key,version,step;
    uint32_t start,end,loop_start,loop_end,gain[2];
    enum pt_voice_loop loop;
    unsigned linear;
};
struct pt_studio_mix;
/* One bounded allocation, at most16 voices. NULL on invalid/refused allocation. */
struct pt_studio_mix *pt_studio_open(const struct pt_allocator *,const struct pt_studio_source *,unsigned voices);
/* Failed trigger retains the old voice/pin. Successful replacement releases it
 * only after the new version has been pinned and validated. */
enum pt_pcm_result pt_studio_trigger(struct pt_studio_mix *,unsigned,const struct pt_studio_note *);
/* Play [start,end) once, then repeat independent [loop_start,loop_end) in
 * the same pinned master. Requires loop=FORWARD. The ranges may be disjoint;
 * interpolation crosses the initial end into the repeat, as pt_voice_init_segment.
 * Failure preserves the current voice AND any pending cross-source handoff. */
enum pt_pcm_result pt_studio_trigger_segment(struct pt_studio_mix *,unsigned,const struct pt_studio_note *);
/* Pin a new source for the next forward-loop boundary without retriggering.
 * Same format/channels/rate required. Replaces a pending handoff transactionally;
 * old source stays pinned until a successful read crosses the boundary. */
enum pt_pcm_result pt_studio_repeat(struct pt_studio_mix *,unsigned channel,
    uint64_t key,uint64_t version,uint32_t start,uint32_t end);
struct pt_studio_control {uint64_t step;uint32_t gain[2];};
/* Apply one tick's pitch/gain changes atomically to selected active voices.
 * controls is indexed by channel. Positive Q32 step; Q16 gains0..65536.
 * Invalid mask/control or inactive selected voice refuses the whole batch.
 * Empty mask succeeds without controls. No phase/loop/pin changes, callbacks,
 * allocation or retrigger; zero gain mutes output but keeps phase advancing. */
enum pt_pcm_result pt_studio_control(struct pt_studio_mix *,uint16_t tracks,
    const struct pt_studio_control *controls);
void pt_studio_stop(struct pt_studio_mix *,unsigned);
void pt_studio_close(struct pt_studio_mix *);
/* Pull 1..256 stereo24 frames at48kHz into caller storage, preserving phase
 * between calls. No allocation/acquire; ended voices release their pins after
 * successful mix. Failure preserves voices/output/clipped. Output must not alias
 * any pinned PCM. Clip count is per block. Caller controls tick/note scheduling;
 * no tracker sequencing, device buffering, underrun recovery or speed guarantee. */
enum pt_pcm_result pt_studio_read(struct pt_studio_mix *,struct pt_pcm *,uint64_t *clipped);
#endif
