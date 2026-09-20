"""Pinned2.3F unused commands: 8xx restores period; E8x is a true no-op."""
from make_vib_fixtures import fixtures as vibrato_fixtures
CASES=[
 ('unused8_vibrato',[(0,0,404,1,4,0xff),(0,1,0,0,15,4),(1,0,0,0,8,0),(2,0,0,0,8,0xff),(3,0,0,0,15,0)]),
 ('unused8_wrap',[(0,0,113,1,1,255),(0,1,0,0,15,2),(1,0,0,0,8,128),(2,0,0,0,15,0)]),
 ('unused8_delay',[(0,0,404,1,4,0xff),(0,1,0,0,15,3),(1,0,0,0,8,1),(1,1,0,0,14,0xe1),(2,0,428,1,8,255),(3,0,0,0,15,0)]),
 ('unusede8_vibrato',[(0,0,404,1,14,0x80),(0,1,0,0,15,4),(1,0,0,0,4,0xff),(2,0,0,0,14,0x8f),(3,0,0,0,14,0x81),(4,0,0,0,15,0)]),
 ('unusede8_wrap',[(0,0,113,1,1,255),(0,1,0,0,15,2),(1,0,0,0,14,0x88),(2,0,0,0,15,0)]),
 ('unusede8_delay',[(0,0,404,1,4,0xff),(0,1,0,0,15,3),(1,0,0,0,14,0x82),(1,1,0,0,14,0xe1),(2,0,428,1,14,0x8f),(3,0,0,0,15,0)]),
]
def fixtures():
    base=next(vibrato_fixtures())[1]
    for name,events in CASES:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0');data[1084:2108]=bytes(1024)
        for row,ch,period,inst,effect,param in events:
            offset=1084+(row*4+ch)*4
            data[offset:offset+4]=bytes([(inst&0xf0)|(period>>8),period&255,((inst&15)<<4)|effect,param])
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
