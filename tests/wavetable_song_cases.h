#include "../src/editor/wavetable_song.h"
#ifdef PT_TEST_PROJECT_VALIDATION_COUNT
extern unsigned pt_test_project_validations;
#define VALIDATIONS_SAVE unsigned validations=pt_test_project_validations
#define VALIDATIONS_UNCHANGED assert(pt_test_project_validations==validations)
#else
#define VALIDATIONS_SAVE ((void)0)
#define VALIDATIONS_UNCHANGED ((void)0)
#endif

#ifdef PT_TEST_PCM_VALIDATION_COUNT
extern unsigned pt_test_pcm_validations;
#define PCM_SAVE unsigned pcm_validations=pt_test_pcm_validations
#define PCM_UNCHANGED assert(pt_test_pcm_validations==pcm_validations)
#else
#define PCM_SAVE ((void)0)
#define PCM_UNCHANGED ((void)0)
#endif
static void song_bind(struct fixture *f,struct pt_sampler_wavetable *bridge,
    struct pt_wavetable_voices *owner,struct dispatch_bus *bus,struct pt_sampler *sampler,struct pt_project *p)
{
    unsigned i;struct pt_wavetable_voice_api api={bus,dispatch_start,dispatch_stop,dispatch_control,NULL};
    init(f,PT_AMIGUS_WAVETABLE);assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,112,112,f,bus_owned,bus_write));
    memset(bus,0,sizeof(*bus));bus->f=f;bus->owner=owner;
    for(i=0;i<16;++i)bus->stop_result[i]=1;
    assert(pt_sampler_wavetable_bind(bridge,sampler,p,&f->cache));assert(pt_wavetable_voices_bind(owner,bridge,&api));
}
static enum pt_wavetable_song_result song_tick(struct pt_wavetable_song *s,uint64_t *frames)
{
    struct pt_render_interval span;uint32_t remaining;VALIDATIONS_SAVE;PCM_SAVE;
    enum pt_wavetable_song_result result=pt_wavetable_song_next(s,&span);
    VALIDATIONS_UNCHANGED;PCM_UNCHANGED;if(result)return result;
    remaining=span.frames;
    while(remaining){uint32_t n=remaining>17?17:remaining;
        assert(pt_wavetable_song_consume(s,n)==PT_WAVETABLE_SONG_OK);remaining-=n;*frames+=n;}
    result=pt_wavetable_song_complete(s);VALIDATIONS_UNCHANGED;PCM_UNCHANGED;return result;
}
static void song_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_wavetable_song *song=NULL,*sentinel=(struct pt_wavetable_song *)(uintptr_t)1;
    struct pt_wavetable_preflight_report report;struct pt_render_report measured;
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct pt_render_interval interval;enum pt_wavetable_song_result result;
    int32_t data[16]={257,-513,1025,-2049,17,31,47,63};unsigned i,baseline;
    uint8_t *saved;size_t size,used;uint64_t frames=0;
    assert(f && bus);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){data,16,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
    d.project.samples[1]=d.project.samples[0];d.project.speed=2;
    d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    d.project.events[4].effect=1;d.project.events[4].parameter=1;
    d.project.events[8].effect=15;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=100;o.frame_limit=100000;
    pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
    assert(pt_project_size(&d.project,&size)==PT_PROJECT_OK);saved=malloc(size);assert(saved);
    assert(pt_project_encode(&d.project,saved,size,&used)==PT_PROJECT_OK && used==size);
    /* The first successful song must be analysed before pin promotion or upload.
     * A late stereo trigger refuses without any sample/bus/voice allocation. */
    d.project.samples[1].pcm.channels=2;d.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    baseline=allocations;
    assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&sentinel)==PT_WAVETABLE_SONG_CAPABILITY);
    assert(sentinel==(struct pt_wavetable_song *)(uintptr_t)1 && !sampler.bytes && !f->writes && !bus->starts && allocations==baseline);
    d.project.samples[1].pcm.channels=1;memset(d.project.events+4,0,sizeof(*d.project.events));
    d.project.events[4].effect=1;d.project.events[4].parameter=1;
    o.row_range=1;assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_RANGE && !song);o.row_range=0;
    /* Refuse analysis/sequence/controller/master-pin allocations in turn.
     * All contexts share this allocator, so partial master promotion may remain
     * sampler-owned; release it before restoring the original borrowed samples. */
    for(i=1;i<=4;++i){struct preflight_alloc memory={0,i};struct pt_allocator failing={&memory,preflight_allocate,release_master};
        sampler.allocator=failing;
        assert(pt_wavetable_song_open(&owner,&o,&format,&failing,&report,&song)==PT_WAVETABLE_SONG_MEMORY && !song);
        assert(!bus->starts && !f->writes && owner.bridge==&bridge);
        pt_sampler_release(&sampler);sampler.allocator=a;
        d.project.samples[0].pcm.data=data;d.project.samples[0].pcm.capacity=16;
        assert(allocations==baseline);}
    assert(pt_render_measure(&d.project,&o,NULL,NULL,&measured)==PT_RENDER_OK);
    assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK && song);
    assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&sentinel)==PT_WAVETABLE_SONG_INVALID && sentinel==(struct pt_wavetable_song *)(uintptr_t)1);
    assert(report.samples[0] && !report.samples[1] && sampler.current[0] && !sampler.current[1]);
    assert(!bus->starts && !f->writes);exact_save(&d.project,saved,size);
    /* Invalid protocol never advances. Initial zero-frame span starts the note
     * only at complete; then consume reflects time elapsed externally. */
    assert(pt_wavetable_song_consume(song,1)==PT_WAVETABLE_SONG_INVALID);
    assert(pt_wavetable_song_complete(song)==PT_WAVETABLE_SONG_INVALID);
    assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_OK && !interval.frames);
    assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_INVALID);
    assert(pt_wavetable_song_complete(song)==PT_WAVETABLE_SONG_OK);
    while(!bus->starts)assert(song_tick(song,&frames)==PT_WAVETABLE_SONG_OK);
    assert(bus->starts==1 && pins(f)==1);
    assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_OK && interval.frames);
    assert(pt_wavetable_song_complete(song)==PT_WAVETABLE_SONG_INVALID);
    assert(pt_wavetable_song_consume(song,257)==PT_WAVETABLE_SONG_INVALID);
    {uint32_t n=interval.frames;while(n){uint32_t block=n>256?256:n;assert(pt_wavetable_song_consume(song,block)==PT_WAVETABLE_SONG_OK);frames+=block;n-=block;}}
    assert(pt_wavetable_song_complete(song)==PT_WAVETABLE_SONG_OK);
    bus->stop_result[0]=0;
    do{result=song_tick(song,&frames);}while(result==PT_WAVETABLE_SONG_OK);
    assert(result==PT_WAVETABLE_SONG_STOPPING && frames==measured.frames && pins(f)==1);
    assert(!pt_wavetable_song_close(&song) && song && !pt_amigus_reservation_close(&f->reservation));
    assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_STOPPING);
    bus->stop_result[0]=1;assert(pt_wavetable_song_close(&song) && !song && !pins(f));
    assert(pt_wavetable_song_close(&song) && pt_amigus_reservation_close(&f->reservation));exact_save(&d.project,saved,size);
    /* Confirmed natural end and an early close both detach without retaining
     * a device lease; ended sessions cannot restart. */
    song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
    o.include_lead_in=1;
    assert(pt_render_measure(&d.project,&o,NULL,NULL,&measured)==PT_RENDER_OK);
    assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
    frames=0;do{result=song_tick(song,&frames);}while(result==PT_WAVETABLE_SONG_OK);
    assert(result==PT_WAVETABLE_SONG_DONE && frames==measured.frames && !pins(f));
    assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_DONE);
    assert(pt_wavetable_song_close(&song) && pt_amigus_reservation_close(&f->reservation));o.include_lead_in=0;
    song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
    assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
    assert(pt_wavetable_song_close(&song) && !bus->starts && !f->writes && pt_amigus_reservation_close(&f->reservation));
    /* Caller option/format edits cannot change a running session. Sampler
     * generation changes poison it before another upload/control/start. */
    song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
    assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
    o.rate=44100;format.bits=8;frames=0;do{assert(song_tick(song,&frames)==PT_WAVETABLE_SONG_OK);}while(!bus->starts);
    assert(bus->plan[0].rate==0x10000000 && (bus->plan[0].control&1));
    ++sampler.generation;bus->stop_result[0]=0;baseline=bus->controls;
    assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_STALE && bus->controls==baseline && pins(f)==1);
    assert(!pt_wavetable_song_close(&song));bus->stop_result[0]=1;
    assert(pt_wavetable_song_close(&song) && pt_amigus_reservation_close(&f->reservation));
    o.rate=48000;format.bits=16;
    /* Failed start keeps lease/controller/source pin until explicit confirmation.
     * Drop sampler's current reference afterwards to prove the session owns a
     * separate source pin, even while stop remains unresolved. */
    song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
    assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
    bus->fail_start=1;bus->stop_result[0]=0;frames=0;
    do{result=song_tick(song,&frames);}while(result==PT_WAVETABLE_SONG_OK);
    assert(result==PT_WAVETABLE_SONG_DEVICE && pins(f)==1);
    assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_DEVICE);
    pt_sampler_release(&sampler);assert(sampler.bytes);
    assert(!pt_wavetable_song_close(&song));bus->stop_result[0]=1;
    assert(pt_wavetable_song_close(&song) && !sampler.bytes && pt_amigus_reservation_close(&f->reservation));
    /* Restore borrowed descriptor before document teardown/save; session close
     * did not modify user content and all owned allocations can now go away. */
    d.project.samples[0].pcm.data=data;d.project.samples[0].pcm.capacity=16;
    exact_save(&d.project,saved,size);free(saved);pt_document_release(&d);free(bus);free(f);assert(!allocations);
    puts("WAVETABLE SONG PASS: gated preflight, selective master pins, interval protocol, pending stop, stale and failed-start ownership; injected only");
}

static int range_restore(void *ctx,unsigned ch,const struct pt_amigus_restore_plan *p)
{
    struct dispatch_bus *b=ctx;assert(!b->active[ch] && b->owner->voice[ch].held);
    assert(pt_cache_data(&b->f->cache.cache,b->owner->voice[ch].lease));
    b->active[ch]=1;b->plan[ch]=p->bounds;b->cursor[ch]=p->cursor_q32;
    ++b->restores;return b->restores!=b->fail_restore;
}
static void range_song_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_wavetable_song *song=NULL;struct pt_wavetable_preflight_report report;
    struct pt_render_report measured;struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct pt_render_sequence *oracle=NULL;struct pt_render_interval span,expected;struct pt_render_plan plan;
    struct pt_render_snapshot snapshot;struct pt_sample empty;enum pt_wavetable_song_result result;
    int32_t data[16]={257,-513,1025,-2049,17,31,47,63};unsigned mode,first,baseline;
    uint8_t *saved;size_t size,used;uint64_t emitted;
    assert(f && bus);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.speed=1;d.project.bpm=131;
    d.project.samples[0].pcm=(struct pt_pcm){data,16,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
    d.project.samples[1].pcm=d.project.samples[0].pcm;d.project.samples[1].volume=64;
    d.project.events[0]=(struct pt_event){321,0,PT_NOTE_PERIOD,1,0,0,0,0};
    d.project.events[1]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0}; /* Finishes before range. */
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=3;o.tick_limit=100;o.frame_limit=100000;
    o.pattern_only=o.row_range=1;o.row_first=1;o.row_end=3;
    pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
    baseline=allocations;
    assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_RANGE && !song);
    owner.api.restore=range_restore;
    /* Even a late unsupported trigger refuses BEFORE pins/uploads/restore. */
    empty=d.project.samples[2];d.project.samples[2]=d.project.samples[0];d.project.samples[2].pcm.channels=2;
    d.project.events[8]=(struct pt_event){321,0,PT_NOTE_PERIOD,3,0,0,0,0};
    assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_CAPABILITY && !song);
    assert(allocations==baseline && !sampler.bytes && !f->writes && !bus->starts && !bus->restores);
    d.project.samples[2]=empty;memset(d.project.events+8,0,sizeof(*d.project.events));
    assert(pt_project_size(&d.project,&size)==PT_PROJECT_OK);saved=malloc(size);assert(saved);
    assert(pt_project_encode(&d.project,saved,size,&used)==PT_PROJECT_OK);
    /* Modes: normal range; single final interval; pending natural stop; uncertain
       restore; cancel before any output; range starts at first row; future trigger. */
    for(mode=0;mode<7;++mode) {
        if(mode){song_bind(f,&bridge,&owner,bus,&sampler,&d.project);owner.api.restore=range_restore;}
        o.row_first=mode==5?0:1;o.row_end=mode==1?2:3;
        if(mode==6)d.project.events[9]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
        assert(pt_render_measure(&d.project,&o,NULL,NULL,&measured)==PT_RENDER_OK);
        assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
        assert(report.samples[0] && (report.samples[1]==(mode==5 || mode==6)));
        assert(!f->writes && !bus->starts && !bus->restores);
        if(mode==4) {assert(pt_wavetable_song_close(&song));assert(!f->writes && !bus->restores);assert(pt_amigus_reservation_close(&f->reservation));continue;}
        if(mode==3){bus->fail_restore=1;bus->stop_result[0]=0;}
        assert(pt_render_sequence_open(&d.project,&o,&a,&oracle)==PT_RENDER_OK);first=1;emitted=0;
        do {
            uint32_t remaining;
            assert(pt_render_sequence_next(oracle,&expected)==PT_RENDER_OK);
            {VALIDATIONS_SAVE;PCM_SAVE;unsigned guard=0;
                span=(struct pt_render_interval){123,4,5};
                do {
                    result=pt_wavetable_song_next_step(song,&span);
                    if(result==PT_WAVETABLE_SONG_UPLOADING) {
                        assert(++guard<100 && span.frames==123 && span.emit==4 && span.end==5);
                        assert(!bus->restores && !bus->starts);
                        assert(pt_wavetable_song_consume(song,1)==PT_WAVETABLE_SONG_UPLOADING);
                        assert(pt_wavetable_song_complete_step(song)==PT_WAVETABLE_SONG_UPLOADING);
                    }
                }while(result==PT_WAVETABLE_SONG_UPLOADING);
                VALIDATIONS_UNCHANGED;PCM_UNCHANGED;}
            if(mode==3 && expected.emit) {
                assert(result==PT_WAVETABLE_SONG_DEVICE && pins(f)==1 && owner.voice[0].uncertain);
                break;
            }
            assert(result==PT_WAVETABLE_SONG_OK && span.frames==expected.frames && span.emit==expected.emit && span.end==expected.end);
            if(!span.emit)assert(!bus->restores && !bus->starts && !bus->controls && !f->writes);
            if(span.emit && first) {
                uint32_t address,bytes;uint64_t position;
                assert(pt_render_sequence_snapshot(oracle,&snapshot)==PT_RENDER_OK);
                assert(pt_sampler_wavetable_location(&bridge,owner.voice[0].lease,&address,&bytes));
                position=snapshot.voice[0].phase+((uint64_t)snapshot.voice[0].loop_start<<32);
                assert(bus->cursor[0]==((uint64_t)address<<32)+position*2);
                if(mode!=5)assert((uint32_t)position);
                assert(bus->restores==(mode==5?2U:1U) && !bus->starts);first=0;
                if(mode==2)bus->stop_result[0]=0;
            }
            remaining=span.frames;
            while(remaining) {uint32_t n=remaining>256?256:remaining;
                assert(pt_render_sequence_consume(oracle,n)==PT_RENDER_OK);
                {VALIDATIONS_SAVE;assert(pt_wavetable_song_consume(song,n)==PT_WAVETABLE_SONG_OK);VALIDATIONS_UNCHANGED;}
                if(span.emit)emitted+=n;
                remaining-=n;}
            assert(pt_render_sequence_complete(oracle,&plan)==PT_RENDER_OK);
            {VALIDATIONS_SAVE;PCM_SAVE;result=pt_wavetable_song_complete(song);VALIDATIONS_UNCHANGED;PCM_UNCHANGED;}
        }while(result==PT_WAVETABLE_SONG_OK);
        pt_render_sequence_close(oracle);oracle=NULL;
        if(mode==2 || mode==3) {
            assert(result==(mode==2?PT_WAVETABLE_SONG_STOPPING:PT_WAVETABLE_SONG_DEVICE));
            assert(!pt_wavetable_song_close(&song) && !pt_amigus_reservation_close(&f->reservation));
            bus->stop_result[0]=1;
        }else assert(result==PT_WAVETABLE_SONG_DONE);
        if(mode!=3)assert(emitted==measured.frames && !first);
        if(mode==6)assert(bus->starts==1);
        assert(pt_wavetable_song_close(&song) && !pins(f) && pt_amigus_reservation_close(&f->reservation));
        if(mode==6)memset(d.project.events+9,0,sizeof(*d.project.events));
        exact_save(&d.project,saved,size);
    }
    free(saved);pt_sampler_release(&sampler);pt_document_release(&d);free(bus);free(f);assert(!allocations);
    puts("WAVETABLE RANGE PASS: silent pre-roll, exact fractional restore before output, selective source pins, final-span timing and uncertain-stop ownership; injected only");
}

static void preparing_song_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct pt_wavetable_preflight_report report;struct pt_render_interval interval;
    struct pt_wavetable_song *song=NULL,*other=NULL;enum pt_wavetable_song_result result;
    int32_t data[16]={257,-513,1025,-2049,17,31,47,63};unsigned mode,baseline,steps;
    assert(f && bus);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){data,16,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
    d.project.speed=1;d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};d.project.events[12].effect=15;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    baseline=allocations;
    for(mode=0;mode<9;++mode) {
        pt_sampler_init(&sampler,&a,mode==6?0:1024*1024);
        if(mode==5)d.project.samples[0].pcm.channels=2;
        song_bind(f,&bridge,&owner,bus,&sampler,&d.project);owner.api.restore=range_restore;
        o.pattern_only=o.row_range=mode==8;o.row_first=1;o.row_end=3;
        assert(pt_wavetable_song_begin(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_PREPARING);
        assert(owner.song_owner==song && !sampler.bytes && allocations==baseline+3);
        assert(pt_wavetable_song_begin(&owner,&o,&format,&a,&report,&other)==PT_WAVETABLE_SONG_INVALID && !other);
        memset(&interval,0x5a,sizeof(interval));
        assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_PREPARING && interval.frames==0x5a5a5a5a);
        assert(pt_wavetable_song_consume(song,1)==PT_WAVETABLE_SONG_PREPARING);
        assert(pt_wavetable_song_complete(song)==PT_WAVETABLE_SONG_PREPARING);
        assert(pt_wavetable_song_prepare(song,NULL)==PT_WAVETABLE_SONG_INVALID);
        result=PT_WAVETABLE_SONG_PREPARING;steps=0;
        if(mode==3)++sampler.generation;
        if(mode==4)++d.project.bpm;
        while(mode && result==PT_WAVETABLE_SONG_PREPARING) {
            if(report.result==PT_WAVETABLE_PENDING)assert(!sampler.bytes);
            result=pt_wavetable_song_prepare(song,&report);++steps;assert(steps<1000);
            assert(!f->writes && !bus->starts && !bus->restores && !bus->controls && !bus->stops);
            if(mode==1 && report.result==PT_WAVETABLE_COMPATIBLE){assert(!sampler.bytes);break;}
            if(mode==2 && sampler.bytes)break;
        }
        if(mode==3 || mode==4)assert(result==PT_WAVETABLE_SONG_STALE);
        if(mode==5)assert(result==PT_WAVETABLE_SONG_CAPABILITY && !sampler.bytes);
        if(mode==6)assert(result==PT_WAVETABLE_SONG_MEMORY && !sampler.bytes);
        if(mode>=7) {
            uint64_t frames=0;
            assert(result==PT_WAVETABLE_SONG_OK && sampler.bytes);
            assert(pt_wavetable_song_prepare(song,&report)==PT_WAVETABLE_SONG_OK);
            do {result=song_tick(song,&frames);}while(result==PT_WAVETABLE_SONG_OK);
            assert(result==PT_WAVETABLE_SONG_DONE && (mode==8?bus->restores:bus->starts));
        }
        assert(pt_wavetable_song_close(&song) && !owner.song_owner && !pins(f));
        if(mode<7)assert(owner.bridge==&bridge && !bus->stops && !f->writes);
        assert(pt_wavetable_voices_close(&owner) && pt_amigus_reservation_close(&f->reservation));
        if(mode==4)--d.project.bpm;
        pt_sampler_release(&sampler);d.project.samples[0].pcm=(struct pt_pcm){data,16,8,48000,1,24};
        assert(allocations==baseline && data[0]==257 && data[1]==-513);
    }
    pt_document_release(&d);free(bus);free(f);assert(!allocations);
    puts("WAVETABLE PREPARING PASS: exclusive pending owner, no early pins/output, cancel/stale/late-refusal/promotion-failure cleanup, transferred song/range playback");
}

static void song_guard_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_amigus_wavetable_cache *replacement=malloc(sizeof(*replacement));struct pt_amigus_reservation reservation_copy;
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0},saved_bridge;struct pt_wavetable_voices owner={0};
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct pt_wavetable_preflight_report report;struct pt_render_interval interval;
    struct pt_wavetable_song *song=NULL;struct pt_project saved_project;enum pt_wavetable_song_result result;
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};unsigned mode,baseline,steps;
    assert(f && bus && replacement);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.speed=1;d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};d.project.events[12].effect=15;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    baseline=allocations;
    assert(!pt_amigus_wavetable_cache_current(NULL));
    for(mode=0;mode<18;++mode) {
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        /* Initial/untrusted PCM still goes through the full validator. */
        data[0]=8388608;
        assert(!pt_sampler_wavetable_sync(&bridge));
        assert(pt_wavetable_song_begin(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_STALE && !song);
        assert(!owner.song_owner && allocations==baseline && !f->writes);data[0]=257;
        assert(pt_wavetable_song_begin(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_PREPARING);
        {VALIDATIONS_SAVE;
            assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_PREPARING);
            assert(pt_wavetable_song_consume(song,1)==PT_WAVETABLE_SONG_PREPARING);
            assert(pt_wavetable_song_complete(song)==PT_WAVETABLE_SONG_PREPARING);
            VALIDATIONS_UNCHANGED;
        }
        saved_bridge=bridge;saved_project=d.project;*replacement=f->cache;reservation_copy=f->reservation;
        switch(mode) {
        case 1:owner.bridge=&saved_bridge;break;
        case 2:bridge.sampler=NULL;break;
        case 3:bridge.project=NULL;break;
        case 4:bridge.table=NULL;break;
        case 5:++bridge.count;break;
        case 6:++bridge.generation;break;
        case 7:++bridge.version;break;
        case 8:bridge.backend=replacement;break;
        case 9:f->cache.reservation=&reservation_copy;break;
        case 10:f->healthy=0;break;
        case 11:f->cache.closing=1;break;
        case 12:f->cache.faulted=1;break;
        case 13:f->reservation.reserved=0;break;
        case 14:f->reservation.access=0;break;
        case 15:f->reservation.resource=PT_AMIGUS_PCM;break;
        case 16:d.project.samples=NULL;break;
        case 17:++d.project.sample_count;break;
        }
        {VALIDATIONS_SAVE;
            result=pt_wavetable_song_prepare(song,&report);
            if(mode){VALIDATIONS_UNCHANGED;}
            assert(!f->writes && !bus->starts && !bus->stops && !bus->controls && !bus->restores);
        }
        if(mode) {
            assert(result==PT_WAVETABLE_SONG_STALE && !owner.song_owner && !sampler.bytes);
            assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_STALE);
            owner.bridge=&bridge;bridge=saved_bridge;d.project=saved_project;
            f->cache.reservation=&f->reservation;f->cache.closing=0;f->healthy=1;
            f->reservation.reserved=f->reservation.access=1;f->reservation.resource=PT_AMIGUS_WAVETABLE;
            if(mode==10 || (mode>=12 && mode<=15))assert(!pt_amigus_wavetable_cache_current(&f->cache));
        } else {
            steps=0;
            do{result=pt_wavetable_song_prepare(song,&report);assert(++steps<1000);}while(result==PT_WAVETABLE_SONG_PREPARING);
            assert(result==PT_WAVETABLE_SONG_OK && sampler.bytes);
            /* Promotion changes the descriptor, not the master content/revision.
             * Selection remains a harmless UI cursor. Ready guards and silent
             * consumption do not call the full project validator. */
            d.project.channels.selected=1;
            {VALIDATIONS_SAVE;
                assert(pt_wavetable_song_prepare(song,&report)==PT_WAVETABLE_SONG_OK);
                assert(pt_wavetable_song_next(song,&interval)==PT_WAVETABLE_SONG_OK);
                if(interval.frames)assert(pt_wavetable_song_consume(song,1)==PT_WAVETABLE_SONG_OK);
                VALIDATIONS_UNCHANGED;
            }
        }
        assert(pt_wavetable_song_close(&song));
        assert(pt_wavetable_voices_close(&owner) && pt_amigus_reservation_close(&f->reservation));
        assert(!pt_amigus_wavetable_cache_current(&f->cache));
        pt_sampler_release(&sampler);d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};
        assert(allocations==baseline && data[0]==257 && data[1]==-513);
    }
    pt_document_release(&d);free(replacement);free(bus);free(f);assert(!allocations);
    puts("WAVETABLE GUARD PASS: invalid initial PCM, immutable revision/identity guards, lost ownership latch, promotion and cancellation; injected only");
}
static void prepared_source_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct pt_wavetable_preflight_report report;struct pt_wavetable_song *song=NULL;
    struct pt_sample saved;struct pt_sample_version *token;enum pt_wavetable_song_result result;
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};unsigned mode,writes;uint64_t frames;
    assert(f && bus);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.speed=1;d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};
    d.project.samples[0].volume=64;d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
    d.project.events[0]=d.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    d.project.events[8].effect=15;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<5;++mode) {
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
        saved=d.project.samples[0];token=sampler.current[0];frames=0;
        switch(mode) {
        case 1:--d.project.samples[0].pcm.capacity;break;
        case 2:sampler.current[0]=NULL;break;
        case 3:d.project.samples[0].pcm.data=data;break;
        case 4:--d.project.samples[0].volume;break;
        }
        do {result=song_tick(song,&frames);}while(result==PT_WAVETABLE_SONG_OK && !bus->starts);
        if(mode)assert(result==(mode==1?PT_WAVETABLE_SONG_RENDER:PT_WAVETABLE_SONG_DEVICE) && !bus->starts && !f->writes && !pins(f));
        else {
            struct pt_sample unused=d.project.samples[1];struct pt_cache_lease lease={0};
            struct pt_render_plan empty={0};uint8_t staging[16];uint32_t address,bytes;
            assert(result==PT_WAVETABLE_SONG_OK && bus->starts==1 && pins(f)==1);writes=f->writes;
            /* Public APIs still validate unused samples, even after private
             * playback. Restore adversarial input before resuming the owner. */
            d.project.samples[1]=saved;d.project.samples[1].pcm.bits=8;
            assert(pt_sampler_wavetable_acquire(&bridge,0,&format,staging,sizeof(staging),&lease)==PT_CACHE_INVALID);
            assert(!pt_sampler_wavetable_location(&bridge,owner.voice[0].lease,&address,&bytes));
            assert(!pt_wavetable_dispatch(&owner,bridge.version,o.rate,&empty,&format,staging,sizeof(staging)));
            d.project.samples[1]=unused;
            do{result=song_tick(song,&frames);}while(result==PT_WAVETABLE_SONG_OK);
            assert(result==PT_WAVETABLE_SONG_DONE && bus->starts==2 && f->writes==writes && !pins(f));
        }
        d.project.samples[0]=saved;sampler.current[0]=token;
        assert(pt_wavetable_song_close(&song) && pt_wavetable_voices_close(&owner) && pt_amigus_reservation_close(&f->reservation));
        assert(d.project.samples[0].pcm.data[0]==257 && d.project.samples[0].pcm.bits==24);
        pt_sampler_release(&sampler);d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};
    }
    pt_document_release(&d);free(bus);free(f);assert(!allocations);
    puts("WAVETABLE PREPARED SOURCES PASS: exact pin/descriptor refusal before upload, cache-hit retrigger, public validation retained; injected only");
}

static void large_preparing_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct pt_wavetable_preflight_report report;struct pt_wavetable_song *song=NULL;
    int32_t *data=malloc(2100*sizeof(int32_t));unsigned mode,i,steps,baseline;enum pt_wavetable_song_result result;
    assert(f && bus && data);for(i=0;i<2100;++i)data[i]=(int32_t)i*257-100000;
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);baseline=allocations;
    d.project.speed=1;d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};d.project.events[4].effect=15;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<3;++mode) {
        d.project.samples[0].pcm=(struct pt_pcm){data,2100,2100,48000,1,24};
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        assert(pt_wavetable_song_begin(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_PREPARING);
        steps=0;
        do{assert(pt_wavetable_song_prepare(song,&report)==PT_WAVETABLE_SONG_PREPARING);assert(++steps<1000);}while(!sampler.bytes);
        assert(!sampler.current[0] && d.project.samples[0].pcm.data==data);
        assert(pt_wavetable_song_prepare(song,&report)==PT_WAVETABLE_SONG_PREPARING);
        assert(!sampler.current[0] && d.project.samples[0].pcm.data==data); /* Only4KiB copied. */
        if(mode==1) {
            ++sampler.generation;
            assert(pt_wavetable_song_prepare(song,&report)==PT_WAVETABLE_SONG_STALE && !sampler.bytes);
        } else if(mode==2) {
            assert(pt_wavetable_song_prepare(song,&report)==PT_WAVETABLE_SONG_PREPARING && !sampler.current[0]);
            assert(pt_wavetable_song_prepare(song,&report)==PT_WAVETABLE_SONG_PREPARING && sampler.current[0]);
            assert(!memcmp(d.project.samples[0].pcm.data,data,8400) && d.project.samples[0].pcm.bits==24);
            result=pt_wavetable_song_prepare(song,&report);assert(result==PT_WAVETABLE_SONG_OK);
        }
        assert(!f->writes && !bus->starts && !bus->controls && !bus->restores && !bus->stops);
        assert(pt_wavetable_song_close(&song) && !owner.song_owner);
        if(mode<2)assert(!sampler.bytes && !sampler.current[0] && d.project.samples[0].pcm.data==data);
        assert(pt_wavetable_voices_close(&owner) && pt_amigus_reservation_close(&f->reservation));
        pt_sampler_release(&sampler);assert(!sampler.bytes && allocations==baseline);
    }
    pt_document_release(&d);free(data);free(bus);free(f);assert(!allocations);
    puts("LARGE PREPARING PASS: allocation separate from4KiB copies, no partial master publication/output, cancel/stale cleanup and intact24-bit ready master");
}

static int batch_start(void *context,unsigned ch,const struct pt_amigus_voice_plan *plan)
{
    struct dispatch_bus *b=context;
    assert(pins(b->f)==3); /* No first/partial start until the WHOLE batch is held. */
    return dispatch_start(context,ch,plan);
}
static void uploading_song_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct pt_wavetable_preflight_report report;struct pt_wavetable_song *song=NULL;
    struct pt_render_interval span,sentinel={123,4,5};enum pt_wavetable_song_result result;
    int32_t data[300];unsigned mode,i,step,before,baseline;uint64_t frames;
    assert(f && bus);for(i=0;i<300;++i)data[i]=(int32_t)i*257-30000;
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);baseline=allocations;
    d.project.speed=1;
    d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    d.project.events[1]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    d.project.events[2]=d.project.events[0];d.project.events[4].effect=15;
    o.rate=48000;o.bits=24;o.tracks=7;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<13;++mode) {
        if(mode>=8){d.project.events[4]=d.project.events[1];d.project.events[4].instrument=mode==8?2:3;d.project.events[8].effect=15;}
        for(i=0;i<(mode>=9?3U:2U);++i){d.project.samples[i].pcm=(struct pt_pcm){data,300,300,48000,1,24};d.project.samples[i].volume=64;}
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        assert(pt_amigus_wavetable_cache_detach(&f->cache));
        assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,4000,mode==5?600:4000,f,bus_owned,bus_write));
        owner.api.start=batch_start;
        assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
        assert(pt_wavetable_song_next_step(song,&span)==PT_WAVETABLE_SONG_OK && !span.frames);
        assert(pt_wavetable_song_complete_step(song)==PT_WAVETABLE_SONG_UPLOADING && !f->writes);
        if(mode==7){bus->fail_start=2;bus->stop_result[1]=0;}
        {VALIDATIONS_SAVE;PCM_SAVE;
        step=0;
        do {
            before=f->writes;span=sentinel;
            assert(pt_wavetable_song_consume(song,1)==PT_WAVETABLE_SONG_UPLOADING);
            assert(pt_wavetable_song_next_step(song,&span)==PT_WAVETABLE_SONG_UPLOADING && !memcmp(&span,&sentinel,sizeof(span)));
            assert(f->writes==before && !bus->starts && !bus->controls);
            result=pt_wavetable_song_complete_step(song);++step;
            /* Each256-byte upload uses64 address/data write pairs. */
            assert(f->writes-before<=128 && step<20);
            VALIDATIONS_UNCHANGED;PCM_UNCHANGED;
            if(result==PT_WAVETABLE_SONG_UPLOADING)assert(!bus->starts && !bus->restores && !bus->stops);
            if((mode==1 && step==1) || (mode==2 && step==2))break;
            if(mode==3 && step==2){++sampler.generation;result=pt_wavetable_song_complete_step(song);break;}
            if(mode==4 && step==4)f->fail=f->writes+1;
            if(mode==6 && step==8)--d.project.samples[0].pcm.capacity; /* Completed earlier source stale. */
            if(mode==0)d.project.channels.selected=step%4;
        }while(result==PT_WAVETABLE_SONG_UPLOADING);
        }
        if(mode==0 || mode>=8) {
            assert(result==PT_WAVETABLE_SONG_OK && step==10 && bus->starts==3 && pins(f)==3);
            assert(f->writes==600 && !memcmp(d.project.samples[0].pcm.data,data,sizeof(data)));
            if(mode==8) {
                assert(pt_wavetable_song_next_step(song,&span)==PT_WAVETABLE_SONG_OK && span.frames);
                while(span.frames){unsigned n=span.frames>256?256:span.frames;assert(pt_wavetable_song_consume(song,n)==PT_WAVETABLE_SONG_OK);span.frames-=n;}
                assert(pt_wavetable_song_complete_step(song)==PT_WAVETABLE_SONG_UPLOADING);
                assert(pt_wavetable_song_complete_step(song)==PT_WAVETABLE_SONG_UPLOADING && pins(f)==4);
                bus->stop_result[1]=0;
                assert(!pt_wavetable_song_close(&song) && song && pins(f)==1 && bus->starts==3 && f->writes==600);
                bus->stop_result[1]=1;
            }else if(mode>=9) {
                unsigned guard=0,writes;struct pt_render_interval unchanged={123,4,5};
                owner.api.start=dispatch_start;
                assert(pt_wavetable_song_next_step(song,&span)==PT_WAVETABLE_SONG_OK && span.frames>1);
                assert(pt_wavetable_song_consume(song,1)==PT_WAVETABLE_SONG_OK);--span.frames;
                {VALIDATIONS_SAVE;PCM_SAVE;
                do {
                    writes=f->writes;result=pt_wavetable_song_prefetch(song);assert(++guard<30 && f->writes-writes<=128);
                    assert(bus->starts==3 && !bus->restores);
                    VALIDATIONS_UNCHANGED;PCM_UNCHANGED;
                    if(result==PT_WAVETABLE_SONG_DEVICE)break;
                    assert(pt_wavetable_song_complete_step(song)==PT_WAVETABLE_SONG_INVALID); /* Still ahead of live time. */
                    if(mode==10 && f->writes>600)break;
                    if(mode==12 && f->writes>600)f->fail=f->writes+1;
                }while(result==PT_WAVETABLE_SONG_UPLOADING);
                }
                if(mode==10) {
                    bus->stop_result[1]=0;
                    assert(!pt_wavetable_song_close(&song) && pins(f)==1 && sampler.bytes);
                    bus->stop_result[1]=1;
                }else if(mode==12)assert(result==PT_WAVETABLE_SONG_DEVICE && !pins(f));
                else {
                    assert(result==PT_WAVETABLE_SONG_OK && pins(f)==4 && f->writes==900);
                    if(mode==11) {
                        --d.project.samples[2].pcm.capacity;
                        assert(pt_wavetable_song_prefetch(song)==PT_WAVETABLE_SONG_DEVICE && !pins(f) && bus->starts==3);
                    }else {
                        assert(pt_wavetable_song_next_step(song,&unchanged)==PT_WAVETABLE_SONG_UPLOADING && unchanged.frames==123);
                        writes=f->writes;assert(pt_wavetable_song_prefetch(song)==PT_WAVETABLE_SONG_OK && writes==f->writes);
                        while(span.frames){unsigned n=span.frames>256?256:span.frames;assert(pt_wavetable_song_consume(song,n)==PT_WAVETABLE_SONG_OK);span.frames-=n;}
                        assert(pt_wavetable_song_complete_step(song)==PT_WAVETABLE_SONG_OK && f->writes==writes && bus->starts==4);
                        frames=0;do{result=song_tick(song,&frames);}while(result==PT_WAVETABLE_SONG_OK);
                        assert(result==PT_WAVETABLE_SONG_DONE);
                    }
                }
            }else {
                frames=0;do{result=song_tick(song,&frames);}while(result==PT_WAVETABLE_SONG_OK);
                assert(result==PT_WAVETABLE_SONG_DONE);
            }
        }else if(mode==7) {
            assert(result==PT_WAVETABLE_SONG_DEVICE && pins(f)==1 && owner.voice[1].uncertain);
            assert(!pt_wavetable_song_close(&song) && song && sampler.bytes);
            bus->stop_result[1]=1;
        }else {
            assert(!bus->starts && !bus->restores && !bus->stops);
            if(mode>2)assert(result==(mode==3?PT_WAVETABLE_SONG_STALE:PT_WAVETABLE_SONG_DEVICE) && !pins(f));
        }
        assert(pt_wavetable_song_close(&song) && !song && !pins(f));
        assert(pt_amigus_reservation_close(&f->reservation));pt_sampler_release(&sampler);
        assert(!sampler.bytes && allocations==baseline);
    }
    pt_document_release(&d);free(bus);free(f);assert(!allocations);
    puts("SONG PREFETCH PASS: upcoming600-byte cache ready before elapsed interval, no early commit, write-free commit, stale/transfer/cancel with old voice ownership");
    puts("SONG UPLOADING PASS: whole-batch leases,256-byte steps, no early output/time, duplicate hits, cancellation/stale/capacity/transfer guards and uncertain stop");
}

#undef PCM_SAVE
#undef PCM_UNCHANGED

#undef VALIDATIONS_SAVE
#undef VALIDATIONS_UNCHANGED
