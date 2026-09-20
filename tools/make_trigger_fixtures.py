"""Native trigger timing fixtures for E9x and EDx, before renderer enablement."""
from make_offset_fixtures import fixtures as offset_fixtures
CASES=[
 ('retrig_note',[(0,0,428,1,14,0x93),(1,0,0,0,15,0)]),
 ('retrig_empty',[(0,0,428,1,0,0),(1,0,0,0,14,0x92),(2,0,0,0,15,0)]),
 ('retrig_zero',[(0,0,428,1,14,0x90),(1,0,0,0,14,0x90),(2,0,0,0,15,0)]),
 ('retrig_every',[(0,0,428,1,14,0x91),(1,0,0,0,15,0)]),
 ('delay_note',[(0,0,428,1,14,0xd3),(1,0,0,0,15,0)]),
 ('delay_zero',[(0,0,428,1,14,0xd0),(1,0,0,0,14,0xd0),(2,0,0,0,15,0)]),
 ('delay_past',[(0,0,428,1,14,0xd6),(1,0,0,0,15,0)]),
 ('trigger_repeat',[(0,0,428,1,14,0x92),(0,1,428,1,14,0xd3),(0,2,0,0,14,0xe1),(1,0,0,0,15,0)]),
 ('trigger_offset',[(0,0,428,1,9,1),(1,0,0,0,14,0x93),(2,0,428,0,14,0xd2),(3,0,0,0,15,0)]),
 ('trigger_loop',[(0,0,428,1,14,0x92),(1,0,428,1,14,0xd2),(2,0,0,0,15,0)]),
]
def fixtures():
    base=next(offset_fixtures())[1]
    for name,events in CASES:
        data=bytearray(base);data[:20]=name.encode().ljust(20,b'\0');data[1084:2108]=bytes(1024)
        if name=='trigger_loop':
            data[46:48]=(128).to_bytes(2,'big');data[48:50]=(384).to_bytes(2,'big')
        for row,ch,period,inst,effect,param in events:
            offset=1084+(row*4+ch)*4
            data[offset:offset+4]=bytes([(inst&0xf0)|(period>>8),period&255,((inst&15)<<4)|effect,param])
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
