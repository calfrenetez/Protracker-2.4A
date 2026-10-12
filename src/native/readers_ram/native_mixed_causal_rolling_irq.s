/* Private rolling RAM Source-only 68000 trampoline.
 * Library-style resource entry: A1=is_Data; ordinary RTS with D0=0, not RTE.
 * Save d2-d7/a2-a6 exactly: eleven longwords, 44 bytes. Supply one 4-byte
 * cdecl argument to C. Including the JSR return address, the visible local
 * increment is 52 bytes; this is not the complete task/system/IRQ stack peak.
 * The incoming A4 is saved/restored with the established preserved set. No
 * correct C data-base convention is established here: the actual native
 * object, A4 convention, full callgraph and indirect callbacks remain gates.
 * No payload read, OS/library call, MMIO, timer, Signal or stack sample occurs
 * in this trampoline. C must refuse before payload reads while its native
 * binding gates remain disabled. Assembly source is not IRQ qualification.
 */
        .text
        .globl _pt_private_native_rolling_irq
_pt_private_native_rolling_irq:
        movem.l %d2-%d7/%a2-%a6,-(%sp)
        move.l %a1,-(%sp)
        jsr _pt_private_native_rolling_irq_dispatch
        addq.l #4,%sp
        movem.l (%sp)+,%d2-%d7/%a2-%a6
        moveq #0,%d0
        rts
