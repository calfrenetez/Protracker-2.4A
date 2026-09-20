#include "pitch.h"
#include <string.h>
void pt_pitch_init(struct pt_pitch *s)
{
    unsigned ch;memset(s,0,sizeof(*s));
    for(ch=0;ch<PT_CHANNEL_LIMIT;++ch)s->channel[ch].empty=1;
}
static void slide(struct pt_pitch_channel *s,unsigned amount,unsigned down)
{
    uint16_t word=(uint16_t)(down?s->period+amount:s->period-amount);
    unsigned low=word&4095;
    if(down?low>=856:low<113)word=(uint16_t)((word&0xf000)|(down?856:113));
    s->period=word;s->output=word&4095;
}
/* mt_ftune0, including the native zero sentinel for notes below period113. */
static uint16_t tone_target(unsigned raw)
{
    static const uint16_t periods[]={856,808,762,720,678,640,604,570,538,508,480,453,
        428,404,381,360,339,320,302,285,269,254,240,226,214,202,190,180,170,160,151,143,135,127,120,113,0};
    unsigned i=0;while(raw<periods[i])++i;return periods[i];
}
static int signed_word(unsigned n) {return n>=32768?(int)n-65536:(int)n;}
static void tone(struct pt_pitch_channel *s)
{
    if(!s->target)return;
    s->period=(uint16_t)(s->up?s->period-s->speed:s->period+s->speed);
    if(s->up?signed_word(s->target)>=signed_word(s->period):signed_word(s->target)<=signed_word(s->period)) {
        s->period=s->target;s->target=0;
    }
    s->output=s->period;
}
void pt_pitch_tick(struct pt_pitch *s,const struct pt_flow *flow,uint16_t tracks)
{
    unsigned ch;
    for(ch=0;ch<flow->project->channels.count;++ch)if(tracks&(1U<<ch)) {
        struct pt_pitch_channel *v=s->channel+ch;
        unsigned effect=flow->effect[ch],param=flow->parameter[ch];
        if(flow->fresh) {
            const struct pt_event *e=flow->project->events+
                ((size_t)flow->project->orders[flow->played_order]*64+flow->played_row)*flow->project->channels.count+ch;
            /* mt_PlayVoice tests the previous packed event before replacing it. */
            if(v->empty)v->output=v->period;
            if(e->instrument) {
                if((effect==3 || effect==5) && v->sounding && v->instrument!=e->instrument)v->unsupported=1;
                v->instrument=e->instrument;
            }
            if(e->kind==PT_NOTE_OFF)v->sounding=0;
            else if(e->kind==PT_NOTE_PERIOD) {
                if(effect==3 || effect==5) {
                    v->target=tone_target(e->pitch);v->up=signed_word(v->target)<signed_word(v->period);
                    if(v->target==v->period)v->target=0;
                } else {v->period=v->output=e->pitch;v->sounding=v->instrument!=0;}
            }
            v->empty=e->kind==PT_NOTE_NONE && !e->instrument && !effect && !param;
            if(effect<=3 || effect==5 || effect==10)v->output=v->period; /* mt_PerNop */
        } else {
            if(effect==1 || effect==2)slide(v,param,effect==2);
            else if(effect==3 || effect==5) {
                if(effect==3 && param)v->speed=(uint8_t)param;
                tone(v);
            }
            else if((effect>=10 && effect<=13) || effect==15)v->output=v->period; /* SetBack */
        }
        if(effect==14 && !flow->counter && ((param>>4)==1 || (param>>4)==2))slide(v,param&15,(param>>4)==2);
    }
}
