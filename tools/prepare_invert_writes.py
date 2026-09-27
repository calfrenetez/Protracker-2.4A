"""Bounded diagnostic-only EFx write events, independent of sample loop length."""
from prepare_replay import once
from prepare_sample_trace import prepare_sample_trace
RECORD_BYTES=208

def prepare_invert_writes(raw,wrapper):
    source=prepare_sample_trace(raw,wrapper).decode('latin1')
    source=once(source,'\tMULU #140,D0','\tMULU #208,D0')
    source=once(source,'\tCLR.L _pt_replay_ticks','\tBSR.W pt_inv_writes_clear\n\tCLR.L _pt_replay_ticks')
    source=once(source,'\tMOVE.B\tD0,(A0)\nmt_funkend','\tMOVE.B\tD0,(A0)\n\tBSR.W pt_inv_write\nmt_funkend')
    source=once(source,'\t; Publishing the count last','''
\tLEA pt_inv_writes(PC),A1
\tMOVEQ #16,D1
pt_inv_writes_copy
\tMOVE.L (A1),(A0)+
\tCLR.L (A1)+
\tDBRA D1,pt_inv_writes_copy
\t; Publishing the count last''')
    source+='''
; Preserve registers and condition codes across the diagnostic write hook.
; Header: count word, overflow word; eight entries: address long, value byte,
; Paula channel mask byte, reserved word. Never allocate inside the ISR.
pt_inv_write
\tMOVE.W SR,-(SP)
\tMOVEM.L D0-D2/A1,-(SP)
\tMOVEQ #0,D1
\tMOVE.W pt_inv_writes(PC),D1
\tCMP.W #8,D1
\tBHS.B pt_inv_write_overflow
\tLSL.W #3,D1
\tLEA pt_inv_writes+4(PC),A1
\tADDA.W D1,A1
\tMOVE.B (A0),4(A1)
\tMOVE.W n_dmabit(A6),D2
\tMOVE.B D2,5(A1)
\tCLR.W 6(A1)
\tMOVE.L A0,D0
\tBSR.W pt_sample_relative
\tMOVE.L D0,(A1)
\tADDQ.W #1,pt_inv_writes
\tBRA.B pt_inv_write_done
pt_inv_write_overflow
\tMOVE.W #1,pt_inv_writes+2
pt_inv_write_done
\tMOVEM.L (SP)+,D0-D2/A1
\tMOVE.W (SP)+,CCR
\tRTS
pt_inv_writes_clear
\tMOVEM.L D0/A0,-(SP)
\tLEA pt_inv_writes(PC),A0
\tMOVEQ #16,D0
pt_inv_writes_zero
\tCLR.L (A0)+
\tDBRA D0,pt_inv_writes_zero
\tMOVEM.L (SP)+,D0/A0
\tRTS
\tEVEN
pt_inv_writes dcb.l 17,0
'''
    return source.encode('latin1')
