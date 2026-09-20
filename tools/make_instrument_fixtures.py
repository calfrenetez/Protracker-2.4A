"""Instrument-without-note reference cases; sample identity stays unchanged."""
from make_offset_fixtures import fixtures as offset_fixtures
CASES=[
 ('instrument_preload',[(0,0,0,1,0,0),(1,0,428,0,12,32),(2,0,0,1,0,0),(3,0,0,0,15,0)]),
 ('instrument_reload',[(0,0,428,1,12,16),(1,0,0,1,0,0),(2,0,0,0,15,0)]),
 ('instrument_offset',[(0,0,428,1,9,1),(1,0,0,1,9,0),(2,0,428,0,0,0),(3,0,0,0,15,0)]),
 ('instrument_delay',[(0,0,428,1,12,64),(0,1,0,0,15,2),(1,0,0,1,14,0xa1),(1,1,0,0,14,0xe1),(2,0,0,0,15,0)]),
 ('instrument_retrig',[(0,0,428,1,0,0),(1,0,0,1,14,0x92),(2,0,0,0,15,0)]),
 ('instrument_noed',[(0,0,428,1,0,0),(1,0,0,1,14,0xd3),(2,0,0,0,15,0)]),
]
def fixtures():
    base=next(offset_fixtures())[1]
    for name,events in CASES:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0');data[1084:2108]=bytes(1024)
        data[45]=32;data[46:48]=(128).to_bytes(2,'big');data[48:50]=(384).to_bytes(2,'big')
        for row,ch,period,inst,effect,param in events:
            offset=1084+(row*4+ch)*4
            data[offset:offset+4]=bytes([(inst&0xf0)|(period>>8),period&255,((inst&15)<<4)|effect,param])
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
