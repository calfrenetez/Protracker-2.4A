#include "mixed_owner_internal.h"
#include "sampler_wavetable_internal.h"
#include "../core/amigus_render_voice.h"
#include "paula_internal.h"
#include "wavetable_internal.h"
#include "sampler_internal.h"
#include "project_snapshot.h"
#include <string.h>
struct mixed_batch {
    struct pt_render_plan split,wave;struct pt_paula_prepared chip;
    struct pt_sampler_upload_job upload;
    struct {struct pt_cache_lease lease;struct pt_amigus_voice_plan command;unsigned held,sample;uint32_t address,bytes;} entry[PT_RENDER_ACTIONS];
    struct pt_paula_voice pv[PT_PAULA_VOICES];struct pt_wavetable_voice av[PT_WAVETABLE_VOICES];
    unsigned phase,index,count;struct {unsigned route,index;} order[PT_RENDER_ACTIONS];uint8_t staging[256];
};
struct pt_mixed_owner {
    struct pt_allocator allocator;struct pt_paula_voices *paula;struct pt_wavetable_voices *amigus;
    struct pt_sampler_paula *pb;struct pt_sampler_wavetable *ab;
    struct pt_sampler *sampler;struct pt_project *project,snapshot;
    struct pt_amigus_wavetable_cache *backend;struct pt_amigus_reservation *reservation;
    struct pt_paula_voice_api pa;struct pt_wavetable_voice_api aa;
    int (*pq)(void *),(*aq)(void *);void *pc,*ac;
    struct pt_render_options options;struct pt_paula_render_caps caps;struct pt_playback_format format;
    struct pt_render_sequence *sequence;struct pt_mixed_report report;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_sampler_pin_job job;
    uint64_t pv,av;unsigned generation,slot,analyzed,ready,closing,drained[2];int8_t map[PT_CHANNEL_LIMIT];
    enum pt_mixed_owner_result failure;struct mixed_batch batch;
};
static int identities(struct pt_mixed_owner *s)
{
    return s->paula->song_owner==s && s->amigus->song_owner==s &&
        s->paula->bridge==s->pb && s->amigus->bridge==s->ab && s->ab->backend==s->backend &&
        s->backend->reservation==s->reservation &&
        s->paula->api.context==s->pa.context && s->paula->api.start==s->pa.start &&
        s->paula->api.stop==s->pa.stop && s->paula->api.control==s->pa.control &&
        s->amigus->api.context==s->aa.context && s->amigus->api.start==s->aa.start &&
        s->amigus->api.stop==s->aa.stop && s->amigus->api.control==s->aa.control && s->amigus->api.restore==s->aa.restore &&
        s->paula->quiesce==s->pq && s->amigus->quiesce==s->aq &&
        s->paula->quiesce_context==s->pc && s->amigus->quiesce_context==s->ac;
}
static enum pt_mixed_owner_result fail(struct pt_mixed_owner *s,enum pt_mixed_owner_result r)
{pt_mixed_stage_cancel(s);s->failure=r;s->closing=1;s->paula->closing=s->amigus->closing=1;return r;}
enum pt_mixed_owner_result pt_mixed_owner_current(struct pt_mixed_owner *s)
{
    unsigned i;struct pt_pcm pcm;struct pt_sample_version *pin;
    if(!s)return PT_MIXED_OWNER_INVALID;
    if(s->failure)return s->failure;
    if(s->closing)return PT_MIXED_OWNER_INVALID;
    s->snapshot.channels.selected=s->project->channels.selected;
    if(!identities(s) || s->paula->closing || s->amigus->closing ||
       s->pb->sampler!=s->sampler || s->ab->sampler!=s->sampler ||
       s->pb->project!=s->project || s->ab->project!=s->project ||
       s->pb->generation!=s->generation || s->ab->generation!=s->generation || s->sampler->generation!=s->generation ||
       s->pb->version!=s->pv || s->ab->version!=s->av ||
       s->pb->table!=s->project->samples || s->ab->table!=s->project->samples ||
       s->pb->count!=s->project->sample_count || s->ab->count!=s->project->sample_count ||
       memcmp(s->map,s->paula->map,sizeof(s->map)) || !pt_project_snapshot_equal(s->project,&s->snapshot) ||
       !pt_amigus_wavetable_cache_current(s->backend))return fail(s,PT_MIXED_OWNER_STALE);
    for(i=0;i<s->project->sample_count;++i)if(s->pin[i]) {
        if(pt_sampler_pin_current(s->sampler,s->project,i,s->generation,s->pin[i],&pcm,&pin)!=PT_EDIT_OK)
            return fail(s,PT_MIXED_OWNER_STALE);
        pt_sampler_unpin(pin);
    }
    return PT_MIXED_OWNER_OK;
}
enum pt_mixed_owner_result pt_mixed_owner_begin(struct pt_paula_voices *p,struct pt_wavetable_voices *w,
    const struct pt_render_options *o,const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,
    const struct pt_allocator *a,struct pt_mixed_owner **out)
{
    struct pt_mixed_owner *s;unsigned i;
    if(!p || !w || !p->bridge || !w->bridge || !o || !caps || !f || !a || !a->allocate || !a->release || !out ||
       p->song_owner || w->song_owner || p->closing || w->closing ||
       p->bridge->sampler!=w->bridge->sampler || p->bridge->project!=w->bridge->project ||
       !pt_paula_render_caps_valid(caps) || (f->bits!=8 && f->bits!=16) || f->channel || f->word_pad || f->little_endian>1)
        return PT_MIXED_OWNER_INVALID;
    for(i=0;i<PT_PAULA_VOICES;++i)if(p->voice[i].held)return PT_MIXED_OWNER_INVALID;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(w->voice[i].held)return PT_MIXED_OWNER_INVALID;
    if(pt_paula_voices_sync(p)!=1 || !pt_sampler_wavetable_sync(w->bridge) ||
       !w->bridge->backend || !pt_amigus_wavetable_cache_current(w->bridge->backend) ||
       w->bridge->backend->reservation->interrupt)return PT_MIXED_OWNER_INVALID;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_MIXED_OWNER_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;s->paula=p;s->amigus=w;s->pb=p->bridge;s->ab=w->bridge;
    s->sampler=s->pb->sampler;s->project=s->pb->project;s->snapshot=*s->project;
    s->backend=s->ab->backend;s->reservation=s->backend->reservation;
    s->pa=p->api;s->aa=w->api;s->pq=p->quiesce;s->aq=w->quiesce;s->pc=p->quiesce_context;s->ac=w->quiesce_context;
    s->options=*o;s->caps=*caps;s->format=*f;s->generation=s->sampler->generation;
    s->pv=s->pb->version;s->av=s->ab->version;memcpy(s->map,p->map,sizeof(s->map));
    p->song_owner=s;w->song_owner=s;*out=s;return PT_MIXED_OWNER_PREPARING;
}
enum pt_mixed_owner_result pt_mixed_owner_prepare(struct pt_mixed_owner *s,struct pt_mixed_report *out)
{
    enum pt_mixed_owner_result r=pt_mixed_owner_current(s);enum pt_edit_result e;unsigned ready;
    struct pt_pcm pcm;struct pt_sample_version *pin;
    if(r!=PT_MIXED_OWNER_OK)return r;
    if(!s->analyzed) {
        enum pt_mixed_result gate=pt_mixed_preflight(s->project,&s->options,s->map,&s->caps,&s->format,
            s->pa.control!=NULL,s->aa.control!=NULL,&s->allocator,&s->report,&s->sequence);
        if(out)*out=s->report;
        if(gate!=PT_MIXED_OK)return fail(s,gate==PT_MIXED_MEMORY?PT_MIXED_OWNER_MEMORY:PT_MIXED_OWNER_CAPABILITY);
        s->analyzed=1;return PT_MIXED_OWNER_PREPARING;
    }
    if(out)*out=s->report;
    if(s->ready)return PT_MIXED_OWNER_OK;
    if(s->job.owner) {
        e=pt_sampler_pin_job_step(&s->job,PT_SAMPLER_PIN_CHUNK,&pcm,&pin,&ready);
        if(e!=PT_EDIT_OK)return fail(s,e==PT_EDIT_CAPACITY?PT_MIXED_OWNER_MEMORY:PT_MIXED_OWNER_STALE);
        if(ready){s->pin[s->slot]=pin;++s->slot;}return PT_MIXED_OWNER_PREPARING;
    }
    while(s->slot<s->project->sample_count && !s->report.samples[0][s->slot] && !s->report.samples[1][s->slot])++s->slot;
    if(s->slot==s->project->sample_count){s->ready=1;return PT_MIXED_OWNER_OK;}
    e=pt_sampler_pin_job_begin(&s->job,s->sampler,s->project,s->slot,s->generation);
    if(e!=PT_EDIT_OK)return fail(s,e==PT_EDIT_CAPACITY?PT_MIXED_OWNER_MEMORY:PT_MIXED_OWNER_STALE);
    return PT_MIXED_OWNER_PREPARING;
}
int pt_mixed_owner_close(struct pt_mixed_owner **owner)
{
    struct pt_mixed_owner *s;struct pt_allocator a;unsigned i;
    if(!owner)return 0;
    s=*owner;if(!s)return 1;
    if(!identities(s))return 0;
    s->closing=1;s->paula->closing=s->amigus->closing=1;
    pt_mixed_stage_cancel(s);pt_sampler_pin_job_cancel(&s->job);pt_render_sequence_close(s->sequence);s->sequence=NULL;
    if(!s->drained[0])s->drained[0]=pt_paula_drain_owned(s->paula,s)==1;
    if(!s->drained[1])s->drained[1]=pt_wavetable_drain_owned(s->amigus,s)==1;
    if(!s->drained[0] || !s->drained[1] || s->reservation->interrupt)return 0;
    for(i=0;i<PT_PROJECT_SAMPLES;++i)pt_sampler_unpin(s->pin[i]);
    s->paula->song_owner=NULL;s->amigus->song_owner=NULL;
    a=s->allocator;a.release(a.context,s);*owner=NULL;return 1;
}

void pt_mixed_stage_cancel(struct pt_mixed_owner *s)
{
    unsigned i;if(!s)return;
    pt_paula_cancel(&s->batch.chip);pt_sampler_upload_cancel(&s->batch.upload);
    for(i=0;i<PT_RENDER_ACTIONS;++i)if(s->batch.entry[i].held)
        pt_cache_unpin(&s->backend->cache,s->batch.entry[i].lease);
    memset(&s->batch,0,sizeof(s->batch));
}
enum pt_mixed_owner_result pt_mixed_stage_begin(struct pt_mixed_owner *s,const struct pt_render_plan *plan)
{
    struct mixed_batch *b;unsigned i,j;uint16_t held=0;struct pt_wavetable_preflight_report report;
    enum pt_mixed_owner_result r=pt_mixed_owner_current(s);
    if(r!=PT_MIXED_OWNER_OK)return r;
    if(!s->ready)return PT_MIXED_OWNER_PREPARING;
    if(!plan || plan->count>PT_RENDER_ACTIONS || s->batch.phase)return PT_MIXED_OWNER_INVALID;
    b=&s->batch;b->count=plan->count;
    for(i=0;i<plan->count;++i) {
        const struct pt_render_action *a=plan->action+i;
        if(a->channel>=s->project->channels.count)goto refused;
        if(a->kind==PT_RENDER_TRIGGER) {
            for(j=0;j<s->project->sample_count;++j)if(a->voice.pcm==&s->project->samples[j].pcm)break;
            if(j==s->project->sample_count || !s->pin[j])goto refused;
        }else j=0;
        if(s->project->channels.track[a->channel].route==PT_PAULA) {
            b->order[i].route=PT_PAULA;b->order[i].index=b->split.count;
            b->split.action[b->split.count++]=*a;
        }
        else if(s->project->channels.track[a->channel].route==PT_AMIGUS) {
            b->order[i].route=PT_AMIGUS;b->order[i].index=b->wave.count;
            b->entry[b->wave.count].sample=j;b->wave.action[b->wave.count++]=*a;
        }else goto refused;
    }
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(s->amigus->voice[i].held && !s->amigus->voice[i].uncertain)held|=(uint16_t)(1U<<i);
    if(pt_wavetable_check_plan(s->project,s->options.rate,&b->wave,&s->format,s->aa.control!=NULL,&held,&report)!=PT_WAVETABLE_COMPATIBLE ||
       !pt_paula_prepare_begin_owned(&b->chip,s->paula,s->pv,s->options.rate,&b->split,&s->caps,s->pin,s))goto refused;
    memcpy(b->pv,s->paula->voice,sizeof(b->pv));memcpy(b->av,s->amigus->voice,sizeof(b->av));
    b->phase=1;return PT_MIXED_OWNER_PREPARING;
refused:pt_mixed_stage_cancel(s);return PT_MIXED_OWNER_CAPABILITY;
}
static int location(struct pt_mixed_owner *s,unsigned i,uint32_t *address,uint32_t *bytes)
{
    struct mixed_batch *b=&s->batch;struct pt_cache_lease lease=b->entry[i].lease;
    return b->entry[i].held && pt_cache_data(&s->backend->cache,lease) &&
        s->backend->cache.entry[lease.slot].valid==1 && s->backend->cache.entry[lease.slot].version==s->av &&
        pt_amigus_wavetable_cache_location(s->backend,lease,address,bytes);
}
enum pt_mixed_owner_result pt_mixed_stage_step(struct pt_mixed_owner *s)
{
    struct mixed_batch *b;unsigned i;uint32_t address,bytes;enum pt_cache_result cache;
    enum pt_mixed_owner_result r=pt_mixed_owner_current(s);
    if(r!=PT_MIXED_OWNER_OK)return r;
    b=&s->batch;if(!b->phase)return PT_MIXED_OWNER_INVALID;
    if(memcmp(b->pv,s->paula->voice,sizeof(b->pv)) || memcmp(b->av,s->amigus->voice,sizeof(b->av)))goto refused;
    if(b->phase==4)return PT_MIXED_OWNER_OK;
    if(b->phase==1) {
        cache=pt_paula_prepare_step_owned(&b->chip);
        if(cache==PT_CACHE_LOAD)b->phase=2;
        else if(cache!=PT_CACHE_PENDING)goto refused;
        return PT_MIXED_OWNER_PREPARING;
    }
    if(b->phase==2) {
        /* Skip <=64 nontrigger actions; one reserve/hit/upload per call. */
        while(b->index<b->wave.count && b->wave.action[b->index].kind!=PT_RENDER_TRIGGER)++b->index;
        if(b->index==b->wave.count){b->index=0;b->phase=3;return PT_MIXED_OWNER_PREPARING;}
        i=b->index;
        if(b->upload.bridge)cache=pt_sampler_upload_step(&b->upload,b->staging,sizeof(b->staging),&b->entry[i].lease);
        else cache=pt_sampler_upload_begin_prepared(&b->upload,s->ab,b->entry[i].sample,s->generation,s->av,
            s->pin[b->entry[i].sample],&s->format,&b->entry[i].lease);
        if(cache==PT_CACHE_LOAD || cache==PT_CACHE_HIT){b->entry[i].held=1;++b->index;}
        else if(cache!=PT_CACHE_PENDING)goto refused;
        return PT_MIXED_OWNER_PREPARING;
    }
    if(b->index<b->wave.count) {
        const struct pt_render_action *a=b->wave.action+b->index;i=b->index;
        if(a->kind==PT_RENDER_TRIGGER) {
            if(!location(s,i,&address,&bytes) || !pt_amigus_render_voice(&a->voice,s->options.rate,a->gain,&s->format,address,bytes,&b->entry[i].command))goto refused;
            b->entry[i].address=address;b->entry[i].bytes=bytes;
        }else if(a->kind==PT_RENDER_CONTROL) {
            if(!pt_amigus_render_control(a->voice.step,s->options.rate,a->gain,&b->entry[i].command.rate,&b->entry[i].command.left,&b->entry[i].command.right))goto refused;
        }else if(a->kind!=PT_RENDER_STOP)goto refused;
        ++b->index;return PT_MIXED_OWNER_PREPARING;
    }
    for(i=0;i<b->wave.count;++i)if(b->wave.action[i].kind==PT_RENDER_TRIGGER &&
        (!location(s,i,&address,&bytes) || address!=b->entry[i].address || bytes!=b->entry[i].bytes))goto refused;
    b->phase=4;return PT_MIXED_OWNER_OK;
refused:pt_mixed_stage_cancel(s);return PT_MIXED_OWNER_CAPABILITY;
}

enum pt_mixed_owner_result pt_mixed_stage_commit(struct pt_mixed_owner *s)
{
    struct mixed_batch *b;unsigned i,j;uint32_t address,bytes;int stop_route=-1,stop_channel=-1,result;
    enum pt_mixed_owner_result r=pt_mixed_owner_current(s);
    if(r!=PT_MIXED_OWNER_OK)return r;
    b=&s->batch;if(b->phase!=4)return PT_MIXED_OWNER_INVALID;
    if(memcmp(b->pv,s->paula->voice,sizeof(b->pv)) || memcmp(b->av,s->amigus->voice,sizeof(b->av)) ||
       !pt_paula_prepared_ready_owned(&b->chip))goto refused;
    /* Immutable plans and unchanged readers preserve the already-gated state
     * transitions. Recheck every live/candidate lease before ANY callback. */
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(b->av[i].held) {
        struct pt_cache_lease lease=b->av[i].lease;
        if(b->av[i].uncertain || !pt_cache_data(&s->backend->cache,lease) ||
           s->backend->cache.entry[lease.slot].valid!=1 || s->backend->cache.entry[lease.slot].version!=s->av ||
           !pt_amigus_wavetable_cache_location(s->backend,lease,&address,&bytes))goto refused;
    }
    for(i=0;i<b->wave.count;++i)if(b->wave.action[i].kind==PT_RENDER_TRIGGER &&
       (!location(s,i,&address,&bytes) || address!=b->entry[i].address || bytes!=b->entry[i].bytes))goto refused;
    for(i=0;i<b->count;++i) {
        j=b->order[i].index;
        if(b->order[i].route==PT_PAULA) {
            result=pt_paula_prepared_action_owned(&b->chip,j);
            if(result!=1) {
                if(result==-2){stop_route=PT_PAULA;stop_channel=(int)b->split.action[j].channel;}
                goto failed;
            }
        }else {
            const struct pt_render_action *a=b->wave.action+j;
            struct pt_wavetable_voice *v=s->amigus->voice+a->channel;
            switch(a->kind) {
            case PT_RENDER_TRIGGER:
                if(pt_wavetable_stop_owned(s->amigus,a->channel,s)!=1){stop_route=PT_AMIGUS;stop_channel=(int)a->channel;goto failed;}
                v->lease=b->entry[j].lease;v->held=v->uncertain=1;b->entry[j].held=0;
                if(s->aa.start(s->aa.context,a->channel,&b->entry[j].command)!=1)goto failed;
                v->uncertain=0;break;
            case PT_RENDER_CONTROL:
                if(!v->held || v->uncertain)goto failed;
                v->uncertain=1;
                if(s->aa.control(s->aa.context,a->channel,b->entry[j].command.rate,
                   b->entry[j].command.left,b->entry[j].command.right)!=1)goto failed;
                v->uncertain=0;break;
            case PT_RENDER_STOP:if(pt_wavetable_stop_owned(s->amigus,a->channel,s)!=1){stop_route=PT_AMIGUS;stop_channel=(int)a->channel;goto failed;}break;
            default:goto failed;
            }
        }
    }
    pt_mixed_stage_cancel(s);return PT_MIXED_OWNER_OK;
refused:pt_mixed_stage_cancel(s);return PT_MIXED_OWNER_CAPABILITY;
failed:
    fail(s,PT_MIXED_OWNER_DEVICE);
    for(i=0;i<PT_PAULA_VOICES;++i)if(s->paula->voice[i].held) {
        s->paula->voice[i].uncertain=1;
        if(stop_route!=PT_PAULA || stop_channel!=s->paula->voice[i].track)
            pt_paula_stop_owned(s->paula,(unsigned)s->paula->voice[i].track,s);
    }
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(s->amigus->voice[i].held) {
        s->amigus->voice[i].uncertain=1;
        if(stop_route!=PT_AMIGUS || stop_channel!=(int)i)pt_wavetable_stop_owned(s->amigus,i,s);
    }
    return PT_MIXED_OWNER_DEVICE;
}
