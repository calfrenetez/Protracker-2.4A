from pathlib import Path
import subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_editor_wavetable import prepare
from test_editor_paula import PAULA
class NativePaulaTransport(unittest.TestCase):
    def test_timer_device_barrier(self):
        with tempfile.TemporaryDirectory() as tmp:
            sources,tree=prepare(tmp);p=Path(tmp);stub=p/'tests/audio_stub'
            audio=stub/'devices/audio.h'
            audio.write_text(audio.read_text().replace('struct Device{int unused;};','struct Device{struct {UWORD lib_Version;} dd_Library;};').replace('struct MsgPort{unsigned mp_SigBit;};','struct Task;struct MsgPort{unsigned mp_SigBit;struct Task *mp_SigTask;};'))
            (stub/'devices/timer.h').write_text('''#pragma once
#include <devices/audio.h>
#define TIMERNAME "timer.device"
#define UNIT_ECLOCK 2
#define UNIT_WAITECLOCK 4
#define TR_ADDREQUEST 9
struct timerequest {struct IORequest tr_node;struct {ULONG tv_secs,tv_micro;} tr_time;};
struct EClockVal {ULONG ev_hi,ev_lo;};
''')
            (stub/'exec/errors.h').write_text('#define IOERR_ABORTED (-2)\n')
            with (stub/'proto/exec.h').open('a') as f:f.write('\nvoid SendIO(struct IORequest *);\nstruct Task;struct Task *FindTask(const char *);\nULONG SetSignal(ULONG,ULONG);ULONG Wait(ULONG);\n')
            (stub/'proto/timer.h').write_text('''#include <devices/timer.h>
ULONG fake_read(struct Device *,struct EClockVal *);
#define ReadEClock(v) fake_read(TimerBase,v)
''')
            exe=p/'test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-Itests/audio_stub','-Isrc/core',
                'tests/native_paula_transport_test.c',*list(dict.fromkeys([*PAULA,*sources])),
                '-o',str(exe)],cwd=p,check=True)
            subprocess.run([str(exe)],check=True)
