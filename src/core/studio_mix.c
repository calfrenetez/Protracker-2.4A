#include "studio_mix.h"
#include <string.h>
struct pt_studio_mix {
    struct pt_allocator allocator;
    struct pt_studio_source source;
    struct pt_voice voice[16];
    struct pt_pcm pcm[16];
    void *token[16];
    uint32_t gain[16][2];
    unsigned count;
    uint8_t pinned[16];
};
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
    s->token[channel]=NULL;memset(&s->pcm[channel],0,sizeof(s->pcm[channel]));
}
void pt_studio_close(struct pt_studio_mix *s)
{
    struct pt_allocator a;unsigned i;if(!s)return;
    a=s->allocator;for(i=0;i<s->count;++i)pt_studio_stop(s,i);
    a.release(a.context,s);
}
enum pt_pcm_result pt_studio_trigger(struct pt_studio_mix *s,unsigned channel,const struct pt_studio_note *note)
{
    struct pt_pcm pcm;struct pt_voice voice;void *token=NULL;enum pt_pcm_result result;
    if(!s || !note || channel>=s->count || note->gain[0]>65536 || note->gain[1]>65536)return PT_PCM_INVALID;
    memset(&pcm,0,sizeof(pcm));
    if(!s->source.acquire(s->source.context,note->key,note->version,&pcm,&token))return PT_PCM_CAPACITY;
    result=pt_voice_init(&voice,&pcm,note->start,note->end,note->loop,note->loop_start,note->loop_end,note->step,note->linear);
    if(result!=PT_PCM_OK) {s->source.release(s->source.context,token);return result;}
    pt_studio_stop(s,channel);s->pcm[channel]=pcm;s->voice[channel]=voice;
    s->voice[channel].pcm=&s->pcm[channel];
    if(voice.repeat_pcm)s->voice[channel].repeat_pcm=&s->pcm[channel];
    s->token[channel]=token;s->pinned[channel]=1;
    s->gain[channel][0]=note->gain[0];s->gain[channel][1]=note->gain[1];return PT_PCM_OK;
}
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
    if(result==PT_PCM_OK)for(i=0;i<s->count;++i)if(!s->voice[i].active)pt_studio_stop(s,i);
    return result;
}
