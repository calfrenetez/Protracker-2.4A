#ifndef PT_WAVETABLE_SONG_H
#define PT_WAVETABLE_SONG_H
#include "wavetable_dispatch.h"
struct pt_wavetable_song;
enum pt_wavetable_song_result {
    PT_WAVETABLE_SONG_OK, PT_WAVETABLE_SONG_DONE, PT_WAVETABLE_SONG_STOPPING,
    PT_WAVETABLE_SONG_INVALID, PT_WAVETABLE_SONG_RANGE, PT_WAVETABLE_SONG_CAPABILITY,
    PT_WAVETABLE_SONG_MEMORY, PT_WAVETABLE_SONG_STALE, PT_WAVETABLE_SONG_RENDER,
    PT_WAVETABLE_SONG_DEVICE
};
/* Serial owner-thread session. A successful open takes exclusive use of an
 * already-bound, idle voice owner/bridge until close (outer reservation remains
 * caller-owned). Do not use its direct trigger/dispatch/close APIs meanwhile.
 * Complete capability preflight occurs BEFORE source pins, uploads or starts.
 * Only slots restored at range start or triggered during output are master-pinned
 * (whole-song sessions pin all triggered slots). Promotion
 * may retain owned Fast copies in sampler.current even if a later open step
 * fails; sample content/precision/history remain unchanged. On failure *out and
 * voice ownership are unchanged; report receives capability analysis if run.
 * Project, sampler, voices, allocator and callback contexts must outlive close.
 * Project patterns/orders/metadata are BORROWED and MUST remain immutable;
 * stop/close before edits, replacement or sampler reinitialization. Sampler
 * generation, table identities and project metadata are checked on every step;
 * these guards do not detect in-place writes to pattern/order/sample arrays.
 * Options/format are copied. Row-range playback requires an explicit exact-restore
 * callback; missing support returns RANGE. No native device/scheduler. */
enum pt_wavetable_song_result pt_wavetable_song_open(struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_playback_format *,const struct pt_allocator *,
    struct pt_wavetable_preflight_report *,struct pt_wavetable_song **out);
/* next -> consume elapsed frames (1..256 each, exactly interval.frames) ->
 * complete. A zero-frame/end interval still requires complete. Caller schedules
 * time for emit=1; emit=0 pre-roll advances silently without waiting or any voice
 * dispatch. consume only advances software phase, never waits or generates audio.
 * For ranges, next restores the exact snapshot ONCE at the first emit=1 interval,
 * before returning its frames. Thus next may upload/invoke restore callbacks and
 * fail with DEVICE. Even a final emitting interval must restore before consuming.
 * complete applies one plan with internal256-byte staging. Natural end requests
 * confirmed stop and returns DONE or STOPPING. No later interval can restart it.
 * Invalid protocol is refused without advancement. Stale/internal/device errors
 * poison playback and attempt bounded stop; close must still complete. A failed
 * dispatch can make its own cleanup stop attempt before the session stop pass. */
enum pt_wavetable_song_result pt_wavetable_song_next(struct pt_wavetable_song *,struct pt_render_interval *);
enum pt_wavetable_song_result pt_wavetable_song_consume(struct pt_wavetable_song *,uint32_t frames);
enum pt_wavetable_song_result pt_wavetable_song_complete(struct pt_wavetable_song *);
/* One stop attempt per held voice; no polling. Retains ALL master pins/controller
 * and unconfirmed device leases until all stops and bridge detach succeed.
 * Returns0 while unresolved; caller must retain/retry *song. On success frees
 * controller, nulls *song and returns1. NULL *song is already closed. */
int pt_wavetable_song_close(struct pt_wavetable_song **song);
#endif
