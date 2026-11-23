        .text
/* Task-only test. Keep the samples in saved registers across PutMidi.
 * The baseline samples use the same setup and CIA reads without the call. */
        .macro camd_pair name, put=0
        .globl \name
\name:
        movem.l %d2-%d5/%a2-%a4/%a6,-(%sp)
        move.l 36(%sp),%a6
        move.l 40(%sp),%a0
        move.l 44(%sp),%d0
        move.l 48(%sp),%a2
        move.l 52(%sp),%a3
        move.l 56(%sp),%a4
        move.b (%a2),%d2
        move.b (%a3),%d3
        move.b (%a2),%d4
        .if \put
        jsr -138(%a6)
        .endif
        move.b (%a2),%d0
        move.b (%a3),%d1
        move.b (%a2),%d5
        move.b %d2,0(%a4)
        move.b %d3,1(%a4)
        move.b %d4,2(%a4)
        move.b %d0,3(%a4)
        move.b %d1,4(%a4)
        move.b %d5,5(%a4)
        move.w #0x2452,6(%a4)
        movem.l (%sp)+,%d2-%d5/%a2-%a4/%a6
        rts
        .endm

        camd_pair _pt_camd_direct_clock_baseline
        camd_pair _pt_camd_direct_clock_put,1
