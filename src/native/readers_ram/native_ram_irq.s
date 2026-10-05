/* RAM diagnostic trampoline, derived from native_cia_aperture_irq.s.
 * Portable assembly does not qualify actual IRQ execution.
 * CIA resource library-style entry: A1=is_Data; ordinary RTS, not RTE.
 * Exact vectors are pinned source facts, not IRQ-safety qualification.
 * Fixed prefix/epilogue and Exec Signal are outside the dispatcher bracket.
 */
        .text
        .globl _pt_private_native_ram_irq
_pt_private_native_ram_irq:
        movem.l %d2-%d7/%a2-%a6,-(%sp)
        move.l %a1,%a2
        tst.l 12(%a2)
        beq .Lram_done
        move.l 0(%a2),%a6
        lea 28(%a2),%a0
        jsr -60(%a6)
        move.l %d0,36(%a2)
        move.l %a2,-(%sp)
        jsr _pt_private_native_ram_dispatch
        addq.l #4,%sp
        move.l %d0,52(%a2)
        move.l 0(%a2),%a6
        lea 40(%a2),%a0
        jsr -60(%a6)
        move.l %d0,48(%a2)
        clr.l 12(%a2)
        move.l 4(%a2),%a1
        move.l 8(%a2),%d0
        move.l 4,%a6
        jsr -324(%a6)
.Lram_done:
        movem.l (%sp)+,%d2-%d7/%a2-%a6
        moveq #0,%d0
        rts
