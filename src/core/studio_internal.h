#ifndef PT_STUDIO_INTERNAL_H
#define PT_STUDIO_INTERNAL_H
#include "studio_song.h"
/* Private call chain for the prepared immutable sampler-song owner ONLY.
 * It has fully validated the project/PCM, completed analysis and pinned every
 * required exact version. Its private provider checks generation/current-token/
 * descriptor identity and independently retains that version on every acquire.
 * All borrowed arrays and pinned values remain immutable until stop/close.
 * These functions omit only repeated PCM-value scans at voice establishment;
 * shape/capacity/geometry/alias checks and pin/rollback/release semantics remain.
 * No validation or ownership token is created here. Never use with arbitrary
 * public providers or mutable EFx banks. Public Studio APIs always fully validate.
 * There is deliberately no public session flag to opt into this call chain. */
enum pt_pcm_result pt_studio_trigger_prepared(struct pt_studio_mix *,unsigned,const struct pt_studio_note *);
enum pt_pcm_result pt_studio_segment_prepared(struct pt_studio_mix *,unsigned,const struct pt_studio_note *);
enum pt_pcm_result pt_studio_repeat_prepared(struct pt_studio_mix *,unsigned,uint64_t,uint64_t,uint32_t,uint32_t);
enum pt_pcm_result pt_studio_dispatch_prepared(struct pt_studio_mix *,unsigned,const struct pt_render_plan *,const struct pt_studio_binding *,unsigned);
enum pt_render_result pt_studio_song_pull_prepared(struct pt_studio_song *,unsigned,const struct pt_pcm **,unsigned *);
#endif
