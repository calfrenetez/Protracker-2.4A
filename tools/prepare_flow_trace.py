#!/usr/bin/env python3
"""Separate bounded ISR instrumentation; never linked into the editor."""
from prepare_replay import once, prepare_replay

RECORD_BYTES = 36

def prepare_flow_trace(raw, wrapper):
    source = prepare_replay(raw, wrapper).decode('latin1')
    source = once(source, '\tBEQ.W\tmt_exit', '\tBEQ.W\tpt_flow_restore')
    source = once(source, '\nmt_GetNewNote\n', '\nmt_GetNewNote\n\tADDQ.W #1,_pt_flow_fetches\n')
    source = once(source, 'mt_exit\tMOVEM.L\t(SP)+,D0-A6',
                  'mt_exit\tBSR.W pt_flow_record\npt_flow_restore\tMOVEM.L\t(SP)+,D0-A6')
    source += '''
\tXREF _pt_flow_data,_pt_flow_count,_pt_flow_capacity,_pt_flow_fetches,_pt_flow_limited
pt_flow_record
\tMOVEQ #0,D0
\tMOVE.W _pt_flow_count,D0
\tCMP.W _pt_flow_capacity,D0
\tBHS.W pt_flow_done
\tMULU #36,D0
\tMOVE.L _pt_flow_data,A0
\tADDA.L D0,A0
\tMOVE.L _pt_replay_ticks(PC),(A0)+
\tMOVE.B _pt_replay_order(PC),(A0)+
\tMOVE.B mt_SongPos(PC),(A0)+
\tMOVE.W _pt_replay_rowbytes(PC),(A0)+
\tMOVE.W mt_PatternPos(PC),(A0)+
\tMOVE.B mt_Counter(PC),(A0)+
\tMOVE.B mt_Speed(PC),(A0)+
\tMOVE.W RealTempo(PC),(A0)+
\tMOVE.B mt_Enable(PC),(A0)+
\tMOVE.B mt_PattDelayTime(PC),(A0)+
\tMOVE.B mt_PattDelayTime2(PC),(A0)+
\tMOVE.B mt_PBreakPos(PC),(A0)+
\tMOVE.B mt_PosJumpFlag(PC),(A0)+
\tMOVE.B mt_PBreakFlag(PC),(A0)+
\tMOVE.B mt_audchan1temp+n_pattpos(PC),(A0)+
\tMOVE.B mt_audchan2temp+n_pattpos(PC),(A0)+
\tMOVE.B mt_audchan3temp+n_pattpos(PC),(A0)+
\tMOVE.B mt_audchan4temp+n_pattpos(PC),(A0)+
\tMOVE.B mt_audchan1temp+n_loopcount(PC),(A0)+
\tMOVE.B mt_audchan2temp+n_loopcount(PC),(A0)+
\tMOVE.B mt_audchan3temp+n_loopcount(PC),(A0)+
\tMOVE.B mt_audchan4temp+n_loopcount(PC),(A0)+
\tMOVE.W _pt_flow_fetches,(A0)+
\tMOVE.W mt_DMACONtemp(PC),(A0)+
\tMOVE.L _pt_replay_rawvol(PC),(A0)+
\t; Publishing the count last makes a complete record visible to the task.
\tADDQ.W #1,_pt_flow_count
\tMOVE.W _pt_flow_count,D0
\tCMP.W _pt_flow_capacity,D0
\tBLO.B pt_flow_done
\t; Stop at the exact budget, but preserve a simultaneous native F00 stop.
\tTST.B mt_Enable
\tBEQ.B pt_flow_done
\tMOVE.W #1,_pt_flow_limited
\tSF mt_Enable
pt_flow_done
\tRTS
'''
    return source.encode('latin1')
