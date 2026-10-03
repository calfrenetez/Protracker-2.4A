import asyncio,importlib.util,json,tempfile
from pathlib import Path
p=Path(__file__).with_name('verify_idle_return.py');spec=importlib.util.spec_from_file_location('checked_return',p);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
root=Path(tempfile.mkdtemp(prefix='idle-mock-',dir=str(p.parent)))
original=[(19081,'owned original')]
class G:
 def __init__(self,*args):pass
 def command(self,c):
  return {'GET_CPU_MODEL':'OK\tmodel=68030','GET_STATUS':'OK\tPaused=false','GET_AUDIO_STATE':'OK\tch0_dma=0\tch1_dma=0\tch2_dma=0\tch3_dma=0'}[c]
m.Guest=G;m.guard=lambda *_:original;m.processes=lambda:original
clock=[0.0]
m.time.monotonic=lambda:clock[0]
async def sleep(n):clock[0]+=n+0.001
m.asyncio.sleep=sleep
r={'passed':False,'observations':[]};out=root/'normal';out.mkdir();asyncio.run(m.verify(out,original,[],r));assert r['passed'] and len(r['observations'])==11 and r['observations'][-1]['elapsed_seconds']>10
m.processes=lambda:[(999,'unexpected conflict')]
r={'passed':False,'observations':[]};out=root/'conflict';out.mkdir()
try:asyncio.run(m.verify(out,original,[],r))
except AssertionError as e:assert str(e)=='Captured process conflict'
else:raise AssertionError('Captured conflict not refused')
assert not r['passed'] and len(r['observations'])==1
r={'passed':False,'observations':[]};out=root/'active-dma';out.mkdir();m.processes=lambda:original
class Active(G):
 def command(self,c):return 'OK\tch0_dma=1\tch1_dma=0\tch2_dma=0\tch3_dma=0' if c=='GET_AUDIO_STATE' else super().command(c)
m.Guest=Active
try:asyncio.run(m.verify(out,original,[],r))
except AssertionError as e:assert str(e)=='Original guest DMA not off'
else:raise AssertionError('Active DMA not refused')
assert not r['passed'] and len(r['observations'])==1
report={'passed':True,'scope':'Host-only mock; no locks/process health/Guest/target operation','cases':['11 stable observations >10s','captured conflicting process refusal persisted','active DMA refusal persisted'],'mock_root':str(root)}
(root.parent/'idle-checker-mocks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
