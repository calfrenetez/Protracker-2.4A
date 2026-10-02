import asyncio
from contextlib import asynccontextmanager
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('physical_discovery', ROOT / 'tools/shared_infra_amigus_discovery.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class PhysicalDiscoveryGuard(unittest.TestCase):
    def test_wavetable_snapshot_requires_exact_reads_release_and_scope(self):
        scope='WAVETABLE-PROBE version=0.9 scope=Mini-7ea663e7-global-status writes=NO bank_select=NO audio=NOT_TESTED capacity=NOT_QUALIFIED voices_stopped=NOT_QUALIFIED'
        rows=[f'WAVETABLE STATUS offset=0x{i:02x} value=0x8001' for i in range(0,16,2)]
        release='WAVETABLE RELEASE confirmed=1 retained=0 driver=0x00000000'
        footer='WAVETABLE result=PASS reason=observed reads=8 rc=0'
        output='\n'.join([scope,*rows,release,footer])
        parsed=runner.wavetable_status_finished(output)
        self.assertFalse(parsed['capacity_qualified'])
        self.assertFalse(parsed['voices_stopped_qualified'])
        self.assertEqual([r['value'] for r in parsed['registers']],[0x8001]*8)
        for bad in (output.replace('offset=0x0e','offset=0x10'),
                    output.replace('offset=0x0e','offset=0x0c'),
                    output.replace('confirmed=1','confirmed=0'),
                    output.replace('retained=0','retained=1'),
                    output.replace('writes=NO','writes=YES'),
                    output.replace('bank_select=NO','bank_select=YES'),
                    output.replace('reads=8','reads=7'),output+'\n'+release,
                    output+'\n'+footer,output+'\nWAVETABLE HOLD',
                    '\n'.join([scope,*rows[:-1],release,footer])):
            with self.assertRaises(RuntimeError):runner.wavetable_status_finished(bad)

    def test_wavetable_exact_fixture_and_separate_mode_gate(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td); (root/'build/diagnostic').mkdir(parents=True)
            lock=root/'amigus-sdk.lock.json';lock.write_text('{"files":{}}')
            binary=root/'AmiGUSTest';binary.write_bytes(b'exact diagnostic')
            fixture=root/'build/diagnostic/PTWavetableReadTest';fixture.write_bytes(b'exact reader')
            manifest=dict(binary_sha256=runner.digest(fixture),binary_bytes=fixture.stat().st_size,
                inputs={},sdk_lock_sha256=runner.digest(lock))
            fixture_build=fixture.parent/'wavetable-read-build.json';fixture_build.write_text(json.dumps(manifest))
            build=root/'build.json';build.write_text(json.dumps(dict(binary_sha256=runner.digest(binary),
                binary_bytes=binary.stat().st_size,inputs={},sdk_lock_sha256=runner.digest(lock))))
            directory=root/'render-owned';directory.mkdir();emulator=directory/'result.json'
            native=dict(AmiGUSTest_sha256=runner.digest(binary),PTWavetableReadTest_sha256=runner.digest(fixture),
                passed=True,run_files_cleaned=True,**{'amigus-discover_returncode':'5',
                'amigus-ownership_returncode':'5','ownership-fixture_returncode':'0','native-abi_returncode':'0',
                'amigus-wavetable_returncode':'5','wavetable-read_returncode':'0'})
            emulator.write_text(json.dumps(native))
            independent=root/'independent.json';independent.write_text(json.dumps(dict(passed=True,
                paths={str(runner.INFRA/'runtime/Dev/Tests'/directory.name):False,
                       str(runner.INFRA/'runtime/Dev/Tests'/('launch-'+directory.name)):False},
                status='OK\tPaused=false',cpu='OK\tmodel=68030',
                audio='OK\tch0_dma=0\tch1_dma=0\tch2_dma=0\tch3_dma=0')))
            with patch.object(runner,'ROOT',root):
                self.assertEqual(runner.qualification(binary,build,emulator,independent,ownership=True,wavetable=True),runner.digest(binary))
                for field,value in (('amigus-wavetable_returncode','20'),('wavetable-read_returncode','20'),
                                    ('PTWavetableReadTest_sha256','old')):
                    altered=dict(native);altered[field]=value;emulator.write_text(json.dumps(altered))
                    with self.assertRaises(RuntimeError):runner.qualification(binary,build,emulator,independent,ownership=True,wavetable=True)
                emulator.write_text(json.dumps(native));fixture.write_bytes(b'unqualified reader')
                with self.assertRaises(RuntimeError):runner.qualification(binary,build,emulator,independent,ownership=True,wavetable=True)
                for kwargs in ({'ownership':False},{'ownership':True,'idle_driver':True}):
                    with self.assertRaisesRegex(RuntimeError,'separate ownership-only'):
                        runner.qualification(binary,build,emulator,independent,wavetable=True,**kwargs)

    def test_bounded_observation_never_promotes_capacity(self):
        footer='PCM CAPACITY OBSERVATION complete=1 polls=8 stores=2048 writes_after_poll_start=0 capacity=NOT_QUALIFIED'
        for counts in ([4094]*8,[4094]+[4096]*7):
            lines=[f'PCM CAPACITY OBSERVE index={i+1} pending_words={n} flags=0x0002 rate=0x0000 mask=0x0050' for i,n in enumerate(counts)]
            output='\n'.join(lines+[footer])
            parsed=runner.capacity_observation_finished(output)
            self.assertFalse(parsed['capacity_qualified'])
            self.assertEqual([row['pending_words'] for row in parsed['polls']],counts)
            for bad in (output.replace('index=8','index=7'),output.replace('pending_words=4094','pending_words=4095'),
                        output.replace('rate=0x0000','rate=0x8000'),output.replace('mask=0x0050','mask=0x0051'),
                        output+'\n'+footer,'\n'.join(lines[:-1]+[footer])):
                with self.assertRaises(RuntimeError):runner.capacity_observation_finished(bad)

    def test_idle_driver_guards_and_positive_restoration(self):
        info = 'Library: AmiGUS.audio\n  Version: 4.23\n  Open count: 0\n  ID string: ' + runner.IDLE_DRIVER_ID + '\n'
        runner.idle_driver_info(info)
        for bad in (info.replace('count: 0','count: 1'),info.replace('4.23','4.24'),
                    info.replace('020 SAS/C','000 SAS/C'), 'unavailable'):
            with self.assertRaises(RuntimeError):runner.idle_driver_info(bad)
        runner.idle_driver_info(info.replace('count: 0','count: 1'),require_idle=False)
        for status,code in (('PASS','0'),('FAIL','20'),('SKIP','5')):
            line = f'IDLE-OWNERSHIP result={status} unloaded=1 restored=1 restore_needed=0 rc={code}'
            self.assertTrue(runner.idle_restoration_finished(line))
            self.assertFalse(runner.idle_restoration_finished(line,registers=True))
            self.assertTrue(runner.idle_restoration_finished(line.replace('IDLE-OWNERSHIP','IDLE-REGISTERS'),registers=True))
            self.assertFalse(runner.idle_restoration_finished(line,registers=True,reset=True))
            self.assertTrue(runner.idle_restoration_finished(line.replace('IDLE-OWNERSHIP','IDLE-RESET'),registers=True,reset=True))
            self.assertFalse(runner.idle_restoration_finished(line,registers=True,reset=True,fifo=True))
            fifo_line = line.replace('IDLE-OWNERSHIP','IDLE-FIFO')
            self.assertTrue(runner.idle_restoration_finished(fifo_line,registers=True,reset=True,fifo=True))
            self.assertFalse(runner.idle_restoration_finished(fifo_line,registers=True,reset=True))
            self.assertFalse(runner.idle_restoration_finished(fifo_line.replace('restore_needed=0','restore_needed=1'),registers=True,reset=True,fifo=True))
            capacity_line = line.replace('IDLE-OWNERSHIP','IDLE-CAPACITY')
            self.assertTrue(runner.idle_restoration_finished(capacity_line,registers=True,reset=True,fifo=True,capacity=True))
            self.assertFalse(runner.idle_restoration_finished(fifo_line,registers=True,reset=True,fifo=True,capacity=True))
            self.assertFalse(runner.idle_restoration_finished(capacity_line.replace('restored=1','restored=0'),registers=True,reset=True,fifo=True,capacity=True))
            observe_line=line.replace('IDLE-OWNERSHIP','IDLE-CAPACITY-OBSERVE')
            self.assertTrue(runner.idle_restoration_finished(observe_line,registers=True,reset=True,fifo=True,capacity=True,observe=True))
            self.assertFalse(runner.idle_restoration_finished(capacity_line,registers=True,reset=True,fifo=True,capacity=True,observe=True))
            for bad in (line.replace('restored=1','restored=0'),line.replace('restore_needed=0','restore_needed=1'),
                        line.replace('rc='+code,'rc=99'),line+'\n'+line,'DRIVER HOLD: restoration unconfirmed'):
                self.assertFalse(runner.idle_restoration_finished(bad))

    def test_exact_bytes_and_independent_cleanup_are_required_before_target_selection(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td); binary = root / 'probe'; binary.write_bytes(b'checked bytes')
            build = root / 'build.json'
            build.write_text(json.dumps(dict(binary_sha256=runner.digest(binary), binary_bytes=binary.stat().st_size,
                inputs={}, sdk_lock_sha256=runner.digest(ROOT / 'amigus-sdk.lock.json'))))
            directory = root / 'render-files-owned'; directory.mkdir()
            emulator = directory / 'result.json'
            native = dict(PTAmiGusDiscovery_sha256=runner.digest(binary), passed=True, amigus_discovery_returncode='0', run_files_cleaned=True)
            native['amigus-discovery_returncode'] = native.pop('amigus_discovery_returncode')
            emulator.write_text(json.dumps(native))
            independent = root / 'independent.json'
            cleanup = dict(passed=True, paths={str(runner.INFRA / 'runtime/Dev/Tests' / directory.name):False,
                str(runner.INFRA / 'runtime/Dev/Tests' / ('launch-' + directory.name)):False},
                status='OK\tPaused=false', cpu='OK\tmodel=68030', audio='OK\tch0_dma=0\tch1_dma=0\tch2_dma=0\tch3_dma=0')
            independent.write_text(json.dumps(cleanup))
            self.assertEqual(runner.qualification(binary,build,emulator,independent),runner.digest(binary))
            for mode in ('changed-bytes','wrong-run','path-reappeared','paused','ambiguous-status','dma-active','failed-native'):
                with self.subTest(mode=mode):
                    altered = json.loads(json.dumps(cleanup)); altered_native = dict(native)
                    if mode=='changed-bytes':binary.write_bytes(b'unqualified bytes')
                    if mode=='wrong-run':altered['paths']={str(root / 'other'):False}
                    if mode=='path-reappeared':altered['paths'][next(iter(altered['paths']))]=True
                    if mode=='paused':altered['status']='OK\tPaused=true'
                    if mode=='ambiguous-status':altered['status']='OK\tPaused=false\tPaused=true'
                    if mode=='dma-active':altered['audio']='OK\tch0_dma=1\tch1_dma=0\tch2_dma=0\tch3_dma=0'
                    if mode=='failed-native':altered_native['passed']=False
                    independent.write_text(json.dumps(altered));emulator.write_text(json.dumps(altered_native))
                    with self.assertRaises(RuntimeError):runner.qualification(binary,build,emulator,independent)
                    binary.write_bytes(b'checked bytes')
            independent.write_text(json.dumps(cleanup))
            own_native=dict(AmiGUSTest_sha256=runner.digest(binary),passed=True,run_files_cleaned=True,
                **{'amigus-discover_returncode':'5','amigus-ownership_returncode':'5','ownership-fixture_returncode':'0','native-abi_returncode':'0'})
            emulator.write_text(json.dumps(own_native))
            self.assertEqual(runner.qualification(binary,build,emulator,independent,True),runner.digest(binary))
            for field,value in (('passed',False),('AmiGUSTest_sha256','different'),('amigus-ownership_returncode','20'),('ownership-fixture_returncode','20')):
                with self.subTest(ownership_field=field):
                    altered=dict(own_native);altered[field]=value;emulator.write_text(json.dumps(altered))
                    with self.assertRaises(RuntimeError):runner.qualification(binary,build,emulator,independent,True)

            # A prior reset qualification cannot qualify the new FIFO mode.
            altered=dict(own_native);altered['amigus-reset_returncode']='5'
            emulator.write_text(json.dumps(altered))
            with self.assertRaisesRegex(RuntimeError,'disabled FIFO qualification missing'):
                runner.qualification(binary,build,emulator,independent,True,True,True,True,True)
            altered['amigus-fifo_returncode']='5';emulator.write_text(json.dumps(altered))
            with self.assertRaisesRegex(RuntimeError,'bounded capacity qualification missing'):
                runner.qualification(binary,build,emulator,independent,True,True,True,True,True,True)
            altered['amigus-capacity_returncode']='5';emulator.write_text(json.dumps(altered))
            with self.assertRaisesRegex(RuntimeError,'bounded observation qualification missing'):
                runner.qualification(binary,build,emulator,independent,True,True,True,True,True,True,True)
            altered['amigus-fifo_returncode']='20';emulator.write_text(json.dumps(altered))
            with self.assertRaisesRegex(RuntimeError,'disabled FIFO qualification missing'):
                runner.qualification(binary,build,emulator,independent,True,True,True,True,True)

    def test_uncertain_script_never_deletes_or_switches_target(self):
        switches=[]; calls=[]
        async def connect(target):switches.append(target)
        def target(root,name):return dict(connected=True,host='fixture',port=2345)
        def require_reply(reply):
            if reply.content[0].text.startswith('[STILL RUNNING]'):raise RuntimeError('Script still running')
        class Guest:
            env={'profile':'fixture'}
            def __init__(self,*args):pass
            def command(self,name):
                return 'OK\tPaused=false' if name=='GET_STATUS' else 'OK\tch0_dma=0\tch1_dma=0\tch2_dma=0\tch3_dma=0'
        class Session:
            def __init__(self,*args):pass
            async def __aenter__(self):return self
            async def __aexit__(self,*args):pass
            async def initialize(self):pass
            async def call_tool(self,name,args):
                calls.append((name,args))
                # A success marker in partial output cannot prove termination.
                return SimpleNamespace(content=[SimpleNamespace(text='[STILL RUNNING]\nPTG-IDENTITY-DONE')])
        @asynccontextmanager
        async def transport(*args):yield (None,None,None)
        modules={'amiga':SimpleNamespace(connect=connect),
            'bridge_checks':SimpleNamespace(target=target,require_reply=require_reply,checksum=lambda *args:None),
            'shared_guest':SimpleNamespace(Guest=Guest),'mcp':SimpleNamespace(ClientSession=Session),
            'mcp.client.streamable_http':SimpleNamespace(streamable_http_client=transport)}
        with tempfile.TemporaryDirectory() as td,patch.dict(sys.modules,modules):
            out=Path(td);binary=out/'PTAmiGusDiscovery';binary.write_bytes(b'fixture');result={'passed':False}
            original_path=list(sys.path)
            try:
                with self.assertRaisesRegex(RuntimeError,'still running'):asyncio.run(runner.run(out,binary,result))
            finally:sys.path[:]=original_path
            self.assertEqual(switches,['real-a1200'])
            self.assertEqual(len(calls),1)
            self.assertTrue(result['script_pending']);self.assertTrue(result['recovery_hold'])
            self.assertNotIn('run_files_cleaned',result);self.assertNotIn('devbench_returned',result)


if __name__ == '__main__':
    unittest.main()
