struct sampled_clock {uint64_t ticks;uint32_t frequency;unsigned reads;int ok;};
static int sampled_read(void *p,uint64_t *ticks,uint32_t *frequency)
{
    struct sampled_clock *c=p;++c->reads;
    if(!c->ok)return 0;
    *ticks=c->ticks;*frequency=c->frequency;return 1;
}
static void clocked_song_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_wavetable_song *song=NULL;struct pt_wavetable_preflight_report report;
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct sampled_clock clock;enum pt_wavetable_song_result r;uint64_t deadline,now,tick_deadline;
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};unsigned mode,guard,i,reads,calls,writes;
    assert(f && bus);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;d.project.speed=3;d.project.bpm=131;
    d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};d.project.events[4].effect=15;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<9;++mode) {
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
        clock=(struct sampled_clock){mode==8?UINT64_MAX-1:100,mode==4?24000:700001,0,1};deadline=77;
        assert(pt_wavetable_song_clocked_begin(song,1000,NULL,&clock)==PT_WAVETABLE_SONG_INVALID && !clock.reads);
        if(mode==6 || mode==7) {
            if(mode==6)clock.ok=0;else clock.frequency=0;
            assert(pt_wavetable_song_clocked_begin(song,1000,sampled_read,&clock)==PT_WAVETABLE_SONG_CLOCK && clock.reads==1);
        }else {
            assert(pt_wavetable_song_clocked_begin(song,1000,sampled_read,&clock)==PT_WAVETABLE_SONG_OK && clock.reads==1);
            if(mode==8) {
                tick_deadline=77;
                assert(pt_wavetable_song_clocked_deadline(song,&tick_deadline)==PT_WAVETABLE_SONG_CLOCK && tick_deadline==77);
                assert(clock.reads==1 && !bus->starts && !pins(f));
                assert(pt_wavetable_song_clocked_service(song,&deadline)==PT_WAVETABLE_SONG_CLOCK && clock.reads==1);
                goto closed_clock;
            }
            assert(pt_wavetable_song_clocked_begin(song,1000,sampled_read,&clock)==PT_WAVETABLE_SONG_INVALID && clock.reads==1);
            assert(pt_wavetable_song_schedule_step(song,0,&deadline)==PT_WAVETABLE_SONG_INVALID && deadline==77);
            assert(pt_wavetable_song_clocked_service(song,NULL)==PT_WAVETABLE_SONG_INVALID && clock.reads==1);
            guard=0;
            do {reads=clock.reads;r=pt_wavetable_song_clocked_service(song,&deadline);
                assert(clock.reads==reads+1 && ++guard<100 && deadline==1000 && !bus->starts);
            }while(r==PT_WAVETABLE_SONG_WAITING);
            assert(r==PT_WAVETABLE_SONG_OK);reads=clock.reads;
            assert(pt_wavetable_song_clocked_deadline(song,&tick_deadline)==PT_WAVETABLE_SONG_OK);
            assert(tick_deadline==100+(1000ULL*clock.frequency+47999)/48000 && clock.reads==reads);
            /* Ceiling conversion reaches the first tick within the requested frame. */
            clock.ticks=100+(1000ULL*clock.frequency+47999)/48000;
            assert(pt_wavetable_song_clocked_service(song,&deadline)==PT_WAVETABLE_SONG_WAITING && bus->starts);
            if(mode) {
                calls=bus->starts+bus->controls;writes=f->writes;bus->stop_result[0]=0;deadline=77;
                if(mode==1)clock.ok=0;
                if(mode==2)++clock.frequency;
                if(mode==3)clock.ticks=99;
                if(mode==4)clock.ticks=UINT64_MAX; /* Conversion overflow before scheduling. */
                if(mode==5)clock.ticks+=700001; /* Missed boundary cannot dispatch late. */
                r=mode==5?PT_WAVETABLE_SONG_DEADLINE:PT_WAVETABLE_SONG_CLOCK;
                assert(pt_wavetable_song_clocked_service(song,&deadline)==r && deadline==77);
                assert(calls==bus->starts+bus->controls && writes==f->writes && pins(f));reads=clock.reads;
                assert(pt_wavetable_song_clocked_service(song,&deadline)==r && clock.reads==reads);
                assert(!pt_wavetable_song_close(&song) && song && sampler.bytes);bus->stop_result[0]=1;
            }else {
                now=1000;guard=0;
                do {
                    for(i=0;i<12;++i)assert(pt_wavetable_song_clocked_service(song,&deadline)==PT_WAVETABLE_SONG_WAITING);
                    now=deadline-now>128?now+128:deadline;
                    reads=clock.reads;
                    assert(pt_wavetable_song_clocked_deadline(song,&tick_deadline)==PT_WAVETABLE_SONG_OK && clock.reads==reads);
                    assert(tick_deadline==100+(deadline*700001+47999)/48000);
                    clock.ticks=now==deadline?tick_deadline:100+(now*700001+47999)/48000;reads=clock.reads;
                    r=pt_wavetable_song_clocked_service(song,&deadline);
                    assert(clock.reads==reads+1 && ++guard<1000);
                }while(r==PT_WAVETABLE_SONG_WAITING);
                assert(r==PT_WAVETABLE_SONG_DONE);reads=clock.reads;
                assert(pt_wavetable_song_clocked_service(song,&deadline)==PT_WAVETABLE_SONG_DONE && clock.reads==reads);
            }
        }
closed_clock:
        assert(pt_wavetable_song_close(&song) && !pins(f));assert(pt_amigus_reservation_close(&f->reservation));
        pt_sampler_release(&sampler);d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};
    }
    pt_document_release(&d);free(bus);free(f);
    puts("WAVETABLE CLOCKED PASS: fractional tick service, single reads, exclusive progression, read/frequency/regression/deadline faults, no recovery reread and uncertain-stop retention; injected only");
}
