; ProTracker 2.4G development: validate ordinary MODs before DoClearSong.
; Original vendor source remains unchanged. All instructions are 68000-safe.
; PP20/PX20 and legacy 15-sample interpretation stay in the original loader.

	SECTION PTGGuard,CODE

PTGPreflight
	MOVEM.L D1-D7/A0-A6,-(SP)
	LEA ModulesPath2,A0
	JSR CopyPath
	LEA DirInputName,A0
	MOVEQ #DirNameLength-1,D0
.path	MOVE.B (A0)+,(A1)+
	DBRA D0,.path
	LEA PTGBadRead,A5
	MOVE.L DOSBase,A6
	MOVE.L #FileName,D1
	MOVE.L #1005,D2
	JSR _LVOOpen(A6)
	MOVE.L D0,D4
	BEQ.W .error
	MOVE.L D4,D1
	MOVE.L #PTGHeader,D2
	MOVE.L #1084,D3
	JSR _LVORead(A6)
	CMPI.L #4,D0
	BLT.W .closeError
	LEA PTGHeader,A2
	CMPI.L #'PP20',(A2)
	BEQ.W .closeOK
	CMPI.L #'PX20',(A2)
	BEQ.W .closeOK
	CMPI.L #1084,D0
	BNE.W .closeError
	CMPI.L #'M.K.',1080(A2)
	BEQ.B .classic
	CMPI.L #'M!K!',1080(A2)
	BNE.W .closeOK
.classic
	LEA PTGBadOrder,A5
	TST.B 950(A2)
	BEQ.W .closeError
	CMPI.B #128,950(A2)
	BHI.W .closeError
	LEA 952(A2),A0
	MOVEQ #127,D0
	MOVEQ #0,D5
.orders	MOVEQ #0,D1
	MOVE.B (A0)+,D1
	CMPI.W #99,D1
	BHI.W .closeError
	CMP.W D1,D5
	BHS.B .nextOrder
	MOVE.W D1,D5
.nextOrder
	DBRA D0,.orders
	ADDQ.W #1,D5
	MOVE.L D5,D6
	MULU.W #1024,D6
	ADDI.L #1084,D6
	LEA 42(A2),A0
	MOVEQ #30,D0
.samples
	MOVEQ #0,D1
	MOVE.W (A0),D1
	ADD.L D1,D1
	ADD.L D1,D6
	LEA 30(A0),A0
	DBRA D0,.samples
	LEA PTGBadRead,A5
	SUBQ.W #1,D5
.patterns
	MOVE.L D4,D1
	MOVE.L #PTGPattern,D2
	MOVE.L #1024,D3
	JSR _LVORead(A6)
	CMPI.L #1024,D0
	BNE.B .closeError
	LEA PTGBadInstrument,A5
	LEA PTGPattern,A0
	MOVE.W #255,D0
.events	MOVE.B (A0),D1
	ANDI.B #$E0,D1
	BNE.B .closeError
	ADDQ.L #4,A0
	DBRA D0,.events
	LEA PTGBadRead,A5
	DBRA D5,.patterns
	; Seek returns OLD position. Seek to EOF, then query current position.
	MOVE.L D4,D1
	MOVEQ #0,D2
	MOVEQ #1,D3
	JSR _LVOSeek(A6)
	TST.L D0
	BMI.B .closeError
	MOVE.L D4,D1
	MOVEQ #0,D2
	MOVEQ #0,D3
	JSR _LVOSeek(A6)
	TST.L D0
	BMI.B .closeError
	CMP.L D6,D0
	BLO.B .closeError
.closeOK
	MOVE.L D4,D1
	JSR _LVOClose(A6)
	MOVEQ #0,D0
	BRA.B .done
.closeError
	MOVE.L D4,D1
	JSR _LVOClose(A6)
.error	MOVE.L A5,A0
	JSR ShowStatusText
	JSR SetErrorPtrCol
	MOVE.W #ERR_WAIT_TIME,WaitTime
	MOVEQ #-1,D0
.done	MOVEM.L (SP)+,D1-D7/A0-A6
	RTS

; Defense in depth for short reads or changed files after preflight. This
; path safely ends the load, but full rollback of late I/O/OOM is future work.
PTGReadFailed
	JSR rmiend
	LEA PTGBadRead,A0
	JSR ShowStatusText
	JSR SetErrorPtrCol
	MOVE.W #ERR_WAIT_TIME,WaitTime
	RTS

PTGBadRead	dc.b 'Module read error',0
PTGBadOrder	dc.b 'Invalid orders!',0
PTGBadInstrument dc.b 'Invalid sample!',0
	EVEN
PTGVersion	dc.b 0,'$VER: ProTracker 2.4G dev2 (19.09.2026)',0
	EVEN
	SECTION PTGGuardData,BSS
PTGHeader	ds.b 1084
PTGPattern	ds.b 1024
