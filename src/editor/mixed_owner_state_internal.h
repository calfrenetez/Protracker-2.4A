#ifndef PT_MIXED_OWNER_STATE_INTERNAL_H
#define PT_MIXED_OWNER_STATE_INTERNAL_H
/* Private shared layout for the genuine owner, never a public ready certificate. */
#include "mixed_owner_internal.h"
#include "paula_internal.h"
#include "wavetable_internal.h"
#include "sampler_internal.h"
#include "../core/render_lookahead.h"
#include "../core/elapsed_clock.h"
struct mixed_batch {
    struct pt_render_plan split,wave;struct pt_paula_prepared chip;
    struct pt_sampler_upload_job upload;
    struct {struct pt_cache_lease lease;struct pt_amigus_voice_plan command;unsigned held,sample;uint32_t address,bytes;} entry[PT_RENDER_ACTIONS];
    struct pt_paula_voice pv[PT_PAULA_VOICES];struct pt_wavetable_voice av[PT_WAVETABLE_VOICES];
    unsigned phase,index,count;struct {unsigned route,index;} order[PT_RENDER_ACTIONS];uint8_t staging[256];
};
struct pt_mixed_owner {
    struct pt_allocator allocator;struct pt_paula_voices *paula;struct pt_wavetable_voices *amigus;
    struct pt_sampler_paula *pb;struct pt_sampler_wavetable *ab;
    struct pt_sampler *sampler;struct pt_project *project,snapshot;
    struct pt_amigus_wavetable_cache *backend;struct pt_amigus_reservation *reservation;
    struct pt_paula_voice_api pa;struct pt_wavetable_voice_api aa;
    int (*pq)(void *),(*aq)(void *);void *pc,*ac;
    struct pt_render_options options;struct pt_paula_render_caps caps;struct pt_playback_format format;
    struct pt_render_sequence *sequence;struct pt_mixed_preflight *analysis;struct pt_mixed_report report;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_sampler_pin_job job;
    uint64_t pv,av;unsigned generation,slot,analyzed,ready,closing,drained[2];int8_t map[PT_CHANNEL_LIMIT];
    enum pt_mixed_owner_result failure;struct mixed_batch batch;
    struct pt_render_lookahead ahead;struct pt_render_plan plan;
    struct pt_render_interval interval;uint32_t remaining;
    unsigned pending,forecast,done,stop_attempted,clock_armed;
    uint64_t clock_start,clock_last,clock_deadline,schedule_start,schedule_last;
    unsigned visited,schedule_phase,schedule_seen;
    /* Optional established-master preparation. Legacy owners leave these zero. */
    void *checked_context;
    enum pt_mixed_owner_result (*checked_prepare)(struct pt_mixed_owner *,struct pt_mixed_report *);
    int (*checked_current)(struct pt_mixed_owner *);
    int (*checked_output)(struct pt_mixed_owner *,const void *,size_t,int);
    void (*checked_cancel)(struct pt_mixed_owner *);
    unsigned checked_busy,checked_faulted;
    struct pt_elapsed_clock elapsed;pt_mixed_clock_read clock_read;void *clock_context;unsigned clock_bound;
};
enum pt_mixed_owner_result pt_mixed_owner_preparation_fail(struct pt_mixed_owner *,enum pt_mixed_owner_result);
#endif
