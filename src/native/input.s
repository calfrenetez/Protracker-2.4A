; Native input.device integration. All button polling keeps its original
; active-low bit semantics, backed by RAWMOUSE transitions rather than CIA/POT.
; Called only while the tracker screen is active. No DOS or allocation here.

	SECTION PTGInput,CODE
PTGMouseButtons
	MOVE.W 6(A1),D0
	CMPI.W #$68,D0
	BEQ.B .leftDown
	CMPI.W #$e8,D0
	BEQ.B .leftUp
	CMPI.W #$69,D0
	BEQ.B .rightDown
	CMPI.W #$e9,D0
	BEQ.B .rightUp
	RTS
.leftDown
	CLR.B PTGLeftButton
	RTS
.leftUp
	MOVE.B #$40,PTGLeftButton
	RTS
.rightDown
	CLR.B PTGRightButton
	RTS
.rightUp
	MOVE.B #4,PTGRightButton
	RTS

; A1 = InputEvent, returns D0=1 if consumed, 0 to pass through unchanged.
; Only absolute PIXEL events addressed to this screen are intercepted.
; TABLET/NEWTABLET and relative PIXEL events retain OS handling.
PTGPointer
	CMPI.B #1,5(A1)
	BNE.B .pass
	BTST #7,8(A1)	; IEQUALIFIER_RELATIVEMOUSE = $8000
	BNE.B .pass
	MOVE.L 10(A1),D0
	BEQ.B .pass
	MOVE.L D0,A3
	MOVE.L (A3),D0
	CMP.L PTScreenHandle,D0
	BNE.B .pass
	TST.B DiskDriveBusy
	BNE.B .consumed
	MOVE.W 4(A3),D0
	BPL.B .xPositive
	MOVEQ #0,D0
.xPositive
	CMPI.W #319,D0
	BLE.B .xReady
	MOVE.W #319,D0
.xReady
	SWAP D0
	MOVE.W 6(A3),D0
	BPL.B .yPositive
	CLR.W D0
.yPositive
	CMPI.W #255,D0
	BLE.B .yReady
	MOVE.W #255,D0
.yReady
	MOVE.L D0,MouseX
	CLR.W MouseXFrac
	CLR.W MouseYFrac
.consumed
	MOVEQ #1,D0
	RTS
.pass
	MOVEQ #0,D0
	RTS
	SECTION PTGInputData,DATA
PTGLeftButton dc.b $40
PTGRightButton dc.b 4
	EVEN
