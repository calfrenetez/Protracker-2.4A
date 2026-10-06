#define PT_TEST_EDITOR_CHECKED_EXEC
#include "editor_mixed_checked_test.c"
#include "../src/editor/editor_mixed_prepare_session.h"

struct session_fixture {
    struct editor_checked_fixture g;
    struct pt_editor_mixed_prepare_session session;
    struct pt_editor_mixed_prepare_session_inputs input;
    struct pt_editor_mixed_establish *master;
    struct pt_editor_mixed_bridges *bridges;
    unsigned allocation_hook,release_hook,fail_at,reentered;
};
static void session_reentry(struct session_fixture *f)
{
    struct pt_editor *e=f->g.editor;unsigned revision=e->history.revision,generation=e->sampler.generation;
    assert(pt_editor_mixed_prepare_session_advance(&f->session)==PT_EDITOR_MIXED_SESSION_FAULT);
    assert(!pt_editor_mixed_prepare_session_close(&f->session));
    assert(!pt_editor_prepare_change(e)&&!pt_editor_dispose(e));
    e->editing=1;e->panel=0;e->row=0;e->project->channels.selected=4;
    pt_editor_key(e,0x46,0);
    assert(revision==e->history.revision&&generation==e->sampler.generation);
    assert(e->project->events[4].kind==PT_NOTE_PERIOD);++f->reentered;
}
static void *session_allocate(void *context,size_t bytes)
{
    struct session_fixture *f=context;struct editor_checked_fixture *g=&f->g;void *p;
    ++g->calls;
    if(f->allocation_hook==g->calls){f->allocation_hook=0;session_reentry(f);}
    if(g->alias)return g->alias;
    if(f->fail_at==g->calls)return NULL;
    p=malloc(bytes);if(p)++g->live;return p;
}
static void session_release(void *context,void *p)
{
    struct session_fixture *f=context;struct editor_checked_fixture *g=&f->g;
    if(p==g->alias){++g->alias_releases;return;}
    if(f->release_hook){f->release_hook=0;session_reentry(f);}
    assert(p&&g->live);--g->live;++g->releases;free(p);
}
static struct session_fixture *session_make(unsigned bits)
{
    struct session_fixture *f=calloc(1,sizeof(*f));struct editor_checked_fixture *g=&f->g;
    struct pt_allocator a={NULL,fast_alloc,fast_free};unsigned i;assert(f);
    g->editor=malloc(sizeof(*g->editor));g->binding=calloc(1,sizeof(*g->binding));
    g->control=calloc(1,sizeof(*g->control));f->master=calloc(1,sizeof(*f->master));
    f->bridges=calloc(1,sizeof(*f->bridges));assert(g->editor&&g->binding&&g->control&&f->master&&f->bridges);
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
    g->editor->sampler.allocator=(struct pt_allocator){f,session_allocate,session_release};
    g->editor->sampler.budget=1024*1024;assert(pt_editor_mixed_attach(g->binding,g->editor));
    g->d.start_result=g->d.control_result=g->d.quiesce_result=g->wd.start_result=g->wd.stop_result=g->wd.barrier_result=1;
    for(i=0;i<4;++i)g->d.stop_result[i]=1;
    f->input.binding=g->binding;f->input.establish=f->master;f->input.bridges=f->bridges;f->input.checked=g->control;
    f->input.devices.backend=&g->bus.cache;f->input.devices.chip_context=&g->d;
    f->input.devices.chip_allocate=chip_alloc;f->input.devices.chip_release=chip_free;f->input.devices.chip_budget=4096;
    f->input.devices.paula=(struct pt_paula_voice_api){&g->d,mixed_start,stop,mixed_control};
    f->input.devices.amigus=(struct pt_wavetable_voice_api){&g->wd,wave_start,wave_stop,wave_control,NULL};
    f->input.devices.paula_quiesce=quiesce;f->input.devices.paula_quiesce_context=&g->d;
    f->input.devices.amigus_quiesce=wave_quiesce;f->input.devices.amigus_quiesce_context=&g->wd;
    f->input.options=(struct pt_render_options){100000,100,48000,65536,(1U<<4)|(1U<<7),0,0,24,0,0,0,0,0};
    f->input.caps=(struct pt_paula_render_caps){3546895,124,65535};f->input.format=(struct pt_playback_format){8,0,0,0};
    f->input.contexts=(struct pt_sampler_storage_span){f,sizeof(*f)};f->input.work=64;output_count=0;return f;
}
static void session_silent(struct session_fixture *f)
{
    struct editor_checked_fixture *g=&f->g;
    assert(!output_count&&!g->d.starts&&!g->wd.starts&&!g->bus.writes&&!g->binding->transport);
    assert(!g->timer.clock.reads&&!g->timer.arms&&!g->timer.polls&&!g->timer.alarm_closes&&!g->timer.counter_closes);
}
static enum pt_editor_mixed_session_result session_to(struct session_fixture *f,unsigned phase)
{
    enum pt_editor_mixed_session_result r=PT_EDITOR_MIXED_SESSION_PENDING;unsigned n=0;
    while(f->session.phase!=phase&&r==PT_EDITOR_MIXED_SESSION_PENDING){
        r=pt_editor_mixed_prepare_session_advance(&f->session);assert(++n<10000);session_silent(f);
    }
    return r;
}
static uint8_t *session_save(struct session_fixture *f,size_t *bytes)
{
    uint8_t *p;size_t used;
    assert(pt_project_size(&f->g.doc.project,bytes)==PT_PROJECT_OK);p=malloc(*bytes);assert(p);
    assert(pt_project_encode(&f->g.doc.project,p,*bytes,&used)==PT_PROJECT_OK&&used==*bytes);return p;
}
static void session_save_same(struct session_fixture *f,const uint8_t *before,size_t bytes)
{
    uint8_t *after;size_t n;f->g.doc.project.channels.selected=0;after=session_save(f,&n);
    assert(n==bytes&&!memcmp(before,after,bytes));free(after);
}
static void session_drop(struct session_fixture *f,unsigned disposed)
{
    struct editor_checked_fixture *g=&f->g;g->alias=NULL;f->allocation_hook=f->release_hook=f->fail_at=0;
    g->d.quiesce_result=g->wd.barrier_result=1;g->bus.healthy=1;
    assert(pt_editor_mixed_prepare_session_close(&f->session));
    assert(pt_editor_mixed_detach(g->binding));if(!disposed)assert(pt_editor_dispose(g->editor));
    assert(!g->live&&!g->alias_releases);
    assert(pt_amigus_wavetable_cache_detach(&g->bus.cache)&&pt_amigus_reservation_close(&g->bus.reservation));
    pt_document_release(&g->doc);free(f->master);free(f->bridges);free(g->control);free(g->binding);free(g->editor);free(f);
}
static void session_lifecycle(unsigned bits,unsigned mode)
{
    struct session_fixture *f=session_make(bits);struct editor_checked_fixture *g=&f->g;
    enum pt_editor_mixed_session_result r;struct pt_sample_version *masters[3];
    size_t bytes;uint8_t *before=session_save(f,&bytes);unsigned n=0,disposed=0,calls;void *child=NULL;
    if(mode==10)f->fail_at=1;
    if(mode==11)f->allocation_hook=1;
    if(mode==12)g->alias=&f->session;
    if(mode==13)g->alias=f->master;
    if(mode==14)g->alias=f->bridges;
    if(mode==15)g->alias=g->control;
    if(mode==16)g->alias=g->pcm;
    if(mode==17)f->input.devices.paula.start=NULL;
    if(mode==18)f->input.caps.clock_hz=0;
    if(mode==31)g->doc.project.events[4].kind=255;
    if(mode==33)g->editor->sampler.budget=1;
    r=pt_editor_mixed_prepare_session_begin(&f->session,&f->input);session_silent(f);
    if(mode>=10&&mode<=16){assert(r!=PT_EDITOR_MIXED_SESSION_PENDING);
        assert(g->binding->preparation_context==f->master&&!g->alias_releases);goto close;}
    assert(r==PT_EDITOR_MIXED_SESSION_PENDING);
    if(mode==31||mode==33){
        while(r==PT_EDITOR_MIXED_SESSION_PENDING){r=pt_editor_mixed_prepare_session_advance(&f->session);assert(++n<10000);session_silent(f);}
        assert(r==PT_EDITOR_MIXED_SESSION_ESTABLISH_ERROR&&!g->editor->sampler.current[0]);goto close;
    }
    if(mode==32){f->input.work=128;calls=g->calls;
        assert(pt_editor_mixed_prepare_session_advance(&f->session)==PT_EDITOR_MIXED_SESSION_CANCELLED&&g->calls==calls);
        f->input.work=64;goto close;}
    if(mode==0)goto close;
    if(mode==1){assert(pt_editor_mixed_prepare_session_advance(&f->session)==r);goto close;}
    if(mode==2||mode==19||mode==20){
        while(!f->master->job.job.owner&&r==PT_EDITOR_MIXED_SESSION_PENDING){
            if(mode==19&&f->master->job.slot==0&&f->master->job.phase>=3)f->fail_at=g->calls+1;
            if(mode==20&&f->master->job.slot==0&&f->master->job.phase>=3)f->allocation_hook=g->calls+1;
            r=pt_editor_mixed_prepare_session_advance(&f->session);assert(++n<10000);
        }
        if(mode==19||mode==20)assert(r!=PT_EDITOR_MIXED_SESSION_PENDING);
        goto close;
    }
    if(mode==3||mode==4){
        if(mode==3){assert(pt_editor_prepare_change(g->editor));
            assert(pt_editor_mixed_prepare_session_get(&f->session)==PT_EDITOR_MIXED_SESSION_CANCELLED);}
        else{assert(pt_editor_dispose(g->editor));disposed=1;
            assert(pt_editor_mixed_prepare_session_advance(&f->session)==PT_EDITOR_MIXED_SESSION_CANCELLED);}
        calls=g->calls;assert(pt_editor_mixed_prepare_session_advance(&f->session)==PT_EDITOR_MIXED_SESSION_CANCELLED&&g->calls==calls);
        goto close;
    }
    assert(session_to(f,PT_EDITOR_MIXED_SESSION_HANDOFF)==PT_EDITOR_MIXED_SESSION_PENDING);
    memcpy(masters,g->editor->sampler.current,sizeof(masters));assert(masters[0]&&masters[1]&&masters[2]);
    if(mode==5)goto close;
    if(mode==21)f->release_hook=1;
    if(mode==22)f->fail_at=g->calls+1;
    if(mode==23)f->allocation_hook=g->calls+1;
    if(mode==24)g->alias=&f->session;
    if(mode==25)g->alias=&f->input;
    r=pt_editor_mixed_prepare_session_advance(&f->session);session_silent(f);
    if(mode==17){assert(r==PT_EDITOR_MIXED_SESSION_BRIDGE_ERROR&&!f->session.bridges_bound);goto closed_masters;}
    if(mode==21){assert(r==PT_EDITOR_MIXED_SESSION_FAULT&&!f->session.bridges_bound);goto closed_masters;}
    if(mode==18||mode==22||mode==23||mode==24||mode==25){
        assert(r!=PT_EDITOR_MIXED_SESSION_PENDING&&f->session.bridges_bound&&g->binding->owner_finish_context==&f->session);
        assert(!g->binding->owner);
        g->alias=NULL;g->d.quiesce_result=g->wd.barrier_result=0;
        assert(!pt_editor_prepare_change(g->editor)&&!pt_editor_mixed_prepare_session_close(&f->session));
        g->d.quiesce_result=1;assert(!pt_editor_mixed_prepare_session_close(&f->session));
        assert(!f->bridges->paula.bridge&&f->bridges->amigus.bridge&&g->binding->owner_finish_context==&f->session);
        g->wd.barrier_result=1;goto closed_masters;
    }
    assert(r==PT_EDITOR_MIXED_SESSION_PENDING&&f->session.phase==PT_EDITOR_MIXED_SESSION_CHECKED);
    if(mode==6)goto closed_masters;
    if(mode==26)f->allocation_hook=g->calls+1;
    if(mode==27)g->bus.healthy=0;
    if(mode==28) { /* Actual change first fails both reader drains; no mutation. */
        unsigned revision=g->editor->history.revision;g->d.quiesce_result=g->wd.barrier_result=0;
        assert(!pt_editor_prepare_change(g->editor));g->editor->editing=1;g->editor->row=0;g->editor->panel=0;
        g->doc.project.channels.selected=4;pt_editor_key(g->editor,0x46,0);
        assert(g->editor->history.revision==revision&&!pt_editor_dispose(g->editor));
        g->d.quiesce_result=1;assert(!pt_editor_prepare_change(g->editor));g->wd.barrier_result=1;
        assert(pt_editor_prepare_change(g->editor));
        assert(pt_editor_mixed_prepare_session_get(&f->session)==PT_EDITOR_MIXED_SESSION_CANCELLED);goto closed_masters;
    }
    if(mode==7){assert(pt_editor_mixed_prepare_session_advance(&f->session)==PT_EDITOR_MIXED_SESSION_PENDING);
        assert(g->control->startup);goto closed_masters;}
    r=session_to(f,PT_EDITOR_MIXED_SESSION_PREPARED);
    if(mode==26||mode==27){assert(r!=PT_EDITOR_MIXED_SESSION_READY);g->bus.healthy=1;goto closed_masters;}
    assert(r==PT_EDITOR_MIXED_SESSION_READY&&!g->binding->transport);
    assert(g->binding->owner->pin[0]&&g->binding->owner->pin[1]&&!g->binding->owner->pin[2]);
    calls=g->calls;assert(pt_editor_mixed_prepare_session_advance(&f->session)==PT_EDITOR_MIXED_SESSION_READY&&g->calls==calls);
    if(mode==8){ /* Actual edit cancels the prepared owner; shared undo restores exact save. */
        unsigned revision=g->editor->history.revision;g->editor->editing=1;g->editor->panel=0;g->editor->row=0;
        g->doc.project.channels.selected=4;pt_editor_key(g->editor,0x46,0);
        assert(g->editor->history.revision!=revision&&g->doc.project.events[4].kind==PT_NOTE_NONE);
        assert(pt_editor_mixed_prepare_session_get(&f->session)==PT_EDITOR_MIXED_SESSION_CANCELLED);
        pt_editor_key(g->editor,0x31,8);assert(g->doc.project.events[4].kind==PT_NOTE_PERIOD);goto closed_masters;
    }
    if(mode==9){assert(pt_editor_dispose(g->editor)&&!g->live);disposed=1;
        assert(pt_editor_mixed_prepare_session_get(&f->session)==PT_EDITOR_MIXED_SESSION_CANCELLED);goto close;}
    if(mode==29){child=g->control->storage.memory.allocator.allocate(g->control->storage.memory.allocator.context,32);assert(child);
        assert(!pt_editor_mixed_prepare_session_close(&f->session)&&!g->binding->owner&&g->binding->owner_finish);
        assert(!f->bridges->paula.bridge&&!f->bridges->amigus.bridge&&g->control->storage.memory.active);
        assert(!pt_editor_dispose(g->editor));
        g->control->storage.memory.allocator.release(g->control->storage.memory.allocator.context,child);}
    if(mode==30)f->release_hook=1;
closed_masters:
    assert(pt_editor_mixed_prepare_session_close(&f->session));
    assert(!memcmp(masters,g->editor->sampler.current,sizeof(masters))&&g->live==3&&!g->binding->owner_finish);
close:
    g->alias=NULL;g->bus.healthy=1;f->allocation_hook=f->fail_at=0;
    assert(pt_editor_mixed_prepare_session_close(&f->session));session_silent(f);
    if(mode==31)g->doc.project.events[4].kind=PT_NOTE_PERIOD;
    if(mode==33)g->editor->sampler.budget=1024*1024;
    if(!disposed)session_save_same(f,before,bytes);
    calls=g->calls;assert(pt_editor_mixed_prepare_session_begin(&f->session,&f->input)==PT_EDITOR_MIXED_SESSION_INVALID&&g->calls==calls);
    if(mode==11||mode==20||mode==21||mode==23||mode==26||mode==30)assert(f->reentered&&f->session.first_error==PT_EDITOR_MIXED_SESSION_FAULT);
    free(before);session_drop(f,disposed);
}
static void session_admission(unsigned bits,unsigned mode)
{
    struct session_fixture *f=session_make(bits);struct editor_checked_fixture *g=&f->g;
    struct pt_editor_mixed_prepare_session *out=&f->session;struct pt_editor_mixed_prepare_session saved=f->session;
    struct pt_editor_mixed_prepare_session_inputs before;unsigned calls=g->calls,i;
    const void *objects[4]={g->editor,f->master,f->bridges,g->control};
    size_t sizes[4]={sizeof(*g->editor),sizeof(*f->master),sizeof(*f->bridges),sizeof(*g->control)};
    void *snapshots[4],*whole;
    switch(mode) {
    case 0:out=(void *)g->editor;break;
    case 1:out=(void *)g->binding;break;
    case 2:out=(void *)f->master;break;
    case 3:out=(void *)f->bridges;break;
    case 4:out=(void *)g->control;break;
    case 5:out=(void *)&f->input;break;
    case 6:out=(void *)&g->bus.cache;break;
    case 7:out=(void *)&g->bus.reservation;break;
    case 8:out=(void *)g->pcm;break;
    case 9:out=(void *)g->doc.project.events;break;
    case 10:out=(void *)(UINTPTR_MAX-1);break;
    case 11:f->input.contexts.data=&f->g;f->input.contexts.bytes=sizeof(f->g);break;
    case 12:f->input.contexts.bytes=sizeof(f->session)-1;break;
    case 13:f->input.contexts.data=(void *)(UINTPTR_MAX-1);break;
    case 14:f->input.checked=(void *)&f->session;break;
    case 15:f->input.bridges=(void *)&f->session;break;
    case 16:f->input.establish=(void *)&f->session;break;
    case 17:f->input.work=0;break;
    case 18:f->input.work=4097;break;
    case 19:f->input.devices.chip_context=&f->session;break;
    case 20:f->input.devices.amigus.context=&f->session;break;
    case 21:f->input.devices.paula_quiesce_context=&f->session;break;
    case 22:f->input.devices.amigus.context=g->editor;break;
    case 23:g->binding->preparation_context=f;break;
    case 24:f->bridges->paula_cache.version=1;break;
    default:{unsigned busy=1;out=(void *)g->pcm;
        memcpy((char *)out+offsetof(struct pt_editor_mixed_prepare_session,busy),&busy,sizeof(busy));break;}
    }
    before=f->input;
    whole=malloc(sizeof(*f));assert(whole);memcpy(whole,f,sizeof(*f));
    for(i=0;i<4;++i){snapshots[i]=malloc(sizes[i]);assert(snapshots[i]);memcpy(snapshots[i],objects[i],sizes[i]);}
    assert(pt_editor_mixed_prepare_session_begin(out,&f->input)==PT_EDITOR_MIXED_SESSION_INVALID);
    assert(!memcmp(&saved,&f->session,sizeof(saved))&&!memcmp(&before,&f->input,sizeof(before))&&g->calls==calls);
    assert(!memcmp(whole,f,sizeof(*f)));free(whole);
    for(i=0;i<4;++i){assert(!memcmp(snapshots[i],objects[i],sizes[i]));free(snapshots[i]);}
    session_silent(f);if(mode==23)g->binding->preparation_context=NULL;
    memset(f->bridges,0,sizeof(*f->bridges));session_drop(f,0);
}
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define TERMINAL_POISON(p,n) __asan_poison_memory_region(p,n)
#define TERMINAL_UNPOISON(p,n) __asan_unpoison_memory_region(p,n)
#endif
#endif
#ifndef TERMINAL_POISON
#define TERMINAL_POISON(p,n) ((void)(p),(void)(n))
#define TERMINAL_UNPOISON(p,n) ((void)(p),(void)(n))
#endif
static void session_terminal(unsigned bits)
{
    struct session_fixture *f=session_make(bits);struct editor_checked_fixture *g=&f->g;
    assert(pt_editor_mixed_prepare_session_begin(&f->session,&f->input)==PT_EDITOR_MIXED_SESSION_PENDING);
    assert(pt_editor_mixed_prepare_session_close(&f->session));
    assert(pt_editor_mixed_detach(g->binding)&&pt_editor_dispose(g->editor)&&!g->live);
    assert(pt_amigus_wavetable_cache_detach(&g->bus.cache)&&pt_amigus_reservation_close(&g->bus.reservation));
    pt_document_release(&g->doc);free(f->master);free(f->bridges);free(g->control);free(g->binding);free(g->editor);
    TERMINAL_POISON(&f->g,sizeof(f->g));TERMINAL_POISON(&f->input,sizeof(f->input));
    assert(pt_editor_mixed_prepare_session_get(&f->session)==PT_EDITOR_MIXED_SESSION_CLOSED);
    assert(pt_editor_mixed_prepare_session_close(&f->session));
    assert(pt_editor_mixed_prepare_session_advance(&f->session)==PT_EDITOR_MIXED_SESSION_CLOSED);
    TERMINAL_UNPOISON(&f->g,sizeof(f->g));TERMINAL_UNPOISON(&f->input,sizeof(f->input));free(f);
}
int main(void)
{
    unsigned bits,mode;assert(!editor_checked_fixture());
    for(bits=8;bits<=24;bits+=8){
        for(mode=0;mode<34;++mode)session_lifecycle(bits,mode);
        for(mode=0;mode<26;++mode)session_admission(bits,mode);
        session_terminal(bits);
    }
    puts("EDITOR PREPARE SESSION PASS:102 master/handoff/checked/edit-undo-dispose/cancel/reentry/retained-close/exact-save lifetimes;78 admission/control/context aliases and3 expired-terminal cases across8/16/24; prior checked39+18/editor15 unchanged; zero voice starts/timer calls/bus uploads");return 0;
}
