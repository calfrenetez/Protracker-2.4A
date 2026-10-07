#ifndef PT_SAMPLER_MIXED_READERS_INTERNAL_H
#define PT_SAMPLER_MIXED_READERS_INTERNAL_H
#include "sampler_mixed_readers.h"
#include "../core/mixed_scheduled_readers_internal.h"

/* Separate private task operation. Exact original LIVE handle and fixed
 * factory controls precede one bounded actual queue readiness call. It returns
 * by value only, retains existing getter/service semantics and publishes no
 * key or READY/ACTIVE certificate. Callbacks may fault the existing owners.
 * Batch construction still repeats its genuine positive key/current gate. */
enum pt_mixed_readers_result pt_sampler_mixed_reader_readiness(
    struct pt_sampler_mixed_pool *,struct pt_sampler_mixed_reader_handle original);
/* Exact original mutable handle SLOT, not a supplied release certificate.
 * Captured numeric guards admit its whole span before a read/write or callback.
 * At most one core retirement call; a retained consumed R-first proof is not
 * repeated. Only the actual terminal+released factory holder clears this slot,
 * before its existing reader_free. Errors never erase retirement_consumed or actual
 * NULL consumption. Local unpublished queue retirement also settles an actual
 * terminal holder without any backend R call. No envelope validity or quiet is
 * asserted. No former project/master traversal during stale cleanup. */
struct pt_mixed_reader_retirement pt_sampler_mixed_retire_original_reader(
    struct pt_sampler_mixed_pool *,struct pt_sampler_mixed_reader_handle *original_slot,
    unsigned cancel);
#endif
