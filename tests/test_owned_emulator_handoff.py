"""Actual upload-stage ledgers and adverse Root receipt models; no targets."""
import copy
import hashlib
import json
import unittest

from owned_native_input import InputPlan, InputStage
from owned_native_run import Plan
from owned_emulator_handoff import make_receipt
from owned_transfer_receipt import verify_receipt
from test_owned_native_input import InputBridge


class HandoffTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.execution = Plan('b2' * 16, 123, b'MODELED BINARY' * 7000, ('model PASS',),
                              copy_source='RAM:PT24G-' + 'b2' * 8 + '-in/NativeTest')
        self.plan = InputPlan(self.execution, None, None, external_transfer=True)
        upload = InputPlan(self.execution, None, None)
        self.bridge = InputBridge(upload)
        self.bridge.files.pop(None)
        self.records = []
        self.stage = InputStage(upload, self.bridge.exchange, self.records.append, preparation_seconds=120)
        self.transfer = await self.stage.prepare()
        self.parent = [{'command': {'type': 'LISTDIR', 'path': 'RAM:', 'offset': 0},
                        'raw': 'DIR|RAM:|2|ENV(0,D),' + self.plan.directory.split(':')[1] + '(0,D)'}]
        self.tasks = 'Tasks (1):\nWorkbench'
        self.kwargs = dict(native_plan_sha256='e6' * 32, owner_run_id='d5' * 16,
                           download_name='PT24G-TEST', now=100.0)

    def make(self, **changes):
        args = dict(plan=self.plan, transfer=self.transfer, records=self.records,
                    parent_records=self.parent, tasks=self.tasks, **self.kwargs)
        return make_receipt(**{**args, **changes})

    async def test_complete_actual_stage_records_produce_exact_emulator_only_receipt(self):
        before = copy.deepcopy(self.bridge.calls)
        result = self.make()
        self.assertEqual(self.bridge.calls, before)
        witnesses = {name: result['root-transfer-' + name + '.json']
                     for name in ('namespace', 'complete', 'placement')}
        self.assertTrue(verify_receipt(result['root-transfer.json'], witnesses,
            expected_sha256=hashlib.sha256(result['root-transfer.json']).hexdigest(),
            plan=self.plan, target='amiberry-030', **{k:v for k,v in self.kwargs.items()}))
        self.assertEqual(json.loads(witnesses['complete'])['method'], 'OWNED_EMULATOR_TRANSFER')
        self.assertFalse(any(c['type'] in ('LAUNCH','DELETEFILE') for c in before))

    async def test_every_missing_reordered_or_changed_upload_record_refuses(self):
        for i in range(len(self.records)):
            with self.subTest(position=i):
                records = copy.deepcopy(self.records); del records[i]
                with self.assertRaises(ValueError): self.make(records=records)
                records = copy.deepcopy(self.records); records[i]['extra'] = True
                with self.assertRaises(ValueError): self.make(records=records)
                records = copy.deepcopy(self.records)
                records[i],records[(i+1)%len(records)] = records[(i+1)%len(records)],records[i]
                with self.assertRaises(ValueError): self.make(records=records)

    async def test_fresh_namespace_write_ack_body_checksum_and_inventory_are_required(self):
        for old,new in (('OK|MAKEDIR|','ERR|MAKEDIR|'), ('|0|','|1|foreign(1,F)'),
                        ('OK|WRITEFILE|','OK|WRITEFILE|0'), ('CHECKSUM|','WRONG|'),
                        ('NativeTest(', 'foreign(')):
            records=copy.deepcopy(self.records)
            index=next(i for i,r in enumerate(records) if r.get('event')=='input_reply' and old in r['raw'])
            records[index]['raw']=records[index]['raw'].replace(old,new,1)
            with self.assertRaises(ValueError):self.make(records=records)
        records=copy.deepcopy(self.records)
        index=next(i for i,r in enumerate(records) if r.get('command',{}).get('type')=='WRITEFILE')
        records[index]['command']['sha256']='f7'*32
        with self.assertRaises(ValueError):self.make(records=records)

    async def test_unexpected_followup_or_only_matching_file_cannot_prove_transfer(self):
        for records in ([], self.records[3:], self.records+[{'event':'followup'}]):
            with self.assertRaises(ValueError):self.make(records=records)
        for transfer in ({},dict(self.transfer,status='UNCONFIRMED')):
            with self.assertRaises(ValueError):self.make(transfer=transfer)
        with self.assertRaises(ValueError):self.make(plan=InputPlan(self.execution,None,None))

    async def test_complete_paginated_parent_accepts_and_incomplete_reordered_duplicate_refuses(self):
        prefix=self.plan.directory.split(':')[1]
        good=[{'command':{'type':'LISTDIR','path':'RAM:','offset':0},'raw':'DIR|RAM:|2|ENV(0,D)'},
              {'command':{'type':'LISTDIR','path':'RAM:','offset':1},'raw':'DIR|RAM:|2|'+prefix+'(0,D)'}]
        self.make(parent_records=good)
        for records in ([],good[:1],good[::-1],good+good[-1:],
                        [good[0],dict(good[1],raw='DIR|RAM:|2|ENV(0,D)')]):
            with self.assertRaises(ValueError):self.make(parent_records=records)

    async def test_parent_must_have_input_directory_and_no_execution_or_download(self):
        name=self.plan.directory.split(':')[1]
        for raw in ('DIR|RAM:|0|','DIR|RAM:|1|'+name+'(0,F)',
                    'DIR|RAM:|2|'+name+'(0,D),PT24G-TEST(1,F)',
                    'DIR|RAM:|2|'+name+'(0,D),'+self.execution.directory.split(':')[1]+'(0,D)'):
            with self.assertRaises(ValueError):self.make(parent_records=[dict(self.parent[0],raw=raw)])

    async def test_reader_task_and_wrong_owner_or_receipt_binding_refuse(self):
        for tasks in ('No tasks found','Tasks (1):\nNativeTest','Tasks (2):\nPT24G-TEST',
                      'Tasks (1):\n'+self.execution.directory.split(':')[1]):
            with self.assertRaises(ValueError):self.make(tasks=tasks)
        for key,value in (('owner_run_id','OTHER'),('native_plan_sha256','changed'),
                          ('download_name','../other'),('now',float('nan'))):
            with self.assertRaises(ValueError):self.make(**{key:value})

    async def test_numeric_true_cannot_replace_order_or_byte_count(self):
        records=copy.deepcopy(self.records);records[1]['number']=True
        with self.assertRaises(ValueError):self.make(records=records)
        transfer=copy.deepcopy(self.transfer);transfer['files']['NativeTest']['bytes']=True
        with self.assertRaises(ValueError):self.make(transfer=transfer)


if __name__ == '__main__': unittest.main()
