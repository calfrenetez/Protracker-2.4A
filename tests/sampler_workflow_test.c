#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../src/editor/sampler_workflow.h"
#include "../src/editor/sampler_internal.h"
#include "../src/editor/sampler_paula.h"
#include "mod_project.h"
struct memory {size_t live,bytes,calls,fail,mutate_at;void *arena;void (*mutate)(void *);void *mutate_context;};
struct allocation {size_t bytes;};
static void *allocate(void *context,size_t bytes)
{
    struct memory *m=context;struct allocation *a;
    ++m->calls;if(m->fail && m->calls==m->fail)return NULL;
    a=m->arena && m->calls==1?m->arena:malloc(sizeof(*a)+bytes);assert(a);a->bytes=bytes;++m->live;m->bytes+=bytes;
    if(m->mutate && m->calls==m->mutate_at)m->mutate(m->mutate_context);
    return a+1;
}
static void release(void *context,void *data)
{struct memory *m=context;struct allocation *a=(struct allocation *)data-1;assert(m->live && m->bytes>=a->bytes);--m->live;m->bytes-=a->bytes;if(a!=m->arena)free(a);}
struct fixture {
    struct memory memory;struct pt_allocator allocator;struct pt_project p;
    struct pt_sampler sampler;struct pt_pattern_history history;
    struct pt_pattern_command commands[8];struct pt_event_change changes[32];
    struct pt_sample samples[6];struct pt_event events[3*64*16];uint16_t orders[1];
    int32_t pcm[3][5208];uint32_t markers[3][4];
};
static void empty(struct pt_sample *s)
{memset(s,0,sizeof(*s));s->pcm.bits=8;s->pcm.channels=1;s->pcm.rate=PT_CLASSIC_RATE;}
static struct fixture *fixture(unsigned bits,unsigned channels)
{
    struct fixture *f=calloc(1,sizeof(*f));unsigned i,j;assert(f);
    f->allocator=(struct pt_allocator){&f->memory,allocate,release};
    pt_channels_init(&f->p.channels);assert(pt_channels_resize(&f->p.channels,16)==PT_CHANNEL_OK);
    f->p.order_count=1;f->p.pattern_count=3;f->p.sample_count=6;f->p.bpm=125;f->p.speed=6;
    f->p.orders=f->orders;f->p.events=f->events;f->p.samples=f->samples;
    for(i=0;i<6;++i)empty(f->samples+i);
    for(i=0;i<3;++i) {
        struct pt_sample *s=f->samples+i;
        strcpy(s->name,i==0?"SOURCE":i==1?"UNUSED A":"UNUSED B");
        s->pcm=(struct pt_pcm){f->pcm[i],5208,2600,44100,(uint8_t)channels,(uint8_t)bits};
        for(j=0;j<2600*channels;++j)f->pcm[i][j]=(int32_t)((j%31)-15)*(bits==24?257:1);
        for(;j<5208;++j)f->pcm[i][j]=0x123456; /* Unused padding is not scanned. */
        f->markers[i][0]=3;f->markers[i][1]=50;f->markers[i][2]=2400;f->markers[i][3]=2590;
        s->slices=f->markers[i];s->slice_count=4;s->loop=PT_LOOP_CROSSFADE;s->loop_start=50;s->loop_end=2400;s->crossfade=17;
        s->volume=43;s->finetune=-3;s->interpolation=1;
    }
    strcpy(f->samples[3].name,"RESERVED NAME");f->samples[4].volume=64;
    f->events[0].instrument=1;f->events[0].kind=PT_NOTE_PERIOD;f->events[0].pitch=428;f->events[0].slice=1;
    f->events[3*64*16-1].instrument=1; /* Final off-order instrument-only. */
    assert(pt_project_validate(&f->p,NULL)==PT_PROJECT_OK);
    pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
    assert(pt_pattern_history_init(&f->history,&f->p,f->commands,8,f->changes,32)==PT_EDIT_OK);
    return f;
}
static void destroy(struct fixture *f)
{pt_pattern_history_release(&f->history);pt_sampler_release(&f->sampler);assert(!f->sampler.bytes && !f->memory.live && !f->memory.bytes);free(f);}
static struct pt_sample_usage_preview *scan(struct fixture *f)
{
    struct pt_sample_usage_scan *job=calloc(1,sizeof(*job));struct pt_sample_usage_preview *v=calloc(1,sizeof(*v));
    struct pt_sample_usage_options o={0};unsigned ready=0,n=0;assert(job && v);
    o.revision=f->history.revision;o.generation=f->sampler.generation;o.reserved_slots[3]=1;
    assert(pt_sample_usage_begin(job,&f->p,&o)==PT_USAGE_OK);
    do {assert(pt_sample_usage_step(job,&f->p,o.revision,o.generation,v,&ready)==PT_USAGE_OK);assert(++n<=12);}while(!ready);
    free(job);return v;
}
static void prepare(struct pt_sampler_workflow *job)
{unsigned ready=0,n=0;do {assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_OK);assert(++n<=48);}while(!ready);}
static void cleanup_matrix(void)
{
    unsigned bits,channels;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels) {
        struct fixture *f=fixture(bits,channels);struct pt_sample_usage_preview *v=scan(f);
        struct pt_sampler_workflow *job=NULL;struct pt_sampler_workflow_stats stats={0};
        struct pt_sample before[6];struct pt_project project=f->p;struct pt_pattern_history history=f->history;
        uint8_t selected[255]={0};int32_t master[3][5208];unsigned ready=7;
        memcpy(before,f->samples,sizeof(before));memcpy(master,f->pcm,sizeof(master));selected[1]=selected[2]=1;
        assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,0,&job)==PT_EDIT_CONFLICT && !job && !f->memory.calls);
        assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_OK);
        assert(!memcmp(&f->p,&project,sizeof(project)) && !memcmp(f->samples,before,sizeof(before)) && !memcmp(&f->history,&history,sizeof(history)));
        assert(!f->sampler.current[1] && !f->sampler.current[2]);
        assert(pt_sampler_workflow_step(job,(unsigned *)(f->pcm[1]+5207))==PT_EDIT_ALIAS);
        assert(!memcmp(master,f->pcm,sizeof(master)));
        assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_INVALID && job);
        assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_OK && !ready);
        assert(!memcmp(f->samples,before,sizeof(before)) && !f->sampler.current[1]);
        prepare(job);
        assert(pt_sampler_workflow_commit(&job,0,&stats)==PT_EDIT_CONFLICT && job && !f->history.count);
        assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_OK && !job);
        assert(f->p.sample_count==6 && !f->samples[1].pcm.frames && !f->samples[1].name[0] && !f->samples[2].pcm.frames);
        assert(!memcmp(f->samples,before,sizeof(before[0])) && !memcmp(f->samples+3,before+3,3*sizeof(before[0])));
        assert(f->history.count==1 && f->history.cursor==1 && f->sampler.generation==1);
        assert(stats.slots_freed==2 && stats.affected_slots==2 && !stats.master_bytes_released && stats.retained_master_bytes==2*5208*4 && stats.staged_bytes);
        assert(!memcmp(master,f->pcm,sizeof(master))); /* Original full capacities survive. */
        assert(pt_pattern_undo(&f->p,&f->history,-1)==PT_EDIT_OK);
        assert(f->samples[1].pcm.bits==bits && f->samples[1].pcm.channels==channels && f->samples[1].pcm.capacity==5208 && !strcmp(f->samples[1].name,"UNUSED A"));
        assert(!memcmp(f->samples[1].pcm.data,master[1],2600*channels*4));assert(f->samples[1].pcm.data!=f->pcm[1]);
        assert(f->samples[1].loop==PT_LOOP_CROSSFADE && f->samples[1].crossfade==17 && f->samples[1].finetune==-3);
        assert(!memcmp(f->samples[1].slices,f->markers[1],sizeof(f->markers[1])));
        assert(pt_pattern_undo(&f->p,&f->history,1)==PT_EDIT_OK && !f->samples[1].pcm.frames);
        free(v);destroy(f);
    }
}
static void cancel_fail_stale(void)
{
    unsigned phase,fail;
    for(phase=0;phase<16;++phase) {
        struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;
        struct pt_sample before[6];struct pt_pattern_history history=f->history;uint8_t selected[255]={0};unsigned i,ready=0;
        memcpy(before,f->samples,sizeof(before));selected[1]=selected[2]=1;
        assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_OK);
        for(i=0;i<phase;++i)assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_OK);
        pt_sampler_workflow_cancel(&job);assert(!job && !f->memory.live && !f->sampler.bytes);
        assert(!memcmp(f->samples,before,sizeof(before)) && !memcmp(&history,&f->history,sizeof(history)));
        free(v);destroy(f);
    }
    for(fail=1;fail<=3;++fail) {
        struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;
        struct pt_sample before[6];uint8_t selected[255]={0};memcpy(before,f->samples,sizeof(before));selected[1]=selected[2]=1;f->memory.fail=fail;
        assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_CAPACITY);
        assert(!job && !f->memory.live && !f->sampler.bytes && !memcmp(before,f->samples,sizeof(before)) && !f->history.count);
        free(v);destroy(f);
    }
    {
        struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;struct pt_sampler_workflow_stats stats;
        uint8_t selected[255]={0};unsigned ready=8;selected[1]=1;
        assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_OK);prepare(job);
        ++f->history.revision;assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_CONFLICT && ready==8);
        assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_CONFLICT && f->samples[1].pcm.frames==2600 && !f->history.count);
        pt_sampler_workflow_cancel(&job);assert(!f->memory.live);free(v);destroy(f);
    }
    {
        struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;
        struct pt_sampler_workflow_stats stats,oldstats;uint8_t selected[255]={0};unsigned ready=99;selected[1]=1;
        memset(&stats,0x53,sizeof(stats));oldstats=stats;
        assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_OK);
        prepare(job);f->p.samples=NULL;
        assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_CONFLICT && ready==99);
        assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_CONFLICT && job && !memcmp(&stats,&oldstats,sizeof(stats)));
        pt_sampler_workflow_cancel(&job);assert(!job && !f->memory.live && !f->sampler.bytes);
        f->p.samples=f->samples;free(v);destroy(f);
    }
    {
        struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;uint8_t selected[255]={0};unsigned i;
        struct pt_channel value=f->p.channels.track[4];selected[1]=1;
        for(i=0;i<8;++i) {value.pan=(uint8_t)(10+i);assert(pt_pattern_channel_apply(&f->p,&f->history,4,&value)==PT_EDIT_OK);}
        free(v);v=scan(f);assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_CAPACITY);
        assert(!job && !f->memory.calls && f->history.count==8 && f->history.cursor==8);
        selected[0]=1;assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_CAPACITY);
        free(v);destroy(f);
    }
}
static void copy_matrix(void)
{
    unsigned bits,channels;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels) {
        struct fixture *f=fixture(bits,channels);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;struct pt_sampler_workflow_stats stats;
        int32_t master[5208];struct pt_sample before[6];memcpy(master,f->pcm[0],sizeof(master));memcpy(before,f->samples,sizeof(before));
        assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,2500,1,&job)==PT_EDIT_OK);prepare(job);
        assert(!memcmp(before,f->samples,sizeof(before)) && !f->sampler.current[0] && !f->sampler.current[5]);
        assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_OK && stats.destination_slot==5 && !stats.appended && f->p.sample_count==6);
        assert(f->samples[5].pcm.frames==2490 && f->samples[5].pcm.bits==bits && f->samples[5].pcm.channels==channels && f->samples[5].pcm.rate==44100);
        assert(f->samples[5].pcm.data!=f->pcm[0]+10*channels && !memcmp(f->samples[5].pcm.data,f->pcm[0]+10*channels,2490*channels*4));
        assert(f->samples[5].volume==43 && f->samples[5].finetune==-3 && f->samples[5].interpolation==1);
        assert(f->samples[5].loop==PT_LOOP_CROSSFADE && f->samples[5].loop_start==40 && f->samples[5].loop_end==2390 && f->samples[5].crossfade==17);
        assert(f->samples[5].slice_count==2 && f->samples[5].slices[0]==40 && f->samples[5].slices[1]==2390);
        assert(pt_pattern_undo(&f->p,&f->history,-1)==PT_EDIT_OK && !f->samples[5].pcm.frames && !f->samples[5].name[0]);
        assert(pt_pattern_undo(&f->p,&f->history,1)==PT_EDIT_OK && f->samples[5].pcm.frames==2490);
        f->samples[5].pcm.data[0]=0;assert(!memcmp(master,f->pcm[0],sizeof(master))); /* Independence. */
        free(v);destroy(f);
    }
    {
        struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v;struct pt_sampler_workflow *job=NULL;struct pt_sampler_workflow_stats stats;
        struct pt_sample *original=f->p.samples;strcpy(f->samples[5].name,"INTENTIONALLY EMPTY");v=scan(f);
        assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,60,1,&job)==PT_EDIT_OK);prepare(job);
        assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_OK && stats.appended && stats.destination_slot==6 && f->p.sample_count==7 && f->p.samples!=original);
        assert(f->p.samples[6].loop==PT_LOOP_NONE && !f->p.samples[6].loop_start && !f->p.samples[6].loop_end && !f->p.samples[6].crossfade);
        assert(f->p.samples[6].slice_count==1 && f->p.samples[6].slices[0]==40);
        assert(pt_pattern_undo(&f->p,&f->history,-1)==PT_EDIT_OK && f->p.sample_count==6 && f->p.samples==original);
        assert(pt_pattern_undo(&f->p,&f->history,1)==PT_EDIT_OK && f->p.sample_count==7);
        free(v);destroy(f);
    }
}
static void mutate_free(void *context)
{
    struct fixture *f=context;strcpy(f->samples[5].name,"NEW CONFIGURATION");f->samples[5].volume=53;
    ++f->history.revision;++f->sampler.generation;
}
static void mutate_later_selected(void *context)
{struct fixture *f=context;f->samples[2].volume=52;}
static void callback_rebase(void)
{
    struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;
    f->memory.mutate=mutate_free;f->memory.mutate_context=f;f->memory.mutate_at=1;
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,60,1,&job)==PT_EDIT_CONFLICT);
    assert(!job && !f->memory.live && !f->sampler.bytes && !strcmp(f->samples[5].name,"NEW CONFIGURATION") && f->samples[5].volume==53);
    assert(f->history.revision==1 && !f->history.count && !f->sampler.current[0] && !f->sampler.current[5]);
    free(v);destroy(f);
    f=fixture(24,2);v=scan(f);
    {
        uint8_t selected[255]={0};selected[1]=selected[2]=1;
        f->memory.mutate=mutate_later_selected;f->memory.mutate_context=f;f->memory.mutate_at=2;
        assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_CONFLICT);
        assert(!job && !f->memory.live && !f->sampler.bytes && f->samples[2].volume==52);
        assert(f->samples[1].pcm.frames==2600 && !f->history.count && !f->sampler.current[1] && !f->sampler.current[2]);
    }
    free(v);destroy(f);
}
static void borrowed_handle_alias(void)
{
    union held {int32_t values[5208];struct {int32_t prefix[5200];struct pt_sampler_workflow *job;int32_t tail[6];} handle;} *storage=calloc(1,sizeof(*storage));
    struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v;unsigned i;assert(storage);
    memcpy(storage->values,f->pcm[0],sizeof(storage->values));f->samples[0].pcm.data=storage->values;
    for(i=0;i<2;++i) {
        union held before;storage->handle.job=i?(struct pt_sampler_workflow *)(void *)f:NULL;before=*storage;v=scan(f);
        assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,20,1,&storage->handle.job)==PT_EDIT_ALIAS);
        assert(!memcmp(&before,storage,sizeof(before)) && !f->memory.calls && !f->sampler.bytes && !f->history.count);free(v);
    }
    f->samples[0].pcm.data=f->pcm[0];destroy(f);free(storage);
}
static void private_handle_alias(void)
{
    struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);
    struct allocation *arena=calloc(1,sizeof(*arena)+65536);struct pt_sampler_workflow **out;
    struct pt_sample before[6];uint8_t selected[255]={0};assert(arena);out=(struct pt_sampler_workflow **)(void *)(arena+1);
    memcpy(before,f->samples,sizeof(before));selected[1]=1;f->memory.arena=arena;
    assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,out)==PT_EDIT_ALIAS);
    assert(!*out && !f->memory.live && !f->sampler.bytes && !memcmp(before,f->samples,sizeof(before)) && !f->history.count);
    f->memory.arena=NULL;free(arena);
    f->p.samples=NULL;
    {struct pt_sampler_workflow *job=NULL;assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_CONFLICT && !job);}
    f->p.samples=f->samples;free(v);destroy(f);
}
static void capacity_reserved_unknown(void)
{
    struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v;struct pt_sampler_workflow *job=NULL;
    struct pt_extension extension={0x554e4b4e,0,1,NULL};struct pt_sample *table=calloc(255,sizeof(*table));unsigned i;
    assert(table);for(i=0;i<255;++i) {empty(table+i);strcpy(table[i].name,"CONFIGURED");}table[0]=f->samples[0];
    f->p.samples=table;f->p.sample_count=255;v=scan(f);
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,0,10,1,&job)==PT_EDIT_CAPACITY && !job && !f->memory.calls);
    free(v);f->p.samples=f->samples;f->p.sample_count=6;free(table);
    strcpy(f->samples[5].name,"CONFIGURED");f->p.extensions=&extension;f->p.extension_count=1;v=scan(f);
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,0,10,1,&job)==PT_EDIT_UNSUPPORTED && !job && !f->memory.calls);
    free(v);f->p.extensions=NULL;f->p.extension_count=0;
    {
        struct pt_sample_usage_scan *scan_job=calloc(1,sizeof(*scan_job));struct pt_sample_usage_options options={0};unsigned ready=0,n=0;
        v=calloc(1,sizeof(*v));assert(scan_job && v);options.reserved_slots[6]=1;
        assert(pt_sample_usage_begin(scan_job,&f->p,&options)==PT_USAGE_OK);
        do {assert(pt_sample_usage_step(scan_job,&f->p,0,0,v,&ready)==PT_USAGE_OK);assert(++n<=12);}while(!ready);
        assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,0,10,1,&job)==PT_EDIT_UNSUPPORTED && !job && !f->memory.calls);
        free(scan_job);free(v);
    }
    destroy(f);
}
static void *chip_allocate(void *context,size_t n) {return allocate(context,n);}
static void chip_release(void *context,void *p,size_t n) {(void)n;release(context,p);}
static void retained_pin_cache(void)
{
    struct fixture *f=fixture(24,1);struct pt_sampler_paula pool={0};struct pt_cache_lease lease;
    struct pt_sample_version *token=NULL;struct pt_pcm pinned;struct pt_sample_usage_preview *v;
    struct pt_sampler_workflow *job=NULL;struct pt_sampler_workflow_stats stats;
    const uint8_t *bytes;size_t count;uint8_t selected[255]={0};uint8_t *saved;
    assert(pt_sampler_pin(&f->sampler,&f->p,1,f->sampler.generation,&pinned,&token)==PT_EDIT_OK);
    assert(pt_sampler_paula_bind(&pool,&f->sampler,&f->p,&f->memory,chip_allocate,chip_release,8192));
    assert(pt_sampler_paula_acquire(&pool,0,2,0,&lease)==PT_CACHE_LOAD);
    assert(pt_sampler_paula_location(&pool,0,lease,&bytes,&count));saved=malloc(count);assert(saved);memcpy(saved,bytes,count);
    v=scan(f);assert(v->rows[1].flags&PT_USAGE_ELIGIBLE);selected[1]=1;
    assert(pt_sampler_cleanup_begin(&f->sampler,&f->p,&f->history,v,selected,1,&job)==PT_EDIT_OK);prepare(job);
    assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_OK && !stats.master_bytes_released);
    assert(pt_sampler_paula_sync(&pool));assert(!pt_sampler_paula_location(&pool,0,lease,&bytes,&count));
    assert(!memcmp(saved,pt_cache_data(&pool.cache,lease),2600)); /* Test only: pinned retired storage survives. */
    assert(pinned.bits==24 && !memcmp(pinned.data,f->pcm[1],2600*sizeof(int32_t)));
    assert(pt_sampler_paula_unpin(&pool,lease));assert(pt_sampler_paula_close(&pool));pt_sampler_unpin(token);
    free(saved);free(v);destroy(f);
}
int main(void)
{cleanup_matrix();cancel_fail_stale();copy_matrix();callback_rebase();borrowed_handle_alias();private_handle_alias();capacity_reserved_unknown();retained_pin_cache();puts("SAMPLER WORKFLOW PASS: sixformats selective atomic cleanup/copy, one undo, metadata/lowbits/master preservation, gaps/append/loop/slices, cancellation/fault/stale/fulljournal and zero resources");return 0;}
