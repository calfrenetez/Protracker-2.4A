#ifndef PT_PLAYBACK_INTERNAL_H
#define PT_PLAYBACK_INTERNAL_H
#include "amigus_wavetable_cache.h"
/* Private immutable prepared-song path ONLY. Caller has fully validated PCM,
 * checked exact current version/descriptor and retains its master pin through
 * the entire call. Values remain immutable; callbacks cannot edit or reenter.
 * Skip only redundant value scans: descriptor/format/overflow/alias checks,
 * ownership checks, conversion, cache publication and failure cleanup remain.
 * No persistent trust mode; public APIs always validate arbitrary PCM, including
 * hits. These helpers do not validate values, acquire master ownership or permit
 * mutable EFx banks. Upload is synchronous despite bounded staging/write chunks. */
enum pt_cache_result pt_playback_pcm_upload_prepared(struct pt_sample_cache *,const struct pt_pcm *,
    uint32_t,uint64_t,const struct pt_playback_format *,uint8_t *,size_t,void *,
    int (*)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *);
enum pt_cache_result pt_amigus_wavetable_cache_acquire_prepared(struct pt_amigus_wavetable_cache *,
    const struct pt_pcm *,uint32_t,uint64_t,const struct pt_playback_format *,uint8_t *,size_t,struct pt_cache_lease *);
#endif
