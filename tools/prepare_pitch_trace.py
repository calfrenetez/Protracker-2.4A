"""Separate pitch-register diagnostic; leaves shipping and dev28 trace objects intact."""
import re
from prepare_replay import once
from prepare_flow_trace import prepare_flow_trace
RECORD_BYTES=52

def prepare_pitch_trace(raw,wrapper):
    source=prepare_flow_trace(raw,wrapper).decode('latin1')
    source=once(source,'\tMULU #36,D0','\tMULU #52,D0')
    source=once(source,'\tCLR.L _pt_replay_ticks','\tCLR.L pt_pitch_output\n\tCLR.L pt_pitch_output+4\n\tCLR.L _pt_replay_ticks')
    pattern=r'MOVE\.W[ \t]+([^\n]+?),6\(A5\)'
    writes=list(re.finditer(pattern,source));assert len(writes)==10,len(writes)
    routines=[]
    def replace(match):
        name='pt_pitch_write'+str(len(routines));operand=match.group(1)
        routines.append(f'''
{name}
\tMOVEM.L D0-D1/A0,-(SP)
\tMOVE.W {operand},D0
\tMOVE.L A5,A0
\tSUBA.L #$DFF0A0,A0
\tMOVE.L A0,D1
\tROR.L #3,D1
\tLEA pt_pitch_output(PC),A0
\tMOVE.W D0,0(A0,D1.W)
\tMOVE.W D0,pt_pitch_last
\tMOVE.W D0,6(A5)
\tMOVEM.L (SP)+,D0-D1/A0
\tTST.W pt_pitch_last
\tRTS
''')
        return 'BSR.W '+name
    source=re.sub(pattern,replace,source)
    extra=''.join(f'\tMOVE.W mt_audchan{ch}temp+n_period(PC),(A0)+\n' for ch in range(1,5))
    extra+='\tMOVE.L pt_pitch_output(PC),(A0)+\n\tMOVE.L pt_pitch_output+4(PC),(A0)+\n'
    source=once(source,'\t; Publishing the count last',extra+'\t; Publishing the count last')
    source+='\n'+''.join(routines)+'\n\tEVEN\npt_pitch_output dc.l 0,0\npt_pitch_last dc.w 0\n'
    return source.encode('latin1')
