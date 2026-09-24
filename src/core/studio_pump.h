#ifndef PT_STUDIO_PUMP_H
#define PT_STUDIO_PUMP_H
#include "studio_queue.h"
#include "render.h"
struct pt_studio_producer {
    void *context;
    enum pt_render_result (*pull)(void *,unsigned,const struct pt_pcm **,unsigned *done);
    void (*stop)(void *);
};
enum pt_pump_result {PT_PUMP_PROGRESS,PT_PUMP_BLOCKED,PT_PUMP_FINISHED,PT_PUMP_ERROR};
/* Caller-owned, serialized owner-thread pump. Borrows queue/producer contexts,
 * which outlive it. Init once; stop before reuse. No allocation here. Queue must
 * be fresh and exclusively produced by this pump. Consumer uses queue leases.
 * Producer follows studio_song pull contract. No callback reentry. */
struct pt_studio_pump {
    struct pt_studio_producer producer;struct pt_studio_queue *queue;
    struct pt_pcm pending;int32_t data[512];unsigned held,ended;
    enum pt_render_result error;
};
int pt_studio_pump_init(struct pt_studio_pump *,const struct pt_studio_producer *,struct pt_studio_queue *);
/* At most one pull or pending push per step. FULL retains a private true24 copy
 * and prevents further pulls until copied to queue. FINISHED means production
 * ended: consumer must still drain until queue DONE. Errors/stop abort unleased
 * queue data while preserving any consumer-held lease. Stop does not cancel a
 * device transfer or free queue/context. No DMA/concurrency/timing guarantee. */
enum pt_pump_result pt_studio_pump_step(struct pt_studio_pump *,unsigned frames);
void pt_studio_pump_stop(struct pt_studio_pump *);
#endif
