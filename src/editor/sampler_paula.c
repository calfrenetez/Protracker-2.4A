#include <string.h>
#include "sampler_paula.h"
static void remember(struct pt_sampler_paula *s)
{
    unsigned i;s->table=s->project->samples;s->count=s->project->sample_count;
    s->generation=s->sampler->generation;s->channels=s->project->channels.count;
    for(i=0;i<PT_CHANNEL_LIMIT;++i)s->routes[i]=s->project->channels.track[i].route;
}
int pt_sampler_paula_bind(struct pt_sampler_paula *s,struct pt_sampler *sampler,struct pt_project *p,
    void *context,void *(*allocate)(void *,size_t),void (*release)(void *,void *,size_t),size_t budget)
{
    if(!s || s->sampler || !sampler || !sampler->allocator.allocate || !sampler->allocator.release ||
       !allocate || !release || pt_project_validate(p,NULL)!=PT_PROJECT_OK)return 0;
    memset(s,0,sizeof(*s));s->sampler=sampler;s->project=p;s->version=1;
    pt_cache_init(&s->cache,context,allocate,release,budget);remember(s);return 1;
}
int pt_sampler_paula_sync(struct pt_sampler_paula *s)
{
    unsigned i,changed;
    if(!s || !s->sampler || s->closing || !s->version || pt_project_validate(s->project,NULL)!=PT_PROJECT_OK)return 0;
    changed=s->generation!=s->sampler->generation || s->table!=s->project->samples ||
        s->count!=s->project->sample_count || s->channels!=s->project->channels.count;
    for(i=0;i<PT_CHANNEL_LIMIT;++i)if(s->routes[i]!=s->project->channels.track[i].route)changed=1;
    if(changed) {
        pt_cache_clear(&s->cache);
        if(s->version==UINT64_MAX) {s->closing=1;return 0;}
        ++s->version;remember(s);
    }
    return 1;
}
static int routed(struct pt_sampler_paula *s,unsigned track)
{return track<s->channels && s->routes[track]==PT_PAULA;}
enum pt_cache_result pt_sampler_paula_acquire(struct pt_sampler_paula *s,unsigned track,
    unsigned sample,unsigned source_channel,struct pt_cache_lease *out)
{
    struct pt_pcm pcm;struct pt_sample_version *pin;enum pt_edit_result edit;enum pt_cache_result result;
    struct pt_playback_format format={8,source_channel,0,1};
    if(!out || !pt_sampler_paula_sync(s) || !routed(s,track) || sample>=s->count ||
       source_channel>=s->project->samples[sample].pcm.channels)return PT_CACHE_INVALID;
    edit=pt_sampler_pin(s->sampler,s->project,sample,s->generation,&pcm,&pin);
    if(edit!=PT_EDIT_OK)return edit==PT_EDIT_CAPACITY?PT_CACHE_CAPACITY:PT_CACHE_INVALID;
    result=pt_playback_pcm_acquire(&s->cache,&pcm,sample+1,s->version,&format,out);
    pt_sampler_unpin(pin);return result;
}
int pt_sampler_paula_location(struct pt_sampler_paula *s,unsigned track,struct pt_cache_lease lease,
    const uint8_t **data,size_t *bytes)
{
    void *p;
    if(!data || !bytes || !pt_sampler_paula_sync(s) || !routed(s,track))return 0;
    p=pt_cache_data(&s->cache,lease);
    if(!p || s->cache.entry[lease.slot].valid!=1 || s->cache.entry[lease.slot].version!=s->version)return 0;
    *data=p;*bytes=s->cache.entry[lease.slot].bytes;return 1;
}
int pt_sampler_paula_unpin(struct pt_sampler_paula *s,struct pt_cache_lease lease)
{return s && s->sampler && pt_cache_unpin(&s->cache,lease);}
int pt_sampler_paula_close(struct pt_sampler_paula *s)
{
    if(!s)return 0;
    if(!s->sampler)return 1;
    s->closing=1;
    if(!pt_cache_clear(&s->cache))return 0;
    memset(s,0,sizeof(*s));return 1;
}

#include "sampler_paula_internal.h"
#include "sampler_internal.h"
#include "project_snapshot.h"
#include "../core/playback_internal.h"
static int prepared_bridge(struct pt_sampler_paula *s)
{
    unsigned i;
    if(!s || !s->sampler || !s->project || s->closing || !s->version ||
       s->generation!=s->sampler->generation || s->table!=s->project->samples ||
       s->count!=s->project->sample_count || !s->count || s->count>PT_PROJECT_SAMPLES ||
       s->channels!=s->project->channels.count || pt_channels_validate(&s->project->channels)!=PT_CHANNEL_OK)return 0;
    for(i=0;i<PT_CHANNEL_LIMIT;++i)if(s->routes[i]!=s->project->channels.track[i].route)return 0;
    return 1;
}
void pt_sampler_paula_job_cancel(struct pt_sampler_paula_job *j)
{
    if(!j)return;
    pt_playback_upload_cancel(&j->upload);pt_sampler_unpin(j->pin);memset(j,0,sizeof(*j));
}
static int copy_chip(void *context,void *resource,size_t offset,const uint8_t *data,size_t bytes)
{
    (void)context;memcpy((uint8_t *)resource+offset,data,bytes);return 1;
}
enum pt_cache_result pt_sampler_paula_job_begin(struct pt_sampler_paula_job *j,
    struct pt_sampler_paula *s,unsigned track,unsigned sample,unsigned source_channel,
    struct pt_sample_version *expected,struct pt_cache_lease *out)
{
    struct pt_playback_format format={8,source_channel,0,1};struct pt_cache_lease lease;
    struct pt_pcm pcm;struct pt_sample_version *pin;enum pt_cache_result result;
    if(!j || j->owner || !out || !prepared_bridge(s) || !routed(s,track) || sample>=s->count ||
       pt_sampler_pin_current(s->sampler,s->project,sample,s->generation,expected,&pcm,&pin)!=PT_EDIT_OK)return PT_CACHE_INVALID;
    memset(j,0,sizeof(*j));j->owner=s;j->sampler=s->sampler;j->project=s->project;
    memcpy(&j->header,s->project,sizeof(j->header));j->pin=pin;j->pcm=pcm;
    j->version=s->version;j->generation=s->generation;j->track=track;j->sample=sample;
    result=pt_playback_upload_begin_prepared(&j->upload,&s->cache,&j->pcm,sample+1,s->version,
        &format,NULL,copy_chip,&lease);
    if(result==PT_CACHE_PENDING) {
        if((uintptr_t)pt_cache_data(&s->cache,j->upload.lease)&1){pt_sampler_paula_job_cancel(j);return PT_CACHE_INVALID;}
        return result;
    }
    if(result==PT_CACHE_HIT) {
        if((uintptr_t)pt_cache_data(&s->cache,lease)&1){pt_cache_unpin(&s->cache,lease);result=PT_CACHE_INVALID;}
        else *out=lease;
    }
    pt_sampler_paula_job_cancel(j);return result;
}
enum pt_cache_result pt_sampler_paula_job_step(struct pt_sampler_paula_job *j,struct pt_cache_lease *out)
{
    struct pt_pcm pcm;struct pt_sample_version *pin;enum pt_cache_result result;
    if(!j || !j->owner)return PT_CACHE_INVALID;
    j->header.channels.selected=j->project->channels.selected;
    if(!out || !prepared_bridge(j->owner) || j->owner->sampler!=j->sampler ||
       j->owner->project!=j->project || j->owner->version!=j->version ||
       !pt_project_snapshot_equal(j->project,&j->header) ||
       pt_sampler_pin_current(j->sampler,j->project,j->sample,j->generation,j->pin,&pcm,&pin)!=PT_EDIT_OK) {
        pt_sampler_paula_job_cancel(j);return PT_CACHE_INVALID;
    }
    pt_sampler_unpin(pin);
    result=pt_playback_upload_step(&j->upload,j->staging,sizeof(j->staging),out);
    if(result!=PT_CACHE_PENDING)pt_sampler_paula_job_cancel(j);
    return result;
}
