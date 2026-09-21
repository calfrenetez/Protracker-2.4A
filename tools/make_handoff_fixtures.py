"""Two-sample active instrument-only handoffs for pinned replay capture."""
from make_offset_fixtures import fixtures as baseline

def fixtures():
    for name,loop,events in [
        ('handoff_loop',True,[(0,1),(1,2)]),
        ('handoff_return',True,[(0,1),(1,2),(2,1)]),
        ('handoff_noloop',False,[(0,1),(1,2)]),
        ('handoff_ed',True,[(0,1),(1,2)]),
    ]:
        data=bytearray(next(baseline())[1]);data[:20]=name.encode().ljust(20,b'\0');data[1084:2108]=bytes(1024)
        # First sample loops over [256,1024); second over [64,320), or the
        # original non-looping two-byte repeat. Distinct signed PCM and volumes.
        data[46:48]=(128).to_bytes(2,'big');data[48:50]=(384).to_bytes(2,'big')
        data[50:72]=b'HANDOFF SECOND'.ljust(22,b'\0');data[72:74]=(256).to_bytes(2,'big');data[75]=32
        data[76:78]=(32 if loop else 0).to_bytes(2,'big');data[78:80]=(128 if loop else 1).to_bytes(2,'big')
        data.extend(bytes(128+i%127 for i in range(512)))
        for row,inst in events:
            pos=1084+row*16;period=428 if row==0 else 0
            effect,param=(14,0xd3) if name=='handoff_ed' and row==1 else (0,0)
            data[pos:pos+4]=bytes([period>>8,period&255,(inst<<4)|effect,param])
        stop=max(row for row,_ in events)+1;data[1084+stop*16+2]=15
        yield name,bytes(data),{'max_ticks':100,'focus':name,'changes':events,'second_loop':loop}

def volume_fixtures():
    base=next(fixtures())[1]
    for name,volume in [('handoff_volume',16),('handoff_clamp',127)]:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x2c;data[1103]=volume
        yield name,bytes(data),{'max_ticks':100,'focus':name,'changes':[(0,1),(1,2)],'second_loop':True,'volume':min(volume,64)}

def slide_fixtures():
    base=next(fixtures())[1]
    for name,effect,param in [('handoff_up',10,0x10),('handoff_down',10,1),('handoff_fineup',14,0xa1),('handoff_finedown',14,0xb1)]:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x20|effect;data[1103]=param
        yield name,bytes(data),{'max_ticks':100,'focus':name,'effect':effect,'parameter':param}

def cut_fixtures():
    base=next(fixtures())[1]
    for name,cut in [('handoff_cut0',0),('handoff_cut3',3)]:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x2e;data[1103]=0xc0|cut
        data[1118]=12;data[1119]=64;data[1134]=15
        yield name,bytes(data),{'max_ticks':100,'focus':name,'cut':cut}
