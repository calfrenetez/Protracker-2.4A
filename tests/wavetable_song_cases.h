#include "../src/editor/wavetable_song.h"
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
    struct pt_render_interval span;uint32_t remaining;
    enum pt_wavetable_song_result result=pt_wavetable_song_next(s,&span);
    if(result)return result;
    remaining=span.frames;
    while(remaining){uint32_t n=remaining>17?17:remaining;
        assert(pt_wavetable_song_consume(s,n)==PT_WAVETABLE_SONG_OK);remaining-=n;*frames+=n;}
    return pt_wavetable_song_complete(s);
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
    /* Refuse plan/sequence/controller/pin/second-sequence allocations in turn.
     * All contexts share this allocator, so partial master promotion may remain
     * sampler-owned; release it before restoring the original borrowed samples. */
    for(i=1;i<=5;++i){struct preflight_alloc memory={0,i};struct pt_allocator failing={&memory,preflight_allocate,release_master};
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
            result=pt_wavetable_song_next(song,&span);
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
                assert(pt_wavetable_song_consume(song,n)==PT_WAVETABLE_SONG_OK);
                if(span.emit)emitted+=n;
                remaining-=n;}
            assert(pt_render_sequence_complete(oracle,&plan)==PT_RENDER_OK);
            result=pt_wavetable_song_complete(song);
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
