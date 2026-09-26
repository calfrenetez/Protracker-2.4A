"""Separate EFx byte snapshots; shipping replay and earlier trace formats unchanged."""
from prepare_replay import once
from prepare_sample_trace import prepare_sample_trace
RECORD_BYTES=164

def prepare_invert_trace(raw,wrapper):
    source=prepare_sample_trace(raw,wrapper).decode('latin1')
    source=once(source,'\tMULU #140,D0','\tMULU #164,D0')
    record='''
\tMOVE.L mt_audchan1temp+n_wavestart(PC),D0
\tBSR.W pt_sample_relative
\tMOVE.L D0,(A0)+
\tMOVE.B mt_audchan1temp+n_glissfunk(PC),(A0)+
\tMOVE.B mt_audchan1temp+n_funkoffset(PC),(A0)+
\tCLR.W (A0)+
\tMOVE.L mt_audchan1temp+n_loopstart(PC),D0
\tBEQ.B pt_inv_empty
\tCMP.W #1,mt_audchan1temp+n_replen
\tBEQ.B pt_inv_word
\tCMP.W #8,mt_audchan1temp+n_replen
\tBLO.B pt_inv_empty
\tMOVE.L D0,A1
\tMOVEQ #15,D1
pt_inv_copy
\tMOVE.B (A1)+,(A0)+
\tDBRA D1,pt_inv_copy
\tBRA.B pt_inv_done
pt_inv_word
\tMOVE.L D0,A1
\tMOVE.W (A1),(A0)+
\tCLR.W (A0)+
\tCLR.L (A0)+
\tCLR.L (A0)+
\tCLR.L (A0)+
\tBRA.B pt_inv_done
pt_inv_empty
\tCLR.L (A0)+
\tCLR.L (A0)+
\tCLR.L (A0)+
\tCLR.L (A0)+
pt_inv_done
'''
    source=once(source,'\t; Publishing the count last',record+'\t; Publishing the count last')
    return source.encode('latin1')
