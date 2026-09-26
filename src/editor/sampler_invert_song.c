#include "sampler_invert_song.h"
#include <string.h>
struct pt_sampler_invert_song {
    struct pt_allocator allocator;struct pt_sampler *sampler;struct pt_project *project;
    struct pt_render_invert_session *song;
    const struct pt_sample *samples;const struct pt_event *events;const uint16_t *orders;
    unsigned generation,count;enum pt_render_result failure;
};
void pt_sampler_invert_song_stop(struct pt_sampler_invert_song *s)
{if(s) {pt_render_invert_close(s->song);s->song=NULL;}}
void pt_sampler_invert_song_close(struct pt_sampler_invert_song *s)
{if(s) {struct pt_allocator a=s->allocator;pt_sampler_invert_song_stop(s);a.release(a.context,s);}}
enum pt_render_result pt_sampler_invert_song_open(struct pt_sampler *sampler,struct pt_project *p,
    const struct pt_render_options *o,size_t budget,const struct pt_allocator *a,struct pt_sampler_invert_song **out)
{
    struct pt_sampler_invert_song *s;enum pt_render_result result;
    if(!sampler || !p || !out || !a || !a->allocate || !a->release)return PT_RENDER_INVALID;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_RENDER_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;s->sampler=sampler;s->project=p;
    s->generation=sampler->generation;s->count=p->sample_count;s->samples=p->samples;s->events=p->events;s->orders=p->orders;
    result=pt_render_invert_open(p,o,budget,a,&s->song);
    if(result!=PT_RENDER_OK) {pt_sampler_invert_song_close(s);return result;}
    *out=s;return PT_RENDER_OK;
}
enum pt_render_result pt_sampler_invert_song_pull(struct pt_sampler_invert_song *s,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    struct pt_project *p;
    if(!s || !pcm || !done || !frames || frames>256)return PT_RENDER_INVALID;
    *pcm=NULL;*done=1;
    if(s->failure)return s->failure;
    if(!s->song)return PT_RENDER_OK;
    p=s->project;
    if(s->generation!=s->sampler->generation || p->samples!=s->samples || p->sample_count!=s->count ||
       p->events!=s->events || p->orders!=s->orders) {
        s->failure=PT_RENDER_INVALID;pt_sampler_invert_song_stop(s);return s->failure;
    }
    s->failure=pt_render_invert_pull(s->song,frames,pcm,done);
    if(s->failure || *done)pt_sampler_invert_song_stop(s);
    return s->failure;
}
