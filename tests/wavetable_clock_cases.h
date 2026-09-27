/* Reused by host and Exec allocator fixtures; no native clock/device binding. */
static void clock_song_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_wavetable_song *song=NULL;struct pt_wavetable_preflight_report report;
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct pt_render_interval span;unsigned mode,i,starts,writes;uint64_t now,end;
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};
    assert(f && bus);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;d.project.speed=1;
    d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    d.project.events[4]=(struct pt_event){404,0,PT_NOTE_PERIOD,1,0,0,0,0};d.project.events[8].effect=15;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<7;++mode) {
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
        do {
            assert(pt_wavetable_song_next(song,&span)==PT_WAVETABLE_SONG_OK);
            if(!span.frames){assert(pt_wavetable_song_clock_arm(song,100)==PT_WAVETABLE_SONG_INVALID);
                assert(pt_wavetable_song_complete(song)==PT_WAVETABLE_SONG_OK);}
        }while(!span.frames);
        assert(span.emit && span.frames>256 && bus->starts==1);starts=bus->starts;
        if(mode==1) {
            assert(pt_wavetable_song_clock_arm(song,UINT64_MAX)==PT_WAVETABLE_SONG_CLOCK);
        }else {
            assert(pt_wavetable_song_clock_arm(song,100)==PT_WAVETABLE_SONG_OK);end=100+span.frames;
            assert(pt_wavetable_song_clock_arm(song,100)==PT_WAVETABLE_SONG_INVALID);
            assert(pt_wavetable_song_next(song,&span)==PT_WAVETABLE_SONG_INVALID);
            assert(pt_wavetable_song_consume(song,1)==PT_WAVETABLE_SONG_INVALID);
            assert(pt_wavetable_song_complete(song)==PT_WAVETABLE_SONG_INVALID);
            if(mode==2)assert(pt_wavetable_song_clock_service(song,99)==PT_WAVETABLE_SONG_CLOCK);
            else if(mode==3)assert(pt_wavetable_song_clock_service(song,end+1)==PT_WAVETABLE_SONG_DEADLINE);
            else if(mode==4 || mode==5) {
                if(mode==4)for(now=356;now<end;now+=256)
                    assert(pt_wavetable_song_clock_service(song,now)==PT_WAVETABLE_SONG_WAITING);
                if(mode==5)for(i=0;i<64;++i)assert(pt_wavetable_song_clock_service(song,100)==PT_WAVETABLE_SONG_WAITING);
                bus->stop_result[0]=0;
                /* Unready batch OR ready batch with excessive phase debt must stop
                   without any catch-up upload/start at the expired boundary. */
                writes=f->writes;
                assert(pt_wavetable_song_clock_service(song,end)==PT_WAVETABLE_SONG_DEADLINE);
                assert(f->writes==writes && bus->starts==starts && pins(f));
                assert(pt_wavetable_song_clock_service(song,end)==PT_WAVETABLE_SONG_DEADLINE);
                assert(!pt_wavetable_song_close(&song) && song && sampler.bytes);
                bus->stop_result[0]=1;
            }else {
                #ifdef PT_TEST_PROJECT_VALIDATION_COUNT
                unsigned validations=pt_test_project_validations;
#endif
#ifdef PT_TEST_PCM_VALIDATION_COUNT
                unsigned pcm_validations=pt_test_pcm_validations;
#endif
                /* Same-time work prepares without advancing/starting; subsequent
                   calls pay phase debt in <=256-frame steps. */
                for(i=0;i<64;++i)assert(pt_wavetable_song_clock_service(song,100)==PT_WAVETABLE_SONG_WAITING);
                assert(bus->starts==starts);
                for(now=117;now<end;now+=17) {
                    assert(pt_wavetable_song_clock_service(song,now)==PT_WAVETABLE_SONG_WAITING);
                    assert(bus->starts==starts);
                }
                writes=f->writes;
                if(mode==6) {
                    assert(pt_wavetable_song_close(&song) && !song);
                    assert(bus->starts==starts);
                }else {
                    assert(pt_wavetable_song_clock_service(song,end)==PT_WAVETABLE_SONG_OK);
                    assert(bus->starts==starts+1 && f->writes==writes);
                    assert(pt_wavetable_song_clock_service(song,end)==PT_WAVETABLE_SONG_INVALID);
                }
                #ifdef PT_TEST_PROJECT_VALIDATION_COUNT
                assert(validations==pt_test_project_validations);
#endif
#ifdef PT_TEST_PCM_VALIDATION_COUNT
                assert(pcm_validations==pt_test_pcm_validations);
#endif
            }
        }
        assert(pt_wavetable_song_close(&song) && !song && !pins(f));
        assert(pt_amigus_reservation_close(&f->reservation));pt_sampler_release(&sampler);
        d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};
    }
    pt_document_release(&d);free(bus);free(f);
    puts("WAVETABLE CLOCK PASS: exact ready deadline, no early/late starts, monotonic/overflow refusal, bounded phase debt and retained uncertain stops; injected interval gate only");
}
