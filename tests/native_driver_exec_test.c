/* Own temporary library only; never registers AmiGUS.audio, opens AHI, or
 * accesses card hardware. Exercises the production Exec callbacks. */
#include "native_driver_window.h"
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Any unexpected fixture failure retains its code/storage/Task, because a
 * registered vector must never point into an exited process. */
#undef assert
#define assert(condition) do { if (!(condition)) { \
    puts("DRIVER EXEC HOLD: fixture invariant failed; Task/storage retained"); \
    fflush(stdout); for (;;) Delay(50); } } while (0)

static unsigned int opens, closes, expunges;
static const char name[] = "ptg-idle-guard-fixture.library";
static const char id[] = "ptg private fixture\r\n";
extern void fixture_open(void), fixture_close(void), fixture_expunge(void), fixture_null(void);
struct Library *fixture_open_c(struct Library *lib)
{ ++opens; ++lib->lib_OpenCnt; lib->lib_Flags &= ~LIBF_DELEXP; return lib; }
unsigned long fixture_close_c(struct Library *lib)
{ assert(lib->lib_OpenCnt); ++closes; --lib->lib_OpenCnt; return 0; }
unsigned long fixture_expunge_c(struct Library *lib)
{ assert(!lib->lib_OpenCnt); ++expunges; Remove((struct Node *)lib); return 0; }
__asm__(".text\n.even\n"
".globl _fixture_open\n_fixture_open:\nmovem.l %d2-%d7/%a2-%a6,-(%sp)\nmove.l %a6,-(%sp)\njsr _fixture_open_c\naddq.l #4,%sp\nmovem.l (%sp)+,%d2-%d7/%a2-%a6\nrts\n"
".globl _fixture_close\n_fixture_close:\nmovem.l %d2-%d7/%a2-%a6,-(%sp)\nmove.l %a6,-(%sp)\njsr _fixture_close_c\naddq.l #4,%sp\nmovem.l (%sp)+,%d2-%d7/%a2-%a6\nrts\n"
".globl _fixture_expunge\n_fixture_expunge:\nmovem.l %d2-%d7/%a2-%a6,-(%sp)\nmove.l %a6,-(%sp)\njsr _fixture_expunge_c\naddq.l #4,%sp\nmovem.l (%sp)+,%d2-%d7/%a2-%a6\nrts\n"
".globl _fixture_null\n_fixture_null:\nmoveq #0,%d0\nrts\n");
static void vector(unsigned char *base, unsigned int offset, void (*fn)(void))
{
    unsigned long a = (unsigned long)fn;
    unsigned char *p = base-offset;
    p[0]=0x4e; p[1]=0xf9; p[2]=(unsigned char)(a>>24);
    p[3]=(unsigned char)(a>>16); p[4]=(unsigned char)(a>>8); p[5]=(unsigned char)a;
}
int main(void)
{
    unsigned char *memory;
    struct Library *lib;
    struct pt_native_driver driver={name,name,id,4,23};
    struct pt_driver_api api=pt_native_driver_api(&driver);
    struct pt_driver_window window;
    struct pt_driver_snapshot s;
    int conflict;
    Forbid(); conflict=FindName(&SysBase->LibList,(STRPTR)name)!=NULL; Permit();
    assert(!conflict);
    api.enter(api.context); s=api.snapshot(api.context); api.leave(api.context);
    if (s.ahi_users) { puts("DRIVER EXEC SKIP: AHI in use"); return PT_SKIP; }
    memory=AllocMem(24+sizeof(*lib),MEMF_PUBLIC|MEMF_CLEAR);
    assert(memory); lib=(struct Library *)(memory+24);
    vector((unsigned char *)lib,6,fixture_open);
    vector((unsigned char *)lib,12,fixture_close);
    vector((unsigned char *)lib,18,fixture_expunge);
    vector((unsigned char *)lib,24,fixture_null);
    lib->lib_Node.ln_Name=(char *)name; lib->lib_Node.ln_Type=NT_LIBRARY;
    lib->lib_IdString=(APTR)id; lib->lib_Version=4; lib->lib_Revision=23;
    lib->lib_NegSize=24; lib->lib_PosSize=sizeof(*lib);
    CacheClearU(); AddLibrary(lib);
    lib->lib_OpenCnt=1;
    window=pt_driver_begin(&api);
    assert(window.result==PT_SKIP && !window.restore_needed && !expunges);
    lib->lib_OpenCnt=0;
    lib->lib_Revision=24;
    window=pt_driver_begin(&api);
    assert(window.result==PT_SKIP && !window.restore_needed && !expunges);
    lib->lib_Revision=23;
    window=pt_driver_begin(&api);
    assert(window.result==PT_PASS && window.unloaded && window.restore_needed && expunges==1);
    /* Supply the owned fixture's reload. The production restore then uses
     * real OpenLibrary/CloseLibrary and checks the fresh resident snapshot. */
    AddLibrary(lib);
    assert(pt_driver_end(&api,&window) && window.restored && !window.restore_needed);
    assert(opens==1 && closes==1);
    api.enter(api.context); s=api.snapshot(api.context);
    assert(s.present && s.supported && !s.driver_users && !s.delayed_expunge);
    api.remove_idle(api.context); s=api.snapshot(api.context); api.leave(api.context);
    assert(!s.present && expunges==2);
    FreeMem(memory,24+sizeof(*lib));
    puts("DRIVER EXEC PASS: own-library refusal unload open-close restoration absence Fast-zero");
    return 0;
}
