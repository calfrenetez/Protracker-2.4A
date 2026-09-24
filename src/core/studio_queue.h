#ifndef PT_STUDIO_QUEUE_H
#define PT_STUDIO_QUEUE_H
#include "document.h"
#include "pcm.h"
struct pt_studio_queue;
enum pt_queue_result {PT_QUEUE_OK,PT_QUEUE_INVALID,PT_QUEUE_FULL,PT_QUEUE_EMPTY,PT_QUEUE_BUSY,PT_QUEUE_DONE,PT_QUEUE_CLOSED};
/* Serialized owner-thread queue; not lock-free, DMA memory or interrupt-safe.
 * One allocator allocation,1..8 blocks, each <=256 stereo24 frames at48k.
 * Push copies samples losslessly; master/source memory is never retained.
 * A full queue refuses without overwriting or consuming source data. */
struct pt_studio_queue *pt_studio_queue_open(const struct pt_allocator *,unsigned blocks);
enum pt_queue_result pt_studio_queue_push(struct pt_studio_queue *,const struct pt_pcm *);
/* One outstanding read lease. Returned PCM/data are READ ONLY and valid until
 * matching release. Busy/empty/error preserve outputs. Monotonic ticket prevents
 * stale release after slot reuse; exhausted tickets refuse new leases.
 * Consumer must release only AFTER its device/copy no longer references memory. */
enum pt_queue_result pt_studio_queue_acquire(struct pt_studio_queue *,const struct pt_pcm **,uint64_t *ticket);
enum pt_queue_result pt_studio_queue_release(struct pt_studio_queue *,uint64_t ticket);
/* Finish disallows pushes and drains queued audio before DONE. Abort drops all
 * unleased audio but preserves an outstanding lease until consumer release.
 * Neither calls a device or claims cancellation of an in-flight transfer. */
void pt_studio_queue_finish(struct pt_studio_queue *);
void pt_studio_queue_abort(struct pt_studio_queue *);
/* Refuses while leased, preserving object/data. Caller must complete/cancel its
 * consumer and release first. NULL closes successfully. */
enum pt_queue_result pt_studio_queue_close(struct pt_studio_queue *);
#endif
