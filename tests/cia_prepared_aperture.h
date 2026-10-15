#ifndef PT_DIAGNOSTIC_CIA_PREPARED_APERTURE_H
#define PT_DIAGNOSTIC_CIA_PREPARED_APERTURE_H
#include "cia_aperture_model.h"
/* A separate diagnostic contract, not a faster implementation of the arbitrary
 * callback API. Task code owns/finalizes the complete synthetic key under the
 * same exclusion as arm. Descriptor/result/read storage remains private and
 * immovable until positive vector closure. No task or unrelated interrupt may
 * mutate it. The native loop calls only the fixed OS ReadEClock entry; it has
 * no caller callback, owner traversal, cancellation callback or queue access.
 * This contract does NOT qualify real sample-reader or scheduler ownership. */
struct pt_prepared_clock {
    uint32_t arm_hi,arm_lo,first_offset,last_offset,frequency,read_cap,gate;
};
struct pt_prepared_read {uint32_t hi,lo,frequency;};
struct pt_prepared_result {
    uint32_t result,reads,commits,shadow,before_index,after_index,reentry;
};
/* Preparation reuses original immutable epoch arithmetic and full key checks.
 * last-arm_at must fit the existing residency cap. Offsets are exact integer
 * differences, not narrowed clocks or calibrated lateness. Native subtraction
 * checks BOTH words, including a low-word carry, before comparing offsets.
 * No source/output overlap, write on invalid input or silent key refresh. */
enum pt_aperture_result pt_prepared_clock_bind(const struct pt_aperture_state *,
    const struct pt_aperture_control *,struct pt_prepared_clock *);
/* Pure replay oracle for complete actually recorded reads; never invokes a
 * clock. read_cap includes the immediate post-store read. Indices are 1-based;
 * zero means no such sample. Missing terminal input refuses without changing
 * output. Stale serialized admission uses zero reads/writes. A failure after
 * the shadow store retains commits=1. reentry is an observed latch, not a
 * prediction of where an asynchronous nested call would have arrived.
 * Native qualification requires a clear reentry latch; a late asynchronous
 * latch cannot be replayed as if it were present before a store. HOST replay
 * cannot prove assembly equivalence, clock latency or IRQ WCET. */
int pt_prepared_replay(const struct pt_prepared_clock *,
    const struct pt_prepared_read *,size_t,unsigned,struct pt_prepared_result *);
#endif
