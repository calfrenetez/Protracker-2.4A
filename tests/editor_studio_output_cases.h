/* End-to-end editor -> master mixer -> queue -> packed FIFO ownership. */
struct output_port {int reset,space,write;unsigned writes,resets,drains;uint32_t hash,first;size_t bytes;};
static uint32_t output_hash(uint32_t hash,unsigned byte) {return (hash^(byte&255u))*16777619u;}
static int output_capacity(void *c) {return ((struct output_port *)c)->space;}
static int output_write3(void *c,const uint32_t *words)
{
    struct output_port *p=c;unsigned i,j;
    if(!p->writes)p->first=words[0];
    ++p->writes;
    for(i=0;i<3;++i)for(j=0;j<4;++j)p->hash=output_hash(p->hash,words[i]>>(24-j*8));
    p->bytes+=12;return p->write;
}
static int output_reset(void *c) {struct output_port *p=c;++p->resets;return p->reset;}
static int output_drain(void *c) {struct output_port *p=c;return ++p->drains>1;}
static void output_shutdown(struct pt_editor_studio_output *o,struct output_port *p)
{
    unsigned i;p->reset=1;
    for(i=0;i<50 && o->queue;++i)pt_editor_studio_output_step(o,17);
    assert(i<50 && !o->queue && !o->producer.song && o->session.phase==PT_AS_IDLE);
}
static void editor_studio_output_cases(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;
    struct pt_editor *e=calloc(1,sizeof(*e));struct pt_editor_studio_output owner={0},other={0};
    struct pt_render_options options={0};struct output_port p={1,3,1,0,0,0,2166136261u,0,0};
    struct pt_amigus_fifo_port port={&p,output_capacity,output_write3,output_reset};
    int32_t pcm[4]={257,-513,1025,-2049};unsigned i,action;uint32_t expected=2166136261u;size_t expected_bytes=0;
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
    /* Edit, undo, explicit Stop, output failure and dispose all retain a leased
     * queue through pending/failed reset, and stop further source production. */
    for(action=0;action<7;++action) {
        p.reset=1;p.space=0;p.write=1;
        assert(pt_editor_studio_output_start(&owner,&options,2,&port,output_drain,&p));
        for(i=0;i<1000 && !owner.session.consumer.leased;++i)assert(pt_editor_studio_output_step(&owner,17)!=PT_CONSUMER_ERROR);
        assert(i<1000 && owner.queue && owner.producer.song);
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
    assert(pt_editor_studio_output_detach(&owner));assert(!e->before_change);
    pt_document_release(&d);free(e);assert(!live && pcm[0]==257 && pcm[3]==-2049);
    puts("EDITOR STUDIO OUTPUT PASS: direct24 byte parity, bounded producer/FIFO steps, natural drain, edit/undo/Stop/dispose and failure reset ownership, restart and zero allocations retained");
}
