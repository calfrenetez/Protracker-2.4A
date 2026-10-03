#include "studio_internal.h"
#include "voice_internal.h"
#include <string.h>
struct pt_studio_mix {
    struct pt_allocator allocator;
    struct pt_studio_source source;
    struct pt_voice voice[16];
    struct pt_pcm pcm[16],pending_pcm[16];
    void *token[16],*pending_token[16];
    uint32_t gain[16][2];
    unsigned count;
    uint8_t pinned[16],pending[16];
};
static int output_overlaps(const void *out,size_t bytes,const void *owner,size_t size)
{
    uintptr_t x=(uintptr_t)out,y=(uintptr_t)owner;
    if(!bytes || !size)return 0;
    if(bytes>UINTPTR_MAX-x || size>UINTPTR_MAX-y)return 1;
    return x<y+size && y<x+bytes;
}
int pt_studio_mix_output_disjoint(const struct pt_studio_mix *s,const void *out,size_t bytes)
{
    unsigned i;
    if(!s || !out || bytes>UINTPTR_MAX-(uintptr_t)out || output_overlaps(out,bytes,s,sizeof(*s)))return 0;
    for(i=0;i<s->count;++i) {
        const struct pt_pcm *pcm[2]={&s->pcm[i],&s->pending_pcm[i]};unsigned j;
        for(j=0;j<2;++j)if(j?s->pending[i]:s->pinned[i]) {
            if(pcm[j]->capacity>SIZE_MAX/sizeof(*pcm[j]->data) ||
               output_overlaps(out,bytes,pcm[j]->data,pcm[j]->capacity*sizeof(*pcm[j]->data)))return 0;
        }
    }
    return 1;
}
struct pt_studio_mix *pt_studio_open(const struct pt_allocator *a,const struct pt_studio_source *source,unsigned count)
{
    struct pt_studio_mix *s;
    if(!a || !a->allocate || !a->release || !source || !source->acquire || !source->release || !count || count>16)return NULL;
    s=a->allocate(a->context,sizeof(*s));if(!s)return NULL;
    memset(s,0,sizeof(*s));s->allocator=*a;s->source=*source;s->count=count;return s;
}
void pt_studio_stop(struct pt_studio_mix *s,unsigned channel)
{
    if(!s || channel>=s->count)return;
    memset(&s->voice[channel],0,sizeof(s->voice[channel]));
    if(s->pinned[channel]) {
        s->pinned[channel]=0;s->source.release(s->source.context,s->token[channel]);
    }
    if(s->pending[channel]) {
        s->pending[channel]=0;s->source.release(s->source.context,s->pending_token[channel]);
    }
    s->pending_token[channel]=NULL;
    s->token[channel]=NULL;memset(&s->pcm[channel],0,sizeof(s->pcm[channel]));
}
void pt_studio_close(struct pt_studio_mix *s)
{
    struct pt_allocator a;unsigned i;if(!s)return;
    a=s->allocator;for(i=0;i<s->count;++i)pt_studio_stop(s,i);
    a.release(a.context,s);
}
static enum pt_pcm_result trigger(struct pt_studio_mix *s,unsigned channel,const struct pt_studio_note *note,unsigned segment,unsigned prepared)
{
    struct pt_pcm pcm;struct pt_voice voice;void *token=NULL;enum pt_pcm_result result;
    if(!s || !note || channel>=s->count || note->gain[0]>65536 || note->gain[1]>65536 || (segment && note->loop!=PT_VOICE_FORWARD))return PT_PCM_INVALID;
    memset(&pcm,0,sizeof(pcm));
    if(!s->source.acquire(s->source.context,note->key,note->version,&pcm,&token))return PT_PCM_CAPACITY;
    if(prepared)result=segment?pt_voice_init_segment_validated(&voice,&pcm,note->start,note->end,note->loop_start,note->loop_end,note->step,note->linear):
        pt_voice_init_validated(&voice,&pcm,note->start,note->end,note->loop,note->loop_start,note->loop_end,note->step,note->linear);
    else result=segment?pt_voice_init_segment(&voice,&pcm,note->start,note->end,note->loop_start,note->loop_end,note->step,note->linear):
        pt_voice_init(&voice,&pcm,note->start,note->end,note->loop,note->loop_start,note->loop_end,note->step,note->linear);
    if(result!=PT_PCM_OK) {s->source.release(s->source.context,token);return result;}
    pt_studio_stop(s,channel);s->pcm[channel]=pcm;s->voice[channel]=voice;
    s->voice[channel].pcm=&s->pcm[channel];
    if(voice.repeat_pcm)s->voice[channel].repeat_pcm=&s->pcm[channel];
    s->token[channel]=token;s->pinned[channel]=1;
    s->gain[channel][0]=note->gain[0];s->gain[channel][1]=note->gain[1];return PT_PCM_OK;
}
enum pt_pcm_result pt_studio_trigger(struct pt_studio_mix *s,unsigned channel,const struct pt_studio_note *note)
{return trigger(s,channel,note,0,0);}
enum pt_pcm_result pt_studio_trigger_segment(struct pt_studio_mix *s,unsigned channel,const struct pt_studio_note *note)
{return trigger(s,channel,note,1,0);}
enum pt_pcm_result pt_studio_trigger_prepared(struct pt_studio_mix *s,unsigned channel,const struct pt_studio_note *note)
{return trigger(s,channel,note,0,1);}
enum pt_pcm_result pt_studio_segment_prepared(struct pt_studio_mix *s,unsigned channel,const struct pt_studio_note *note)
{return trigger(s,channel,note,1,1);}
static enum pt_pcm_result repeat(struct pt_studio_mix *s,unsigned channel,
    uint64_t key,uint64_t version,uint32_t start,uint32_t end,unsigned prepared)
{
    struct pt_pcm pcm;struct pt_voice voice;void *token=NULL;enum pt_pcm_result result;
    if(!s || channel>=s->count || !s->voice[channel].active)return PT_PCM_INVALID;
    memset(&pcm,0,sizeof(pcm));
    if(!s->source.acquire(s->source.context,key,version,&pcm,&token))return PT_PCM_CAPACITY;
    voice=s->voice[channel];result=prepared?pt_voice_set_repeat_source_validated(&voice,&pcm,start,end):pt_voice_set_repeat_source(&voice,&pcm,start,end);
    if(result!=PT_PCM_OK) {s->source.release(s->source.context,token);return result;}
    if(s->pending[channel])s->source.release(s->source.context,s->pending_token[channel]);
    s->pending_pcm[channel]=pcm;s->pending_token[channel]=token;s->pending[channel]=1;
    s->voice[channel]=voice;s->voice[channel].repeat_pcm=&s->pending_pcm[channel];return PT_PCM_OK;
}
enum pt_pcm_result pt_studio_repeat(struct pt_studio_mix *s,unsigned channel,uint64_t key,uint64_t version,uint32_t start,uint32_t end)
{return repeat(s,channel,key,version,start,end,0);}
enum pt_pcm_result pt_studio_repeat_prepared(struct pt_studio_mix *s,unsigned channel,uint64_t key,uint64_t version,uint32_t start,uint32_t end)
{return repeat(s,channel,key,version,start,end,1);}
enum pt_pcm_result pt_studio_control(struct pt_studio_mix *s,uint16_t tracks,
    const struct pt_studio_control *control)
{
    unsigned i;
    if(!s || ((uint32_t)tracks>>s->count) || (tracks && !control))return PT_PCM_INVALID;
    for(i=0;i<s->count;++i)if(tracks&(1U<<i)) {
        if(!s->voice[i].active || !control[i].step || control[i].gain[0]>65536 || control[i].gain[1]>65536)return PT_PCM_INVALID;
    }
    for(i=0;i<s->count;++i)if(tracks&(1U<<i)) {
        s->voice[i].step=control[i].step;
        s->gain[i][0]=control[i].gain[0];s->gain[i][1]=control[i].gain[1];
    }
    return PT_PCM_OK;
}
enum pt_pcm_result pt_studio_read(struct pt_studio_mix *s,struct pt_pcm *output,uint64_t *clipped)
{
    enum pt_pcm_result result;unsigned i;
    if(!s || !output || !clipped || output->rate!=48000 || output->bits!=24 || output->channels!=2 || !output->frames || output->frames>256)return PT_PCM_INVALID;
    result=pt_voice_mix(s->voice,s->count,(const uint32_t (*)[2])s->gain,output,clipped);
    if(result==PT_PCM_OK)for(i=0;i<s->count;++i) {
        if(!s->voice[i].active)pt_studio_stop(s,i);
        else if(s->pending[i] && s->voice[i].pcm==&s->pending_pcm[i]) {
            /* The entire block has completed, so no sample read still borrows
               the old source. Move descriptor ownership without copying PCM. */
            s->source.release(s->source.context,s->token[i]);
            s->pcm[i]=s->pending_pcm[i];s->token[i]=s->pending_token[i];
            s->voice[i].pcm=&s->pcm[i];s->pending[i]=0;s->pending_token[i]=NULL;
        }
    }
    return result;
}
