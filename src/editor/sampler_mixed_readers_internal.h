#ifndef PT_SAMPLER_MIXED_READERS_INTERNAL_H
#define PT_SAMPLER_MIXED_READERS_INTERNAL_H
#include "sampler_mixed_readers.h"

/* Separate private task operation. Exact original LIVE handle and fixed
 * factory controls precede one bounded actual queue readiness call. It returns
 * by value only, retains existing getter/service semantics and publishes no
 * key or READY/ACTIVE certificate. Callbacks may fault the existing owners.
 * Batch construction still repeats its genuine positive key/current gate. */
enum pt_mixed_readers_result pt_sampler_mixed_reader_readiness(
    struct pt_sampler_mixed_pool *,struct pt_sampler_mixed_reader_handle original);
#endif
