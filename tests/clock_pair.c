#include "clock_pair.h"
#include <stddef.h>
int pt_clock_pair_analyse(const struct pt_clock_pair *pair,struct pt_clock_pair_result *out)
{
    uintptr_t p=(uintptr_t)pair,o=(uintptr_t)out;
    struct pt_clock_pair_result result={PT_CLOCK_PAIR_OK,0,0};uint64_t first,second,delta;
    if(!pair || !out || p>UINTPTR_MAX-sizeof(*pair) || o>UINTPTR_MAX-sizeof(*out) ||
       (p<o+sizeof(*out) && o<p+sizeof(*pair)))return 0;
    first=((uint64_t)pair->first.hi<<32)|pair->first.lo;
    second=((uint64_t)pair->second.hi<<32)|pair->second.lo;
    if(!pair->first.frequency || pair->first.frequency!=pair->second.frequency)result.status=PT_CLOCK_PAIR_RATE;
    else if(second<first)result.status=PT_CLOCK_PAIR_REGRESSION;
    else {delta=second-first;result.delta_hi=(uint32_t)(delta>>32);result.delta_lo=(uint32_t)delta;}
    *out=result;return 1;
}
