#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/editor/editor_studio.h"
#include "../src/core/amigus_session.h"
static unsigned live;
static void *allocate(void *c,size_t n) {void *p;(void)c;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;if(p){assert(live);--live;free(p);}}
static void start(struct pt_editor_studio *o,struct pt_render_options *options)
{
    unsigned i,done;const struct pt_pcm *out;
    assert(pt_editor_studio_start(o,options)==PT_RENDER_OK);
    for(i=0;i<100;++i) {assert(pt_editor_studio_pull(o,1,&out,&done)==PT_RENDER_OK && !done);if(out) {assert(out->data[0]==257);return;}}
    assert(0);
}
static unsigned output_stops;
static void count_stop(void *c) {++*(unsigned *)c;}
static int port_space(void *c) {(void)c;return 3;}
static int port_write(void *c,const uint32_t *p) {(void)c;(void)p;return 1;}
static int port_reset(void *c) {return *(int *)c;}
static int port_drain(void *c) {(void)c;return 0;}
static void output_stop(void *c) {++output_stops;pt_amigus_session_stop(c);}
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_editor *e=calloc(1,sizeof(*e));
    struct pt_editor_studio owner={0},second={0};struct pt_render_options options={0};
    int32_t pcm[4]={257,-513,1025,-2049};unsigned baseline,done,action;const struct pt_pcm *out;
    assert(e);pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_editor_init(e,&d.project));pt_sampler_init(&e->sampler,&a,1024*1024);
    d.project.samples[0].pcm=(struct pt_pcm){pcm,4,4,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=4;d.project.channels.track[0].pan=0;
    d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;d.project.events[0].instrument=1;d.project.events[4].effect=15;
    options.rate=48000;options.bits=24;options.tracks=1;options.gain_q16=65536;options.tick_limit=1000;options.frame_limit=1000000;
    assert(pt_editor_studio_attach(&owner,e) && !pt_editor_studio_attach(&second,e));
    start(&owner,&options);assert(owner.song && e->sampler.bytes);
    pt_editor_studio_stop(&owner);baseline=live;
    start(&owner,&options);pt_editor_key(e,0x4d,0);assert(owner.song);e->row=0;e->editing=1;
    pt_editor_key(e,0x31,0);assert(!owner.song && live==baseline);
    /* Undo is also guarded while pins are active. */
    d.project.events[0].pitch=428;start(&owner,&options);pt_editor_key(e,0x31,8);assert(!owner.song);
    /* Queued edits discard waiting audio, but never invalidate consumer memory. */
    for(action=0;action<3;++action) {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned i;
        assert(q && pt_editor_studio_start_queued(&owner,&options,q)==PT_RENDER_OK);
        for(i=0;i<50;++i) {assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
            if(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK)break;}
        assert(i<50 && out->data[0]==257);
        for(i=0;i<20;++i)assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
        assert(owner.pump.held);e->row=0;
        if(action==0)pt_editor_key(e,0x31,0);
        else if(action==1)pt_editor_key(e,0x31,8);
        else pt_editor_studio_stop(&owner);
        assert(!owner.song && !owner.queue && !owner.pump.held && out->data[0]==257);
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_DONE);
        assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
        d.project.events[0].pitch=428;
    }
    /* Editor stop requests reach an output with no host lease but an odd tail. */
    for(action=0;action<3;++action) {
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);struct pt_amigus_session session={0};
        int reset=1;struct pt_amigus_fifo_port port={&reset,port_space,port_write,port_reset};unsigned i;
        assert(q && pt_editor_studio_start_queued(&owner,&options,q)==PT_RENDER_OK);
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
        assert(!owner.song && !owner.queue && !owner.output_stop && session.phase==PT_AS_RESET);
        assert(output_stops==action+1);pt_editor_studio_stop(&owner);assert(output_stops==action+1);
        reset=0;assert(pt_amigus_session_step(&session)==PT_CONSUMER_WAIT);
        assert(!pt_amigus_session_detach(&session));reset=1;
        assert(pt_amigus_session_step(&session)==PT_CONSUMER_FINISHED);
        assert(pt_amigus_session_detach(&session));assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
        d.project.events[0].pitch=428;
    }
    /* Natural end retains the last queued block for draining. */
    {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned i,finished=0,received=0,stops=0;
        assert(q && pt_editor_studio_start_queued(&owner,&options,q)==PT_RENDER_OK);
        assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&stops));
        for(i=0;i<10000;++i) {
            enum pt_queue_result qr;
            if(pt_editor_studio_step(&owner,256)==PT_PUMP_FINISHED)finished=1;
            qr=pt_studio_queue_acquire(q,&out,&ticket);
            if(qr==PT_QUEUE_OK) {received+=out->frames;assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);}
            else if(qr==PT_QUEUE_DONE)break;
        }
        assert(i<10000 && finished && received && !owner.song && owner.queue==q);
        assert(!stops && owner.output_stop);pt_editor_studio_stop(&owner);assert(stops==1);assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    {struct pt_studio_queue *q=pt_studio_queue_open(&a,2);uint64_t ticket;unsigned i;
        assert(q && pt_editor_studio_start_queued(&owner,&options,q)==PT_RENDER_OK);
        for(i=0;i<50;++i) {assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
            if(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK)break;}
        assert(i<50);assert(pt_editor_studio_bind_output_stop(&owner,count_stop,&output_stops));
        pt_editor_dispose(e);assert(output_stops==4 && !owner.output_stop);
        assert(!owner.song && !owner.queue && !e->sampler.bytes && out->data[0]==257);
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
        assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    assert(pt_editor_studio_pull(&owner,1,&out,&done)==PT_RENDER_OK && done && !out);
    pt_editor_studio_detach(&owner);assert(!e->before_change && !owner.editor);
    pt_document_release(&d);free(e);assert(!live && pcm[0]==257);
    puts("EDITOR STUDIO PASS: real pinned song stopped by edit/undo/dispose, queued edit/undo/Stop/dispose held leases, natural drain, navigation and cleanup");return 0;
}
