from pathlib import Path
import hashlib,subprocess,tempfile,unittest,wave
from test_sampler import SOURCES
from test_bounce import EXTRA
from test_bounce_invert import INVERT
ROOT=Path(__file__).resolve().parents[1]
WORKFLOW_SOURCES=list(dict.fromkeys([*SOURCES[1:],*EXTRA,*INVERT,'src/core/stems.c','src/core/safe_save.c','src/platform/file_save.c','src/platform/project_file.c','src/platform/render_file.c','src/platform/stem_file.c','src/platform/render_invert_file.c']))

def reference_pcm(mod,trace,sample_rate=48000,gain=65536,record_bytes=164):
    data=bytearray(mod);offset=2108
    for i in range(31):
        size=int.from_bytes(data[42+30*i:44+30*i],'big')*2
        if size and int.from_bytes(data[48+30*i:50+30*i],'big')<=1:data[offset:offset+2]=b'\0\0'
        offset+=size
    frames=0;phase=end=last_trigger=clock=0;output=bytearray()
    for start in range(0,len(trace),record_bytes):
        r=trace[start:start+record_bytes]
        word=lambda n:int.from_bytes(r[n:n+2],'big')
        lng=lambda n:int.from_bytes(r[n:n+4],'big')
        if not r[14] or not word(28):continue
        loop=lng(58);length=word(62)*2
        assert length in (2,16)
        for ch in range(2 if record_bytes==188 else 1):
            begin=lng(58+22*ch);size=word(62+22*ch)*2
            assert size in (2,16)
            data[begin:begin+size]=r[148+24*ch:148+24*ch+size]
        if word(72)!=last_trigger:phase=lng(66)<<32;end=lng(66)+word(70)*2;last_trigger=word(72)
        clock+=(120000<<32)//word(12);tick_end=clock>>32
        while frames<tick_end:
            value=data[phase>>32];value=value if value<128 else value-256
            output+=(value*1024*r[32]*gain//65536).to_bytes(3,'little',signed=True)+b'\0\0\0'
            phase+=(sample_rate*428<<32)//(48000*word(44))
            if phase>>32>=end:phase=((phase-(end<<32))%(length<<32))+(loop<<32);end=loop+length
            frames+=1
    assert frames==17280
    return bytes(output)

def verify_outputs(folder,mod,trace,record_bytes=164):
    expected=reference_pcm(mod,trace,record_bytes=record_bytes)
    for path,audio in [('song.wav',expected),('stems/track-01.wav',expected),('stems/track-02.wav',bytes(len(expected))),('groups/group-01.wav',expected)]:
        with wave.open(str(folder/path),'rb') as wav:
            assert (wav.getnchannels(),wav.getsampwidth(),wav.getframerate(),wav.getnframes())==(2,3,48000,17280)
            assert wav.readframes(17281)==audio,path
    assert (folder/'master.ptg').read_bytes()==(folder/'undone.ptg').read_bytes()
    assert sorted(p.name for p in folder.iterdir())==['bounced.ptg','groups','master.ptg','song.wav','stems','undone.ptg']
    assert sorted(p.name for p in (folder/'stems').iterdir())==['track-01.wav','track-02.wav']
    assert [p.name for p in (folder/'groups').iterdir()]==['group-01.wav']
    return hashlib.sha256(expected).hexdigest()

class InvertHandoffWorkflows(unittest.TestCase):
    def test_reference_exports_and_master_transactions(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);exe=tmp/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/invert_handoff_workflow_test.c',*WORKFLOW_SOURCES,'-o',str(exe)],cwd=ROOT,check=True)
            cases=list((ROOT/'evidence/enhanced-editor/invert-handoff').glob('*.mod'))+list((ROOT/'evidence/enhanced-editor/invert-handoff-commands').glob('*.mod'))
            self.assertEqual(len(cases),10)
            for mod in sorted(cases):
                folder=tmp/mod.stem;folder.mkdir()
                subprocess.run([str(exe),str(mod),str(folder)],check=True)
                verify_outputs(folder,mod.read_bytes(),mod.with_suffix('.trace').read_bytes())

    def test_editor_default_rate_reference(self):
        folder=ROOT/'evidence/enhanced-editor/invert-handoff-workflows/host-reference'
        mod=(folder/'input.mod').read_bytes()
        trace=(ROOT/'evidence/enhanced-editor/invert-handoff/invert_swaponce.trace').read_bytes()
        expected=reference_pcm(mod,trace,8287,32768)
        for name in ['render.wav','track.wav','stems/track-01.wav']:
            with wave.open(str(folder/name),'rb') as wav:self.assertEqual(wav.readframes(17281),expected)
        with wave.open(str(folder/'cli-stems/track-01.wav'),'rb') as wav:
            self.assertEqual(wav.readframes(17281),reference_pcm(mod,trace,8287,65536))
