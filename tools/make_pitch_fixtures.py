"""Synthetic legal-note pitch-slide fixtures; native behavior is captured separately."""
CASES=[
 ('slides',428,[(0,0,1,3),(0,1,15,3),(1,0,2,5),(2,0,1,0),(3,0,2,0),(4,0,15,0)]),
 ('bounds',113,[(0,0,1,15),(0,1,15,3),(1,0,2,255),(2,0,2,255),(3,0,15,0)]),
 ('fine',428,[(0,0,14,0x1f),(0,1,15,3),(1,0,14,0x2f),(2,0,14,0x10),(3,0,14,0x20),(4,0,15,0)]),
 ('fine_delay',113,[(0,0,14,0x1f),(0,1,14,0xe2),(0,2,15,2),(1,0,14,0x2f),(1,1,14,0xe2),(2,0,15,0)]),
 ('slide_delay',428,[(0,0,1,3),(0,1,14,0xe2),(0,2,15,2),(1,0,2,5),(1,1,14,0xe1),(2,0,15,0)]),
 ('wrap_restore',113,[(0,0,1,255),(0,1,15,2),(1,0,12,32),(2,0,0,0),(3,0,14,0x21),(4,0,15,0)]),
 ('zero_refused',113,[(0,0,1,255),(0,1,15,2),(1,0,2,142),(2,0,15,0)]),
]
def fixtures():
    for name,period,events in CASES:
        data=bytearray(1084+1024+64)
        data[:20]=name.encode().ljust(20,b'\0');data[20:42]=b'PITCH ORACLE'.ljust(22,b'\0')
        data[42:44]=(32).to_bytes(2,'big');data[45]=64;data[48:50]=(32).to_bytes(2,'big')
        data[950]=1;data[951]=127;data[1080:1084]=b'M.K.'
        data[1084:1088]=bytes([period>>8,period&255,16,0])
        data[-64:]=bytes(range(64))
        for row,ch,effect,param in events:
            offset=1084+(row*4+ch)*4;data[offset+2]=(data[offset+2]&0xf0)|effect;data[offset+3]=param
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
