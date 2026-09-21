#ifndef PT_INVERT_PCM_H
#define PT_INVERT_PCM_H
#include "pcm.h"
#include "invert_loop.h"
/* Caller-owned private sample workspace. Share one instance between channels
 * using one instrument. Source and storage must outlive it and remain disjoint.
 * No allocation: setup requires frames*sizeof(int32_t) caller-provided bytes. */
struct pt_invert_pcm { const struct pt_pcm *source; struct pt_pcm pcm; };
enum pt_pcm_result pt_invert_pcm_init(struct pt_invert_pcm *,const struct pt_pcm *,int32_t *,size_t capacity);
/* Reset private bytes from the immutable source for a fresh render. Reset the
 * separate channel clocks too. Do not reset on ordinary instrument reloads. */
enum pt_pcm_result pt_invert_pcm_reset(struct pt_invert_pcm *);
/* Apply one clock update, complementing the selected signed8 value if due.
 * Invalid state/ranges preserve both clock and PCM. */
enum pt_pcm_result pt_invert_pcm_update(struct pt_invert_pcm *,struct pt_invert_loop *);
#endif
