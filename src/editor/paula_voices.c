#include <string.h>
#include "paula_internal.h"
static int stop_slot(struct pt_paula_voices *v,unsigned slot)
{
    struct pt_paula_voice *voice=&v->voice[slot];int result;
    if(!voice->held)return 1;
    result=v->api.stop(v->api.context,slot);
    if(result!=1)return result==0?0:-1;
    if(!pt_sampler_paula_unpin(v->bridge,voice->lease))return -1;
    memset(voice,0,sizeof(*voice));voice->track=-1;return 1;
}
int pt_paula_voices_bind(struct pt_paula_voices *v,struct pt_sampler_paula *s,const struct pt_paula_voice_api *api)
{
    int8_t map[PT_CHANNEL_LIMIT];unsigned i;struct pt_paula_voice_api copy;
    if(!v || v->bridge || !api || !api->start || !api->stop || !pt_sampler_paula_sync(s) ||
       pt_channels_paula_map(&s->project->channels,NULL,map)!=PT_CHANNEL_OK)return 0;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(s->cache.entry[i].pins)return 0;
    copy=*api;memset(v,0,sizeof(*v));v->bridge=s;v->api=copy;memcpy(v->map,map,sizeof(map));
    for(i=0;i<PT_PAULA_VOICES;++i)v->voice[i].track=-1;
    return 1;
}
int pt_paula_voices_bind_quiesce(struct pt_paula_voices *v,int (*quiesce)(void *),void *context)
{
    unsigned i;
    if(!v || !v->bridge || v->closing || v->song_owner || v->started || v->quiesce || !quiesce)return 0;
    for(i=0;i<PT_PAULA_VOICES;++i)if(v->voice[i].held)return 0;
    v->quiesce=quiesce;v->quiesce_context=context;return 1;
}
int pt_paula_voices_sync(struct pt_paula_voices *v)
{
    int8_t next[PT_CHANNEL_LIMIT];unsigned i;int result,complete=1;
    if(!v || !v->bridge || v->closing || v->song_owner || !pt_sampler_paula_sync(v->bridge) ||
       pt_channels_paula_map(&v->bridge->project->channels,v->map,next)!=PT_CHANNEL_OK)return -2;
    for(i=0;i<PT_PAULA_VOICES;++i)if(v->voice[i].held &&
       next[(unsigned)v->voice[i].track]!=(int8_t)i) {
        result=stop_slot(v,i);
        if(result<0)complete=-1;
        else if(!result && complete==1)complete=0;
    }
    if(complete==1)memcpy(v->map,next,sizeof(next));
    return complete;
}
static int geometry(const struct pt_pcm *pcm,unsigned channel,const struct pt_paula_voice_request *r)
{
    uint64_t bytes;
    if(!r || !pcm || channel>=pcm->channels || !r->period || r->volume>64 ||
       (r->offset&1) || (r->length&1) || r->length<2 || r->length>131070 || r->offset>=pcm->frames)return 0;
    bytes=(uint64_t)pcm->frames+(pcm->frames&1);
    return (uint64_t)r->offset+r->length<=bytes;
}
enum pt_paula_voice_result pt_paula_voices_trigger(struct pt_paula_voices *v,unsigned track,
    unsigned sample,unsigned channel,const struct pt_paula_voice_request *request)
{
    struct pt_paula_voice_request r;struct pt_paula_voice_plan plan;struct pt_cache_lease lease;
    struct pt_paula_voice *voice;const uint8_t *data;size_t bytes;enum pt_cache_result loaded;
    int result,slot;
    if(!v || !v->bridge || v->closing || track>=PT_CHANNEL_LIMIT || !request)return PT_PAULA_VOICE_REFUSED;
    r=*request;result=pt_paula_voices_sync(v);
    if(result!=1)return result==0?PT_PAULA_VOICE_STOP_PENDING:result==-1?PT_PAULA_VOICE_STOP_FAILED:PT_PAULA_VOICE_REFUSED;
    slot=v->map[track];
    if(slot<0 || sample>=v->bridge->count || !geometry(&v->bridge->project->samples[sample].pcm,channel,&r))return PT_PAULA_VOICE_REFUSED;
    loaded=pt_sampler_paula_acquire(v->bridge,track,sample,channel,&lease);
    if(loaded!=PT_CACHE_LOAD && loaded!=PT_CACHE_HIT)return PT_PAULA_VOICE_REFUSED;
    if(!pt_sampler_paula_location(v->bridge,track,lease,&data,&bytes) ||
       !geometry(&v->bridge->project->samples[sample].pcm,channel,&r) ||
       (uintptr_t)data&1 || (uint64_t)r.offset+r.length>bytes) {
        pt_sampler_paula_unpin(v->bridge,lease);return PT_PAULA_VOICE_REFUSED;
    }
    result=stop_slot(v,(unsigned)slot);
    if(result!=1) {
        pt_sampler_paula_unpin(v->bridge,lease);
        return result==0?PT_PAULA_VOICE_STOP_PENDING:PT_PAULA_VOICE_STOP_FAILED;
    }
    if(!pt_sampler_paula_location(v->bridge,track,lease,&data,&bytes)) {
        pt_sampler_paula_unpin(v->bridge,lease);return PT_PAULA_VOICE_REFUSED;
    }
    plan.data=data+r.offset;plan.words=(uint16_t)(r.length/2);plan.period=r.period;plan.volume=r.volume;
    voice=&v->voice[slot];voice->lease=lease;voice->held=1;voice->uncertain=1;voice->track=(int8_t)track;
    v->started=1;
    if(v->api.start(v->api.context,(unsigned)slot,&plan)==1) {voice->uncertain=0;return PT_PAULA_VOICE_ACTIVE;}
    return PT_PAULA_VOICE_UNCERTAIN;
}
enum pt_paula_voice_result pt_paula_voices_control(struct pt_paula_voices *v,unsigned track,uint16_t period,uint8_t volume)
{
    int slot,result;struct pt_paula_voice *voice;
    if(!v || !v->bridge || v->closing || track>=PT_CHANNEL_LIMIT || !period || volume>64 || !v->api.control)return PT_PAULA_VOICE_REFUSED;
    result=pt_paula_voices_sync(v);
    if(result!=1)return result==0?PT_PAULA_VOICE_STOP_PENDING:result==-1?PT_PAULA_VOICE_STOP_FAILED:PT_PAULA_VOICE_REFUSED;
    slot=v->map[track];if(slot<0)return PT_PAULA_VOICE_REFUSED;
    voice=&v->voice[slot];if(!voice->held || voice->uncertain)return PT_PAULA_VOICE_REFUSED;
    if(v->api.control(v->api.context,(unsigned)slot,period,volume)==1)return PT_PAULA_VOICE_ACTIVE;
    voice->uncertain=1;return PT_PAULA_VOICE_UNCERTAIN;
}
int pt_paula_stop_owned(struct pt_paula_voices *v,unsigned track,void *owner)
{
    int slot;
    if(!v || !v->bridge || v->song_owner!=owner || track>=PT_CHANNEL_LIMIT)return -1;
    slot=v->map[track];if(slot<0)return 1;
    return stop_slot(v,(unsigned)slot);
}
int pt_paula_drain_owned(struct pt_paula_voices *v,void *owner)
{
    unsigned i;int complete=1;
    if(!v || v->song_owner!=owner)return 0;
    if(!v->bridge)return 1;
    v->closing=1;
    for(i=0;i<PT_PAULA_VOICES;++i)if(stop_slot(v,i)!=1)complete=0;
    if(!complete || (v->quiesce && v->quiesce(v->quiesce_context)!=1))return 0;
    return 1;
}
int pt_paula_close_owned(struct pt_paula_voices *v,void *owner)
{
    if(!pt_paula_drain_owned(v,owner))return 0;
    if(!v->bridge)return 1;
    if(!pt_sampler_paula_close(v->bridge))return 0;
    memset(v,0,sizeof(*v));return 1;
}

int pt_paula_voices_stop(struct pt_paula_voices *v,unsigned track)
{return pt_paula_stop_owned(v,track,NULL);}
int pt_paula_voices_close(struct pt_paula_voices *v)
{return pt_paula_close_owned(v,NULL);}
