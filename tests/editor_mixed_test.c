#define PT_TEST_MIXED_EXEC
#include "mixed_owner_test.c"
#include "../src/editor/editor_mixed.h"
#include "../src/platform/sample_import.h"
static int editor_occupied(void *c){(void)c;return 1;}
static void editor_mixed_case(unsigned bits,unsigned mode)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;
    struct pt_editor *e=malloc(sizeof(*e));struct fixture *f=malloc(sizeof(*f));
    struct pt_editor_mixed o={0},other={0};struct pt_sampler_paula pb={0};struct pt_sampler_wavetable ab={0};
    struct pt_paula_voices pv={0};struct pt_wavetable_voices av={0};struct driver d={0};struct wave_driver wd={0};
    struct pt_paula_voice_api pa={&d,mixed_start,stop,mixed_control};
    struct pt_wavetable_voice_api wa={&wd,wave_start,wave_stop,wave_control,NULL};
    struct pt_render_options options={0};struct pt_paula_render_caps caps={3546895,124,65535};
    struct pt_playback_format format={8,0,0,0};struct pt_mixed_transport pump={0};struct pump_timer timer={0};
    struct pt_mixed_timer_api api={&timer,pump_read,pump_poll,pump_arm,pump_alarm_close,pump_counter_close,pump_signal};
    enum pt_mixed_owner_result r;int32_t pcm[2048];unsigned i,n=0,revision,generation,stops;size_t bytes,size,used;uint8_t *saved,*out;
    assert(e && f);for(i=0;i<2048;++i)pcm[i]=(int32_t)(i%120)+1;
    init(f,PT_AMIGUS_WAVETABLE);assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,0,4096,4096,f,bus_owned,bus_write));
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
    doc.project.channels.track[4].route=PT_PAULA;doc.project.channels.track[4].pan=0;
    doc.project.samples[0].pcm=(struct pt_pcm){pcm,2048,2048,8000,1,(uint8_t)bits};doc.project.samples[0].volume=64;
    doc.project.speed=1;doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    doc.project.events[7]=doc.project.events[4];doc.project.events[32+15].effect=15;
    assert(pt_editor_init(e,&doc.project));e->sampler.allocator=a;e->sampler.budget=1024*1024;
    assert(pt_editor_change_barrier(e,editor_occupied,NULL));assert(!pt_editor_mixed_attach(&o,e));
    assert(pt_editor_change_barrier(e,NULL,NULL));assert(pt_editor_mixed_attach(&o,e));
    assert(!pt_editor_mixed_attach(&o,e) && !pt_editor_mixed_attach(&other,e));
    assert(!pt_editor_change_barrier(e,editor_occupied,NULL));assert(pt_editor_mixed_service(&other)==PT_MIXED_OWNER_INVALID);
    assert(pt_sampler_paula_bind(&pb,&e->sampler,&doc.project,&d,chip_alloc,chip_free,4096));
    assert(pt_sampler_wavetable_bind(&ab,&e->sampler,&doc.project,&f->cache));
    assert(pt_paula_voices_bind(&pv,&pb,&pa));assert(pt_wavetable_voices_bind(&av,&ab,&wa));
    assert(pt_paula_voices_bind_quiesce(&pv,quiesce,&d));assert(pt_wavetable_voices_bind_quiesce(&av,wave_quiesce,&wd));
    d.start_result=d.control_result=d.quiesce_result=wd.start_result=wd.stop_result=wd.barrier_result=1;
    for(i=0;i<4;++i)d.stop_result[i]=1;
    output_count=0;
    options.rate=48000;options.bits=24;options.tracks=(1U<<4)|(1U<<7);options.gain_q16=65536;options.tick_limit=100;options.frame_limit=100000;
    pb.project=NULL;assert(pt_editor_mixed_begin(&o,&pv,&av,&options,&caps,&format)==PT_MIXED_OWNER_INVALID);pb.project=&doc.project;
    ab.sampler=NULL;assert(pt_editor_mixed_begin(&o,&pv,&av,&options,&caps,&format)==PT_MIXED_OWNER_INVALID);ab.sampler=&e->sampler;
    assert(pt_editor_mixed_begin(&o,&pv,&av,&options,&caps,&format)==PT_MIXED_OWNER_PREPARING);
    assert(pt_editor_mixed_begin(&o,&pv,&av,&options,&caps,&format)==PT_MIXED_OWNER_INVALID);
    if(mode==0) {
        d.quiesce_result=wd.barrier_result=0;assert(!pt_editor_prepare_change(e) && o.owner);
        assert(!pt_editor_dispose(e) && !pt_editor_mixed_detach(&o));
        d.quiesce_result=wd.barrier_result=1;assert(pt_editor_prepare_change(e) && !o.owner);goto closed;
    }
    do{r=pt_editor_mixed_prepare(&o,NULL);assert(++n<100);}while(r==PT_MIXED_OWNER_PREPARING);
    assert(r==PT_MIXED_OWNER_OK);timer.clock=(struct mixed_counter){100,48000,0,mode==3?0:1};
    assert(pt_editor_mixed_start(&o,&pump,1000,0,&api)==PT_MIXED_OWNER_INVALID && !o.transport && !timer.clock.reads);
    r=pt_editor_mixed_start(&o,&pump,1000,128,&api);
    assert(o.transport==&pump && pump.active && r==(mode==3?PT_MIXED_OWNER_CLOCK:PT_MIXED_OWNER_OK));
    assert(pt_editor_mixed_prepare(&o,NULL)==PT_MIXED_OWNER_INVALID);
    assert(pt_editor_mixed_start(&o,&pump,1000,128,&api)==PT_MIXED_OWNER_INVALID);
    if(mode==3){assert(pt_editor_mixed_service(&o)==PT_MIXED_OWNER_CLOCK);goto retained;}
    n=0;
    do {
        r=pt_editor_mixed_service(&o);assert(++n<2000);
        assert(r==PT_MIXED_OWNER_PREPARING || r==PT_MIXED_OWNER_WAITING || r==PT_MIXED_OWNER_DONE);
        if(r==PT_MIXED_OWNER_WAITING){assert(timer.pending && pt_editor_mixed_signal(&o)==32);timer.clock.ticks=timer.deadline;}
        if(mode==1 && timer.pending)break;
        if(mode==2 && d.starts && wd.starts)break;
    }while(r!=PT_MIXED_OWNER_DONE);
    if(mode==4){assert(r==PT_MIXED_OWNER_DONE && o.owner && o.transport);goto retained;}
    assert(mode==1 || (mode==2 && d.live && wd.starts));
    revision=e->history.revision;generation=e->sampler.generation;bytes=e->sampler.bytes;stops=d.stops;
    pt_editor_key(e,0x4d,0);assert(d.stops==stops); /* Navigation doesn't close. */
    e->project->channels.selected=4;
    assert(pt_project_size(&doc.project,&size)==PT_PROJECT_OK);saved=malloc(size);out=malloc(size);assert(saved && out);
    assert(pt_project_encode(&doc.project,saved,size,&used)==PT_PROJECT_OK);
    d.stop_result[0]=0;wd.stop_result=0;d.quiesce_result=wd.barrier_result=0;
    e->project->channels.selected=4;e->row=0;e->editing=1;e->panel=0;
    pt_editor_key(e,0x46,0);assert(e->history.revision==revision && doc.project.events[4].kind==PT_NOTE_PERIOD);
    e->panel=5;pt_editor_sample_all(e);pt_editor_key(e,0x13,0);
    assert(e->sampler.generation==generation && e->sampler.bytes==bytes);
    assert(pt_editor_sample_file_import(e,"no-file.raw",1,0,NULL)==PT_EDIT_CONFLICT);
    pt_editor_key(e,0x31,8);assert(e->history.revision==revision);
    assert(!pt_editor_dispose(e) && !pt_editor_mixed_detach(&o) && o.owner && o.transport && e->change_ready);
    assert(pt_project_encode(&doc.project,out,size,&used)==PT_PROJECT_OK && !memcmp(out,saved,size));free(out);free(saved);
    d.stop_result[0]=wd.stop_result=1;timer.alarm_close_result=1;
    assert(!pt_editor_prepare_change(e) && o.owner && !timer.counter_closed); /* Readers still retain. */
    d.quiesce_result=wd.barrier_result=1;
retained:
    timer.counter_close_result=0;
    assert(!pt_editor_prepare_change(e) && o.transport && !timer.counter_closed);
    timer.alarm_close_result=1;
    assert(!pt_editor_prepare_change(e) && !o.owner && o.transport && timer.alarm_closed && !timer.counter_closed);
    timer.counter_close_result=1;assert(pt_editor_prepare_change(e) && !o.owner && !o.transport && !pump.active);
closed:
    assert(pt_editor_dispose(e) && !e->sampler.bytes);
    assert(pt_editor_mixed_detach(&o) && !e->change_ready && !o.editor);
    assert(pt_editor_mixed_stop(&o) && pt_editor_mixed_detach(&o));
    assert(pt_amigus_wavetable_cache_detach(&f->cache));assert(pt_amigus_reservation_close(&f->reservation));
    pt_document_release(&doc);free(e);free(f);
}
int editor_mixed_fixture(void)
{unsigned bits,mode;(void)mixed_owner_fixture;for(bits=8;bits<=24;bits+=8)for(mode=0;mode<5;++mode)editor_mixed_case(bits,mode);
 puts("EDITOR MIXED PASS:15 binding scenarios; preparation/live/timer-failure/DONE, edits/import/undo/disposal veto and alarm/reader/counter retention; injected timers/voices, no UI/output binding");return 0;}
#ifndef PT_TEST_EDITOR_MIXED_EXEC
int main(void){return editor_mixed_fixture();}
#endif
