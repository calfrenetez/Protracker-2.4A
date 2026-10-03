#include "sampler_invert_song.h"
#include "project_snapshot.h"
#include "sampler_internal.h"
#include <string.h>
static int overlaps(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!an || !bn)return 0;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return 1;
    return x<y+bn && y<x+an;
}
struct pt_sampler_invert_song {
    struct pt_allocator allocator;struct pt_sampler *sampler;struct pt_project *project;
    struct pt_render_invert_session *song;struct pt_project snapshot;
    unsigned generation,ready;enum pt_render_result failure;
};
static int current(struct pt_sampler_invert_song *s);
static int fixed_output_disjoint(const struct pt_sampler_invert_song *s,const void *out,size_t bytes)
{
    return out && bytes && bytes<=UINTPTR_MAX-(uintptr_t)out && !overlaps(out,bytes,s,sizeof(*s)) &&
        !overlaps(out,bytes,s->sampler,sizeof(*s->sampler)) &&
        !overlaps(out,bytes,s->project,sizeof(*s->project));
}
int pt_sampler_invert_song_matches_owner(const struct pt_sampler_invert_song *s,
    const struct pt_sampler *sampler,const struct pt_project *project)
{return s && s->sampler==sampler && s->project==project;}
int pt_sampler_invert_song_output_disjoint(struct pt_sampler_invert_song *s,const void *out,size_t bytes)
{
    if(!s || !fixed_output_disjoint(s,out,bytes))return 0;
    if(s->failure || !s->song || !current(s))return 1;
    return pt_render_invert_output_disjoint(s->song,out,bytes) &&
        pt_sampler_output_disjoint(s->sampler,out,bytes);
}
void pt_sampler_invert_song_stop(struct pt_sampler_invert_song *s)
{if(s) {pt_render_invert_close(s->song);s->song=NULL;s->ready=0;}}
void pt_sampler_invert_song_close(struct pt_sampler_invert_song *s)
{if(s) {struct pt_allocator a=s->allocator;pt_sampler_invert_song_stop(s);a.release(a.context,s);}}
enum pt_render_result pt_sampler_invert_song_begin(struct pt_sampler *sampler,struct pt_project *p,
    const struct pt_render_options *o,size_t budget,const struct pt_allocator *a,struct pt_sampler_invert_song **out)
{
    struct pt_sampler_invert_song *s;enum pt_render_result result;
    if(!sampler || !p || !o || !out || !a || !a->allocate || !a->release ||
       overlaps(out,sizeof(*out),o,sizeof(*o)) || overlaps(out,sizeof(*out),a,sizeof(*a)))return PT_RENDER_INVALID;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_RENDER_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;s->sampler=sampler;s->project=p;
    s->generation=sampler->generation;memcpy(&s->snapshot,p,sizeof(*p));
    result=pt_render_invert_begin(p,o,budget,a,&s->song);
    if(result!=PT_RENDER_OK) {pt_sampler_invert_song_close(s);return result;}
    if(!current(s) || !fixed_output_disjoint(s,out,sizeof(*out)) ||
       !pt_sampler_invert_song_output_disjoint(s,out,sizeof(*out))) {
        pt_sampler_invert_song_close(s);return PT_RENDER_INVALID;
    }
    *out=s;return PT_RENDER_OK;
}
static enum pt_render_result fail(struct pt_sampler_invert_song *s,enum pt_render_result result)
{s->failure=result;pt_sampler_invert_song_stop(s);return result;}
static int current(struct pt_sampler_invert_song *s)
{
    s->snapshot.channels.selected=s->project->channels.selected;
    return s->generation==s->sampler->generation &&
        pt_project_snapshot_equal(s->project,&s->snapshot);
}
enum pt_render_result pt_sampler_invert_song_prepare(struct pt_sampler_invert_song *s,unsigned *ready)
{
    enum pt_render_result result;
    if(!s || !fixed_output_disjoint(s,ready,sizeof(*ready)))return PT_RENDER_INVALID;
    if(s->failure)return s->failure;
    if(!s->song)return PT_RENDER_INVALID;
    if(!current(s))return fail(s,PT_RENDER_INVALID);
    if(!pt_sampler_invert_song_output_disjoint(s,ready,sizeof(*ready)))return PT_RENDER_INVALID;
    *ready=0;
    if(s->ready) {*ready=1;return PT_RENDER_OK;}
    result=pt_render_invert_prepare(s->song,&s->ready);
    if(result!=PT_RENDER_OK)return fail(s,result);
    *ready=s->ready;return PT_RENDER_OK;
}
enum pt_render_result pt_sampler_invert_song_open(struct pt_sampler *sampler,struct pt_project *p,
    const struct pt_render_options *o,size_t budget,const struct pt_allocator *a,struct pt_sampler_invert_song **out)
{
    struct pt_sampler_invert_song *s=NULL;unsigned ready=0;enum pt_render_result result;
    if(!out || !o || !a || overlaps(out,sizeof(*out),o,sizeof(*o)) ||
       overlaps(out,sizeof(*out),a,sizeof(*a)))return PT_RENDER_INVALID;
    result=pt_sampler_invert_song_begin(sampler,p,o,budget,a,&s);
    while(result==PT_RENDER_OK && !ready)result=pt_sampler_invert_song_prepare(s,&ready);
    if(result!=PT_RENDER_OK) {pt_sampler_invert_song_close(s);return result;}
    if(!current(s) || !fixed_output_disjoint(s,out,sizeof(*out)) ||
       !pt_sampler_invert_song_output_disjoint(s,out,sizeof(*out))) {
        pt_sampler_invert_song_close(s);return PT_RENDER_INVALID;
    }
    *out=s;return PT_RENDER_OK;
}
enum pt_render_result pt_sampler_invert_song_pull(struct pt_sampler_invert_song *s,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    unsigned ready;
    if(!s || !pcm || !done || !frames || frames>256 ||
       !fixed_output_disjoint(s,pcm,sizeof(*pcm)) || !fixed_output_disjoint(s,done,sizeof(*done)) ||
       overlaps(pcm,sizeof(*pcm),done,sizeof(*done)))return PT_RENDER_INVALID;
    if(s->failure)return s->failure;
    if(s->song && !current(s))return fail(s,PT_RENDER_INVALID);
    if(!pt_sampler_invert_song_output_disjoint(s,pcm,sizeof(*pcm)) ||
       !pt_sampler_invert_song_output_disjoint(s,done,sizeof(*done)))return PT_RENDER_INVALID;
    *pcm=NULL;*done=1;
    if(!s->song)return PT_RENDER_OK;
    if(!s->ready) {
        enum pt_render_result result=pt_sampler_invert_song_prepare(s,&ready);
        *done=result!=PT_RENDER_OK;return result;
    }
    s->failure=pt_render_invert_pull(s->song,frames,pcm,done);
    if(s->failure || *done)pt_sampler_invert_song_stop(s);
    return s->failure;
}
