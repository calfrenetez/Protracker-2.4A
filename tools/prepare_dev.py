"""Fail-closed edits to the generated native source, never the vendor copy."""
from prepare_asm import prepare


def prepare_dev(raw, guard, input_source):
    source, counts = prepare(raw)
    def replace(old, new):
        nonlocal source
        if source.count(old) != 1:
            raise ValueError('Expected one native anchor: ' + repr(old[:80]))
        source = source.replace(old, new)
    replace(b"rb_Progname\tdc.b\t'PT2.3F',0", b"rb_Progname\tdc.b\t'PT2.4G',0")
    replace(b"VersionText\tdc.b\t'ProTracker v2.3F',0", b"VersionText\tdc.b\t'ProTracker v2.4G',0")
    replace(b'dc.b "ProTracker 2.3F",0', b'dc.b "ProTracker 2.4G",0')
    replace(b'\nLoadModule\n\tCLR.W\tOutOfMemoryFlag',
            b'\nLoadModule\n\tJSR PTGPreflight\n\tTST.L D0\n\tBEQ.B PTGLoadValid\n\tRTS\n'
            b'PTGLoadValid\n\tCLR.W\tOutOfMemoryFlag')
    replace(b"\tCMP.L\t#'PX20',(A0)\n\tBEQ.W\tUnpackPPFile",
            b"\tCMP.L\t#'PX20',(A0)\n\tBEQ.W\tUnpackPPFile\n"
            b'\tCMPI.L #1084,D0\n\tBEQ.B *+8\n\tJMP PTGReadFailed')
    replace(b'.L3\tMULU.W\t#64*4*4,D3\n\tMOVE.L\tFileHandle(PC),D1',
            b'.L3\tMULU.W\t#64*4*4,D3\n'
            b'\tMOVE.L SongAllocSize,D0\n\tSUBI.L #1084,D0\n'
            b'\tCMP.L D0,D3\n\tBLS.B *+8\n\tJMP PTGReadFailed\n'
            b'\tMOVE.L\tFileHandle(PC),D1')
    replace(b'\tJSR\t_LVORead(A6)\n\tMOVE.L\tSongDataPtr(PC),A0\n'
            b"\tMOVE.L\t#'M.K.',sd_magicid(A0)",
            b'\tJSR\t_LVORead(A6)\n\tCMP.L D3,D0\n\tBEQ.B *+8\n\tJMP PTGReadFailed\n'
            b'\tMOVE.L\tSongDataPtr(PC),A0\n'
            b"\tMOVE.L\t#'M.K.',sd_magicid(A0)")
    replace(b"\tMOVE.L\tD6,D3\t\t; read length\n\tJSR\t_LVORead(A6)",
            b"\tMOVE.L\tD6,D3\t\t; read length\n\tJSR\t_LVORead(A6)\n"
            b'\tCMP.L D6,D0\n\tBEQ.B *+8\n\tJMP PTGReadFailed')
    # Every GUI polling site uses the same active-low event state. Keep all
    # unrelated CIA/filter/LED accesses untouched and fail on baseline drift.
    for old, new, expected in [
        (b'BTST\t#6,$BFE001', b'BTST\t#6,PTGLeftButton', 29),
        (b'BTST\t#2,$DFF016', b'BTST\t#2,PTGRightButton', 81),
        (b'BTST.B\t#10-8,$DFF016', b'BTST.B\t#2,PTGRightButton', 2),
    ]:
        if source.count(old) != expected:
            raise ValueError('Unexpected mouse polling site count: ' + repr(old))
        source = source.replace(old, new)
    replace(b'\tSF\tStopInputFlag\n', b'\tSF\tStopInputFlag\n'
            b'\tMOVE.B #$40,PTGLeftButton\n\tMOVE.B #4,PTGRightButton\n')
    replace(b'\tBEQ.B\tInpRawmouse\n\tMOVE.L\tA1,A2',
            b'\tBEQ.B\tInpRawmouse\n\tCMPI.B #$13,D0\n\tBNE.B PTGInputPass\n'
            b'\tJSR PTGPointer\n\tTST.L D0\n\tBEQ.B PTGInputPass\n'
            b'\tBSR.W InpUnchain\n\tBRA.W InpNext\nPTGInputPass\n\tMOVE.L\tA1,A2')
    replace(b'InpRawmouse\n\tBSR.B\tInpUnchain',
            b'InpRawmouse\n\tBSR.W\tInpUnchain\n\tJSR PTGMouseButtons')
    replace(b'\nEND\n', b'\n' + guard + b'\n' + input_source + b'\nEND\n')
    return source, counts
