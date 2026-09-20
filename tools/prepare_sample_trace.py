"""Separate sample-range diagnostic, extending the immutable 52-byte pitch schema."""
import re
from prepare_replay import once
from prepare_pitch_trace import prepare_pitch_trace
RECORD_BYTES=140

def prepare_sample_trace(raw,wrapper):
    source=prepare_pitch_trace(raw,wrapper).decode('latin1')
    source=once(source,'\tMULU #52,D0','\tMULU #140,D0')
    clear=''.join(f'\tCLR.L pt_sample_trigger+{n}\n' for n in range(0,32,4))
    source=once(source,'\tCLR.L _pt_replay_ticks',clear+'\tCLR.L _pt_replay_ticks')
    source=once(source,'\tMOVE.L 56(SP),A1','\tMOVE.L 56(SP),A1\n\tMOVE.L A1,pt_sample_silence')
    pattern=r'MOVE\.L[ \t]+n_start\(A6\),\(A5\)'
    assert len(re.findall(pattern,source))==2
    source=re.sub(pattern,'BSR.W pt_sample_write',source)
    record=''
    for ch in range(4):
        voice=f'mt_audchan{ch+1}temp'
        for field,size in [('n_start','L'),('n_length','W'),('n_loopstart','L'),('n_replen','W'),('n_sampleoffset','B')]:
            if size=='L':record+=f'\tMOVE.L {voice}+{field}(PC),D0\n\tBSR.W pt_sample_relative\n\tMOVE.L D0,(A0)+\n'
            else:record+=f'\tMOVE.{size} {voice}+{field}(PC),(A0)+\n'
        record+='\tCLR.B (A0)+\n'
        record+=f'\tMOVE.L pt_sample_trigger+{ch*8}(PC),D0\n\tBSR.W pt_sample_relative\n\tMOVE.L D0,(A0)+\n\tMOVE.L pt_sample_trigger+{ch*8+4}(PC),(A0)+\n'
    source=once(source,'\t; Publishing the count last',record+'\t; Publishing the count last')
    source+='''
pt_sample_write
\tMOVEM.L D0-D1/A0-A1,-(SP)
\tMOVE.L A5,A0
\tSUBA.L #$DFF0A0,A0
\tMOVE.L A0,D1
\tROR.L #1,D1
\tLEA pt_sample_trigger(PC),A0
\tADDA.W D1,A0
\tMOVE.L n_start(A6),D0
\tMOVE.L D0,(A0)
\tMOVE.W n_length(A6),4(A0)
\tMOVE.W 6(A0),D1
\tMOVE.L D1,A1
\tLEA 1(A1),A1
\tMOVE.W A1,6(A0)
\tMOVE.L D0,pt_sample_last
\tMOVE.L D0,(A5)
\tMOVEM.L (SP)+,D0-D1/A0-A1
\tTST.L pt_sample_last
\tRTS
pt_sample_relative
\tTST.L D0
\tBEQ.B pt_sample_guard
\tCMP.L pt_sample_silence(PC),D0
\tBEQ.B pt_sample_guard
\tSUB.L _pt_replay_data(PC),D0
\tRTS
pt_sample_guard
\tMOVEQ #-1,D0
\tRTS
\tEVEN
pt_sample_trigger dc.l 0,0,0,0,0,0,0,0
pt_sample_silence dc.l 0
pt_sample_last dc.l 0
'''
    return source.encode('latin1')
