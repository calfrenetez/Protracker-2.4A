; Standalone CLI harness for the SAME mod_guard.s used in PT2.4G.
; Only UI text/cursor routines are stubbed. DOS file reads/seeks are real.
	SECTION Harness,CODE
Start
	MOVEM.L D2-D7/A2-A6,-(SP)
	MOVE.L 4.W,A6
	LEA DosName,A1
	MOVEQ #0,D0
	JSR -552(A6)
	MOVE.L D0,DOSBase
	BEQ.W .failed
	LEA Cases,A4
	MOVEQ #0,D6
	MOVEQ #0,D7
.case
	LEA DirInputName,A1
	MOVE.L A4,A0
	MOVEQ #29,D0
.copy	MOVE.B (A0)+,(A1)+
	DBRA D0,.copy
	JSR PTGPreflight
	CMP.W 30(A4),D0
	BEQ.B .pass
	ADDQ.L #1,D7
	MOVE.L #'FAIL',Result
	BRA.B .report
.pass	MOVE.L #'PASS',Result
.report
	MOVEQ #0,D0
	MOVE.W D6,D0
	DIVU.W #10,D0
	ADDI.B #'0',D0
	MOVE.B D0,Number+1
	SWAP D0
	ADDI.B #'0',D0
	MOVE.B D0,Number+2
	MOVE.L DOSBase,A6
	JSR -60(A6)
	MOVE.L D0,D1
	MOVE.L #Message,D2
	MOVE.L #MessageEnd-Message,D3
	JSR -48(A6)
	ADDQ.W #1,D6
	LEA 32(A4),A4
	CMP.W #CaseCount,D6
	BLO.B .case
	MOVE.L DOSBase,A1
	MOVE.L 4.W,A6
	JSR -414(A6)
	TST.L D7
	BNE.B .failed
	MOVEQ #0,D0
	BRA.B .done
.failed	MOVEQ #20,D0
.done	MOVEM.L (SP)+,D2-D7/A2-A6
	RTS

CopyPath
	LEA FileName,A1
.copy	MOVE.B (A0)+,(A1)+
	BNE.B .copy
	SUBQ.L #1,A1
	RTS
ShowStatusText
SetErrorPtrCol
rmiend	RTS

_LVOOpen EQU -30
_LVOClose EQU -36
_LVORead EQU -42
_LVOSeek EQU -66
DirNameLength EQU 30
ERR_WAIT_TIME EQU 40
DOSBase dc.l 0
DosName dc.b 'dos.library',0
ModulesPath2 dc.b 'PTDEV:guard/',0
	EVEN
Message dc.b 'GUARD case='
Number dc.b '000 result='
Result dc.b 'PASS',10
MessageEnd
	EVEN
DirInputName ds.b 30
FileName ds.b 96
WaitTime ds.w 1
; Build tool appends aligned Cases, the exact guard source and END.
