#ifndef PT_STUDIO_CONSUMER_H
#define PT_STUDIO_CONSUMER_H
#include "studio_queue.h"
/* Serialized owner-thread transport. No callback may reenter this owner.
 * submit: 1 accepts a read-only buffer until poll/cancel returns 1; 0 refuses
 * temporarily, -1 fails. Any non-1 submit MUST retain no buffer reference.
 * poll/cancel: 1 confirms ALL buffer references gone; 0 pending; -1 failure
 * with ownership uncertain. Failure is never permission to release memory.
 * Each callback must be bounded/nonblocking. No DMA or hardware claim here. */
struct pt_studio_transport {
    void *context;
    int (*submit)(void *,const struct pt_pcm *);
    int (*poll)(void *);
    int (*cancel)(void *);
};
enum pt_consumer_result {PT_CONSUMER_PROGRESS,PT_CONSUMER_WAIT,PT_CONSUMER_FINISHED,PT_CONSUMER_ERROR};
/* Zero initialize once. Queue and transport context outlive detach success.
 * Sole consumer; do not externally acquire/release or close its queue.
 * Stop producer separately before stopping consumer. External queue abort alone
 * cannot cancel transport; call stop to request cancellation. */
struct pt_studio_consumer {
    struct pt_studio_queue *queue;
    struct pt_studio_transport transport;
    const struct pt_pcm *pcm;
    uint64_t ticket;
    unsigned leased,submitted,stopped,failed;
};
int pt_studio_consumer_attach(struct pt_studio_consumer *,struct pt_studio_queue *,const struct pt_studio_transport *);
enum pt_consumer_result pt_studio_consumer_step(struct pt_studio_consumer *);
/* Abort unleased audio and request cancellation once per call. An uncertain
 * cancel retains lease; step polls for eventual confirmation even after error. */
enum pt_consumer_result pt_studio_consumer_stop(struct pt_studio_consumer *);
/* Calls stop; refuses while leased. On success all borrowed references cleared. */
int pt_studio_consumer_detach(struct pt_studio_consumer *);
#endif
