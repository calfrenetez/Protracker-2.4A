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
