        .text
        .globl _pt_cia_reload_irq
/* cia.resource puts the retained callback state in A1. */
_pt_cia_reload_irq:
        movem.l %a6,-(%sp)
        addq.l #1,8(%a1)
        move.l 4(%a1),%d0
        move.l 0(%a1),%a1
        move.l 4.w,%a6
        jsr -324(%a6)
        movem.l (%sp)+,%a6
        moveq #0,%d0
        rts
