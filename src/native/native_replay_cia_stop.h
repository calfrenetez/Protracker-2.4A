/* PRIVATE SOURCE PROPOSAL. Not compiled or native-qualified. */
#ifndef PT_PRIVATE_NATIVE_REPLAY_CIA_STOP_H
#define PT_PRIVATE_NATIVE_REPLAY_CIA_STOP_H
#if !defined(PT_REPLAY_STOP_PRIVATE_SOURCE) || PT_REPLAY_STOP_PRIVATE_SOURCE != 1
#error "This unqualified replay-stop proposal requires explicit private selection"
#endif
#ifdef PT_REPLAY_STOP_STUB
#include "../../tests/native_replay_cia_stop_stubs.h"
#else
#include <stdint.h>
#include <limits.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/nodes.h>
#include <exec/tasks.h>
#include <hardware/cia.h>
#include <proto/exec.h>
#define CIA_BASE_NAME cia_resource
#include <proto/cia.h>
#endif

/* Called ONLY at the generated replay's actual AddICRVector call site. The
 * function itself calls AddICRVector once; it never adopts a supplied ownership
 * flag or a pre-populated vector record. Arguments are the original register
 * arguments plus the original chip address. Successful actual acquisition is
 * recorded under the same Disable/Enable pair, before callbacks can resume.
 * Result is zero on actual success, otherwise the actual foreign pointer bits
 * or a nonzero local refusal sentinel. This scalar result uses the ordinary C
 * scalar ABI rather than assuming a compiler's pointer-return register.
 * This prototype is ONE-SHOT; repeat-PLAY generation support is a later slice.
 */
uintptr_t pt_native_replay_add_cia_vector(struct Library *,unsigned,
    struct Interrupt *,volatile struct CIA *);

/* 1 means this exact acquired producer completed the proposed native source
 * protocol. -1 is sticky UNKNOWN, never retried. Documentation/API semantics,
 * single-CPU normal-Process exclusion and native readback still need exact
 * candidate qualification. This does NOT prove Paula DMA or audio retirement.
 */
int pt_native_replay_cia_retire(void);
#endif
