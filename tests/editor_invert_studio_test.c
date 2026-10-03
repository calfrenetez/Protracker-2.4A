#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/editor/editor_studio.h"
#include "../src/core/amigus_session.h"
#include <string.h>
#ifdef __amigaos__
static void test_failure(const char *condition,unsigned line)
{fprintf(stderr,"EDITOR INVERT STUDIO FAIL: line=%u condition=%s\n",line,condition);exit(20);}
#undef assert
#define assert(condition) ((condition)?(void)0:test_failure(#condition,__LINE__))
#endif
static unsigned live,calls,fail_at;
static void *allocate(void *c,size_t n) {void *p;(void)c;if(++calls==fail_at)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;if(p){assert(live);--live;free(p);}}
static void start(struct pt_editor_studio *o,struct pt_render_options *options)
{
    unsigned i,done;const struct pt_pcm *out;
    assert(pt_editor_studio_start_invert(o,options,SIZE_MAX)==PT_RENDER_OK);
    for(i=0;i<100;++i) {assert(pt_editor_studio_pull(o,1,&out,&done)==PT_RENDER_OK && !done);if(out) {assert(out->data[0]==(17*65536));return;}}
    assert(0);
}
static unsigned output_stops;
static void count_stop(void *c) {++*(unsigned *)c;}
static int port_space(void *c) {(void)c;return 3;}
static int port_write(void *c,const uint32_t *p) {(void)c;(void)p;return 1;}
static int port_reset(void *c) {return *(int *)c;}
static int port_drain(void *c) {(void)c;return 0;}
static void output_stop(void *c) {++output_stops;pt_amigus_session_stop(c);}
static void incremental_cases(void)
{
    static int32_t master[8192];struct pt_allocator a={NULL,allocate,release};struct pt_document d;
    struct pt_editor *e=calloc(1,sizeof(*e));struct pt_editor_studio owner={0};struct pt_render_options o={0},limited;
    const struct pt_pcm *out;unsigned ready,done,i,baseline,action,phase,mode,count;
    assert(e);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_editor_init(e,&d.project));pt_sampler_init(&e->sampler,&a,1024*1024);
    for(i=0;i<8192;++i)master[i]=(int32_t)(i%127)-63;
    master[0]=17;d.project.samples[0].pcm=(struct pt_pcm){master,8192,8192,48000,1,8};
    d.project.samples[0].volume=64;d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8192;
    d.project.channels.track[0].pan=0;d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,14,255,0,0};d.project.events[4].effect=15;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=1000;o.frame_limit=1000000;
    assert(pt_editor_studio_attach(&owner,e));baseline=live;
    ready=9;assert(pt_editor_studio_prepare(&owner,&ready)==PT_RENDER_INVALID && ready==9);
    assert(pt_editor_studio_begin_invert(&owner,&o,SIZE_MAX)==PT_RENDER_OK && owner.invert_song && !owner.song);
    assert(pt_editor_studio_pull(&owner,1,&out,&done)==PT_RENDER_OK && !out && !done && !e->sampler.bytes);
    pt_editor_key(e,0x4d,0);assert(owner.invert_song);
    for(i=0,ready=0;i<64 && !ready;++i)assert(pt_editor_studio_prepare(&owner,&ready)==PT_RENDER_OK);
    assert(i<64 && ready);
    for(i=0;i<100;++i) {assert(pt_editor_studio_pull(&owner,1,&out,&done)==PT_RENDER_OK && !done);if(out)break;}
    assert(i<100 && out->data[0]==17*65536 && master[0]==17 && !e->sampler.bytes);
    pt_editor_studio_stop(&owner);assert(live==baseline);d.project.channels.selected=0;
    for(i=16;i<=24;i+=8) {
        d.project.samples[0].pcm.bits=(uint8_t)i;
        assert(pt_editor_studio_begin_invert(&owner,&o,SIZE_MAX)==PT_RENDER_SAMPLE && !owner.invert_song && live==baseline);
        assert(d.project.samples[0].pcm.bits==i && master[0]==17);
    }
    d.project.samples[0].pcm.bits=8;
    /* Pump-driven pending copies publish no audio; readiness itself still leaves
     * the queue empty. Stop preserves a consumer lease from the ready producer. */
    {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned stops=0;
        assert(q && pt_editor_studio_begin_invert_queued(&owner,&o,SIZE_MAX,q)==PT_RENDER_OK);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        assert(pt_editor_studio_step(&owner,0)==PT_PUMP_ERROR && owner.invert_song && !stops);
        for(i=0;i<3;++i) {
            assert(pt_editor_studio_step(&owner,17)==PT_PUMP_PROGRESS && !owner.pump.held);
            assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_EMPTY);
        }
        for(i=0,ready=0;i<64 && !ready;++i) {
            assert(pt_editor_studio_prepare(&owner,&ready)==PT_RENDER_OK);
            assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_EMPTY);
        }
        assert(i<64 && ready && !stops);
        for(i=0;i<50;++i) {assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);if(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK)break;}
        assert(i<50 && out->data[0]==17*65536);
        for(i=0;i<20;++i)assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
        assert(owner.pump.held);pt_editor_studio_stop(&owner);
        assert(!owner.invert_song && !owner.queue && !owner.pump.held && stops==1 && out->data[0]==17*65536);
        pt_editor_studio_stop(&owner);assert(stops==1 && pt_studio_queue_close(q)==PT_QUEUE_BUSY);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK && pt_studio_queue_close(q)==PT_QUEUE_OK && live==baseline);
    }
    /* Explicit Stop and real editor edit/undo barriers cancel a partially copied
     * bank without creating master storage or any queue block. */
    for(action=0;action<3;++action) {
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned stops=0;
        e->editing=1;e->row=0;e->panel=0;
        if(action==2) {pt_editor_key(e,0x31,0);e->row=0;}
        assert(q && pt_editor_studio_begin_invert_queued(&owner,&o,SIZE_MAX,q)==PT_RENDER_OK);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        for(i=0;i<3;++i)assert(pt_editor_studio_step(&owner,17)==PT_PUMP_PROGRESS);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_EMPTY);
        if(!action)pt_editor_studio_stop(&owner);
        else pt_editor_key(e,0x31,action==2?8:0);
        assert(!owner.invert_song && !owner.queue && stops==1 && !e->sampler.bytes);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_DONE && pt_studio_queue_close(q)==PT_QUEUE_OK && live==baseline);
        if(action==1)pt_editor_key(e,0x31,8);
        assert(d.project.events[0].pitch==428 && d.project.samples[0].pcm.data==master && master[0]==17);
    }
    /* Editor/owner output aliases leave the live producer usable; delegated
     * private-source aliases follow the existing producer-error Stop lifecycle. */
    for(phase=0;phase<2;++phase)for(mode=0;mode<2;++mode) {
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned stops=0,generation=e->sampler.generation;
        assert(q && pt_editor_studio_begin_invert_queued(&owner,&o,SIZE_MAX,q)==PT_RENDER_OK);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        if(phase) {for(i=0,ready=0;i<64 && !ready;++i)assert(pt_editor_studio_prepare(&owner,&ready)==PT_RENDER_OK);assert(i<64 && ready);}
        assert(pt_editor_studio_prepare(&owner,(unsigned *)(void *)&owner.pump.error)==PT_RENDER_INVALID && owner.invert_song && owner.queue==q && !stops);
        assert(pt_editor_studio_prepare(&owner,&e->sampler.generation)==PT_RENDER_INVALID && e->sampler.generation==generation && owner.invert_song && !stops);
        assert(pt_editor_studio_prepare(&owner,mode?&d.project.midi_flags:(unsigned *)(void *)master)==PT_RENDER_INVALID);
        assert(master[0]==17 && master[1]==-62 && !owner.invert_song && !owner.queue && stops==1);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_DONE && pt_studio_queue_close(q)==PT_QUEUE_OK && live==baseline);
    }
    /* Stale fixed-header and generation changes fail the queued owner while
     * pending and fully prepared; cleanup requests bound output Stop once. */
    for(phase=0;phase<2;++phase)for(mode=0;mode<4;++mode) {
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);struct pt_project saved;
        uint64_t ticket;unsigned stops=0,generation=e->sampler.generation;
        memcpy(&saved,&d.project,sizeof(saved));
        assert(q && pt_editor_studio_begin_invert_queued(&owner,&o,SIZE_MAX,q)==PT_RENDER_OK);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        if(phase) {for(i=0,ready=0;i<64 && !ready;++i)assert(pt_editor_studio_prepare(&owner,&ready)==PT_RENDER_OK);assert(i<64 && ready);}
        else for(i=0;i<3;++i)assert(pt_editor_studio_step(&owner,17)==PT_PUMP_PROGRESS);
        if(mode==0)++d.project.bpm;else if(mode==1)d.project.samples=NULL;
        else if(mode==2)d.project.midi_output[0][0]^=1;else ++e->sampler.generation;
        assert(pt_editor_studio_step(&owner,17)==PT_PUMP_ERROR && !owner.invert_song && !owner.queue && stops==1);
        pt_editor_studio_stop(&owner);assert(stops==1);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_DONE && pt_studio_queue_close(q)==PT_QUEUE_OK && live==baseline);
        memcpy(&d.project,&saved,sizeof(saved));e->sampler.generation=generation;
    }
    {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned stops=0;
        limited=o;limited.tick_limit=1;
        assert(q && pt_editor_studio_begin_invert_queued(&owner,&limited,SIZE_MAX,q)==PT_RENDER_OK);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        for(i=0;i<64;++i) {if(pt_editor_studio_step(&owner,17)==PT_PUMP_ERROR)break;assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_EMPTY);}
        assert(i<64 && !owner.invert_song && !owner.queue && stops==1 && !e->sampler.bytes);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_DONE && pt_studio_queue_close(q)==PT_QUEUE_OK && live==baseline);
    }
    calls=0;assert(pt_editor_studio_begin_invert(&owner,&o,SIZE_MAX)==PT_RENDER_OK);count=calls;pt_editor_studio_stop(&owner);
    for(i=1;i<=count;++i) {
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);assert(q);calls=0;fail_at=i;
        assert(pt_editor_studio_begin_invert_queued(&owner,&o,SIZE_MAX,q)==PT_RENDER_MEMORY && !owner.invert_song && !owner.queue);
        fail_at=0;assert(pt_studio_queue_close(q)==PT_QUEUE_OK && live==baseline);
    }
    {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned stops=0;
        assert(q && pt_editor_studio_begin_invert_queued(&owner,&o,SIZE_MAX,q)==PT_RENDER_OK);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        for(i=0;i<3;++i)assert(pt_editor_studio_step(&owner,17)==PT_PUMP_PROGRESS);
        pt_editor_dispose(e);assert(!owner.invert_song && !owner.queue && stops==1 && !e->sampler.bytes);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_DONE && pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    pt_editor_studio_detach(&owner);pt_document_release(&d);free(e);
    assert(!live);
    for(i=0;i<8192;++i)assert(master[i]==(i?(int32_t)(i%127)-63:17));
}
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_editor *e;
    incremental_cases();e=calloc(1,sizeof(*e));
    struct pt_editor_studio owner={0},second={0};struct pt_render_options options={0};
    int32_t pcm[4]={17,-93,30,40};unsigned baseline,done,action;const struct pt_pcm *out;
    assert(e);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_editor_init(e,&d.project));pt_sampler_init(&e->sampler,&a,1024*1024);
    d.project.samples[0].pcm=(struct pt_pcm){pcm,4,4,48000,1,8};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=4;d.project.channels.track[0].pan=0;
    d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;d.project.events[0].instrument=1;d.project.events[4].effect=15;d.project.events[0].effect=14;d.project.events[0].parameter=255;
    options.rate=48000;options.bits=24;options.tracks=1;options.gain_q16=65536;options.tick_limit=1000;options.frame_limit=1000000;
    assert(pt_editor_studio_attach(&owner,e) && !pt_editor_studio_attach(&second,e));
    assert(pt_editor_studio_start(&owner,&options)==PT_RENDER_EFFECT && !owner.song);
    baseline=live;
    assert(pt_editor_studio_start_invert(&owner,&options,1)==PT_RENDER_MEMORY && !owner.invert_song && live==baseline);
    d.project.samples[0].pcm.bits=24;
    assert(pt_editor_studio_start_invert(&owner,&options,SIZE_MAX)==PT_RENDER_SAMPLE && !owner.invert_song);
    assert(d.project.samples[0].pcm.bits==24 && pcm[0]==17);d.project.samples[0].pcm.bits=8;
    start(&owner,&options);assert(owner.invert_song && !owner.song && !e->sampler.bytes);
    pt_editor_studio_stop(&owner);baseline=live;
    start(&owner,&options);pt_editor_key(e,0x4d,0);assert(owner.invert_song);e->row=0;e->editing=1;
    pt_editor_key(e,0x31,0);assert(!owner.invert_song && live==baseline);
    /* Undo is also guarded while pins are active. */
    d.project.events[0].pitch=428;start(&owner,&options);pt_editor_key(e,0x31,8);assert(!owner.invert_song);
    /* Queued edits discard waiting audio, but never invalidate consumer memory. */
    for(action=0;action<3;++action) {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned i;
        assert(q && pt_editor_studio_start_invert_queued(&owner,&options,SIZE_MAX,q)==PT_RENDER_OK);
        for(i=0;i<50;++i) {assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
            if(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK)break;}
        assert(i<50 && out->data[0]==(17*65536));
        for(i=0;i<20;++i)assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
        assert(owner.pump.held);e->row=0;
        if(action==0)pt_editor_key(e,0x31,0);
        else if(action==1)pt_editor_key(e,0x31,8);
        else pt_editor_studio_stop(&owner);
        assert(!owner.invert_song && !owner.queue && !owner.pump.held && out->data[0]==(17*65536));
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_DONE);
        assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
        d.project.events[0].pitch=428;
    }
    /* Real sample edit/undo keys stop private voices before master publication. */
    for(action=0;action<2;++action) {
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned i,stops=0;
        int32_t held;
        assert(q && pt_editor_studio_start_invert_queued(&owner,&options,SIZE_MAX,q)==PT_RENDER_OK);
        for(i=0;i<50;++i) {assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
            if(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK)break;}
        assert(i<50);held=out->data[0];assert(held==(action?34:17)*65536);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        e->panel=5;e->sample=1;pt_editor_sample_all(e);
        pt_editor_key(e,action?0x31:0x24,action?8:0);
        assert(!owner.invert_song && !owner.song && !owner.queue && stops==1);
        assert(d.project.samples[0].pcm.data[0]==(action?17:34) && out->data[0]==held);
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK && pt_studio_queue_close(q)==PT_QUEUE_OK);
        assert(pcm[0]==17 && pcm[1]==-93);
    }
    e->panel=0;
    /* Defensive stale generation also requests output shutdown on pump failure. */
    {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned i,stops=0;
        assert(q && pt_editor_studio_start_invert_queued(&owner,&options,SIZE_MAX,q)==PT_RENDER_OK);
        for(i=0;i<50;++i) {assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
            if(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK)break;}
        assert(i<50 && out->data[0]==17*65536);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        assert(pt_editor_studio_step(&owner,0)==PT_PUMP_ERROR && owner.invert_song && !stops);
        assert(pt_sampler_edit(&e->sampler,&d.project,&e->history,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
        assert(pt_editor_studio_step(&owner,17)==PT_PUMP_ERROR);
        assert(!owner.invert_song && !owner.queue && !owner.output_stop && stops==1 && out->data[0]==17*65536);
        pt_editor_studio_stop(&owner);assert(stops==1);
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK && pt_studio_queue_close(q)==PT_QUEUE_OK);
        assert(pt_pattern_undo(&d.project,&e->history,-1)==PT_EDIT_OK);
    }
    /* Editor stop requests reach an output with no host lease but an odd tail. */
    for(action=0;action<3;++action) {
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);struct pt_amigus_session session={0};
        int reset=1;struct pt_amigus_fifo_port port={&reset,port_space,port_write,port_reset};unsigned i;
        assert(q && pt_editor_studio_start_invert_queued(&owner,&options,SIZE_MAX,q)==PT_RENDER_OK);
        assert(pt_amigus_session_open(&session,q,&port,port_drain,NULL));
        assert(pt_editor_studio_bind_output_stop(&owner,output_stop,&session));
        assert(!pt_editor_studio_bind_output_stop(&owner,output_stop,&session));
        for(i=0;i<50 && !session.fifo.pack.held;++i) {
            assert(pt_editor_studio_step(&owner,1)!=PT_PUMP_ERROR);
            assert(pt_amigus_session_step(&session)!=PT_CONSUMER_ERROR);
        }
        assert(i<50);assert(pt_amigus_session_step(&session)==PT_CONSUMER_PROGRESS);
        assert(!session.consumer.leased && session.fifo.pack.held);
        e->row=0;
        if(action==0)pt_editor_key(e,0x31,0);
        else if(action==1)pt_editor_key(e,0x31,8);
        else pt_editor_studio_stop(&owner);
        assert(!owner.invert_song && !owner.queue && !owner.output_stop && session.phase==PT_AS_RESET);
        assert(output_stops==action+1);pt_editor_studio_stop(&owner);assert(output_stops==action+1);
        reset=0;assert(pt_amigus_session_step(&session)==PT_CONSUMER_WAIT);
        assert(!pt_amigus_session_detach(&session));reset=1;
        assert(pt_amigus_session_step(&session)==PT_CONSUMER_FINISHED);
        assert(pt_amigus_session_detach(&session));assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
        d.project.events[0].pitch=428;
    }
    /* Natural end retains the last queued block for draining. */
    {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned i,finished=0,received=0,stops=0;
        assert(q && pt_editor_studio_start_invert_queued(&owner,&options,SIZE_MAX,q)==PT_RENDER_OK);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        for(i=0;i<10000;++i) {
            enum pt_queue_result qr;
            if(pt_editor_studio_step(&owner,256)==PT_PUMP_FINISHED)finished=1;
            qr=pt_studio_queue_acquire(q,&out,&ticket);
            if(qr==PT_QUEUE_OK) {received+=out->frames;assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);}
            else if(qr==PT_QUEUE_DONE)break;
        }
        assert(i<10000 && finished && received && !owner.invert_song && owner.queue==q);
        assert(!stops && owner.output_stop);pt_editor_studio_stop(&owner);assert(stops==1);assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned i;
        assert(q && pt_editor_studio_start_invert_queued(&owner,&options,SIZE_MAX,q)==PT_RENDER_OK);
        for(i=0;i<50;++i) {assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
            if(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK)break;}
        assert(i<50);assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&output_stops));
        pt_editor_dispose(e);assert(output_stops==4 && !owner.output_stop);
        assert(!owner.invert_song && !owner.queue && !e->sampler.bytes && out->data[0]==(17*65536));
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
        assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    assert(pt_editor_studio_pull(&owner,1,&out,&done)==PT_RENDER_OK && done && !out);
    pt_editor_studio_detach(&owner);assert(!e->before_change && !owner.editor);
    pt_document_release(&d);free(e);assert(!live && pcm[0]==17 && pcm[1]==-93);
    puts("EDITOR INVERT STUDIO PASS: incremental readiness/cancellation/header guards, private bank stopped by pattern/sample edits, undo/dispose, held leases, stale output shutdown, natural drain, refusal and cleanup");return 0;
}
