#define PT_TEST_EDITOR_MIXED_EXEC
#include "editor_mixed_test.c"
#include "../src/editor/editor_mixed_checked.h"
#include "../src/editor/editor_mixed_establish.h"
#include "../src/editor/mixed_owner_established_internal.h"
#include "../src/editor/mixed_owner_state_internal.h"
struct editor_checked_fixture {
    struct pt_editor *editor;struct pt_editor_mixed *binding;struct pt_mixed_established *control;
    struct pt_document doc;struct fixture bus;
    struct pt_sampler_paula pb;struct pt_sampler_wavetable ab;
    struct pt_paula_voices pv;struct pt_wavetable_voices av;
    struct driver d;struct wave_driver wd;
    struct pt_paula_voice_api pa;struct pt_wavetable_voice_api wa;
    struct pt_render_options options;struct pt_paula_render_caps caps;struct pt_playback_format format;
    struct pt_mixed_transport pump;struct pump_timer timer;struct pt_mixed_timer_api api;
    int32_t pcm[2048];void *alias;
    unsigned calls,live,releases,alias_releases,hook_call,release_hook,reentries,checked;
};
static void checked_editor_reentry(struct editor_checked_fixture *g)
{
    struct pt_editor *e=g->editor;unsigned revision=e->history.revision,generation=e->sampler.generation;
    assert(!pt_editor_prepare_change(e));e->editing=1;e->row=0;e->panel=0;e->project->channels.selected=4;
    pt_editor_key(e,0x46,0);
    assert(e->history.revision==revision&&e->sampler.generation==generation&&e->project->events[4].kind==PT_NOTE_PERIOD);
    assert(!pt_editor_dispose(e)&&!pt_editor_mixed_detach(g->binding));
    assert(g->binding->owner_finish_context==g->control&&g->binding->owner_finish);++g->reentries;
}
static void *checked_editor_allocate(void *context,size_t bytes)
{
    struct editor_checked_fixture *g=context;void *p;++g->calls;
    if(g->checked){assert(g->binding->owner_finish_context==g->control&&g->binding->owner_finish);
        if(g->calls==g->hook_call)checked_editor_reentry(g);}
    if(g->alias)return g->alias;
    p=malloc(bytes);if(p)++g->live;return p;
}
static void checked_editor_release(void *context,void *p)
{
    struct editor_checked_fixture *g=context;
    if(p==g->alias){++g->alias_releases;return;}
    if(g->release_hook)checked_editor_reentry(g);
    assert(p&&g->live);--g->live;++g->releases;free(p);
}
static struct editor_checked_fixture *checked_editor_make(unsigned bits)
{
    struct editor_checked_fixture *g=calloc(1,sizeof(*g));struct pt_allocator a={NULL,fast_alloc,fast_free};
    struct pt_editor_mixed_establish *master=calloc(1,sizeof(*master));struct pt_sampler_storage_span parent;
    enum pt_establish_result r;unsigned i,n=0;assert(g&&master);
    g->editor=malloc(sizeof(*g->editor));g->binding=calloc(1,sizeof(*g->binding));g->control=calloc(1,sizeof(*g->control));
    assert(g->editor&&g->binding&&g->control);
    for(i=0;i<2048;++i)g->pcm[i]=((int32_t)(i%120)-60)*(bits==8?1:bits==16?251:65537);
    init(&g->bus,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&g->bus.cache,&g->bus.reservation,0,4096,4096,&g->bus,bus_owned,bus_write));
    pt_document_init(&g->doc,&a);assert(pt_document_new(&g->doc,16,SIZE_MAX)==PT_PROJECT_OK);
    g->doc.project.sample_count=3;
    for(i=0;i<16;++i)g->doc.project.channels.track[i].route=PT_AMIGUS;
    g->doc.project.channels.track[4].route=PT_PAULA;g->doc.project.channels.track[4].pan=0;
    for(i=0;i<3;++i){g->doc.project.samples[i].pcm=(struct pt_pcm){g->pcm,2048,2048,8000,1,(uint8_t)bits};g->doc.project.samples[i].volume=64;}
    g->doc.project.speed=1;g->doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    g->doc.project.events[7]=g->doc.project.events[4];g->doc.project.events[16+7]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    g->doc.project.events[32+15].effect=15;
    assert(pt_editor_init(g->editor,&g->doc.project));
    g->editor->sampler.allocator=(struct pt_allocator){g,checked_editor_allocate,checked_editor_release};g->editor->sampler.budget=1024*1024;
    assert(pt_editor_mixed_attach(g->binding,g->editor));parent=(struct pt_sampler_storage_span){g,sizeof(*g)};
    assert(pt_editor_mixed_establish_begin(master,g->binding,&parent,1)==PT_ESTABLISH_PENDING);
    do{r=pt_editor_mixed_establish_step(master,256);assert(++n<10000);}while(r==PT_ESTABLISH_PENDING);
    assert(r==PT_ESTABLISH_READY&&g->live==4&&!g->binding->owner);
    assert(pt_editor_mixed_stop(g->binding)&&g->live==3);free(master);
    assert(pt_sampler_paula_bind(&g->pb,&g->editor->sampler,&g->doc.project,&g->d,chip_alloc,chip_free,4096));
    assert(pt_sampler_wavetable_bind(&g->ab,&g->editor->sampler,&g->doc.project,&g->bus.cache));
    g->pa=(struct pt_paula_voice_api){&g->d,mixed_start,stop,mixed_control};
    g->wa=(struct pt_wavetable_voice_api){&g->wd,wave_start,wave_stop,wave_control,NULL};
    assert(pt_paula_voices_bind(&g->pv,&g->pb,&g->pa)&&pt_wavetable_voices_bind(&g->av,&g->ab,&g->wa));
    assert(pt_paula_voices_bind_quiesce(&g->pv,quiesce,&g->d)&&pt_wavetable_voices_bind_quiesce(&g->av,wave_quiesce,&g->wd));
    g->d.start_result=g->d.control_result=g->d.quiesce_result=g->wd.start_result=g->wd.stop_result=g->wd.barrier_result=1;
    for(i=0;i<4;++i)g->d.stop_result[i]=1;
    g->options=(struct pt_render_options){100000,100,48000,65536,(1U<<4)|(1U<<7),0,0,24,0,0,0,0,0};
    g->caps=(struct pt_paula_render_caps){3546895,124,65535};g->format=(struct pt_playback_format){8,0,0,0};
    g->api=(struct pt_mixed_timer_api){&g->timer,pump_read,pump_poll,pump_arm,pump_alarm_close,pump_counter_close,pump_signal};
    output_count=0;g->checked=1;return g;
}
static enum pt_mixed_owner_result checked_editor_begin(struct editor_checked_fixture *g)
{return pt_editor_mixed_checked_begin(g->binding,g->control,&g->pv,&g->av,&g->options,&g->caps,&g->format,
    (struct pt_sampler_storage_span){g,sizeof(*g)},64);}
static void checked_editor_drop(struct editor_checked_fixture *g)
{
    g->alias=NULL;g->hook_call=g->release_hook=0;
    assert(pt_editor_mixed_stop(g->binding)&&!g->control->storage.memory.active&&!g->binding->owner_finish);
    assert(pt_editor_mixed_detach(g->binding)&&pt_editor_dispose(g->editor)&&!g->live&&!g->alias_releases);
    assert(pt_paula_voices_close(&g->pv)&&pt_wavetable_voices_close(&g->av));
    assert(pt_amigus_wavetable_cache_detach(&g->bus.cache)&&pt_amigus_reservation_close(&g->bus.reservation));
    pt_document_release(&g->doc);free(g->editor);free(g->binding);free(g->control);free(g);
}
static void checked_editor_case(unsigned bits,unsigned mode)
{
    struct editor_checked_fixture *g=checked_editor_make(bits);struct pt_sample_version *masters[3];
    struct pt_editor_mixed_establish *probe=calloc(1,sizeof(*probe));enum pt_mixed_owner_result r;
    unsigned n=0,revision=g->editor->history.revision,generation=g->editor->sampler.generation;size_t bytes,used;
    uint8_t *before,*after;void *child=NULL;assert(probe);
    memcpy(masters,g->editor->sampler.current,sizeof(masters));
    assert(pt_project_size(&g->doc.project,&bytes)==PT_PROJECT_OK);before=malloc(bytes);after=malloc(bytes);assert(before&&after);
    assert(pt_project_encode(&g->doc.project,before,bytes,&used)==PT_PROJECT_OK&&used==bytes);
    if(mode==10)g->hook_call=g->calls+1;
    r=checked_editor_begin(g);
    assert(r==(mode==10?PT_MIXED_OWNER_MEMORY:PT_MIXED_OWNER_PREPARING));
    assert(g->binding->owner_finish&&g->binding->owner_finish_context==g->control);
    assert(checked_editor_begin(g)==PT_MIXED_OWNER_INVALID);
    assert(pt_editor_mixed_establish_begin(probe,g->binding,NULL,0)==PT_ESTABLISH_INVALID);free(probe);
    if(mode==10){assert(!g->binding->owner&&g->reentries);goto confirmed;}
    assert(g->binding->owner&&g->control->storage.memory.active&&g->live==4&&!output_count);
    if(mode==0)goto close;
    if(mode==11){g->hook_call=g->calls+1;r=pt_editor_mixed_prepare(g->binding,NULL);
        assert((r==PT_MIXED_OWNER_STALE||r==PT_MIXED_OWNER_MEMORY)&&g->reentries);goto close;}
    assert(pt_editor_mixed_prepare(g->binding,NULL)==PT_MIXED_OWNER_PREPARING&&g->control->startup);
    if(mode==1)goto close;
    if(mode==2){while(g->control->startup)assert(pt_editor_mixed_prepare(g->binding,NULL)==PT_MIXED_OWNER_PREPARING&&++n<10000);
        assert(!g->binding->owner->analyzed);goto close;}
    do{r=pt_editor_mixed_prepare(g->binding,NULL);assert(++n<10000&&!output_count);}while(r==PT_MIXED_OWNER_PREPARING);
    assert(r==PT_MIXED_OWNER_OK&&g->binding->owner->pin[0]&&g->binding->owner->pin[1]&&!g->binding->owner->pin[2]);
    if(mode==3||mode>=8)goto close;
    g->timer.clock=(struct mixed_counter){100,48000,0,mode==6?0:1};
    r=pt_editor_mixed_start(g->binding,&g->pump,1000,128,&g->api);
    assert(g->binding->transport==&g->pump&&g->pump.active&&r==(mode==6?PT_MIXED_OWNER_CLOCK:PT_MIXED_OWNER_OK));
    if(mode==6)goto close;
    do{r=pt_editor_mixed_service(g->binding);assert(++n<15000);
        assert(r==PT_MIXED_OWNER_PREPARING||r==PT_MIXED_OWNER_WAITING||r==PT_MIXED_OWNER_DONE);
        if(r==PT_MIXED_OWNER_WAITING){assert(g->timer.pending);g->timer.clock.ticks=g->timer.deadline;}
        if(mode==4&&g->timer.pending)break;
        if(mode==5&&g->d.starts&&g->wd.starts)break;
    }while(r!=PT_MIXED_OWNER_DONE);
    assert(mode!=7||r==PT_MIXED_OWNER_DONE);
close:
    if(mode==9){/* Private exact residual child injects zero-child finish refusal. */
        child=g->control->storage.memory.allocator.allocate(g->control->storage.memory.allocator.context,32);assert(child);}
    if(mode==12)g->release_hook=1;
    g->d.quiesce_result=g->wd.barrier_result=0;
    assert(!pt_editor_prepare_change(g->editor)&&g->binding->owner&&g->control->storage.memory.active);
    g->editor->editing=1;g->editor->row=0;g->editor->panel=0;g->doc.project.channels.selected=4;
    pt_editor_key(g->editor,0x46,0);
    assert(g->editor->history.revision==revision&&g->editor->sampler.generation==generation);
    assert(!pt_editor_dispose(g->editor)&&!pt_editor_mixed_detach(g->binding));
    g->d.quiesce_result=1;assert(!pt_editor_prepare_change(g->editor)&&g->binding->owner);
    g->wd.barrier_result=1;
    if(mode==8){g->bus.reservation.interrupt=1;
        assert(!pt_editor_prepare_change(g->editor)&&g->binding->owner&&g->control->storage.memory.active);g->bus.reservation.interrupt=0;}
    if(g->binding->transport){
        assert(!pt_editor_prepare_change(g->editor)&&!g->binding->owner&&g->binding->transport&&g->control->storage.memory.active);
        assert(g->binding->owner_finish&&g->live==3&&!g->timer.counter_closed);
        g->timer.alarm_close_result=1;assert(!pt_editor_prepare_change(g->editor)&&g->timer.alarm_closed&&!g->timer.counter_closed);
        assert(g->control->storage.memory.active&&g->binding->owner_finish);g->timer.counter_close_result=1;
    }
    if(child){assert(!pt_editor_prepare_change(g->editor)&&!g->binding->owner&&g->control->storage.memory.active&&g->binding->owner_finish);
        g->control->storage.memory.allocator.release(g->control->storage.memory.allocator.context,child);}
confirmed:
    assert(pt_editor_prepare_change(g->editor)&&!g->binding->owner&&!g->binding->transport&&!g->binding->owner_finish);
    assert(!g->binding->owner_finish_context&&!g->control->storage.memory.active&&g->live==3);
    assert(!memcmp(masters,g->editor->sampler.current,sizeof(masters)));
    g->doc.project.channels.selected=0;
    assert(pt_project_encode(&g->doc.project,after,bytes,&used)==PT_PROJECT_OK&&used==bytes&&!memcmp(before,after,bytes));
    g->release_hook=0;g->checked=0;g->editor->editing=1;g->editor->row=0;g->editor->panel=0;g->doc.project.channels.selected=4;
    pt_editor_key(g->editor,0x46,0);assert(g->editor->history.revision!=revision&&g->doc.project.events[4].kind==PT_NOTE_NONE);
    if(mode==12)assert(g->reentries);
    free(before);free(after);checked_editor_drop(g);
}
static void checked_editor_alias(unsigned bits,unsigned mode)
{
    struct editor_checked_fixture *g=checked_editor_make(bits);struct pt_editor_mixed before=*g->binding;
    struct pt_mixed_owner *out=NULL;struct pt_sampler_storage_span parent={g->binding,sizeof(*g->binding)};
    unsigned calls=g->calls;struct pt_editor *saved=malloc(sizeof(*saved));assert(saved);memcpy(saved,g->editor,sizeof(*saved));
    /* Original public extra-span contract still rejects publisher overlap. */
    assert(pt_mixed_owner_established_begin(g->control,&g->pv,&g->av,&g->options,&g->caps,&g->format,
        &g->editor->sampler.allocator,&parent,1,0,64,&g->binding->owner)==PT_MIXED_OWNER_INVALID);
    assert(pt_mixed_owner_established_begin_bound(g->control,&g->pv,&g->av,&g->options,&g->caps,&g->format,
        &g->editor->sampler.allocator,g->binding,sizeof(*g->binding),NULL,0,0,64,&out)==PT_MIXED_OWNER_INVALID);
    assert(!memcmp(&before,g->binding,sizeof(before))&&!out&&g->calls==calls);
    switch(mode){case 0:g->alias=g->binding;break;case 1:g->alias=&g->binding->owner;break;
        case 2:g->alias=(uint8_t *)g->binding+sizeof(*g->binding)-1;break;case 3:g->alias=g->editor;break;
        case 4:g->alias=g->control;break;default:g->alias=g;break;}
    assert(checked_editor_begin(g)==PT_MIXED_OWNER_MEMORY&&!g->binding->owner&&g->live==3&&!g->alias_releases);
    assert(!memcmp(saved,g->editor,sizeof(*saved)));free(saved);
    assert(g->binding->owner_finish_context==g->control&&g->binding->owner_finish);
    g->alias=NULL;g->checked=0;checked_editor_drop(g);
}
int main(void)
{
    unsigned bits,mode;assert(!editor_mixed_fixture());
    for(bits=8;bits<=24;bits+=8){for(mode=0;mode<13;++mode)checked_editor_case(bits,mode);
        for(mode=0;mode<6;++mode)checked_editor_alias(bits,mode);}
    puts("EDITOR CHECKED PASS:39 actual establishment/editor/checked-owner lifecycle cases;18 whole-control allocator-alias and unchanged public-admission cases;15 legacy transport regressions; host injected clocks/voices only");return 0;
}
