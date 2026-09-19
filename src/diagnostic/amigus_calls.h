/* GCC calls from the published, pinned amigus_lib.sfd register ABI.
 * Library stays replaceable; no driver implementation is linked here. */
#ifndef PT_AMIGUS_CALLS_H
#define PT_AMIGUS_CALLS_H
#include <amigus/amigus.h>
#include <inline/macros.h>
extern struct Library *AmiGUS_Base;
#define PT_FindCard(card) LP1(0x1e, struct AmiGUS *, PT_FindCard, \
    struct AmiGUS *, card, a0, , AmiGUS_Base)
#define PT_Reserve(card, flag, owner) LP3(0x24, ULONG, PT_Reserve, \
    struct AmiGUS *, card, a0, LONG, flag, d0, APTR, owner, d1, , AmiGUS_Base)
#define PT_Free(card, flag, owner) LP3NR(0x2a, PT_Free, \
    struct AmiGUS *, card, a0, LONG, flag, d0, APTR, owner, d1, , AmiGUS_Base)
#endif
