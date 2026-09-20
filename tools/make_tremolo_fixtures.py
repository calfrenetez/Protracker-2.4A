"""Native tremolo output-volume and waveform-memory fixtures."""
from make_offset_fixtures import fixtures as offset_fixtures
CASES=[
 ('trem_sine',[(0,0,428,1,7,0x8f),(1,0,0,0,7,0),(2,0,0,0,15,0)]),
 ('trem_memory',[(0,0,428,1,7,0x43),(1,0,0,0,7,0x08),(2,0,0,0,7,0x90),(3,0,0,0,7,0),(4,0,0,0,15,0)]),
 ('trem_waves',[(0,c,428,1,14,0x70+c) for c in range(4)]+[(1,c,0,0,7,0x8f) for c in range(4)]+[(2,c,0,0,7,0) for c in range(4)]+[(3,0,0,0,15,0)]),
 ('trem_reset',[(0,0,428,1,7,0x43),(1,0,428,1,7,0),(2,0,0,0,15,0)]),
 ('trem_keep',[(0,0,428,1,14,0x74),(1,0,0,0,7,0x43),(2,0,428,1,7,0),(3,0,0,0,15,0)]),
 ('trem_delay',[(0,0,428,1,7,0x8f),(0,1,0,0,14,0xe2),(1,0,0,0,15,0)]),
 ('trem_ramp_vib',[(0,0,428,1,4,0x81),(1,0,0,0,14,0x71),(2,0,0,0,7,0x8f),(3,0,0,0,15,0)]),
 ('trem_restore',[(0,0,428,1,7,0x8f),(1,0,0,0,0,0),(2,0,0,0,7,0),(3,0,0,0,15,0)]),
]
def fixtures():
    base=next(offset_fixtures())[1]
    for name,events in CASES:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0');data[1084:2108]=bytes(1024);data[45]=32
        if name=='trigger_loop':
            data[46:48]=(128).to_bytes(2,'big');data[48:50]=(384).to_bytes(2,'big')
        for row,ch,period,inst,effect,param in events:
            offset=1084+(row*4+ch)*4
            data[offset:offset+4]=bytes([(inst&0xf0)|(period>>8),period&255,((inst&15)<<4)|effect,param])
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
