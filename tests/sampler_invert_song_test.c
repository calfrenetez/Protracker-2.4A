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
static unsigned live,calls,fail_at,capture;
static void *observed;static size_t observed_bytes;
static void *alloc(void *c,size_t n) {void *p;(void)c;if(++calls==fail_at)return NULL;p=malloc(n);if(p) {++live;if(capture) {observed=p;observed_bytes=n;capture=0;}}return p;}
static void drop(void *c,void *p) {(void)c;if(p){assert(live);--live;free(p);}}
static int32_t first(struct pt_sampler_invert_song *song)
{
    const struct pt_pcm *out;unsigned done,i;
    for(i=0;i<100;++i) {assert(pt_sampler_invert_song_pull(song,1,&out,&done)==PT_RENDER_OK && !done);if(out)return out->data[0];}
    assert(0);return 0;
}
/* The8192-frame bank needs eight4096-byte copy slices; the selected pattern
 * needs more than256 measured ticks. The64-step guard bounds this fixture's
 * preparation only, independent of its existing audio/runtime guards. */
static unsigned prepared(struct pt_sampler_invert_song *song)
{
    unsigned ready=0,steps=0;
    while(!ready && steps<64) {assert(pt_sampler_invert_song_prepare(song,&ready)==PT_RENDER_OK);++steps;}
    assert(ready && steps<64);return steps;
}
static void change_header(struct pt_project *p,unsigned mode)
{
    switch(mode) {
    case 0:p->title[0]^=1;break;
    case 1:p->channels.track[0].pan^=1;break;
    case 2:p->channels.count=1;break;
    case 3:p->order_count=0;break;
    case 4:p->pattern_count=0;break;
    case 5:p->sample_count=0;break;
    case 6:++p->bpm;break;
    case 7:++p->speed;break;
    case 8:p->mode^=1;break;
    case 9:p->orders=NULL;break;
    case 10:p->events=NULL;break;
    case 11:p->samples=NULL;break;
    case 12:p->midi_input[0]^=1;break;
    case 13:p->midi_output[0][0]^=1;break;
    case 14:p->midi_flags^=1;break;
    case 15:p->extensions=(struct pt_extension *)p;break;
    case 16:++p->extension_count;break;
    default:assert(0);
    }
}
static void incremental(struct pt_sampler *sampler,struct pt_project *p,const struct pt_render_options *o,const struct pt_allocator *a)
{
    static int32_t master[8192];struct pt_sample sample=p->samples[0];struct pt_event end=p->events[4];
    struct pt_project snapshot;struct pt_sampler_invert_song *song=NULL,*sync=NULL;
    struct pt_render_options limited=*o;const struct pt_pcm *left,*right;
    unsigned i,j,steps,ready,done,other,before=live,total,phase,mode,generation=sampler->generation;
    for(i=0;i<8192;++i)master[i]=(int32_t)(i%127)-63;
    master[0]=17;p->samples[0].pcm=(struct pt_pcm){master,8192,8192,48000,1,8};p->samples[0].loop_end=8192;
    p->events[4].effect=0;p->events[63*4].effect=15;
    memcpy(&snapshot,p,sizeof(snapshot));calls=0;
    assert(pt_sampler_invert_song_begin(sampler,p,o,SIZE_MAX,a,&song)==PT_RENDER_OK);
    assert(pt_sampler_invert_song_pull(song,1,&left,&done)==PT_RENDER_OK && !left && !done);
    assert(pt_sampler_invert_song_prepare(song,NULL)==PT_RENDER_INVALID);
    steps=prepared(song)+1;total=calls;
    assert(steps>8 && !sampler->bytes && master[0]==17 && p->samples[0].pcm.data==master);
    ready=0;i=calls;assert(pt_sampler_invert_song_prepare(song,&ready)==PT_RENDER_OK && ready && calls==i);
    pt_sampler_invert_song_close(song);assert(live==before);
    /* Compare the complete six-tick output, including EFx mutations, without
     * turning the longer measurement fixture into a native timing workload. */
    p->events[4].effect=15;p->events[63*4].effect=0;
    assert(pt_sampler_invert_song_begin(sampler,p,o,SIZE_MAX,a,&song)==PT_RENDER_OK);prepared(song);
    assert(pt_sampler_invert_song_open(sampler,p,o,SIZE_MAX,a,&sync)==PT_RENDER_OK);
    for(i=0;i<10000;++i) {
        assert(pt_sampler_invert_song_pull(song,256,&left,&done)==PT_RENDER_OK);
        assert(pt_sampler_invert_song_pull(sync,256,&right,&other)==PT_RENDER_OK);
        assert(done==other && (!!left)==(!!right));
        if(left) {assert(left->frames==right->frames && left->frames<=256);assert(!memcmp(left->data,right->data,left->frames*2*sizeof(*left->data)));}
        if(done)break;
    }
    assert(i<10000 && master[0]==17 && master[1]==-62 && !sampler->bytes);
    pt_sampler_invert_song_close(song);pt_sampler_invert_song_close(sync);assert(live==before);
    p->events[4].effect=0;p->events[63*4].effect=15;
    /* Cancel every pending phase, including mid-copy and a yielded measurement. */
    for(phase=0;phase<steps;++phase) {
        assert(pt_sampler_invert_song_begin(sampler,p,o,SIZE_MAX,a,&song)==PT_RENDER_OK);
        for(j=0;j<phase;++j) {ready=9;assert(pt_sampler_invert_song_prepare(song,&ready)==PT_RENDER_OK && !ready);}
        pt_sampler_invert_song_stop(song);assert(live==before+1);
        assert(pt_sampler_invert_song_pull(song,1,&left,&done)==PT_RENDER_OK && !left && done);
        ready=9;assert(pt_sampler_invert_song_prepare(song,&ready)==PT_RENDER_INVALID && ready==9);
        pt_sampler_invert_song_close(song);assert(live==before && master[0]==17 && !sampler->bytes);
    }
    /* Fail every allocation observed over successful begin/prepare. */
    for(i=1;i<=total;++i) {
        enum pt_render_result result;struct pt_sampler_invert_song *sentinel=(struct pt_sampler_invert_song *)p;
        calls=0;fail_at=i;song=sentinel;result=pt_sampler_invert_song_begin(sampler,p,o,SIZE_MAX,a,&song);
        if(result==PT_RENDER_OK) {
            for(j=0,ready=0;j<64 && result==PT_RENDER_OK && !ready;++j)result=pt_sampler_invert_song_prepare(song,&ready);
            assert(result==PT_RENDER_MEMORY && !ready && live==before+1);
            assert(pt_sampler_invert_song_prepare(song,&ready)==PT_RENDER_MEMORY && !ready);
            left=(const struct pt_pcm *)1;done=77;
            assert(pt_sampler_invert_song_pull(song,1,&left,&done)==PT_RENDER_MEMORY && left==(const struct pt_pcm *)1 && done==77);
            pt_sampler_invert_song_close(song);
        } else assert(result==PT_RENDER_MEMORY && song==sentinel);
        assert(live==before);
    }
    fail_at=0;
    /* External begin/open publication must not overwrite aligned source storage. */
    for(phase=0;phase<2;++phase) {
        enum pt_render_result result;
        struct pt_sampler_invert_song **aliased=(struct pt_sampler_invert_song **)(void *)master;
        result=phase?pt_sampler_invert_song_open(sampler,p,o,SIZE_MAX,a,aliased):
            pt_sampler_invert_song_begin(sampler,p,o,SIZE_MAX,a,aliased);
        assert(result==PT_RENDER_INVALID && live==before && master[0]==17 && master[1]==-62);
    }
    /* Readiness aliases are refused before writing or poisoning usable state. */
    for(phase=0;phase<2;++phase) {
        unsigned char *image;unsigned allocations;struct pt_sampler saved_sampler;
        capture=1;assert(pt_sampler_invert_song_begin(sampler,p,o,SIZE_MAX,a,&song)==PT_RENDER_OK);
        assert(observed==song && observed_bytes>=sizeof(unsigned));if(phase)prepared(song);
        image=malloc(observed_bytes);assert(image);memcpy(image,observed,observed_bytes);allocations=live;
        assert(pt_sampler_invert_song_prepare(song,(unsigned *)observed)==PT_RENDER_INVALID);
        assert(!memcmp(image,observed,observed_bytes) && live==allocations);free(image);
        assert(pt_sampler_invert_song_prepare(song,(unsigned *)(void *)master)==PT_RENDER_INVALID && master[0]==17 && master[1]==-62 && live==allocations);
        assert(pt_sampler_invert_song_prepare(song,&sampler->generation)==PT_RENDER_INVALID && sampler->generation==generation);
        assert(pt_sampler_invert_song_prepare(song,&p->midi_flags)==PT_RENDER_INVALID && p->midi_flags==snapshot.midi_flags);
        assert(pt_sampler_invert_song_prepare(song,&p->samples[0].pcm.frames)==PT_RENDER_INVALID && p->samples[0].pcm.frames==8192);
        prepared(song);pt_sampler_invert_song_stop(song);assert(live==before+1);
        memcpy(&saved_sampler,sampler,sizeof(saved_sampler));memset(sampler,0,sizeof(*sampler));
        p->samples=NULL;p->events=NULL;p->orders=NULL;ready=9;
        assert(pt_sampler_invert_song_prepare(song,&ready)==PT_RENDER_INVALID && ready==9 && live==before+1);
        image=malloc(observed_bytes);assert(image);memcpy(image,observed,observed_bytes);
        assert(pt_sampler_invert_song_prepare(song,(unsigned *)observed)==PT_RENDER_INVALID && !memcmp(image,observed,observed_bytes));free(image);
        memcpy(sampler,&saved_sampler,sizeof(*sampler));memcpy(p,&snapshot,sizeof(*p));
        pt_sampler_invert_song_close(song);assert(live==before);
    }
    /* Every fixed header field family is guarded both pending and ready. Pointed
     * arrays/PCM remain immutable by contract; these are same-header-object edits. */
    for(phase=0;phase<2;++phase)for(mode=0;mode<17;++mode) {
        assert(pt_sampler_invert_song_begin(sampler,p,o,SIZE_MAX,a,&song)==PT_RENDER_OK);
        if(phase)prepared(song);
        change_header(p,mode);ready=9;
        assert(pt_sampler_invert_song_prepare(song,&ready)==PT_RENDER_INVALID && ready==9 && live==before+1);
        left=(const struct pt_pcm *)1;done=77;
        assert(pt_sampler_invert_song_pull(song,1,&left,&done)==PT_RENDER_INVALID && left==(const struct pt_pcm *)1 && done==77);
        pt_sampler_invert_song_close(song);memcpy(p,&snapshot,sizeof(*p));assert(live==before);
    }
    for(phase=0;phase<2;++phase) {
        assert(pt_sampler_invert_song_begin(sampler,p,o,SIZE_MAX,a,&song)==PT_RENDER_OK);
        if(phase)prepared(song);
        ++sampler->generation;
        left=(const struct pt_pcm *)1;done=77;
        assert(pt_sampler_invert_song_pull(song,1,&left,&done)==PT_RENDER_INVALID && left==(const struct pt_pcm *)1 && done==77 && live==before+1);
        ready=9;assert(pt_sampler_invert_song_prepare(song,&ready)==PT_RENDER_INVALID && ready==9);
        pt_sampler_invert_song_close(song);sampler->generation=generation;assert(live==before);
    }
    assert(pt_sampler_invert_song_begin(sampler,p,o,SIZE_MAX,a,&song)==PT_RENDER_OK);
    p->channels.selected=1;prepared(song);assert(first(song)==17*65536);
    pt_sampler_invert_song_close(song);memcpy(p,&snapshot,sizeof(*p));assert(live==before);
    /* A late measurement refusal releases the copied bank before returning. */
    limited.tick_limit=1;assert(pt_sampler_invert_song_begin(sampler,p,&limited,SIZE_MAX,a,&song)==PT_RENDER_OK);
    for(i=0,ready=0;i<64;++i) {
        enum pt_render_result result=pt_sampler_invert_song_prepare(song,&ready);
        if(result!=PT_RENDER_OK) {assert(result==PT_RENDER_TICK_LIMIT && !ready);break;}
        assert(!ready);
    }
    assert(i<64 && live==before+1);
    left=(const struct pt_pcm *)1;done=77;
    assert(pt_sampler_invert_song_pull(song,1,&left,&done)==PT_RENDER_TICK_LIMIT && left==(const struct pt_pcm *)1 && done==77);
    {struct pt_sampler saved_sampler;memcpy(&saved_sampler,sampler,sizeof(saved_sampler));memset(sampler,0,sizeof(*sampler));
        p->samples=NULL;p->events=NULL;p->orders=NULL;ready=9;
        assert(pt_sampler_invert_song_prepare(song,&ready)==PT_RENDER_TICK_LIMIT && ready==9 && live==before+1);
        memcpy(sampler,&saved_sampler,sizeof(*sampler));memcpy(p,&snapshot,sizeof(*p));}
    pt_sampler_invert_song_close(song);assert(live==before);
    p->samples[0]=sample;p->events[4]=end;p->events[63*4].effect=0;
    assert(!sampler->bytes);
    for(i=0;i<8192;++i)assert(master[i]==(i?(int32_t)(i%127)-63:17));
}
#define PT_STUDIO_ALIAS_WITH_INVERT
#define PT_STUDIO_ALIAS_INVERT_ONLY
#include "studio_wrapper_alias_cases.h"
int main(void)
{
    struct pt_allocator a={NULL,alloc,drop};struct pt_document d;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[2];struct pt_event_change changes[2];
    struct pt_sampler_invert_song *song=NULL;struct pt_render_options o={0};
    int32_t original[]={17,-93,30,40};unsigned done,before,i,count;const struct pt_pcm *out;
    studio_alias_owner_cases(&a,1,0);assert(!live);
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){original,4,4,48000,1,8};
    d.project.samples[0].volume=64;d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=4;
    d.project.channels.track[0].pan=0;d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;d.project.events[0].instrument=1;
    d.project.events[0].effect=14;d.project.events[0].parameter=255;d.project.events[4].effect=15;
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,&d.project,commands,2,changes,2)==PT_EDIT_OK);
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=1000;o.frame_limit=1000000;
    incremental(&s,&d.project,&o,&a);
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
    out=(const struct pt_pcm *)1;done=77;
    assert(pt_sampler_invert_song_pull(song,256,&out,&done)==PT_RENDER_INVALID && out==(const struct pt_pcm *)1 && done==77);
    assert(pt_sampler_invert_song_pull(song,256,&out,&done)==PT_RENDER_INVALID);pt_sampler_invert_song_close(song);
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);
    assert(first(song)==68*65536);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    out=(const struct pt_pcm *)1;done=77;
    assert(pt_sampler_invert_song_pull(song,17,&out,&done)==PT_RENDER_INVALID && out==(const struct pt_pcm *)1 && done==77);
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
        out=(const struct pt_pcm *)1;done=77;
    assert(pt_sampler_invert_song_pull(song,17,&out,&done)==PT_RENDER_INVALID && out==(const struct pt_pcm *)1 && done==77);
        pt_sampler_invert_song_close(song);d.project.samples=samples;d.project.events=events;d.project.orders=orders;d.project.sample_count=n;
    }
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);assert(first(song)==68*65536);
    for(i=0,done=0;!done && i<10000;++i)assert(pt_sampler_invert_song_pull(song,256,&out,&done)==PT_RENDER_OK);
    assert(done);pt_sampler_invert_song_close(song);
    assert(pt_sampler_invert_song_open(&s,&d.project,&o,SIZE_MAX,&a,&song)==PT_RENDER_OK);
    pt_sampler_invert_song_stop(song);pt_pattern_history_release(&h);pt_sampler_release(&s);pt_document_release(&d);
    assert(!s.bytes && pt_sampler_invert_song_pull(song,1,&out,&done)==PT_RENDER_OK && done && !out);
    pt_sampler_invert_song_close(song);assert(!live && original[0]==17 && original[1]==-93);
    puts("SAMPLER INVERT SONG PASS: private bank, immutable masters, allocation/budget/precision refusal, stop/edit/restart, incremental parity/phase cancellation, generation/header guards and owner cleanup");return 0;
}
