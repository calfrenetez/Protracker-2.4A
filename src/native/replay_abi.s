; C ABI, caller owns all four Paula channels and serialized Chip RAM MOD.
; Engine disabled throughout setup. All persistent state reset on each start.
	EVEN
	XDEF _pt_replay_start,_pt_replay_stop
	XDEF _pt_replay_data,_pt_replay_ticks,_pt_replay_order,_pt_replay_rowbytes
	XDEF _pt_replay_audible,_pt_replay_rawvol,_pt_replay_outputvol
	XDEF _pt_replay_voices,_pt_replay_speed,_pt_replay_tempo,_pt_replay_enabled
_pt_replay_enabled EQU mt_Enable
_pt_replay_voices EQU mt_audchan1temp
_pt_replay_speed EQU mt_Speed
_pt_replay_tempo EQU RealTempo
_pt_replay_start
	MOVEM.L D2-D7/A2-A6,-(SP)
	MOVE.L 48(SP),_pt_replay_data
	MOVE.L 60(SP),D0
	MOVE.W D0,_pt_replay_audible
	CLR.L _pt_replay_outputvol
	CLR.L _pt_replay_rawvol
	CLR.L _pt_replay_ticks
	CLR.B _pt_replay_order
	CLR.W _pt_replay_rowbytes
	LEA mt_audchan1temp(PC),A0
	MOVE.W #43,D0
pt_clearvoices
	CLR.L (A0)+
	DBRA D0,pt_clearvoices
	; An instrument-zero note before any sample must never read address zero.
	MOVE.L 56(SP),A1
	LEA mt_audchan1temp(PC),A0
	MOVEQ #3,D0
pt_init_silence
	MOVE.L A1,n_start(A0)
	MOVE.L A1,n_loopstart(A0)
	MOVE.L A1,n_wavestart(A0)
	MOVE.W #1,n_length(A0)
	MOVE.W #1,n_replen(A0)
	LEA 44(A0),A0
	DBRA D0,pt_init_silence
	MOVE.W #1,mt_audchan1temp+n_dmabit
	MOVE.W #2,mt_audchan2temp+n_dmabit
	MOVE.W #4,mt_audchan3temp+n_dmabit
	MOVE.W #8,mt_audchan4temp+n_dmabit
	; Period lookup must be valid even for effects before the first instrument.
	LEA mt_ftune0(PC),A0
	MOVE.L A0,mt_audchan1temp+n_peroffset
	MOVE.L A0,mt_audchan2temp+n_peroffset
	MOVE.L A0,mt_audchan3temp+n_peroffset
	MOVE.L A0,mt_audchan4temp+n_peroffset
	CLR.W mt_DMACONtemp
	CLR.B mt_PBreakPos
	CLR.B mt_PosJumpFlag
	CLR.B mt_PBreakFlag
	MOVE.B #$FF,mt_LowMask
	SF mt_Enable
	BSR.W SetCIAInt
	TST.L CIAAbase
	BEQ.B pt_startfailed
	BSR.W mt_init
	; Resolve empty samples after mt_init has calculated real data offsets.
	; AUDxLEN=0 means 65536 words, so publish the owned two-byte guard instead.
	MOVE.L _pt_replay_data(PC),A0
	LEA 42(A0),A0
	LEA mt_SampleStarts(PC),A1
	MOVE.L 56(SP),D1
	MOVEQ #30,D0
pt_empty_samples
	TST.W (A0)
	BNE.B pt_sample_present
	MOVE.W #1,(A0)
	CLR.W 4(A0)
	MOVE.W #1,6(A0)
	MOVE.L D1,(A1)
pt_sample_present
	ADDQ.L #4,A1
	LEA 30(A0),A0
	DBRA D0,pt_empty_samples
	MOVE.L 52(SP),D0
	MOVE.B D0,mt_SongPos
	MOVE.B D0,_pt_replay_order
	ST mt_Enable
	MOVEQ #1,D0
	BRA.B pt_startdone
pt_startfailed
	MOVEQ #0,D0
pt_startdone
	MOVEM.L (SP)+,D2-D7/A2-A6
	RTS
_pt_replay_stop
	MOVEM.L D2-D7/A2-A6,-(SP)
	SF mt_Enable
	BSR.W ResetCIAInt
	BSR.W mt_end
	MOVEM.L (SP)+,D2-D7/A2-A6
	RTS
	; Preserve registers and the MOVE.W condition codes at each replaced write.
	; Raw volume retains tremolo/slide/cut output even while inaudible.
pt_write_volume
	MOVEM.L D0-D1/A0,-(SP)
	MOVE.L A5,A0
	SUBA.L #$DFF0A0,A0
	MOVE.L A0,D1
	ROR.L #4,D1
	LEA _pt_replay_rawvol(PC),A0
	MOVE.B D0,0(A0,D1.W)
	LEA _pt_replay_outputvol(PC),A0
	ADDA.W D1,A0
	MOVE.W n_dmabit(A6),D1
	AND.W _pt_replay_audible(PC),D1
	BNE.B pt_volume_audible
	MOVEQ #0,D0
pt_volume_audible
	MOVE.B D0,(A0)
	MOVE.W D0,8(A5)
	MOVEM.L (SP)+,D0-D1/A0
	TST.W D0
	RTS
	CNOP 0,4
_pt_replay_outputvol dc.l 0
_pt_replay_rawvol dc.l 0
_pt_replay_audible dc.w 15
	CNOP 0,4
_pt_replay_data dc.l 0
_pt_replay_ticks dc.l 0
_pt_replay_rowbytes dc.w 0
_pt_replay_order dc.b 0
	EVEN
