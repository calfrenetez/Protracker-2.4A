#ifndef PT_SAMPLER_WAVETABLE_INTERNAL_H
#define PT_SAMPLER_WAVETABLE_INTERNAL_H
#include "sampler_wavetable.h"
/* Source-bridge metadata query for an already-validated immutable project and
 * genuine live bridge/sampler controls. Serialized, no callbacks or edits during
 * the query. Reads only fixed controls/header fields; no sample/order/event/PCM
 * values, allocation, promotion, cache retirement, pins or backend callbacks.
 * Not complete project validation, a semantic certificate, or backend/device
 * ownership, availability, capacity or completion proof. Metadata may still match
 * a closing/faulted/unowned backend. Keep the appropriate live ownership gate
 * separate; callers must not treat this result as pt_amigus_wavetable_cache_current.
 * No public upload or mixed-owner/editor PLAY path is switched to this query. */
int pt_sampler_wavetable_prepared_metadata_current(const struct pt_sampler_wavetable *);
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
