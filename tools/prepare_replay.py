#!/usr/bin/env python3
"""Build an ABI adapter from the pinned CIA replay; never edit vendor input."""
from pathlib import Path
from prepare_asm import prepare

def once(source, old, new):
    if source.count(old) != 1:
        raise ValueError('Replay anchor changed: '+repr(old[:70]))
    return source.replace(old, new, 1)

def prepare_replay(raw, wrapper):
    source = raw.decode('latin1')
    start = source.index('\nmain\t')
    end = source.index(';---- CIA Interrupt ----')
    source = source[:start]+'\n\tSECTION replay,CODE\n'+source[end:]
    source = once(source, '\tBEQ\tResetCIAInt', '\tBEQ\tPTCIAFailed')
    source = once(source, '\tCLR.L\tCIAAbase\n\tRTS\n\nResetCIAInt', 'PTCIAFailed\n\tCLR.L\tCIAAbase\n\tRTS\n\nResetCIAInt')
    source = once(source, 'RemInt\tLEA\tMusicIntServer(PC),A1\n\tMOVEQ\t#0,d0', 'RemInt\tLEA\tMusicIntServer(PC),A1')
    # Clear inherited one-shot/CNT modes; retain unrelated TOD/serial control.
    for register, mask in [('ciacra', '$C0'), ('ciacrb', '$80')]:
        source = once(source, '\tBSET\t#0,'+register+'(A5)',
                      '\tMOVE.B\t'+register+'(A5),D0\n\tANDI.B\t#'+mask+',D0\n\tORI.B\t#$11,D0\n\tMOVE.B\tD0,'+register+'(A5)')
    source = once(source, '\tLEA\tmt_data,A0', '\tMOVE.L\t_pt_replay_data(PC),A0')
    # Keep the reference first-word/loop initialization, but load each DMA
    # pointer from caller-owned Chip buffers rather than contiguous MOD PCM.
    source = once(source, '\tLEA\tmt_SampleStarts(PC),A1\n\tMOVEQ\t#31-1,D3',
                  '\tLEA\tmt_SampleStarts(PC),A1\n\tMOVE.L\t_pt_replay_samples(PC),A3\n\tMOVEQ\t#31-1,D3')
    source = once(source, 'mtloop3\tMOVEQ\t#0,D0', 'mtloop3\tMOVE.L\t(A3)+,A2\n\tMOVEQ\t#0,D0')
    source = once(source, '\nmt_GetNewNote\n', '\nmt_GetNewNote\n\tMOVE.B\tmt_SongPos(PC),_pt_replay_order\n\tMOVE.W\tmt_PatternPos(PC),_pt_replay_rowbytes\n')
    source = once(source, '\tADDQ.B\t#1,mt_Counter', '\tADDQ.L\t#1,_pt_replay_ticks\n\tADDQ.B\t#1,mt_Counter')
    # Preserve all effect state; gate only final hardware volume writes.
    volume = '\tMOVE.W\tD0,8(A5)'
    if source.count(volume) != 6:
        raise ValueError('Replay volume anchors changed')
    source = source.replace(volume, '\tBSR.W\tpt_write_volume')
    dma = '\tMOVE.W\tD0,$DFF096'
    if source.count(dma) != 2:
        raise ValueError('Replay DMA trigger anchors changed')
    source = source.replace(dma, '\tBSR.W\tpt_start_dma')
    source = once(source, '\tSECTION music,DATA_C\n\n\tCNOP 0,4\nmt_data INCBIN "music.mod"', wrapper.decode('ascii'))
    return prepare(source.encode('latin1'))[0]

if __name__ == '__main__':
    root=Path(__file__).resolve().parents[1]
    (root/'build/dev/replay.s').write_bytes(prepare_replay((root/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_bytes(),(root/'src/native/replay_abi.s').read_bytes()))
