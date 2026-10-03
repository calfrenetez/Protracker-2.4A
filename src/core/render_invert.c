#include "render_invert.h"
#include "render_commands.h"
#include "invert_bank.h"
#include "invert_sequence.h"
#include <string.h>
struct staging {
    struct pt_project playback;
    struct pt_sample samples[PT_PROJECT_SAMPLES];
    struct pt_invert_bank bank;
    struct pt_invert_bank_job preparation;
    struct pt_invert_sequence sequence;
    uint16_t tracks;
};
static int tick(void *ctx,const struct pt_flow *flow)
{
    struct staging *s=ctx;
    return pt_invert_sequence_tick(&s->sequence,flow,s->tracks,s->bank.entries,s->bank.count)==PT_PCM_OK;
}
static enum pt_render_result stage_begin(const struct pt_project *p,const struct pt_render_options *o,
    size_t budget,const struct pt_allocator *a,struct staging **out)
{
    uint8_t used[PT_PROJECT_PATTERNS]={0},selected[PT_PROJECT_SAMPLES]={0};
    unsigned pat,row,ch,i;size_t offset=0;struct staging *s;
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
    bank_result=pt_invert_bank_begin(&s->preparation,&s->bank,p->samples,p->sample_count,selected,budget-sizeof(*s),a);
    if(bank_result!=PT_INVERT_BANK_OK) {
        a->release(a->context,s);
        return bank_result==PT_INVERT_BANK_INVALID?PT_RENDER_SAMPLE:PT_RENDER_MEMORY;
    }
    for(i=0;i<p->sample_count;++i)if(selected[i]) {
        /* Stable private addresses exist before copying, but no sequencer can
         * read them until the complete bank and measured timeline are ready. */
        s->samples[i].pcm.data=s->preparation.bank.storage+offset;
        s->samples[i].pcm.capacity=s->samples[i].pcm.frames;offset+=s->samples[i].pcm.frames;
    }
    *out=s;return PT_RENDER_OK;
}
static enum pt_render_result stage_prepare(struct staging *s,unsigned *ready)
{
    enum pt_invert_bank_result result;unsigned i;
    result=pt_invert_bank_prepare(&s->preparation,ready);
    if(result!=PT_INVERT_BANK_OK)return PT_RENDER_SAMPLE;
    if(*ready)for(i=0;i<s->bank.count;++i)if(s->bank.entries[i].source) {
        /* Descriptors already have these stable private addresses. mt_Init
         * changes only the complete private first words (<=2040 bytes total). */
        if(!s->samples[i].loop)s->samples[i].pcm.data[0]=s->samples[i].pcm.data[1]=0;
    }
    return PT_RENDER_OK;
}
static void stage_close(struct staging *s,const struct pt_allocator *a)
{if(s) {pt_invert_bank_cancel(&s->preparation);pt_invert_bank_close(&s->bank);a->release(a->context,s);}}
static enum pt_render_result stage_open(const struct pt_project *p,const struct pt_render_options *o,
    size_t budget,const struct pt_allocator *a,struct staging **out)
{
    struct staging *s=NULL;unsigned ready=0;enum pt_render_result result=stage_begin(p,o,budget,a,&s);
    while(result==PT_RENDER_OK && !ready)result=stage_prepare(s,&ready);
    if(result!=PT_RENDER_OK){stage_close(s,a);return result;}
    *out=s;return PT_RENDER_OK;
}
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
    uint32_t remaining;unsigned pending,done,copied,ready;enum pt_render_result failure;
};
static int overlaps(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!an || !bn)return 0;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return 1;
    return x<y+bn && y<x+an;
}
int pt_render_invert_output_disjoint(const struct pt_render_invert_session *s,const void *out,size_t bytes)
{
    const struct staging *stage;const struct pt_invert_bank *bank;size_t i;
    if(!s || !out)return 0;
    stage=s->stage;
    if(overlaps(out,bytes,s,sizeof(*s)))return 0;
    if(!stage)return 1;
    if(overlaps(out,bytes,stage,sizeof(*stage)) ||
       !pt_render_sequence_output_disjoint(s->sequence,out,bytes))return 0;
    bank=stage->bank.entries?&stage->bank:&stage->preparation.bank;
    if(bank->count>PT_PROJECT_SAMPLES || bank->count>SIZE_MAX/sizeof(*bank->entries) ||
       bank->allocated_bytes<bank->count*sizeof(*bank->entries) ||
       overlaps(out,bytes,bank->entries,bank->count*sizeof(*bank->entries)) ||
       overlaps(out,bytes,bank->storage,bank->allocated_bytes-bank->count*sizeof(*bank->entries)))return 0;
    for(i=0;i<stage->playback.sample_count;++i) {
        const struct pt_pcm *pcm=&stage->samples[i].pcm;
        if(pcm->capacity>SIZE_MAX/sizeof(int32_t) ||
           overlaps(out,bytes,pcm->data,pcm->capacity*sizeof(int32_t)))return 0;
    }
    for(i=0;i<bank->count;++i)if(bank->entries[i].source) {
        const struct pt_pcm *pcm=bank->entries[i].source;
        if(pcm->capacity>SIZE_MAX/sizeof(int32_t) ||
           overlaps(out,bytes,pcm->data,pcm->capacity*sizeof(int32_t)))return 0;
    }
    return 1;
}
void pt_render_invert_stop(struct pt_render_invert_session *s)
{
    if(!s)return;
    /* Discard every live private voice before freeing its sample bank. Queued
       audio is a distinct copy and may remain leased by the consumer. */
    pt_render_sequence_close(s->sequence);s->sequence=NULL;
    stage_close(s->stage,&s->allocator);s->stage=NULL;
    s->done=1;s->pending=0;s->remaining=0;s->ready=0;
}
void pt_render_invert_close(struct pt_render_invert_session *s)
{if(s) {struct pt_allocator a=s->allocator;pt_render_invert_stop(s);a.release(a.context,s);}}
enum pt_render_result pt_render_invert_begin(const struct pt_project *p,const struct pt_render_options *o,
    size_t budget,const struct pt_allocator *a,struct pt_render_invert_session **out)
{
    struct pt_render_invert_session *s;struct pt_render_mutation mutation;enum pt_render_result result;
    if(!out || !o || o->rate!=48000 || o->bits!=24 || !a || !a->allocate || !a->release)return PT_RENDER_INVALID;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_RENDER_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;
    result=stage_begin(p,o,budget,a,&s->stage);if(result!=PT_RENDER_OK)goto fail;
    mutation=(struct pt_render_mutation){&s->stage->playback,s->stage,tick};
    result=pt_render_mutating_sequence_begin(p,o,a,&s->sequence,&mutation);if(result!=PT_RENDER_OK)goto fail;
    if(!pt_render_invert_output_disjoint(s,out,sizeof(*out))){result=PT_RENDER_INVALID;goto fail;}
    s->block=(struct pt_pcm){s->data,512,0,48000,2,24};*out=s;return PT_RENDER_OK;
fail:
    pt_render_invert_close(s);return result;
}
enum pt_render_result pt_render_invert_prepare(struct pt_render_invert_session *s,unsigned *ready)
{
    enum pt_render_result result;
    if(!s || !ready || !pt_render_invert_output_disjoint(s,ready,sizeof(*ready)))return PT_RENDER_INVALID;
    *ready=0;if(s->failure)return s->failure;
    if(s->done)return PT_RENDER_INVALID;
    if(s->ready){*ready=1;return PT_RENDER_OK;}
    if(!s->copied)result=stage_prepare(s->stage,&s->copied);
    else result=pt_render_sequence_prepare(s->sequence,256,&s->ready);
    if(result!=PT_RENDER_OK){s->failure=result;pt_render_invert_stop(s);return result;}
    *ready=s->ready;return PT_RENDER_OK;
}
enum pt_render_result pt_render_invert_open(const struct pt_project *p,const struct pt_render_options *o,
    size_t budget,const struct pt_allocator *a,struct pt_render_invert_session **out)
{
    struct pt_render_invert_session *s=NULL;unsigned ready=0;enum pt_render_result result;
    if(!out)return PT_RENDER_INVALID;
    result=pt_render_invert_begin(p,o,budget,a,&s);
    while(result==PT_RENDER_OK && !ready)result=pt_render_invert_prepare(s,&ready);
    if(result==PT_RENDER_OK && !pt_render_invert_output_disjoint(s,out,sizeof(*out)))result=PT_RENDER_INVALID;
    if(result!=PT_RENDER_OK){pt_render_invert_close(s);return result;}
    *out=s;return PT_RENDER_OK;
}
enum pt_render_result pt_render_invert_pull(struct pt_render_invert_session *s,unsigned max_frames,
    const struct pt_pcm **pcm,unsigned *done)
{
    enum pt_render_result result;
    if(!s || !pcm || !done || !max_frames || max_frames>256 ||
       !pt_render_invert_output_disjoint(s,pcm,sizeof(*pcm)) ||
       !pt_render_invert_output_disjoint(s,done,sizeof(*done)) ||
       overlaps(pcm,sizeof(*pcm),done,sizeof(*done)))return PT_RENDER_INVALID;
    *pcm=NULL;*done=s->done;
    if(s->failure)return s->failure;
    if(s->done)return PT_RENDER_OK;
    if(!s->ready)return PT_RENDER_INVALID;
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
