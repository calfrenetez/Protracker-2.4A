"""Offline diagnostic parsing, never target access or qualification by itself."""
import re
import traceback

MAX_LOG_BYTES=65536
BEGIN=b'PTFIXTURE BEGIN MASTER_ESTABLISH_PROGRESS_V1'
END=b'PTFIXTURE END MASTER_ESTABLISH_PROGRESS_V1 0'
LINE=re.compile(rb'PTPROGRESS ([0-9]+) (OWNER|MASTER|FAULT) (BEGIN|END) ([0-9]+) ([0-9]+)')

def expected_cases():
    cases=[]
    for modes in (range(56),range(58,80),range(81,83),range(83,88)):
        cases.extend(('OWNER',bits,mode) for bits in (8,16,24) for mode in modes)
    for bits in (8,16,24):
        cases.extend(('MASTER',bits,channels) for channels in (1,2))
        cases.extend(('FAULT',bits,mode) for mode in range(27))
    return cases

def progress_snapshot(data,complete=False):
    """Accept an exact finite prefix; refuse gaps, duplicate starts and fake END.
    File bytes/host observation time do not prove target elapsed time or quiet.
    A partial trailing line may be observed only while complete=False.
    """
    if len(data)>MAX_LOG_BYTES:raise ValueError('fixture log exceeds bounded size')
    if complete and not data.endswith(b'\n'):raise ValueError('incomplete final line')
    lines=data.split(b'\n')[:-1]
    records=[(group,phase,bits,mode) for group,bits,mode in expected_cases() for phase in ('BEGIN','END')]
    seen=0;begun=ended=False;last=None
    for line in lines:
        if line==BEGIN:
            if begun or seen or ended:raise ValueError('duplicate/misplaced fixture begin')
            begun=True
        elif line.startswith(b'PTPROGRESS'):
            match=LINE.fullmatch(line)
            if not begun or ended or not match or seen>=len(records):raise ValueError('invalid progress record')
            sequence,group,phase,bits,mode=match.groups()
            actual=(group.decode('ascii'),phase.decode('ascii'),int(bits),int(mode))
            if int(sequence)!=seen+1 or actual!=records[seen]:raise ValueError('progress sequence/workload mismatch')
            seen+=1;last=actual
        elif line.startswith(b'PTFIXTURE END'):
            if line!=END or not begun or ended or seen!=len(records):raise ValueError('premature/invalid fixture end')
            ended=True
        elif line.startswith(b'PTFIXTURE'):raise ValueError('unknown fixture marker')
    if complete and not ended:raise ValueError('missing complete fixture markers')
    return {'begun':begun,'ended':ended,'progressRecords':seen,'completedCases':seen//2,
            'totalCases':len(records)//2,'lastRecord':last,'bytes':len(data)}

def exception_observation(error):
    """Preserve nested traceback and leaf causes; no replay or target diagnosis."""
    leaves=[]
    def visit(e,depth=0):
        if depth>16 or len(leaves)>=32:raise ValueError('exception nesting exceeds bound')
        children=getattr(e,'exceptions',None)
        if children:
            for child in children:visit(child,depth+1)
        else:leaves.append({'type':type(e).__name__,'message':str(e)[:2000]})
    visit(error)
    return {'type':type(error).__name__,'message':str(error)[:2000],'leaves':leaves,
            'traceback':''.join(traceback.format_exception(error))[:131072]}
