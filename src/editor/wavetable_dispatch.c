#include "wavetable_dispatch.h"
#include "../core/amigus_render_voice.h"
static int resolve(struct pt_sampler_wavetable *s,const struct pt_pcm *pcm,unsigned *slot)
{
    unsigned i;
    for(i=0;i<s->count;++i)if(pcm==&s->project->samples[i].pcm){*slot=i;return 1;}
    return 0;
}
static int control(struct pt_wavetable_voices *v,const struct pt_render_action *a,unsigned rate)
{
    struct pt_wavetable_voice *voice=v->voice+a->channel;
    uint32_t frequency,address,bytes;uint16_t left,right;
    if(!voice->held || voice->uncertain || !pt_sampler_wavetable_location(v->bridge,voice->lease,&address,&bytes) ||
       !pt_amigus_render_control(a->voice.step,rate,a->gain,&frequency,&left,&right))return 0;
    if(v->api.control(v->api.context,a->channel,frequency,left,right)==1)return 1;
    voice->uncertain=1;return 0;
}
static int trigger(struct pt_wavetable_voices *v,const struct pt_render_action *a,unsigned rate,
    const struct pt_playback_format *f,uint8_t *staging,size_t capacity)
{
    unsigned slot;struct pt_cache_lease lease;enum pt_cache_result result;
    struct pt_amigus_voice_plan p;struct pt_wavetable_voice *voice=v->voice+a->channel;
    uint32_t address,bytes;
    if(!resolve(v->bridge,a->voice.pcm,&slot))return 0;
    result=pt_sampler_wavetable_acquire(v->bridge,slot,f,staging,capacity,&lease);
    if(result!=PT_CACHE_LOAD && result!=PT_CACHE_HIT)return 0;
    if(!pt_sampler_wavetable_location(v->bridge,lease,&address,&bytes) ||
       !pt_amigus_render_voice(&a->voice,rate,a->gain,f,address,bytes,&p) ||
       pt_wavetable_voices_stop(v,a->channel)!=1) {
        pt_sampler_wavetable_unpin(v->bridge,lease);return 0;
    }
    if(!pt_sampler_wavetable_location(v->bridge,lease,&address,&bytes)) {
        pt_sampler_wavetable_unpin(v->bridge,lease);return 0;
    }
    voice->lease=lease;voice->held=1;voice->uncertain=1;
    if(v->api.start(v->api.context,a->channel,&p)!=1)return 0;
    voice->uncertain=0;return 1;
}
int pt_wavetable_dispatch(struct pt_wavetable_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *plan,const struct pt_playback_format *format,uint8_t *staging,size_t capacity)
{
    unsigned i,slot;uint16_t held=0;uint32_t frequency;uint16_t left,right;
    struct pt_amigus_voice_plan prepared;
    if(!v || !v->bridge || v->closing || !plan || plan->count>PT_RENDER_ACTIONS || !format ||
       (rate!=44100 && rate!=48000) || !pt_sampler_wavetable_sync(v->bridge) || v->bridge->version!=version)return 0;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(v->voice[i].held && !v->voice[i].uncertain)held|=(uint16_t)(1U<<i);
    for(i=0;i<plan->count;++i) {
        const struct pt_render_action *a=plan->action+i;
        if(a->channel>=v->bridge->project->channels.count)return 0;
        switch(a->kind) {
        case PT_RENDER_TRIGGER: {
            uint64_t size;
            if(!resolve(v->bridge,a->voice.pcm,&slot))return 0;
            size=(uint64_t)v->bridge->project->samples[slot].pcm.frames*(format->bits/8);
            if(size>UINT32_MAX || !pt_amigus_render_voice(&a->voice,rate,a->gain,format,0,(uint32_t)size,&prepared))return 0;
            held|=(uint16_t)(1U<<a->channel);break;
        }
        case PT_RENDER_CONTROL:
            if(!v->api.control || !(held&(1U<<a->channel)) ||
               !pt_amigus_render_control(a->voice.step,rate,a->gain,&frequency,&left,&right))return 0;
            break;
        case PT_RENDER_STOP:held&=(uint16_t)~(1U<<a->channel);break;
        default:return 0;
        }
    }
    for(i=0;i<plan->count;++i) {
        const struct pt_render_action *a=plan->action+i;int ok;
        if(a->kind==PT_RENDER_TRIGGER)ok=trigger(v,a,rate,format,staging,capacity);
        else if(a->kind==PT_RENDER_CONTROL)ok=control(v,a,rate);
        else ok=pt_wavetable_voices_stop(v,a->channel)==1;
        if(!ok) {
            unsigned ch;v->closing=1;
            for(ch=0;ch<PT_WAVETABLE_VOICES;++ch)pt_wavetable_voices_stop(v,ch);
            return -1;
        }
    }
    return 1;
}
