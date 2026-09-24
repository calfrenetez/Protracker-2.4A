#ifndef PT_AMIGUS_SESSION_H
#define PT_AMIGUS_SESSION_H
#include "amigus_fifo.h"
#include "studio_consumer.h"
enum pt_amigus_session_phase {PT_AS_IDLE,PT_AS_RUN,PT_AS_TAIL,PT_AS_DRAIN,PT_AS_RESET,PT_AS_DONE};
/* Caller-owned, zero-init, serialized. No native device implementation.
 * drain(context): 1 confirms all FIFO audio consumed, 0 pending, -1 fault.
 * port.reset must confirm playback disabled, FIFO empty/aligned and no retained
 * references, not just acknowledge a reset request. All callbacks bounded.
 * Queue, port and drain contexts outlive successful detach. Stop producer before
 * stop/detach. Do not independently manipulate internal consumer/FIFO or queue. */
struct pt_amigus_session {
    struct pt_amigus_fifo fifo;
    struct pt_studio_consumer consumer;
    void *drain_context;int (*drain)(void *);
    enum pt_amigus_session_phase phase;
    unsigned reset_confirmed,failed,padding;
};
/* Requires initially confirmed reset. Failed reset leaves a recoverable owner:
 * call stop/step until detach succeeds before freeing its contexts. */
int pt_amigus_session_open(struct pt_amigus_session *,struct pt_studio_queue *,const struct pt_amigus_fifo_port *,int (*drain)(void *),void *);
enum pt_consumer_result pt_amigus_session_step(struct pt_amigus_session *);
/* Requests abort; step performs bounded reset retries even after errors. */
void pt_amigus_session_stop(struct pt_amigus_session *);
/* No I/O: succeeds only after confirmed reset/cleanup, or when never opened. */
int pt_amigus_session_detach(struct pt_amigus_session *);
#endif
