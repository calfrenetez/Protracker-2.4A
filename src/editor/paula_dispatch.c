#include <string.h>
#include "paula_internal.h"
static void release_batch(struct pt_paula_voices *v,struct pt_paula_batch *b)
{
    unsigned i;for(i=0;i<PT_RENDER_ACTIONS;++i)if(b->entry[i].held) {
        pt_sampler_paula_unpin(v->bridge,b->entry[i].lease);b->entry[i].held=0;
    }
}
static int current(struct pt_paula_voices *v,uint64_t version,uint16_t *held,void *owner)
{
    int8_t map[PT_CHANNEL_LIMIT];unsigned i;uint16_t state=0;
    if(!v || !v->bridge || v->song_owner!=owner || v->closing || !pt_sampler_paula_sync(v->bridge) ||
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
int pt_paula_dispatch_owned(struct pt_paula_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *p,const struct pt_paula_render_caps *caps,struct pt_paula_batch *b,void *owner)
{
    struct pt_paula_preflight_report report;struct pt_paula_render_plan r;
    unsigned i,j;uint16_t held;uint16_t period;uint8_t volume;
    const uint8_t *data;size_t bytes;enum pt_cache_result loaded;
    if(!b || !current(v,version,&held,owner) ||
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
            e->held=1;
            if(!pt_sampler_paula_location(v->bridge,a->channel,e->lease,&data,&bytes) ||
               ((uintptr_t)data&1) || (uint64_t)r.offset+r.length>bytes)goto refused;
            e->plan.data=data+r.offset;e->plan.words=(uint16_t)(r.length/2);
            e->plan.period=r.period;e->plan.volume=r.volume;
        }else if(a->kind==PT_RENDER_CONTROL) {
            if(!pt_paula_render_control(a->voice.step,rate,a->gain,(unsigned)slot,caps,&period,&volume))goto refused;
            e->plan.period=period;e->plan.volume=volume;
        }
    }
    /* Promotion replaces storage inside the same descriptor; revalidate exact
     * source identities, map/revision and every pinned address before output. */
    if(!current(v,version,&held,owner) ||
       pt_paula_check_plan(v->bridge->project,rate,v->map,p,caps,v->api.control!=NULL,&held,&report)!=PT_PAULA_COMPATIBLE)goto refused;
    for(i=0;i<p->count;++i)if(b->entry[i].held) {
        const struct pt_render_action *a=&p->action[i];struct pt_paula_batch_entry *e=&b->entry[i];
        if(a->voice.pcm!=&v->bridge->project->samples[e->sample].pcm ||
           !pt_paula_render_voice(&a->voice,rate,a->gain,(unsigned)v->map[a->channel],caps,&r) ||
           !pt_sampler_paula_location(v->bridge,a->channel,e->lease,&data,&bytes) ||
           ((uintptr_t)data&1) || (uint64_t)r.offset+r.length>bytes ||
           e->plan.data!=data+r.offset || e->plan.words!=r.length/2 ||
           e->plan.period!=r.period || e->plan.volume!=r.volume)goto refused;
    }
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
refused:release_batch(v,b);return 0;
failed:
    v->closing=1;release_batch(v,b);
    for(i=0;i<PT_PAULA_VOICES;++i)if(v->voice[i].held)
        pt_paula_stop_owned(v,(unsigned)v->voice[i].track,owner);
    return -1;
}

int pt_paula_dispatch(struct pt_paula_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *p,const struct pt_paula_render_caps *caps,struct pt_paula_batch *b)
{return pt_paula_dispatch_owned(v,version,rate,p,caps,b,NULL);}
