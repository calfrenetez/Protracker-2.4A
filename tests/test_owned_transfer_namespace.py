"""Fresh namespace/deadline refusal models; no FTP, sockets or processes."""
import asyncio
import copy
import unittest

from owned_native_input import InputPlan, InputStage
from owned_native_run import Plan
from owned_transfer_namespace import TransferNamespace
from test_owned_native_input import InputBridge


class NamespaceTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        self.execution = Plan('b2' * 16, 123, b'MODEL BINARY' * 500, ('model PASS',),
                              copy_source='RAM:PT24G-' + 'b2' * 8 + '-in/NativeTest')
        self.plan = InputPlan(self.execution, None, None, external_transfer=True)
        self.bridge = InputBridge(self.plan)
        self.bridge.files.pop(None)
        self.records = []
        self.now = 100.0
        self.new()

    def new(self, **kw):
        self.stage = TransferNamespace(self.plan, self.bridge.exchange, self.records.append,
            download_name='PT24G-TEST', clock=lambda: self.now, **kw)
        return self.stage

    async def test_fresh_namespace_only_and_claim_once_retains_original_deadlines(self):
        original = copy.deepcopy(self.bridge.files)
        ready = await self.stage.prepare()
        self.assertEqual(ready['status'], 'FRESH_NAMESPACE_WAITING_EXTERNAL_TRANSFER')
        self.assertEqual(ready['plan'], self.plan.description())
        self.assertEqual(self.bridge.files, original)
        self.assertEqual([c['type'] for c in self.bridge.calls], ['LISTDIR','MAKEDIR','LISTDIR'])
        self.now = 180
        bounds = self.stage.claim()
        self.assertEqual(bounds, {'deadline':350.0,'prepareDeadline':220.0})
        # Actual caller adoption cannot restart either original bound.
        adopt = InputStage(self.plan, self.bridge.exchange, self.records.append, clock=lambda:self.now)
        adopt.deadline = min(adopt.deadline,bounds['deadline'])
        adopt.prepare_deadline = min(adopt.prepare_deadline,bounds['prepareDeadline'])
        self.assertEqual((adopt.deadline,adopt.prepare_deadline),(350,220))
        before = copy.deepcopy(self.bridge.calls)
        with self.assertRaises(ValueError):self.stage.claim()
        with self.assertRaises(ValueError):await self.stage.prepare()
        self.assertEqual(self.bridge.calls,before)

    async def test_existing_execution_input_or_download_preserved_before_mutation(self):
        for name in (self.execution.directory,self.plan.directory,'RAM:PT24G-TEST'):
            self.setUp()
            self.bridge.files[name]=b'foreign'
            before=copy.deepcopy(self.bridge.files)
            with self.assertRaises(ValueError):await self.stage.prepare()
            self.assertEqual(self.stage.phase,'FAILED')
            self.assertEqual(self.bridge.files,before)
            self.assertEqual([c['type'] for c in self.bridge.calls],['LISTDIR'])

    async def test_each_exchange_failure_or_cancellation_stops_without_followup(self):
        for index in (1,2,3):
            for failure in (TimeoutError,asyncio.CancelledError):
                self.setUp();calls=[]
                async def ask(command,reply_type,seconds):
                    calls.append(command)
                    if len(calls)==index:raise failure()
                    return await self.bridge.exchange(command,reply_type,seconds)
                self.stage.exchange=ask
                with self.assertRaises(failure):await self.stage.prepare()
                self.assertEqual((self.stage.phase,len(calls)),('FAILED',index))
                with self.assertRaises(ValueError):await self.stage.prepare()
                self.assertEqual(len(calls),index)
                self.assertFalse(any(c['type'] in ('WRITEFILE','LAUNCH','DELETEFILE') for c in calls))

    async def test_failed_creation_ack_or_nonempty_fresh_directory_refuses(self):
        for mode in ('ack','nonempty'):
            self.setUp()
            async def ask(command,reply_type,seconds):
                raw=await self.bridge.exchange(command,reply_type,seconds)
                if command['type']=='MAKEDIR':
                    if mode=='ack':return 'OK|MAKEDIR|RAM:other'
                    self.bridge.files[self.plan.path('NativeTest')]=b'foreign'
                return raw
            self.stage.exchange=ask
            with self.assertRaises(ValueError):await self.stage.prepare()
            self.assertEqual(self.stage.phase,'FAILED')
            self.assertFalse(any(c['type'] in ('WRITEFILE','LAUNCH','DELETEFILE') for c in self.bridge.calls))

    async def test_raw_bridge_error_is_preserved_and_not_replayed(self):
        import native_file_protocol as wire
        async def ask(*a):raise wire.BridgeReplyError('ERR|observed failure')
        self.stage.exchange=ask
        with self.assertRaises(wire.BridgeReplyError) as caught:await self.stage.prepare()
        self.assertEqual(caught.exception.raw,'ERR|observed failure')
        self.assertEqual(self.records[-1]['raw'],'ERR|observed failure')
        self.assertEqual(self.stage.calls,1)

    async def test_original_preparation_expiry_before_after_reply_and_waiting_claim_refuses(self):
        for mode in ('before','after','waiting'):
            self.setUp()
            if mode=='before':self.now=221
            elif mode=='after':
                async def ask(command,reply_type,seconds):
                    raw=await self.bridge.exchange(command,reply_type,seconds);self.now=221;return raw
                self.stage.exchange=ask
            if mode=='waiting':
                await self.stage.prepare();self.now=221
                with self.assertRaises(ValueError):self.stage.claim()
                self.assertEqual(len(self.bridge.calls),3)
            else:
                with self.assertRaises(ValueError):await self.stage.prepare()
                self.assertEqual(len(self.bridge.calls),0 if mode=='before' else 1)
            self.assertEqual(self.stage.phase,'FAILED')

    async def test_shorter_whole_lifetime_and_bounded_real_async_timeout(self):
        self.new(seconds=30)
        await self.stage.prepare();self.now=131
        with self.assertRaises(ValueError):self.stage.claim()
        self.setUp();self.new(seconds=.02)
        async def blocked(*a):await asyncio.Event().wait()
        self.stage.exchange=blocked
        with self.assertRaises(TimeoutError):await self.stage.prepare()
        self.assertEqual((self.stage.phase,self.stage.calls),('FAILED',1))

    async def test_wrong_plan_filename_and_bounds_refuse_without_effects(self):
        for name in ('../foreign','NativeTest',self.plan.directory.split(':')[1],self.execution.directory.split(':')[1]):
            with self.assertRaises(ValueError):TransferNamespace(self.plan,self.bridge.exchange,self.records.append,download_name=name)
        for args in (dict(seconds=True),dict(seconds=251),dict(seconds=float('nan')),
                     dict(preparation_seconds=121),dict(preparation_seconds=False)):
            with self.assertRaises(ValueError):self.new(**args)
        with self.assertRaises(ValueError):TransferNamespace(InputPlan(self.execution,None,None),self.bridge.exchange,self.records.append,download_name='TEST')
        with self.assertRaises(ValueError):self.stage.claim()
        self.assertEqual(self.bridge.calls,[])


if __name__ == '__main__':unittest.main()
