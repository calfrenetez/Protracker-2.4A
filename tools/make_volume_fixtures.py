"""Synthetic constant-sample MODs for pinned native volume-effect evidence."""
CASES = [
    ('fine_up', [(0,0,14,0xaf),(1,0,14,0xaf),(2,0,14,0xaf),(3,0,14,0xa0),(4,0,15,0)]),
    ('fine_down', [(0,0,14,0xbf),(1,0,14,0xbf),(2,0,14,0xb0),(3,0,15,0)]),
    ('cut_restore', [(0,0,14,0xc2),(1,0,12,64),(2,0,14,0xc0),(3,0,12,32),(4,0,14,0xcf),(5,0,15,0)]),
    ('delay_up', [(0,0,14,0xaf),(0,1,14,0xe2),(0,2,15,3),(1,0,15,0)]),
    ('delay_down', [(0,0,14,0xbf),(0,1,14,0xe2),(0,2,15,3),(1,0,15,0)]),
    ('delay_cut', [(0,0,14,0xc1),(0,1,14,0xe2),(0,2,15,3),(1,0,12,32),(2,0,15,0)]),
    ('speed_one', [(0,0,14,0xa1),(0,1,15,1),(1,0,14,0xc1),(2,0,14,0xc0),(3,0,12,64),(4,0,15,0)]),
]

def fixtures():
    for name,events in CASES:
        data=bytearray(1084+1024+64)
        data[:20]=name.encode().ljust(20,b'\0');data[20:42]=b'VOLUME ORACLE'.ljust(22,b'\0')
        data[42:44]=(32).to_bytes(2,'big');data[45]=24;data[48:50]=(32).to_bytes(2,'big')
        data[950]=1;data[951]=127;data[1080:1084]=b'M.K.'
        data[1084:1088]=bytes([1,172,16,0])  # 428 period, sample 1.
        data[-64:]=bytes([64])*64  # Native raw volume directly predicts constant PCM gain.
        for row,ch,effect,parameter in events:
            offset=1084+(row*4+ch)*4
            data[offset+2]=(data[offset+2]&0xf0)|effect;data[offset+3]=parameter
        yield name,bytes(data),{'focus':name,'events':events,'max_ticks':100,'native_trace':'NOT RUN'}
