from make_handoff_fixtures import fixtures as base

def fixtures():
    for name,speed,disable in [('invert_fast',15,False),('invert_slow',8,False),('invert_disable',15,True)]:
        data=bytearray(next(base())[1]);data[:20]=name.encode().ljust(20,b'\0')
        data[48:50]=(8).to_bytes(2,'big');data[1084:2108]=bytes(1024)
        data[1084:1088]=bytes([1,172,0x1e,0xf0|speed])
        if disable:data[1102:1104]=bytes([14,0xf0])
        data[1134]=15;data[2364:2380]=bytes(range(16))
        yield name,bytes(data),{'max_ticks':100,'speed':speed,'disable':disable}


def extended_fixtures():
    """Ordering cases needed before private-workspace renderer integration."""
    original=next(fixtures())[1]
    for name in ['invert_delay','invert_shared','invert_reload']:
        data=bytearray(original);data[:20]=name.encode().ljust(20,b'\0')
        if name=='invert_delay':data[1106:1108]=bytes([14,0xe2])
        elif name=='invert_shared':data[1088:1092]=bytes([1,172,0x1e,0xf8])
        else:data[1100:1104]=bytes([0,0,0x10,0])
        yield name,bytes(data),{'max_ticks':100,'ordering_case':name}


def one_shot_fixtures():
    """Nonzero master first word; reference init clears only playback storage."""
    original=next(fixtures())[1]
    for name in ['invert_once','invert_onceretrig','invert_oncedelay']:
        data=bytearray(original);data[:20]=name.encode().ljust(20,b'\0')
        data[46:50]=bytes([0,0,0,1]);data[2108:2110]=bytes([17,163])
        if name=='invert_onceretrig':data[1102:1104]=bytes([14,0x92])
        if name=='invert_oncedelay':data[1100:1104]=bytes([1,172,14,0xd3])
        data[1118:1120]=bytes([14,0xf0])
        yield name,bytes(data),{'max_ticks':100,'ordering_case':name}
