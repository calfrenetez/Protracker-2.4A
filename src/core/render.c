#include "render.h"
#include "pitch.h"
#include <string.h>
struct sample_range {uint32_t start,length,trigger_start,trigger_length;uint8_t offset,loaded,retrigger;};
struct run {
    struct pt_project view;
    uint16_t order;
    struct pt_timeline timeline;
    struct pt_pitch pitch;
    uint16_t tracks,offset_tracks;
    struct sample_range range[16];
    uint64_t frames;
    uint8_t started,pending_end;
};
static uint16_t offset_tracks(const struct pt_project *p,const struct pt_render_options *o)
{
    uint8_t used[256]={0};unsigned pat,row,ch;uint16_t mask=0;
    if(o->pattern_only)used[o->pattern]=1;
    else for(pat=0;pat<p->order_count;++pat)used[p->orders[pat]]=1;
    for(pat=0;pat<p->pattern_count;++pat)if(used[pat])for(row=0;row<64;++row)for(ch=0;ch<p->channels.count;++ch)
        {
            const struct pt_event *e=p->events+((size_t)pat*64+row)*p->channels.count+ch;
            if(e->effect==9 || (e->effect==14 && (e->parameter>>4)==9))mask|=(uint16_t)(1U<<ch);
        }
    return mask&o->tracks;
}
static void apply_offset(struct sample_range *v,unsigned parameter)
{
    uint32_t amount;if(parameter)v->offset=(uint8_t)parameter;amount=(uint32_t)v->offset*256;
    if(amount<v->length) {v->start+=amount;v->length-=amount;}
    else v->length=2;
}
static int ranges_tick(struct run *r)
{
    const struct pt_flow *f=&r->timeline.flow;unsigned ch;
    for(ch=0;ch<r->view.channels.count;++ch)if(r->offset_tracks&(1U<<ch)) {
        const struct pt_event *e=r->view.events+((size_t)r->view.orders[f->played_order]*64+f->played_row)*r->view.channels.count+ch;
        struct sample_range *v=r->range+ch;
        v->retrigger=0;
        if(f->fresh) {
            if(e->instrument) {
                const struct pt_sample *s=r->view.samples+e->instrument-1;
                v->start=0;v->length=s->loop && s->loop_start?s->loop_end:s->pcm.frames;v->loaded=1;
            }
            if(e->kind==PT_NOTE_PERIOD && e->effect!=3 && e->effect!=5) {
                if(e->effect==9)apply_offset(v,e->parameter);
                v->trigger_start=v->start;v->trigger_length=v->length;
            }
            if(e->effect==9)apply_offset(v,e->parameter);
        }
        if(f->effect[ch]==14 && (f->parameter[ch]>>4)==9 && (f->parameter[ch]&15) &&
           !(!f->counter && e->kind==PT_NOTE_PERIOD) && !(f->counter%(f->parameter[ch]&15)) && v->loaded) {
            v->retrigger=1;v->trigger_start=v->start;v->trigger_length=v->length;
        }
        if(v->loaded) {
            uint32_t frames=r->view.samples[r->pitch.channel[ch].instrument-1].pcm.frames;
            if(v->start>frames || v->length>frames-v->start || v->trigger_start>frames || v->trigger_length>frames-v->trigger_start)return 0;
        }
    }
    return 1;
}
static enum pt_render_result preflight(const struct pt_project *p,const struct pt_render_options *o)
{
    unsigned ch,pat,row;uint8_t used[256]={0};uint16_t offsets;
    if(!p || !o || pt_project_validate(p,NULL)!=PT_PROJECT_OK || !o->tick_limit || !o->frame_limit ||
       (o->rate!=44100 && o->rate!=48000) || (o->bits!=16 && o->bits!=24) ||
       !o->tracks || (o->tracks>>p->channels.count) || o->gain_q16>65536 ||
       o->pattern_only>1 || o->include_lead_in>1 ||
       (o->pattern_only?o->pattern>=p->pattern_count:o->start_order>=p->order_count))return PT_RENDER_INVALID;
    offsets=offset_tracks(p,o);
    for(ch=0;ch<p->channels.count;++ch)if((o->tracks&(1U<<ch)) && p->channels.track[ch].route==PT_MIDI)return PT_RENDER_ROUTE;
    if(o->pattern_only)used[o->pattern]=1;
    else for(pat=0;pat<p->order_count;++pat)used[p->orders[pat]]=1;
    for(pat=0;pat<p->pattern_count;++pat)if(used[pat])for(row=0;row<64;++row)for(ch=0;ch<p->channels.count;++ch) {
        const struct pt_event *e=p->events+((size_t)pat*64+row)*p->channels.count+ch;
        if(!(o->tracks&(1U<<ch)))continue;
        if(e->kind==PT_NOTE_MIDI || (e->instrument && e->kind!=PT_NOTE_PERIOD))return PT_RENDER_EFFECT;
        if(!(e->effect==0 || e->effect==1 || e->effect==2 || e->effect==3 || e->effect==4 || e->effect==5 || e->effect==6 || e->effect==9 || (e->effect>=10 && e->effect<=13) || e->effect==15 ||
             (e->effect==14 && ((e->parameter>>4)==1 || (e->parameter>>4)==2 || (e->parameter>>4)==4 || (e->parameter>>4)==6 || (e->parameter>>4)==9 || ((e->parameter>>4)>=10 && (e->parameter>>4)<=12) ||
                               (e->parameter>>4)==14))))return PT_RENDER_EFFECT;
        if((e->effect==3 || e->effect==5) && e->slice)return PT_RENDER_EFFECT;
        if((offsets&(1U<<ch)) && e->slice)return PT_RENDER_EFFECT;
        if(e->instrument) {
            const struct pt_sample *s=p->samples+e->instrument-1;
            if((offsets&(1U<<ch)) && (s->pcm.bits!=8 || s->pcm.channels!=1 || s->pcm.frames<2 ||
               s->pcm.frames>131070 || (s->pcm.frames&1) || s->loop>PT_LOOP_FORWARD ||
               (s->loop && ((s->loop_start|s->loop_end)&1))))return PT_RENDER_SAMPLE;
            if(s->finetune || s->loop==PT_LOOP_CROSSFADE)return PT_RENDER_SAMPLE;
        }
    }
    return PT_RENDER_OK;
}
static int start_run(struct run *r,const struct pt_project *p,const struct pt_render_options *o)
{
    memset(r,0,sizeof(*r));r->view=*p;r->started=o->include_lead_in;
    r->tracks=o->tracks;r->offset_tracks=offset_tracks(p,o);pt_pitch_init(&r->pitch);
    if(o->pattern_only) {r->order=o->pattern;r->view.orders=&r->order;r->view.order_count=1;}
    return pt_timeline_init(&r->timeline,&r->view,PT_FLOW_EXTENDED256,o->pattern_only?0:o->start_order,
                            o->rate,o->tick_limit,o->frame_limit)==PT_TIMELINE_TICK;
}
static enum pt_render_result next_tick(struct run *r,struct pt_tick_span *span,unsigned *end)
{
    uint32_t returned=r->timeline.flow.returns;enum pt_timeline_result result=pt_timeline_next(&r->timeline,span);
    *end=0;
    if(result!=PT_TIMELINE_TICK)return result==PT_TIMELINE_TICK_LIMIT?PT_RENDER_TICK_LIMIT:
        result==PT_TIMELINE_FRAME_LIMIT?PT_RENDER_FRAME_LIMIT:PT_RENDER_INVALID;
    if(!r->started) {
        span->frames=0;
        if(r->timeline.flow.fresh)r->started=1;
    }
    r->frames+=span->frames;
    if(!r->timeline.flow.active)*end=1; /* F00. */
    else if(r->pending_end && r->timeline.flow.fresh)*end=2;
    if(r->timeline.flow.returns!=returned)r->pending_end=1;
    if(!*end) {
        unsigned ch;pt_pitch_tick(&r->pitch,&r->timeline.flow,r->tracks);
        if(!ranges_tick(r))return PT_RENDER_SAMPLE;
        /* A zero register period has no defined reference PCM rate yet.
           Reject it in measurement, before sinks, staging or bounce commit. */
        for(ch=0;ch<r->view.channels.count;++ch)
            if(r->pitch.channel[ch].unsupported || (r->pitch.channel[ch].sounding && !r->pitch.channel[ch].output))return PT_RENDER_EFFECT;
    }
    return PT_RENDER_OK;
}
static void report_run(const struct run *r,unsigned end,uint64_t clips,struct pt_render_report *out)
{
    struct pt_render_report result;memset(&result,0,sizeof(result));result.frames=r->frames;
    result.ticks=r->timeline.flow.ticks;result.clipped=clips;result.end=end==1?PT_RENDER_F00:PT_RENDER_POSITION_RETURN;*out=result;
}
enum pt_render_result pt_render_measure(const struct pt_project *p,const struct pt_render_options *o,
                                       pt_render_progress progress,void *ctx,struct pt_render_report *out)
{
    struct run r;struct pt_tick_span span;unsigned end;enum pt_render_result result=preflight(p,o);
    if(!out)return PT_RENDER_INVALID;
    if(result!=PT_RENDER_OK)return result;
    if(!start_run(&r,p,o))return PT_RENDER_INVALID;
    do {
        if(progress && !progress(ctx,PT_RENDER_ANALYSE,r.timeline.flow.ticks,r.frames))return PT_RENDER_CANCELLED;
        result=next_tick(&r,&span,&end);if(result!=PT_RENDER_OK)return result;
    } while(!end);
    report_run(&r,end,0,out);return PT_RENDER_OK;
}
static uint64_t step(const struct pt_sample *s,unsigned period,unsigned rate)
{return ((uint64_t)s->pcm.rate*428<<32)/((uint64_t)rate*period);}
static void gains_for(const struct pt_project *p,const struct pt_render_options *o,
                      const struct pt_voice *voices,const uint8_t *volume,const uint8_t *velocity,uint32_t gain[16][2])
{
    unsigned ch;
    for(ch=0;ch<p->channels.count;++ch) {
        unsigned pan=p->channels.track[ch].pan;uint32_t base;
        gain[ch][0]=gain[ch][1]=0;
        if(!(o->tracks&(1U<<ch)) || !pt_channel_audible(&p->channels,ch,PT_PAULA|PT_AMIGUS))continue;
        base=(uint32_t)((uint64_t)volume[ch]*1024*velocity[ch]*o->gain_q16/(127ULL*65536));
        if(voices[ch].pcm && voices[ch].pcm->channels==2) {
            gain[ch][0]=pan<=128?base:(uint32_t)((uint64_t)base*(255-pan)/127);
            gain[ch][1]=pan>=128?base:(uint32_t)((uint64_t)base*pan/128);
        } else {
            gain[ch][0]=(uint32_t)((uint64_t)base*(255-pan)/255);
            gain[ch][1]=(uint32_t)((uint64_t)base*pan/255);
        }
    }
}
static enum pt_render_result commands(const struct pt_project *p,const struct pt_render_options *o,
                                      const struct pt_flow *flow,const struct pt_pitch *pitch,const struct sample_range *ranges,uint16_t offsets,struct pt_voice *voice,
                                      uint8_t *instrument,uint8_t *volume,uint8_t *velocity)
{
    unsigned ch;
    for(ch=0;ch<p->channels.count;++ch)if(o->tracks&(1U<<ch)) {
        if(flow->fresh) {
            const struct pt_event *e=flow->project->events+((size_t)flow->project->orders[flow->played_order]*64+flow->played_row)*p->channels.count+ch;
            if(e->instrument) {instrument[ch]=e->instrument;volume[ch]=p->samples[e->instrument-1].volume;}
            if(e->kind==PT_NOTE_OFF)voice[ch].active=0;
            else if(e->kind==PT_NOTE_PERIOD && instrument[ch] && e->effect!=3 && e->effect!=5) {
                const struct pt_sample *s=p->samples+instrument[ch]-1;uint32_t a=0,b=s->pcm.frames,la=0,lb=0;
                enum pt_voice_loop loop=PT_VOICE_ONCE;
                if(e->slice) {a=s->slices[e->slice-1];if(e->slice<s->slice_count)b=s->slices[e->slice];}
                if(s->loop && s->loop_start>=a && s->loop_end<=b) {
                    la=s->loop_start;lb=s->loop_end;loop=s->loop==PT_LOOP_PINGPONG?PT_VOICE_PINGPONG:PT_VOICE_FORWARD;
                }
                if(offsets&(1U<<ch)) {
                    a=ranges[ch].trigger_start;b=a+ranges[ch].trigger_length;
                    if(s->loop) {
                        if(pt_voice_init_segment(voice+ch,&s->pcm,a,b,s->loop_start,s->loop_end,step(s,e->pitch,o->rate),s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
                    } else if(pt_voice_init(voice+ch,&s->pcm,a,b,PT_VOICE_ONCE,0,0,step(s,e->pitch,o->rate),s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
                } else if(pt_voice_init(voice+ch,&s->pcm,a,b,loop,la,lb,step(s,e->pitch,o->rate),s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
                velocity[ch]=(e->flags&1)?e->velocity:127;
            }
            if(e->kind==PT_NOTE_PERIOD && (e->effect==3 || e->effect==5) && (e->flags&1))velocity[ch]=e->velocity;
            if(e->effect==12)volume[ch]=e->parameter>64?64:e->parameter;
        } else if(flow->effect[ch]==10 || flow->effect[ch]==5 || flow->effect[ch]==6) {
            unsigned param=flow->parameter[ch],up=param>>4,down=param&15;
            if(up)volume[ch]=(uint8_t)(volume[ch]+up>64?64:volume[ch]+up);
            else volume[ch]=(uint8_t)(volume[ch]<down?0:volume[ch]-down);
        }
        if(ranges[ch].retrigger && instrument[ch]) {
            const struct pt_sample *s=p->samples+instrument[ch]-1;
            uint32_t a=ranges[ch].trigger_start,b=a+ranges[ch].trigger_length;
            uint64_t rate=step(s,pitch->channel[ch].output,o->rate);
            if(s->loop) {
                if(pt_voice_init_segment(voice+ch,&s->pcm,a,b,s->loop_start,s->loop_end,rate,s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
            } else if(pt_voice_init(voice+ch,&s->pcm,a,b,PT_VOICE_ONCE,0,0,rate,s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
        }
        if(voice[ch].pcm && pitch->channel[ch].output)
            voice[ch].step=step(p->samples+instrument[ch]-1,pitch->channel[ch].output,o->rate);
        /* Extended volume commands also run on delayed tick zero, without
           fetching the instrument or restarting the sample. ECx changes only
           volume; a later Cxx must reveal the continuing voice phase. */
        if(flow->effect[ch]==14) {
            unsigned command=flow->parameter[ch]>>4,amount=flow->parameter[ch]&15;
            if(command==10 && !flow->counter)volume[ch]=(uint8_t)(volume[ch]+amount>64?64:volume[ch]+amount);
            else if(command==11 && !flow->counter)volume[ch]=(uint8_t)(volume[ch]<amount?0:volume[ch]-amount);
            else if(command==12 && flow->counter==amount)volume[ch]=0;
        }
    }
    return PT_RENDER_OK;
}
enum pt_render_result pt_render_stream(const struct pt_project *p,const struct pt_render_options *o,
                                      pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *progress_ctx,
                                      struct pt_render_report *out)
{
    struct pt_render_report planned;struct run r;struct pt_tick_span span;
    struct pt_voice voice[16];uint32_t gain[16][2];uint8_t instrument[16]={0},volume[16]={0},velocity[16];
    int32_t samples[512];struct pt_pcm block;uint64_t offset=0,clips=0;unsigned end;enum pt_render_result result;
    if(!sink || !out)return PT_RENDER_INVALID;
    result=pt_render_measure(p,o,progress,progress_ctx,&planned);if(result!=PT_RENDER_OK)return result;
    if(!start_run(&r,p,o))return PT_RENDER_INVALID;
    memset(voice,0,sizeof(voice));memset(velocity,127,sizeof(velocity));memset(&block,0,sizeof(block));
    block.data=samples;block.capacity=512;block.channels=2;block.bits=o->bits;block.rate=o->rate;
    do {
        uint32_t remaining;result=next_tick(&r,&span,&end);if(result!=PT_RENDER_OK)return result;remaining=span.frames;
        gains_for(p,o,voice,volume,velocity,gain);
        while(remaining) {
            uint64_t clipped;
            if(progress && !progress(progress_ctx,PT_RENDER_MIX,r.timeline.flow.ticks,offset))return PT_RENDER_CANCELLED;
            block.frames=remaining>256?256:remaining;
            if(pt_voice_mix(voice,p->channels.count,gain,&block,&clipped)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
            if(!sink(sink_ctx,&block,offset))return PT_RENDER_SINK;
            clips+=clipped;offset+=block.frames;remaining-=block.frames;
        }
        if(!end) {result=commands(p,o,&r.timeline.flow,&r.pitch,r.range,r.offset_tracks,voice,instrument,volume,velocity);if(result!=PT_RENDER_OK)return result;}
    } while(!end);
    if(offset!=planned.frames || r.timeline.flow.ticks!=planned.ticks)return PT_RENDER_INVALID;
    report_run(&r,end,clips,out);return PT_RENDER_OK;
}
