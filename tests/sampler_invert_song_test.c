#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/editor/sampler_invert_song.h"
#ifdef __amigaos__
static void test_failure(const char *condition,unsigned line)
{fprintf(stderr,"SAMPLER INVERT SONG FAIL: line=%u condition=%s\n",line,condition);exit(20);}
#undef assert
#define assert(condition) ((condition)?(void)0:test_failure(#condition,__LINE__))
#endif
static unsigned live,calls,fail_at;
static void *alloc(void *c,size_t n) {void *p;(void)c;if(++calls==fail_at)return NULL;p=malloc(n);if(p)++live;return p;}
static void drop(void *c,void *p) {(void)c;if(p){assert(live);--live;free(p);}}
static int32_t first(struct pt_sampler_invert_song *song)
{
    const struct pt_pcm *out;unsigned done,i;
    for(i=0;i<100;++i) {assert(pt_sampler_invert_song_pull(song,1,&out,&done)==PT_RENDER_OK && !done);if(out)return out->data[0];}
    assert(0);return 0;
}
int main(void)
{
    struct pt_allocator a={NULL,alloc,drop};struct pt_document d;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[2];struct pt_event_change changes[2];
    struct pt_sampler_invert_song *song=NULL;struct pt_render_options o={0};
    int32_t original[]={17,-93,30,40};unsigned done,before,i,count;const struct pt_pcm *out;
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){original,4,4,48000,1,8};
    d.project.samples[0].volume=64;d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=4;
    d.project.channels.track[0].pan=0;d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;d.project.events[0].instrument=1;
    d.project.events[0].effect=14;d.project.events[0].parameter=255;d.project.events[4].effect=15;
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,&d.project,commands,2,changes,2)==PT_EDIT_OK);
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=1000;o.frame_limit=1000000;
    before=live;calls=0;
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);count=calls;
    assert(first(song)==17*65536 && d.project.samples[0].pcm.data==original && !s.bytes);
    pt_sampler_invert_song_close(song);song=NULL;assert(live==before);
    for(i=1;i<=count;++i) {calls=0;fail_at=i;
        assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_MEMORY && !song && live==before);
    }
    fail_at=0;
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,1,&a,&song)==PT_RENDER_MEMORY && !song && live==before);
    d.project.samples[0].pcm.bits=24;
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_SAMPLE && !song && live==before);
    assert(d.project.samples[0].pcm.bits==24 && original[0]==17);d.project.samples[0].pcm.bits=8;
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);
    assert(first(song)==17*65536);pt_sampler_invert_song_stop(song);pt_sampler_invert_song_close(song);
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);assert(first(song)==34*65536);
    /* Defensive generation check, even for a caller missing the required stop. */
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
    assert(pt_sampler_invert_song_pull(song,256,&out,&done)==PT_RENDER_INVALID && !out && done);
    assert(pt_sampler_invert_song_pull(song,256,&out,&done)==PT_RENDER_INVALID);pt_sampler_invert_song_close(song);
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);
    assert(first(song)==68*65536);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    assert(pt_sampler_invert_song_pull(song,17,&out,&done)==PT_RENDER_INVALID && !out && done);
    pt_sampler_invert_song_close(song);
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);
    assert(first(song)==34*65536);pt_sampler_invert_song_close(song);
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK);
    /* Replaced tables must fail before any dangling table can be dereferenced. */
    for(i=0;i<4;++i) {
        struct pt_sample *samples=d.project.samples;struct pt_event *events=d.project.events;uint16_t *orders=d.project.orders;
        unsigned n=d.project.sample_count;
        assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);
        if(i==0)d.project.samples=NULL;else if(i==1)d.project.events=NULL;else if(i==2)d.project.orders=NULL;else d.project.sample_count=0;
        assert(pt_sampler_invert_song_pull(song,17,&out,&done)==PT_RENDER_INVALID && !out && done);
        pt_sampler_invert_song_close(song);d.project.samples=samples;d.project.events=events;d.project.orders=orders;d.project.sample_count=n;
    }
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);assert(first(song)==68*65536);
    for(i=0,done=0;!done && i<10000;++i)assert(pt_sampler_invert_song_pull(song,256,&out,&done)==PT_RENDER_OK);
    assert(done);pt_sampler_invert_song_close(song);
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);
    pt_sampler_invert_song_stop(song);pt_pattern_history_release(&h);pt_sampler_release(&s);pt_document_release(&d);
    assert(!s.bytes && pt_sampler_invert_song_pull(song,1,&out,&done)==PT_RENDER_OK && done && !out);
    pt_sampler_invert_song_close(song);assert(!live && original[0]==17 && original[1]==-93);
    puts("SAMPLER INVERT SONG PASS: private bank, immutable masters, allocation/budget/precision refusal, stop/edit/restart, generation/table guards and owner cleanup");return 0;
}
