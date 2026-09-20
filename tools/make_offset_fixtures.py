"""Classic sample-offset fixtures; ranges deliberately expose repeated application."""
CASES=[
 ('offset_double',2048,0,2,[(0,0,428,1,9,1),(1,0,0,0,15,0)]),
 ('offset_memory',2048,0,2,[(0,0,428,1,9,1),(1,0,428,0,9,0),(2,0,428,0,0,0),(3,0,0,0,15,0)]),
 ('offset_command',2048,0,2,[(0,0,428,1,0,0),(1,0,0,0,9,2),(2,0,428,0,0,0),(3,0,0,0,15,0)]),
 ('offset_reload',2048,0,2,[(0,0,428,1,9,2),(1,0,428,1,9,0),(2,0,0,0,15,0)]),
 ('offset_bounds',512,0,2,[(0,0,428,1,9,2),(1,0,428,1,9,1),(2,0,0,0,9,255),(3,0,428,0,0,0),(4,0,0,0,15,0)]),
 ('offset_loop',2048,256,768,[(0,0,428,1,9,2),(1,0,428,0,0,0),(2,0,0,0,15,0)]),
 ('offset_delay',2048,0,2,[(0,0,428,1,9,1),(0,1,0,0,15,2),(1,0,428,0,9,0),(1,1,0,0,14,0xe2),(2,0,0,0,15,0)]),
 ('offset_four',4096,0,2,[(0,c,428,1,9,c+1) for c in range(4)]+[(1,0,0,0,15,0)]),
]
def fixtures():
    for name,length,start,replen,events in CASES:
        data=bytearray(1084+1024+length);data[:20]=name.encode().ljust(20,b'\0');data[20:42]=b'OFFSET ORACLE'.ljust(22,b'\0')
        data[42:44]=(length//2).to_bytes(2,'big');data[45]=64;data[46:48]=(start//2).to_bytes(2,'big');data[48:50]=(replen//2).to_bytes(2,'big')
        data[950]=1;data[951]=127;data[1080:1084]=b'M.K.';data[2108:]=bytes(i%127 for i in range(length))
        for row,ch,period,inst,effect,param in events:
            offset=1084+(row*4+ch)*4
            data[offset:offset+4]=bytes([(inst&0xf0)|(period>>8),period&255,((inst&15)<<4)|effect,param])
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
