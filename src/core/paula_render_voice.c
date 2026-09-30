#include "paula_render_voice.h"
int pt_paula_render_caps_valid(const struct pt_paula_render_caps *c)
{return c && c->clock_hz && c->minimum_period && c->maximum_period>=c->minimum_period;}
int pt_paula_render_control(uint64_t step,unsigned rate,const uint32_t gains[2],unsigned slot,
    const struct pt_paula_render_caps *c,uint16_t *period,uint8_t *volume)
{
    uint64_t numerator,denominator,value,remainder;unsigned side;
    if(!period || !volume || !gains || slot>=4 || !pt_paula_render_caps_valid(c) ||
       (rate!=44100 && rate!=48000) || !step || step>UINT64_MAX/rate)return 0;
    side=slot==0 || slot==3?0:1;
    if(gains[1-side] || gains[side]>65536 || gains[side]%1024)return 0;
    numerator=(uint64_t)c->clock_hz<<32;denominator=step*rate;
    value=numerator/denominator;remainder=numerator%denominator;
    /* Comparison avoids overflow from 2*remainder or rounded numerator. */
    if(remainder>=denominator-remainder)++value;
    if(value<c->minimum_period || value>c->maximum_period)return 0;
    *period=(uint16_t)value;*volume=(uint8_t)(gains[side]/1024);return 1;
}
int pt_paula_render_voice(const struct pt_voice *v,unsigned rate,const uint32_t gains[2],unsigned slot,
    const struct pt_paula_render_caps *c,struct pt_paula_render_plan *out)
{
    struct pt_paula_render_plan p={0};uint32_t length;
    if(!v || !out || !v->pcm || !v->pcm->data || v->pcm->channels!=1 ||
       (v->pcm->bits!=8 && v->pcm->bits!=16 && v->pcm->bits!=24) || v->pcm->capacity<v->pcm->frames ||
       v->active!=1 || v->segment || v->repeat_pcm || v->linear || v->loop!=PT_VOICE_ONCE ||
       v->looped || v->loop_start || v->loop_end || v->cycle || v->start>=v->end ||
       v->end>v->pcm->frames || v->phase!=((uint64_t)v->start<<32) || (v->start&1))return 0;
    length=v->end-v->start;
    if((length&1) || length<2 || length>131070 ||
       !pt_paula_render_control(v->step,rate,gains,slot,c,&p.period,&p.volume))return 0;
    p.offset=v->start;p.length=length;*out=p;return 1;
}
