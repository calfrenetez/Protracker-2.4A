; Timer-only diagnostic ABI. No Paula MMIO, DOS, allocation or task waiting.
; Data offsets checked at C compilation. ReadEClock documented IRQ-callable.
; A1 is data; preserve D2-D7/A2-A6 per CIA resource handler convention.
        SECTION diagnostic,CODE
        XDEF _pt_diagnostic_cia_irq
        XDEF _pt_diagnostic_cia_arm_native
        XDEF _pt_diagnostic_cia_count_native
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
; C stack ABI; payload offsets also checked by C. The task has already stopped
; and masked its owned timer, cleared pending and enabled its owned ICR bit
; under Disable. No foreign MMIO. Actual ReadEClock then bounded count/START;
; no calibrated compensation or target substitution. Zero refuses no START.
_pt_diagnostic_cia_arm_native
        MOVEM.L D2-D7/A2-A6,-(SP)
        MOVE.L 48(SP),A2
        MOVE.L 0(A2),A6
        LEA 4(A2),A0
        JSR -60(A6)
        MOVE.L D0,20(A2)
        CMP.L 24(A2),D0
        BNE.B pt_cia_arm_fail
        BSR.B pt_cia_count_words
        TST.L D0
        BEQ.B pt_cia_arm_fail
        MOVE.L D0,D1
        MOVE.L 28(A2),A0
        MOVE.B D0,(A0)
        LSR.L #8,D0
        MOVE.L 32(A2),A0
        MOVE.B D0,(A0)
        MOVE.L 36(A2),A0
        MOVE.L 40(A2),D0
        MOVE.B D0,(A0)
        MOVE.L D1,44(A2)
        MOVEQ #1,D0
        BRA.B pt_cia_arm_return
pt_cia_arm_fail
        MOVEQ #0,D0
pt_cia_arm_return
        MOVEM.L (SP)+,D2-D7/A2-A6
        RTS
; Native pre-acquisition fixture calls the SAME arithmetic to verify low-word
; borrow, expired/equal, uint64 extremes and 16-bit timer capacity.
_pt_diagnostic_cia_count_native
        MOVEM.L D2/A2,-(SP)
        MOVE.L 12(SP),A2
        BSR.B pt_cia_count_words
        MOVEM.L (SP)+,D2/A2
        RTS
pt_cia_count_words
        MOVE.L 12(A2),D1
        MOVE.L 16(A2),D0
        SUB.L 8(A2),D0
        MOVE.L 4(A2),D2
        SUBX.L D2,D1
        BCS.B pt_cia_count_fail
        TST.L D1
        BNE.B pt_cia_count_fail
        TST.L D0
        BEQ.B pt_cia_count_fail
        CMP.L #65535,D0
        BHI.B pt_cia_count_fail
        RTS
pt_cia_count_fail
        MOVEQ #0,D0
        RTS
        END
