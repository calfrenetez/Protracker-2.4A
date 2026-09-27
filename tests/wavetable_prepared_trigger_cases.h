/* Count the production converter through a host-only link wrapper. */
#ifdef PT_TEST_RENDER_VOICE_COUNT
extern unsigned pt_test_render_voice_plans,pt_test_render_controls;
#define COMMAND_COUNT (pt_test_render_voice_plans+pt_test_render_controls)
#else
#define COMMAND_COUNT 0U
#endif
static void prepared_trigger_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_wavetable_song *song=NULL;struct pt_wavetable_preflight_report report;
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    enum pt_wavetable_song_result r;uint64_t deadline,now;
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};unsigned mode,i,guard,before,total,writes,controls;uint32_t old_rate;
    assert(f && bus);o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=7;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<6;++mode) {
        pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
        d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
        for(i=0;i<3;++i)d.project.events[i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
        d.project.events[0].effect=1;d.project.events[0].parameter=1;
        d.project.events[4].effect=15;format.bits=mode==1?8:16;
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
        assert(pt_wavetable_song_schedule_begin(song,1000)==PT_WAVETABLE_SONG_OK);
        total=COMMAND_COUNT;guard=0;i=0;
        do {
            before=COMMAND_COUNT;r=pt_wavetable_song_schedule_step(song,0,&deadline);
            assert(COMMAND_COUNT-before<=1 && !bus->starts && !bus->controls && !bus->stops && ++guard<100);
            if(mode==3 && pins(f)==3 && ++i==2)break; /* Cancellation while commands are still being prepared. */
        }while(r==PT_WAVETABLE_SONG_WAITING);
        if(mode!=3) {
            assert(r==PT_WAVETABLE_SONG_OK && pins(f)==3);
#ifdef PT_TEST_RENDER_VOICE_COUNT
            assert(COMMAND_COUNT-total==6);
#endif
            before=COMMAND_COUNT;writes=f->writes;
            for(i=0;i<3;++i)assert(pt_wavetable_song_schedule_step(song,0,&deadline)==PT_WAVETABLE_SONG_OK);
            assert(COMMAND_COUNT==before && f->writes==writes);
            if(mode==2) {
                --d.project.samples[0].pcm.capacity;
                assert(pt_wavetable_song_schedule_step(song,1000,&deadline)==PT_WAVETABLE_SONG_DEVICE);
                assert(!bus->starts && !pins(f));
            }else {
                assert(pt_wavetable_song_schedule_step(song,1000,&deadline)==PT_WAVETABLE_SONG_WAITING);
                assert(bus->starts==3 && pins(f)==3 && writes==f->writes);
#ifdef PT_TEST_RENDER_VOICE_COUNT
                /* Whole-batch live-state validation remains; immutable geometry
                   and trigger lowering both reuse the prepared commands. */
                assert(COMMAND_COUNT==before);
#endif
                for(i=0;i<3;++i) {
                    assert(bus->plan[i].end_exclusive-bus->plan[i].start==8U*(format.bits/8U));
                    assert(bus->plan[i].loop==bus->plan[i].start && bus->plan[i].rate);
                    assert((bus->plan[i].control&3)==(unsigned)(2+(format.bits==16)));
                }
                assert(bus->plan[0].rate==bus->plan[1].rate && bus->plan[1].rate==bus->plan[2].rate);
                controls=bus->controls;old_rate=bus->plan[0].rate;total=COMMAND_COUNT;
                for(i=0;i<20;++i) {
                    before=COMMAND_COUNT;
                    assert(pt_wavetable_song_schedule_step(song,1000,&deadline)==PT_WAVETABLE_SONG_WAITING);
                    assert(COMMAND_COUNT-before<=1 && bus->controls==controls && f->writes==writes);
                }
#ifdef PT_TEST_RENDER_VOICE_COUNT
                assert(COMMAND_COUNT-total==3); /* Three control-only conversions. */
#endif
                before=COMMAND_COUNT;now=1000;
                while(deadline-now>256) {
                    now+=256;assert(pt_wavetable_song_schedule_step(song,now,&deadline)==PT_WAVETABLE_SONG_WAITING);
                    assert(COMMAND_COUNT==before && bus->controls==controls);
                }
                if(mode==4){owner.voice[2].uncertain=1;bus->stop_result[2]=0;}
                if(mode==5)owner.api.control=NULL;
                r=pt_wavetable_song_schedule_step(song,deadline,&deadline);
                assert(COMMAND_COUNT==before && f->writes==writes);
                if(mode>=4) {
                    assert(r==PT_WAVETABLE_SONG_DEVICE && bus->controls==controls && bus->starts==3);
                    if(mode==4) {
                        assert(pins(f)==1 && !pt_wavetable_song_close(&song));
                        bus->stop_result[2]=1;
                    }else assert(!pins(f));
                }else {
                    assert(r==PT_WAVETABLE_SONG_WAITING && bus->controls==controls+3);
                    assert(bus->plan[0].rate>old_rate && pins(f)==3);
                }
            }
        }
        (void)total;assert(pt_wavetable_song_close(&song) && !pins(f));assert(pt_amigus_reservation_close(&f->reservation));
        pt_sampler_release(&sampler);d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};pt_document_release(&d);
    }
    free(bus);free(f);
    puts("PREPARED BATCH PASS: one command per preparation step, zero commit conversions,8/16-bit plans, controls, duplicate leases, stale/cancel and whole-batch active-state refusal with uncertain-stop retention");
}
#undef COMMAND_COUNT
