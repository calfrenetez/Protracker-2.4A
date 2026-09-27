/* Native duration observations around unchanged production calls. Logical song
 * timestamps are injected so prepared dispatch can execute without a real bus. */
struct native_cost_clock {
    struct pt_native_eclock clock;uint64_t frame,sample_at,first,last;uint32_t frequency;unsigned callbacks;
};
static struct native_cost_clock *native_cost_active;
static uint64_t native_cost_tick(struct native_cost_clock *c)
{
    uint64_t ticks;uint32_t frequency;
    assert(pt_native_eclock_read(&c->clock,&ticks,&frequency));
    if(c->frequency)assert(frequency==c->frequency);else c->frequency=frequency;
    return ticks;
}
static int native_cost_read(void *p,uint64_t *ticks,uint32_t *frequency)
{
    struct native_cost_clock *c=p;c->sample_at=native_cost_tick(c);
    *ticks=c->frame;*frequency=48000;return 1;
}
static int native_cost_start(void *p,unsigned ch,const struct pt_amigus_voice_plan *plan)
{
    struct native_cost_clock *c=native_cost_active;uint64_t now=native_cost_tick(c);
    if(!c->callbacks)c->first=now;
    c->last=now;++c->callbacks;
    return dispatch_start(p,ch,plan);
}
static void native_wavetable_cost_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_wavetable_song *song=NULL;struct pt_wavetable_preflight_report report;
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct native_cost_clock clock={0};enum pt_wavetable_song_result r;
    uint64_t before,after,deadline;unsigned mode,i,guard,writes,allocs;
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};
    assert(f && bus && pt_native_eclock_open(&clock.clock));native_cost_active=&clock;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<2;++mode) {
        unsigned voices=mode?16:1;
        pt_document_init(&d,&a);assert(pt_document_new(&d,mode?16:4,SIZE_MAX)==PT_PROJECT_OK);
        d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
        d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
        for(i=0;i<voices;++i)d.project.events[i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
        d.project.events[d.project.channels.count].effect=15;o.tracks=mode?65535:1;
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        owner.api.start=native_cost_start;clock.frame=0;clock.callbacks=0;
        assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
        assert(pt_wavetable_song_clocked_begin(song,1000,native_cost_read,&clock)==PT_WAVETABLE_SONG_OK);
        guard=0;do{r=pt_wavetable_song_clocked_service(song,&deadline);assert(++guard<1000);}while(r==PT_WAVETABLE_SONG_WAITING);
        assert(r==PT_WAVETABLE_SONG_OK && pins(f) && !bus->starts);
        for(i=0;i<4;++i) {
            before=native_cost_tick(&clock);
            assert(pt_wavetable_song_clocked_service(song,&deadline)==PT_WAVETABLE_SONG_OK);
            after=native_cost_tick(&clock);
            assert(before<=clock.sample_at && clock.sample_at<=after && !clock.callbacks);
            printf("NATIVE COST ready voices=%u before_sample=%lu after_sample=%lu total=%lu frequency=%lu case=%u\n",
                voices,(unsigned long)(clock.sample_at-before),(unsigned long)(after-clock.sample_at),
                (unsigned long)(after-before),(unsigned long)clock.frequency,i);
        }
        clock.frame=1000;writes=f->writes;allocs=native_allocations;before=native_cost_tick(&clock);
        assert(pt_wavetable_song_clocked_service(song,&deadline)==PT_WAVETABLE_SONG_WAITING);
        after=native_cost_tick(&clock);
        assert(clock.callbacks==voices && bus->starts==voices && writes==f->writes && allocs==native_allocations);
        assert(before<=clock.sample_at && clock.sample_at<=clock.first && clock.first<=clock.last && clock.last<=after);
        printf("NATIVE COST start voices=%u before_sample=%lu sample_to_first=%lu first_to_last=%lu last_to_return=%lu total=%lu frequency=%lu\n",
            voices,(unsigned long)(clock.sample_at-before),(unsigned long)(clock.first-clock.sample_at),
            (unsigned long)(clock.last-clock.first),(unsigned long)(after-clock.last),(unsigned long)(after-before),(unsigned long)clock.frequency);
        assert(pt_wavetable_song_close(&song) && !pins(f));assert(pt_amigus_reservation_close(&f->reservation));
        pt_sampler_release(&sampler);d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};pt_document_release(&d);
    }
    native_cost_active=NULL;pt_native_eclock_close(&clock.clock);free(bus);free(f);
    puts("NATIVE COST PASS: unchanged source guards, measured ready/one/16voice prepared commit, no allocation/upload at commit, exact lease cleanup; injected logical clock/voice bus, no audio or realtime qualification");
}
