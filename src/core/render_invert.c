#include "render_invert.h"
#include "render_commands.h"
#include "invert_bank.h"
#include "invert_sequence.h"
#include <string.h>
struct staging {
    struct pt_project playback;
    struct pt_sample samples[PT_PROJECT_SAMPLES];
    struct pt_invert_bank bank;
    struct pt_invert_sequence sequence;
    uint16_t tracks;
};
static int tick(void *ctx,const struct pt_flow *flow)
{
    struct staging *s=ctx;
    return pt_invert_sequence_tick(&s->sequence,flow,s->tracks,s->bank.entries,s->bank.count)==PT_PCM_OK;
}
static enum pt_render_result stage_open(const struct pt_project *p,const struct pt_render_options *o,
    size_t budget,const struct pt_allocator *a,struct staging **out)
{
    uint8_t used[PT_PROJECT_PATTERNS]={0},selected[PT_PROJECT_SAMPLES]={0};
    unsigned pat,row,ch,i;struct staging *s;
    enum pt_invert_bank_result bank_result;
    if(!p || !o || !out || !a || !a->allocate || !a->release ||
       pt_project_validate(p,NULL)!=PT_PROJECT_OK || o->pattern_only>1 ||
       (o->pattern_only && o->pattern>=p->pattern_count))return PT_RENDER_INVALID;
    if(o->pattern_only)used[o->pattern]=1;
    else for(i=0;i<p->order_count;++i)used[p->orders[i]]=1;
    for(pat=0;pat<p->pattern_count;++pat)if(used[pat])for(row=0;row<64;++row)for(ch=0;ch<p->channels.count;++ch) {
        const struct pt_event *e=p->events+((size_t)pat*64+row)*p->channels.count+ch;
        if(e->slice)return PT_RENDER_EFFECT;
        if(e->instrument)selected[e->instrument-1]=1;
    }
    for(i=0;i<p->sample_count;++i)if(selected[i]) {
        const struct pt_sample *sample=p->samples+i;
        if(sample->pcm.bits!=8 || sample->pcm.channels!=1 || sample->pcm.frames<2 ||
           sample->pcm.frames>131070 || (sample->pcm.frames&1) || sample->interpolation ||
           sample->loop>PT_LOOP_FORWARD || (sample->loop &&
           (sample->loop_end-sample->loop_start<4 || ((sample->loop_start|sample->loop_end)&1))))return PT_RENDER_SAMPLE;
    }
    if(budget<sizeof(*s))return PT_RENDER_MEMORY;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_RENDER_MEMORY;
    memset(s,0,sizeof(*s));s->playback=*p;s->playback.samples=s->samples;s->tracks=(uint16_t)((1UL<<p->channels.count)-1);
    /* Selection/mute/solo affect audio only. Every channel keeps its EFx clock
       and can mutate a sample heard by another selected channel. */
    memcpy(s->samples,p->samples,p->sample_count*sizeof(*s->samples));
    bank_result=pt_invert_bank_open(&s->bank,p->samples,p->sample_count,selected,budget-sizeof(*s),a);
    if(bank_result!=PT_INVERT_BANK_OK) {
        a->release(a->context,s);
        return bank_result==PT_INVERT_BANK_INVALID?PT_RENDER_SAMPLE:PT_RENDER_MEMORY;
    }
    for(i=0;i<p->sample_count;++i)if(selected[i]) {
        s->samples[i].pcm=s->bank.entries[i].pcm;
        /* mt_Init clears a non-looping sample's first DMA word. Apply this
           only to private playback storage, never the editable master. */
        if(!s->samples[i].loop)s->samples[i].pcm.data[0]=s->samples[i].pcm.data[1]=0;
    }
    *out=s;return PT_RENDER_OK;
}
static void stage_close(struct staging *s,const struct pt_allocator *a)
{if(s) {pt_invert_bank_close(&s->bank);a->release(a->context,s);}}
enum pt_render_result pt_render_invert_stream(const struct pt_project *p,const struct pt_render_options *o,
    pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *progress_ctx,
    struct pt_render_report *out,size_t budget,const struct pt_allocator *a)
{
    struct staging *s;struct pt_render_mutation mutation;enum pt_render_result result;
    if(!sink || !out)return PT_RENDER_INVALID;
    result=stage_open(p,o,budget,a,&s);if(result!=PT_RENDER_OK)return result;
    mutation.playback=&s->playback;mutation.context=s;mutation.tick=tick;
    result=pt_render_mutating_allocated(p,o,sink,sink_ctx,progress,progress_ctx,out,a,&mutation);
    stage_close(s,a);return result;
}
struct pt_render_invert_session {
    struct pt_allocator allocator;struct staging *stage;struct pt_render_sequence *sequence;
    struct pt_render_interval interval;struct pt_pcm block;int32_t data[512];
    uint32_t remaining;unsigned pending,done;enum pt_render_result failure;
};
void pt_render_invert_stop(struct pt_render_invert_session *s)
{
    if(!s)return;
    /* Discard every live private voice before freeing its sample bank. Queued
       audio is a distinct copy and may remain leased by the consumer. */
    pt_render_sequence_close(s->sequence);s->sequence=NULL;
    stage_close(s->stage,&s->allocator);s->stage=NULL;
    s->done=1;s->pending=0;s->remaining=0;
}
void pt_render_invert_close(struct pt_render_invert_session *s)
{if(s) {struct pt_allocator a=s->allocator;pt_render_invert_stop(s);a.release(a.context,s);}}
enum pt_render_result pt_render_invert_open(const struct pt_project *p,const struct pt_render_options *o,
    size_t budget,const struct pt_allocator *a,struct pt_render_invert_session **out)
{
    struct pt_render_invert_session *s;struct pt_render_mutation mutation;enum pt_render_result result;
    if(!out || !o || o->rate!=48000 || o->bits!=24 || !a || !a->allocate || !a->release)return PT_RENDER_INVALID;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_RENDER_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;
    result=stage_open(p,o,budget,a,&s->stage);if(result!=PT_RENDER_OK)goto fail;
    mutation=(struct pt_render_mutation){&s->stage->playback,s->stage,tick};
    result=pt_render_mutating_sequence_open(p,o,a,&s->sequence,&mutation);if(result!=PT_RENDER_OK)goto fail;
    s->block=(struct pt_pcm){s->data,512,0,48000,2,24};*out=s;return PT_RENDER_OK;
fail:
    pt_render_invert_close(s);return result;
}
enum pt_render_result pt_render_invert_pull(struct pt_render_invert_session *s,unsigned max_frames,
    const struct pt_pcm **pcm,unsigned *done)
{
    enum pt_render_result result;
    if(!s || !pcm || !done || !max_frames || max_frames>256)return PT_RENDER_INVALID;
    *pcm=NULL;*done=s->done;
    if(s->failure)return s->failure;
    if(s->done)return PT_RENDER_OK;
    if(!s->pending) {
        result=pt_render_sequence_next(s->sequence,&s->interval);if(result!=PT_RENDER_OK)goto fail;
        s->pending=1;s->remaining=s->interval.frames;return PT_RENDER_OK;
    }
    if(s->remaining) {
        s->block.frames=s->remaining<max_frames?s->remaining:max_frames;
        result=pt_render_mutating_sequence_read(s->sequence,&s->block);if(result!=PT_RENDER_OK)goto fail;
        s->remaining-=s->block.frames;if(s->interval.emit)*pcm=&s->block;return PT_RENDER_OK;
    }
    result=pt_render_mutating_sequence_complete(s->sequence);if(result!=PT_RENDER_OK)goto fail;
    if(s->interval.end) {pt_render_invert_stop(s);*done=1;return PT_RENDER_OK;}
    s->pending=0;return PT_RENDER_OK;
fail:
    s->failure=result;pt_render_invert_stop(s);*done=1;return result;
}
