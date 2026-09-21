#include "invert_sequence.h"
static enum pt_pcm_result update(struct pt_invert_sequence *s,unsigned ch,struct pt_invert_pcm *bank)
{
    uint32_t unused;
    if(!s->instrument[ch]){pt_invert_loop_update(s->channel+ch,&unused);return PT_PCM_OK;}
    return pt_invert_pcm_update(bank+s->instrument[ch]-1,s->channel+ch);
}
enum pt_pcm_result pt_invert_sequence_tick(struct pt_invert_sequence *s,const struct pt_flow *f,uint16_t tracks,struct pt_invert_pcm *bank,size_t count)
{
    unsigned ch;const struct pt_project *p;enum pt_pcm_result result;
    if(!s || !f || !(p=f->project) || !bank || count<p->sample_count || !p->channels.count || p->channels.count>16 ||
       (tracks>>p->channels.count) || f->played_order>=p->order_count || f->played_row>=64 || p->orders[f->played_order]>=p->pattern_count)return PT_PCM_INVALID;
    for(ch=0;ch<p->channels.count;++ch)if(tracks&(1U<<ch)) {
        unsigned effect=f->effect[ch],parameter=f->parameter[ch];
        if(s->instrument[ch]>count)return PT_PCM_INVALID;
        if(f->fresh) {
            const struct pt_event *e=p->events+((size_t)p->orders[f->played_order]*64+f->played_row)*p->channels.count+ch;
            if(e->slice)return PT_PCM_INVALID;
            if(e->instrument) {
                const struct pt_sample *sample;
                if(e->instrument>p->sample_count)return PT_PCM_INVALID;
                sample=p->samples+e->instrument-1;
                if(sample->loop>PT_LOOP_FORWARD || sample->interpolation ||
                   bank[e->instrument-1].source!=&sample->pcm ||
                   !pt_invert_loop_bind(s->channel+ch,sample->pcm.frames,sample->loop?sample->loop_start:0,sample->loop?sample->loop_end:2))return PT_PCM_INVALID;
                s->instrument[ch]=e->instrument;
            }
        } else {result=update(s,ch,bank);if(result!=PT_PCM_OK)return result;}
        if(!f->counter && effect==14 && (parameter>>4)==15) {
            if(!pt_invert_loop_speed(s->channel+ch,parameter&15))return PT_PCM_INVALID;
            if(parameter&15){result=update(s,ch,bank);if(result!=PT_PCM_OK)return result;}
        }
    }
    return PT_PCM_OK;
}
