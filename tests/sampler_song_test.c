#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/editor/sampler_song.h"
static unsigned live;
static void *alloc(void *c,size_t n) {void *p;(void)c;p=malloc(n);if(p)++live;return p;}
static void drop(void *c,void *p) {(void)c;if(p){assert(live);--live;free(p);}}
static int32_t first(struct pt_sampler_song *song)
{
    const struct pt_pcm *out;unsigned done,i;
    for(i=0;i<100;++i) {assert(pt_sampler_song_pull(song,1,&out,&done)==PT_RENDER_OK && !done);if(out)return out->data[0];}
    assert(0);return 0;
}
int main(void)
{
    struct pt_allocator a={NULL,alloc,drop};struct pt_document d;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[2];struct pt_event_change changes[2];
    struct pt_sampler_song *song=NULL;struct pt_render_options o={0};
    int32_t original[]={257,-513,1025,-2049};unsigned done;const struct pt_pcm *out;
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){original,4,4,48000,1,24};
    d.project.samples[0].volume=64;d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=4;
    d.project.channels.track[0].pan=0;d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;d.project.events[0].instrument=1;
    d.project.events[4].effect=15;
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,&d.project,commands,2,changes,2)==PT_EDIT_OK);
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=1000;o.frame_limit=1000000;
    assert(pt_sampler_song_open(&s,&d.project,&o,&a,&song)==PT_RENDER_OK);assert(first(song)==257);
    assert(d.project.samples[0].pcm.data!=original && s.bytes);
    /* Required edit lifecycle: stop before publishing the edited master. */
    pt_sampler_song_stop(song);pt_sampler_song_close(song);
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
    assert(pt_sampler_song_open(&s,&d.project,&o,&a,&song)==PT_RENDER_OK);assert(first(song)==514);
    /* Defensive stale-generation gate releases pins without emitting more audio. */
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
    assert(pt_sampler_song_pull(song,256,&out,&done)==PT_RENDER_INVALID && !out && done);
    assert(pt_sampler_song_pull(song,256,&out,&done)==PT_RENDER_INVALID);pt_sampler_song_close(song);
    assert(pt_sampler_song_open(&s,&d.project,&o,&a,&song)==PT_RENDER_OK);assert(first(song)==1028);
    pt_sampler_song_stop(song);
    pt_pattern_history_release(&h);pt_sampler_release(&s);pt_document_release(&d);
    assert(!s.bytes);assert(pt_sampler_song_pull(song,1,&out,&done)==PT_RENDER_OK && done && !out);
    pt_sampler_song_close(song);assert(!live && original[0]==257);
    puts("SAMPLER SONG PASS: generation binding, true24 promotion, stop/edit/restart, stale refusal and owner cleanup");return 0;
}
