"""Synthetic tone-portamento MODs, with native period/volume/DMA evidence captured separately."""
# Rows are (row, channel, period, instrument, effect, parameter).
CASES=[
 ('gliss_up',[(0,0,428,1,14,0x31),(1,0,214,0,3,7),(2,0,0,0,3,0),(3,0,0,0,15,0)]),
 ('gliss_down',[(0,0,214,1,14,0x31),(1,0,428,0,3,7),(2,0,0,0,3,0),(3,0,0,0,15,0)]),
 ('gliss_toggle',[(0,0,428,1,14,0x31),(1,0,214,0,3,7),(2,0,0,0,14,0x30),(3,0,0,0,3,0),(4,0,0,0,15,0)]),
 ('gliss_nonzero',[(0,0,428,1,14,0x3f),(1,0,214,0,3,7),(2,0,0,0,5,2),(3,0,0,0,15,0)]),
 ('gliss_delay',[(0,0,428,1,14,0x31),(1,0,214,0,3,7),(1,1,0,0,14,0xe2),(2,0,0,0,15,0)]),
 ('gliss_arrival',[(0,0,428,1,14,0x31),(1,0,404,0,3,7),(2,0,0,0,3,0),(3,0,0,0,15,0)]),
 ('gliss_zero',[(0,0,113,1,14,0x31),(1,0,1,0,3,31),(2,0,0,0,3,0),(3,0,0,0,15,0)]),
]
def fixtures():
    for name,events in CASES:
        data=bytearray(1084+1024+64);data[:20]=name.encode().ljust(20,b'\0');data[20:42]=b'TONE ORACLE'.ljust(22,b'\0')
        data[42:44]=(32).to_bytes(2,'big');data[45]=64;data[48:50]=(32).to_bytes(2,'big')
        data[950]=1;data[951]=127;data[1080:1084]=b'M.K.';data[-64:]=bytes(range(64))
        for row,ch,period,inst,effect,param in events:
            offset=1084+(row*4+ch)*4
            data[offset:offset+4]=bytes([(inst&0xf0)|(period>>8),period&255,((inst&15)<<4)|effect,param])
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
