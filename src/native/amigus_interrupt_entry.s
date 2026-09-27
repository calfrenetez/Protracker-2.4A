/* Published library callback: context in a0, handled result in d0.
 * Explicitly bridge to the pinned compiler's stack-argument C ABI. Preserve all
 * other registers, including volatile registers, for a conservative entry ABI.
 * This is an ordinary library callback, not a CPU interrupt exception handler:
 * return with RTS, never RTE. No installation, MMIO or OS call is made here. */
    .text
    .even
    .globl _pt_native_amigus_interrupt_entry
_pt_native_amigus_interrupt_entry:
    movem.l %d1-%d7/%a0-%a6,-(%sp)
    move.l %a0,-(%sp)
    jsr _pt_native_amigus_interrupt_dispatch
    addq.l #4,%sp
    movem.l (%sp)+,%d1-%d7/%a0-%a6
    rts
