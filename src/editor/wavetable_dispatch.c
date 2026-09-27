#include "wavetable_dispatch.h"
#include "../core/amigus_render_voice.h"
#include "../core/document.h"
#include <limits.h>
#include <string.h>
static int resolve(const struct pt_project *p,const struct pt_pcm *pcm,unsigned *slot)
{
    unsigned i;
    for(i=0;i<p->sample_count;++i)if(pcm==&p->samples[i].pcm){*slot=i;return 1;}
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
    if(!resolve(v->bridge->project,a->voice.pcm,&slot))return 0;
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
static int valid_format(const struct pt_playback_format *f)
{return f && (f->bits==8 || f->bits==16) && !f->channel && !f->word_pad && f->little_endian<=1;}
/* Shared capability rules for silent whole-song analysis and live batches.
 * The tentative held mask changes only after the entire batch is accepted. */
static enum pt_wavetable_capability check_plan(const struct pt_project *p,unsigned rate,
    const struct pt_render_plan *plan,const struct pt_playback_format *format,unsigned controls,
    uint16_t *held_state,unsigned *action,uint8_t *samples)
{
    unsigned i,slot;uint16_t held=*held_state;uint32_t frequency;uint16_t left,right;
    struct pt_amigus_voice_plan prepared;
    if(!valid_format(format))return PT_WAVETABLE_FORMAT;
    if(!plan || plan->count>PT_RENDER_ACTIONS || (rate!=44100 && rate!=48000))return PT_WAVETABLE_INVALID;
    for(i=0;i<plan->count;++i) {
        const struct pt_render_action *a=plan->action+i;*action=i;
        if(a->channel>=p->channels.count || a->channel>=PT_WAVETABLE_VOICES)return PT_WAVETABLE_CHANNEL;
        switch(a->kind) {
        case PT_RENDER_TRIGGER: {
            uint64_t size;
            if(!resolve(p,a->voice.pcm,&slot))return PT_WAVETABLE_SOURCE;
            size=(uint64_t)p->samples[slot].pcm.frames*(format->bits/8);
            if(size>UINT32_MAX || !pt_amigus_render_voice(&a->voice,rate,a->gain,format,0,(uint32_t)size,&prepared))return PT_WAVETABLE_GEOMETRY;
            if(samples)samples[slot]=1;
            held|=(uint16_t)(1U<<a->channel);break;
        }
        case PT_RENDER_CONTROL:
            if(!controls || !(held&(1U<<a->channel)) ||
               !pt_amigus_render_control(a->voice.step,rate,a->gain,&frequency,&left,&right))return PT_WAVETABLE_CONTROL;
            break;
        case PT_RENDER_STOP:held&=(uint16_t)~(1U<<a->channel);break;
        default:return PT_WAVETABLE_OPERATION;
        }
    }
    *held_state=held;*action=UINT_MAX;return PT_WAVETABLE_COMPATIBLE;
}
enum pt_wavetable_capability pt_wavetable_preflight(const struct pt_project *p,
    const struct pt_render_options *o,const struct pt_playback_format *format,unsigned controls,
    const struct pt_allocator *a,struct pt_wavetable_preflight_report *out)
{
    struct pt_wavetable_preflight_report r;
    struct pt_render_sequence *sequence=NULL;struct pt_render_plan *plan;uint16_t held=0;
    memset(&r,0,sizeof(r));r.result=PT_WAVETABLE_INVALID;r.action=UINT_MAX;
    if(!out)return PT_WAVETABLE_INVALID;
    if(!p || !o || !a || !a->allocate || !a->release)goto done;
    if(!valid_format(format)){r.result=PT_WAVETABLE_FORMAT;goto done;}
    plan=a->allocate(a->context,sizeof(*plan));
    if(!plan){r.result=PT_WAVETABLE_MEMORY;goto done;}
    r.render_result=pt_render_sequence_open(p,o,a,&sequence);
    if(r.render_result!=PT_RENDER_OK)goto release;
    do {
        struct pt_render_interval interval;uint32_t remaining;
        r.render_result=pt_render_sequence_next(sequence,&interval);
        if(r.render_result!=PT_RENDER_OK)break;
        ++r.intervals;remaining=interval.frames;
        while(remaining) {
            uint32_t block=remaining>256?256:remaining;
            r.render_result=pt_render_sequence_consume(sequence,block);
            if(r.render_result!=PT_RENDER_OK)break;
            remaining-=block;r.frames+=block;
        }
        if(r.render_result!=PT_RENDER_OK)break;
        r.render_result=pt_render_sequence_complete(sequence,plan);
        if(r.render_result!=PT_RENDER_OK)break;
        r.result=check_plan(p,o->rate,plan,format,controls,&held,&r.action,r.samples);
        if(r.result!=PT_WAVETABLE_COMPATIBLE) {
            if(r.action<plan->count){r.channel=plan->action[r.action].channel;r.kind=plan->action[r.action].kind;}
            break;
        }
        if(interval.end)break;
    }while(1);
release:
    pt_render_sequence_close(sequence);a->release(a->context,plan);
    if(r.render_result!=PT_RENDER_OK)r.result=r.render_result==PT_RENDER_MEMORY?PT_WAVETABLE_MEMORY:PT_WAVETABLE_RENDER;
done:
    *out=r;return r.result;
}
enum pt_wavetable_capability pt_wavetable_restore_preflight(const struct pt_project *p,
    const struct pt_render_snapshot *snapshot,unsigned rate,const struct pt_playback_format *format,unsigned exact_restore,
    struct pt_wavetable_preflight_report *out)
{
    struct pt_wavetable_preflight_report r;unsigned ch,slot;
    memset(&r,0,sizeof(r));r.result=PT_WAVETABLE_INVALID;r.action=UINT_MAX;r.channel=UINT_MAX;
    if(!out)return PT_WAVETABLE_INVALID;
    if(!p || !snapshot || !p->channels.count || p->channels.count>16 || snapshot->channels!=p->channels.count ||
       p->sample_count>PT_PROJECT_SAMPLES || (p->sample_count && !p->samples) || (rate!=44100 && rate!=48000))goto done;
    if(!valid_format(format)){r.result=PT_WAVETABLE_FORMAT;goto done;}
    if(exact_restore!=1){r.result=PT_WAVETABLE_RESTORE;goto done;}
    for(ch=0;ch<snapshot->channels;++ch) {
        const struct pt_voice *v=snapshot->voice+ch;struct pt_amigus_restore_plan plan;uint64_t size;
        if(!v->active)continue;
        r.channel=ch;
        if(!resolve(p,v->pcm,&slot)){r.result=PT_WAVETABLE_SOURCE;goto done;}
        size=(uint64_t)p->samples[slot].pcm.frames*(format->bits/8);
        if(size>UINT32_MAX || !pt_amigus_render_restore(v,rate,snapshot->gain[ch],format,0,(uint32_t)size,&plan)) {
            r.result=PT_WAVETABLE_GEOMETRY;goto done;
        }
        r.samples[slot]=1;
    }
    r.channel=UINT_MAX;r.result=PT_WAVETABLE_COMPATIBLE;
done:
    *out=r;return r.result;
}
int pt_wavetable_dispatch(struct pt_wavetable_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *plan,const struct pt_playback_format *format,uint8_t *staging,size_t capacity)
{
    unsigned i,action;uint16_t held=0;
    if(!v || !v->bridge || v->closing || !plan || plan->count>PT_RENDER_ACTIONS || !format ||
       (rate!=44100 && rate!=48000) || !pt_sampler_wavetable_sync(v->bridge) || v->bridge->version!=version)return 0;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(v->voice[i].held && !v->voice[i].uncertain)held|=(uint16_t)(1U<<i);
    if(check_plan(v->bridge->project,rate,plan,format,v->api.control!=NULL,&held,&action,NULL)!=PT_WAVETABLE_COMPATIBLE)return 0;
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

int pt_wavetable_restore_dispatch(struct pt_wavetable_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_snapshot *snapshot,const struct pt_playback_format *format,uint8_t *staging,size_t capacity)
{
    struct pt_wavetable_preflight_report report;struct pt_cache_lease lease[16];
    struct pt_amigus_restore_plan plan[16];unsigned held[16]={0},ch,slot;uint32_t address,bytes;
    enum pt_cache_result loaded;
    if(!v || !v->bridge || v->closing || !v->api.restore || !pt_sampler_wavetable_sync(v->bridge) || v->bridge->version!=version)return 0;
    for(ch=0;ch<16;++ch)if(v->voice[ch].held)return 0;
    if(pt_wavetable_restore_preflight(v->bridge->project,snapshot,rate,format,1,&report)!=PT_WAVETABLE_COMPATIBLE)return 0;
    for(ch=0;ch<snapshot->channels;++ch)if(snapshot->voice[ch].active) {
        if(!resolve(v->bridge->project,snapshot->voice[ch].pcm,&slot))goto refused;
        loaded=pt_sampler_wavetable_acquire(v->bridge,slot,format,staging,capacity,&lease[ch]);
        if(loaded!=PT_CACHE_LOAD && loaded!=PT_CACHE_HIT)goto refused;
        held[ch]=1;
        if(!pt_sampler_wavetable_location(v->bridge,lease[ch],&address,&bytes) ||
           !pt_amigus_render_restore(snapshot->voice+ch,rate,snapshot->gain[ch],format,address,bytes,plan+ch))goto refused;
    }
    /* All resources exist before any voice may read them. Recheck ownership
       after acquisitions; caller callbacks cannot change sampler state. */
    if(!pt_sampler_wavetable_sync(v->bridge) || v->bridge->version!=version)goto refused;
    for(ch=0;ch<16;++ch)if(held[ch] && !pt_sampler_wavetable_location(v->bridge,lease[ch],&address,&bytes))goto refused;
    for(ch=0;ch<16;++ch)if(held[ch]) {
        if(!pt_sampler_wavetable_location(v->bridge,lease[ch],&address,&bytes))goto failed;
        v->voice[ch].lease=lease[ch];v->voice[ch].held=1;v->voice[ch].uncertain=1;held[ch]=0;
        if(v->api.restore(v->api.context,ch,plan+ch)!=1)goto failed;
        v->voice[ch].uncertain=0;
    }
    return 1;
failed:
    v->closing=1;
    for(ch=0;ch<16;++ch)if(held[ch])pt_sampler_wavetable_unpin(v->bridge,lease[ch]);
    for(ch=0;ch<16;++ch)pt_wavetable_voices_stop(v,ch);
    return -1;
refused:
    for(ch=0;ch<16;++ch)if(held[ch])pt_sampler_wavetable_unpin(v->bridge,lease[ch]);
    return 0;
}
