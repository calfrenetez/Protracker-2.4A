#define main ownership_fixture_main
#include "paula_voices_test.c"
#undef main
#include "../src/editor/paula_song.h"
static void *no_allocate(void *c,size_t n) {(void)c;(void)n;return NULL;}
static enum pt_paula_song_result prepare_song(struct pt_paula_song *s,struct pt_paula_preflight_report *r)
{
    enum pt_paula_song_result result;unsigned n=0;
    do{result=pt_paula_song_prepare(s,r);assert(++n<100);}while(result==PT_PAULA_SONG_PREPARING);
    return result;
}
static void song_fixture(unsigned bits)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;struct pt_sampler sampler;
    struct pt_sampler_paula cache={0};struct pt_paula_voices owner={0};struct driver d={0};
    struct pt_paula_voice_api api={&d,start,stop,control};struct pt_paula_render_caps caps={3546895,124,65535};
    struct pt_render_options o={0},saved_options;struct pt_render_report measured;
    struct pt_paula_preflight_report report;struct pt_paula_song *song=NULL,*unchanged=(void *)(uintptr_t)1;
    struct pt_render_interval span,before_span={17,18,19};struct pt_render_plan direct={0};struct pt_paula_batch batch;
    int32_t pcm[16],stereo[32];unsigned i,stops,starts;uint64_t frames=0;size_t pinned;
    struct pt_paula_voice_request request={0,16,428,64};enum pt_paula_song_result result;
    for(i=0;i<16;++i){pcm[i]=(int32_t)i+1;stereo[i*2]=stereo[i*2+1]=(int32_t)i+1;}
    d.start_result=d.control_result=d.quiesce_result=1;for(i=0;i<4;++i)d.stop_result[i]=1;
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
    doc.project.channels.track[4].route=doc.project.channels.track[7].route=PT_PAULA;
    doc.project.channels.track[4].pan=0;doc.project.channels.track[7].pan=255;
    doc.project.samples[0].pcm=doc.project.samples[2].pcm=(struct pt_pcm){pcm,16,16,8000,1,(uint8_t)bits};
    doc.project.samples[1].pcm=(struct pt_pcm){stereo,32,16,8000,2,(uint8_t)bits};
    doc.project.samples[0].volume=doc.project.samples[2].volume=64;
    doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    doc.project.events[16*2+7]=(struct pt_event){428,0,PT_NOTE_PERIOD,3,0,0,0,0};
    doc.project.events[16+15].effect=15;doc.project.events[16+15].parameter=150;
    doc.project.events[16*3+15].effect=15;doc.project.events[16*3+15].parameter=0;
    o.rate=48000;o.bits=24;o.tracks=(1U<<4)|(1U<<7);o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    assert(pt_render_measure(&doc.project,&o,NULL,NULL,&measured)==PT_RENDER_OK);saved_options=o;
    pt_sampler_init(&sampler,&a,1024*1024);
#define BIND() do{assert(pt_sampler_paula_bind(&cache,&sampler,&doc.project,&d,chip_alloc,chip_free,32));assert(pt_paula_voices_bind(&owner,&cache,&api));assert(pt_paula_voices_bind_quiesce(&owner,quiesce,&d));}while(0)
    BIND();{
        struct pt_allocator refused={NULL,no_allocate,fast_free};
        assert(pt_paula_song_begin(&owner,&o,&caps,&refused,&unchanged)==PT_PAULA_SONG_MEMORY);
        assert(unchanged==(void *)(uintptr_t)1 && !owner.song_owner && !d.starts);
    }o.tracks|=1U<<15;
    assert(pt_paula_song_begin(&owner,&o,&caps,&a,&unchanged)==PT_PAULA_SONG_CAPABILITY && unchanged==(void *)(uintptr_t)1);
    o=saved_options;o.row_range=1;
    assert(pt_paula_song_begin(&owner,&o,&caps,&a,&unchanged)==PT_PAULA_SONG_CAPABILITY);
    o=saved_options;assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
    assert(!d.starts && !sampler.bytes);
    assert(pt_paula_voices_sync(&owner)==-2 && !pt_paula_voices_close(&owner));
    assert(pt_paula_voices_stop(&owner,4)==-1);
    assert(pt_paula_voices_trigger(&owner,4,0,0,&request)==PT_PAULA_VOICE_REFUSED);
    assert(!pt_paula_dispatch(&owner,cache.version,48000,&direct,&caps,&batch));
    span=before_span;assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_PREPARING && !memcmp(&span,&before_span,sizeof(span)));
    o.tracks=0; /* Copied settings remain stable. */
    assert(prepare_song(song,&report)==PT_PAULA_SONG_OK && !d.starts);
    assert(sampler.current[0] && sampler.current[2] && !sampler.current[1]);
    assert(report.frames==measured.frames && report.samples[0] && report.samples[2] && !report.samples[1]);
    doc.project.channels.selected=15;
    do {
        assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_OK);frames+=span.frames;
        assert(pt_paula_song_next(song,&before_span)==PT_PAULA_SONG_INVALID);
        assert(pt_paula_song_consume(song,0)==PT_PAULA_SONG_INVALID);
        if(span.frames)assert(pt_paula_song_complete(song)==PT_PAULA_SONG_INVALID);
        while(span.frames){uint32_t n=span.frames>256?256:span.frames;assert(pt_paula_song_consume(song,n)==PT_PAULA_SONG_OK);span.frames-=n;}
        result=pt_paula_song_complete(song);
        assert(result==PT_PAULA_SONG_OK || result==PT_PAULA_SONG_DONE);
    }while(result!=PT_PAULA_SONG_DONE);
    assert(frames==measured.frames && d.starts==2 && d.controls>0);
    assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_DONE);
    pinned=sampler.bytes;d.quiesce_result=0;
    assert(!pt_paula_song_close(&song) && song && sampler.bytes==pinned);
    d.quiesce_result=1;assert(pt_paula_song_close(&song) && !song && !d.live);
    for(i=0;i<16;++i)assert(doc.project.samples[0].pcm.data[i]==pcm[i]);
    /* A late stereo note fails before ANY new master promotion or callback. */
    pt_sampler_release(&sampler);
    doc.project.samples[0].pcm=doc.project.samples[2].pcm=(struct pt_pcm){pcm,16,16,8000,1,(uint8_t)bits};
    pt_sampler_init(&sampler,&a,1024*1024);
    BIND();o=saved_options;doc.project.events[16*2+7].instrument=2;starts=d.starts;
    assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
    assert(prepare_song(song,&report)==PT_PAULA_SONG_CAPABILITY && !sampler.bytes && d.starts==starts);
    assert(pt_paula_song_close(&song));doc.project.events[16*2+7].instrument=3;
    /* Capacity failure after whole analysis retains a closeable handle. */
    BIND();sampler.budget=1;
    assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
    assert(prepare_song(song,&report)==PT_PAULA_SONG_MEMORY && d.starts==starts);
    assert(pt_paula_song_close(&song));sampler.budget=1024*1024;
    BIND();assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
    assert(prepare_song(song,&report)==PT_PAULA_SONG_OK);
    for(i=0;!d.reading[0];++i) {
        assert(i<20 && pt_paula_song_next(song,&span)==PT_PAULA_SONG_OK);
        while(span.frames){uint32_t n=span.frames>256?256:span.frames;assert(pt_paula_song_consume(song,n)==PT_PAULA_SONG_OK);span.frames-=n;}
        assert(pt_paula_song_complete(song)==PT_PAULA_SONG_OK);
    }
    stops=d.stops;pinned=sampler.bytes;d.stop_result[0]=0;doc.project.channels.track[4].pan=1;
    assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_STALE && d.stops==stops+1 && d.reading[0]);
    assert(!pt_paula_song_close(&song) && sampler.bytes==pinned);
    d.stop_result[0]=1;doc.project.channels.track[4].pan=0;assert(pt_paula_song_close(&song));
    /* Cancel while a promotion job owns unpublished storage. */
    pt_sampler_release(&sampler);
    doc.project.samples[0].pcm=doc.project.samples[2].pcm=(struct pt_pcm){pcm,16,16,8000,1,(uint8_t)bits};
    pt_sampler_init(&sampler,&a,1024*1024);BIND();
    assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
    assert(pt_paula_song_prepare(song,&report)==PT_PAULA_SONG_PREPARING);
    assert(pt_paula_song_prepare(song,&report)==PT_PAULA_SONG_PREPARING && sampler.bytes);
    pinned=sampler.bytes;d.quiesce_result=0;assert(!pt_paula_song_close(&song) && sampler.bytes==pinned);
    d.quiesce_result=1;assert(pt_paula_song_close(&song) && !sampler.bytes);
    BIND();assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
    assert(prepare_song(song,&report)==PT_PAULA_SONG_OK);
    d.start_result=0;d.stop_result[0]=0;stops=d.stops;
    for(i=0;i<20;++i) {
        assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_OK);
        while(span.frames){uint32_t n=span.frames>256?256:span.frames;assert(pt_paula_song_consume(song,n)==PT_PAULA_SONG_OK);span.frames-=n;}
        result=pt_paula_song_complete(song);if(result==PT_PAULA_SONG_DEVICE)break;
        assert(result==PT_PAULA_SONG_OK);
    }
    assert(i<20 && d.reading[0] && d.stops==stops+1);
    pinned=sampler.bytes;stops=d.stops;assert(pt_paula_song_complete(song)==PT_PAULA_SONG_DEVICE && d.stops==stops);
    assert(!pt_paula_song_close(&song) && sampler.bytes==pinned);
    d.stop_result[0]=1;assert(pt_paula_song_close(&song) && !d.live);
    pt_sampler_release(&sampler);pt_document_release(&doc);
#undef BIND
}
int paula_song_fixture(void)
{song_fixture(8);song_fixture(16);song_fixture(24);puts("PAULA SONG PASS: shared sequence, selected master pins, late refusal and retained cleanup");return 0;}
#ifndef PT_TEST_SONG_EXEC
int main(void) {return paula_song_fixture();}
#endif
