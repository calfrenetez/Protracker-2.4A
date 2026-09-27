/* Count the production converter through a host-only link wrapper. */
#ifdef PT_TEST_RENDER_VOICE_COUNT
extern unsigned pt_test_render_voice_plans;
#define COMMAND_COUNT pt_test_render_voice_plans
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
    enum pt_wavetable_song_result r;uint64_t deadline;
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};unsigned mode,i,guard,before,total,writes;
    assert(f && bus);o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=7;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<4;++mode) {
        pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
        d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
        for(i=0;i<3;++i)d.project.events[i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
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
            assert(COMMAND_COUNT-total==3);
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
                /* Whole-batch capability validation remains. Trigger lowering
                   reuses the three prepared plans instead of converting again. */
                assert(COMMAND_COUNT-before==3);
#endif
                for(i=0;i<3;++i) {
                    assert(bus->plan[i].end_exclusive-bus->plan[i].start==8U*(format.bits/8U));
                    assert(bus->plan[i].loop==bus->plan[i].start && bus->plan[i].rate);
                    assert((bus->plan[i].control&3)==(unsigned)(2+(format.bits==16)));
                }
                assert(bus->plan[0].rate==bus->plan[1].rate && bus->plan[1].rate==bus->plan[2].rate);
            }
        }
        (void)total;assert(pt_wavetable_song_close(&song) && !pins(f));assert(pt_amigus_reservation_close(&f->reservation));
        pt_sampler_release(&sampler);d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};pt_document_release(&d);
    }
    free(bus);free(f);
    puts("PREPARED TRIGGER PASS: bounded pre-start conversion, repeated readiness without recompute,8/16-bit plans, duplicate leases, stale refusal and cancellation; whole-batch validation retained");
}
#undef COMMAND_COUNT
