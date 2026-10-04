#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/editor/editor.h"
#include "../src/editor/sampler_internal.h"
#define FRAMES 1048576U
#define CHANNELS 2U
#define VALUES ((size_t)FRAMES * CHANNELS)
#define PADDING 64U
struct memory {size_t live,bytes,peak,largest,requests,total_requested;};
union allocation {long double alignment;struct {size_t bytes;} info;};
static void *allocate(void *context,size_t bytes)
{
    struct memory *m=context;union allocation *a;
    assert(bytes<=SIZE_MAX-sizeof(*a) && bytes<=SIZE_MAX-m->bytes && bytes<=SIZE_MAX-m->total_requested);
    a=malloc(sizeof(*a)+bytes);assert(a);a->info.bytes=bytes;
    ++m->live;++m->requests;m->bytes+=bytes;m->total_requested+=bytes;
    if(m->bytes>m->peak)m->peak=m->bytes;
    if(bytes>m->largest)m->largest=bytes;
    return a+1;
}
static void release(void *context,void *data)
{
    struct memory *m=context;union allocation *a=(union allocation *)data-1;
    assert(m->live && m->bytes>=a->info.bytes);--m->live;m->bytes-=a->info.bytes;free(a);
}
static clock_t now(void) {clock_t n=clock();assert(n!=(clock_t)-1);return n;}
static double seconds(clock_t ticks) {return (double)ticks/(double)CLOCKS_PER_SEC;}
static int32_t value(size_t i)
{
    if(i%997U==0)return i&1U?8388607:-8388608;
    return (int32_t)((i*257U)&0xffffffU)-8388608;
}
static void verify_master(const int32_t *pcm)
{
    size_t i;
    for(i=0;i<VALUES;++i)assert(pcm[i]==value(i));
    for(;i<VALUES+PADDING;++i)assert(pcm[i]==0x123456);
}
static void verify_bins(const struct pt_wave_summary *summary,const int32_t *pcm)
{
    unsigned column,channel;
    for(column=0;column<PT_WAVE_SUMMARY_COLUMNS;++column)for(channel=0;channel<CHANNELS;++channel) {
        uint32_t first=(uint32_t)((uint64_t)FRAMES*column/PT_WAVE_SUMMARY_COLUMNS);
        uint32_t end=(uint32_t)((uint64_t)FRAMES*(column+1U)/PT_WAVE_SUMMARY_COLUMNS),frame;
        int32_t minimum=pcm[(size_t)first*CHANNELS+channel],maximum=minimum;
        const struct pt_wave_summary_bin *bin=summary->bins+(size_t)column*CHANNELS+channel;
        for(frame=first+1;frame<end;++frame) {
            int32_t v=pcm[(size_t)frame*CHANNELS+channel];
            if(v<minimum)minimum=v;
            if(v>maximum)maximum=v;
        }
        assert(bin->minimum==minimum && bin->maximum==maximum);
    }
}
int main(void)
{
    struct memory m={0};struct pt_allocator a={&m,allocate,release};struct pt_document d;
    struct pt_editor *e=allocate(&m,sizeof(*e));int32_t *pcm=allocate(&m,(VALUES+PADDING)*sizeof(*pcm));
    struct pt_sample original;struct pt_wave_summary previous;struct pt_sample_range view={0,FRAMES};
    uint32_t markers[4]={1024,FRAMES/4,FRAMES/2,FRAMES-1};size_t i,reads=0,base_bytes,copy_live,sampler_live,copy_requests;
    size_t peak,largest,requests,total_requested,live_after_undo;
    unsigned ready=0,steps=0,max_reads=0,copy_steps=0,warm_calls=128;
    clock_t t,start,summary_begin,summary_total,summary_max=0,warm_total,copy_request,copy_total,copy_max=0,undo_ticks,redo_ticks,dispose_ticks;
    pt_document_init(&d,&a);assert(pt_document_new(&d,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<VALUES;++i)pcm[i]=value(i);
    for(;i<VALUES+PADDING;++i)pcm[i]=0x123456;
    d.project.samples[0].pcm=(struct pt_pcm){pcm,VALUES+PADDING,FRAMES,44100,CHANNELS,24};
    strcpy(d.project.samples[0].name,"8 MiB stereo24 master");d.project.samples[0].volume=37;d.project.samples[0].finetune=-2;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_start=FRAMES/3;d.project.samples[0].loop_end=FRAMES/2;
    d.project.samples[0].slices=markers;d.project.samples[0].slice_count=4;
    d.project.events[0].instrument=1;d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;
    assert(pt_editor_init(e,&d.project));pt_sampler_init(&e->sampler,&a,32UL*1024*1024);
    pt_document_init(&e->sample_source,&a);pt_song_init(&e->song,&a,8UL*1024*1024);
    original=d.project.samples[0];e->sample=1;e->panel=5;pt_editor_sample_all(e);
    start=now();assert(pt_wave_summary_begin(&e->workflow.wave_job,&d.project,0,e->sampler.generation,&view,
        e->workflow.wave_bins[0],PT_WAVE_SUMMARY_BINS)==PT_WAVE_SUMMARY_OK);summary_begin=now()-start;
    start=now();
    do {
        enum pt_wave_summary_result r;
        t=now();r=pt_wave_summary_step(&e->workflow.wave_job,e->sampler.generation,&ready);t=now()-t;
        assert(r==PT_WAVE_SUMMARY_OK || r==PT_WAVE_SUMMARY_PENDING);
        assert(e->workflow.wave_job.last_values<=PT_WAVE_SUMMARY_VALUES_PER_STEP);
        reads+=e->workflow.wave_job.last_values;
        if(e->workflow.wave_job.last_values>max_reads)max_reads=e->workflow.wave_job.last_values;
        if(t>summary_max)summary_max=t;
        assert(++steps<=VALUES/PT_WAVE_SUMMARY_VALUES_PER_STEP+1);
    } while(!ready);
    assert(pt_wave_summary_take(&e->workflow.wave_job,e->sampler.generation,&e->workflow.wave)==PT_WAVE_SUMMARY_OK);
    summary_total=now()-start;assert(reads==VALUES && steps==512 && max_reads==4096);
    verify_bins(&e->workflow.wave,pcm);previous=e->workflow.wave;
    start=now();
    for(i=0;i<warm_calls;++i) {
        e->sample_start=(uint32_t)i;e->sample_end=FRAMES-(uint32_t)i;
        pt_editor_status(e,"SELECTION/STATUS ONLY");(void)pt_editor_workflow_idle(e);
        assert(!e->workflow.wave_building && !e->workflow.wave_job.last_values);
        assert(!memcmp(&previous,&e->workflow.wave,sizeof(previous)));
    }
    warm_total=now()-start;
    e->sample_start=FRAMES/4;e->sample_end=FRAMES*3U/4;
    assert(pt_editor_key(e,0x27,9)==PT_UI_NONE && e->panel==PT_WORKFLOW_TOOLBOX);
    /* Keep copy observation separate from overview: existing cancelled-summary
     * state suppresses automatic builds; no stale descriptor is drawn here. */
    e->workflow.wave_cancelled=1;base_bytes=m.bytes;copy_requests=m.requests;
    start=now();assert(pt_editor_key(e,0x33,0)==PT_UI_NONE);copy_request=now()-start;
    start=now();
    while(e->workflow.scanning || e->workflow.busy) {
        t=now();(void)pt_editor_workflow_idle(e);t=now()-t;
        if(t>copy_max)copy_max=t;
        /* Initial full-master validation now yields between bounded steps,
         * separately from the exact selected-range copy and input polls. */
        assert(++copy_steps<=VALUES/PT_PROJECT_VALIDATION_WORK_MAX+
            VALUES/2/(PT_SAMPLER_PIN_CHUNK/sizeof(int32_t))+128);
    }
    copy_total=now()-start;
    assert(copy_steps>=VALUES/PT_PROJECT_VALIDATION_WORK_MAX+
        VALUES/2/(PT_SAMPLER_PIN_CHUNK/sizeof(int32_t)));
    assert(e->sample==2 && e->history.count==1 && e->sampler.generation==1);
    assert(!memcmp(&original,d.project.samples,sizeof(original)) && !e->sampler.current[0]);
    {const struct pt_sample *copied=d.project.samples+1;
     assert(copied->pcm.frames==FRAMES/2 && copied->pcm.bits==24 && copied->pcm.channels==2 && copied->pcm.data!=pcm);
     for(i=0;i<(size_t)copied->pcm.frames*CHANNELS;++i)assert(copied->pcm.data[i]==pcm[(size_t)(FRAMES/4)*CHANNELS+i]);
     assert(copied->loop==PT_LOOP_FORWARD && copied->loop_start==FRAMES/3-FRAMES/4 && copied->loop_end==FRAMES/4);
     assert(copied->slice_count==2 && copied->slices[0]==0 && copied->slices[1]==FRAMES/4);
     assert(copied->volume==37 && copied->finetune==-2);}
    copy_requests=m.requests-copy_requests;copy_live=m.bytes;sampler_live=e->sampler.bytes;
    assert(sampler_live>VALUES/2*sizeof(int32_t) && copy_live>base_bytes && e->workflow.stats.master_bytes_released==0);
    start=now();assert(pt_editor_key(e,0x31,8)==PT_UI_NONE);undo_ticks=now()-start;
    live_after_undo=m.bytes;assert(!d.project.samples[1].pcm.frames && e->sampler.bytes==sampler_live && m.bytes==copy_live);
    start=now();assert(pt_editor_key(e,0x31,9)==PT_UI_NONE);redo_ticks=now()-start;
    assert(d.project.samples[1].pcm.frames==FRAMES/2 && m.bytes==copy_live && e->sampler.bytes==sampler_live);
    verify_master(pcm);assert(!memcmp(&original,d.project.samples,sizeof(original)));
    peak=m.peak;largest=m.largest;requests=m.requests;total_requested=m.total_requested;
    start=now();assert(pt_editor_dispose(e));dispose_ticks=now()-start;assert(!e->sampler.bytes);
    pt_document_release(&d);release(&m,pcm);release(&m,e);assert(!m.live && !m.bytes);
    printf("WORKFLOW PERFORMANCE HOST: master_active_bytes=%lu master_capacity_bytes=%lu frames=%u bits=24 channels=2 summary_steps=%u summary_reads=%lu summary_max_values=%u summary_begin_cpu_s=%.9f summary_total_cpu_s=%.9f summary_max_step_cpu_s=%.9f warm_calls=%u warm_cpu_s=%.9f copy_active_bytes=%lu copy_idle_calls=%u copy_request_cpu_s=%.9f copy_idle_total_cpu_s=%.9f copy_max_idle_cpu_s=%.9f undo_cpu_s=%.9f redo_cpu_s=%.9f dispose_cpu_s=%.9f allocator_base_bytes=%lu allocator_copy_live_bytes=%lu allocator_undo_live_bytes=%lu sampler_retained_bytes=%lu copy_allocations=%lu allocator_peak_requested_bytes=%lu allocator_largest_request=%lu allocator_requests=%lu allocator_cumulative_requested_bytes=%lu final_live_allocations=0 final_requested_bytes=0\n",
        (unsigned long)(VALUES*sizeof(int32_t)),(unsigned long)((VALUES+PADDING)*sizeof(int32_t)),FRAMES,steps,(unsigned long)reads,max_reads,
        seconds(summary_begin),seconds(summary_total),seconds(summary_max),warm_calls,seconds(warm_total),(unsigned long)(VALUES/2*sizeof(int32_t)),copy_steps,
        seconds(copy_request),seconds(copy_total),seconds(copy_max),seconds(undo_ticks),seconds(redo_ticks),seconds(dispose_ticks),
        (unsigned long)base_bytes,(unsigned long)copy_live,(unsigned long)live_after_undo,(unsigned long)sampler_live,(unsigned long)copy_requests,
        (unsigned long)peak,(unsigned long)largest,(unsigned long)requests,(unsigned long)total_requested);
    puts("WORKFLOW PERFORMANCE PASS: host process CPU observations and allocator requested payload bytes only; not RSS, native030 latency, native memory availability, device, audio or physical acceptance");return 0;
}
