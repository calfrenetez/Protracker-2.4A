"""Synthetic tone-portamento MODs, with native period/volume/DMA evidence captured separately."""
# Rows are (row, channel, period, instrument, effect, parameter).
CASES=[
 ('tone_up_down',[(0,0,428,1,0,0),(0,1,0,0,15,3),(1,0,381,0,3,32),(2,0,480,0,3,0),(3,0,480,0,3,0),(4,0,0,0,15,0)]),
 ('tone_memory',[(0,0,428,1,3,7),(0,1,0,0,15,3),(1,0,428,1,0,0),(2,0,214,0,3,0),(3,0,0,0,3,0),(4,0,0,0,15,0)]),
 ('tone_volume',[(0,0,428,1,0,0),(0,1,0,0,15,3),(1,0,214,0,3,32),(2,0,0,0,5,3),(3,0,0,0,5,0x12),(4,0,0,0,15,0)]),
 ('tone_delay',[(0,0,428,1,0,0),(0,1,0,0,15,2),(1,0,214,0,3,20),(1,1,0,0,14,0xe2),(2,0,856,0,5,1),(2,1,0,0,14,0xe1),(3,0,0,0,15,0)]),
 ('tone_reset_volume',[(0,0,404,1,12,16),(0,1,0,0,15,3),(1,0,214,1,3,32),(2,0,0,0,5,1),(3,0,0,0,15,0)]),
 ('tone_quantize',[(0,0,428,1,0,0),(0,1,0,0,15,3),(1,0,427,0,3,64),(2,0,4095,0,3,0),(3,0,1,0,3,0),(4,0,0,0,15,0)]),
 ('tone_speed_one',[(0,0,428,1,3,4),(0,1,0,0,15,2),(1,0,428,1,0,0),(2,0,214,0,3,99),(2,1,0,0,15,1),(3,0,0,0,5,1),(3,1,0,0,15,3),(4,0,0,0,15,0)]),
 ('tone_old_target',[(0,0,428,1,0,0),(0,1,0,0,15,3),(1,0,214,0,3,16),(2,0,856,1,0,0),(3,0,0,0,3,0),(4,0,0,0,15,0)]),
 ('tone_wrapped',[(0,0,113,1,1,255),(0,1,0,0,15,2),(1,0,428,0,3,15),(2,0,0,0,3,0),(3,0,0,0,15,0)]),
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
