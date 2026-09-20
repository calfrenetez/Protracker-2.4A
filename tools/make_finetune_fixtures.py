"""Synthetic finetune MODs, with native period/volume/DMA evidence captured separately."""
# Rows are (row, channel, period, instrument, effect, parameter).
CASES=[('fine_%02d'%f,f,[(0,0,427,1,0,0),(1,0,0,0,15,0)]) for f in range(16)]+[
 ('fine_override',0,[(0,0,428,1,14,0x57),(1,0,0,0,14,0x58),(2,0,428,0,0,0),(3,0,0,0,15,0)]),
 ('fine_reload',14,[(0,0,428,1,14,0x57),(1,0,428,1,0,0),(2,0,0,0,14,0x5f),(3,0,428,0,0,0),(4,0,0,0,15,0)]),
 ('fine_arp',0,[(0,0,428,1,0,0),(1,0,0,0,14,0x57),(2,0,0,0,0,0x37),(3,0,0,0,15,0)]),
 ('fine_overflow_last',15,[(0,0,113,1,0,0xff),(1,0,0,0,15,0)]),
 ('fine_overflow_next',7,[(0,0,113,1,0,0xff),(1,0,0,0,15,0)]),
 ('fine_tone_negative',8,[(0,0,428,1,14,0x31),(1,0,214,0,3,17),(2,0,0,0,5,1),(3,0,0,0,15,0)]),
 ('fine_raw_boundary',0,[(0,0,4095,1,0,0),(1,0,112,0,0,0),(2,0,0,0,15,0)]),
]
def fixtures():
    for name,fine,events in CASES:
        data=bytearray(1084+1024+64);data[:20]=name.encode().ljust(20,b'\0');data[20:42]=b'TONE ORACLE'.ljust(22,b'\0')
        data[42:44]=(32).to_bytes(2,'big');data[44]=fine;data[45]=64;data[48:50]=(32).to_bytes(2,'big')
        data[950]=1;data[951]=127;data[1080:1084]=b'M.K.';data[-64:]=bytes(range(64))
        for row,ch,period,inst,effect,param in events:
            offset=1084+(row*4+ch)*4
            data[offset:offset+4]=bytes([(inst&0xf0)|(period>>8),period&255,((inst&15)<<4)|effect,param])
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
