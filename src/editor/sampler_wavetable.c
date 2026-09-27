#include <string.h>
#include "sampler_wavetable.h"
int pt_sampler_wavetable_bind(struct pt_sampler_wavetable *s,struct pt_sampler *sampler,
    struct pt_project *project,struct pt_amigus_wavetable_cache *backend)
{
    if(!s || s->backend || !sampler || !backend || !backend->reservation ||
       backend->closing || backend->faulted || backend->cache.bytes ||
       pt_project_validate(project,NULL)!=PT_PROJECT_OK)return 0;
    memset(s,0,sizeof(*s));s->sampler=sampler;s->project=project;s->backend=backend;
    s->table=project->samples;s->count=project->sample_count;s->generation=sampler->generation;s->version=1;
    return 1;
}
int pt_sampler_wavetable_sync(struct pt_sampler_wavetable *s)
{
    struct pt_project *p;
    if(!s || !s->backend || !s->backend->reservation || s->backend->closing || s->backend->faulted || !s->version)return 0;
    p=s->project;if(pt_project_validate(p,NULL)!=PT_PROJECT_OK)return 0;
    if(s->generation!=s->sampler->generation || s->table!=p->samples || s->count!=p->sample_count) {
        pt_cache_clear(&s->backend->cache); /* Busy retired blocks remain pinned. */
        if(s->version==UINT64_MAX) {s->version=0;return 0;}
        ++s->version;s->generation=s->sampler->generation;s->table=p->samples;s->count=p->sample_count;
    }
    return 1;
}
enum pt_cache_result pt_sampler_wavetable_acquire(struct pt_sampler_wavetable *s,unsigned slot,
    const struct pt_playback_format *format,uint8_t *staging,size_t capacity,struct pt_cache_lease *out)
{
    struct pt_pcm pcm;struct pt_sample_version *pin;struct pt_cache_lease lease;
    enum pt_edit_result edit;enum pt_cache_result result;
    if(!out || !pt_sampler_wavetable_sync(s) || slot>=s->count)return PT_CACHE_INVALID;
    edit=pt_sampler_pin(s->sampler,s->project,slot,s->generation,&pcm,&pin);
    if(edit!=PT_EDIT_OK)return edit==PT_EDIT_CAPACITY?PT_CACHE_CAPACITY:PT_CACHE_INVALID;
    result=pt_amigus_wavetable_cache_acquire(s->backend,&pcm,slot+1,s->version,format,staging,capacity,&lease);
    pt_sampler_unpin(pin);
    if(result==PT_CACHE_LOAD || result==PT_CACHE_HIT)*out=lease;
    return result;
}
int pt_sampler_wavetable_location(struct pt_sampler_wavetable *s,struct pt_cache_lease lease,
    uint32_t *address,uint32_t *bytes)
{
    struct pt_sample_cache *cache;
    if(!pt_sampler_wavetable_sync(s))return 0;
    cache=&s->backend->cache;
    if(!pt_cache_data(cache,lease) || cache->entry[lease.slot].valid!=1 ||
       cache->entry[lease.slot].version!=s->version)return 0;
    return pt_amigus_wavetable_cache_location(s->backend,lease,address,bytes);
}
int pt_sampler_wavetable_unpin(struct pt_sampler_wavetable *s,struct pt_cache_lease lease)
{return s && s->backend && pt_amigus_wavetable_cache_unpin(s->backend,lease);}
int pt_sampler_wavetable_close(struct pt_sampler_wavetable *s)
{
    if(!s)return 0;
    if(!s->backend)return 1;
    if(!pt_amigus_wavetable_cache_detach(s->backend))return 0;
    memset(s,0,sizeof(*s));return 1;
}
