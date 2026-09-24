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
enum pt_render_result pt_render_invert_stream(const struct pt_project *p,const struct pt_render_options *o,
    pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *progress_ctx,
    struct pt_render_report *out,size_t budget,const struct pt_allocator *a)
{
    uint8_t used[PT_PROJECT_PATTERNS]={0},selected[PT_PROJECT_SAMPLES]={0};
    unsigned pat,row,ch,i;struct staging *s;struct pt_render_mutation mutation;
    enum pt_invert_bank_result bank_result;enum pt_render_result result;
    if(!p || !o || !sink || !out || !a || !a->allocate || !a->release ||
       pt_project_validate(p,NULL)!=PT_PROJECT_OK || o->pattern_only>1 ||
       (o->pattern_only && o->pattern>=p->pattern_count))return PT_RENDER_INVALID;
    /* Until stem mutation dependencies are represented explicitly, refuse a
       partial track selection rather than changing shared sample semantics. */
    if(o->tracks!=(uint16_t)((1UL<<p->channels.count)-1))return PT_RENDER_EFFECT;
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
           sample->loop!=PT_LOOP_FORWARD || sample->loop_end-sample->loop_start<4 ||
           ((sample->loop_start|sample->loop_end)&1))return PT_RENDER_SAMPLE;
    }
    if(budget<sizeof(*s))return PT_RENDER_MEMORY;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_RENDER_MEMORY;
    memset(s,0,sizeof(*s));s->playback=*p;s->playback.samples=s->samples;s->tracks=o->tracks;
    memcpy(s->samples,p->samples,p->sample_count*sizeof(*s->samples));
    bank_result=pt_invert_bank_open(&s->bank,p->samples,p->sample_count,selected,budget-sizeof(*s),a);
    if(bank_result!=PT_INVERT_BANK_OK) {
        a->release(a->context,s);
        return bank_result==PT_INVERT_BANK_INVALID?PT_RENDER_SAMPLE:PT_RENDER_MEMORY;
    }
    for(i=0;i<p->sample_count;++i)if(selected[i])s->samples[i].pcm=s->bank.entries[i].pcm;
    mutation.playback=&s->playback;mutation.context=s;mutation.tick=tick;
    result=pt_render_mutating_allocated(p,o,sink,sink_ctx,progress,progress_ctx,out,a,&mutation);
    pt_invert_bank_close(&s->bank);a->release(a->context,s);return result;
}
