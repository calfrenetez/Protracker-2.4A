        .text
/* C stack ABI. The 64 coherent-read attempts use the original precomputed
 * descending window. An expired or exhausted attempt writes no marker.
 * The diagnostic Paula form measures a single fixed register write. */
        .macro local_activate name, paula=0
        .globl \name
\name:
        movem.l %d2-%d7/%a2,-(%sp)
        move.l 32(%sp),%a0
        move.l 36(%sp),%a1
        move.l 40(%sp),%a2
        .if \paula
        move.l 44(%sp),%d6
        cmpi.l #0x800f,%d6
        beq.b valid_word\@
        cmpi.l #0x000a,%d6
        beq.b valid_word\@
        moveq #3,%d0
        movem.l (%sp)+,%d2-%d7/%a2
        rts
valid_word\@:
        .endif
        move.w 0(%a2),%d0
        move.w 2(%a2),%d1
        moveq #63,%d7
local_poll\@:
        moveq #0,%d2
        move.b (%a0),%d2
        move.b (%a1),%d3
        move.b (%a0),%d4
        cmp.b %d2,%d4
        bne.b local_retry\@
        move.w %d2,%d5
        lsl.w #8,%d5
        move.b %d3,%d5
        cmp.w %d0,%d5
        bhi.b local_retry\@
        cmp.w %d1,%d5
        bls.b local_expired\@
        .if \paula
        move.w %d6,0xdff096
        .else
        move.w #0x4752,6(%a2)
        .endif
        move.b (%a0),%d5
        move.b (%a1),%d6
        move.b (%a0),%d1
        move.b %d5,11(%a2)
        move.b %d6,12(%a2)
        move.b %d1,13(%a2)
        .if \paula
        move.w #0x4752,6(%a2)
        .endif
        moveq #1,%d0
        bra.b local_record\@
local_retry\@:
        dbra %d7,local_poll\@
        moveq #0,%d7
        moveq #0,%d0
        bra.b local_record\@
local_expired\@:
        moveq #2,%d0
local_record\@:
        move.b %d2,8(%a2)
        move.b %d3,9(%a2)
        move.b %d4,10(%a2)
        moveq #64,%d1
        sub.w %d7,%d1
        move.w %d1,4(%a2)
        movem.l (%sp)+,%d2-%d7/%a2
        rts
        .endm

        local_activate _pt_cia_local_activate
        local_activate _pt_cia_local_paula_dma,1

        .globl _pt_cia_local_alarm_irq

/* CIA resource interrupt ABI, A1 is the immutable owned state address.
 * The invoked bounded aperture uses the same actual countdown and prepared
 * frame as the task experiment. No task or timer APIs run here except Signal. */
_pt_cia_local_alarm_irq:
        movem.l %d2-%d7/%a2-%a6,-(%sp)
        move.l %a1,%a2
        tst.l 20(%a2)
        beq.b local_irq_unarmed
        move.l 8(%a2),-(%sp)
        move.l 4(%a2),-(%sp)
        move.l 0(%a2),-(%sp)
        jsr _pt_cia_local_activate
        lea 12(%sp),%sp
        move.l %d0,28(%a2)
        clr.l 20(%a2)
        addq.l #1,24(%a2)
        move.l 12(%a2),%a1
        move.l 16(%a2),%d0
        move.l 4.w,%a6
        jsr -324(%a6)
        bra.b local_irq_done
local_irq_unarmed:
        addq.l #1,32(%a2)
local_irq_done:
        movem.l (%sp)+,%d2-%d7/%a2-%a6
        moveq #0,%d0
        rts
