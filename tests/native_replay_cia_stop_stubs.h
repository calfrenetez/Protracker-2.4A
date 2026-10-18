/* Test-only register/API model. It is never a native ownership certificate. */
#ifndef PT_REPLAY_STOP_STUBS_H
#define PT_REPLAY_STOP_STUBS_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t UBYTE;
typedef const UBYTE *CONST_STRPTR;
typedef int16_t WORD;
typedef uint16_t UWORD;
#define NT_PROCESS 13
struct Library {unsigned lib_Version;};
struct Node {unsigned ln_Type;};
struct Task {struct Node tc_Node;void *tc_SPLower,*tc_SPUpper;};
struct ExecBase {struct Library LibNode;struct Task *ThisTask;signed char IDNestCnt,TDNestCnt;};
struct Interrupt {void (*is_Code)(void);void *is_Data;};
struct CIA {volatile UBYTE ciacra,ciacrb;};
extern struct ExecBase *SysBase;
extern uintptr_t pt_replay_stop_stub_sp;
extern volatile struct CIA pt_replay_stop_stub_hardware[2];
#define PT_REPLAY_STOP_HARDWARE(chip) (&pt_replay_stop_stub_hardware[(chip)])
void pt_replay_stop_stub_control_write(volatile UBYTE *,UBYTE);
#define PT_REPLAY_STOP_CONTROL_WRITE(control,value) pt_replay_stop_stub_control_write((control),(value))
void Disable(void);
void Enable(void);
void Wait(unsigned long);
struct Task *FindTask(const char *);
struct Library *OpenResource(CONST_STRPTR);
struct Interrupt *AddICRVector(struct Library *,unsigned,struct Interrupt *);
void RemICRVector(struct Library *,unsigned,struct Interrupt *);
WORD AbleICR(struct Library *,WORD);
WORD SetICR(struct Library *,WORD);
#endif
