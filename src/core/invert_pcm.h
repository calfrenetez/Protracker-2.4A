#ifndef PT_INVERT_PCM_H
#define PT_INVERT_PCM_H
#include "pcm.h"
#include "invert_loop.h"
/* Caller-owned private sample workspace. Share one instance between channels
 * using one instrument. Source and storage must outlive it and remain disjoint.
 * No allocation: setup requires frames*sizeof(int32_t) caller-provided bytes. */
struct pt_invert_pcm { const struct pt_pcm *source; struct pt_pcm pcm; };
enum pt_pcm_result pt_invert_pcm_init(struct pt_invert_pcm *,const struct pt_pcm *,int32_t *,size_t capacity);
/* Cancellable setup. Zero-initialize job, keep its storage/inputs/destination
 * disjoint and immutable, and serialize until cancel. Begin validates the whole
 * source synchronously but copies no PCM. Prepare copies at most 4096 bytes;
 * only ready=1 publishes the workspace. Cancellation leaves an unpublished
 * destination unchanged, although private copy storage may contain a prefix.
 * Invalid arguments do not advance or change ready; internal stale refusal is
 * sticky. No allocation, source writes or playback is authorized while pending. */
#define PT_INVERT_PCM_COPY_MAX 4096
struct pt_invert_pcm_job {
    struct pt_invert_pcm value;struct pt_pcm source;
    struct pt_invert_pcm *destination;size_t copied;
    unsigned complete;enum pt_pcm_result failure;
};
enum pt_pcm_result pt_invert_pcm_begin(struct pt_invert_pcm_job *,struct pt_invert_pcm *,
    const struct pt_pcm *,int32_t *,size_t capacity);
enum pt_pcm_result pt_invert_pcm_prepare(struct pt_invert_pcm_job *,unsigned *ready);
void pt_invert_pcm_cancel(struct pt_invert_pcm_job *);
/* Internal bank path ONLY: the complete immutable source was already validated
 * at bank begin. Descriptor/format/capacity/alias checks remain; value scans are
 * omitted. This is not an arbitrary-input validator or persistent trust mode. */
enum pt_pcm_result pt_invert_pcm_begin_prepared(struct pt_invert_pcm_job *,struct pt_invert_pcm *,
    const struct pt_pcm *,int32_t *,size_t capacity);
/* Reset private bytes from the immutable source for a fresh render. Reset the
 * separate channel clocks too. Do not reset on ordinary instrument reloads. */
enum pt_pcm_result pt_invert_pcm_reset(struct pt_invert_pcm *);
/* Apply one clock update, complementing the selected signed8 value if due.
 * Invalid state/ranges preserve both clock and PCM. */
enum pt_pcm_result pt_invert_pcm_update(struct pt_invert_pcm *,struct pt_invert_loop *);
#endif
