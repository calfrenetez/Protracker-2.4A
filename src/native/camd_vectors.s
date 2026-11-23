        .text
        .globl _pt_native_camd_vector
_pt_native_camd_vector:
        move.l %a6,-(%sp)
        move.l 8(%sp),%a6
        move.l 12(%sp),%d1
        move.l 16(%sp),%d0
        move.l 20(%sp),%a0
        move.l 24(%sp),%a1
        jsr 0(%a6,%d1.w)
        move.l (%sp)+,%a6
        rts
