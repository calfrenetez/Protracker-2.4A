/* Source-only HOST fixture proposal. No owner enrollment/native gate is added.
 * Every inventory node/reservation below is created by genuine core lifetimes. */
#include "../src/editor/sampler.h"
#include "../src/editor/sampler_internal.h"
#include "../src/editor/sampler_lifetime_internal.h"
#include "../src/editor/sampler_workflow.h"
#include "../src/editor/sampler_establish.h"
#include "sample_usage.h"
#include "mod_project.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "This fixture proposes serialized HOST lifetime coverage only."
#endif
#define TRACKED 8192U
#define COMMANDS 4096U
struct allocation {void *pointer;size_t bytes;};
struct fixture {
    struct pt_sampler sampler;
    struct pt_project project;
    struct pt_pattern_history history;
    struct pt_allocator allocator;
    struct pt_sample samples[2];
    struct pt_event events[PT_PROJECT_ROWS];
    uint16_t orders[1];
    int32_t original[6];
    uint32_t markers[2];
    struct pt_pattern_command commands[COMMANDS];
    struct pt_event_change changes[8];
    struct allocation allocated[TRACKED];
    size_t live_bytes,live_allocations,allocations,releases;
    unsigned observe,observed_allocate,observed_release,fail_next,budget_drop_after,stale_after,stale_observed;
    struct pt_sample_usage_scan scan;
    struct pt_sample_usage_preview preview;
    struct pt_sampler_establish establishment;
};
static void *allocate(void *context,size_t bytes)
{
    struct fixture *f=context;void *p;unsigned i;
    if(f->observe){
        assert(!pt_sampler_lifetime_idle(&f->sampler));++f->observed_allocate;
    }
    ++f->allocations;
    if(f->stale_after&&!--f->stale_after){++f->project.bpm;++f->stale_observed;}
    if(f->budget_drop_after&&!--f->budget_drop_after)f->sampler.budget=f->sampler.bytes;
    if(f->fail_next){f->fail_next=0;return NULL;}
    p=malloc(bytes);assert(p);
    for(i=0;i<TRACKED&&f->allocated[i].pointer;++i){}
    assert(i<TRACKED);f->allocated[i]=(struct allocation){p,bytes};
    ++f->live_allocations;f->live_bytes+=bytes;return p;
}
static void release(void *context,void *pointer)
{
    struct fixture *f=context;unsigned i;
    if(f->observe){
        assert(!pt_sampler_lifetime_idle(&f->sampler));++f->observed_release;
    }
    for(i=0;i<TRACKED&&f->allocated[i].pointer!=pointer;++i){}
    assert(i<TRACKED&&pointer);
    if(f->observe)assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,pointer,f->allocated[i].bytes));
    f->live_bytes-=f->allocated[i].bytes;
    --f->live_allocations;++f->releases;
    f->allocated[i]=(struct allocation){NULL,0};free(pointer);
}
static struct fixture *begin(void)
{
    static const int32_t values[6]={-3,17,40,-55,9,100};
    struct fixture *f=calloc(1,sizeof(*f));assert(f);
    f->allocator=(struct pt_allocator){f,allocate,release};
    pt_channels_init(&f->project.channels);
    assert(pt_channels_resize(&f->project.channels,1)==PT_CHANNEL_OK);
    f->project.pattern_count=f->project.order_count=1;f->project.sample_count=2;
    f->project.speed=6;f->project.bpm=125;f->project.mode=PT_MODE_WAVETABLE;
    f->project.orders=f->orders;f->project.events=f->events;f->project.samples=f->samples;
    memcpy(f->original,values,sizeof(values));f->markers[1]=3;
    f->samples[0].pcm=(struct pt_pcm){f->original,6,6,PT_CLASSIC_RATE,1,8};
    strcpy(f->samples[0].name,"SOURCE");f->samples[0].volume=64;
    f->samples[0].slices=f->markers;f->samples[0].slice_count=2;
    f->samples[1].pcm=(struct pt_pcm){NULL,0,0,PT_CLASSIC_RATE,1,8};
    assert(pt_project_validate(&f->project,NULL)==PT_PROJECT_OK);
    assert(pt_pattern_history_init(&f->history,&f->project,f->commands,COMMANDS,
        f->changes,8)==PT_EDIT_OK);
    pt_sampler_init(&f->sampler,&f->allocator,4U*1024U*1024U);f->observe=1;
    assert(pt_sampler_lifetime_idle(&f->sampler));return f;
}
static void emptied(struct fixture *f)
{
    assert(!f->sampler.lifetime_fault&&!f->sampler.live_head&&!f->sampler.live_count);
    assert(pt_sampler_lifetime_idle(&f->sampler));
    assert(!f->sampler.bytes&&!f->live_allocations&&!f->live_bytes);
    assert(f->observed_allocate&&f->observed_release);
}
static void finish(struct fixture *f)
{
    pt_pattern_history_release(&f->history);pt_sampler_release(&f->sampler);
    emptied(f);free(f);
}
static void scan(struct fixture *f)
{
    struct pt_sample_usage_options options={0};unsigned ready=0,i;
    options.revision=f->history.revision;options.generation=f->sampler.generation;
    assert(pt_sample_usage_begin(&f->scan,&f->project,&options)==PT_USAGE_OK);
    for(i=0;i<100&&!ready;++i)
        assert(pt_sample_usage_step(&f->scan,&f->project,f->history.revision,
            f->sampler.generation,&f->preview,&ready)==PT_USAGE_OK);
    assert(ready);
}
static void workflow_ready(struct pt_sampler_workflow *job)
{
    unsigned ready=0,i;
    for(i=0;i<1000&&!ready;++i)assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_OK);
    assert(ready);
}
static struct pt_sample_version *pin(struct fixture *f,struct pt_pcm *pcm)
{
    struct pt_sample_version *token=NULL;
    assert(pt_sampler_pin(&f->sampler,&f->project,0,f->sampler.generation,pcm,&token)==PT_EDIT_OK);
    assert(token&&f->sampler.live_count&&pt_sampler_lifetime_idle(&f->sampler));return token;
}
static void promotion(void)
{
    struct fixture *f=begin();struct pt_pcm pcm;struct pt_sample_version *token=pin(f,&pcm);
    assert(pcm.data!=f->original&&f->sampler.generation==0&&!f->history.count);
    assert(!memcmp(pcm.data,f->original,sizeof(f->original)));
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,token,1));
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,pcm.data,pcm.capacity*sizeof(int32_t)));
    assert(pt_sampler_lifetime_output_disjoint(&f->sampler,&pcm,sizeof(pcm)));
    pt_sampler_unpin(token);finish(f);
}
static void old_external_pcm_history(void)
{
    struct fixture *f=begin();struct pt_pcm pcm;struct pt_sample_version *old=pin(f,&pcm);
    int32_t *old_data=pcm.data;uint32_t *old_markers=f->project.samples[0].slices;
    assert(pt_sampler_edit(&f->sampler,&f->project,&f->history,0,PT_PCM_REVERSE,0,6,0)==PT_EDIT_OK);
    assert(f->sampler.generation==1&&f->project.samples[0].pcm.data!=old_data);
    assert(f->project.samples[0].pcm.data[0]==100&&old_data[0]==-3);
    assert(pt_pattern_undo(&f->project,&f->history,-1)==PT_EDIT_OK);
    assert(f->project.samples[0].pcm.data[0]==-3);
    assert(pt_pattern_undo(&f->project,&f->history,1)==PT_EDIT_OK);
    assert(f->project.samples[0].pcm.data[0]==100);
    pt_pattern_history_release(&f->history);pt_sampler_release(&f->sampler);
    assert(f->sampler.live_count==1&&f->live_allocations==1&&f->sampler.bytes);
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,old,1));
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,old_data,sizeof(int32_t)));
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,old_markers,2*sizeof(uint32_t)));
    assert(pt_sampler_lifetime_idle(&f->sampler)&&old_data[0]==-3);
    pt_sampler_unpin(old);emptied(f);free(f);
}
static void metadata_backing(void)
{
    struct fixture *f=begin();struct pt_pcm pcm;struct pt_sample_version *old=pin(f,&pcm);
    int32_t *data=pcm.data;
    assert(pt_sampler_attributes(&f->sampler,&f->project,&f->history,0,"META",32,1)==PT_EDIT_OK);
    assert(f->project.samples[0].pcm.data==data&&f->sampler.live_count>=3);
    pt_sampler_unpin(old);pt_pattern_history_release(&f->history);
    assert(f->sampler.live_count==2); /* Current metadata header plus genuine flat backing. */
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,data,sizeof(int32_t)));
    pt_sampler_release(&f->sampler);emptied(f);free(f);
}
static void owned_spare_and_table(void)
{
    struct fixture *f=begin();struct pt_pcm pcm;struct pt_sample_version *token;
    int32_t *data;unsigned i;f->observe=0;data=allocate(f,32*sizeof(*data));f->observe=1;
    for(i=0;i<32;++i)data[i]=0;
    data[0]=10;pcm=(struct pt_pcm){data,32,6,PT_CLASSIC_RATE,1,8};
    assert(pt_sampler_append_owned(&f->sampler,&f->project,&f->history,&pcm,&f->allocator,"OWNED")==PT_EDIT_OK);
    assert(!pcm.data&&!pcm.capacity&&f->project.sample_count==3);
    assert(f->sampler.table_bytes==PT_PROJECT_SAMPLES*sizeof(struct pt_sample));
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,data+31,sizeof(*data)));
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,f->project.samples+254,sizeof(struct pt_sample)));
    assert(pt_sampler_pin(&f->sampler,&f->project,2,f->sampler.generation,&pcm,&token)==PT_EDIT_OK);
    pt_sampler_unpin(token);finish(f);
}
static void current_job_cancel(void)
{
    struct fixture *f=begin();struct pt_pcm pcm;struct pt_sample_version *token=pin(f,&pcm);
    struct pt_sampler_pin_job job={0};size_t calls=f->allocations;
    assert(pt_sampler_pin_job_begin(&job,&f->sampler,&f->project,0,f->sampler.generation)==PT_EDIT_OK);
    assert(f->sampler.pending_count==1&&!pt_sampler_lifetime_idle(&f->sampler)&&f->allocations==calls);
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,&job,sizeof(job)));
    pt_sampler_pin_job_cancel(&job);
    assert(pt_sampler_lifetime_idle(&f->sampler)&&!job.value);pt_sampler_unpin(token);finish(f);
}
static void job_alias_then_transfer(void)
{
    struct fixture *f=begin();struct pt_sampler_pin_job job={0},before;
    struct pt_sampler sampler_before;struct pt_pcm output;struct pt_sample_version *token=NULL;
    unsigned ready=7;size_t allocations,bytes;
    assert(pt_sampler_pin_job_begin(&job,&f->sampler,&f->project,0,0)==PT_EDIT_OK);
    memcpy(&before,&job,sizeof(before));memcpy(&sampler_before,&f->sampler,sizeof(sampler_before));
    allocations=f->allocations;bytes=f->sampler.bytes;
    assert(pt_sampler_pin_job_step(&job,1,(struct pt_pcm *)(void *)&job,&token,&ready)==PT_EDIT_ALIAS);
    assert(!memcmp(&before,&job,sizeof(job))&&!memcmp(&sampler_before,&f->sampler,sizeof(sampler_before)));
    assert(!token&&ready==7&&f->allocations==allocations&&f->sampler.bytes==bytes);
    ready=0;
    while(!ready)assert(pt_sampler_pin_job_step(&job,PT_SAMPLER_PIN_CHUNK,&output,&token,&ready)==PT_EDIT_OK);
    assert(token&&!job.value&&pt_sampler_lifetime_idle(&f->sampler)&&f->sampler.generation==0);
    assert(!memcmp(output.data,f->original,sizeof(f->original)));pt_sampler_unpin(token);finish(f);
}
static void cleanup_pre_pin_cancel(void)
{
    struct fixture *f=begin();struct pt_sampler_workflow *job=NULL;
    struct pt_project project;struct pt_pattern_history history;uint8_t selected[PT_PROJECT_SAMPLES]={0};
    scan(f);selected[0]=1;memcpy(&project,&f->project,sizeof(project));memcpy(&history,&f->history,sizeof(history));
    assert(pt_sampler_cleanup_begin(&f->sampler,&f->project,&f->history,&f->preview,selected,1,&job)==PT_EDIT_OK);
    assert(job&&!pt_sampler_lifetime_idle(&f->sampler));
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,job,1));
    pt_sampler_workflow_cancel(&job);
    assert(!job&&pt_sampler_lifetime_idle(&f->sampler));
    assert(!memcmp(&project,&f->project,sizeof(project))&&!memcmp(&history,&f->history,sizeof(history)));finish(f);
}
static void copy_job_to_journal(void)
{
    struct fixture *f=begin();struct pt_sampler_workflow *job=NULL;struct pt_sampler_workflow_stats stats;
    scan(f);
    assert(pt_sampler_copy_begin(&f->sampler,&f->project,&f->history,&f->preview,0,1,5,1,&job)==PT_EDIT_OK);
    assert(!pt_sampler_lifetime_idle(&f->sampler));workflow_ready(job);
    assert(!pt_sampler_lifetime_idle(&f->sampler));
    assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_OK);
    assert(!job&&pt_sampler_lifetime_idle(&f->sampler)&&f->history.count==1);
    assert(f->project.samples[1].pcm.frames==4&&f->project.samples[1].pcm.data[0]==17);
    assert(pt_pattern_undo(&f->project,&f->history,-1)==PT_EDIT_OK);
    assert(!f->project.samples[1].pcm.frames);
    assert(pt_pattern_undo(&f->project,&f->history,1)==PT_EDIT_OK);
    assert(f->project.samples[1].pcm.data[0]==17);finish(f);
}
static void establishment_all_phases(void)
{
    struct fixture *f=begin();enum pt_establish_result r;unsigned i,validation=0,partial=0;
    r=pt_sampler_establish_begin(&f->establishment,&f->sampler,&f->project,&f->allocator,NULL,0,f->history.revision);
    assert(r==PT_ESTABLISH_PENDING&&f->sampler.pending_count==1&&f->sampler.live_count==1);
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,f->establishment.metadata,1));
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,&f->establishment,sizeof(f->establishment)));
    for(i=0;i<10000&&r==PT_ESTABLISH_PENDING;++i){
        assert(!pt_sampler_lifetime_idle(&f->sampler));
        r=pt_sampler_establish_step(&f->establishment,f->history.revision,f->sampler.generation,1);
        if(f->establishment.validation.project)validation=1;
        if(f->establishment.job.value){partial=1;assert(f->sampler.pending_count==2);}
    }
    assert(r==PT_ESTABLISH_READY&&validation&&partial&&f->establishment.active);
    assert(!pt_sampler_lifetime_idle(&f->sampler)&&f->sampler.generation==0&&!f->history.count);
    assert(pt_sampler_establish_cancel(&f->establishment));
    assert(pt_sampler_lifetime_idle(&f->sampler)&&f->sampler.current[0]&&f->sampler.current[1]);
    finish(f);
}
static void failed_establishment_until_cancel(void)
{
    struct fixture *f=begin();enum pt_establish_result r;f->fail_next=1;
    r=pt_sampler_establish_begin(&f->establishment,&f->sampler,&f->project,&f->allocator,NULL,0,f->history.revision);
    assert(r==PT_ESTABLISH_CAPACITY&&f->establishment.active&&!pt_sampler_lifetime_idle(&f->sampler));
    assert(f->sampler.pending_count==1&&!f->sampler.live_count&&!f->sampler.bytes);
    assert(pt_sampler_establish_cancel(&f->establishment));
    assert(pt_sampler_lifetime_idle(&f->sampler));
    /* This genuine failure allocated no owner; no fake release is expected. */
    assert(!f->live_allocations&&!f->live_bytes&&!f->sampler.lifetime_fault);free(f);
}
static void genuine_bound_refusal(void)
{
    struct fixture *f=begin();struct pt_pcm pcm;struct pt_sample_version *token=pin(f,&pcm);
    unsigned i;size_t bytes,count,calls;unsigned output=0;
    pt_sampler_unpin(token);
    for(i=0;i<2050;++i)
        assert(pt_sampler_attributes(&f->sampler,&f->project,&f->history,0,i&1?"A":"B",64,0)==PT_EDIT_OK);
    assert(f->sampler.live_count>PT_SAMPLER_LIVE_CHECK_LIMIT&&pt_sampler_lifetime_idle(&f->sampler));
    bytes=f->sampler.bytes;count=f->sampler.live_count;calls=f->allocations;
    assert(!pt_sampler_lifetime_output_disjoint(&f->sampler,&output,sizeof(output)));
    assert(!output&&bytes==f->sampler.bytes&&count==f->sampler.live_count&&calls==f->allocations);
    finish(f);
}
static void table_postcallback_budget_refusal(void)
{
    struct fixture *f=begin();struct pt_project project;struct pt_pattern_history history;
    memcpy(&project,&f->project,sizeof(project));memcpy(&history,&f->history,sizeof(history));
    /* First genuine allocation is the slot journal control; second is table.
     * This deliberate scalar budget change demands clean capacity refusal. */
    f->budget_drop_after=2;
    assert(pt_sampler_add_slot(&f->sampler,&f->project,&f->history)==PT_EDIT_CAPACITY);
    assert(!memcmp(&project,&f->project,sizeof(project))&&!memcmp(&history,&f->history,sizeof(history)));
    assert(pt_sampler_lifetime_idle(&f->sampler)&&!f->sampler.table);
    emptied(f);free(f);
}
static void workflow_control_stale_release(void)
{
    struct fixture *f=begin();struct pt_sampler_workflow *job=NULL;
    struct pt_pattern_history history;struct pt_sample samples[2];uint8_t selected[PT_PROJECT_SAMPLES]={0};
    scan(f);selected[0]=1;memcpy(&history,&f->history,sizeof(history));memcpy(samples,f->samples,sizeof(samples));
    f->stale_after=1;
    assert(pt_sampler_cleanup_begin(&f->sampler,&f->project,&f->history,&f->preview,selected,1,&job)==PT_EDIT_CONFLICT);
    assert(!job&&f->stale_observed==1&&f->project.bpm==126);
    assert(!memcmp(&history,&f->history,sizeof(history))&&!memcmp(samples,f->samples,sizeof(samples)));
    emptied(f);free(f);
}
static void workflow_version_stale_release(void)
{
    struct fixture *f=begin();struct pt_sampler_workflow *job=NULL;
    struct pt_pattern_history history;struct pt_sample samples[2];uint8_t selected[PT_PROJECT_SAMPLES]={0};
    scan(f);selected[0]=1;memcpy(&history,&f->history,sizeof(history));memcpy(samples,f->samples,sizeof(samples));
    /* Real control allocation completes; its first unpublished version then goes stale. */
    f->stale_after=2;
    assert(pt_sampler_cleanup_begin(&f->sampler,&f->project,&f->history,&f->preview,selected,1,&job)==PT_EDIT_CONFLICT);
    assert(!job&&f->stale_observed==1&&f->project.bpm==126);
    assert(!memcmp(&history,&f->history,sizeof(history))&&!memcmp(samples,f->samples,sizeof(samples)));
    emptied(f);free(f);
}
static void establishment_metadata_stale_release(void)
{
    struct fixture *f=begin();struct pt_pattern_history history;struct pt_sample samples[2];
    memcpy(&history,&f->history,sizeof(history));memcpy(samples,f->samples,sizeof(samples));f->stale_after=1;
    assert(pt_sampler_establish_begin(&f->establishment,&f->sampler,&f->project,&f->allocator,NULL,0,f->history.revision)==PT_ESTABLISH_STALE);
    assert(f->stale_observed==1&&f->project.bpm==126&&f->establishment.active);
    assert(!pt_sampler_lifetime_idle(&f->sampler)&&!f->live_allocations);
    assert(!memcmp(&history,&f->history,sizeof(history))&&!memcmp(samples,f->samples,sizeof(samples)));
    assert(pt_sampler_establish_cancel(&f->establishment));emptied(f);free(f);
}
static void establishment_master_stale_release(void)
{
    struct fixture *f=begin();enum pt_establish_result r;unsigned i;
    struct pt_pattern_history history;struct pt_sample samples[2];
    memcpy(&history,&f->history,sizeof(history));memcpy(samples,f->samples,sizeof(samples));
    assert(pt_sampler_establish_begin(&f->establishment,&f->sampler,&f->project,&f->allocator,NULL,0,f->history.revision)==PT_ESTABLISH_PENDING);
    /* Metadata is genuine/live. The next real allocation is a guarded master. */
    f->stale_after=1;r=PT_ESTABLISH_PENDING;
    for(i=0;i<10000&&r==PT_ESTABLISH_PENDING;++i)
        r=pt_sampler_establish_step(&f->establishment,f->history.revision,f->sampler.generation,1);
    assert(r==PT_ESTABLISH_STALE&&f->stale_observed==1&&f->project.bpm==126);
    assert(f->establishment.active&&!pt_sampler_lifetime_idle(&f->sampler)&&f->live_allocations==1);
    assert(!memcmp(&history,&f->history,sizeof(history))&&!memcmp(samples,f->samples,sizeof(samples)));
    assert(pt_sampler_establish_cancel(&f->establishment));emptied(f);free(f);
}
int main(void)
{
    promotion();old_external_pcm_history();metadata_backing();owned_spare_and_table();
    current_job_cancel();job_alias_then_transfer();cleanup_pre_pin_cancel();copy_job_to_journal();
    establishment_all_phases();failed_establishment_until_cancel();genuine_bound_refusal();table_postcallback_budget_refusal();
    workflow_control_stale_release();workflow_version_stale_release();
    establishment_metadata_stale_release();establishment_master_stale_release();
    puts("sampler lifetime16 genuine owner cases");return 0;
}
