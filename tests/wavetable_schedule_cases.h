/* Frame timestamps are injected; test callbacks assert exact scheduled boundaries. */
static uint64_t scheduled_now,scheduled_boundary,scheduled_first;
static unsigned scheduled_callbacks;
static void scheduled_callback(void)
{
    assert(scheduled_now==scheduled_boundary);
    if(!scheduled_callbacks)scheduled_first=scheduled_now;
    ++scheduled_callbacks;
}
static int scheduled_start(void *p,unsigned ch,const struct pt_amigus_voice_plan *plan)
{scheduled_callback();return dispatch_start(p,ch,plan);}
static int scheduled_control(void *p,unsigned ch,uint32_t rate,uint16_t left,uint16_t right)
{scheduled_callback();return dispatch_control(p,ch,rate,left,right);}
static int scheduled_restore(void *p,unsigned ch,const struct pt_amigus_restore_plan *plan)
{scheduled_callback();return range_restore(p,ch,plan);}
static void schedule_song_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_wavetable_song *song=NULL;struct pt_wavetable_preflight_report report;
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};struct pt_render_report measured;
    enum pt_wavetable_song_result r;struct pt_render_interval span;uint64_t deadline,now,boundaries[100];unsigned boundary_count,boundary_at;
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};unsigned mode,guard,i;
    assert(f && bus);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;d.project.speed=3;d.project.bpm=131;
    d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    d.project.events[4]=(struct pt_event){404,0,PT_NOTE_PERIOD,1,15,137,0,0};
    d.project.events[8].effect=15;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<9;++mode) {
        o.pattern_only=o.row_range=mode==1;o.row_first=1;o.row_end=2;o.include_lead_in=mode==2;
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        owner.api.start=scheduled_start;owner.api.control=scheduled_control;owner.api.restore=scheduled_restore;
        scheduled_callbacks=0;scheduled_first=0;
        assert(pt_render_measure(&d.project,&o,NULL,NULL,&measured)==PT_RENDER_OK);
        assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
        boundary_count=boundary_at=0;
        if(mode<3) {
            struct pt_render_sequence *oracle=NULL;struct pt_render_plan *plan=malloc(sizeof(*plan));
            struct pt_render_interval interval;uint64_t total=1000;
            assert(plan && pt_render_sequence_open(&d.project,&o,&a,&oracle)==PT_RENDER_OK);
            do {
                uint32_t left;
                assert(pt_render_sequence_next(oracle,&interval)==PT_RENDER_OK);
                if(interval.emit && interval.frames){total+=interval.frames;assert(boundary_count<100);boundaries[boundary_count++]=total;}
                for(left=interval.frames;left;) {uint32_t n=left>256?256:left;
                    assert(pt_render_sequence_consume(oracle,n)==PT_RENDER_OK);left-=n;}
                assert(pt_render_sequence_complete(oracle,plan)==PT_RENDER_OK);
            }while(!interval.end);
            assert(boundary_count && total==1000+measured.frames);pt_render_sequence_close(oracle);free(plan);
        }
#ifdef PT_TEST_PROJECT_VALIDATION_COUNT
        unsigned validations=pt_test_project_validations;
#endif
#ifdef PT_TEST_PCM_VALIDATION_COUNT
        unsigned pcm_validations=pt_test_pcm_validations;
#endif
        if(mode==5) {
            assert(pt_wavetable_song_schedule_begin(song,UINT64_MAX)==PT_WAVETABLE_SONG_CLOCK);
        }else {
            assert(pt_wavetable_song_schedule_begin(song,1000)==PT_WAVETABLE_SONG_OK);
            assert(pt_wavetable_song_schedule_begin(song,1000)==PT_WAVETABLE_SONG_INVALID);
            assert(pt_wavetable_song_next(song,&span)==PT_WAVETABLE_SONG_INVALID);
            assert(pt_wavetable_song_prefetch(song)==PT_WAVETABLE_SONG_INVALID);
            assert(pt_wavetable_song_consume(song,1)==PT_WAVETABLE_SONG_INVALID);
            assert(pt_wavetable_song_complete(song)==PT_WAVETABLE_SONG_INVALID);
            assert(pt_wavetable_song_clock_service(song,0)==PT_WAVETABLE_SONG_INVALID);
            deadline=77;assert(pt_wavetable_song_schedule_step(song,0,NULL)==PT_WAVETABLE_SONG_INVALID);
            if(mode==3) {
                assert(pt_wavetable_song_schedule_step(song,1000,&deadline)==PT_WAVETABLE_SONG_DEADLINE && deadline==77);
            }else if(mode==4) {
                assert(pt_wavetable_song_schedule_step(song,100,&deadline)==PT_WAVETABLE_SONG_WAITING);
                assert(pt_wavetable_song_schedule_step(song,99,&deadline)==PT_WAVETABLE_SONG_CLOCK);
            }else {
                guard=0;scheduled_now=0;scheduled_boundary=1000;
                do {r=pt_wavetable_song_schedule_step(song,0,&deadline);assert(++guard<1000 && deadline==1000);
                    assert(!bus->starts && !bus->restores && !bus->controls && !bus->stops);
                }while(r==PT_WAVETABLE_SONG_WAITING);
                assert(r==PT_WAVETABLE_SONG_OK);
                for(i=0;i<3;++i)assert(pt_wavetable_song_schedule_step(song,999,&deadline)==PT_WAVETABLE_SONG_OK);
                assert(!scheduled_callbacks);
                if(mode==8)assert(pt_wavetable_song_close(&song) && !scheduled_callbacks);
                else {
                    scheduled_now=1000;r=pt_wavetable_song_schedule_step(song,1000,&deadline);
                    assert(r==PT_WAVETABLE_SONG_WAITING && deadline>1000);
                    if(mode<3)assert(deadline==boundaries[0]);
                    if(mode==6 || mode==7) {
                        unsigned calls=scheduled_callbacks,writes=f->writes;
                        bus->stop_result[0]=0;
                        scheduled_now=deadline+(mode==6);
                        assert(pt_wavetable_song_schedule_step(song,scheduled_now,&deadline)==PT_WAVETABLE_SONG_DEADLINE);
                        assert(scheduled_callbacks==calls && f->writes==writes && pins(f));
                        assert(!pt_wavetable_song_close(&song) && song && sampler.bytes);bus->stop_result[0]=1;
                    }else {
                        now=1000;guard=0;
                        do {
                            /* Give preparation bounded service opportunities while
                               time is unchanged, then advance by <=128 frames. */
                            for(i=0;i<12;++i)assert(pt_wavetable_song_schedule_step(song,now,&deadline)==PT_WAVETABLE_SONG_WAITING);
                            scheduled_boundary=deadline;
                            now=deadline-now>128?now+128:deadline;scheduled_now=now;
                            r=pt_wavetable_song_schedule_step(song,now,&deadline);
                            if(now==scheduled_boundary && r==PT_WAVETABLE_SONG_WAITING) {
                                assert(++boundary_at<boundary_count && deadline==boundaries[boundary_at]);
                            }
                            assert(++guard<1000);
                        }while(r==PT_WAVETABLE_SONG_WAITING);
                        assert(r==PT_WAVETABLE_SONG_DONE && now==1000+measured.frames && scheduled_callbacks);
                        if(mode==2)assert(scheduled_first>1000 && scheduled_first<now);
                        else assert(scheduled_first==1000);
                        assert(pt_wavetable_song_schedule_step(song,now,&deadline)==PT_WAVETABLE_SONG_DONE);
                    }
                }
            }
        }
#ifdef PT_TEST_PROJECT_VALIDATION_COUNT
        assert(validations==pt_test_project_validations);
#endif
#ifdef PT_TEST_PCM_VALIDATION_COUNT
        assert(pcm_validations==pt_test_pcm_validations);
#endif
        assert(pt_wavetable_song_close(&song) && !pins(f));assert(pt_amigus_reservation_close(&f->reservation));
        pt_sampler_release(&sampler);d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};
    }
    pt_document_release(&d);free(bus);free(f);
    puts("WAVETABLE SCHEDULE PASS: future startup, silent range priming, fractional/tempo absolute boundaries, lead-in, no early/late starts, deadline/clock refusal and uncertain-stop retention; injected only");
}
