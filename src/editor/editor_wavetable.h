#ifndef PT_EDITOR_WAVETABLE_H
#define PT_EDITOR_WAVETABLE_H
#include "editor.h"
#include "wavetable_song.h"
/* Zero-initialized caller-owned binding, owner thread only. Context/editor
 * survive pending stops. Attach refuses another editor guard/Studio owner.
 * Detach BEFORE freeing/reinitializing editor, even after successful dispose.
 * No native device, PLAY action or clock is enabled by this binding. */
struct pt_editor_wavetable {struct pt_editor *editor;struct pt_wavetable_song *song;};
int pt_editor_wavetable_attach(struct pt_editor_wavetable *,struct pt_editor *);
/* Stop/detach return0 until all device stops are confirmed. Pending detach
 * retains guard and context. Caller must retry explicitly, never force-free. */
int pt_editor_wavetable_stop(struct pt_editor_wavetable *);
int pt_editor_wavetable_detach(struct pt_editor_wavetable *);
/* Requires no existing song and a freshly bound idle voice owner whose bridge
 * belongs to THIS editor sampler/project. Caller owns outer reservation and
 * rebinding after close. Allocations use the editor's sampler allocator. */
enum pt_wavetable_song_result pt_editor_wavetable_start(struct pt_editor_wavetable *,struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_playback_format *,struct pt_wavetable_preflight_report *);
/* Incremental start: pending owner.song participates in the existing edit/Stop/
 * dispose barrier. With a bound quiescence callback, cancellation also retains
 * preparation/master contexts until that callback and IRQ guard are clear.
 * Without it, cancellation before readiness leaves voices/bridge bound; caller
 * must close/reuse that synchronous idle backend before freeing its contexts. */
enum pt_wavetable_song_result pt_editor_wavetable_begin(struct pt_editor_wavetable *,struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_playback_format *,struct pt_wavetable_preflight_report *);
enum pt_wavetable_song_result pt_editor_wavetable_prepare(struct pt_editor_wavetable *,struct pt_wavetable_preflight_report *);
enum pt_wavetable_song_result pt_editor_wavetable_next(struct pt_editor_wavetable *,struct pt_render_interval *);
enum pt_wavetable_song_result pt_editor_wavetable_consume(struct pt_editor_wavetable *,uint32_t);
enum pt_wavetable_song_result pt_editor_wavetable_complete(struct pt_editor_wavetable *);
/* Yielding playback: repeat the same step on UPLOADING; editor edits/Stop/dispose
 * cancel uploads before mutation, retaining any uncertain active-voice leases. */
enum pt_wavetable_song_result pt_editor_wavetable_next_step(struct pt_editor_wavetable *,struct pt_render_interval *);
enum pt_wavetable_song_result pt_editor_wavetable_complete_step(struct pt_editor_wavetable *);
enum pt_wavetable_song_result pt_editor_wavetable_prefetch(struct pt_editor_wavetable *);
enum pt_wavetable_song_result pt_editor_wavetable_next_prepare(struct pt_editor_wavetable *,struct pt_render_interval *);
enum pt_wavetable_song_result pt_editor_wavetable_next_commit(struct pt_editor_wavetable *);
/* Strict per-interval injected-clock gate; same editor Stop/edit ownership. */
enum pt_wavetable_song_result pt_editor_wavetable_clock_arm(struct pt_editor_wavetable *,uint64_t);
enum pt_wavetable_song_result pt_editor_wavetable_clock_service(struct pt_editor_wavetable *,uint64_t);
enum pt_wavetable_song_result pt_editor_wavetable_schedule_begin(struct pt_editor_wavetable *,uint64_t);
enum pt_wavetable_song_result pt_editor_wavetable_schedule_step(struct pt_editor_wavetable *,uint64_t,uint64_t *);
enum pt_wavetable_song_result pt_editor_wavetable_clocked_begin(struct pt_editor_wavetable *,uint64_t,pt_wavetable_clock_read,void *);
enum pt_wavetable_song_result pt_editor_wavetable_clocked_service(struct pt_editor_wavetable *,uint64_t *);
enum pt_wavetable_song_result pt_editor_wavetable_clocked_deadline(struct pt_editor_wavetable *,uint64_t *ticks);
#endif
