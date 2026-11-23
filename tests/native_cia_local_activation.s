        .text
/* C stack ABI. The 64 coherent-read attempts use the original precomputed
 * descending window. An expired or exhausted attempt writes no marker.
 * The diagnostic Paula form measures a single fixed register write. */
        .macro local_activate name, paula=0, mixed=0, tail=0, serial=0, wire=0
        .globl \name
\name:
        .set .Lwire_clock_fast\@,0
        .ifdef pt_exp_mixed_wire_clock_fast
        .set .Lwire_clock_fast\@,\wire
        .endif
        .if \mixed
        .if \wire
        movem.l %d2-%d7/%a2-%a6,-(%sp)
        .set local_args\@,48
        .if .Lwire_clock_fast\@
        lea 0xdff030,%a6
        .else
        lea 0xdff000,%a6
        .endif
        .else
        movem.l %d2-%d7/%a2-%a5,-(%sp)
        .set local_args\@,44
        .endif
        .else
        movem.l %d2-%d7/%a2,-(%sp)
        .set local_args\@,32
        .endif
        move.l local_args\@(%sp),%a0
        move.l local_args\@+4(%sp),%a1
        move.l local_args\@+8(%sp),%a2
        .if \mixed
        .if .Lwire_clock_fast\@
        /* Pack the two modeled card words into one RAM write. Keep the
         * two real custom-register addresses ready before the aperture. */
        move.l 16(%a2),%d0
        swap %d0
        move.w 22(%a2),%d0
        movea.l %d0,%a3
        lea 0xdff096,%a4
        .else
        movea.l 16(%a2),%a3
        movea.l 20(%a2),%a4
        .endif
        .if \wire
        movea.w 36(%a2),%a5
        .else
        movea.l 24(%a2),%a5
        .endif
        .endif
        .if \paula
        move.l local_args\@+12(%sp),%d6
        cmpi.l #0x800f,%d6
        beq.b valid_word\@
        cmpi.l #0x000a,%d6
        beq.b valid_word\@
        cmpi.l #0x000f,%d6
        beq.b valid_word\@
        moveq #3,%d0
        bra.w local_return\@
valid_word\@:
        .endif
        .if \serial+\wire
        .if \wire
        /* Preserve the Paula word and validate the first serial word separately. */
        move.l %a5,%d0
        cmpi.l #0x180,%d0
        blo.w serial_refused\@
        cmpi.l #0x19f,%d0
        .else
        move.l local_args\@+12(%sp),%d6
        cmpi.l #0x180,%d6
        blo.w serial_refused\@
        cmpi.l #0x19f,%d6
        .endif
        bhi.w serial_refused\@
        move.w 0xdff018,%d0
        andi.w #0xf000,%d0
        cmpi.w #0x3000,%d0
        bne.w serial_refused\@
        .endif
        move.w 0(%a2),%d0
        move.w 2(%a2),%d1
        .if \wire
        /* Reserve the observed issue-and-bracket span. The fast path
         * compares low bytes after matching the high byte. A window
         * that straddles a low-byte boundary needs the full counter. */
        .if .Lwire_clock_fast\@
        addi.w #9,%d1
        .else
        addq.w #8,%d1
        .endif
        bcs.w serial_refused\@
        cmp.w %d1,%d0
        bls.w serial_refused\@
        move.w %d0,%d5
        eor.w %d1,%d5
        andi.w #0xff00,%d5
        .if .Lwire_clock_fast\@
        bne.w wire_full_setup\@
        .else
        bne.w serial_refused\@
        .endif
        move.w %d0,%d5
        lsr.w #8,%d5
        .endif
        .if \tail
        .if \mixed
        /* This fixture only arms with the owner's one-count end guard. */
        cmpi.w #1,%d1
        bne.w local_mixed_tail_refused\@
        .else
        /* Keep a whole six-tick observation inside the chosen aperture. */
        move.w %d1,%d6
        addq.w #6,%d6
        .endif
        .endif
        moveq #63,%d7
local_poll\@:
        moveq #0,%d2
        move.b (%a0),%d2
        move.b (%a1),%d3
        move.b (%a0),%d4
        cmp.b %d2,%d4
        bne.b local_retry\@
        .if \wire
        cmp.b %d5,%d2
        bhi.b local_retry\@
        blo.b local_expired\@
        cmp.b %d0,%d3
        bhi.b local_retry\@
        cmp.b %d1,%d3
        bls.b local_expired\@
        .else
        move.w %d2,%d5
        lsl.w #8,%d5
        move.b %d3,%d5
        .if \tail
        /* The separately armed alert starts near zero. An upper sample
         * can indicate rollover before the owner's callback is dispatched. */
        cmpi.w #512,%d5
        bhi.b local_tail_stale\@
        .if \mixed
        cmpi.w #8,%d5
        .else
        cmp.w %d6,%d5
        .endif
        bls.b local_expired\@
        .endif
        cmp.w %d0,%d5
        bhi.b local_retry\@
        cmp.w %d1,%d5
        bls.b local_expired\@
        .endif
local_issue\@:
        .if \paula
        .if \wire
        .if .Lwire_clock_fast\@
        move.w %d6,(%a4)
        move.l %a3,28(%a2)
        move.w %a5,(%a6)
        .else
        move.w %d6,0x96(%a6)
        /* A3 then A4 to the two contiguous modeled card fields. */
        movem.w %a3-%a4,28(%a2)
        move.w %a5,0x30(%a6)
        .endif
        .else
        move.w %d6,0xdff096
        .if \mixed
        /* These two stores still model the AmiGUS registers in RAM. */
        move.w %a3,28(%a2)
        move.w %a4,30(%a2)
        move.l %a5,32(%a2)
        .endif
        .endif
        .else
        .if \serial
        move.w %d6,0xdff030
        .else
        move.w #0x4752,6(%a2)
        .endif
        .endif
        move.b (%a0),%d5
        move.b (%a1),%d6
        move.b (%a0),%d1
        move.b %d5,11(%a2)
        move.b %d6,12(%a2)
        move.b %d1,13(%a2)
        .if \paula+\serial
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
        .if \tail
        bra.b local_record\@
local_tail_stale\@:
        moveq #3,%d0
        .endif
local_record\@:
        move.b %d2,8(%a2)
        move.b %d3,9(%a2)
        move.b %d4,10(%a2)
        moveq #64,%d1
        sub.w %d7,%d1
        move.w %d1,4(%a2)
        .if \tail
        .if \mixed
        bra.b local_return\@
local_mixed_tail_refused\@:
        moveq #3,%d0
        .endif
        .endif
        .if \serial+\wire
        bra.b local_return\@
serial_refused\@:
        moveq #3,%d0
        .endif
local_return\@:
        .if \mixed
        .if \wire
        movem.l (%sp)+,%d2-%d7/%a2-%a6
        .else
        movem.l (%sp)+,%d2-%d7/%a2-%a5
        .endif
        .else
        movem.l (%sp)+,%d2-%d7/%a2
        .endif
        rts
        .if .Lwire_clock_fast\@
        /* Only a window crossing the low-byte boundary takes this path.
         * Keep both original bounds and the same early-stop margin. */
wire_full_setup\@:
        moveq #63,%d7
wire_full_poll\@:
        moveq #0,%d2
        move.b (%a0),%d2
        move.b (%a1),%d3
        move.b (%a0),%d4
        cmp.b %d2,%d4
        bne.b wire_full_retry\@
        move.w %d2,%d5
        lsl.w #8,%d5
        move.b %d3,%d5
        cmp.w %d0,%d5
        bhi.b wire_full_retry\@
        cmp.w %d1,%d5
        bls.w local_expired\@
        bra.w local_issue\@
wire_full_retry\@:
        dbra %d7,wire_full_poll\@
        moveq #0,%d7
        moveq #0,%d0
        bra.w local_record\@
        .endif
        .endm

        local_activate _pt_cia_local_activate
        .ifdef pt_exp_serial_midi
        local_activate _pt_cia_local_serial_midi,0,0,0,1
        .endif
        local_activate _pt_cia_local_paula_dma,1
        local_activate _pt_cia_local_mixed_dma,1,1
        .ifdef pt_exp_mixed_wire
        local_activate _pt_cia_local_mixed_wire,1,1,0,0,1
        .endif
        .ifdef pt_exp_cia_tail
        local_activate _pt_cia_local_tail,0,0,1
        .endif
        .ifdef pt_exp_mixed_tail
        local_activate _pt_cia_local_mixed_tail_dma,1,1,1
        .endif

/* CIA resource interrupt ABI, A1 is the immutable owned state address.
 * The invoked bounded aperture uses the same actual countdown and prepared
 * frame as the task experiment. No task or timer APIs run here except Signal. */
        .macro local_alarm_irq name, paula=0, mixed=0, epoch=0, tail=0, serial=0, wire=0
        .globl \name
\name:
        movem.l %d2-%d7/%a2-%a6,-(%sp)
        move.l %a1,%a2
        tst.l 20(%a2)
        beq.w local_irq_unarmed\@
        .if \epoch
        tst.l 48(%a2)
        beq.b local_irq_stale\@
        move.l 48(%a2),%a0
        move.l (%a0),%d0
        move.l %d0,56(%a2)
        cmp.l 52(%a2),%d0
        bne.b local_irq_stale\@
        .endif
        .if \paula
        tst.l 36(%a2)
        beq.b local_irq_refused\@
        movea.l 36(%a2),%a0
        cmpi.b #5,8(%a0)
        bne.b local_irq_refused\@
        cmpi.l #15,24(%a0)
        bne.b local_irq_refused\@
        move.w 44(%a2),%d0
        cmp.w 32(%a0),%d0
        bne.b local_irq_refused\@
        move.w 0xdff002,%d0
        andi.w #0x020f,%d0
        cmp.w 46(%a2),%d0
        bne.b local_irq_refused\@
        move.l 40(%a2),-(%sp)
        .endif
        .if \serial
        move.l 40(%a2),-(%sp)
        .endif
        move.l 8(%a2),-(%sp)
        move.l 4(%a2),-(%sp)
        move.l 0(%a2),-(%sp)
        .if \paula
        .if \mixed
        .if \tail
        cmpi.l #0x000f,40(%a2)
        bne.b local_mixed_start\@
        jsr _pt_cia_local_mixed_tail_dma
        bra.b local_mixed_done\@
local_mixed_start\@:
        .endif
        .if \wire
        jsr _pt_cia_local_mixed_wire
        .else
        jsr _pt_cia_local_mixed_dma
        .endif
        .if \tail
local_mixed_done\@:
        .endif
        .else
        jsr _pt_cia_local_paula_dma
        .endif
        lea 16(%sp),%sp
        bra.b local_irq_result\@
local_irq_refused\@:
        moveq #3,%d0
local_irq_result\@:
        .else
        .if \tail
        jsr _pt_cia_local_tail
        .else
        .if \serial
        jsr _pt_cia_local_serial_midi
        .else
        jsr _pt_cia_local_activate
        .endif
        .endif
        .if \serial
        lea 16(%sp),%sp
        .else
        lea 12(%sp),%sp
        .endif
        .endif
        .if \epoch
        bra.b local_irq_epoch_result\@
local_irq_stale\@:
        moveq #3,%d0
local_irq_epoch_result\@:
        .endif
        move.l %d0,28(%a2)
        clr.l 20(%a2)
        addq.l #1,24(%a2)
        move.l 12(%a2),%a1
        move.l 16(%a2),%d0
        move.l 4.w,%a6
        jsr -324(%a6)
        bra.b local_irq_done\@
local_irq_unarmed\@:
        addq.l #1,32(%a2)
local_irq_done\@:
        movem.l (%sp)+,%d2-%d7/%a2-%a6
        moveq #0,%d0
        rts
        .endm

        local_alarm_irq _pt_cia_local_alarm_irq
        local_alarm_irq _pt_cia_local_paula_alarm_irq,1
        local_alarm_irq _pt_cia_local_mixed_alarm_irq,1,1
        .ifdef pt_exp_mixed_wire
        local_alarm_irq _pt_cia_local_mixed_wire_irq,1,1,0,0,0,1
        .endif
        .ifdef pt_exp_mixed_wire_reload
        local_alarm_irq _pt_cia_local_mixed_wire_reload_irq,1,1,1,0,0,1
        .endif
        local_alarm_irq _pt_cia_local_epoch_alarm_irq,0,0,1
        .ifdef pt_exp_cia_tail
        local_alarm_irq _pt_cia_local_tail_irq,0,0,1,1
        .endif
        .ifdef pt_exp_mixed_reload
        local_alarm_irq _pt_cia_local_mixed_reload_irq,1,1,1
        .endif
        .ifdef pt_exp_mixed_tail
        local_alarm_irq _pt_cia_local_mixed_tail_irq,1,1,1,1
        .endif


        .ifdef pt_exp_serial_midi
        local_alarm_irq _pt_cia_local_serial_midi_irq,0,0,0,0,1
        .endif
