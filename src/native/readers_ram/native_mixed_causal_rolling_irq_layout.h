/* Private rolling RAM IRQ envelope: fixed 32-bit words, no native pointers.
 * These layout assertions can be evaluated on HOST without admitting native
 * execution. The native C entry must separately require 32-bit uintptr_t and
 * refuse before reading this payload while its binding gates remain disabled.
 * Library-style IRQ entry supplies A1=is_Data and returns through ordinary
 * RTS with D0=0. Layout, source and trampoline facts do not qualify an object,
 * C data-base convention, indirect callback, residency or complete IRQ stack.
 * This envelope and every referenced object must remain live through genuine
 * independent source quiet and the full outer task leave/restoration.
 */
#ifndef PT_PRIVATE_NATIVE_MIXED_CAUSAL_ROLLING_IRQ_LAYOUT_H
#define PT_PRIVATE_NATIVE_MIXED_CAUSAL_ROLLING_IRQ_LAYOUT_H

#include <stdint.h>
#include <stddef.h>

#define PT_PRIVATE_NATIVE_ROLLING_IRQ32_VERSION 1U
#define PT_PRIVATE_NATIVE_ROLLING_IRQ32_BYTES 80U

struct pt_private_native_rolling_irq32 {
    uint32_t version;
    uint32_t bytes;
    uint32_t bridge_addr;
    uint32_t self_addr;
    uint32_t owner_addr;
    uint32_t queue_addr;
    uint32_t session;
    uint32_t generation;
    uint32_t ticket_hi;
    uint32_t ticket_lo;
    uint32_t first_hi;
    uint32_t first_lo;
    uint32_t last_hi;
    uint32_t last_lo;
    uint32_t frequency;
    uint32_t frame;
    uint32_t serial;
    volatile uint32_t calls;
    volatile int32_t result;
    volatile uint32_t state;
};

typedef char pt_private_native_rolling_uint32_size[(sizeof(uint32_t)==4)?1:-1];
typedef char pt_private_native_rolling_int32_size[(sizeof(int32_t)==4)?1:-1];
#define PT_PRIVATE_NATIVE_ROLLING_OFFSET(field,value) \
    typedef char pt_private_native_rolling_offset_##field[ \
        (offsetof(struct pt_private_native_rolling_irq32,field)==(value))?1:-1]

PT_PRIVATE_NATIVE_ROLLING_OFFSET(version,0);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(bytes,4);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(bridge_addr,8);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(self_addr,12);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(owner_addr,16);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(queue_addr,20);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(session,24);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(generation,28);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(ticket_hi,32);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(ticket_lo,36);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(first_hi,40);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(first_lo,44);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(last_hi,48);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(last_lo,52);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(frequency,56);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(frame,60);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(serial,64);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(calls,68);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(result,72);
PT_PRIVATE_NATIVE_ROLLING_OFFSET(state,76);
typedef char pt_private_native_rolling_irq32_size[
    (sizeof(struct pt_private_native_rolling_irq32)==80)?1:-1];
#undef PT_PRIVATE_NATIVE_ROLLING_OFFSET

int pt_private_native_rolling_irq_dispatch(struct pt_private_native_rolling_irq32 *);
void pt_private_native_rolling_irq(void);

#endif
