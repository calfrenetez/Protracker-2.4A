#ifndef PT_NATIVE_PAULA_H
#define PT_NATIVE_PAULA_H
#include "project.h"
#include "playback.h"
#include "scope.h"
#include "sample_cache.h"
#include "master_memory.h"
struct IOAudio;
struct MsgPort;
struct pt_paula {
    struct MsgPort *port;
    struct IOAudio *audio,*lock;
    uint8_t *data,*staging,*check,*silence;
    uint8_t *sample_data[31];
    size_t sample_bytes[31],chip_bytes;
    struct pt_sample_cache cache;
    struct pt_cache_lease lease[31];
    uint64_t cache_version;
    struct pt_master_memory memory;
    size_t source_bytes;
    uint32_t cached_instruments;
    size_t bytes,pattern_bytes;
    uint16_t order_count,orders[PT_PROJECT_ORDERS];
    unsigned opened,locked,started,mode,pattern,audible;
    struct pt_scope_phase scope[4];
};
/* Zero-initialize once. 0=song, 1=selected pattern. No channel stealing. */
const char *pt_paula_play(struct pt_paula *,const struct pt_project *,unsigned mode,unsigned position,unsigned pattern);
const char *pt_paula_audition(struct pt_paula *,const struct pt_project *,unsigned sample,unsigned period);
void pt_paula_stop(struct pt_paula *);
const char *pt_paula_sync(struct pt_paula *,const struct pt_project *);
void pt_paula_poll(struct pt_paula *,struct pt_playback *);
#endif
