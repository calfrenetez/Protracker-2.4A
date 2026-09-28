/* End-to-end editor -> master mixer -> queue -> packed FIFO ownership. */
struct output_port {int reset,space,write;unsigned writes,resets,drains;uint32_t hash,first;size_t bytes;struct pt_amigus_reservation *reservation;int start_result;unsigned starts,gate,started,prefill,target;};
static uint32_t output_hash(uint32_t hash,unsigned byte) {return (hash^(byte&255u))*16777619u;}
static void output_access(struct output_port *p)
{if(p->reservation)assert(p->reservation->opened && p->reservation->reserved && p->reservation->access && p->reservation->resource==PT_AMIGUS_PCM);}
static int output_capacity(void *c) {output_access(c);return ((struct output_port *)c)->space;}
static int output_write3(void *c,const uint32_t *words)
{
    struct output_port *p=c;unsigned i,j;
    output_access(p);if(p->gate) {assert(p->prefill<p->target || p->started);++p->prefill;}if(!p->writes)p->first=words[0];
    ++p->writes;
    for(i=0;i<3;++i)for(j=0;j<4;++j)p->hash=output_hash(p->hash,words[i]>>(24-j*8));
    p->bytes+=12;return p->write;
}
static int output_reset(void *c) {struct output_port *p=c;output_access(p);++p->resets;if(p->reset==1)p->started=p->prefill=0;return p->reset;}
static int output_start(void *c)
{
    struct output_port *p=c;output_access(p);assert(p->gate && p->prefill==p->target);
    ++p->starts;if(p->start_result==1)p->started=1;return p->start_result;
}
static int output_drain(void *c) {struct output_port *p=c;output_access(p);return ++p->drains>1;}
static void output_shutdown(struct pt_editor_studio_output *o,struct output_port *p)
{
    unsigned i;p->reset=1;
    for(i=0;i<50 && o->queue;++i)pt_editor_studio_output_step(o,17);
    assert(i<50 && !o->queue && !o->producer.song && o->session.phase==PT_AS_IDLE);
}
struct output_library {unsigned opens,closes,reserves,releases,quiesces;int quiet;struct pt_editor_studio_output *owner;struct pt_amigus_reservation *reservation;};
static int output_library_open(void *c) {++((struct output_library *)c)->opens;return 1;}
static void output_library_close(void *c) {++((struct output_library *)c)->closes;}
static void *output_library_find(void *c,void *previous) {return previous?NULL:c;}
static int output_library_supported(void *c,void *card,enum pt_amigus_resource resource) {return c==card && resource==PT_AMIGUS_PCM;}
static unsigned long output_library_reserve(void *c,void *card,enum pt_amigus_resource resource,void *owner)
{struct output_library *f=c;assert(card==c && resource==PT_AMIGUS_PCM && owner==f->reservation);++f->reserves;return 0;}
static void output_library_release(void *c,void *card,enum pt_amigus_resource resource,void *owner)
{struct output_library *f=c;assert(card==c && resource==PT_AMIGUS_PCM && owner==f->reservation && !f->reservation->access);++f->releases;}
static int output_quiesce(void *c)
{
    struct output_library *f=c;
    assert(f->reservation->access && f->reservation->reserved);
    assert(f->owner->queue?f->owner->session.phase==PT_AS_DONE:f->owner->session.phase==PT_AS_IDLE);
    if(f->owner->queue) {
        const struct pt_pcm *pcm=NULL;uint64_t ticket=0;
        /* A delayed adapter callback can still inspect its borrowed queue after
         * reset. ASan would detect release before this quiescence callback. */
        assert(pt_studio_queue_acquire(f->owner->queue,&pcm,&ticket)==PT_QUEUE_DONE);
    }
    ++f->quiesces;return f->quiet;
}
static void output_reserved_cases(struct pt_editor_studio_output *o,struct pt_render_options *options,struct output_port *p,struct pt_amigus_fifo_port *port)
{
    unsigned mode,i;
    for(mode=0;mode<5;++mode) {
        struct pt_amigus_reservation r={0};struct output_library f={0};
        struct pt_amigus_reservation_api api={&f,output_library_open,output_library_close,output_library_find,output_library_supported,output_library_reserve,output_library_release};
        f.owner=o;f.reservation=&r;p->reservation=&r;p->reset=mode!=0;p->space=3;p->write=1;
        assert(pt_amigus_reservation_open(&r,&api,0)==PT_AMIGUS_RESERVED);
        assert(!pt_editor_studio_output_start_reserved(o,options,2,port,output_drain,p,&r,NULL,&f));assert(!r.access);
        r.resource=PT_AMIGUS_WAVETABLE;
        assert(!pt_editor_studio_output_start_reserved(o,options,2,port,output_drain,p,&r,output_quiesce,&f));assert(!r.access);r.resource=PT_AMIGUS_PCM;
        if(mode==2)fail_next=1;
        assert(pt_editor_studio_output_start_reserved(o,options,2,port,output_drain,p,&r,output_quiesce,&f)==(mode==1 || mode==3 || mode==4));
        assert(r.access && pt_editor_studio_output_busy(o) && !pt_amigus_reservation_close(&r));
        assert(!pt_editor_studio_output_start(o,options,2,port,output_drain,p));
        if(mode==3) {
            for(i=0;i<1000 && !o->session.consumer.leased;++i)pt_editor_studio_output_step(o,17);
            assert(i<1000);pt_editor_studio_output_stop(o);p->reset=0;
        }
        if(mode==0 || mode==3) {
            pt_editor_studio_output_step(o,17);assert(o->queue && !f.quiesces && !pt_editor_studio_output_detach(o));p->reset=1;
        }
        for(i=0;i<30000 && o->queue && o->session.phase!=PT_AS_DONE;++i)pt_editor_studio_output_step(o,17);
        assert(i<30000 && !f.quiesces && r.access && !pt_amigus_reservation_close(&r));
        assert(!pt_editor_studio_output_detach(o));
        pt_editor_studio_output_step(o,17);assert(f.quiesces==1 && pt_editor_studio_output_busy(o));
        assert(mode==2 || o->queue); /* Queue stays valid through pending quiescence. */
        f.quiet=2;pt_editor_studio_output_step(o,17);
        assert(r.access && (mode==2 || o->queue) && !pt_editor_studio_output_detach(o));
        /* An adapter claiming success cannot override a retained IRQ guard. */
        r.interrupt=1;f.quiet=1;pt_editor_studio_output_step(o,17);
        assert(r.access && !o->quiesced && (mode==2 || o->queue));r.interrupt=0;
        if(mode!=4) {f.quiet=-1;assert(pt_editor_studio_output_step(o,17)==PT_CONSUMER_ERROR && r.access && !pt_amigus_reservation_close(&r));}
        f.quiet=1;assert(pt_editor_studio_output_step(o,17)==(mode==4?PT_CONSUMER_FINISHED:PT_CONSUMER_ERROR) && !r.access && !pt_editor_studio_output_busy(o));
        if(mode==4)assert(!o->failed);
        assert(pt_amigus_reservation_close(&r));assert(f.opens==1 && f.closes==1 && f.reserves==1 && f.releases==1);
        p->reservation=NULL;
    }
    puts("EDITOR STUDIO RESERVED PASS: PCM-only lease spans failed start, natural drain, leased Stop, reset and adapter quiescence; early release/restart refused; fake library/port only");
}
static void output_start_cases(struct pt_editor_studio_output *o,struct pt_render_options *options,struct output_port *p,struct pt_amigus_fifo_port *port)
{
    struct pt_editor *e=o->producer.editor;unsigned mode,i,writes;
    assert(!pt_editor_studio_output_bind_prefill(o,output_start,p,0));
    assert(pt_editor_studio_output_bind_prefill(o,output_start,p,2));p->gate=1;p->target=2;
    for(mode=0;mode<6;++mode) {
        struct pt_amigus_reservation r={0};struct output_library f={0};
        struct pt_amigus_reservation_api api={&f,output_library_open,output_library_close,output_library_find,output_library_supported,output_library_reserve,output_library_release};
        p->start_result=0;p->starts=0;p->space=6;p->reset=p->write=1;
        if(mode==5) {
            f.owner=o;f.reservation=&r;p->reservation=&r;
            assert(pt_amigus_reservation_open(&r,&api,0)==PT_AMIGUS_RESERVED);
            assert(pt_editor_studio_output_start_reserved(o,options,2,port,output_drain,p,&r,output_quiesce,&f));
        } else assert(pt_editor_studio_output_start(o,options,2,port,output_drain,p));
        assert(!pt_editor_studio_output_bind_start(o,NULL,NULL));
        for(i=0;i<1000 && !p->starts;++i)assert(pt_editor_studio_output_step(o,17)!=PT_CONSUMER_ERROR);
        assert(i<1000 && p->prefill==2 && !p->started);writes=p->writes;
        for(i=0;i<5;++i)assert(pt_editor_studio_output_step(o,17)==PT_CONSUMER_WAIT);
        assert(p->writes==writes && o->session.consumer.leased);
        e->row=0;e->editing=1;
        if(mode==0)p->start_result=1;
        else if(mode==1)pt_editor_key(e,0x31,0);
        else if(mode==2)pt_editor_key(e,0x31,8);
        else if(mode==3 || mode==5)pt_editor_studio_output_stop(o);
        else {p->start_result=2;assert(pt_editor_studio_output_step(o,17)==PT_CONSUMER_ERROR);}
        if(mode) {
            p->reset=0;pt_editor_studio_output_step(o,17);
            assert(!o->producer.song && !o->producer.queue && o->queue && o->session.consumer.leased);
            assert(!pt_editor_studio_output_detach(o) && !pt_editor_studio_output_bind_start(o,NULL,NULL));
            if(mode==5)assert(r.access && !pt_amigus_reservation_close(&r));
            p->reset=1;
        }
        for(i=0;i<30000 && o->queue && o->session.phase!=PT_AS_DONE;++i)pt_editor_studio_output_step(o,17);
        assert(i<30000 && o->session.phase==PT_AS_DONE);
        if(mode==5) {
            pt_editor_studio_output_step(o,17);assert(o->queue && r.access && !pt_amigus_reservation_close(&r));f.quiet=1;
        }
        pt_editor_studio_output_step(o,17);assert(!pt_editor_studio_output_busy(o));
        assert(o->failed==(mode==4) && o->start==output_start);
        if(mode==5) {assert(pt_amigus_reservation_close(&r));p->reservation=NULL;}
        e->project->events[0].kind=PT_NOTE_PERIOD;e->project->events[0].pitch=428;e->project->events[0].instrument=1;
    }
    assert(pt_editor_studio_output_bind_start(o,NULL,NULL));p->gate=0;
    puts("EDITOR STUDIO START PASS: configurable prefill, acknowledgement gate, edit/undo/Stop, unknown result and reserved quiescence retain ownership");
}
static void editor_studio_output_cases(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;
    struct pt_editor *e=calloc(1,sizeof(*e));struct pt_editor_studio_output owner={0},other={0};
    struct pt_render_options options={0};struct output_port p={0};
    struct pt_amigus_fifo_port port={&p,output_capacity,output_write3,output_reset};
    int32_t pcm[4]={257,-513,1025,-2049};unsigned i,action;uint32_t expected=2166136261u;size_t expected_bytes=0;
    p.reset=p.write=1;p.space=3;p.hash=2166136261u;
    assert(e);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_editor_init(e,&d.project));pt_sampler_init(&e->sampler,&a,1024*1024);
    d.project.samples[0].pcm=(struct pt_pcm){pcm,4,4,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=4;d.project.channels.track[0].pan=0;
    d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;d.project.events[0].instrument=1;d.project.events[4].effect=15;
    options.rate=48000;options.bits=24;options.tracks=1;options.gain_q16=65536;options.tick_limit=1000;options.frame_limit=1000000;
    /* Independent direct-pull oracle; compare every packed output byte, including
     * low precision bits and block boundaries, against the existing mixer path. */
    {struct pt_sampler_song *song=NULL;const struct pt_pcm *block;unsigned done=0,j;
        assert(pt_sampler_song_open(&e->sampler,&d.project,&options,&a,&song)==PT_RENDER_OK);
        for(i=0;i<2000 && !done;++i) {
            assert(pt_sampler_song_pull(song,113,&block,&done)==PT_RENDER_OK);
            if(block)for(j=0;j<block->frames*2;++j) {
                uint32_t sample=(uint32_t)block->data[j];
                expected=output_hash(expected,sample>>16);expected=output_hash(expected,sample>>8);expected=output_hash(expected,sample);expected_bytes+=3;
            }
        }
        assert(done && expected_bytes);pt_sampler_song_close(song);
        if((expected_bytes/6)&1u)for(j=0;j<6;++j) {expected=output_hash(expected,0);++expected_bytes;}
    }
    assert(pt_editor_studio_output_attach(&owner,e));assert(!pt_editor_studio_output_attach(&other,e));
    assert(!pt_editor_studio_output_start(&owner,&options,0,&port,output_drain,&p));
    assert(!pt_editor_studio_output_start(&owner,&options,2,&port,NULL,&p));assert(!p.resets && !owner.queue);
    options.bits=16;assert(!pt_editor_studio_output_start(&owner,&options,2,&port,output_drain,&p));
    assert(!owner.queue && !owner.producer.song && !p.resets);options.bits=24;
    fail_next=1;assert(!pt_editor_studio_output_start(&owner,&options,2,&port,output_drain,&p));assert(!owner.queue && !p.resets);
    assert(pt_editor_studio_output_start(&owner,&options,2,&port,output_drain,&p));
    assert(!pt_editor_studio_output_start(&owner,&options,2,&port,output_drain,&p));
    assert(pt_editor_studio_output_step(&owner,0)==PT_CONSUMER_ERROR && owner.queue && !owner.failed);
    for(i=0;i<30000 && owner.queue;++i) {
        unsigned writes=p.writes,resets=p.resets;p.space=i%5?3:0;
        assert(pt_editor_studio_output_step(&owner,17)!=PT_CONSUMER_ERROR);
        assert(p.writes-writes<=1 && p.resets-resets<=1);
    }
    assert(i<30000 && !owner.failed && p.hash==expected && p.bytes==expected_bytes && p.first==0x00010100u && p.drains>=2);
    /* Failed initial reset keeps queue/context owned and refuses restart/detach. */
    p.reset=0;assert(!pt_editor_studio_output_start(&owner,&options,2,&port,output_drain,&p));
    assert(owner.queue && !owner.producer.song && owner.failed);
    assert(!pt_editor_studio_output_detach(&owner));assert(owner.producer.editor==e);
    assert(pt_editor_studio_output_step(&owner,17)==PT_CONSUMER_ERROR && owner.queue);
    output_shutdown(&owner,&p);
    output_reserved_cases(&owner,&options,&p,&port);
    output_start_cases(&owner,&options,&p,&port);
    /* Edit, undo, explicit Stop, output failure and dispose all retain a leased
     * queue through pending/failed reset, and stop further source production. */
    for(action=0;action<7;++action) {
        p.reset=1;p.space=0;p.write=1;
        if(action==6) {assert(pt_editor_studio_output_bind_start(&owner,output_start,&p));p.gate=1;p.target=1;p.space=3;p.starts=0;p.start_result=0;}
        assert(pt_editor_studio_output_start(&owner,&options,2,&port,output_drain,&p));
        for(i=0;i<1000 && !owner.session.consumer.leased;++i)assert(pt_editor_studio_output_step(&owner,17)!=PT_CONSUMER_ERROR);
        assert(i<1000 && owner.queue && owner.producer.song);
        if(action==6) {for(i=0;i<100 && !p.starts;++i)pt_editor_studio_output_step(&owner,17);assert(i<100 && owner.session.start_pending);}
        p.reset=0;e->row=0;e->editing=1;
        if(action==0)pt_editor_key(e,0x31,0);
        else if(action==1)pt_editor_key(e,0x31,8);
        else if(action==2)pt_editor_studio_output_stop(&owner);
        else if(action==3) {
            p.space=-1;
            for(i=0;i<4 && !owner.failed;++i)pt_editor_studio_output_step(&owner,17);
            assert(owner.failed);
        } else if(action==4) {
            p.space=3;p.write=0;
            for(i=0;i<4 && !owner.failed;++i)pt_editor_studio_output_step(&owner,17);
            assert(owner.failed);
        } else if(action==5) {
            /* Unexpected source-header changes are still rejected while queued. */
            d.project.channels.track[0].pan=1;
            for(i=0;i<10 && !owner.failed;++i)pt_editor_studio_output_step(&owner,17);
            assert(owner.failed);d.project.channels.track[0].pan=0;
        } else pt_editor_dispose(e);
        assert(!owner.producer.song && !owner.producer.queue && owner.queue);
        assert(!pt_editor_studio_output_detach(&owner));
        pt_editor_studio_output_step(&owner,17);assert(owner.queue && owner.session.consumer.leased);
        p.reset=-1;assert(pt_editor_studio_output_step(&owner,17)==PT_CONSUMER_ERROR && owner.queue);
        output_shutdown(&owner,&p);assert(owner.failed);
        d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;d.project.events[0].instrument=1;
    }
    assert(pt_editor_studio_output_detach(&owner));assert(!e->before_change && !owner.start && !owner.start_context);
    pt_document_release(&d);free(e);assert(!live && pcm[0]==257 && pcm[3]==-2049);
    puts("EDITOR STUDIO OUTPUT PASS: direct24 byte parity, bounded producer/FIFO steps, natural drain, edit/undo/Stop/dispose and failure reset ownership, restart and zero allocations retained");
}
