        .text
        .globl _pt_cia_direct_clock_pair
        .globl _pt_cia_direct_clock_stray

/* C stack ABI. The opaque user registers are saved before the observation.
 * Preserve six raw hardware bytes without writing the report until after the
 * second high/low/high read. The sole intervening write is to owned RAM. */
_pt_cia_direct_clock_pair:
        movem.l %d2-%d5/%a2,-(%sp)
        move.l 24(%sp),%a0
        move.l 28(%sp),%a1
        move.l 32(%sp),%a2
        move.b (%a0),%d0
        move.b (%a1),%d1
        move.b (%a0),%d2
        move.w #0x2452,6(%a2)
        move.b (%a0),%d3
        move.b (%a1),%d4
        move.b (%a0),%d5
        move.b %d0,0(%a2)
        move.b %d1,1(%a2)
        move.b %d2,2(%a2)
        move.b %d3,3(%a2)
        move.b %d4,4(%a2)
        move.b %d5,5(%a2)
        movem.l (%sp)+,%d2-%d5/%a2
        rts

/* Installed only for resource ownership. A masked timer must never dispatch. */
_pt_cia_direct_clock_stray:
        addq.l #1,(%a1)
        moveq #0,%d0
        rts
