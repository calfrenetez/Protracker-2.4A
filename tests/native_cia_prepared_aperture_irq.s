/* Separate prepared RAM-shadow experiment. CIA library-style entry: A1 is a
 * positively owned, private payload. C checks every ABI offset. Task exclusion
 * has finalized the full synthetic key and copied a descriptor with cap=64.
 * No arbitrary callback, live owner lookup, editor or audio operation here.
 * Fixed ReadEClock follows the library ABI (D2-D7/A2-A6 preserved).
 * All returned raw words/rates are retained, including a failed final read.
 * No claim that this finite read cap bounds total hardware interrupt WCET. */
    .text
    .globl _pt_diagnostic_cia_prepared_aperture_irq
_pt_diagnostic_cia_prepared_aperture_irq:
    movem.l %d2-%d7/%a2-%a6,-(%sp)
    move.l %a1,%a2
    tst.l 12(%a2)
    beq .Lreturn
    addq.l #1,16(%a2)
    tst.l 20(%a2)
    bne .Lnested
    moveq #1,%d0
    move.l %d0,20(%a2)
    lea 24(%a2),%a4 /* Immutable clock descriptor. */
    lea 52(%a2),%a5 /* Result; only this handler writes until closure. */
    lea 80(%a2),%a3 /* 64 raw EClock high/low/frequency records. */
    moveq #0,%d3
    tst.l 24(%a4)
    bne .Lstale /* Serialized task admission refused: no clock/store. */
    move.l 0(%a2),%a6
    move.l 0(%a4),%d4
    move.l 4(%a4),%d5
    move.l 8(%a4),%d6
    move.l 12(%a4),%d7
    moveq #0,%d2 /* Previous exact offset, valid after first sample. */
    /* Common window wholly within one high word. Compute its absolute bounds
     * once, before the first actual read. A4 holds the expected high WORD VALUE
     * (not an address) on this path; library ABI preserves it. A low-word carry
     * in the window itself takes the unchanged general path below. */
    move.l %d4,%a4
    add.l %d5,%d6
    bcc .Lwindow_high
    addq.l #1,%a4
.Lwindow_high:
    add.l %d5,%d7
    cmp.l %d6,%d7
    bhi .Lfast_read
    lea 24(%a2),%a4
    move.l 8(%a4),%d6
    move.l 12(%a4),%d7
.Lread:
    move.l %a3,%a0
    jsr -60(%a6)
    move.l %d0,8(%a3)
    addq.l #1,%d3
    cmp.l 16(%a4),%d0
    bne .Lclock
    /* Full 64-bit subtraction, not a low-word/modular-clock comparison.
     * SUBX's cumulative Z cannot test the high word; use carry, then TST. */
    move.l (%a3)+,%d0
    move.l (%a3)+,%d1
    addq.l #4,%a3
    sub.l %d5,%d1
    subx.l %d4,%d0
    bcs .Lbelow_arm
    tst.l %d0
    bne .Lexpired
    cmp.l %d2,%d1
    bcs .Lclock
    move.l %d1,%d2
    cmp.l %d7,%d1
    bcc .Lexpired
    cmp.l %d6,%d1
    bcs .Lpoll
    tst.l 24(%a5)
    bne .Lreentry
    /* Every successful pre/post sample is in [first,last). Preparation proves
     * last-arm <= residency cap; entry >= arm and monotonicity then imply the
     * original residency bound without per-read 64-bit metadata traversal. */
    move.l #0x41504552,12(%a5) /* The only irreversible diagnostic write. */
    move.l %a3,%a0
    jsr -60(%a6) /* Immediately following actual sample, before bookkeeping. */
.Lpost_observed:
    move.l %d0,8(%a3)
    addq.l #1,%d3
    moveq #1,%d1
    move.l %d1,8(%a5) /* Record irreversible effect even if post-read fails. */
    cmp.l 16(%a4),%d0
    bne .Lclock
    move.l (%a3),%d0
    move.l 4(%a3),%d1
    sub.l %d5,%d1
    subx.l %d4,%d0
    bcs .Lclock
    tst.l %d0
    bne .Lpost_window
    cmp.l %d2,%d1
    bcs .Lclock
    tst.l 24(%a5)
    bne .Lreentry
    cmp.l %d7,%d1
    bcc .Lpost_window
    cmp.l %d6,%d1
    bcs .Lpost_window
    moveq #1,%d0 /* COMMITTED */
    bra .Lfinish
.Lfast_read:
    move.l %a3,%a0
    jsr -60(%a6)
    move.l %d0,8(%a3)
    addq.l #1,%d3
    cmp.l 40(%a2),%d0 /* Original descriptor frequency, BEFORE any store. */
    bne .Lclock
    cmpa.l (%a3),%a4 /* Full high-word equality; no modular-clock admission. */
    bne .Lfast_general
    move.l 4(%a3),%d0
    cmp.l %d6,%d0
    bcs .Lfast_general
    cmp.l %d7,%d0
    bcc .Lexpired
    /* Every earlier accepted pre-store read took the below-first branch.
     * Thus this full-width [first,last) sample exceeds that previous read.
     * Entry>=arm and the residency bound follow from the prepared window.
     * Neither frequency, high word, window nor reentry checks are deferred. */
    tst.l 24(%a5)
    bne .Lreentry
    move.l #0x41504552,12(%a5)
    lea 12(%a3),%a0
    jsr -60(%a6)
    /* The post-store sample is already actual. Now recover the exact previous
     * offset and ordinary descriptor registers for common post-read checks.
     * Bounded subtraction is exact even if arm->first crossed a low word. */
    move.l 4(%a3),%d2
    sub.l %d5,%d2
    lea 12(%a3),%a3
    lea 24(%a2),%a4
    move.l 8(%a4),%d6
    move.l 12(%a4),%d7
    bra .Lpost_observed
.Lfast_general:
    /* Pre-window samples still receive all full-width arm/regression checks.
     * A different high word cannot qualify through the absolute low bounds. */
    move.l (%a3)+,%d0
    move.l (%a3)+,%d1
    addq.l #4,%a3
    sub.l %d5,%d1
    subx.l %d4,%d0
    bcs .Lbelow_arm
    tst.l %d0
    bne .Lexpired
    cmp.l %d2,%d1
    bcs .Lclock
    move.l %d1,%d2
    cmp.l 36(%a2),%d1
    bcc .Lexpired
    cmp.l 32(%a2),%d1
    bcc .Lclock /* Impossible for immutable same-high-word window; fail closed. */
    move.l 44(%a2),%d0
    subq.l #1,%d0
    cmp.l %d0,%d3
    bcs .Lfast_read
    moveq #7,%d0
    bra .Lfinish
.Lpoll:
    move.l 20(%a4),%d0
    subq.l #1,%d0 /* Always reserve one actual post-store read. */
    cmp.l %d0,%d3
    bcs .Lread
    moveq #7,%d0 /* READ_LIMIT */
    bra .Lfinish
.Lbelow_arm:
    cmp.l #1,%d3
    bne .Lclock
    moveq #6,%d0 /* EARLY */
    bra .Lfinish
.Lstale:
    moveq #3,%d0
    bra .Lfinish
.Lclock:
    moveq #4,%d0
    bra .Lfinish
.Lexpired:
    moveq #5,%d0
    bra .Lfinish
.Lpost_window:
    moveq #9,%d0
    bra .Lfinish
.Lreentry:
    moveq #11,%d0
.Lfinish:
    move.l %d0,0(%a5)
    move.l %d3,4(%a5)
    move.l %d3,%d0
    tst.l 8(%a5)
    beq .Lbefore
    move.l %d3,20(%a5)
    subq.l #1,%d0
.Lbefore:
    move.l %d0,16(%a5)
    clr.l 20(%a2)
    clr.l 12(%a2)
    move.l 4(%a2),%a1
    move.l 8(%a2),%d0
    move.l 4,%a6
    jsr -324(%a6)
    bra .Lreturn
.Lnested:
    moveq #1,%d0
    move.l %d0,76(%a2) /* Sticky reentry latch, no nested clock/store/Signal. */
.Lreturn:
    movem.l (%sp)+,%d2-%d7/%a2-%a6
    moveq #0,%d0
    rts
