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
