#ifndef PT_EXECBASE_STUB_H
#define PT_EXECBASE_STUB_H
#include <devices/audio.h>
struct Library{UWORD lib_Version;};
struct ExecBase{struct Library LibNode;ULONG ex_EClockFrequency;};
extern struct ExecBase *SysBase;
#endif
