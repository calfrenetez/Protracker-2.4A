#define AMIGUS_CAPTURE_ENTRY reserved_capture_fixture
#include "amigus_capture_test.c"
#undef AMIGUS_CAPTURE_ENTRY
#include "../src/editor/editor_capture.h"
static void setup_editor(struct pt_editor *e,struct pt_document *d)
{
    memset(e,0,sizeof(*e));pt_document_init(d,&allocator);
    assert(pt_document_new(d,4,SIZE_MAX)==PT_PROJECT_OK && pt_editor_init(e,&d->project));
    pt_sampler_release(&e->sampler);pt_sampler_init(&e->sampler,&allocator,1024*1024);
    e->song.allocator=allocator;pt_document_init(&e->sample_source,&allocator);
}
static void mutation_barrier(void)
{
    unsigned action;
    for(action=0;action<3;++action) {
        struct pt_editor *e=malloc(sizeof(*e));struct pt_document d;struct pt_editor_capture o={0},other={0};
        struct resources f={0};struct fake in={0};struct pt_capture_input p=input(&in);
        struct pt_amigus_reservation r={0};struct pt_amigus_reservation_api a=api(&f);
        uint32_t revision;size_t before;unsigned sample;struct pt_event event;
        assert(e);setup_editor(e,&d);e->editing=1;pt_editor_key(e,0x31,0);e->row=0;
        revision=e->history.revision;event=d.project.events[0];sample=e->sample;
        assert(pt_editor_capture_attach(&o,e) && !pt_editor_capture_attach(&other,e));
        assert(pt_amigus_reservation_open(&r,&a,0)==PT_AMIGUS_RESERVED);
        before=allocations;p.format.bits=16;
        assert(!pt_editor_capture_start(&o,&r,&p,24,2,48000,4,32));
        assert(!r.access && !pt_editor_capture_busy(&o) && allocations==before && !in.starts);
        assert(e->history.revision==revision && e->sample==sample);
        p.format.bits=24;
        assert(pt_editor_capture_start(&o,&r,&p,24,2,48000,4,32));
        in.start_rc=in.read_rc=1;in.frames=2;
        assert(pt_editor_capture_step(&o)==PT_CS_PENDING && pt_editor_capture_step(&o)==PT_CS_PENDING);
        assert(!pt_editor_capture_pcm(&o) && pt_editor_capture_publish(&o,"early")==PT_EDIT_INVALID);
        pt_editor_key(e,0x4d,0);assert(o.device.session.phase==PT_CS_RECORD);e->row=0;
        if(action==0)pt_editor_key(e,0x32,0);
        if(action==1)pt_editor_key(e,0x31,8);
        if(action==2)assert(!pt_editor_dispose(e));
        assert(o.device.session.phase==PT_CS_STOP && e->history.revision==revision && !memcmp(&event,&d.project.events[0],sizeof(event)));
        assert(pt_editor_capture_step(&o)==PT_CS_PENDING && r.access && !pt_editor_prepare_change(e));
        in.stop_rc=1;assert(pt_editor_capture_step(&o)==PT_CS_COMPLETE && !r.access);
        assert(pt_editor_capture_pcm(&o) && pt_editor_capture_pcm(&o)->frames==2 && pt_editor_capture_busy(&o));
        assert(!pt_editor_prepare_change(e) && !pt_editor_dispose(e));
        assert(!pt_editor_capture_start(&o,&r,&p,24,2,48000,4,32));
        before=live;fail_allocate=1;
        assert(pt_editor_capture_publish(&o,"captured")==PT_EDIT_CAPACITY);fail_allocate=0;
        assert(live==before && pt_editor_capture_pcm(&o) && e->history.revision==revision && e->sample==sample);
        assert(pt_editor_capture_publish(&o,"captured")==PT_EDIT_OK && !pt_editor_capture_busy(&o));
        assert(e->sample==32 && e->sample_end==2 && d.project.sample_count==32);
        assert(d.project.samples[31].pcm.bits==24 && d.project.samples[31].pcm.data[2]==1);
        pt_editor_key(e,0x31,8);assert(d.project.sample_count==31);
        pt_editor_key(e,0x31,9);assert(d.project.sample_count==32 && d.project.samples[31].pcm.data[2]==1);
        assert(pt_editor_capture_detach(&o) && !e->change_ready);
        assert(pt_editor_dispose(e) && pt_amigus_reservation_close(&r));pt_document_release(&d);free(e);assert(!live);
    }
}
static void abort_detach(void)
{
    struct pt_editor *e=malloc(sizeof(*e));struct pt_document d;struct pt_editor_capture o={0};
    struct resources f={0};struct fake in={0};struct pt_capture_input p=input(&in);
    struct pt_amigus_reservation r={0};struct pt_amigus_reservation_api a=api(&f);size_t before;
    assert(e);setup_editor(e,&d);before=live;
    assert(pt_editor_capture_attach(&o,e) && pt_amigus_reservation_open(&r,&a,0)==PT_AMIGUS_RESERVED);
    p.format=(struct pt_capture_format){16,1,22050};
    assert(pt_editor_capture_start(&o,&r,&p,16,1,22050,4,16));
    assert(pt_editor_capture_step(&o)==PT_CS_PENDING); /* pending start */
    assert(!pt_editor_capture_detach(&o) && e->change_ready && r.access && live==before+1);
    in.stop_rc=2;assert(pt_editor_capture_step(&o)==PT_CS_ERROR && !pt_editor_dispose(e));
    assert(!pt_editor_capture_detach(&o));in.stop_rc=1;
    assert(pt_editor_capture_step(&o)==PT_CS_ERROR && !pt_editor_capture_busy(&o));
    assert(live==before && !pt_editor_capture_pcm(&o) && !r.access);
    assert(o.fault==PT_CS_STOP_FAULT && pt_editor_capture_step(&o)==PT_CS_ERROR);
    assert(pt_editor_capture_detach(&o) && pt_editor_dispose(e));
    assert(pt_amigus_reservation_close(&r));pt_document_release(&d);free(e);assert(!live);
}
#ifndef EDITOR_CAPTURE_ENTRY
#define EDITOR_CAPTURE_ENTRY main
#endif
int EDITOR_CAPTURE_ENTRY(void)
{
    assert(reserved_capture_fixture()==0);mutation_barrier();abort_detach();
    puts("EDITOR CAPTURE PASS: edit/undo/dispose veto, nonempty recording retained, explicit publish/discard, failed append retry, exact low bits and asynchronous detach; injected input only");return 0;
}
