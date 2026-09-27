from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class NativeEClock(unittest.TestCase):
    def test_owner_failures(self):
        self.run_fixture("native_eclock_test.c")
    def test_alarm_failures(self):
        self.run_fixture("native_alarm_test.c")
    def run_fixture(self,fixture):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)
            for folder in ['exec','devices','proto']:(p/folder).mkdir()
            (p/'exec/execbase.h').write_text('''#pragma once
#include <stdint.h>
typedef uint32_t ULONG;typedef int32_t LONG;
struct Library {unsigned lib_Version;};
struct ExecBase {struct Library LibNode;};extern struct ExecBase *SysBase;
struct Device {struct Library dd_Library;};struct MsgPort {unsigned mp_SigBit;};
struct IORequest {struct Device *io_Device;unsigned io_Command,io_Flags;int io_Error;};
''')
            (p/'devices/timer.h').write_text('''#pragma once
#include <exec/execbase.h>
#define TIMERNAME "timer.device"
#define UNIT_ECLOCK 2
#define UNIT_WAITECLOCK 4
#define TR_ADDREQUEST 9
struct timerequest {struct IORequest tr_node;struct {ULONG tv_secs,tv_micro;} tr_time;};
struct EClockVal {ULONG ev_hi,ev_lo;};
''')
            (p/'exec/errors.h').write_text('#define IOERR_ABORTED (-2)\n')
            (p/'proto/exec.h').write_text('''#include <exec/execbase.h>
struct MsgPort *CreateMsgPort(void);void DeleteMsgPort(struct MsgPort *);
struct IORequest *CreateIORequest(struct MsgPort *,unsigned long);void DeleteIORequest(struct IORequest *);
int OpenDevice(const char *,unsigned,struct IORequest *,unsigned);void CloseDevice(struct IORequest *);
struct IORequest *CheckIO(struct IORequest *);LONG WaitIO(struct IORequest *);void AbortIO(struct IORequest *);void SendIO(struct IORequest *);
''')
            (p/'proto/timer.h').write_text('''#include <devices/timer.h>
ULONG fake_read(struct Device *,struct EClockVal *);
#define ReadEClock(v) fake_read(TimerBase,v)
''')
            exe=p/'test'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+tmp,'tests/'+fixture,'-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe)],check=True)
