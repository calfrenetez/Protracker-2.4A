; Timer-only diagnostic ABI. No Paula MMIO, DOS, allocation or task waiting.
; Data offsets checked at C compilation. ReadEClock documented IRQ-callable.
; A1 is data; preserve D2-D7/A2-A6 per CIA resource handler convention.
        SECTION diagnostic,CODE
        XDEF _pt_diagnostic_cia_irq
_pt_diagnostic_cia_irq
        MOVEM.L D2-D7/A2-A6,-(SP)
        MOVE.L A1,A2
        TST.L 28(A2)
        BEQ.B pt_cia_irq_done
        MOVE.L 0(A2),A6
        LEA 4(A2),A0
        JSR -60(A6)
        MOVE.L D0,12(A2)
        ADDQ.L #1,16(A2)
        CLR.L 28(A2)
        MOVE.L 20(A2),A1
        MOVE.L 24(A2),D0
        MOVE.L 4.W,A6
        JSR -324(A6) ; Exec Signal, IRQ-safe; no Wait or buffer ownership change.
pt_cia_irq_done
        MOVEM.L (SP)+,D2-D7/A2-A6
        MOVEQ #0,D0
        RTS
        END
