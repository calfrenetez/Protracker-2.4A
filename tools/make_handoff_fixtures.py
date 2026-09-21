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

def silent_tail_fixtures():
    for name,data,meta in fixtures():
        if name=='handoff_noloop':
            out=bytearray(data);out[:20]=b'handoff_silent'.ljust(20,b'\0');out[4156:4158]=bytes(2)
            yield 'handoff_silent',bytes(out),{'max_ticks':100,'focus':'canonical zero first word non-looping repeat'}

def pitch_fixtures():
    base=next(fixtures())[1]
    for name,effect,param in [('handoff_pitchup',1,3),('handoff_pitchdown',2,3),('handoff_finepup',14,0x13),('handoff_finepdown',14,0x23)]:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x20|effect;data[1103]=param
        yield name,bytes(data),{'max_ticks':100,'focus':name,'effect':effect,'parameter':param}

def modulation_fixtures():
    base=next(fixtures())[1]
    for name,effect,param in [('handoff_arp',0,0x37),('handoff_vib',4,0x47),('handoff_trem',7,0x47),('handoff_vibvol',6,0x01)]:
        assert len(name)<=20
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x20|effect;data[1103]=param
        if effect==6:data[1086]=0x14;data[1087]=0x47
        yield name,bytes(data),{'max_ticks':100,'focus':name,'effect':effect,'parameter':param}

def tone_fixtures():
    base=next(fixtures())[1]
    for name,target,effect,param in [('handoff_toneup',381,3,0),('handoff_tonedown',480,3,0),('handoff_tonevolup',381,5,1),('handoff_tonevoldown',480,5,1)]:
        assert len(name)<=20
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1100:1104]=bytes([target>>8,target&255,0x13,3])
        data[1116:1120]=bytes([0,0,0x20|effect,param]);data[1134]=15
        yield name,bytes(data),{'max_ticks':100,'focus':name,'effect':effect,'target':target}

def tone_note_fixtures():
    base=next(fixtures())[1]
    for name,target,effect,param in [('handoff_noteup',381,3,3),('handoff_notedown',480,3,3),('handoff_notevolup',381,5,1),('handoff_notevoldown',480,5,1)]:
        assert len(name)<=20
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1100:1104]=bytes([1,172,0x13,3])
        data[1116:1120]=bytes([target>>8,target&255,0x20|effect,param]);data[1134]=15
        yield name,bytes(data),{'max_ticks':100,'focus':name,'effect':effect,'target':target}

def waveform_fixtures():
    base=next(fixtures())[1]
    for name,control,effect in [('handoff_vibramp',0x41,4),('handoff_vibsquare',0x42,4),('handoff_tremramp',0x71,7),('handoff_tremsquare',0x72,7)]:
        assert len(name)<=20
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x2e;data[1103]=control
        data[1118]=effect;data[1119]=0x47;data[1134]=15
        yield name,bytes(data),{'max_ticks':100,'focus':name,'effect':effect,'control':control}

def tuning_fixtures():
    base=next(fixtures())[1]
    for name,control in [('handoff_tuneplus',0x51),('handoff_tuneminus',0x5f),('handoff_glisson',0x31),('handoff_glissoff',0x30)]:
        assert len(name)<=20
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x2e;data[1103]=control
        data[1116:1120]=bytes([1,125,3,7]) if control<0x40 else bytes([0,0,0,0x37])
        data[1134]=15
        yield name,bytes(data),{'max_ticks':100,'focus':name,'control':control}

def flow_fixtures():
    base=next(fixtures())[1]
    for name,control in [('handoff_delay',0xe1),('handoff_loopflow',0x60)]:
        assert len(name)<=20
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x2e;data[1103]=control
        if control==0x60:data[1118]=14;data[1119]=0x61;data[1134]=15
        yield name,bytes(data),{'max_ticks':100,'focus':name,'control':control}

def tempo_fixtures():
    base=next(fixtures())[1]
    for name,value in [('handoff_speed',3),('handoff_tempo',137)]:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x2f;data[1103]=value
        yield name,bytes(data),{'max_ticks':100,'focus':name,'value':value}

def jump_fixtures():
    base=next(fixtures())[1]
    for name,effect,param,row in [('handoff_jump',11,1,0),('handoff_break',13,3,3)]:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[950]=2;data[953]=1
        data[1102]=0x20|effect;data[1103]=param
        pattern=bytearray(1024)
        for skipped in range(row):pattern[skipped*16:skipped*16+4]=bytes([1,172,0x10,0])
        pattern[row*16+2]=15
        data[2108:2108]=pattern
        yield name,bytes(data),{'max_ticks':100,'focus':name,'effect':effect,'destination_row':row}

def retrigger_fixtures():
    base=next(fixtures())[1]
    for name,interval in [('handoff_retrig2',2),('handoff_retrig3',3)]:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        data[1102]=0x2e;data[1103]=0x90|interval
        yield name,bytes(data),{'max_ticks':100,'focus':name,'interval':interval}

def repeat_track_fixtures():
    base=next(fixtures())[1]
    for name,zero in [('handoff_retrigzero',True),('handoff_retriglater',False)]:
        assert len(name)<=20
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0')
        if zero:data[1102]=0x2e;data[1103]=0x90
        else:data[1118]=14;data[1119]=0x92;data[1134]=15
        yield name,bytes(data),{'max_ticks':100,'focus':name,'zero':zero}
