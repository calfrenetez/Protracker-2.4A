/* Timer/software-commit diagnostic ABI only. No Paula/audio/queue callbacks.
 * A1 is the owned immutable IRQ payload. C offsets are checked before linking.
 * CIA resource calls a library-style handler, not a raw exception entry. */
    .text
    .globl _pt_diagnostic_cia_aperture_irq
_pt_diagnostic_cia_aperture_irq:
    movem.l %d2-%d7/%a2-%a6,-(%sp)
    move.l %a1,%a2
    tst.l 12(%a2)
    beq 1f
    move.l 0(%a2),%a6
    lea 28(%a2),%a0
    jsr -60(%a6) /* Actual dispatcher-before counter, not a prediction. */
    move.l %d0,36(%a2)
    move.l %a2,-(%sp)
    jsr _pt_diagnostic_cia_aperture_dispatch
    addq.l #4,%sp
    move.l 0(%a2),%a6
    lea 40(%a2),%a0
    jsr -60(%a6) /* Actual dispatcher-after bracket; epilogue excluded. */
    move.l %d0,48(%a2)
    clr.l 12(%a2)
    move.l 4(%a2),%a1
    move.l 8(%a2),%d0
    move.l 4,%a6
    jsr -324(%a6) /* Exec Signal: no Wait, owner release or buffer change. */
1:
    movem.l (%sp)+,%d2-%d7/%a2-%a6
    moveq #0,%d0
    rts
