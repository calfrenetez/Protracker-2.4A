        .text
        .globl _pt_cia_local_activate

/* C stack ABI. The 64 coherent-read attempts use the original precomputed
 * descending window. An expired or exhausted attempt writes no marker.
 * Preserve the actual raw before and after samples even after a late write. */
_pt_cia_local_activate:
        movem.l %d2-%d7/%a2,-(%sp)
        move.l 32(%sp),%a0
        move.l 36(%sp),%a1
        move.l 40(%sp),%a2
        move.w 0(%a2),%d0
        move.w 2(%a2),%d1
        moveq #63,%d7
local_poll:
        moveq #0,%d2
        move.b (%a0),%d2
        move.b (%a1),%d3
        move.b (%a0),%d4
        cmp.b %d2,%d4
        bne.b local_retry
        move.w %d2,%d5
        lsl.w #8,%d5
        move.b %d3,%d5
        cmp.w %d0,%d5
        bhi.b local_retry
        cmp.w %d1,%d5
        bls.b local_expired
        move.w #0x4752,6(%a2)
        move.b (%a0),%d5
        move.b (%a1),%d6
        move.b (%a0),%d1
        move.b %d5,11(%a2)
        move.b %d6,12(%a2)
        move.b %d1,13(%a2)
        moveq #1,%d0
        bra.b local_record
local_retry:
        dbra %d7,local_poll
        moveq #0,%d7
        moveq #0,%d0
        bra.b local_record
local_expired:
        moveq #2,%d0
local_record:
        move.b %d2,8(%a2)
        move.b %d3,9(%a2)
        move.b %d4,10(%a2)
        moveq #64,%d1
        sub.w %d7,%d1
        move.w %d1,4(%a2)
        movem.l (%sp)+,%d2-%d7/%a2
        rts
