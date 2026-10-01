#define main ownership_fixture_main
#include "paula_voices_test.c"
#undef main
#include "../src/editor/paula_song.h"
struct test_clock {uint64_t ticks;uint32_t frequency;unsigned reads;int result;struct pt_project *mutate;};
static int clock_read(void *context,uint64_t *ticks,uint32_t *frequency)
{struct test_clock *c=context;++c->reads;*ticks=c->ticks;*frequency=c->frequency;if(c->mutate)c->mutate->title[0]^=1;return c->result;}
static enum pt_paula_song_result schedule_poll(struct pt_paula_song *song,struct test_clock *c,unsigned sampled,uint64_t now,uint64_t *deadline)
{
    unsigned reads=c->reads;enum pt_paula_song_result r;
    if(!sampled)return pt_paula_song_schedule_step(song,now,deadline);
    c->ticks=100+now*2;
    if(sampled==2) {
        uint64_t counter=17,queried;
        r=pt_paula_song_clocked_service_counter(song,&counter);
        if(r==PT_PAULA_SONG_OK || r==PT_PAULA_SONG_WAITING) {
            assert(pt_paula_song_clocked_deadline(song,&queried)==PT_PAULA_SONG_OK && queried==counter);
            assert(counter>=100 && !(counter&1));*deadline=(counter-100)/2;
        }else assert(counter==17);
    }else r=pt_paula_song_clocked_service(song,deadline);
    assert(c->reads==reads+1);return r;
}
static void *no_allocate(void *c,size_t n) {(void)c;(void)n;return NULL;}
static enum pt_paula_song_result prepare_song(struct pt_paula_song *s,struct pt_paula_preflight_report *r)
{
    enum pt_paula_song_result result;unsigned n=0;
    do{result=pt_paula_song_prepare(s,r);assert(++n<100);}while(result==PT_PAULA_SONG_PREPARING);
    return result;
}
static unsigned stage_song(struct pt_paula_song *song,struct driver *d)
{
    enum pt_paula_song_result result;unsigned n=0,starts=d->starts,stops=d->stops,controls=d->controls;size_t calls;
    do {
        result=pt_paula_song_stage(song);assert(++n<2000);
        assert(d->starts==starts && d->stops==stops && d->controls==controls);
        if(result==PT_PAULA_SONG_PREPARING) {
            calls=d->calls;assert(pt_paula_song_complete(song)==PT_PAULA_SONG_PREPARING && d->calls==calls);
        }
    }while(result==PT_PAULA_SONG_PREPARING);
    assert(result==PT_PAULA_SONG_OK);return n;
}
static void song_fixture(unsigned bits)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;struct pt_sampler sampler;
    struct pt_sampler_paula cache={0};struct pt_paula_voices owner={0};struct driver d={0};
    struct pt_paula_voice_api api={&d,start,stop,control};struct pt_paula_render_caps caps={3546895,124,65535};
    struct pt_render_options o={0},saved_options;struct pt_render_report measured;
    struct pt_paula_preflight_report report;struct pt_paula_song *song=NULL,*unchanged=(void *)(uintptr_t)1;
    struct pt_render_interval span,before_span={17,18,19};struct pt_render_plan direct={0};struct pt_paula_batch batch;
    int32_t pcm[1024],stereo[2048];unsigned i,stops,starts,polls,mode;uint64_t frames=0;size_t pinned,calls;
    struct pt_paula_voice_request request={0,16,428,64};enum pt_paula_song_result result;
    for(i=0;i<1024;++i){pcm[i]=(int32_t)(i%120)+1;stereo[i*2]=stereo[i*2+1]=(int32_t)(i%120)+1;}
    d.start_result=d.control_result=d.quiesce_result=1;for(i=0;i<4;++i)d.stop_result[i]=1;
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
    doc.project.channels.track[4].route=doc.project.channels.track[7].route=PT_PAULA;
    doc.project.channels.track[4].pan=0;doc.project.channels.track[7].pan=255;
    doc.project.samples[0].pcm=doc.project.samples[2].pcm=(struct pt_pcm){pcm,1024,1024,8000,1,(uint8_t)bits};
    doc.project.samples[1].pcm=(struct pt_pcm){stereo,2048,1024,8000,2,(uint8_t)bits};
    doc.project.samples[0].volume=doc.project.samples[2].volume=64;
    doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    doc.project.events[16*2+7]=(struct pt_event){428,0,PT_NOTE_PERIOD,3,0,0,0,0};
    doc.project.events[16+15].effect=15;doc.project.events[16+15].parameter=150;
    doc.project.events[16*3+15].effect=15;doc.project.events[16*3+15].parameter=0;
    o.rate=48000;o.bits=24;o.tracks=(1U<<4)|(1U<<7);o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    assert(pt_render_measure(&doc.project,&o,NULL,NULL,&measured)==PT_RENDER_OK);saved_options=o;
    pt_sampler_init(&sampler,&a,1024*1024);
#define BIND() do{assert(pt_sampler_paula_bind(&cache,&sampler,&doc.project,&d,chip_alloc,chip_free,2048));assert(pt_paula_voices_bind(&owner,&cache,&api));assert(pt_paula_voices_bind_quiesce(&owner,quiesce,&d));}while(0)
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
        if(span.frames && span.emit) {
            uint64_t now=1000,end=now+span.frames;
            starts=d.starts;stops=d.stops;
            assert(pt_paula_song_clock_arm(song,now)==PT_PAULA_SONG_OK);
            assert(pt_paula_song_clock_arm(song,now)==PT_PAULA_SONG_INVALID);
            assert(pt_paula_song_next(song,&before_span)==PT_PAULA_SONG_INVALID);
            assert(pt_paula_song_prepare(song,NULL)==PT_PAULA_SONG_INVALID);
            assert(pt_paula_song_consume(song,1)==PT_PAULA_SONG_INVALID);
            assert(pt_paula_song_prefetch(song)==PT_PAULA_SONG_INVALID);
            assert(pt_paula_song_stage(song)==PT_PAULA_SONG_INVALID);
            assert(pt_paula_song_complete(song)==PT_PAULA_SONG_INVALID);
            for(polls=0;polls<100;++polls)assert(pt_paula_song_clock_service(song,now)==PT_PAULA_SONG_WAITING);
            while(end-now>128) {
                now+=128;assert(pt_paula_song_clock_service(song,now)==PT_PAULA_SONG_WAITING);
                assert(d.starts==starts && d.stops==stops);
            }
            calls=d.calls;d.fail=1;result=pt_paula_song_clock_service(song,end);d.fail=0;
            assert(d.calls==calls && (result==PT_PAULA_SONG_OK || result==PT_PAULA_SONG_DONE));
            assert(pt_paula_song_clock_service(song,end)==PT_PAULA_SONG_INVALID);
            continue;
        }
        if(span.frames)assert(pt_paula_song_complete(song)==PT_PAULA_SONG_INVALID);
        /* Forecast at a partially consumed interval; ready must neither consume
         * the remaining live frames nor emit. Exercise all source depths. */
        if(span.frames>256) {
            assert(pt_paula_song_consume(song,256)==PT_PAULA_SONG_OK);span.frames-=256;
        }
        starts=d.starts;stops=d.stops;polls=0;
        do {
            result=pt_paula_song_prefetch(song);assert(++polls<2000);
            assert(d.starts==starts && d.stops==stops);
            if(polls==1 && span.frames>17) {
                assert(pt_paula_song_consume(song,17)==PT_PAULA_SONG_OK);span.frames-=17;
            }
            if(span.frames)assert(pt_paula_song_complete(song)==PT_PAULA_SONG_INVALID);
            else if(result==PT_PAULA_SONG_PREPARING)
                assert(pt_paula_song_complete(song)==PT_PAULA_SONG_PREPARING);
        }while(result==PT_PAULA_SONG_PREPARING);
        assert(result==PT_PAULA_SONG_OK);
        calls=d.calls;assert(pt_paula_song_prefetch(song)==PT_PAULA_SONG_OK && d.calls==calls);
        while(span.frames){uint32_t n=span.frames>256?256:span.frames;assert(pt_paula_song_consume(song,n)==PT_PAULA_SONG_OK);span.frames-=n;}
        starts=d.starts;stops=d.stops;
        stage_song(song,&d);
        calls=d.calls;assert(pt_paula_song_stage(song)==PT_PAULA_SONG_OK && d.calls==calls);
        assert(d.starts==starts && d.stops==stops);
        d.fail=1;result=pt_paula_song_complete(song);d.fail=0;assert(d.calls==calls);
        assert(result==PT_PAULA_SONG_OK || result==PT_PAULA_SONG_DONE);
    }while(result!=PT_PAULA_SONG_DONE);
    assert(frames==measured.frames && d.starts==2 && d.controls>0);
    assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_DONE);
    pinned=sampler.bytes;d.quiesce_result=0;
    assert(!pt_paula_song_close(&song) && song && sampler.bytes==pinned);
    d.quiesce_result=1;assert(pt_paula_song_close(&song) && !song && !d.live);
    for(i=0;i<1024;++i)assert(doc.project.samples[0].pcm.data[i]==pcm[i]);
    /* A late stereo note fails before ANY new master promotion or callback. */
    pt_sampler_release(&sampler);
    doc.project.samples[0].pcm=doc.project.samples[2].pcm=(struct pt_pcm){pcm,1024,1024,8000,1,(uint8_t)bits};
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
    /* Public prefetch must validate even when its private body is shared with
     * an already validated schedule step. No device work may follow staleness. */
    BIND();assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
    assert(prepare_song(song,&report)==PT_PAULA_SONG_OK);
    assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_OK);
    calls=d.calls;starts=d.starts;doc.project.title[0]^=1;
    assert(pt_paula_song_prefetch(song)==PT_PAULA_SONG_STALE && d.calls==calls && d.starts==starts);
    doc.project.title[0]^=1;assert(pt_paula_song_close(&song));
    /* Cancel while a promotion job owns unpublished storage. */
    pt_sampler_release(&sampler);
    doc.project.samples[0].pcm=doc.project.samples[2].pcm=(struct pt_pcm){pcm,1024,1024,8000,1,(uint8_t)bits};
    pt_sampler_init(&sampler,&a,1024*1024);BIND();
    assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
    assert(pt_paula_song_prepare(song,&report)==PT_PAULA_SONG_PREPARING);
    assert(pt_paula_song_prepare(song,&report)==PT_PAULA_SONG_PREPARING && sampler.bytes);
    pinned=sampler.bytes;d.quiesce_result=0;assert(!pt_paula_song_close(&song) && sampler.bytes==pinned);
    d.quiesce_result=1;assert(pt_paula_song_close(&song) && !sampler.bytes);
    BIND();assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
    assert(prepare_song(song,&report)==PT_PAULA_SONG_OK);
    /* A staged but unstarted candidate cancels before callbacks; context
     * quiescence still retains the session/master pins until confirmed. */
    starts=d.starts;calls=d.calls;
    for(i=0;i<20;++i) {
        assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_OK);
        polls=0;
        do{result=pt_paula_song_prefetch(song);assert(++polls<2000 && d.starts==starts);}while(result==PT_PAULA_SONG_PREPARING && d.calls==calls);
        if(d.calls==calls)while(span.frames){uint32_t n=span.frames>256?256:span.frames;assert(pt_paula_song_consume(song,n)==PT_PAULA_SONG_OK);span.frames-=n;}
        if(d.calls>calls)break;
        assert(result==PT_PAULA_SONG_OK && pt_paula_song_complete(song)==PT_PAULA_SONG_OK && d.starts==starts);
    }
    assert(i<20 && d.calls>calls && result==PT_PAULA_SONG_PREPARING);
    assert(pt_paula_song_complete(song)==(span.frames?PT_PAULA_SONG_INVALID:PT_PAULA_SONG_PREPARING));
    pinned=sampler.bytes;d.quiesce_result=0;
    assert(!pt_paula_song_close(&song) && sampler.bytes==pinned && d.starts==starts && !d.reading[0]);
    d.quiesce_result=1;assert(pt_paula_song_close(&song) && !d.live);
    /* Whole-song startup and each next interval share absolute phase. */
    for(mode=0;mode<14;++mode) {
        uint64_t now=1000,deadline=77,last,ticks;unsigned reads;
        struct test_clock clock={100,96000,0,1,NULL};unsigned sampled=mode==13?2:mode==0 || mode>=7;
        BIND();assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
        assert(prepare_song(song,&report)==PT_PAULA_SONG_OK);
        starts=d.starts;stops=d.stops;
        if(mode==4) {
            assert(pt_paula_song_schedule_begin(song,UINT64_MAX)==PT_PAULA_SONG_CLOCK);
            assert(d.starts==starts && pt_paula_song_close(&song));continue;
        }
        if(mode==11) {
            clock.frequency=1;clock.ticks=0;
            assert(pt_paula_song_clocked_begin(song,1000,clock_read,&clock)==PT_PAULA_SONG_OK);
            clock.ticks=UINT64_MAX;reads=clock.reads;
            assert(pt_paula_song_clocked_service(song,&deadline)==PT_PAULA_SONG_CLOCK && clock.reads==reads+1 && deadline==77);
            assert(pt_paula_song_close(&song));continue;
        }
        if(sampled) {
            assert(pt_paula_song_clocked_begin(song,1000,clock_read,&clock)==PT_PAULA_SONG_OK && clock.reads==1);
            assert(pt_paula_song_clocked_begin(song,1000,clock_read,&clock)==PT_PAULA_SONG_INVALID && clock.reads==1);
            assert(pt_paula_song_schedule_step(song,999,&deadline)==PT_PAULA_SONG_INVALID && deadline==77);
            assert(pt_paula_song_clocked_service(song,NULL)==PT_PAULA_SONG_INVALID && clock.reads==1);
            assert(pt_paula_song_clocked_service_counter(song,NULL)==PT_PAULA_SONG_INVALID && clock.reads==1);
            assert(pt_paula_song_clocked_deadline(song,&ticks)==PT_PAULA_SONG_OK && ticks==2100 && clock.reads==1);
            /* Single ticks carry half a frame rather than rounding per poll. */
            clock.ticks=101;assert(pt_paula_song_clocked_service(song,&deadline)==PT_PAULA_SONG_WAITING);
            assert(pt_paula_song_clocked_deadline(song,&ticks)==PT_PAULA_SONG_OK && ticks==2100);
            clock.ticks=102;assert(pt_paula_song_clocked_service(song,&deadline)==PT_PAULA_SONG_WAITING);
            assert(pt_paula_song_clocked_deadline(song,&ticks)==PT_PAULA_SONG_OK && ticks==2100);
            if(mode==12) {
                /* First tick at/after deadline cannot be represented. */
                assert(pt_paula_song_close(&song));BIND();
                assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
                assert(prepare_song(song,&report)==PT_PAULA_SONG_OK);
                clock.ticks=UINT64_MAX-1;
                assert(pt_paula_song_clocked_begin(song,1000,clock_read,&clock)==PT_PAULA_SONG_OK);
                reads=clock.reads;ticks=17;
                assert(pt_paula_song_clocked_deadline(song,&ticks)==PT_PAULA_SONG_CLOCK && ticks==17 && clock.reads==reads);
                assert(pt_paula_song_close(&song));continue;
            }
        }else assert(pt_paula_song_schedule_begin(song,1000)==PT_PAULA_SONG_OK);
        assert(pt_paula_song_schedule_begin(song,1000)==PT_PAULA_SONG_INVALID);
        assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_INVALID);
        assert(pt_paula_song_prepare(song,NULL)==PT_PAULA_SONG_INVALID);
        assert(pt_paula_song_clock_arm(song,1000)==PT_PAULA_SONG_INVALID);
        assert(pt_paula_song_schedule_step(song,999,NULL)==PT_PAULA_SONG_INVALID && deadline==(sampled?1000:77));
        if(mode==1) {
            assert(pt_paula_song_schedule_step(song,1000,&deadline)==PT_PAULA_SONG_DEADLINE && deadline==77);
            assert(d.starts==starts && pt_paula_song_close(&song));continue;
        }
        polls=0;
        do {
            result=schedule_poll(song,&clock,sampled,999,&deadline);assert(++polls<2000);
            assert(deadline==1000 && d.starts==starts && d.stops==stops);
        }while(result==PT_PAULA_SONG_WAITING);
        assert(result==PT_PAULA_SONG_OK);
        calls=d.calls;assert(schedule_poll(song,&clock,sampled,999,&deadline)==PT_PAULA_SONG_OK && d.calls==calls);
        if(mode==2 || mode==3) {
            enum pt_paula_song_result expected=mode==2?PT_PAULA_SONG_DEADLINE:PT_PAULA_SONG_CLOCK;
            assert(pt_paula_song_schedule_step(song,mode==2?1001:998,&deadline)==expected && deadline==1000);
            assert(d.starts==starts && pt_paula_song_close(&song));continue;
        }
        if(mode==6) {
            pinned=sampler.bytes;d.quiesce_result=0;
            assert(!pt_paula_song_close(&song) && sampler.bytes==pinned && d.starts==starts);
            d.quiesce_result=1;assert(pt_paula_song_close(&song));continue;
        }
        d.fail=1;assert(schedule_poll(song,&clock,sampled,now,&deadline)==PT_PAULA_SONG_WAITING);d.fail=0;
        assert(d.calls==calls && d.reading[0] && deadline>now);
        if(mode>=7 && mode<13) {
            enum pt_paula_song_result expected=mode==10?PT_PAULA_SONG_DEADLINE:PT_PAULA_SONG_CLOCK;
            reads=clock.reads;stops=d.stops;starts=d.starts;pinned=sampler.bytes;last=deadline;d.stop_result[0]=0;
            if(mode==7)clock.result=0;
            else if(mode==8)++clock.frequency;
            else if(mode==9)--clock.ticks;
            else clock.ticks=100+(deadline+1)*2;
            assert(pt_paula_song_clocked_service(song,&deadline)==expected && clock.reads==reads+1 && deadline==last);
            assert(d.starts==starts && d.stops==stops+1 && d.reading[0]);reads=clock.reads;
            assert(pt_paula_song_clocked_service(song,&deadline)==expected && clock.reads==reads);
            assert(!pt_paula_song_close(&song) && sampler.bytes==pinned);
            d.stop_result[0]=1;assert(pt_paula_song_close(&song));continue;
        }
        if(mode==5) {
            d.stop_result[0]=0;pinned=sampler.bytes;starts=d.starts;stops=d.stops;last=deadline;
            assert(pt_paula_song_schedule_step(song,deadline+1,&deadline)==PT_PAULA_SONG_DEADLINE && deadline==last);
            assert(d.starts==starts && d.stops==stops+1 && d.reading[0]);
            assert(!pt_paula_song_close(&song) && sampler.bytes==pinned);
            d.stop_result[0]=1;assert(pt_paula_song_close(&song));continue;
        }
        for(i=0;i<100;++i) {
            last=deadline;
            for(polls=0;polls<100;++polls)assert(schedule_poll(song,&clock,sampled,now,&deadline)==PT_PAULA_SONG_WAITING && deadline==last);
            while(last-now>128){now+=128;assert(schedule_poll(song,&clock,sampled,now,&deadline)==PT_PAULA_SONG_WAITING && deadline==last);}
            now=last;calls=d.calls;d.fail=1;result=schedule_poll(song,&clock,sampled,now,&deadline);d.fail=0;
            assert(d.calls==calls);
            if(result==PT_PAULA_SONG_DONE){assert(deadline==last);break;}
            assert(result==PT_PAULA_SONG_WAITING && deadline>last);
        }
        assert(i<100 && now==1000+measured.frames);
        assert(pt_paula_song_close(&song) && !d.live);
    }
    /* Combined output is atomic on stale-before-read, callback mutation and
     * unrepresentable counter conversion. Neither path may emit output. */
    for(mode=0;mode<3;++mode) {
        struct test_clock clock={mode==2?UINT64_MAX-10:100,96000,0,1,NULL};
        uint64_t counter=17;unsigned reads;
        BIND();assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
        assert(prepare_song(song,&report)==PT_PAULA_SONG_OK);
        assert(pt_paula_song_clocked_begin(song,1000,clock_read,&clock)==PT_PAULA_SONG_OK);
        reads=clock.reads;starts=d.starts;
        if(mode==0)doc.project.title[0]^=1;
        if(mode==1)clock.mutate=&doc.project;
        result=pt_paula_song_clocked_service_counter(song,&counter);
        assert(result==(mode==2?PT_PAULA_SONG_CLOCK:PT_PAULA_SONG_STALE) && counter==17);
        assert(clock.reads==reads+(mode!=0) && d.starts==starts);
        reads=clock.reads;assert(pt_paula_song_clocked_service_counter(song,&counter)==result && clock.reads==reads && counter==17);
        if(mode<2)doc.project.title[0]^=1;
        assert(pt_paula_song_close(&song) && !d.live);
    }
    /* Clock failures with an active reader never catch up or retry output;
     * uncertain stop retains session/master ownership until explicit close. */
    for(mode=0;mode<5;++mode) {
        enum pt_paula_song_result expected=mode<2?PT_PAULA_SONG_CLOCK:PT_PAULA_SONG_DEADLINE;
        BIND();assert(pt_paula_song_begin(&owner,&o,&caps,&a,&song)==PT_PAULA_SONG_PREPARING);
        assert(prepare_song(song,&report)==PT_PAULA_SONG_OK);
        for(i=0;!d.reading[0];++i) {
            assert(i<20 && pt_paula_song_next(song,&span)==PT_PAULA_SONG_OK);
            while(span.frames){uint32_t n=span.frames>256?256:span.frames;assert(pt_paula_song_consume(song,n)==PT_PAULA_SONG_OK);span.frames-=n;}
            assert(pt_paula_song_complete(song)==PT_PAULA_SONG_OK);
        }
        assert(pt_paula_song_next(song,&span)==PT_PAULA_SONG_OK && span.emit && span.frames>256);
        starts=d.starts;stops=d.stops;pinned=sampler.bytes;d.stop_result[0]=0;
        if(mode==0)assert(pt_paula_song_clock_arm(song,UINT64_MAX)==expected);
        else {
            assert(pt_paula_song_clock_arm(song,1000)==PT_PAULA_SONG_OK);
            if(mode==4)for(polls=0;polls<100;++polls)assert(pt_paula_song_clock_service(song,1000)==PT_PAULA_SONG_WAITING);
            assert(pt_paula_song_clock_service(song,mode==1?999:1000+span.frames+(mode==2?1:0))==expected);
        }
        assert(d.starts==starts && d.stops==stops+1 && d.reading[0]);stops=d.stops;
        assert(pt_paula_song_clock_service(song,1000)==expected && d.stops==stops);
        assert(pt_paula_song_next(song,&span)==expected && d.starts==starts);
        assert(!pt_paula_song_close(&song) && sampler.bytes==pinned);
        d.stop_result[0]=1;assert(pt_paula_song_close(&song) && !d.live);
    }
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
