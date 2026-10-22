"""Injected HOST endpoints only. No original harness import or actual target."""
import asyncio
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from editor_blank_capture_protocol import BLANK_FRAME, recent_line
from editor_mouse_capture_protocol import SELECTED_FRAME
from editor_mouse_probe_case import run_selection, run_editor_mouse_probe, ALLOWED, LIMITS


def picture(index):
    return {'captureID':'fixture-capture-'+str(index),'controllerID':'fixture-controller',
            'captureNonce':'fixture-nonce-'+str(index),
            'geometry':{'path':'fixture-'+str(index),'capture_nonce':'fixture-nonce-'+str(index),
                        'runtime_id':1,'geometry_revision':2,'monitor_id':0},
            'metadata':{'actionable':True,'coordinate_space':'screenshot_pixels',
                        'image_width':756,'image_height':576,
                        'actionable_bounds':{'x':0,'y':0,'width':756,'height':576}}}


class FakeAPI:
    def __init__(self):
        self.now=100.;self.events=[];self.options={};self.clicked=False
        self.baseline=(recent_line('blank-capture-firstmouse-r1')+b'\n'
                       b'EDITOR READY channels=4 patterns=1 bitmap_bytes=163840 history_bytes=182388\n'
                       +BLANK_FRAME+b'\n')
    def clock(self):return self.now
    async def sleep(self,n):self.now+=n
    def owner(self,runid):assert runid=='fixture-owner'
    def pinned(self,ref):
        if self.options.get('badpin'):raise RuntimeError('fixture input drift')
        return b'fixture'
    def save(self,path,value):
        with Path(path).open('x') as f:json.dump(value,f)
    def blank_log(self):
        if self.options.get('changedmove') and 'move' in self.events:
            return self.baseline+b'EDITOR REPLAY active=1\n'
        if self.clicked and not self.options.get('noselection'):
            line=SELECTED_FRAME
            if self.options.get('wrongrow'):line=line.replace(b'row=4',b'row=3')
            tail=line+b'\n'
            if self.options.get('changedcapture') and 'after-click-capture' in self.events:tail*=2
            return self.baseline+tail
        return self.baseline
    def log_snapshot(self,out,body):pass
    async def root_observation(self,out,stage,capture,point,deadline):
        self.events.append(stage+'-observation')
        if self.options.get('rootdeadline'):self.now+=241
        if stage=='before-move':return {'editorBlankAndUnmodal':True,'screenRectangle':[76,38,640,512]}
        if stage=='after-move':
            tip=self.options.get('tip',[294,462])
            return {'pointerTipIdentifiable':tip is not None,'observedPointerTip':tip}
        return {'row4Channel1CursorVisible':not self.options.get('wrongvisual'),
                'noUnexpectedModalOrContentChange':True}
    async def state_query(self,out,label,deadline):
        self.events.append(label+'-state')
        state={'geometry_valid':True,'runtime_id':1,'geometry_revision':2,'monitor_id':0,
               'button_mask':0,'focus_ready':True,'pending_effective_diverged':False,
               'pending_tablet_mode':1,'effective_tablet_mode':1,
               'pending_mouse_untrap':False,'effective_mouse_untrap':False}
        if self.options.get('badfocus'):state['focus_ready']=False
        if self.options.get('heldafter') and label=='after-move':state['button_mask']=1
        if self.options.get('settings') and label=='after-move':state['effective_tablet_mode']=2
        return state
    def ack(self,kind,capture,point,request):
        return {'ok':True,'code':'ok','execution_state':'completed','cleanup_state':'confirmed',
                'failure_phase':None,'next_action':'none','deduplicated':False,'action':kind,
                'capture_id':capture['captureID'],'controller_id':capture['controllerID'],
                'request_id':request,'requested_coordinates':[{'x':point[0],'y':point[1]}]}
    async def move(self,capture,point,request,deadline):
        self.events.append('move')
        d=self.ack('move',capture,point,request)
        if self.options.get('badmove'):d['deduplicated']=True
        if self.options.get('api_unready'):
            d.update(ok=False,code='input_not_ready',execution_state='not_started')
        return d
    async def click(self,capture,point,request,deadline):
        self.events.append('click');self.clicked=True
        d=self.ack('click',capture,point,request)
        if self.options.get('badclick'):d['cleanup_state']='unknown'
        return d
    async def capture(self,path,deadline):
        kind='after-click-capture' if self.clicked else 'after-move-capture'
        self.events.append(kind)
        d=picture(3 if self.clicked else 2)
        if self.options.get('mapping'):d['geometry']['geometry_revision']=3
        return d

    async def passive_pointer(self,out,deadline):
        self.events.append('passive-probe')
        if self.options.get('probe-failure'):
            raise RuntimeError('synthetic guest mismatch')
        if self.options.get('probe-deadline'): self.now += 21
        return {'state':'PASSIVE_EDITOR_V2_DATA_RECORDED', 'readbackAttempts':1,
                'executionAttempts':1, 'inputAuthorized':False, 'hotspotMappingAccepted':False,
                'screen':{'mouse_x':219 if self.options.get('native-mismatch') else 218,'mouse_y':424}}


class MousePhaseTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.out=Path(self.temp.name)/'phase'
        self.api=FakeAPI()
        self.b={'state':'ROOT_ADMITTED_EDITOR_MOUSE_WITH_PASSIVE_V2','acceptedByRoot':True,
                'allowedActions':copy.deepcopy(ALLOWED),'limits':copy.deepcopy(LIMITS),
                'nativeSize':[640,512],'ownerRunID':'fixture-owner','sourcePins':{'fake':{}},
                'mountedCandidate':{},'runLabel':'blank-capture-firstmouse-r1'}
        self.blank={'state':'BLANK_LOG_AND_REGISTERED_CAPTURE_RECORDED','ownerRunID':'fixture-owner',
                    'attempts':{'candidate-readback':1,'environment-setup':1,'application-launch':1,'capture':1},
                    'capture':picture(1)}
    def tearDown(self):self.temp.cleanup()
    def run_phase(self):return asyncio.run(run_selection(self.b,self.blank,self.out,self.api))
    def failed(self,option):
        self.api.options[option]=True
        with self.assertRaises((RuntimeError,ValueError)):self.run_phase()
        self.assertEqual(json.loads((self.out/'result.json').read_text())['state'],'FIRST_MOUSE_PHASE_FAILED_RETAINED')

    def test_exact_selection_once(self):
        result=self.run_phase()
        self.assertEqual(result['state'],'ROW4_CHANNEL1_MOUSE_SELECTION_RECORDED')
        self.assertEqual(result['attempts'],{'move':1,'click':1,'capture':2,'state':3,'probeInvocation':1})
        self.assertTrue(result['selectionAccepted'])
        self.assertLess(self.api.events.index('move'),self.api.events.index('passive-probe'))
        self.assertLess(self.api.events.index('passive-probe'),self.api.events.index('after-move-capture'))
        self.assertLess(self.api.events.index('after-move-observation'),self.api.events.index('click'))
    def test_pointer_mismatch_is_recorded_without_click(self):
        self.api.options['tip']=[295,462]
        result=self.run_phase()
        self.assertEqual(result['state'],'POINTER_MISMATCH_RECORDED_NO_CLICK')
        self.assertFalse(result['selectionAccepted']);self.assertNotIn('click',self.api.events)
    def test_native_pointer_mismatch_vetoes_even_an_exact_image_tip(self):
        self.api.options['native-mismatch']=True
        result=self.run_phase()
        self.assertEqual(result['state'],'NATIVE_POINTER_MISMATCH_RECORDED_NO_CLICK')
        self.assertEqual(result['nativePointer'],[219,424])
        self.assertEqual(result['sourceNativePoint'],[218,424])
        self.assertIn('after-move-observation',self.api.events)
        self.assertFalse(result['selectionAccepted']);self.assertNotIn('click',self.api.events)
    def test_unidentified_pointer_never_clicks(self):
        self.api.options['tip']=None;result=self.run_phase()
        self.assertEqual(result['state'],'POINTER_MISMATCH_RECORDED_NO_CLICK')
        self.assertNotIn('click',self.api.events)
    def test_numeric_equality_cannot_hide_fractional_pointer(self):
        self.api.options['tip']=[294.0,462.0]
        with self.assertRaises(RuntimeError):self.run_phase()
        self.assertNotIn('click',self.api.events)
    def test_outside_transaction_focus_false_is_diagnostic(self):
        self.api.options['badfocus']=True
        result=self.run_phase()
        self.assertEqual(result['state'],'ROW4_CHANNEL1_MOUSE_SELECTION_RECORDED')
        self.assertEqual(self.api.events.count('move'),1)
    def test_original_api_readiness_refusal_still_prevents_click(self):
        self.api.options['badfocus']=True
        self.failed('api_unready')
        self.assertEqual(self.api.events.count('move'),1)
        self.assertNotIn('click',self.api.events)
    def test_source_pin_refusal_before_observation(self):
        self.failed('badpin');self.assertEqual(self.api.events,[])
    def test_move_ack_refusal_never_probes_or_clicks(self):
        self.failed('badmove');self.assertEqual(self.api.events.count('move'),1)
        self.assertNotIn('passive-probe',self.api.events);self.assertNotIn('click',self.api.events)
    def test_probe_failure_never_captures_or_clicks_or_replays(self):
        self.failed('probe-failure')
        self.assertEqual(self.api.events.count('passive-probe'),1)
        self.assertNotIn('after-move-capture',self.api.events);self.assertNotIn('click',self.api.events)
        with self.assertRaises(RuntimeError):self.run_phase()
        self.assertEqual(self.api.events.count('move'),1)
    def test_old_mouse_admission_cannot_borrow_probe_scope(self):
        self.b['state']='ROOT_ADMITTED_CURRENT_EDITOR_FIRST_MOUSE_ONLY'
        with self.assertRaises(RuntimeError):self.run_phase()
        self.assertFalse(self.out.exists());self.assertEqual(self.api.events,[])

    def test_mapping_change_never_clicks(self):
        self.failed('mapping');self.assertNotIn('click',self.api.events)
    def test_held_button_never_clicks(self):
        self.failed('heldafter');self.assertNotIn('click',self.api.events)
    def test_settings_restoration_refusal_never_clicks(self):
        self.failed('settings');self.assertNotIn('click',self.api.events)
    def test_unexpected_activity_before_click_refuses(self):
        self.failed('changedmove');self.assertNotIn('click',self.api.events)
    def test_click_ack_refusal_never_repeats(self):
        self.failed('badclick');self.assertEqual(self.api.events.count('click'),1)
    def test_wrong_selected_row_refuses(self):
        self.failed('wrongrow');self.assertEqual(self.api.events.count('click'),1)
    def test_selection_log_deadline_never_repeats_click(self):
        self.failed('noselection');self.assertEqual(self.api.events.count('click'),1)
    def test_final_capture_requires_unchanged_selected_log(self):
        self.failed('changedcapture');self.assertEqual(self.api.events.count('click'),1)
    def test_final_visual_refusal_retained(self):
        self.failed('wrongvisual');self.assertEqual(self.api.events.count('click'),1)
    def test_root_deadline_no_move(self):
        self.failed('rootdeadline');self.assertNotIn('move',self.api.events)
    def test_old_owner_admission_refuses(self):
        self.blank['ownerRunID']='other-owner'
        with self.assertRaises(RuntimeError):self.run_phase()
        self.assertFalse(self.out.exists());self.assertEqual(self.api.events,[])

    def composition(self):
        original = SimpleNamespace(frame=object(),binding={'ownerRunID':'fixture-owner'})
        self.api.original=original; self.api.frame=original.frame; self.api.binding=self.b
        self.b['passiveProbe']={'ownerRunID':self.b['ownerRunID'],
                                'sourcePins':self.b['sourcePins'],
                                'editorCandidate':self.b['mountedCandidate']}
        return original

    def test_parent_composes_same_original_frame_and_preserves_root_click_gate(self):
        original=self.composition()
        fakeprobe=SimpleNamespace(run=self.api.passive_pointer)
        self.api.options['tip']=[295,463]
        with patch('editor_pointer_probe.EditorPointerProbe',return_value=fakeprobe) as factory:
            result=asyncio.run(run_editor_mouse_probe(self.b,self.blank,self.out,self.api,original))
        factory.assert_called_once_with(self.b['passiveProbe'],original)
        self.assertEqual(result['state'],'POINTER_MISMATCH_RECORDED_NO_CLICK')
        self.assertNotIn('click',self.api.events)

    def test_parent_frame_or_probe_owner_change_refuses_before_phase(self):
        original=self.composition();self.api.frame=object()
        with self.assertRaises(RuntimeError):
            asyncio.run(run_editor_mouse_probe(self.b,self.blank,self.out,self.api,original))
        self.api.frame=original.frame;self.b['passiveProbe']['ownerRunID']='other'
        with self.assertRaises(RuntimeError):
            asyncio.run(run_editor_mouse_probe(self.b,self.blank,self.out,self.api,original))
        self.assertFalse(self.out.exists());self.assertEqual(self.api.events,[])


if __name__=='__main__':unittest.main()
