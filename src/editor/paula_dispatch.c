#include <string.h>
#include "paula_internal.h"
#include "project_snapshot.h"
#include "sampler_internal.h"
static void release_batch(struct pt_paula_voices *v,struct pt_paula_batch *b)
{
    unsigned i;for(i=0;i<PT_RENDER_ACTIONS;++i)if(b->entry[i].held) {
        pt_sampler_paula_unpin(v->bridge,b->entry[i].lease);b->entry[i].held=0;
    }
}
static int current(struct pt_paula_voices *v,uint64_t version,uint16_t *held,void *owner,unsigned prepared)
{
    int8_t map[PT_CHANNEL_LIMIT];unsigned i;uint16_t state=0;
    if(!v || !v->bridge || v->song_owner!=owner || v->closing || !(prepared?pt_sampler_paula_prepared_current(v->bridge):pt_sampler_paula_sync(v->bridge)) ||
       v->bridge->version!=version ||
       pt_channels_paula_map(&v->bridge->project->channels,v->map,map)!=PT_CHANNEL_OK ||
       memcmp(map,v->map,sizeof(map)))return 0;
    for(i=0;i<PT_PAULA_VOICES;++i)if(v->voice[i].held) {
        int track=v->voice[i].track;
        if(v->voice[i].uncertain || track<0 || track>=PT_CHANNEL_LIMIT || map[track]!=(int8_t)i)return 0;
        state|=(uint16_t)(1U<<track);
    }
    *held=state;return 1;
}
static int prepare_batch(struct pt_paula_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *p,const struct pt_paula_render_caps *caps,struct pt_paula_batch *b,void *owner)
{
    struct pt_paula_preflight_report report;struct pt_paula_render_plan r;
    unsigned i,j;uint16_t held;uint16_t period;uint8_t volume;
    const uint8_t *data;size_t bytes;enum pt_cache_result loaded;
    if(!b || !current(v,version,&held,owner,0) ||
       pt_paula_check_plan(v->bridge->project,rate,v->map,p,caps,v->api.control!=NULL,&held,&report)!=PT_PAULA_COMPATIBLE)return 0;
    memset(b,0,sizeof(*b));
    for(i=0;i<p->count;++i) {
        const struct pt_render_action *a=&p->action[i];struct pt_paula_batch_entry *e=&b->entry[i];
        int slot=v->map[a->channel];if(slot<0)continue;
        if(a->kind==PT_RENDER_TRIGGER) {
            for(j=0;j<v->bridge->count;++j)if(a->voice.pcm==&v->bridge->project->samples[j].pcm)break;
            if(j==v->bridge->count || !pt_paula_render_voice(&a->voice,rate,a->gain,(unsigned)slot,caps,&r))goto refused;
            e->sample=j;
            loaded=pt_sampler_paula_acquire(v->bridge,a->channel,j,0,&e->lease);
            if(loaded!=PT_CACHE_LOAD && loaded!=PT_CACHE_HIT)goto refused;
            e->held=1;memcpy(&e->source,a->voice.pcm,sizeof(e->source));
            if(!pt_sampler_paula_location(v->bridge,a->channel,e->lease,&data,&bytes) ||
               ((uintptr_t)data&1) || (uint64_t)r.offset+r.length>bytes)goto refused;
            e->offset=r.offset;e->length=r.length;
            e->plan.data=data+r.offset;e->plan.words=(uint16_t)(r.length/2);
            e->plan.period=r.period;e->plan.volume=r.volume;
        }else if(a->kind==PT_RENDER_CONTROL) {
            if(!pt_paula_render_control(a->voice.step,rate,a->gain,(unsigned)slot,caps,&period,&volume))goto refused;
            e->plan.period=period;e->plan.volume=volume;
        }
    }
    return 1;
refused:release_batch(v,b);return 0;
}
static int validate_batch(struct pt_paula_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *p,const struct pt_paula_render_caps *caps,struct pt_paula_batch *b,void *owner,unsigned prepared)
{
    struct pt_paula_preflight_report report;struct pt_paula_render_plan r;
    unsigned i;uint16_t held;const uint8_t *data;size_t bytes;
    /* Promotion replaces storage inside the same descriptor; revalidate exact
     * source identities, map/revision and every pinned address before output. */
    if(!current(v,version,&held,owner,prepared) ||
       pt_paula_check_plan(v->bridge->project,rate,v->map,p,caps,v->api.control!=NULL,&held,&report)!=PT_PAULA_COMPATIBLE)return 0;
    for(i=0;i<p->count;++i)if(b->entry[i].held) {
        const struct pt_render_action *a=&p->action[i];struct pt_paula_batch_entry *e=&b->entry[i];
        if(a->voice.pcm!=&v->bridge->project->samples[e->sample].pcm ||
           memcmp(a->voice.pcm,&e->source,sizeof(e->source)) ||
           !pt_paula_render_voice(&a->voice,rate,a->gain,(unsigned)v->map[a->channel],caps,&r) ||
           !(prepared?pt_sampler_paula_prepared_location(v->bridge,a->channel,e->lease,&data,&bytes):
              pt_sampler_paula_location(v->bridge,a->channel,e->lease,&data,&bytes)) ||
           ((uintptr_t)data&1) || (uint64_t)r.offset+r.length>bytes ||
           e->plan.data!=data+r.offset || e->plan.words!=r.length/2 ||
           e->plan.period!=r.period || e->plan.volume!=r.volume)return 0;
    }
    return 1;
}
static int apply_batch(struct pt_paula_voices *v,const struct pt_render_plan *p,
    struct pt_paula_batch *b,void *owner)
{
    unsigned i;
    for(i=0;i<p->count;++i) {
        const struct pt_render_action *a=&p->action[i];struct pt_paula_batch_entry *e=&b->entry[i];
        int slot=v->map[a->channel];struct pt_paula_voice *voice;
        if(slot<0)continue;
        switch(a->kind) {
        case PT_RENDER_TRIGGER:
            if(pt_paula_stop_owned(v,a->channel,owner)!=1)goto failed;
            voice=&v->voice[slot];voice->lease=e->lease;voice->held=voice->uncertain=1;
            voice->track=(int8_t)a->channel;e->held=0;v->started=1;
            if(v->api.start(v->api.context,(unsigned)slot,&e->plan)!=1)goto failed;
            voice->uncertain=0;break;
        case PT_RENDER_STOP:if(pt_paula_stop_owned(v,a->channel,owner)!=1)goto failed;break;
        case PT_RENDER_CONTROL:
            voice=&v->voice[slot];voice->uncertain=1;
            if(v->api.control(v->api.context,(unsigned)slot,e->plan.period,e->plan.volume)!=1)goto failed;
            voice->uncertain=0;break;
        default:goto failed;
        }
    }
    release_batch(v,b);return 1;
failed:
    v->closing=1;release_batch(v,b);
    for(i=0;i<PT_PAULA_VOICES;++i)if(v->voice[i].held)
        pt_paula_stop_owned(v,(unsigned)v->voice[i].track,owner);
    return -1;
}

int pt_paula_dispatch_owned(struct pt_paula_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *p,const struct pt_paula_render_caps *caps,struct pt_paula_batch *b,void *owner)
{
    if(!prepare_batch(v,version,rate,p,caps,b,owner))return 0;
    if(!validate_batch(v,version,rate,p,caps,b,owner,0)){release_batch(v,b);return 0;}
    return apply_batch(v,p,b,owner);
}

int pt_paula_dispatch(struct pt_paula_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *p,const struct pt_paula_render_caps *caps,struct pt_paula_batch *b)
{return pt_paula_dispatch_owned(v,version,rate,p,caps,b,NULL);}

static int voices_unchanged(struct pt_paula_prepared *p)
{
    const struct pt_paula_voices *v=p->voices;unsigned i;
    p->header.channels.selected=p->project->channels.selected;
    if(v->bridge!=p->bridge || p->bridge->project!=p->project ||
       !pt_project_snapshot_equal(p->project,&p->header) || memcmp(v->map,p->map,sizeof(p->map)) ||
       v->api.context!=p->api.context || v->api.start!=p->api.start ||
       v->api.stop!=p->api.stop || v->api.control!=p->api.control ||
       v->quiesce!=p->quiesce || v->quiesce_context!=p->quiesce_context)return 0;
    for(i=0;i<PT_PAULA_VOICES;++i) {
        const struct pt_paula_voice *a=&v->voice[i],*b=&p->voice[i];
        if(a->held!=b->held || a->uncertain!=b->uncertain || a->track!=b->track ||
           a->lease.slot!=b->lease.slot || a->lease.serial!=b->lease.serial)return 0;
    }
    return 1;
}
void pt_paula_cancel(struct pt_paula_prepared *p)
{
    struct pt_paula_voices original;unsigned i;
    if(!p || (!p->ready && !p->preparing))return;
    pt_sampler_paula_job_cancel(&p->job);
    /* Release only unstarted candidates against the captured bridge. */
    memset(&original,0,sizeof(original));original.bridge=p->bridge;
    release_batch(&original,&p->batch);
    if(p->claims && p->voices->song_owner==p)p->voices->song_owner=NULL;
    p->ready=p->preparing=0;p->voices=NULL;p->bridge=NULL;
    for(i=0;i<PT_PAULA_VOICES;++i)p->voice[i].held=0;
}
int pt_paula_prepare_owned(struct pt_paula_prepared *p,struct pt_paula_voices *v,
    uint64_t version,unsigned rate,const struct pt_render_plan *plan,
    const struct pt_paula_render_caps *caps,void *owner)
{
    if(!p || p->ready || p->preparing || !plan || !caps ||
       !prepare_batch(v,version,rate,plan,caps,&p->batch,owner))return 0;
    if(!validate_batch(v,version,rate,plan,caps,&p->batch,owner,0)) {
        release_batch(v,&p->batch);return 0;
    }
    p->voices=v;p->bridge=v->bridge;p->project=v->bridge->project;
    memcpy(&p->header,p->project,sizeof(p->header));p->version=version;p->rate=rate;
    p->plan=*plan;p->caps=*caps;p->api=v->api;
    p->quiesce=v->quiesce;p->quiesce_context=v->quiesce_context;
    memcpy(p->voice,v->voice,sizeof(p->voice));memcpy(p->map,v->map,sizeof(p->map));
    p->owner=owner;p->claims=owner==NULL;p->incremental=0;p->ready=1;
    if(p->claims){v->song_owner=p;p->owner=p;}
    return 1;
}
int pt_paula_prepare(struct pt_paula_prepared *p,struct pt_paula_voices *v,
    uint64_t version,unsigned rate,const struct pt_render_plan *plan,const struct pt_paula_render_caps *caps)
{return pt_paula_prepare_owned(p,v,version,rate,plan,caps,NULL);}
static int prepared_sources(struct pt_paula_prepared *p);
int pt_paula_apply(struct pt_paula_prepared *p)
{
    int result;
    if(!p || !p->ready)return 0;
    if(!voices_unchanged(p) || (p->incremental && !prepared_sources(p)) || !validate_batch(p->voices,p->version,p->rate,
       &p->plan,&p->caps,&p->batch,p->owner,p->incremental)){pt_paula_cancel(p);return 0;}
    result=apply_batch(p->voices,&p->plan,&p->batch,p->owner);
    pt_paula_cancel(p);return result;
}

static int prepared_sources(struct pt_paula_prepared *p)
{
    unsigned i;struct pt_pcm pcm;struct pt_sample_version *pin;
    for(i=0;i<p->plan.count;++i)if(p->master[i]) {
        if(pt_sampler_pin_current(p->bridge->sampler,p->project,p->batch.entry[i].sample,
           p->bridge->generation,p->master[i],&pcm,&pin)!=PT_EDIT_OK)return 0;
        pt_sampler_unpin(pin);
    }
    return 1;
}
int pt_paula_prepare_begin_owned(struct pt_paula_prepared *p,struct pt_paula_voices *v,
    uint64_t version,unsigned rate,const struct pt_render_plan *plan,
    const struct pt_paula_render_caps *caps,struct pt_sample_version *const *pins,void *owner)
{
    struct pt_paula_preflight_report report;unsigned i,j;uint16_t held;
    if(!p || p->ready || p->preparing || !owner || !pins || !plan || !caps ||
       !current(v,version,&held,owner,1) ||
       pt_paula_check_plan(v->bridge->project,rate,v->map,plan,caps,v->api.control!=NULL,&held,&report)!=PT_PAULA_COMPATIBLE)return 0;
    memset(p,0,sizeof(*p));p->voices=v;p->bridge=v->bridge;p->project=v->bridge->project;
    memcpy(&p->header,p->project,sizeof(p->header));p->version=version;p->rate=rate;
    p->plan=*plan;p->caps=*caps;p->api=v->api;p->owner=owner;p->incremental=p->preparing=1;
    p->quiesce=v->quiesce;p->quiesce_context=v->quiesce_context;
    memcpy(p->voice,v->voice,sizeof(p->voice));memcpy(p->map,v->map,sizeof(p->map));
    for(i=0;i<plan->count;++i)if(plan->action[i].kind==PT_RENDER_TRIGGER && v->map[plan->action[i].channel]>=0) {
        for(j=0;j<p->project->sample_count;++j)if(plan->action[i].voice.pcm==&p->project->samples[j].pcm)break;
        if(j==p->project->sample_count || !pins[j]){pt_paula_cancel(p);return 0;}
        p->master[i]=pins[j];p->batch.entry[i].sample=j;
        memcpy(&p->batch.entry[i].source,plan->action[i].voice.pcm,sizeof(struct pt_pcm));
    }
    if(!prepared_sources(p)){pt_paula_cancel(p);return 0;}
    return 1;
}
enum pt_cache_result pt_paula_prepare_step_owned(struct pt_paula_prepared *p)
{
    uint16_t held;enum pt_cache_result result;struct pt_paula_batch_entry *e;
    const struct pt_render_action *a;struct pt_paula_render_plan r;
    const uint8_t *data;size_t bytes;int slot;
    if(!p || !p->preparing)return PT_CACHE_INVALID;
    if(!voices_unchanged(p) || !current(p->voices,p->version,&held,p->owner,1) || !prepared_sources(p))goto refused;
    if(p->index==p->plan.count) {
        if(!validate_batch(p->voices,p->version,p->rate,&p->plan,&p->caps,&p->batch,p->owner,1))goto refused;
        p->preparing=0;p->ready=1;return PT_CACHE_LOAD;
    }
    a=&p->plan.action[p->index];e=&p->batch.entry[p->index];slot=p->map[a->channel];
    if(slot<0){++p->index;return PT_CACHE_PENDING;}
    if(a->kind!=PT_RENDER_TRIGGER) {
        if(a->kind==PT_RENDER_CONTROL && !pt_paula_render_control(a->voice.step,p->rate,a->gain,
           (unsigned)slot,&p->caps,&e->plan.period,&e->plan.volume))goto refused;
        ++p->index;return PT_CACHE_PENDING;
    }
    if(p->job.owner)result=pt_sampler_paula_job_step(&p->job,&e->lease);
    else result=pt_sampler_paula_job_begin(&p->job,p->bridge,a->channel,e->sample,0,p->master[p->index],&e->lease);
    if(result==PT_CACHE_PENDING)return result;
    if(result!=PT_CACHE_HIT && result!=PT_CACHE_LOAD)goto refused;
    e->held=1;
    if(!pt_paula_render_voice(&a->voice,p->rate,a->gain,(unsigned)slot,&p->caps,&r) ||
       !pt_sampler_paula_prepared_location(p->bridge,a->channel,e->lease,&data,&bytes) ||
       ((uintptr_t)data&1) || (uint64_t)r.offset+r.length>bytes)goto refused;
    e->offset=r.offset;e->length=r.length;
    e->plan.data=data+r.offset;e->plan.words=(uint16_t)(r.length/2);e->plan.period=r.period;e->plan.volume=r.volume;
    ++p->index;return PT_CACHE_PENDING;
refused:pt_paula_cancel(p);return PT_CACHE_INVALID;
}

int pt_paula_prepared_ready_owned(struct pt_paula_prepared *p)
{
    unsigned i;uint16_t held;const uint8_t *data;size_t bytes;
    if(!p || !p->ready || !p->incremental || !p->owner ||
       !voices_unchanged(p) || !prepared_sources(p) ||
       !current(p->voices,p->version,&held,p->owner,1))return 0;
    /* current() checked bridge metadata once in this serialized call. These
     * lookups have no callbacks; retain every live/candidate lease check. */
    for(i=0;i<PT_PAULA_VOICES;++i)if(p->voice[i].held &&
       !pt_sampler_paula_prepared_location_validated(p->bridge,(unsigned)p->voice[i].track,
           p->voice[i].lease,&data,&bytes))return 0;
    for(i=0;i<p->plan.count;++i)if(p->plan.action[i].kind==PT_RENDER_TRIGGER) {
        struct pt_paula_batch_entry *e=p->batch.entry+i;
        const struct pt_render_action *a=p->plan.action+i;
        if(!e->held || e->sample>=p->project->sample_count ||
           a->voice.pcm!=&p->project->samples[e->sample].pcm ||
           memcmp(a->voice.pcm,&e->source,sizeof(e->source)) ||
           !pt_sampler_paula_prepared_location_validated(p->bridge,a->channel,e->lease,&data,&bytes) ||
           ((uintptr_t)data&1) || (uint64_t)e->offset+e->length>bytes ||
           e->plan.data!=data+e->offset || e->plan.words!=e->length/2)return 0;
    }
    return 1;
}
int pt_paula_prepared_action_owned(struct pt_paula_prepared *p,unsigned index)
{
    struct pt_paula_voices *v;struct pt_paula_voice *voice;
    struct pt_paula_batch_entry *e;const struct pt_render_action *a;int slot;
    if(!p || !p->ready || !p->incremental || index>=p->plan.count)return 0;
    v=p->voices;if(v->song_owner!=p->owner || v->closing)return 0;
    a=p->plan.action+index;e=p->batch.entry+index;slot=p->map[a->channel];
    if(slot<0)return 0;
    voice=v->voice+slot;
    switch(a->kind) {
    case PT_RENDER_TRIGGER:
        if(!e->held)return 0;
        if(pt_paula_stop_owned(v,a->channel,p->owner)!=1)return -2;
        voice->lease=e->lease;voice->held=voice->uncertain=1;
        voice->track=(int8_t)a->channel;e->held=0;v->started=1;
        if(v->api.start(v->api.context,(unsigned)slot,&e->plan)!=1)return -1;
        voice->uncertain=0;return 1;
    case PT_RENDER_STOP:return pt_paula_stop_owned(v,a->channel,p->owner)==1?1:-2;
    case PT_RENDER_CONTROL:
        if(!voice->held || voice->uncertain)return -1;
        voice->uncertain=1;
        if(v->api.control(v->api.context,(unsigned)slot,e->plan.period,e->plan.volume)!=1)return -1;
        voice->uncertain=0;return 1;
    default:return -1;
    }
}
