#define main inherited_project_memory_renderer_main
#include "render_sequence_startup_test.c"
#undef main
#include "../src/editor/sampler_prepare_project.h"
#include "../src/core/render_storage_internal.h"
#include "../src/editor/mixed_preflight.h"
static const struct pt_paula_render_caps project_caps={3546895,124,65535};
static const struct pt_playback_format project_format={8,0,0,0};
struct project_memory_fixture {
    struct startup_fixture *source;
    struct pt_sampler sampler;
    struct pt_sampler_prepare_project owner;
    struct pt_allocator base;
    void *alias;
    unsigned calls,releases,live,alias_releases,mode,fail;
    uint16_t alternate_orders[2];
    struct pt_sample alternate_samples[2];
    int32_t alternate_pcm[64];
    unsigned char parent[32];
};
static void *project_test_allocate(void *context,size_t bytes)
{
    struct project_memory_fixture *g=context;void *p;++g->calls;
    if(g->mode==1)++g->source->project.bpm;
    if(g->mode==2)g->source->project.orders=g->alternate_orders;
    if(g->mode==3)g->source->project.samples=g->alternate_samples;
    if(g->mode==4)assert(!g->owner.memory.allocator.allocate(&g->owner.memory,8));
    if(g->fail)return NULL;
    if(g->alias)return g->alias;
    p=malloc(bytes);if(p)++g->live;return p;
}
static void project_test_release(void *context,void *p)
{
    struct project_memory_fixture *g=context;
    if(p==g->alias){++g->alias_releases;return;}
    assert(p&&g->live);--g->live;++g->releases;free(p);
}
static struct project_memory_fixture *project_init(unsigned bits)
{
    struct project_memory_fixture *g=calloc(1,sizeof(*g));assert(g);
    g->source=malloc(sizeof(*g->source));assert(g->source);
    startup_init(g->source,bits,1);g->source->samples[1].pcm.channels=1;
    g->base=(struct pt_allocator){g,project_test_allocate,project_test_release};
    pt_sampler_init(&g->sampler,&g->base,1024*1024);
    memcpy(g->alternate_orders,g->source->orders,sizeof(g->alternate_orders));
    memcpy(g->alternate_samples,g->source->samples,sizeof(g->alternate_samples));
    g->alternate_samples[0].pcm.data=g->alternate_pcm;
    g->alternate_samples[0].pcm.capacity=64;
    memset(g->parent,0x5a,sizeof(g->parent));return g;
}
static void project_begin(struct project_memory_fixture *g)
{
    struct pt_allocator base=g->base;
    struct pt_sampler_storage_span parent={g->parent,sizeof(g->parent)};
    assert(pt_sampler_prepare_project_begin(&g->owner,&g->sampler,&g->source->project,&base,&parent,1));
    memset(&base,0,sizeof(base));memset(&parent,0,sizeof(parent));
}
static void project_close(struct project_memory_fixture *g)
{
    unsigned i;assert(pt_sampler_prepare_project_finish(&g->owner));
    assert(!g->live&&!g->alias_releases&&!g->sampler.bytes&&!g->sampler.table&&!g->sampler.table_original);
    for(i=0;i<PT_PROJECT_SAMPLES;++i)assert(!g->sampler.current[i]);
    for(i=0;i<sizeof(g->parent);++i)assert(g->parent[i]==0x5a);
    pt_sampler_release(&g->sampler);free(g->source);free(g);
}
static void project_alias_cases(void)
{
    unsigned bits,kind;
    for(bits=8;bits<=24;bits+=8)for(kind=0;kind<10;++kind) {
        struct project_memory_fixture *g=project_init(bits);
        struct startup_fixture *before=malloc(sizeof(*before));
        struct pt_sampler sampler=g->sampler;struct pt_mixed_preflight_setup *j=NULL;
        assert(before);memcpy(before,g->source,sizeof(*before));
        startup_values_poison(g->source,1);project_begin(g);
        switch(kind) {
        case 0:g->alias=&g->source->project;break;
        case 1:g->alias=(unsigned char *)g->source->orders+sizeof(g->source->orders)-1;break;
        case 2:g->alias=(unsigned char *)g->source->events+sizeof(g->source->events)-1;break;
        case 3:g->alias=(unsigned char *)g->source->samples+sizeof(g->source->samples)-1;break;
        case 4:g->alias=g->source->pcm[1].values+STARTUP_VALUES-1;break;
        case 5:g->alias=g->source->slices+1;break;
        case 6:g->alias=(unsigned char *)&g->source->extension+sizeof(g->source->extension)-1;break;
        case 7:g->alias=g->source->payload+sizeof(g->source->payload)-1;break;
        case 8:g->alias=(unsigned char *)&g->owner+sizeof(g->owner)-1;break;
        case 9:g->alias=g->parent+sizeof(g->parent)-1;break;
        }
        assert(pt_mixed_preflight_setup_begin(&g->source->project,&g->source->options,NULL,
            &project_caps,&project_format,1,1,&g->owner.memory.allocator,
            STARTUP_REVISION,STARTUP_GENERATION,&j)==PT_RENDER_SETUP_CAPACITY&&!j);
        startup_values_unpoison(g->source);
        assert(g->owner.memory.faulted&&g->calls==1&&!g->live&&!g->releases&&!g->alias_releases);
        assert(!memcmp(before,g->source,sizeof(*before))&&!memcmp(&sampler,&g->sampler,sizeof(sampler)));
        for(unsigned i=0;i<PT_SAMPLER_PREPARE_BLOCKS;++i)assert(!g->owner.memory.block[i].data);
        free(before);project_close(g);
    }
}
static void project_mixed_success(void)
{
    unsigned bits;for(bits=8;bits<=24;bits+=8) {
        struct project_memory_fixture *g=project_init(bits);
        struct pt_mixed_preflight_setup *j=NULL;struct pt_mixed_preflight *audit=NULL;
        struct pt_mixed_report report;struct pt_render_sequence *s=NULL;
        struct pt_project before=g->source->project;unsigned n=0;
        startup_values_poison(g->source,1);project_begin(g);
        assert(pt_mixed_preflight_setup_begin(&g->source->project,&g->source->options,NULL,
            &project_caps,&project_format,1,1,&g->owner.memory.allocator,
            STARTUP_REVISION,STARTUP_GENERATION,&j)==PT_RENDER_SETUP_PENDING&&j);
        startup_values_unpoison(g->source);
        assert(g->calls==2&&g->live==2&&!g->owner.memory.faulted);
        assert(!pt_sampler_prepare_project_finish(&g->owner));
        while(pt_mixed_preflight_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,7)==PT_RENDER_SETUP_PENDING)
            assert(++n<20000);
        assert(pt_mixed_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&audit)==PT_RENDER_SETUP_READY&&!j&&audit);
        n=0;do{assert(++n<20000);}while(pt_mixed_preflight_step(audit,&report)==PT_MIXED_PENDING);
        assert(report.result==PT_MIXED_OK&&pt_mixed_preflight_take(audit,&s)&&s);
        pt_mixed_preflight_close(&audit);assert(g->live==1);
        pt_render_sequence_close(s);assert(!g->live&&g->calls==3&&g->releases==3);
        assert(!memcmp(&before,&g->source->project,sizeof(before))&&!g->owner.memory.faulted);
        project_close(g);
    }
}
static void project_header_faults(void)
{
    unsigned kind;for(kind=0;kind<7;++kind) {
        struct project_memory_fixture *g=project_init(24);struct pt_project before=g->source->project;
        void *child=NULL;project_begin(g);
        if(kind==0){++g->source->project.bpm;assert(!g->owner.memory.allocator.allocate(&g->owner.memory,32)&&!g->calls);}
        if(kind==1||kind==2) {
            g->mode=kind==1?1:4;assert(!g->owner.memory.allocator.allocate(&g->owner.memory,32));
            assert(g->calls==1&&g->releases==1&&!g->live);
        }
        if(kind==3||kind==4) {
            g->mode=2;g->alias=kind==3?(void *)g->source->orders:(void *)g->alternate_orders;
            assert(!g->owner.memory.allocator.allocate(&g->owner.memory,32));
            assert(g->calls==1&&!g->releases&&!g->alias_releases);
        }
        if(kind==5) {
            g->mode=3;g->alias=g->alternate_pcm+63;
            assert(!g->owner.memory.allocator.allocate(&g->owner.memory,32)&&!g->releases&&!g->alias_releases);
        }
        if(kind==6) {
            child=g->owner.memory.allocator.allocate(&g->owner.memory,32);assert(child);
            ++g->source->project.bpm;assert(!g->owner.memory.allocator.allocate(&g->owner.memory,16)&&g->calls==1);
            STARTUP_POISON(&g->source->project,sizeof(g->source->project));
            g->owner.memory.allocator.release(&g->owner.memory,child);
            STARTUP_UNPOISON(&g->source->project,sizeof(g->source->project));
        }
        assert(g->owner.memory.faulted&&!g->live);
        g->source->project=before;project_close(g);
    }
}
static void project_capacity_cursor_and_begin(void)
{
    struct project_memory_fixture *g=project_init(24);struct pt_sampler_prepare_project before=g->owner;
    struct pt_sampler_storage_span parents[PT_SAMPLER_PREPARE_PARENTS]={{NULL,0}};void *p;
    assert(!pt_sampler_prepare_project_begin(&g->owner,&g->sampler,&g->source->project,&g->base,
        parents,PT_SAMPLER_PREPARE_PARENTS)&&!memcmp(&before,&g->owner,sizeof(before))&&!g->calls);
    assert(!pt_sampler_prepare_project_begin((void *)g->source->orders,&g->sampler,
        &g->source->project,&g->base,NULL,0));
    project_begin(g);assert(!pt_sampler_prepare_project_begin(&g->owner,&g->sampler,&g->source->project,&g->base,NULL,0));
    g->fail=1;assert(!g->owner.memory.allocator.allocate(&g->owner.memory,16)&&!g->owner.memory.faulted);
    g->fail=0;g->source->project.channels.selected=5;
    p=g->owner.memory.allocator.allocate(&g->owner.memory,16);assert(p&&!g->owner.memory.faulted);
    assert(!pt_sampler_prepare_project_finish(&g->owner));g->owner.memory.allocator.release(&g->owner.memory,p);
    g->source->project.channels.selected=PT_CHANNEL_LIMIT;
    assert(!g->owner.memory.allocator.allocate(&g->owner.memory,16)&&g->owner.memory.faulted&&g->calls==2);
    g->source->project.channels.selected=0;project_close(g);
}
static void project_span_refusals(void)
{
    struct project_memory_fixture *g=project_init(24);struct pt_project p=g->source->project;
    unsigned char out[8];startup_values_poison(g->source,1);
    assert(pt_render_project_storage_output_disjoint(&p,out,sizeof(out)));
    p.sample_count=PT_PROJECT_SAMPLES+1;STARTUP_POISON(g->source->samples,sizeof(g->source->samples));
    assert(!pt_render_project_storage_output_disjoint(&p,out,sizeof(out)));
    STARTUP_UNPOISON(g->source->samples,sizeof(g->source->samples));p=g->source->project;
    p.extension_count=4091;STARTUP_POISON(&g->source->extension,sizeof(g->source->extension));
    assert(!pt_render_project_storage_output_disjoint(&p,out,sizeof(out)));
    STARTUP_UNPOISON(&g->source->extension,sizeof(g->source->extension));p=g->source->project;
    p.samples=NULL;assert(!pt_render_project_storage_output_disjoint(&p,out,sizeof(out)));
    p=g->source->project;p.orders=(void *)(uintptr_t)(UINTPTR_MAX-1);
    assert(!pt_render_project_storage_output_disjoint(&p,out,sizeof(out)));
    p=g->source->project;g->source->samples[1].pcm.capacity=SIZE_MAX;
    assert(!pt_render_project_storage_output_disjoint(&p,out,sizeof(out)));
    g->source->samples[1].pcm.capacity=STARTUP_VALUES;
    assert(!pt_render_project_storage_output_disjoint(&p,(void *)(uintptr_t)(UINTPTR_MAX-1),4));
    startup_values_unpoison(g->source);project_close(g);
}
int main(void)
{
    project_alias_cases();project_mixed_success();project_header_faults();
    project_capacity_cursor_and_begin();project_span_refusals();
    puts("PREPARE PROJECT PASS:30 complete project/control/parent aliases without child ownership or release,3 genuine mixed transfers,7 header/reentry fault cases, capacity/cursor/begin and wrapped/table span refusals; host only");return 0;
}
