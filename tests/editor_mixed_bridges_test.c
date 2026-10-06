#define PT_TEST_EDITOR_CHECKED_EXEC
#include "editor_mixed_checked_test.c"
#include "../src/editor/editor_mixed_bridges.h"
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define VALUES_POISON(p,n) __asan_poison_memory_region(p,n)
#define VALUES_UNPOISON(p,n) __asan_unpoison_memory_region(p,n)
#endif
#endif
#ifndef VALUES_POISON
#define VALUES_POISON(p,n) ((void)(p),(void)(n))
#define VALUES_UNPOISON(p,n) ((void)(p),(void)(n))
#endif
static unsigned metadata_binding,metadata_ownership_calls;
static int metadata_owned(void *p)
{assert(!metadata_binding);++metadata_ownership_calls;return bus_owned(p);}
static struct editor_checked_fixture *bridge_fixture(unsigned bits)
{
    struct editor_checked_fixture *g=checked_editor_make(bits);
    /* End the old fixture's genuine idle owners before starting a separate path. */
    assert(pt_paula_voices_close(&g->pv)&&pt_wavetable_voices_close(&g->av));
    assert(!g->bus.reservation.access);
    memset(&g->pb,0,sizeof(g->pb));memset(&g->ab,0,sizeof(g->ab));
    memset(&g->pv,0,sizeof(g->pv));memset(&g->av,0,sizeof(g->av));memset(&g->bus.cache,0,sizeof(g->bus.cache));
    assert(pt_amigus_wavetable_cache_attach(&g->bus.cache,&g->bus.reservation,0,4096,4096,&g->bus,metadata_owned,bus_write));
    return g;
}
static struct pt_editor_mixed_bridge_inputs bridge_inputs(struct editor_checked_fixture *g)
{
    struct pt_editor_mixed_bridge_inputs in={0};in.backend=&g->bus.cache;in.chip_context=&g->d;
    in.chip_allocate=chip_alloc;in.chip_release=chip_free;in.chip_budget=4096;
    in.paula=g->pa;in.amigus=g->wa;in.paula_quiesce=quiesce;in.paula_quiesce_context=&g->d;
    in.amigus_quiesce=wave_quiesce;in.amigus_quiesce_context=&g->wd;return in;
}
static int bridge_bind(struct pt_editor_mixed_bridges *b,struct editor_checked_fixture *g,
    const struct pt_editor_mixed_bridge_inputs *in,const struct pt_sampler_storage_span *p,unsigned count)
{
    unsigned calls=metadata_ownership_calls;int result;metadata_binding=1;
    result=pt_editor_mixed_bridges_bind(b,g->binding,in,p,count);
    metadata_binding=0;assert(calls==metadata_ownership_calls);return result;
}
static void bridge_drop(struct editor_checked_fixture *g,struct pt_editor_mixed_bridges *b)
{
    assert(pt_editor_mixed_stop(g->binding));
    assert(pt_paula_voices_close(&b->paula)&&pt_wavetable_voices_close(&b->amigus));
    free(b);checked_editor_drop(g);
}
static void bridge_values(struct editor_checked_fixture *g,unsigned poison)
{
    struct pt_project *p=&g->doc.project;unsigned i;
    size_t events=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count*sizeof(*p->events);
    if(poison){VALUES_POISON(p->events,events);VALUES_POISON(p->orders,p->order_count);}
    else{VALUES_UNPOISON(p->events,events);VALUES_UNPOISON(p->orders,p->order_count);}
    for(i=0;i<p->sample_count;++i) {
        if(poison){VALUES_POISON(p->samples[i].pcm.data,p->samples[i].pcm.capacity*sizeof(int32_t));
            VALUES_POISON(p->samples[i].slices,p->samples[i].slice_count*sizeof(uint32_t));}
        else{VALUES_UNPOISON(p->samples[i].pcm.data,p->samples[i].pcm.capacity*sizeof(int32_t));
            VALUES_UNPOISON(p->samples[i].slices,p->samples[i].slice_count*sizeof(uint32_t));}
    }
}
static void bridge_lifecycle(unsigned bits,unsigned mode)
{
    struct editor_checked_fixture *g=bridge_fixture(bits);struct pt_editor_mixed_bridges *b=calloc(1,sizeof(*b));
    struct pt_editor_mixed_bridge_inputs in=bridge_inputs(g);struct pt_sampler_storage_span context={g,sizeof(*g)};
    struct editor_checked_fixture *saved=malloc(sizeof(*saved));enum pt_mixed_owner_result r;
    struct pt_sample_version *masters[3];unsigned n=0;assert(b&&saved);
    memcpy(masters,g->editor->sampler.current,sizeof(masters));
    if(mode==1){g->doc.project.events[4].kind=255;
        assert(!pt_sampler_paula_bind(&g->pb,&g->editor->sampler,&g->doc.project,&g->d,chip_alloc,chip_free,4096));
        assert(!pt_sampler_wavetable_bind(&g->ab,&g->editor->sampler,&g->doc.project,&g->bus.cache));}
    if(mode==2)g->bus.healthy=0;
    memcpy(saved,g,sizeof(*saved));bridge_values(g,1);
    assert(bridge_bind(b,g,&in,&context,1));
    bridge_values(g,0);assert(!memcmp(saved,g,sizeof(*saved))&&!output_count);
    assert(b->paula.bridge==&b->paula_cache&&b->amigus.bridge==&b->amigus_cache);
    assert(b->paula_cache.sampler==&g->editor->sampler&&b->amigus_cache.sampler==&g->editor->sampler);
    assert(b->paula.quiesce==quiesce&&b->amigus.quiesce==wave_quiesce&&b->paula.map[4]==0);
    assert(!bridge_bind(b,g,&in,&context,1)); /* No zeroing/rebind over live controls. */
    assert(pt_editor_mixed_checked_begin(g->binding,g->control,&b->paula,&b->amigus,&g->options,&g->caps,&g->format,
        context,64)==PT_MIXED_OWNER_PREPARING);
    do{r=pt_editor_mixed_prepare(g->binding,NULL);assert(++n<10000&&!output_count);}while(r==PT_MIXED_OWNER_PREPARING);
    if(mode){assert(r!=PT_MIXED_OWNER_OK&&!g->d.starts&&!g->wd.starts);
        g->bus.healthy=1;
    }else {
        assert(r==PT_MIXED_OWNER_OK);g->timer.clock=(struct mixed_counter){100,48000,0,1};
        assert(pt_editor_mixed_start(g->binding,&g->pump,1000,128,&g->api)==PT_MIXED_OWNER_OK);
        do{r=pt_editor_mixed_service(g->binding);assert(++n<15000);
            assert(r==PT_MIXED_OWNER_PREPARING||r==PT_MIXED_OWNER_WAITING);
            if(r==PT_MIXED_OWNER_WAITING)g->timer.clock.ticks=g->timer.deadline;
        }while(!g->d.starts||!g->wd.starts);
        g->d.quiesce_result=g->wd.barrier_result=0;
        assert(!pt_editor_prepare_change(g->editor)&&g->binding->owner_finish);
        g->d.quiesce_result=1;assert(!pt_editor_prepare_change(g->editor));g->wd.barrier_result=1;
        assert(!pt_editor_prepare_change(g->editor)&&g->binding->transport&&g->binding->owner_finish);
        g->timer.alarm_close_result=g->timer.counter_close_result=1;
    }
    assert(pt_editor_prepare_change(g->editor)&&!g->binding->owner_finish&&g->live==3);
    assert(!memcmp(masters,g->editor->sampler.current,sizeof(masters)));
    if(mode==1)g->doc.project.events[4].kind=PT_NOTE_PERIOD; /* Only after borrow closure. */
    free(saved);bridge_drop(g,b);
}
static void bridge_refusal(unsigned bits,unsigned mode)
{
    struct editor_checked_fixture *g=bridge_fixture(bits);struct pt_editor_mixed_bridges *b=calloc(1,sizeof(*b)),*out=b;
    struct pt_editor_mixed_bridge_inputs in=bridge_inputs(g);struct pt_sampler_storage_span parent={g,sizeof(*g)};
    struct pt_editor_mixed_bridges *saved=malloc(sizeof(*saved));
    struct editor_checked_fixture *before=malloc(sizeof(*before));struct pt_editor_mixed_bridge_inputs input_before;
    unsigned count=1,changed=0;assert(b&&saved&&before);
    switch(mode) {
    case 0:out=(void *)g->editor;break;
    case 1:out=(void *)g->binding;break;
    case 2:out=(void *)&g->bus.cache;break;
    case 3:out=(void *)&g->bus.reservation;break;
    case 4:out=(void *)&in;break;
    case 5:out=(void *)g->doc.project.events;break;
    case 6:out=(void *)g->doc.project.samples[2].pcm.data;break; /* Unused master full capacity. */
    case 7:out=(void *)&g->d;break; /* Complete named opaque context. */
    case 8:out=(void *)(UINTPTR_MAX-1);break;
    case 9:parent.data=b;parent.bytes=sizeof(*b);break;
    case 10:parent.data=(void *)(UINTPTR_MAX-1);parent.bytes=4;break;
    case 11:count=PT_EDITOR_BRIDGE_CONTEXTS+1;break;
    case 12:in.paula.stop=NULL;break;
    case 13:in.amigus_quiesce=NULL;break;
    case 14:g->bus.cache.cache.entry[0].pins=1;changed=1;break;
    case 15:g->bus.cache.arena.block[0].reserved=4;changed=2;break;
    case 16:g->bus.reservation.interrupt=1;changed=3;break;
    case 17:g->binding->preparation_context=g;changed=4;break;
    case 18:b->paula_cache.version=1;break;
    case 19:g->doc.project.channels.track[0].route=PT_PAULA;
        g->doc.project.channels.track[1].route=PT_PAULA;g->doc.project.channels.track[2].route=PT_PAULA;
        g->doc.project.channels.track[3].route=PT_PAULA;break; /* Existing track4 makes five. */
    case 20:in.chip_context=b;break;
    case 21:in.paula.context=&b->paula_cache;break;
    case 22:in.amigus.context=&b->amigus;break;
    case 23:in.paula_quiesce_context=(uint8_t *)b+sizeof(*b)-1;break;
    default:in.amigus_quiesce_context=&b->amigus_cache;break;
    }
    memcpy(saved,b,sizeof(*b));memcpy(before,g,sizeof(*before));memcpy(&input_before,&in,sizeof(in));bridge_values(g,1);
    assert(!bridge_bind(out,g,&in,&parent,count));bridge_values(g,0);
    assert(!memcmp(saved,b,sizeof(*b))&&!output_count&&g->live==3);
    assert(!memcmp(before,g,sizeof(*before))&&!memcmp(&input_before,&in,sizeof(in)));
    if(changed==1)g->bus.cache.cache.entry[0].pins=0;
    if(changed==2)g->bus.cache.arena.block[0].reserved=0;
    if(changed==3)g->bus.reservation.interrupt=0;
    if(changed==4)g->binding->preparation_context=NULL;
    if(mode==19){unsigned i;for(i=0;i<4;++i)g->doc.project.channels.track[i].route=PT_AMIGUS;}
    memset(b,0,sizeof(*b));free(before);free(saved);free(b);checked_editor_drop(g);
}
int main(void)
{
    unsigned bits,mode;assert(!editor_checked_fixture());
    for(bits=8;bits<=24;bits+=8){for(mode=0;mode<3;++mode)bridge_lifecycle(bits,mode);
        for(mode=0;mode<25;++mode)bridge_refusal(bits,mode);}
    puts("EDITOR BRIDGES PASS:9 poisoned-metadata/checked validation/ownership/live-drain cases;75 protected-output/context/cache/admission refusals across8/16/24; prior checked39+18/editor15 unchanged; injected callbacks only");return 0;
}
