"""Extract the pinned replay's complete tuning tables and intentional overflow words."""
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def generated():
    source=(ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_text()
    table=source.split('\nmt_PeriodTable\n',1)[1].split('\nmt_audchan1temp',1)[0]
    words=[]
    for line in table.splitlines():
        m=re.match(r'\s*dc\.w\s+([0-9,]+)',line)
        if m:words.extend(map(int,m[1].split(',')))
    assert len(words)==16*37+15 and all(words[i*37+36]==0 for i in range(16))
    rows=['    '+','.join(map(str,words[i:i+12]))+',' for i in range(0,len(words),12)]
    return '/* Generated from pinned PT2.3F_replay_cia.s; retain upstream license/credits. */\nstatic const uint16_t periods[]={\n'+'\n'.join(rows)+'\n};\n'
if __name__=='__main__':(ROOT/'src/core/pitch_tables.h').write_text(generated())
