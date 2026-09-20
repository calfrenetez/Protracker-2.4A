from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from make_tremolo_fixtures import fixtures
from test_porta_emulator import decode_trace
COMMON=['src/core/pitch.c','src/core/flow.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class TremoloRender(unittest.TestCase):
    def test_native_range_driven_pcm(self):
        evidence=ROOT/'evidence/enhanced-editor/dev45/native'
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'offset'
            flags=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core']
            subprocess.run([*flags,'tests/render_tremolo_test.c','src/core/render.c','src/core/timeline.c','src/core/frame_clock.c','src/core/voice.c',*COMMON,'-o',str(binary)],cwd=ROOT,check=True)
            for name,data,meta in fixtures():
                for ch in range(4 if name=='trem_waves' else 1):
                    subprocess.run([str(binary),str(evidence/(name+'.mod')),str(evidence/(name+'0.log')),str(ch)],check=True)
