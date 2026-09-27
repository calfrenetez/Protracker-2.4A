#ifndef PT_SAMPLER_WAVETABLE_INTERNAL_H
#define PT_SAMPLER_WAVETABLE_INTERNAL_H
#include "sampler_wavetable.h"
/* Prepared immutable song only: full project/PCM validation and capability
 * analysis are complete, and expected is a genuine held pin for this exact
 * generation/cache version. The caller checks its owner/header/device identities
 * before begin. Retains an independent reference for the job; caller may unpin
 * its own reference afterwards. No promotion or value scans. All public job
 * lifetime/immutability rules apply. Not a public validation bypass. */
enum pt_cache_result pt_sampler_upload_begin_prepared(struct pt_sampler_upload_job *,struct pt_sampler_wavetable *,
    unsigned slot,unsigned generation,uint64_t version,struct pt_sample_version *expected,
    const struct pt_playback_format *,struct pt_cache_lease *);
#endif
