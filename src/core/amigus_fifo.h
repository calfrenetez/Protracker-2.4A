#ifndef PT_AMIGUS_FIFO_H
#define PT_AMIGUS_FIFO_H
#include "amigus_pcm_pack.h"
/* Abstract serialized FIFO port, NOT native MMIO. Adapter must normalize free
 * capacity to 32-bit words (upstream usage is 16-bit units), guarantee exclusive
 * writer ownership, and confirm reset/alignment before returning reset=1.
 * write3=1 confirms all three words copied; any other result may be partial and
 * poisons the stream until reset. No callback retains the words pointer.
 * capacity<0 means fault. All callbacks bounded/nonblocking, no reentry. */
struct pt_amigus_fifo_port {
    void *context;
    int (*capacity)(void *);
    int (*write3)(void *,const uint32_t *);
    int (*reset)(void *);
};
struct pt_amigus_fifo {
    struct pt_amigus_fifo_port port;
    struct pt_amigus_pcm_pack pack;
    uint32_t words[384];size_t count,offset;
    unsigned ready,failed;
};
/* Zero-init once, context outlives owner. Init requires confirmed reset. */
int pt_amigus_fifo_init(struct pt_amigus_fifo *,const struct pt_amigus_fifo_port *);
/* studio_consumer-compatible callbacks: submit copies; poll makes at most one
 * capacity query and one three-word write. Completion means host data is safe,
 * NOT device silence or FIFO drain. A pending odd frame is held in pack.tail. */
int pt_amigus_fifo_submit(void *,const struct pt_pcm *);
int pt_amigus_fifo_poll(void *);
int pt_amigus_fifo_cancel(void *);
/* After producer/consumer completion, explicitly pad final odd frame; use poll
 * to submit these words. Device drain/stop remains adapter responsibility. */
int pt_amigus_fifo_finish(struct pt_amigus_fifo *,unsigned *padding);
#endif
