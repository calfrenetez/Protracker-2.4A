"""76-byte diagnostic: existing output-volume prefix plus stored volume/state."""
from prepare_replay import once
from prepare_pitch_trace import prepare_pitch_trace
RECORD_BYTES=76

def prepare_volume_trace(raw,wrapper):
    source=prepare_pitch_trace(raw,wrapper).decode('latin1')
    source=once(source,'\tMULU #52,D0','\tMULU #76,D0')
    # Existing pt_write_volume already captures every final D0 write in the
    # prefix at 32..35, before mute gating. Do not wrap its six callers again.
    extra=''
    for ch in range(1,5):
        extra+=f'\tMOVEQ #0,D0\n\tMOVE.B mt_audchan{ch}temp+n_volume(PC),D0\n\tMOVE.W D0,(A0)+\n'
    for ch in range(1,5):
        for field in ['n_tremolocmd','n_tremolopos','n_wavecontrol','n_vibratopos']:
            extra+=f'\tMOVE.B mt_audchan{ch}temp+{field}(PC),(A0)+\n'
    source=once(source,'\t; Publishing the count last',extra+'\t; Publishing the count last')
    return source.encode('latin1')
