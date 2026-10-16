/* C ABI: pointer argument at4(SP). Both direct calls use the same owned timer
 * device base; library ABI preserves A2/A6. Only storing the first returned
 * rate and addressing the second output separate the two actual calls.
 * Counter values are library sampling instants, not call entry/return times.
 * Caller excludes interrupts and owns storage throughout these two calls.
 * No CIA acquisition, device programming, signal, audio or RAM-shadow commit. */
    .text
    .globl _pt_diagnostic_clock_pair
_pt_diagnostic_clock_pair:
    movem.l %a2/%a6,-(%sp)
    move.l 12(%sp),%a2
    move.l (%a2),%a6
    lea 4(%a2),%a0
    jsr -60(%a6)
    move.l %d0,12(%a2)
    lea 16(%a2),%a0
    jsr -60(%a6)
    move.l %d0,24(%a2)
    movem.l (%sp)+,%a2/%a6
    rts
