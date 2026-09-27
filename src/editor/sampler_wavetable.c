#include <string.h>
#include "sampler_wavetable_internal.h"
#include "sampler_internal.h"
#include "../core/playback_internal.h"
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
void pt_sampler_upload_cancel(struct pt_sampler_upload_job *j)
{
    if(!j)return;
    pt_amigus_upload_cancel(&j->upload);pt_sampler_unpin(j->pin);memset(j,0,sizeof(*j));
}
static enum pt_cache_result begin(struct pt_sampler_upload_job *j,struct pt_sampler_wavetable *s,unsigned slot,
    unsigned generation,uint64_t version,struct pt_sample_version *expected,
    const struct pt_playback_format *format,struct pt_cache_lease *out,unsigned prepared)
{
    struct pt_cache_lease lease;enum pt_edit_result edit;enum pt_cache_result result;
    if(!j || j->bridge || j->pin || j->upload.backend || !s || !s->sampler || !s->project || !out)return PT_CACHE_INVALID;
    if(prepared) {
        if(s->generation!=generation || s->sampler->generation!=generation || s->version!=version || !version ||
           s->table!=s->project->samples || s->count!=s->project->sample_count || !pt_amigus_wavetable_cache_current(s->backend))return PT_CACHE_INVALID;
    }else if(!pt_sampler_wavetable_sync(s))return PT_CACHE_INVALID;
    if(slot>=s->count)return PT_CACHE_INVALID;
    edit=prepared?pt_sampler_pin_current(s->sampler,s->project,slot,generation,expected,&j->pcm,&j->pin):
        pt_sampler_pin(s->sampler,s->project,slot,s->generation,&j->pcm,&j->pin);
    if(edit!=PT_EDIT_OK)return edit==PT_EDIT_CAPACITY?PT_CACHE_CAPACITY:PT_CACHE_INVALID;
    j->bridge=s;j->sampler=s->sampler;j->project=s->project;j->backend=s->backend;
    j->generation=s->generation;j->version=s->version;j->slot=slot;memcpy(&j->snapshot,s->project,sizeof(j->snapshot));
    result=prepared?pt_amigus_upload_begin_prepared(&j->upload,s->backend,&j->pcm,slot+1,s->version,format,&lease):
        pt_amigus_upload_begin(&j->upload,s->backend,&j->pcm,slot+1,s->version,format,&lease);
    if(result!=PT_CACHE_PENDING) {
        pt_sampler_upload_cancel(j);if(result==PT_CACHE_HIT)*out=lease;
    }
    return result;
}
enum pt_cache_result pt_sampler_upload_begin(struct pt_sampler_upload_job *j,struct pt_sampler_wavetable *s,
    unsigned slot,const struct pt_playback_format *format,struct pt_cache_lease *out)
{return begin(j,s,slot,0,0,NULL,format,out,0);}
enum pt_cache_result pt_sampler_upload_begin_prepared(struct pt_sampler_upload_job *j,struct pt_sampler_wavetable *s,
    unsigned slot,unsigned generation,uint64_t version,struct pt_sample_version *expected,
    const struct pt_playback_format *format,struct pt_cache_lease *out)
{return begin(j,s,slot,generation,version,expected,format,out,1);}
static int current(struct pt_sampler_upload_job *j)
{
    struct pt_sampler_wavetable *s=j->bridge;struct pt_pcm pcm;struct pt_sample_version *pin;
    j->snapshot.channels.selected=j->project->channels.selected;
    if(s->sampler!=j->sampler || s->project!=j->project || s->backend!=j->backend ||
       s->generation!=j->generation || s->version!=j->version || j->sampler->generation!=j->generation ||
       memcmp(j->project,&j->snapshot,sizeof(j->snapshot)) || s->table!=j->project->samples || s->count!=j->project->sample_count ||
       pt_sampler_pin_current(j->sampler,j->project,j->slot,j->generation,j->pin,&pcm,&pin)!=PT_EDIT_OK)return 0;
    pt_sampler_unpin(pin);return 1;
}
enum pt_cache_result pt_sampler_upload_step(struct pt_sampler_upload_job *j,uint8_t *staging,size_t capacity,struct pt_cache_lease *out)
{
    enum pt_cache_result result;
    if(!j || !j->bridge)return PT_CACHE_INVALID;
    if(!out || !current(j)){pt_sampler_upload_cancel(j);return PT_CACHE_INVALID;}
    result=pt_amigus_upload_step(&j->upload,staging,capacity,out);
    if(result!=PT_CACHE_PENDING)pt_sampler_upload_cancel(j);
    return result;
}
enum pt_cache_result pt_sampler_wavetable_acquire(struct pt_sampler_wavetable *s,unsigned slot,
    const struct pt_playback_format *format,uint8_t *staging,size_t capacity,struct pt_cache_lease *out)
{
    struct pt_sampler_upload_job job={0};enum pt_cache_result result=pt_sampler_upload_begin(&job,s,slot,format,out);
    while(result==PT_CACHE_PENDING)result=pt_sampler_upload_step(&job,staging,capacity,out);
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
