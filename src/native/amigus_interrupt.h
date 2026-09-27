#ifndef PT_NATIVE_AMIGUS_INTERRUPT_H
#define PT_NATIVE_AMIGUS_INTERRUPT_H
#include <exec/types.h>
/* Private stable callback context. Caller owns interrupt installation/removal and
 * all device access; this shim installs nothing and does no MMIO. Set up/arm only
 * before installing or while the source is disabled and callbacks are quiescent.
 * Keep context/handler alive until confirmed interrupt removal. Handler must be
 * bounded, nonblocking, interrupt-safe and nonreentrant; never allocate or use DOS.
 * Only handler result1 reports handled; all other results are normalized to0.
 * Call the entry via the library's a0-context/d0-result ABI, never as a C function
 * with arguments. SDK __REG__ may expand away with the pinned GCC configuration. */
struct pt_native_amigus_interrupt {
    LONG (*handler)(void *);
    void *context;
    unsigned armed;
};
LONG pt_native_amigus_interrupt_dispatch(struct pt_native_amigus_interrupt *);
extern void pt_native_amigus_interrupt_entry(void);
#endif
