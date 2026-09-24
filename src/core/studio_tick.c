#include "studio_tick.h"
#include "frame_clock.h"
struct pt_studio_tick {
    struct pt_allocator allocator;
    struct pt_studio_mix *mixer;
    struct pt_frame_clock clock;
    uint32_t remaining;
};
struct pt_studio_tick *pt_studio_tick_open(const struct pt_allocator *a,struct pt_studio_mix *mixer,uint64_t limit)
{
    struct pt_studio_tick *s;struct pt_frame_clock clock;
    if(!a || !a->allocate || !a->release || !mixer || pt_frame_clock_init(&clock,48000,limit)!=PT_CLOCK_OK)return NULL;
    s=a->allocate(a->context,sizeof(*s));if(!s)return NULL;
    s->allocator=*a;s->mixer=mixer;s->clock=clock;s->remaining=0;return s;
}
void pt_studio_tick_close(struct pt_studio_tick *s)
{if(s){struct pt_allocator a=s->allocator;a.release(a.context,s);}}
enum pt_pcm_result pt_studio_tick_begin(struct pt_studio_tick *s,unsigned bpm)
{
    uint32_t frames;enum pt_clock_result result;
    if(!s || s->remaining)return PT_PCM_INVALID;
    result=pt_frame_clock_tick(&s->clock,bpm,&frames);
    if(result!=PT_CLOCK_OK)return result==PT_CLOCK_LIMIT?PT_PCM_CAPACITY:PT_PCM_INVALID;
    s->remaining=frames;return PT_PCM_OK;
}
uint32_t pt_studio_tick_remaining(const struct pt_studio_tick *s)
{return s?s->remaining:0;}
enum pt_pcm_result pt_studio_tick_read(struct pt_studio_tick *s,struct pt_pcm *output,uint64_t *clipped)
{
    enum pt_pcm_result result;
    if(!s || !output || !output->frames || output->frames>s->remaining)return PT_PCM_INVALID;
    result=pt_studio_read(s->mixer,output,clipped);
    if(result==PT_PCM_OK)s->remaining-=output->frames;
    return result;
}
