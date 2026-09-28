#ifndef PT_EDITOR_STUDIO_H
#define PT_EDITOR_STUDIO_H
#include "editor.h"
#include "sampler_song.h"
#include "sampler_invert_song.h"
#include "../core/studio_pump.h"
/* Caller-owned, owner-thread binding; zero/init once, detach before editor memory
 * is freed or reinitialized. Editor dispose may run first: its guard stops song
 * before sample owners are released. Does not start an audio device/backend. */
struct pt_editor_studio {struct pt_editor *editor;struct pt_sampler_song *song;struct pt_sampler_invert_song *invert_song;struct pt_studio_queue *queue;struct pt_studio_pump pump;void (*output_stop)(void *);void *output_context;};
/* Refuses an existing editor guard rather than overwriting another owner. */
int pt_editor_studio_attach(struct pt_editor_studio *,struct pt_editor *);
void pt_editor_studio_stop(struct pt_editor_studio *);
void pt_editor_studio_detach(struct pt_editor_studio *);
/* Stops any prior session before starting; uses editor sampler allocator. Options
 * must specify48k/stereo24. Project stays immutable while active; external writes
 * must prepare_change first. No PLAY action or device queue is enabled here. */
enum pt_render_result pt_editor_studio_start(struct pt_editor_studio *,const struct pt_render_options *);
/* Begin keeps pending preparation in the same edit/Stop/dispose barrier.
 * Prepare or direct pull advances one bounded step; readiness never starts a
 * device. The queued variant advances preparation through the normal pump,
 * publishing no queue blocks until all required master pins are ready. */
enum pt_render_result pt_editor_studio_begin(struct pt_editor_studio *,const struct pt_render_options *);
enum pt_render_result pt_editor_studio_prepare(struct pt_editor_studio *,unsigned *ready);
enum pt_render_result pt_editor_studio_begin_queued(struct pt_editor_studio *,const struct pt_render_options *,struct pt_studio_queue *);
/* Explicit classic EFx private-bank mode. Same editor guard/stop lifecycle;
 * sample_budget bounds additional copies from the editor's sampler allocator.
 * Unsupported16/24-bit sources are refused, never converted. Does not enable
 * native PLAY or a device transport. Ordinary start retains immutable pins. */
enum pt_render_result pt_editor_studio_start_invert(struct pt_editor_studio *,const struct pt_render_options *,size_t sample_budget);
enum pt_render_result pt_editor_studio_pull(struct pt_editor_studio *,unsigned,const struct pt_pcm **,unsigned *);
/* Borrow a fresh empty queue until stop/detach, even after natural end. Owner
 * must stop/detach BEFORE closing the queue. No direct pull while queued. Natural
 * end drains; editor edit/undo/dispose/Stop aborts unleased data and pending audio.
 * Held leases remain valid until consumer release; this does not cancel hardware. */
enum pt_render_result pt_editor_studio_start_queued(struct pt_editor_studio *,const struct pt_render_options *,struct pt_studio_queue *);
enum pt_render_result pt_editor_studio_start_invert_queued(struct pt_editor_studio *,const struct pt_render_options *,size_t sample_budget,struct pt_studio_queue *);
/* Producer/queue failure closes source ownership and requests bound output stop.
 * Invalid block sizes alone are refused without changing a valid session. */
enum pt_pump_result pt_editor_studio_step(struct pt_editor_studio *,unsigned frames);
/* Bind one nonblocking, nonreentrant output-stop request after queued start.
 * Context outlives stop/detach. Explicit/editor stops call it once after closing
 * producer pins, then clear binding. Natural producer end leaves it bound so
 * later edits still stop buffered output. Caller must poll output shutdown and
 * confirm detach before freeing output context/queue; this hook cannot wait. */
int pt_editor_studio_bind_output_stop(struct pt_editor_studio *,void (*)(void *),void *);
/* Integrated serialized editor/queue/PCM-session owner. Zero-init once; attach
 * before start. This is an injected FIFO adapter, not native MMIO or PLAY.
 * Editor, port and drain contexts outlive successful detach. Never manipulate
 * the embedded owners or queue independently, and never reenter callbacks.
 * Start failure may retain a queue while reset is uncertain: keep stepping until
 * output_busy()==0, even after ERROR. Only successful detach permits freeing contexts.
 * Each step advances at most one producer operation and one output operation.
 * No allocation occurs after preparation, and no hardware timing is promised. */
#include "../core/amigus_session.h"
#include "../core/amigus_reservation.h"
struct pt_editor_studio_output {
    struct pt_editor_studio producer;
    struct pt_amigus_session session;
    struct pt_studio_queue *queue;
    unsigned failed,quiesced;
    int (*start)(void *);void *start_context;unsigned prefill_triplets;
    struct pt_amigus_reservation *reservation;
    int (*quiesce)(void *);void *quiesce_context;
};
int pt_editor_studio_output_attach(struct pt_editor_studio_output *,struct pt_editor *);
/* Optional bounded device-start gate, bound only while attached and idle.
 * Applies to both ordinary and reserved starts; persists across Stop/restart.
 * Context outlives successful detach or an idle rebind. NULL clears the gate.
 * Pending/failed acknowledgement retains output ownership through reset. */
int pt_editor_studio_output_bind_start(struct pt_editor_studio_output *,int (*)(void *),void *);
/* Same binding with explicit nonzero startup prefill in two-frame triplets.
 * Each start checks it against the reset port capacity; no guessed device depth. */
int pt_editor_studio_output_bind_prefill(struct pt_editor_studio_output *,int (*)(void *),void *,unsigned triplets);
int pt_editor_studio_output_start(struct pt_editor_studio_output *,const struct pt_render_options *,unsigned queue_blocks,const struct pt_amigus_fifo_port *,int (*drain)(void *),void *);
/* Optional PCM reservation lease: held before any reset/output callback until
 * session reset AND quiesce(context)==1, before queue/session storage is released.
 * Quiesce must stop/detach any interrupt owner and confirm all adapter
 * references/interrupts removed; 0 pending, -1 failure, bounded/nonreentrant.
 * Caller retains library/card reservation after the lease ends and closes it.
 * This API neither verifies hardware capabilities nor permits native MMIO. */
int pt_editor_studio_output_start_reserved(struct pt_editor_studio_output *,const struct pt_render_options *,unsigned,const struct pt_amigus_fifo_port *,int (*drain)(void *),void *,struct pt_amigus_reservation *,int (*quiesce)(void *),void *);
int pt_editor_studio_output_busy(const struct pt_editor_studio_output *);
enum pt_consumer_result pt_editor_studio_output_step(struct pt_editor_studio_output *,unsigned frames);
void pt_editor_studio_output_stop(struct pt_editor_studio_output *);
/* Requests Stop but performs no I/O; refuses until pending reset is confirmed by
 * step. On success removes the editor guard and releases all owned memory. */
int pt_editor_studio_output_detach(struct pt_editor_studio_output *);
#endif
