#include "voice.h"
#include <string.h>
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!an || !bn)return 0;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return 1;
    return x<y+bn && y<x+an;
}
static int64_t rounded(int64_t n,int64_t divisor)
{return n<0?-((-n+divisor/2)/divisor):(n+divisor/2)/divisor;}
enum pt_pcm_result pt_voice_init(struct pt_voice *v,const struct pt_pcm *p,
                                uint32_t start,uint32_t end,enum pt_voice_loop loop,
                                uint32_t a,uint32_t b,uint64_t step,unsigned linear)
{
    struct pt_voice next;enum pt_pcm_result result;
    if(!v || !p || !step || linear>1 || start>end || end>p->frames ||
       (start==end && p->frames) || loop>PT_VOICE_PINGPONG || loop<PT_VOICE_ONCE)
        return PT_PCM_INVALID;
    if(loop==PT_VOICE_ONCE) {if(a || b)return PT_PCM_INVALID;}
    else if(a<start || b>end || a>=b || (loop==PT_VOICE_PINGPONG && b-a>0x80000000UL))return PT_PCM_INVALID;
    result=pt_pcm_validate(p);if(result!=PT_PCM_OK)return result;
    if(overlap(v,sizeof(*v),p,sizeof(*p)) || overlap(v,sizeof(*v),p->data,(size_t)p->frames*p->channels*sizeof(*p->data)))return PT_PCM_ALIAS;
    memset(&next,0,sizeof(next));next.pcm=p;next.start=start;next.end=end;next.step=step;
    next.loop=(uint8_t)loop;next.loop_start=a;next.loop_end=b;next.linear=(uint8_t)linear;
    next.active=start<end;next.phase=(uint64_t)start<<32;
    if(loop) {
        next.cycle=(uint64_t)(b-a-(loop==PT_VOICE_PINGPONG))<<32;
        if(loop==PT_VOICE_PINGPONG)next.cycle*=2;
        if(start==a) {next.looped=1;next.phase=0;}
    }
    *v=next;return PT_PCM_OK;
}
enum pt_pcm_result pt_voice_init_segment(struct pt_voice *v,const struct pt_pcm *p,
                                        uint32_t start,uint32_t end,uint32_t a,uint32_t b,
                                        uint64_t step,unsigned linear)
{
    enum pt_pcm_result result;
    if(!p || start>=end || end>p->frames)return PT_PCM_INVALID;
    result=pt_voice_init(v,p,0,p->frames,PT_VOICE_FORWARD,a,b,step,linear);
    if(result!=PT_PCM_OK)return result;
    v->start=start;v->end=end;v->phase=(uint64_t)start<<32;v->looped=0;v->segment=1;
    return PT_PCM_OK;
}
enum pt_pcm_result pt_voice_set_repeat(struct pt_voice *v,uint32_t start,uint32_t end)
{
    struct pt_voice next;
    if(!v || !v->pcm || !v->active || v->loop==PT_VOICE_PINGPONG ||
       start>=end || end>v->pcm->frames)return PT_PCM_INVALID;
    next=*v;
    if(next.looped) {
        next.phase+=((uint64_t)next.loop_start<<32);
        next.end=next.loop_end;
    } else if(next.loop && !next.segment)next.end=next.loop_start;
    next.looped=0;next.segment=1;next.loop=PT_VOICE_FORWARD;
    next.loop_start=start;next.loop_end=end;next.cycle=(uint64_t)(end-start)<<32;
    *v=next;return PT_PCM_OK;
}
static void advance(struct pt_voice *v)
{
    uint64_t distance,amount;
    if(!v->loop) {
        distance=((uint64_t)v->end<<32)-v->phase;
        if(v->step>=distance) {v->active=0;v->phase=(uint64_t)v->end<<32;}
        else v->phase+=v->step;
    } else if(!v->looped) {
        distance=((uint64_t)(v->segment?v->end:v->loop_start)<<32)-v->phase;
        if(v->step<distance)v->phase+=v->step;
        else {v->looped=1;v->phase=v->cycle?(v->step-distance)%v->cycle:0;}
    } else if(v->cycle) {
        amount=v->step%v->cycle;distance=v->cycle-v->phase;
        v->phase=amount>=distance?amount-distance:v->phase+amount;
    }
}
static void frame(struct pt_voice *v,int32_t out[2])
{
    uint64_t phase=v->phase;uint32_t index,next,fraction;unsigned side;
    out[0]=out[1]=0;if(!v->active || !v->pcm)return;
    if(v->looped) {
        if(v->loop==PT_VOICE_PINGPONG && phase>v->cycle/2)phase=v->cycle-phase;
        phase+=((uint64_t)v->loop_start<<32);
    }
    index=(uint32_t)(phase>>32);fraction=(uint32_t)phase;next=index+1;
    if(v->segment && !v->looped) {if(next==v->end)next=v->loop_start;}
    else if(v->loop && next==v->loop_end)next=v->loop==PT_VOICE_FORWARD?v->loop_start:index;
    else if(!v->loop && next==v->end)next=index;
    for(side=0;side<2;++side) {
        unsigned channel=v->pcm->channels==1?0:side;
        int32_t scale=(int32_t)1<<(24-v->pcm->bits);
        int32_t value=v->pcm->data[(size_t)index*v->pcm->channels+channel]*scale;
        if(v->linear && fraction) {
            int32_t other=v->pcm->data[(size_t)next*v->pcm->channels+channel]*scale;
            value+=(int32_t)rounded((int64_t)(other-value)*fraction,4294967296LL);
        }
        out[side]=value;
    }
    advance(v);
}
enum pt_pcm_result pt_voice_frame(struct pt_voice *v,int32_t out[2])
{
    if(!v || !out || !v->pcm)return PT_PCM_INVALID;
    if(overlap(out,2*sizeof(*out),v,sizeof(*v)) || overlap(out,2*sizeof(*out),v->pcm,sizeof(*v->pcm)) ||
       overlap(out,2*sizeof(*out),v->pcm->data,(size_t)v->pcm->frames*v->pcm->channels*sizeof(*out)))return PT_PCM_ALIAS;
    frame(v,out);return PT_PCM_OK;
}
enum pt_pcm_result pt_voice_mix(struct pt_voice *v,unsigned count,const uint32_t (*gains)[2],
                               struct pt_pcm *out,uint64_t *clipped)
{
    unsigned ch,side;uint32_t i;size_t bytes;uint64_t clips=0;int32_t high,low;int64_t divisor;
    if(!out || !clipped || count>16 || (count && (!v || !gains)) || out->channels!=2 ||
       (out->bits!=16 && out->bits!=24) || !out->rate || out->rate>192000 || (out->frames && !out->data))return PT_PCM_INVALID;
    if(out->frames>out->capacity/out->channels || out->frames>SIZE_MAX/out->channels/sizeof(*out->data))return PT_PCM_CAPACITY;
    bytes=(size_t)out->frames*2*sizeof(*out->data);
    if(overlap(out->data,bytes,out,sizeof(*out)) || overlap(out->data,bytes,v,count*sizeof(*v)) ||
       overlap(out->data,bytes,gains,count*sizeof(*gains)) || overlap(out->data,bytes,clipped,sizeof(*clipped)) ||
       overlap(v,count*sizeof(*v),gains,count*sizeof(*gains)) || overlap(v,count*sizeof(*v),out,sizeof(*out)) ||
       overlap(clipped,sizeof(*clipped),v,count*sizeof(*v)) || overlap(clipped,sizeof(*clipped),out,sizeof(*out)) ||
       overlap(clipped,sizeof(*clipped),gains,count*sizeof(*gains)))return PT_PCM_ALIAS;
    for(ch=0;ch<count;++ch) {
        const struct pt_pcm *p=v[ch].pcm;
        if(gains[ch][0]>65536 || gains[ch][1]>65536)return PT_PCM_INVALID;
        if(p && (overlap(v,count*sizeof(*v),p,sizeof(*p)) || overlap(v,count*sizeof(*v),p->data,(size_t)p->frames*p->channels*sizeof(*p->data)) ||
                 overlap(out->data,bytes,p,sizeof(*p)) || overlap(out->data,bytes,p->data,(size_t)p->frames*p->channels*sizeof(*p->data)) ||
                 overlap(clipped,sizeof(*clipped),p,sizeof(*p)) || overlap(clipped,sizeof(*clipped),p->data,(size_t)p->frames*p->channels*sizeof(*p->data))))return PT_PCM_ALIAS;
    }
    high=((int32_t)1<<(out->bits-1))-1;low=-high-1;divisor=(int64_t)65536<<(24-out->bits);
    for(i=0;i<out->frames;++i) {
        int64_t sum[2]={0,0};
        for(ch=0;ch<count;++ch) {
            int32_t sample[2];frame(v+ch,sample);
            for(side=0;side<2;++side)sum[side]+=(int64_t)sample[side]*gains[ch][side];
        }
        for(side=0;side<2;++side) {
            int64_t value=rounded(sum[side],divisor);
            if(value>high) {value=high;++clips;}else if(value<low) {value=low;++clips;}
            out->data[(size_t)i*2+side]=(int32_t)value;
        }
    }
    *clipped=clips;return PT_PCM_OK;
}
