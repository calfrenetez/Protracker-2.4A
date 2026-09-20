"""Arpeggio edge cases; expected register writes come from pinned native replay."""
CASES=[
 ('arp_cycle',[(0,0,428,1,0,0x37),(1,0,0,0,0,0x10),(2,0,0,0,0,0x01),(3,0,0,0,0,0),(4,0,0,0,15,0)]),
 ('arp_delay',[(0,0,404,1,0,0x47),(0,1,0,0,15,2),(1,0,0,0,0,0x37),(1,1,0,0,14,0xe2),(2,0,0,0,15,0)]),
 ('arp_spill',[(0,0,113,1,0,0x2f),(0,1,0,0,15,3),(1,0,0,0,0,0xf2),(2,0,0,0,15,0)]),
 ('arp_zero',[(0,0,113,1,0,0x10),(0,1,0,0,15,3),(1,0,0,0,15,0)]),
 ('arp_slide',[(0,0,428,1,1,1),(0,1,0,0,15,3),(1,0,0,0,0,0x37),(2,0,0,0,3,5),(3,0,0,0,15,0)]),
 ('arp_wrapped',[(0,0,113,1,1,255),(0,1,0,0,15,2),(1,0,0,0,0,0xf1),(1,1,0,0,15,4),(2,0,0,0,15,0)]),
 ('arp_inactive',[(0,0,0,0,0,0xff),(0,1,0,0,15,3),(1,0,404,1,0,0x37),(2,0,0,0,15,0)]),
 ('arp_speed31',[(0,0,428,1,0,0x37),(0,1,0,0,15,31),(1,0,0,0,15,0)]),
 ('arp_four',[(0,c,p,1,0,a) for c,p,a in [(0,428,0x37),(1,404,0x4f),(2,113,0xf2),(3,856,0x01)]]+[(1,0,0,0,15,0)]),
]
def fixtures():
    for name,events in CASES:
        data=bytearray(1084+1024+64);data[:20]=name.encode().ljust(20,b'\0');data[20:42]=b'ARPEGGIO ORACLE'.ljust(22,b'\0')
        data[42:44]=(32).to_bytes(2,'big');data[45]=64;data[48:50]=(32).to_bytes(2,'big')
        data[950]=1;data[951]=127;data[1080:1084]=b'M.K.';data[-64:]=bytes(range(64))
        for row,ch,period,inst,effect,param in events:
            offset=1084+(row*4+ch)*4
            data[offset:offset+4]=bytes([(inst&0xf0)|(period>>8),period&255,((inst&15)<<4)|effect,param])
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
