#ifndef PT_EDITOR_PAULA_H
#define PT_EDITOR_PAULA_H
#include "editor.h"
#include "paula_song.h"
/* Zero-init caller-owned binding, serialized editor thread. Attach uses existing
 * veto-capable change barrier; refuses other guard/Studio/wavetable owners.
 * Editor, binding and all callback/allocator contexts survive failed stop.
 * Detach BEFORE editor reinit/free, including after successful dispose.
 * No native device, PLAY action, DMA, timer or clock is enabled. */
struct pt_editor_paula {struct pt_editor *editor;struct pt_paula_song *song;};
int pt_editor_paula_attach(struct pt_editor_paula *,struct pt_editor *);
/* One close attempt, no polling/forced release. Pending/failed reader stop or
 * adapter quiescence retains song, master pins and editor barrier. Explicit retry
 * required before mutation/disposal/replacement. NULL song already stopped. */
int pt_editor_paula_stop(struct pt_editor_paula *);
int pt_editor_paula_detach(struct pt_editor_paula *);
/* Freshly bound idle voices/bridge must belong to THIS editor sampler/project.
 * Uses editor sampler allocator for session/sequence/master work. Native allocator
 * must be bounded Fast RAM; Chip cache allocator remains bridge-owned. Caller
 * must rebind closed voices/cache before restart. Begin/prepare never start audio.
 * Pending preparation participates in the same edit/disposal barrier. */
enum pt_paula_song_result pt_editor_paula_begin(struct pt_editor_paula *,struct pt_paula_voices *,
    const struct pt_render_options *,const struct pt_paula_render_caps *);
enum pt_paula_song_result pt_editor_paula_prepare(struct pt_editor_paula *,struct pt_paula_preflight_report *);
enum pt_paula_song_result pt_editor_paula_next(struct pt_editor_paula *,struct pt_render_interval *);
enum pt_paula_song_result pt_editor_paula_consume(struct pt_editor_paula *,uint32_t);
enum pt_paula_song_result pt_editor_paula_prefetch(struct pt_editor_paula *);
enum pt_paula_song_result pt_editor_paula_stage(struct pt_editor_paula *);
enum pt_paula_song_result pt_editor_paula_complete(struct pt_editor_paula *);
#endif
