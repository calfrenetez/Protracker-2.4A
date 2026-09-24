#include "render.h"
#include "render_commands.h"
#include "pitch.h"
#include "document.h"
#include <string.h>
struct run {
    struct pt_project view;
    uint16_t order;
    struct pt_timeline timeline;
    struct pt_pitch pitch;
    uint16_t tracks,offset_tracks,sliced_tracks;
    struct pt_render_range range[16];
    uint64_t frames;
    uint8_t started,pending_end,capturing,emit,row_range,row_first,row_end;
};
static uint16_t offset_tracks(const struct pt_project *p,const struct pt_render_options *o)
{
    uint8_t used[256]={0};unsigned pat,row,ch;uint16_t mask=0;
    if(o->pattern_only)used[o->pattern]=1;
    else for(pat=0;pat<p->order_count;++pat)used[p->orders[pat]]=1;
    for(pat=0;pat<p->pattern_count;++pat)if(used[pat])for(row=0;row<64;++row)for(ch=0;ch<p->channels.count;++ch)
        {
            const struct pt_event *e=p->events+((size_t)pat*64+row)*p->channels.count+ch;
            if(e->effect==9 || (e->effect==14 && ((e->parameter>>4)==9 || ((e->parameter>>4)==13 && e->kind==PT_NOTE_PERIOD))))mask|=(uint16_t)(1U<<ch);
        }
    return mask&o->tracks;
}
static void apply_offset(struct pt_render_range *v,unsigned parameter)
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
        struct pt_render_range *v=r->range+ch;
        v->retrigger=0;
        if(f->fresh) {
            if(e->instrument) {
                const struct pt_sample *s=r->view.samples+e->instrument-1;
                v->start=0;v->length=s->loop && s->loop_start?s->loop_end:s->pcm.frames;v->loaded=1;
            }
            if(e->kind==PT_NOTE_PERIOD && e->effect!=3 && e->effect!=5 && !(e->effect==14 && (e->parameter>>4)==13)) {
                if(e->effect==9)apply_offset(v,e->parameter);
                v->trigger_start=v->start;v->trigger_length=v->length;
                v->trigger_frames=v->loaded?r->view.samples[r->pitch.channel[ch].instrument-1].pcm.frames:0;
            }
            if(e->effect==9)apply_offset(v,e->parameter);
        }
        if(v->loaded && f->effect[ch]==14 &&
           (((f->parameter[ch]>>4)==9 && (f->parameter[ch]&15) &&
             !(!f->counter && e->kind==PT_NOTE_PERIOD) && !(f->counter%(f->parameter[ch]&15))) ||
            ((f->parameter[ch]>>4)==13 && e->kind==PT_NOTE_PERIOD && f->counter==(f->parameter[ch]&15)))) {
            v->retrigger=1;v->trigger_start=v->start;v->trigger_length=v->length;
            v->trigger_frames=v->loaded?r->view.samples[r->pitch.channel[ch].instrument-1].pcm.frames:0;
        }
        if(v->loaded) {
            uint32_t frames=r->view.samples[r->pitch.channel[ch].instrument-1].pcm.frames;
            if(v->start>frames || v->length>frames-v->start || v->trigger_start>v->trigger_frames || v->trigger_length>v->trigger_frames-v->trigger_start)return 0;
        }
    }
    return 1;
}
static int handoff_sample(const struct pt_sample *s)
{
    return s->pcm.bits==8 && s->pcm.channels==1 && s->pcm.frames>=2 &&
        s->pcm.frames<=131070 && !(s->pcm.frames&1) && s->loop==PT_LOOP_FORWARD &&
        s->loop_end-s->loop_start>=4 && !((s->loop_start|s->loop_end)&1) && !s->interpolation;
}
static int handoff_effect(const struct pt_event *e)
{
    unsigned sub=e->parameter>>4;
    if(e->effect==14)return sub>=1 && sub<=14;
    return e->effect<=15;
}
static int silent_handoff_sample(const struct pt_sample *s)
{
    return s->pcm.bits==8 && s->pcm.channels==1 && s->pcm.frames>=2 &&
        s->pcm.frames<=131070 && !(s->pcm.frames&1) && s->loop==PT_LOOP_NONE &&
        !s->interpolation && s->pcm.data[0]==0 && s->pcm.data[1]==0;
}
static enum pt_render_result preflight(const struct pt_project *p,const struct pt_render_options *o,unsigned invert)
{
    unsigned ch,pat,row;uint8_t used[256]={0};uint16_t offsets;
    if(!p || !o || pt_project_validate(p,NULL)!=PT_PROJECT_OK || !o->tick_limit || !o->frame_limit ||
       (o->rate!=44100 && o->rate!=48000) || (o->bits!=16 && o->bits!=24) ||
       !o->tracks || (o->tracks>>p->channels.count) || o->gain_q16>65536 ||
       o->pattern_only>1 || o->include_lead_in>1 || o->row_range>1 ||
       (o->row_range && (!o->pattern_only || o->include_lead_in || o->row_first>=o->row_end || o->row_end>64)) ||
       (o->pattern_only?o->pattern>=p->pattern_count:o->start_order>=p->order_count))return PT_RENDER_INVALID;
    offsets=offset_tracks(p,o);
    for(ch=0;ch<p->channels.count;++ch)if((o->tracks&(1U<<ch)) && p->channels.track[ch].route==PT_MIDI)return PT_RENDER_ROUTE;
    if(o->pattern_only)used[o->pattern]=1;
    else for(pat=0;pat<p->order_count;++pat)used[p->orders[pat]]=1;
    for(pat=0;pat<p->pattern_count;++pat)if(used[pat])for(row=0;row<64;++row)for(ch=0;ch<p->channels.count;++ch) {
        const struct pt_event *e=p->events+((size_t)pat*64+row)*p->channels.count+ch;
        if(!(o->tracks&(1U<<ch)))continue;
        if(e->kind==PT_NOTE_MIDI || (e->instrument && e->kind!=PT_NOTE_PERIOD && e->kind!=PT_NOTE_NONE) ||
           (e->kind==PT_NOTE_NONE && e->slice))return PT_RENDER_EFFECT;
        if(!(e->effect==0 || e->effect==1 || e->effect==2 || e->effect==3 || e->effect==4 || e->effect==5 || e->effect==6 || e->effect==7 || e->effect==8 || e->effect==9 || (e->effect>=10 && e->effect<=13) || e->effect==15 ||
             (e->effect==14 && ((e->parameter>>4)==1 || (e->parameter>>4)==2 || (e->parameter>>4)==3 || (e->parameter>>4)==4 || (e->parameter>>4)==5 || (e->parameter>>4)==6 || (e->parameter>>4)==7 || (e->parameter>>4)==8 || (e->parameter>>4)==9 || ((e->parameter>>4)>=10 && (e->parameter>>4)<=13) ||
                               (e->parameter>>4)==14 || (invert && (e->parameter>>4)==15)))))return PT_RENDER_EFFECT;
        if((e->effect==3 || e->effect==5) && e->slice)return PT_RENDER_EFFECT;
        if((offsets&(1U<<ch)) && e->slice)return PT_RENDER_EFFECT;
        if(e->instrument) {
            const struct pt_sample *s=p->samples+e->instrument-1;
            if((offsets&(1U<<ch)) && (s->pcm.bits!=8 || s->pcm.channels!=1 || s->pcm.frames<2 ||
               s->pcm.frames>131070 || (s->pcm.frames&1) || s->loop>PT_LOOP_FORWARD ||
               (s->loop && ((s->loop_start|s->loop_end)&1))))return PT_RENDER_SAMPLE;
            if(s->loop==PT_LOOP_CROSSFADE)return PT_RENDER_SAMPLE;
        }
    }
    return PT_RENDER_OK;
}
static int start_run(struct run *r,const struct pt_project *p,const struct pt_render_options *o)
{
    memset(r,0,sizeof(*r));r->view=*p;r->started=o->include_lead_in;
    r->capturing=!o->row_range;r->row_range=o->row_range;r->row_first=o->row_first;r->row_end=o->row_end;
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
    r->emit=r->capturing;
    if(r->emit)r->frames+=span->frames;
    if(!r->timeline.flow.active)*end=1; /* F00. */
    else if(r->pending_end && r->timeline.flow.fresh)*end=2;
    if(r->timeline.flow.returns!=returned)r->pending_end=1;
    if(!*end && r->row_range && r->timeline.flow.fresh) {
        unsigned row=r->timeline.flow.played_row;
        if(row>=r->row_first && row<r->row_end)r->capturing=1;
        else if(r->capturing)*end=3;
    }
    if(*end && r->row_range && !r->capturing)return PT_RENDER_EMPTY_RANGE;
    if(!*end) {
        unsigned ch;
        if(r->timeline.flow.fresh)for(ch=0;ch<r->view.channels.count;++ch)if(r->tracks&(1U<<ch)) {
            const struct pt_flow *f=&r->timeline.flow;
            const struct pt_event *e=r->view.events+((size_t)r->view.orders[f->played_order]*64+f->played_row)*r->view.channels.count+ch;
            const struct pt_pitch_channel *v=r->pitch.channel+ch;
            /* A bounded classic looped handoff changes the next repeat source,
               not the current iteration. Unsupported combinations fail here
               during measurement, before any output is published. */
            if((e->kind==PT_NOTE_NONE || (e->kind==PT_NOTE_PERIOD && (e->effect==3 || e->effect==5 || (e->effect==14 && (e->parameter>>4)==13)))) && e->instrument && v->sounding) {
                if(r->sliced_tracks&(1U<<ch))return PT_RENDER_EFFECT;
                if(e->instrument!=v->instrument) {
                    const struct pt_sample *a=r->view.samples+v->instrument-1,*b=r->view.samples+e->instrument-1;
                    if(!handoff_effect(e) || (!handoff_sample(a) && !silent_handoff_sample(a)) || (!handoff_sample(b) && !silent_handoff_sample(b)) ||
                       a->pcm.rate!=b->pcm.rate)return PT_RENDER_EFFECT;
                }
            }
            if(e->kind==PT_NOTE_OFF)r->sliced_tracks&=(uint16_t)~(1U<<ch);
            else if(e->kind==PT_NOTE_PERIOD && e->effect!=3 && e->effect!=5 &&
                    !(e->effect==14 && (e->parameter>>4)==13)) {
                if(e->slice)r->sliced_tracks|=(uint16_t)(1U<<ch);
                else r->sliced_tracks&=(uint16_t)~(1U<<ch);
            }
        }
        pt_pitch_tick(&r->pitch,&r->timeline.flow,r->tracks);
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
    result.ticks=r->timeline.flow.ticks;result.clipped=clips;result.end=end==1?PT_RENDER_F00:end==3?PT_RENDER_ROW_EXIT:PT_RENDER_POSITION_RETURN;*out=result;
}
static enum pt_render_result measure(const struct pt_project *p,const struct pt_render_options *o,
                                       pt_render_progress progress,void *ctx,struct pt_render_report *out,struct run *r,unsigned invert)
{
    struct pt_tick_span span;unsigned end;enum pt_render_result result=preflight(p,o,invert);
    if(!out)return PT_RENDER_INVALID;
    if(result!=PT_RENDER_OK)return result;
    if(!start_run(r,p,o))return PT_RENDER_INVALID;
    do {
        if(progress && !progress(ctx,PT_RENDER_ANALYSE,r->timeline.flow.ticks,r->frames))return PT_RENDER_CANCELLED;
        result=next_tick(r,&span,&end);if(result!=PT_RENDER_OK)return result;
    } while(!end);
    report_run(r,end,0,out);return PT_RENDER_OK;
}
enum pt_render_result pt_render_measure(const struct pt_project *p,const struct pt_render_options *o,
    pt_render_progress progress,void *ctx,struct pt_render_report *out)
{
    struct run r;return measure(p,o,progress,ctx,out,&r,0);
}
enum pt_render_result pt_render_measure_allocated(const struct pt_project *p,const struct pt_render_options *o,
    pt_render_progress progress,void *ctx,struct pt_render_report *out,const struct pt_allocator *a)
{
    struct run *r;enum pt_render_result result;
    if(!a || !a->allocate || !a->release || !out)return PT_RENDER_INVALID;
    r=a->allocate(a->context,sizeof(*r));if(!r)return PT_RENDER_MEMORY;
    result=measure(p,o,progress,ctx,out,r,0);a->release(a->context,r);return result;
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
static uint8_t tremolo_volume(struct pt_render_tremolo *t,unsigned volume,unsigned parameter,unsigned vib_phase)
{
    static const uint8_t sine[]={0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,
        255,253,250,244,235,224,212,197,180,161,141,120,97,74,49,24};
    unsigned index=(t->phase>>2)&31,wave=t->control&3,amount;int output;
    if(parameter&15)t->command=(uint8_t)((t->command&240)|(parameter&15));
    if(parameter&240)t->command=(uint8_t)((t->command&15)|(parameter&240));
    if(!wave)amount=sine[index];
    else if(wave==1)amount=(vib_phase&128)?255-index*8:index*8;
    else amount=255;
    amount=amount*(t->command&15)>>6;
    output=(int)volume+((t->phase&128)?-(int)amount:(int)amount);
    t->phase=(uint8_t)(t->phase+((t->command>>4)*4));
    return (uint8_t)(output<0?0:output>64?64:output);
}
static int action(struct pt_render_plan *plan,unsigned ch,enum pt_render_action_kind kind,const struct pt_voice *voice)
{
    struct pt_render_action *a;if(!plan)return 1;
    if(plan->count>=PT_RENDER_ACTIONS)return 0;
    a=&plan->action[plan->count++];memset(a,0,sizeof(*a));a->kind=kind;a->channel=ch;a->voice=*voice;return 1;
}
static enum pt_pcm_result command_trigger(struct pt_render_plan *plan,unsigned ch,struct pt_voice *v,const struct pt_pcm *p,
    uint32_t start,uint32_t end,enum pt_voice_loop loop,uint32_t a,uint32_t b,uint64_t step,unsigned linear)
{
    enum pt_pcm_result result=pt_voice_init(v,p,start,end,loop,a,b,step,linear);
    if(result==PT_PCM_OK && !action(plan,ch,PT_RENDER_TRIGGER,v))return PT_PCM_CAPACITY;
    return result;
}
static enum pt_pcm_result command_segment(struct pt_render_plan *plan,unsigned ch,struct pt_voice *v,const struct pt_pcm *p,
    uint32_t start,uint32_t end,uint32_t a,uint32_t b,uint64_t step,unsigned linear)
{
    enum pt_pcm_result result=pt_voice_init_segment(v,p,start,end,a,b,step,linear);
    if(result==PT_PCM_OK && !action(plan,ch,PT_RENDER_SEGMENT,v))return PT_PCM_CAPACITY;
    return result;
}
static enum pt_pcm_result command_repeat(struct pt_render_plan *plan,unsigned ch,struct pt_voice *v,const struct pt_pcm *p,uint32_t a,uint32_t b)
{
    enum pt_pcm_result result=pt_voice_set_repeat_source(v,p,a,b);
    if(result==PT_PCM_OK && !action(plan,ch,PT_RENDER_REPEAT,v))return PT_PCM_CAPACITY;
    return result;
}
static enum pt_render_result commands(const struct pt_project *p,const struct pt_render_options *o,
                                      const struct pt_flow *flow,const struct pt_pitch *pitch,const struct pt_render_range *ranges,uint16_t offsets,struct pt_voice *voice,
                                      uint8_t *instrument,uint8_t *volume,uint8_t *velocity,uint8_t *output_volume,struct pt_render_tremolo *trem,struct pt_render_plan *plan)
{
    unsigned ch;
    for(ch=0;ch<p->channels.count;++ch)if(o->tracks&(1U<<ch)) {
        if(flow->fresh) {
            const struct pt_event *e=flow->project->events+((size_t)flow->project->orders[flow->played_order]*64+flow->played_row)*p->channels.count+ch;
            if((e->kind==PT_NOTE_NONE || (e->kind==PT_NOTE_PERIOD && (e->effect==3 || e->effect==5 || (e->effect==14 && (e->parameter>>4)==13)))) && e->instrument && e->instrument!=instrument[ch] && voice[ch].active) {
                const struct pt_sample *next=p->samples+e->instrument-1;
                if(command_repeat(plan,ch,voice+ch,&next->pcm,next->loop?next->loop_start:0,next->loop?next->loop_end:2)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
            }
            if(e->instrument) {instrument[ch]=e->instrument;volume[ch]=p->samples[e->instrument-1].volume;}
            if(e->kind==PT_NOTE_PERIOD && e->effect!=3 && e->effect!=5 &&
               !(e->effect==14 && (e->parameter>>4)==13) && !(trem[ch].control&4))trem[ch].phase=0;
            if(e->kind==PT_NOTE_OFF) {voice[ch].active=0;if(!action(plan,ch,PT_RENDER_STOP,voice+ch))return PT_RENDER_INVALID;}
            else if(e->kind==PT_NOTE_PERIOD && instrument[ch] && e->effect!=3 && e->effect!=5 && !(e->effect==14 && (e->parameter>>4)==13)) {
                const struct pt_sample *s=p->samples+instrument[ch]-1;uint32_t a=0,b=s->pcm.frames,la=0,lb=0;
                enum pt_voice_loop loop=PT_VOICE_ONCE;
                if(e->slice) {a=s->slices[e->slice-1];if(e->slice<s->slice_count)b=s->slices[e->slice];}
                if(s->loop && s->loop_start>=a && s->loop_end<=b) {
                    la=s->loop_start;lb=s->loop_end;loop=s->loop==PT_LOOP_PINGPONG?PT_VOICE_PINGPONG:PT_VOICE_FORWARD;
                }
                if(offsets&(1U<<ch)) {
                    a=ranges[ch].trigger_start;b=a+ranges[ch].trigger_length;
                    if(s->loop) {
                        if(command_segment(plan,ch,voice+ch,&s->pcm,a,b,s->loop_start,s->loop_end,step(s,e->pitch,o->rate),s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
                    } else if(silent_handoff_sample(s)) {
                        if(command_segment(plan,ch,voice+ch,&s->pcm,a,b,0,2,step(s,e->pitch,o->rate),0)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
                    } else if(command_trigger(plan,ch,voice+ch,&s->pcm,a,b,PT_VOICE_ONCE,0,0,step(s,e->pitch,o->rate),s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
                } else if(!e->slice && silent_handoff_sample(s)) {
                    if(command_segment(plan,ch,voice+ch,&s->pcm,a,b,0,2,step(s,e->pitch,o->rate),0)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
                } else if(command_trigger(plan,ch,voice+ch,&s->pcm,a,b,loop,la,lb,step(s,e->pitch,o->rate),s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
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
            if(flow->effect[ch]==14 && (flow->parameter[ch]>>4)==13) {
                const struct pt_event *e=flow->project->events+((size_t)flow->project->orders[flow->played_order]*64+flow->played_row)*p->channels.count+ch;
                velocity[ch]=(e->flags&1)?e->velocity:127;
            }
            if(s->loop) {
                if(command_segment(plan,ch,voice+ch,&s->pcm,a,b,s->loop_start,s->loop_end,rate,s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
            } else if(silent_handoff_sample(s)) {
                if(command_segment(plan,ch,voice+ch,&s->pcm,a,b,0,2,rate,0)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
            } else if(command_trigger(plan,ch,voice+ch,&s->pcm,a,b,PT_VOICE_ONCE,0,0,rate,s->interpolation)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
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
        if(flow->effect[ch]==14 && (flow->parameter[ch]>>4)==7)trem[ch].control=flow->parameter[ch]&15;
        output_volume[ch]=(!flow->fresh && flow->effect[ch]==7)?
            tremolo_volume(trem+ch,volume[ch],flow->parameter[ch],pitch->channel[ch].vib_phase):volume[ch];
    }
    return PT_RENDER_OK;
}
void pt_render_commands_init(struct pt_render_command_state *state)
{
    memset(state,0,sizeof(*state));memset(state->velocity,127,sizeof(state->velocity));
}
void pt_render_commands_gains(const struct pt_project *p,const struct pt_render_options *o,
    struct pt_render_command_state *state)
{gains_for(p,o,state->voice,state->output_volume,state->velocity,state->gain);}
enum pt_render_result pt_render_commands_tick(const struct pt_project *p,const struct pt_render_options *o,
    const struct pt_flow *flow,const struct pt_pitch *pitch,const struct pt_render_range *ranges,uint16_t offsets,
    struct pt_render_command_state *state)
{
    return commands(p,o,flow,pitch,ranges,offsets,state->voice,state->instrument,
        state->volume,state->velocity,state->output_volume,state->trem,NULL);
}
enum pt_render_result pt_render_commands_plan(const struct pt_project *p,const struct pt_render_options *o,
    const struct pt_flow *flow,const struct pt_pitch *pitch,const struct pt_render_range *ranges,uint16_t offsets,
    struct pt_render_command_state *state,struct pt_render_plan *plan)
{
    enum pt_render_result result;unsigned ch;
    if(!plan)return PT_RENDER_INVALID;
    plan->count=0;
    result=commands(p,o,flow,pitch,ranges,offsets,state->voice,state->instrument,
        state->volume,state->velocity,state->output_volume,state->trem,plan);
    if(result!=PT_RENDER_OK) {plan->count=0;return result;}
    pt_render_commands_gains(p,o,state);
    for(ch=0;ch<p->channels.count;++ch)if((o->tracks&(1U<<ch)) && state->voice[ch].active) {
        struct pt_render_action *a;
        if(!action(plan,ch,PT_RENDER_CONTROL,&state->voice[ch])) {plan->count=0;return PT_RENDER_INVALID;}
        a=&plan->action[plan->count-1];a->gain[0]=state->gain[ch][0];a->gain[1]=state->gain[ch][1];
    }
    return PT_RENDER_OK;
}
struct workspace {
    struct run run;struct pt_render_command_state commands;int32_t samples[512];
};
static enum pt_render_result stream(const struct pt_project *p,const struct pt_render_options *o,
                                      pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *progress_ctx,
                                      struct pt_render_report *out,struct workspace *w,const struct pt_render_mutation *mutation)
{
    struct pt_render_report planned;struct run *r=&w->run;struct pt_tick_span span;
    struct pt_voice *voice=w->commands.voice;uint32_t (*gain)[2]=w->commands.gain;
    struct pt_pcm block;uint64_t offset=0,clips=0;unsigned end;enum pt_render_result result;
    if(!sink || !out)return PT_RENDER_INVALID;
    result=measure(p,o,progress,progress_ctx,&planned,r,mutation!=NULL);if(result!=PT_RENDER_OK)return result;
    memset(w,0,sizeof(*w));
    if(!start_run(r,p,o))return PT_RENDER_INVALID;
    pt_render_commands_init(&w->commands);memset(&block,0,sizeof(block));
    block.data=w->samples;block.capacity=512;block.channels=2;block.bits=o->bits;block.rate=o->rate;
    do {
        uint32_t remaining;result=next_tick(r,&span,&end);if(result!=PT_RENDER_OK)return result;remaining=span.frames;
        pt_render_commands_gains(p,o,&w->commands);
        while(remaining) {
            uint64_t clipped;
            if(progress && !progress(progress_ctx,PT_RENDER_MIX,r->timeline.flow.ticks,offset))return PT_RENDER_CANCELLED;
            block.frames=remaining>256?256:remaining;
            if(pt_voice_mix(voice,p->channels.count,gain,&block,&clipped)!=PT_PCM_OK)return PT_RENDER_SAMPLE;
            if(r->emit) {
                if(!sink(sink_ctx,&block,offset))return PT_RENDER_SINK;
                clips+=clipped;offset+=block.frames;
            }
            remaining-=block.frames;
        }
        if(!end) {
            if(mutation && !mutation->tick(mutation->context,&r->timeline.flow))return PT_RENDER_SAMPLE;
            result=pt_render_commands_tick(mutation?mutation->playback:p,o,&r->timeline.flow,&r->pitch,r->range,r->offset_tracks,&w->commands);if(result!=PT_RENDER_OK)return result;}
    } while(!end);
    if(offset!=planned.frames || r->timeline.flow.ticks!=planned.ticks)return PT_RENDER_INVALID;
    report_run(r,end,clips,out);return PT_RENDER_OK;
}

enum pt_render_result pt_render_stream(const struct pt_project *p,const struct pt_render_options *o,
    pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *progress_ctx,struct pt_render_report *out)
{
    struct workspace w;return stream(p,o,sink,sink_ctx,progress,progress_ctx,out,&w,NULL);
}
enum pt_render_result pt_render_stream_allocated(const struct pt_project *p,const struct pt_render_options *o,
    pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *progress_ctx,
    struct pt_render_report *out,const struct pt_allocator *a)
{
    struct workspace *w;enum pt_render_result result;
    if(!a || !a->allocate || !a->release || !sink || !out)return PT_RENDER_INVALID;
    w=a->allocate(a->context,sizeof(*w));if(!w)return PT_RENDER_MEMORY;
    result=stream(p,o,sink,sink_ctx,progress,progress_ctx,out,w,NULL);a->release(a->context,w);return result;
}


enum pt_render_result pt_render_mutating_allocated(const struct pt_project *p,const struct pt_render_options *o,
    pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *progress_ctx,
    struct pt_render_report *out,const struct pt_allocator *a,const struct pt_render_mutation *mutation)
{
    struct workspace *w;enum pt_render_result result;
    if(!a || !a->allocate || !a->release || !sink || !out || !mutation ||
       !mutation->playback || !mutation->tick)return PT_RENDER_INVALID;
    w=a->allocate(a->context,sizeof(*w));if(!w)return PT_RENDER_MEMORY;
    result=stream(p,o,sink,sink_ctx,progress,progress_ctx,out,w,mutation);
    a->release(a->context,w);return result;
}

struct pt_render_sequence {
    struct pt_allocator allocator;struct pt_render_options options;
    const struct pt_project *project;struct run run;
    struct pt_render_command_state commands;
    uint32_t remaining;unsigned pending,end,done,failed;
};
enum pt_render_result pt_render_sequence_open(const struct pt_project *p,const struct pt_render_options *o,
    const struct pt_allocator *a,struct pt_render_sequence **out)
{
    struct pt_render_sequence *s;struct pt_render_report report;enum pt_render_result result;
    if(!a || !a->allocate || !a->release || !out)return PT_RENDER_INVALID;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_RENDER_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;
    result=measure(p,o,NULL,NULL,&report,&s->run,0);
    if(result!=PT_RENDER_OK) {a->release(a->context,s);return result;}
    s->options=*o;s->project=p;
    if(!start_run(&s->run,p,&s->options)) {a->release(a->context,s);return PT_RENDER_INVALID;}
    pt_render_commands_init(&s->commands);*out=s;return PT_RENDER_OK;
}
enum pt_render_result pt_render_sequence_next(struct pt_render_sequence *s,struct pt_render_interval *out)
{
    struct pt_tick_span span;enum pt_render_result result;
    if(!s || !out || s->pending || s->done || s->failed)return PT_RENDER_INVALID;
    result=next_tick(&s->run,&span,&s->end);
    if(result!=PT_RENDER_OK) {s->failed=1;return result;}
    s->remaining=span.frames;s->pending=1;
    out->frames=span.frames;out->emit=s->run.emit;out->end=s->end;return PT_RENDER_OK;
}
enum pt_render_result pt_render_sequence_consume(struct pt_render_sequence *s,uint32_t frames)
{
    if(!s || !s->pending || s->failed || !frames || frames>256 || frames>s->remaining)return PT_RENDER_INVALID;
    if(pt_voice_advance(s->commands.voice,s->project->channels.count,frames)!=PT_PCM_OK) {s->failed=1;return PT_RENDER_SAMPLE;}
    s->remaining-=frames;return PT_RENDER_OK;
}
enum pt_render_result pt_render_sequence_complete(struct pt_render_sequence *s,struct pt_render_plan *plan)
{
    enum pt_render_result result;
    if(!plan)return PT_RENDER_INVALID;
    plan->count=0;
    if(!s || !s->pending || s->remaining || s->failed)return PT_RENDER_INVALID;
    if(s->end) {s->done=1;s->pending=0;return PT_RENDER_OK;}
    result=pt_render_commands_plan(s->project,&s->options,&s->run.timeline.flow,&s->run.pitch,
        s->run.range,s->run.offset_tracks,&s->commands,plan);
    if(result!=PT_RENDER_OK) {s->failed=1;return result;}
    s->pending=0;return PT_RENDER_OK;
}
void pt_render_sequence_close(struct pt_render_sequence *s)
{if(s) {struct pt_allocator a=s->allocator;a.release(a.context,s);}}
