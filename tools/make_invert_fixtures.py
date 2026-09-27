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


def mixed_fixture():
    """One-shot master, separate loop and unselected shared EF8 clock."""
    data=bytearray(next(one_shot_fixtures())[1]);data[:20]=b'invert_mixed'.ljust(20,b'\0')
    data[1088:1092]=bytes([1,172,0x20,0])
    data[1092:1096]=bytes([0,0,0x1e,0xf8])
    return bytes(data)


def handoff_fixtures():
    """Two private cache allocations; non-boundary loop change and return."""
    original=next(base())[1]
    for name,once,effect,parameter in [
        ('invert_swapfast',False,14,0xff),
        ('invert_swapinherit',False,0,0),
        ('invert_swaponce',True,14,0xff),
        ('invert_swapoff',True,14,0xf0),
    ]:
        data=bytearray(original);data[:20]=name.encode().ljust(20,b'\0')
        data[1084:2108]=bytes(1024)
        data[48:50]=(8).to_bytes(2,'big')
        data[76:80]=bytes([0,0,0,1]) if once else bytes([0,32,0,8])
        data[4156:4158]=bytes([17,163])
        #381 gives fractional software phase, so the next repeat is not aligned
        #with the row boundary. EF8 carries an unfinished accumulator into row1.
        data[1084:1088]=bytes([1,125,0x1e,0xf8])
        data[1100:1104]=bytes([0,0,0x20|effect,parameter])
        data[1116:1120]=bytes([0,0,0x1e,0xff])
        data[1134]=15
        yield name,bytes(data),{'max_ticks':100,'ordering_case':name,'second_one_shot':once}


def handoff_command_fixtures():
    """Inherited EF8 clock with E9/ED/9xx on a new instrument, then return."""
    originals=list(handoff_fixtures())
    for command,period,effect,parameter in [('retrig',0,14,0x92),('delay',480,14,0xd3),('offset',480,9,1)]:
        for once in (False,True):
            name='inv_'+command+('_once' if once else '_loop')
            data=bytearray(originals[2 if once else 0][1])
            data[:20]=name.encode().ljust(20,b'\0')
            data[1100:1104]=bytes([period>>8,period&255,0x20|effect,parameter])
            # The looped offset case deliberately exceeds its initial repeat end;
            # the one-shot case instead starts at byte256 of its512-byte body.
            yield name,bytes(data),{'max_ticks':100,'ordering_case':name,'second_one_shot':once,'command':command}


def shared_handoff_fixtures():
    """Channel1 mutates shared/old/new banks while channel0 changes instruments."""
    originals=list(handoff_fixtures())
    for name,once,second in [('inv_shared_join',False,2),('inv_shared_split',False,1),('inv_shared_once',True,1)]:
        data=bytearray(originals[2 if once else 0][1]);data[:20]=name.encode().ljust(20,b'\0')
        data[1088:1092]=bytes([0,0,0x1e,0xff])
        data[1100:1104]=bytes([0,0,0x20,0])
        data[1104:1108]=bytes([0,0,(second<<4)|14,0xf8])
        data[1120:1124]=bytes([0,0,0x2e,0xff])
        yield name,bytes(data),{'max_ticks':100,'ordering_case':name,'second_one_shot':once,'mutator_row1_instrument':second}


def long_loop_fixtures():
    """Full-bank oracle cases: >16 bytes, cursor wrap, collisions and handoffs."""
    baseline=list(shared_handoff_fixtures())[1][1]
    yield 'inv_writes_baseline',baseline,{'max_ticks':100,'loop_bytes':16,'ordering_case':'baseline against prior188-byte capture'}
    for length in (18,64,256):
        name='inv_writes_'+str(length);data=bytearray(next(handoff_fixtures())[1])
        data[:20]=name.encode().ljust(20,b'\0');data[1084:2108]=bytes(1024)
        for header in (48,78):data[header:header+2]=(length//2).to_bytes(2,'big')
        data[1084:1088]=bytes([1,125,0x1e,0xff])
        data[1088:1092]=bytes([0,0,0x1e,0xff])
        data[1120:1124]=bytes([0,0,14,0xf8])
        data[1148:1152]=bytes([0,0,0x2e,0xff])
        data[1152:1156]=bytes([0,0,0x1e,0xff])
        data[1180:1184]=bytes([0,0,0x1e,0xff])
        data[1184:1188]=bytes([0,0,0x2e,0xff])
        data[1198]=15
        yield name,bytes(data),{'max_ticks':100,'loop_bytes':length,'ordering_case':name}


def four_clock_delay_fixtures():
    """Four clocks, shared writes and repeated tick-zero EFx under EEx."""
    for name,delay,third in [('inv_delay_four',2,0xff),('inv_delay_disable',1,0xf0)]:
        data=bytearray(next(handoff_fixtures())[1]);data[:20]=name.encode().ljust(20,b'\0')
        data[1084:2108]=bytes(1024)
        for header in (48,78):data[header:header+2]=(9).to_bytes(2,'big')
        for ch in range(4):
            data[1084+4*ch:1088+4*ch]=bytes([1 if ch==0 else 0,125 if ch==0 else 0,0x1e,0xff])
        for ch,inst,param in [(0,2,0xff),(1,1,0xff),(2,2,third),(3,0,0xe0|delay)]:
            data[1100+4*ch:1104+4*ch]=bytes([0,0,(inst<<4)|14,param])
        for ch,inst in enumerate((1,2,1,2)):
            data[1116+4*ch:1120+4*ch]=bytes([0,0,(inst<<4)|14,0xff])
        data[1134]=15
        yield name,bytes(data),{'max_ticks':100,'loop_bytes':18,'delay':delay,'active_ticks':18+6*delay,'ordering_case':name}
