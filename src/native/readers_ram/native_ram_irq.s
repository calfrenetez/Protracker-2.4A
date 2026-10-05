/* RAM diagnostic trampoline, derived from native_cia_aperture_irq.s.
 * Portable assembly does not qualify actual IRQ execution.
 * CIA resource library-style entry: A1=is_Data; ordinary RTS, not RTE.
 * Exact vectors are pinned source facts, not IRQ-safety qualification.
 * Fixed prefix/epilogue and Exec Signal are outside the dispatcher bracket.
 * Stack samples update only the append-only owned tail. No new push, call,
 * library operation or notification is added. D0/D1/A0 are caller-clobbered;
 * native object review must verify this existing library-entry convention.
 * Dispatcher samples surround the existing JSR with its4-byte argument
 * still present; they are caller boundaries, not callee minima.
 * Unobserved library/nested interrupt peaks remain unknown. The first MOVEM
 * occurs before its post-save sample: these samples do not prevent overflow.
 */
        .text
/* D0=actual sampled SP; base=owned payload; D1 is temporary. The exact seven
 * phase/count observations and flags are checked only after source close.
 */
        .macro RAM_STACK_SAMPLE phase, base
        cmpi.l #7,84(\base)
        bhs .Lstack_count_bad\@
        addq.l #1,84(\base)
        bra .Lstack_count_done\@
.Lstack_count_bad\@:
        ori.l #4,80(\base)
.Lstack_count_done\@:
        move.l 80(\base),%d1
        andi.l #\phase,%d1
        beq .Lstack_phase_done\@
        ori.l #8,80(\base)
.Lstack_phase_done\@:
        ori.l #\phase,80(\base)
        cmpi.l #1,56(\base)
        beq .Lstack_version_done\@
        ori.l #16,80(\base)
.Lstack_version_done\@:
        move.l 60(\base),%d1
        beq .Lstack_bounds_bad\@
        btst #0,%d1
        bne .Lstack_bounds_bad\@
        move.l 64(\base),%d1
        btst #0,%d1
        bne .Lstack_bounds_bad\@
        cmp.l 60(\base),%d1
        bls .Lstack_bounds_bad\@
        btst #0,%d0
        bne .Lstack_sample_bad\@
        cmp.l 60(\base),%d0
        blo .Lstack_sample_bad\@
        cmp.l 64(\base),%d0
        bhs .Lstack_sample_bad\@
        tst.l 72(\base)
        beq .Lstack_min\@
        cmp.l 72(\base),%d0
        bhs .Lstack_done\@
.Lstack_min\@:
        move.l %d0,72(\base)
        bra .Lstack_done\@
.Lstack_bounds_bad\@:
        ori.l #1,80(\base)
        bra .Lstack_done\@
.Lstack_sample_bad\@:
        ori.l #2,80(\base)
.Lstack_done\@:
        .endm
        .globl _pt_private_native_ram_irq
_pt_private_native_ram_irq:
        move.l %sp,%d0
        movem.l %d2-%d7/%a2-%a6,-(%sp)
        move.l %a1,%a2
        tst.l 12(%a2)
        beq .Lram_done
        move.l %d0,68(%a2)
        RAM_STACK_SAMPLE 0x0100,%a2
        move.l %sp,%d0
        RAM_STACK_SAMPLE 0x0200,%a2
        move.l 0(%a2),%a6
        lea 28(%a2),%a0
        jsr -60(%a6)
        move.l %d0,36(%a2)
        move.l %a2,-(%sp)
        move.l %sp,%d0
        RAM_STACK_SAMPLE 0x0400,%a2
        jsr _pt_private_native_ram_dispatch
        move.l %d0,52(%a2)
        move.l %sp,%d0
        RAM_STACK_SAMPLE 0x0800,%a2
        addq.l #4,%sp
        move.l 0(%a2),%a6
        lea 40(%a2),%a0
        jsr -60(%a6)
        move.l %d0,48(%a2)
        move.l %sp,%d0
        RAM_STACK_SAMPLE 0x1000,%a2
        clr.l 12(%a2)
        move.l 4(%a2),%a1
        move.l 8(%a2),%d0
        move.l 4,%a6
        jsr -324(%a6)
        move.l %sp,%d0
        RAM_STACK_SAMPLE 0x2000,%a2
        move.l %a2,%a0
        movem.l (%sp)+,%d2-%d7/%a2-%a6
        move.l %sp,%d0
        move.l %d0,76(%a0)
        RAM_STACK_SAMPLE 0x4000,%a0
        moveq #0,%d0
        rts
.Lram_done:
        movem.l (%sp)+,%d2-%d7/%a2-%a6
        moveq #0,%d0
        rts
