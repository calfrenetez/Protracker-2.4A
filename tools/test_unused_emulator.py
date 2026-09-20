#!/usr/bin/env python3
"""Reserve a private emulator window before the unused-command parity suite."""
import json,subprocess
from pathlib import Path
from build_diagnostic import ROOT,digest
from make_unused_fixtures import fixtures
from test_porta_emulator import main
if __name__=='__main__':
    processes=subprocess.check_output(['ps','ax','-o','pid=,comm='],text=True)
    assert not any(s.lower().endswith('/amiberry') for s in processes.splitlines())
    holders=subprocess.run(['lsof',str(ROOT/'local/baseline.hdf')],capture_output=True,text=True)
    assert holders.returncode==1 and not holders.stdout and not Path('/tmp/amiberry.sock').exists()
    assert digest(ROOT/'local/share/launch')=='9c801b94c85b06cd124fe2d611f185bfe135c8c7442f8263a761c79102f8ddf3'
    manifest=json.loads((ROOT/'build/dev/core-build.json').read_text())
    assert all(digest(ROOT/p)==h for p,h in manifest['sources'].items())
    main(fixtures,'unused','Pinned2.3F8xx stored-period restoration and E8x no-op, including vibrato carry, wrapped periods, notes and pattern delays. Native traces repeated exactly and compared to m68k state and reference PCM. No modern panning/Karplus effect or physical acceptance claim.')
