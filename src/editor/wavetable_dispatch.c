#include "wavetable_internal.h"
#include "../core/amigus_render_voice.h"
#include "../core/document.h"
#include <limits.h>
#include <string.h>
static int source_current(struct pt_wavetable_voices *v,const struct pt_wavetable_prepared *p)
{return p?v->song_owner==p->context && p->current && p->acquire && p->location && p->current(p->context):!v->song_owner && pt_sampler_wavetable_sync(v->bridge);}
static enum pt_cache_result source_acquire(struct pt_wavetable_voices *v,const struct pt_wavetable_prepared *p,
    unsigned slot,const struct pt_playback_format *f,uint8_t *staging,size_t capacity,struct pt_cache_lease *lease)
{return p?p->acquire(p->context,slot,f,staging,capacity,lease):pt_sampler_wavetable_acquire(v->bridge,slot,f,staging,capacity,lease);}
static int source_location(struct pt_wavetable_voices *v,const struct pt_wavetable_prepared *p,
    struct pt_cache_lease lease,uint32_t *address,uint32_t *bytes)
{return p?p->location(p->context,lease,address,bytes):pt_sampler_wavetable_location(v->bridge,lease,address,bytes);}
static int resolve(const struct pt_project *p,const struct pt_pcm *pcm,unsigned *slot)
{
    unsigned i;
    for(i=0;i<p->sample_count;++i)if(pcm==&p->samples[i].pcm){*slot=i;return 1;}
    return 0;
}
static int control(struct pt_wavetable_voices *v,const struct pt_render_action *a,unsigned rate,const struct pt_wavetable_prepared *sources)
{
    struct pt_wavetable_voice *voice=v->voice+a->channel;
    uint32_t frequency,address,bytes;uint16_t left,right;
    if(!voice->held || voice->uncertain || !source_location(v,sources,voice->lease,&address,&bytes) ||
       !(sources?sources->control_plan && sources->control_plan(sources->context,a,rate,&frequency,&left,&right):
            pt_amigus_render_control(a->voice.step,rate,a->gain,&frequency,&left,&right)))return 0;
    if(v->api.control(v->api.context,a->channel,frequency,left,right)==1)return 1;
    voice->uncertain=1;return 0;
}
static int trigger(struct pt_wavetable_voices *v,const struct pt_render_action *a,unsigned rate,
    const struct pt_playback_format *f,uint8_t *staging,size_t capacity,const struct pt_wavetable_prepared *sources)
{
    unsigned slot;struct pt_cache_lease lease;enum pt_cache_result result;
    struct pt_amigus_voice_plan p;struct pt_wavetable_voice *voice=v->voice+a->channel;
    uint32_t address,bytes;
    if(!resolve(v->bridge->project,a->voice.pcm,&slot))return 0;
    result=source_acquire(v,sources,slot,f,staging,capacity,&lease);
    if(result!=PT_CACHE_LOAD && result!=PT_CACHE_HIT)return 0;
    if(!source_location(v,sources,lease,&address,&bytes) ||
       !(sources?sources->trigger_plan && sources->trigger_plan(sources->context,a,rate,f,address,bytes,&p):
            pt_amigus_render_voice(&a->voice,rate,a->gain,f,address,bytes,&p)) ||
       pt_wavetable_stop_owned(v,a->channel,sources?sources->context:NULL)!=1) {
        pt_sampler_wavetable_unpin(v->bridge,lease);return 0;
    }
    if(!source_location(v,sources,lease,&address,&bytes)) {
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
    uint16_t *held_state,unsigned *action,uint8_t *samples,unsigned converted)
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
            if(size>UINT32_MAX || (!converted && !pt_amigus_render_voice(&a->voice,rate,a->gain,format,0,(uint32_t)size,&prepared)))return PT_WAVETABLE_GEOMETRY;
            if(samples)samples[slot]=1;
            held|=(uint16_t)(1U<<a->channel);break;
        }
        case PT_RENDER_CONTROL:
            if(!controls || !(held&(1U<<a->channel)) ||
               (!converted && !pt_amigus_render_control(a->voice.step,rate,a->gain,&frequency,&left,&right)))return PT_WAVETABLE_CONTROL;
            break;
        case PT_RENDER_STOP:held&=(uint16_t)~(1U<<a->channel);break;
        default:return PT_WAVETABLE_OPERATION;
        }
    }
    *held_state=held;*action=UINT_MAX;return PT_WAVETABLE_COMPATIBLE;
}
enum pt_wavetable_capability pt_wavetable_check_plan(const struct pt_project *p,unsigned rate,
    const struct pt_render_plan *plan,const struct pt_playback_format *format,unsigned controls,
    uint16_t *held,struct pt_wavetable_preflight_report *out)
{
    struct pt_wavetable_preflight_report r;
    memset(&r,0,sizeof(r));r.result=PT_WAVETABLE_INVALID;r.action=r.channel=UINT_MAX;
    if(!out)return PT_WAVETABLE_INVALID;
    if(!p || pt_channels_validate(&p->channels)!=PT_CHANNEL_OK || !p->samples ||
       !p->sample_count || p->sample_count>PT_PROJECT_SAMPLES || !held || controls>1 ||
       (*held & (uint16_t)~((1UL<<p->channels.count)-1)))goto done;
    r.result=check_plan(p,rate,plan,format,controls,held,&r.action,r.samples,0);
    if(plan && r.action<plan->count){r.channel=plan->action[r.action].channel;r.kind=plan->action[r.action].kind;}
done:
    if(r.result!=PT_WAVETABLE_COMPATIBLE)memset(r.samples,0,sizeof(r.samples));
    *out=r;return r.result;
}
struct pt_wavetable_preflight {
    struct pt_allocator allocator;const struct pt_project *project;
    struct pt_render_options options;struct pt_playback_format format;
    struct pt_render_sequence *sequence;struct pt_render_plan plan;
    struct pt_render_snapshot snapshot;struct pt_render_interval interval;
    struct pt_wavetable_preflight_report report;
    uint32_t remaining;uint16_t held;unsigned controls,restores,range,restored,phase;
};
void pt_wavetable_preflight_close(struct pt_wavetable_preflight **work)
{
    struct pt_wavetable_preflight *w;struct pt_allocator a;
    if(!work || !*work)return;
    w=*work;a=w->allocator;pt_render_sequence_close(w->sequence);
    a.release(a.context,w);*work=NULL;
}
enum pt_wavetable_capability pt_wavetable_preflight_begin(const struct pt_project *p,
    const struct pt_render_options *o,const struct pt_playback_format *format,unsigned controls,unsigned session,unsigned restores,
    const struct pt_allocator *a,struct pt_wavetable_preflight_report *out,struct pt_wavetable_preflight **work)
{
    struct pt_wavetable_preflight_report r;struct pt_wavetable_preflight *w=NULL;
    memset(&r,0,sizeof(r));r.result=PT_WAVETABLE_INVALID;r.action=UINT_MAX;
    if(!out)return PT_WAVETABLE_INVALID;
    if(!work || !p || !o || !a || !a->allocate || !a->release || session>1)goto done;
    if(!valid_format(format)){r.result=PT_WAVETABLE_FORMAT;goto done;}
    if(session && o->row_range && restores!=1){r.result=PT_WAVETABLE_RESTORE;goto done;}
    w=a->allocate(a->context,sizeof(*w));
    if(!w){r.result=PT_WAVETABLE_MEMORY;goto done;}
    memset(w,0,sizeof(*w));w->allocator=*a;
    r.render_result=pt_render_sequence_begin(p,o,a,&w->sequence);
    if(r.render_result!=PT_RENDER_OK) {
        r.result=r.render_result==PT_RENDER_MEMORY?PT_WAVETABLE_MEMORY:PT_WAVETABLE_RENDER;
        pt_wavetable_preflight_close(&w);goto done;
    }
    w->project=p;w->options=*o;w->format=*format;w->controls=controls;w->restores=restores;
    w->range=session && o->row_range;r.result=PT_WAVETABLE_PENDING;w->report=r;*work=w;
done:
    *out=r;return r.result;
}
enum pt_wavetable_capability pt_wavetable_preflight_step(struct pt_wavetable_preflight *w,
    struct pt_wavetable_preflight_report *out)
{
    struct pt_wavetable_preflight_report *r;unsigned ready;
    if(!w || !out)return PT_WAVETABLE_INVALID;
    r=&w->report;
    if(r->result!=PT_WAVETABLE_PENDING)goto done;
    switch(w->phase) {
    case 0: /* Audited timeline measurement: at most256 ticks, no voice advance. */
        r->render_result=pt_render_sequence_prepare(w->sequence,256,&ready);
        if(r->render_result==PT_RENDER_OK && ready)w->phase=1;
        break;
    case 1:
        r->render_result=pt_render_sequence_next(w->sequence,&w->interval);
        if(r->render_result!=PT_RENDER_OK)break;
        ++r->intervals;w->remaining=w->interval.frames;
        if(w->range && w->interval.emit && !w->restored) {
            struct pt_wavetable_preflight_report restore;unsigned ch,i;
            r->render_result=pt_render_sequence_snapshot(w->sequence,&w->snapshot);
            if(r->render_result!=PT_RENDER_OK)break;
            if(pt_wavetable_restore_preflight(w->project,&w->snapshot,w->options.rate,&w->format,w->restores,&restore)!=PT_WAVETABLE_COMPATIBLE) {
                r->result=restore.result;r->channel=restore.channel;r->action=UINT_MAX;break;
            }
            w->held=0;
            for(ch=0;ch<w->snapshot.channels;++ch)if(w->snapshot.voice[ch].active)w->held|=(uint16_t)(1U<<ch);
            for(i=0;i<PT_PROJECT_SAMPLES;++i)r->samples[i]|=restore.samples[i];
            w->restored=1;
        }
        w->phase=w->remaining?2:3;break;
    case 2: {
        uint32_t block=w->remaining>256?256:w->remaining;
        r->render_result=pt_render_sequence_consume(w->sequence,block);
        if(r->render_result!=PT_RENDER_OK)break;
        w->remaining-=block;r->frames+=block;if(!w->remaining)w->phase=3;
        break;
    }
    case 3:
        r->render_result=pt_render_sequence_complete(w->sequence,&w->plan);
        if(r->render_result!=PT_RENDER_OK)break;
        r->result=check_plan(w->project,w->options.rate,&w->plan,&w->format,w->controls,&w->held,&r->action,
            w->range && !w->restored?NULL:r->samples,0);
        if(r->result!=PT_WAVETABLE_COMPATIBLE) {
            if(r->action<w->plan.count){r->channel=w->plan.action[r->action].channel;r->kind=w->plan.action[r->action].kind;}
        }else if(!w->interval.end){r->result=PT_WAVETABLE_PENDING;w->phase=1;}
        break;
    }
    if(r->render_result!=PT_RENDER_OK)r->result=r->render_result==PT_RENDER_MEMORY?PT_WAVETABLE_MEMORY:PT_WAVETABLE_RENDER;
done:
    *out=*r;return r->result;
}
int pt_wavetable_preflight_take(struct pt_wavetable_preflight *w,struct pt_render_sequence **out)
{
    if(!w || !out || w->report.result!=PT_WAVETABLE_COMPATIBLE || !w->sequence ||
       pt_render_sequence_rewind(w->sequence)!=PT_RENDER_OK)return 0;
    *out=w->sequence;w->sequence=NULL;return 1;
}
static enum pt_wavetable_capability preflight(const struct pt_project *p,
    const struct pt_render_options *o,const struct pt_playback_format *format,unsigned controls,unsigned session,unsigned restores,
    const struct pt_allocator *a,struct pt_wavetable_preflight_report *out)
{
    struct pt_wavetable_preflight *w=NULL;
    enum pt_wavetable_capability result=pt_wavetable_preflight_begin(p,o,format,controls,session,restores,a,out,&w);
    while(result==PT_WAVETABLE_PENDING)result=pt_wavetable_preflight_step(w,out);
    pt_wavetable_preflight_close(&w);return result;
}
enum pt_wavetable_capability pt_wavetable_preflight(const struct pt_project *p,
    const struct pt_render_options *o,const struct pt_playback_format *format,unsigned controls,
    const struct pt_allocator *a,struct pt_wavetable_preflight_report *out)
{return preflight(p,o,format,controls,0,0,a,out);}
enum pt_wavetable_capability pt_wavetable_session_preflight(const struct pt_project *p,
    const struct pt_render_options *o,const struct pt_playback_format *format,unsigned controls,unsigned restores,
    const struct pt_allocator *a,struct pt_wavetable_preflight_report *out)
{return preflight(p,o,format,controls,1,restores,a,out);}
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
static int dispatch(struct pt_wavetable_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *plan,const struct pt_playback_format *format,uint8_t *staging,size_t capacity,const struct pt_wavetable_prepared *sources)
{
    unsigned i,action;uint16_t held=0;
    if(!v || !v->bridge || v->closing || !plan || plan->count>PT_RENDER_ACTIONS || !format ||
       (rate!=44100 && rate!=48000) || !source_current(v,sources) || v->bridge->version!=version)return 0;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(v->voice[i].held && !v->voice[i].uncertain)held|=(uint16_t)(1U<<i);
    /* Private preparation validated immutable command values. Still recheck the
       entire live channel/source/held-state transition before ANY callback. */
    if(sources && (!sources->batch_ready || !sources->batch_ready(sources->context,plan,rate,format)))return 0;
    if(check_plan(v->bridge->project,rate,plan,format,v->api.control!=NULL,&held,&action,NULL,sources!=NULL)!=PT_WAVETABLE_COMPATIBLE)return 0;
    for(i=0;i<plan->count;++i) {
        const struct pt_render_action *a=plan->action+i;int ok;
        if(a->kind==PT_RENDER_TRIGGER)ok=trigger(v,a,rate,format,staging,capacity,sources);
        else if(a->kind==PT_RENDER_CONTROL)ok=control(v,a,rate,sources);
        else ok=pt_wavetable_stop_owned(v,a->channel,sources?sources->context:NULL)==1;
        if(!ok) {
            unsigned ch;v->closing=1;
            for(ch=0;ch<PT_WAVETABLE_VOICES;++ch)pt_wavetable_stop_owned(v,ch,sources?sources->context:NULL);
            return -1;
        }
    }
    return 1;
}

static int restore_dispatch(struct pt_wavetable_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_snapshot *snapshot,const struct pt_playback_format *format,uint8_t *staging,size_t capacity,const struct pt_wavetable_prepared *sources)
{
    struct pt_wavetable_preflight_report report;struct pt_cache_lease lease[16];
    struct pt_amigus_restore_plan plan[16];unsigned held[16]={0},ch,slot;uint32_t address,bytes;
    enum pt_cache_result loaded;
    if(!v || !v->bridge || v->closing || !v->api.restore || !source_current(v,sources) || v->bridge->version!=version)return 0;
    for(ch=0;ch<16;++ch)if(v->voice[ch].held)return 0;
    if(sources) {
        if(!sources->restore_ready || !sources->restore_ready(sources->context,snapshot,rate,format))return 0;
    }else if(pt_wavetable_restore_preflight(v->bridge->project,snapshot,rate,format,1,&report)!=PT_WAVETABLE_COMPATIBLE)return 0;
    for(ch=0;ch<snapshot->channels;++ch)if(snapshot->voice[ch].active) {
        if(!resolve(v->bridge->project,snapshot->voice[ch].pcm,&slot))goto refused;
        loaded=source_acquire(v,sources,slot,format,staging,capacity,&lease[ch]);
        if(loaded!=PT_CACHE_LOAD && loaded!=PT_CACHE_HIT)goto refused;
        held[ch]=1;
        if(!source_location(v,sources,lease[ch],&address,&bytes) ||
           !(sources?sources->restore_plan && sources->restore_plan(sources->context,snapshot,ch,rate,format,address,bytes,plan+ch):
               pt_amigus_render_restore(snapshot->voice+ch,rate,snapshot->gain[ch],format,address,bytes,plan+ch)))goto refused;
    }
    /* All resources exist before any voice may read them. Recheck ownership
       after acquisitions; caller callbacks cannot change sampler state. */
    if(!source_current(v,sources) || v->bridge->version!=version)goto refused;
    for(ch=0;ch<16;++ch)if(held[ch] && !source_location(v,sources,lease[ch],&address,&bytes))goto refused;
    for(ch=0;ch<16;++ch)if(held[ch]) {
        if(!source_location(v,sources,lease[ch],&address,&bytes))goto failed;
        v->voice[ch].lease=lease[ch];v->voice[ch].held=1;v->voice[ch].uncertain=1;held[ch]=0;
        if(v->api.restore(v->api.context,ch,plan+ch)!=1)goto failed;
        v->voice[ch].uncertain=0;
    }
    return 1;
failed:
    v->closing=1;
    for(ch=0;ch<16;++ch)if(held[ch])pt_sampler_wavetable_unpin(v->bridge,lease[ch]);
    for(ch=0;ch<16;++ch)pt_wavetable_stop_owned(v,ch,sources?sources->context:NULL);
    return -1;
refused:
    for(ch=0;ch<16;++ch)if(held[ch])pt_sampler_wavetable_unpin(v->bridge,lease[ch]);
    return 0;
}

int pt_wavetable_dispatch(struct pt_wavetable_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *plan,const struct pt_playback_format *f,uint8_t *staging,size_t capacity)
{return dispatch(v,version,rate,plan,f,staging,capacity,NULL);}
int pt_wavetable_dispatch_prepared(struct pt_wavetable_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_plan *plan,const struct pt_playback_format *f,uint8_t *staging,size_t capacity,const struct pt_wavetable_prepared *sources)
{return sources?dispatch(v,version,rate,plan,f,staging,capacity,sources):0;}
int pt_wavetable_restore_dispatch(struct pt_wavetable_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_snapshot *snapshot,const struct pt_playback_format *f,uint8_t *staging,size_t capacity)
{return restore_dispatch(v,version,rate,snapshot,f,staging,capacity,NULL);}
int pt_wavetable_restore_prepared(struct pt_wavetable_voices *v,uint64_t version,unsigned rate,
    const struct pt_render_snapshot *snapshot,const struct pt_playback_format *f,uint8_t *staging,size_t capacity,const struct pt_wavetable_prepared *sources)
{return sources?restore_dispatch(v,version,rate,snapshot,f,staging,capacity,sources):0;}
