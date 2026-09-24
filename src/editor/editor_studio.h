#ifndef PT_EDITOR_STUDIO_H
#define PT_EDITOR_STUDIO_H
#include "editor.h"
#include "sampler_song.h"
#include "../core/studio_pump.h"
/* Caller-owned, owner-thread binding; zero/init once, detach before editor memory
 * is freed or reinitialized. Editor dispose may run first: its guard stops song
 * before sample owners are released. Does not start an audio device/backend. */
struct pt_editor_studio {struct pt_editor *editor;struct pt_sampler_song *song;struct pt_studio_queue *queue;struct pt_studio_pump pump;};
/* Refuses an existing editor guard rather than overwriting another owner. */
int pt_editor_studio_attach(struct pt_editor_studio *,struct pt_editor *);
void pt_editor_studio_stop(struct pt_editor_studio *);
void pt_editor_studio_detach(struct pt_editor_studio *);
/* Stops any prior session before starting; uses editor sampler allocator. Options
 * must specify48k/stereo24. Project stays immutable while active; external writes
 * must prepare_change first. No PLAY action or device queue is enabled here. */
enum pt_render_result pt_editor_studio_start(struct pt_editor_studio *,const struct pt_render_options *);
enum pt_render_result pt_editor_studio_pull(struct pt_editor_studio *,unsigned,const struct pt_pcm **,unsigned *);
/* Borrow a fresh empty queue until stop/detach, even after natural end. Owner
 * must stop/detach BEFORE closing the queue. No direct pull while queued. Natural
 * end drains; editor edit/undo/dispose/Stop aborts unleased data and pending audio.
 * Held leases remain valid until consumer release; this does not cancel hardware. */
enum pt_render_result pt_editor_studio_start_queued(struct pt_editor_studio *,const struct pt_render_options *,struct pt_studio_queue *);
enum pt_pump_result pt_editor_studio_step(struct pt_editor_studio *,unsigned frames);
#endif
