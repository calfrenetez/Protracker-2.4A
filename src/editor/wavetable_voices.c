#include <string.h>
#include "wavetable_voices.h"
int pt_wavetable_voices_bind(struct pt_wavetable_voices *v,struct pt_sampler_wavetable *s,
    const struct pt_wavetable_voice_api *api)
{
    unsigned i;
    if(!v || v->bridge || !api || !api->start || !api->stop ||
       !pt_sampler_wavetable_sync(s))return 0;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(s->backend->cache.entry[i].pins)return 0;
    memset(v,0,sizeof(*v));v->bridge=s;v->api=*api;return 1;
}
int pt_wavetable_voices_bind_quiesce(struct pt_wavetable_voices *v,int (*quiesce)(void *),void *context)
{
    unsigned i;
    if(!v || !v->bridge || v->closing || v->song_owner || v->quiesce || !quiesce)return 0;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(v->voice[i].held)return 0;
    v->quiesce=quiesce;v->quiesce_context=context;return 1;
}
int pt_wavetable_voices_stop(struct pt_wavetable_voices *v,unsigned id)
{
    struct pt_wavetable_voice *voice;int result;
    if(!v || !v->bridge || id>=PT_WAVETABLE_VOICES)return -1;
    voice=&v->voice[id];if(!voice->held)return 1;
    result=v->api.stop(v->api.context,id);
    if(result!=1)return result==0?0:-1;
    if(!pt_sampler_wavetable_unpin(v->bridge,voice->lease))return -1;
    memset(voice,0,sizeof(*voice));return 1;
}
enum pt_voice_result pt_wavetable_voices_trigger(struct pt_wavetable_voices *v,unsigned id,
    unsigned sample,const struct pt_playback_format *format,const struct pt_amigus_voice_request *request,
    uint8_t *staging,size_t capacity)
{
    struct pt_cache_lease candidate;struct pt_wavetable_voice *voice;
    enum pt_cache_result loaded;uint32_t address,bytes;int stopped;
    struct pt_amigus_voice_plan plan;const struct pt_sample *source;uint64_t logical;
    if(!v || !v->bridge || v->closing || id>=PT_WAVETABLE_VOICES || !format)return PT_VOICE_REFUSED;
    if(!pt_sampler_wavetable_sync(v->bridge) || sample>=v->bridge->count)return PT_VOICE_REFUSED;
    source=v->bridge->project->samples+sample;
    logical=(uint64_t)source->pcm.frames*(format->bits/8);
    if(logical>UINT32_MAX || !pt_amigus_voice_plan_prepare(source,format,request,0,(uint32_t)logical,&plan))return PT_VOICE_REFUSED;
    loaded=pt_sampler_wavetable_acquire(v->bridge,sample,format,staging,capacity,&candidate);
    if(loaded!=PT_CACHE_LOAD && loaded!=PT_CACHE_HIT)return PT_VOICE_REFUSED;
    /* Promotion can replace sample table contents; fetch current metadata. */
    source=v->bridge->project->samples+sample;
    if(!pt_sampler_wavetable_location(v->bridge,candidate,&address,&bytes) ||
       !pt_amigus_voice_plan_prepare(source,format,request,address,bytes,&plan)) {
        pt_sampler_wavetable_unpin(v->bridge,candidate);return PT_VOICE_REFUSED;
    }
    stopped=pt_wavetable_voices_stop(v,id);
    if(stopped!=1) {
        pt_sampler_wavetable_unpin(v->bridge,candidate);
        return stopped==0?PT_VOICE_STOP_PENDING:PT_VOICE_STOP_FAILED;
    }
    if(!pt_sampler_wavetable_location(v->bridge,candidate,&address,&bytes)) {
        pt_sampler_wavetable_unpin(v->bridge,candidate);return PT_VOICE_REFUSED;
    }
    voice=&v->voice[id];voice->lease=candidate;voice->held=1;voice->uncertain=1;
    if(v->api.start(v->api.context,id,&plan)==1) {
        voice->uncertain=0;return PT_VOICE_ACTIVE;
    }
    return PT_VOICE_UNCERTAIN;
}
int pt_wavetable_voices_close(struct pt_wavetable_voices *v)
{
    unsigned i;int complete=1;
    if(!v)return 0;
    if(!v->bridge)return 1;
    v->closing=1;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(pt_wavetable_voices_stop(v,i)!=1)complete=0;
    if(!complete)return 0;
    if(v->quiesce && !v->quiesced) {
        if(v->quiesce(v->quiesce_context)!=1 ||
           !v->bridge->backend || !v->bridge->backend->reservation ||
           v->bridge->backend->reservation->interrupt)return 0;
        v->quiesced=1;
    }
    if(!pt_sampler_wavetable_close(v->bridge))return 0;
    memset(v,0,sizeof(*v));return 1;
}
