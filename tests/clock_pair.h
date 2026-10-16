#ifndef PT_DIAGNOSTIC_CLOCK_PAIR_H
#define PT_DIAGNOSTIC_CLOCK_PAIR_H
#include <stdint.h>
/* Saved observations only. This is not an output/scheduling admission API. */
struct pt_clock_sample {uint32_t hi,lo,frequency;};
struct pt_clock_pair {struct pt_clock_sample first,second;};
enum pt_clock_pair_status {PT_CLOCK_PAIR_OK,PT_CLOCK_PAIR_RATE,PT_CLOCK_PAIR_REGRESSION};
struct pt_clock_pair_result {uint32_t status,delta_hi,delta_lo;};
/* Invalid/null/overlapping extents leave output untouched. Valid observations
 * retain invalid-clock status; a backwards counter is never modular elapsed. */
int pt_clock_pair_analyse(const struct pt_clock_pair *,struct pt_clock_pair_result *);
#endif
