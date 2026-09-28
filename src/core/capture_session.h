#ifndef PT_CAPTURE_SESSION_H
#define PT_CAPTURE_SESSION_H
#include "capture.h"
enum pt_capture_phase {PT_CS_IDLE,PT_CS_START,PT_CS_RECORD,PT_CS_STOP,PT_CS_DONE};
enum pt_capture_poll {PT_CS_PENDING,PT_CS_COMPLETE,PT_CS_ERROR};
enum pt_capture_fault {PT_CS_NO_FAULT,PT_CS_INPUT_FAULT,PT_CS_OVERRUN,PT_CS_DATA_FAULT,PT_CS_STOP_FAULT};
/* Injected, serialized input adapter; no native device implementation.
 * Format is negotiated before open. start returns exactly1 for ready, 0 pending,
 * anything else a fault (possibly partially started). Repeated pending start
 * polls must be idempotent. read synchronously copies up to max_frames into the
 * supplied interleaved signed-int32 buffer: 1 chunk, 0 no data, -2 overrun, any
 * other value a fault. It must set frames=0 when pending and may NOT retain or
 * asynchronously write the supplied pointer. stop returns exactly1 only when
 * disabled, callbacks quiescent and all borrowed references released, 0 pending,
 * anything else a fault. It must also stop a pending/failed/unstarted adapter.
 * All callbacks are bounded and non-reentrant. No callback owns the collector.
 * Context and session storage remain valid and unmoved until close succeeds. */
struct pt_capture_input {
    void *context;
    int (*start)(void *,unsigned bits,unsigned channels,uint32_t rate);
    int (*read)(void *,int32_t *,unsigned max_frames,unsigned *frames);
    int (*stop)(void *);
};
struct pt_capture_session {
    struct pt_capture capture;
    struct pt_capture_input input;
    int32_t scratch[512];
    enum pt_capture_phase phase;
    enum pt_capture_fault fault;
    unsigned cancelled;
};
/* Zero-init once. Allocates before any input callback; failed open owns nothing.
 * Do not access/move/close internal capture directly while attached. */
enum pt_capture_result pt_capture_session_open(struct pt_capture_session *,const struct pt_allocator *,const struct pt_capture_input *,unsigned bits,unsigned channels,uint32_t rate,uint32_t frames,size_t budget);
/* At most one adapter callback per poll, no allocation. ERROR is not cleanup:
 * continue polling STOP until close succeeds. Never busy-wait on the UI thread. */
enum pt_capture_poll pt_capture_session_step(struct pt_capture_session *);
/* Finish keeps the recorded prefix only after confirmed stop. Reaching the
 * frame limit requests finish automatically. Abort discards after stop. */
void pt_capture_session_finish(struct pt_capture_session *);
void pt_capture_session_abort(struct pt_capture_session *);
/* Move a successful, stopped, nonempty collector into a zero-initialized owner.
 * No allocation/copy. The recipient may publish it through sampler_capture.
 * Publication failure can therefore retain the recording for retry. */
int pt_capture_session_take(struct pt_capture_session *,struct pt_capture *);
/* No I/O. Refuses even after stop errors until an exact quiescence ack arrives.
 * Success discards any untaken recording and releases the adapter binding. */
int pt_capture_session_close(struct pt_capture_session *);
#endif
