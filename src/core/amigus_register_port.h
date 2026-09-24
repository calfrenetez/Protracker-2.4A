#ifndef PT_AMIGUS_REGISTER_PORT_H
#define PT_AMIGUS_REGISTER_PORT_H
#include "amigus_fifo.h"
/* Injected bus operations, no native MMIO. Return1 only on confirmed completion;
 * failed writes may be partial. read16 writes its output only on success.
 * owned returns1 only while caller holds exclusive PCM reservation AND serializes
 * all register access (including interrupts). Context outlives every port use.
 * Native adapter must verify readback semantics and write32 bus ordering first. */
struct pt_amigus_register_io {
    void *context;
    int (*owned)(void *);
    int (*read16)(void *,unsigned,uint16_t *);
    int (*write16)(void *,unsigned,uint16_t);
    int (*write32)(void *,unsigned,uint32_t);
};
struct pt_amigus_register_port {struct pt_amigus_register_io io;unsigned capacity_words,aligned,fault;};
/* Initialize once before binding; never reinitialize an active port.
 * No I/O. Explicit verified capacity in16-bit words,6..65535. Never guesses card
 * depth. Reset must succeed before capacity/write/drain. No playback start API. */
int pt_amigus_register_port_init(struct pt_amigus_register_port *,const struct pt_amigus_register_io *,unsigned capacity_words);
int pt_amigus_register_capacity(void *);
int pt_amigus_register_write3(void *,const uint32_t *);
/* Disable playback, clear playback IRQ flags/masks, reset, then check disable,
 * mask and empty readback. Pending readback returns0; errors return-1. Repeated
 * calls retry the full bounded sequence. Failure never grants alignment. */
int pt_amigus_register_reset(void *);
/* FIFO-empty only; no assertion about DAC pipeline/physical silence. */
int pt_amigus_register_drain(void *);
#endif
