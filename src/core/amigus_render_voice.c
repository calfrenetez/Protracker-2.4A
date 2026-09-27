#include "amigus_render_voice.h"
int pt_amigus_render_control(uint64_t step,unsigned output_rate,const uint32_t gains[2],
    uint32_t *rate,uint16_t *left,uint16_t *right)
{
    uint64_t product,value;
    if(!rate || !left || !right || !gains || (output_rate!=44100 && output_rate!=48000) ||
       !step || step>(((uint64_t)192000<<32)/output_rate) || gains[0]>65536 || gains[1]>65536)return 0;
    /* (step/2^32 * output_rate) *2^30/192000; bound above prevents overflow. */
    product=step*output_rate;value=product/768000;
    if(!value)return 0;
    *rate=(uint32_t)value;
    *left=(uint16_t)(((uint64_t)gains[0]*65535+32768)/65536);
    *right=(uint16_t)(((uint64_t)gains[1]*65535+32768)/65536);return 1;
}
int pt_amigus_render_voice(const struct pt_voice *v,unsigned output_rate,const uint32_t gains[2],
    const struct pt_playback_format *f,uint32_t address,uint32_t bytes,struct pt_amigus_voice_plan *out)
{
    struct pt_amigus_voice_plan p={0};uint32_t width,end;uint64_t size;
    if(!v || !v->pcm || !f || !out || !v->active || v->segment || v->repeat_pcm ||
       v->pcm->channels!=1 || (v->pcm->bits!=8 && v->pcm->bits!=16 && v->pcm->bits!=24) ||
       f->channel || (f->bits!=8 && f->bits!=16) || f->little_endian>1 || f->word_pad ||
       v->linear>1 || v->loop>PT_VOICE_FORWARD || v->looped>1 ||
       v->start>=v->end || v->end>v->pcm->frames || address>=PT_AMIGUS_RAM_ADDRESS_SPACE)return 0;
    width=f->bits/8;size=(uint64_t)v->pcm->frames*width;
    if(size!=bytes || size>PT_AMIGUS_RAM_ADDRESS_SPACE-address)return 0;
    if(v->loop==PT_VOICE_ONCE) {
        if(v->loop_start || v->loop_end || v->looped)return 0;
        end=v->end;p.loop=address;
    } else {
        if(v->loop_start<v->start || v->loop_start>=v->loop_end || v->loop_end>v->end)return 0;
        end=v->loop_end;p.loop=address+v->loop_start*width;
    }
    if(v->looped) {if(v->start!=v->loop_start || v->phase)return 0;}
    else if(v->phase!=((uint64_t)v->start<<32))return 0;
    p.start=address+v->start*width;p.end_exclusive=address+end*width;
    if(((p.start|p.loop|p.end_exclusive)&1) || p.end_exclusive>=PT_AMIGUS_RAM_ADDRESS_SPACE)return 0;
    if(!pt_amigus_render_control(v->step,output_rate,gains,&p.rate,&p.left,&p.right))return 0;
    p.control=(uint16_t)(0x8000|(f->bits==16?1:0)|(v->loop?2:0)|(v->linear?4:0)|
        (f->bits==16 && f->little_endian?8:0));*out=p;return 1;
}

int pt_amigus_render_restore(const struct pt_voice *v,unsigned output_rate,const uint32_t gains[2],
    const struct pt_playback_format *f,uint32_t address,uint32_t bytes,struct pt_amigus_restore_plan *out)
{
    struct pt_amigus_restore_plan p={0};struct pt_voice initial;uint64_t position,cycle;
    if(!v || !out || v->active!=1 || v->loop>PT_VOICE_FORWARD || v->looped>1)return 0;
    initial=*v;initial.looped=(uint8_t)(v->loop && v->start==v->loop_start);
    initial.phase=initial.looped?0:(uint64_t)v->start<<32;
    if(!pt_amigus_render_voice(&initial,output_rate,gains,f,address,bytes,&p.bounds))return 0;
    position=v->phase;
    if(v->loop==PT_VOICE_ONCE) {
        if(v->looped || v->cycle || position<((uint64_t)v->start<<32) || position>=((uint64_t)v->end<<32))return 0;
    } else {
        cycle=(uint64_t)(v->loop_end-v->loop_start)<<32;
        if(v->cycle!=cycle)return 0;
        if(v->looped) {
            if(position>=cycle)return 0;
            position+=(uint64_t)v->loop_start<<32;
        } else if(position<((uint64_t)v->start<<32) || position>=((uint64_t)v->loop_start<<32))return 0;
    }
    /* Bounds validation limits the whole cache below2^25 bytes, so this exact
       conversion fits below2^57 with no rounding, truncation or overflow. */
    p.cursor_q32=((uint64_t)address<<32)+position*(f->bits/8);
    *out=p;return 1;
}
