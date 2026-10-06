"""Exact promoted editor fixture through the caller's guarded shared transport.

No connection/reservation, device access, restoration or automatic recovery is
implemented here. Caller holds the real-a1200 lease, test lock and fresh peer /
Safari authority, checks the live target before EVERY call, and pins this module,
plan, product and proof bytes. Caller wraps the transaction in a shared admitted
operation. First failure propagates without another target request. Settlement
and independent release remain separate operations owned by the caller.
"""
import asyncio,hashlib,math,re,time,zlib
from pathlib import Path

BINARY='PTEditorBridgesTest'
SHA='21c5efc7fe5d066f56e81ea8cbcea8bc7e95892e8af0a55608a1410d882d9b1e'
BYTES=385144
LOG_SHA='80a8a9731ef96568d0356de80dfdf5299ee7bef777cc0a8e7a5d038ead11d80a'
FILES=(BINARY,'Run-once','fixture.log','fixture.rc','complete.flag','launcher.log')

def digest(data):return hashlib.sha256(data).hexdigest()
def namespace(run_id):
    if not isinstance(run_id,str) or not re.fullmatch('[0-9a-f]{32}',run_id):
        raise ValueError('Exact ordinary run identity required')
    return 'RAM:PT-editor-bridges-'+run_id
def _dest(value):
    prefix='RAM:PT-editor-bridges-'
    if not isinstance(value,str) or not value.startswith(prefix) or value!=namespace(value[len(prefix):]):
        raise ValueError('Exact private RAM namespace required')
    return value
def qualify(native,release,binary,expected):
    """Saved promotion, never live target availability or physical acceptance."""
    if native.get('target')!='amiberry-030' or native.get('status')!='PASS_EXACT_030_SOFTWARE_FIXTURE_SETTLEMENT_PENDING' or type(native.get('launches')) is not int or native['launches']!=1:
        raise ValueError('One complete exact native fixture required')
    if native.get('candidate')!={'bytes':BYTES,'sha256':SHA} or len(binary)!=BYTES or digest(binary)!=SHA:
        raise ValueError('Exact qualified candidate bytes required')
    elapsed=native.get('elapsedSeconds')
    if type(elapsed) not in (int,float) or not math.isfinite(elapsed) or not 0<=elapsed<=420 or len(expected)!=624 or digest(expected)!=LOG_SHA:
        raise ValueError('Exact complete native markers and bounded result required')
    if native.get('downloadedLogs',{}).get('fixture.log')!={'bytes':624,'sha256':LOG_SHA}:
        raise ValueError('Native complete stdout binding absent')
    for name,data in (('fixture.rc',b'0\n'),('complete.flag',b'COMPLETE\n')):
        if native.get('downloadedLogs',{}).get(name)!={'bytes':len(data),'sha256':digest(data)}:
            raise ValueError('Native complete RC/flag binding absent')
    guard=release.get('guardAfter',{})
    if release.get('status')!='PASS_COMPLETED_DIAGNOSTIC_FIXTURE_VERIFIED_RELEASE' or \
       release.get('release',{}).get('released') is not True or guard.get('phase')!='EMPTY' or \
       guard.get('reservation','missing') is not None or guard.get('inflight','missing') is not None or \
       guard.get('holds')!=[] or guard.get('potential_resident') is not False:
        raise ValueError('Independent normal release required')
    if type(guard.get('fence')) is not int or guard['fence']<1 or release.get('release',{}).get('fence')!=guard['fence']:
        raise ValueError('Release identity mismatch')
    return True
def launcher(dest):
    dest=_dest(dest)
    return ('FailAt 21\nStack 65536\nCD '+dest+'\n'+BINARY+' >fixture.log\n'
            'Set PTEditorResult $RC\nEcho $PTEditorResult >fixture.rc\n'
            'Echo COMPLETE >complete.flag\nQuit $PTEditorResult\n').encode('ascii')
def create_script(dest):
    dest=_dest(dest)
    return ('FailAt 21\nIf EXISTS '+dest+'\n Echo PT-EXISTS\nElse\n MakeDir '+dest+
            '\n If WARN\n  Quit 20\n EndIf\n Echo PT-CREATED\nEndIf\n')
def poll_script(dest):
    dest=_dest(dest)
    return ('If EXISTS '+dest+'/complete.flag\n Echo PT-COMPLETE\n Type '+dest+
            '/fixture.rc\n Type '+dest+'/complete.flag\nElse\n Echo PT-WAIT\nEndIf\n')
def reply(text):
    if not isinstance(text,str) or len(text)>1048576 or not text.startswith('[OK]\n'):
        raise ValueError('Positive bounded completed script response required')
    return text.splitlines()[1:]
def launch_reply(text):
    # The established shared tool has two positive forms; a prefix alone is not
    # a completed response. In particular, [OK]garbage must not admit polling.
    if text=='[OK] (no output)':return
    reply(text)
def complete(text):
    lines=reply(text)
    if lines==['PT-WAIT']:return False
    if lines!=['PT-COMPLETE','0','COMPLETE']:
        raise ValueError('First nonzero or uncertain completion retained')
    return True
def checksum(text,data):
    if not isinstance(text,str) or len(text)>4096:raise ValueError('Bounded guest checksum response required')
    crc=re.findall(r'^CRC32: ([0-9A-Fa-f]{8})$',text,re.M)
    size=re.findall(r'^Size: ([0-9]+) bytes$',text,re.M)
    if len(crc)!=1 or len(size)!=1 or int(crc[0],16)!=zlib.crc32(data) or int(size[0])!=len(data):
        raise ValueError('Exact guest CRC and size required')
def validate_logs(data,expected):
    if set(data)!=set(FILES) or data['fixture.log']!=expected or len(expected)!=624 or digest(expected)!=LOG_SHA or \
       data['fixture.rc']!=b'0\n' or data['complete.flag']!=b'COMPLETE\n' or \
       not data['launcher.log'] or len(data['launcher.log'])>4096 or \
       len(data[BINARY])!=BYTES or digest(data[BINARY])!=SHA:
        raise ValueError('Exact complete six-file custody required')
    return {n:{'bytes':len(b),'sha256':digest(b)} for n,b in data.items()}

async def execute_once(native,release,binary_path,expected_path,dest,out,call,*,clock=time.monotonic,sleep=asyncio.sleep):
    """One background launch; bounded observations are not program retries.

    call(name,args,seconds) must check current admitted owner / exact physical
    endpoint, require a positive result and impose the requested timeout.
    Any timeout, uncertain reply, checksum or log failure stops this function;
    no finally block sends a restoration, stop, cleanup or retry request.
    """
    binary_path=Path(binary_path);expected_path=Path(expected_path);out=Path(out)
    if binary_path.is_symlink() or expected_path.is_symlink() or out.is_symlink() or not out.is_dir() or any(out.iterdir()):
        raise ValueError('Ordinary immutable inputs and fresh empty custody directory required')
    if not binary_path.is_file() or binary_path.stat().st_size!=BYTES or not expected_path.is_file() or expected_path.stat().st_size!=624:
        raise ValueError('Exact bounded ordinary input files required')
    binary=binary_path.read_bytes();expected=expected_path.read_bytes()
    qualify(native,release,binary,expected);dest=_dest(dest)
    # Freeze both upload files into the new host custody directory. Transfers
    # never reread a mutable original after its promotion hash was checked.
    upload_path=out/BINARY
    with upload_path.open('xb') as stream:stream.write(binary)
    script=launcher(dest);launch_path=out/'Run-once'
    with launch_path.open('xb') as stream:stream.write(script)
    made=await call('amiga_run_script',{'script':create_script(dest),'timeout':10},20)
    if reply(made)!=['PT-CREATED']:raise ValueError('Fresh namespace not created; retain first result')
    for name,path,data in ((BINARY,upload_path,binary),('Run-once',launch_path,script)):
        await call('amiga_push_file',{'local_path':str(path),'amiga_path':dest+'/'+name},180)
        checksum(await call('amiga_checksum',{'path':dest+'/'+name},20),data)
    started=clock();deadline=started+420;observations=0
    launch=await call('amiga_run_script',{'script':'Run >'+dest+'/launcher.log <NIL: Execute '+dest+'/Run-once','timeout':10},20)
    launch_reply(launch)
    while True:
        remaining=deadline-clock()
        if remaining<=0:raise TimeoutError('First exact420second fixture deadline retained')
        result=await call('amiga_run_script',{'script':poll_script(dest),'timeout':min(10,remaining)},min(20,remaining))
        observations+=1
        if clock()>deadline:raise TimeoutError('Absolute execution deadline exceeded')
        if complete(result):break
        await sleep(min(5,max(0,deadline-clock())))
    elapsed=clock()-started
    data={BINARY:binary,'Run-once':script}
    for name in ('fixture.log','fixture.rc','complete.flag','launcher.log'):
        path=out/name
        await call('amiga_pull_file',{'amiga_path':dest+'/'+name,'local_path':str(path)},30)
        if path.is_symlink() or not path.is_file() or path.stat().st_size>1048576:
            raise ValueError('Missing or oversized downloaded result')
        data[name]=path.read_bytes()
        checksum(await call('amiga_checksum',{'path':dest+'/'+name},20),data[name])
    files=validate_logs(data,expected)
    return {'status':'PASS_EXACT_PHYSICAL_SOFTWARE_FIXTURE_SETTLEMENT_PENDING','target':'real-a1200','launches':1,
            'elapsedSeconds':elapsed,'observations':observations,'namespace':dest,'files':files,
            'scope':'Physical CPU/software with injected timers/voices/ordinary-RAM callbacks; no device/audio/placement/timing proof'}

def cleanup_script(dest):
    """Only six verified, completed, backed-up run files, never recursive Delete."""
    dest=_dest(dest);lines=['FailAt 21','CD RAM:']
    for n in FILES:lines+=['Delete '+dest+'/'+n+' QUIET']
    lines+=['Delete '+dest+' QUIET','If EXISTS '+dest,' Echo PT-REMAINS','Else',' Echo PT-CLEAN','EndIf']
    return '\n'.join(lines)+'\n'
def absence_script(dest):
    dest=_dest(dest)
    return 'If EXISTS '+dest+'\n Echo PT-REMAINS\nElse\n Echo PT-ABSENT\nEndIf\n'
async def settle_completed(result,data,expected,call):
    """Caller independently checked guest idleness before this separate operation."""
    if result.get('target')!='real-a1200' or result.get('status')!='PASS_EXACT_PHYSICAL_SOFTWARE_FIXTURE_SETTLEMENT_PENDING' or type(result.get('launches')) is not int or result['launches']!=1:
        raise ValueError('Completed exact result required before cleanup')
    files=validate_logs(data,expected)
    if result.get('files')!=files or data['Run-once']!=launcher(result.get('namespace')):
        raise ValueError('Result, exact launcher and custody differ; no cleanup')
    dest=_dest(result['namespace'])
    for n,b in data.items():checksum(await call('amiga_checksum',{'path':dest+'/'+n},20),b)
    text=await call('amiga_run_script',{'script':cleanup_script(dest),'timeout':10},20)
    if reply(text)!=['PT-CLEAN']:raise ValueError('Cleanup uncertain; no extra request')
    text=await call('amiga_run_script',{'script':absence_script(dest),'timeout':10},20)
    if reply(text)!=['PT-ABSENT']:raise ValueError('Independent namespace absence uncertain')
    return {'status':'PASS_COMPLETED_PHYSICAL_FIXTURE_FILES_ABSENT','files':files,'namespace':dest,
            'scope':'Exact completed RAM files only; control/restoration/independent guard release remain separate'}
