#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/editor/mixed_quantized_audit.c"
#include "../src/editor/mixed_preflight.h"
#include "../src/editor/sampler_internal.h"
#include "../src/core/amigus_render_voice.h"
/* SOURCE-ONLY helper draft. Not included, compiled or executed here.
 * Fixture initializer and literal expectations only; no audit owner, renderer
 * traversal, allocator, pin, ready certificate or target operation is provided.
 * Author must use genuine project/master lifetime and actual audit APIs.
 * Supply six readable/writable, mutually disjoint caller objects/extents.
 * Keep ALL declared source capacity alive and immutable after initialization.
 */
#include <limits.h>
#include <string.h>
#include "render.h"
#define QF_PCM_CAPACITY 16384U
#define QF_EVENT_COUNT (64U*4U)
enum qf_case { QF_LATE_PAULA_901, QF_TEMPO_DELAY, QF_EMPTY };
struct qf_boundary {
    unsigned frames; uint64_t frame;
    unsigned raw_actions, normalized_records, emit, end;
};
/* UINT_MAX at the late boundary means Q refusal, not an output count. The final
 * late row below is an independent renderer/old-capability expectation only:
 * a strict audit must already have refused and must not reach/publish it. */
static const struct qf_boundary qf_late[] = {
    {0,0,2,1,1,0},
    {960,960,1,1,1,0},
    {960,1920,1,1,1,0},
    {960,2880,2,UINT_MAX,1,0},
    {960,3840,0,0,1,1}
};
static const struct qf_boundary qf_tempo_trimmed[] = {
    {0,0,0,0,1,0},
    {0,0,2,1,1,0},
    {960,960,1,1,1,0},
    {960,1920,1,1,1,0},
    {875,2795,1,1,1,0},
    {876,3671,1,1,1,0},
    {876,4547,1,1,1,0},
    {876,5423,1,1,1,0},
    {876,6299,1,1,1,0},
    {876,7175,0,0,1,1}
};
static const struct qf_boundary qf_tempo_lead[] = {
    {960,960,0,0,1,0},
    {960,1920,2,1,1,0},
    {960,2880,1,1,1,0},
    {960,3840,1,1,1,0},
    {875,4715,1,1,1,0},
    {876,5591,1,1,1,0},
    {876,6467,1,1,1,0},
    {876,7343,1,1,1,0},
    {876,8219,1,1,1,0},
    {876,9095,0,0,1,1}
};
static const struct qf_boundary qf_empty[] = {
    {0,0,0,0,1,0},
    {960,960,0,0,1,0},
    {960,1920,0,0,1,1}
};
/* Preconditions: which is one of the three enum values; lead is 0/1 and only
 * QF_TEMPO_DELAY uses lead=1. Full source PCM extent is 16384 int32_t values,
 * including spare values. This is not a guarded public initializer/API. */
static void qf_init(struct pt_project *p,struct pt_sample *sample,
    struct pt_event events[QF_EVENT_COUNT],uint16_t orders[1],
    int32_t pcm[QF_PCM_CAPACITY],struct pt_render_options *options,
    enum qf_case which,unsigned lead)
{
    unsigned i,frames=which==QF_LATE_PAULA_901?4096U:
        which==QF_TEMPO_DELAY?16384U:16U;
    memset(p,0,sizeof(*p));memset(sample,0,sizeof(*sample));
    memset(events,0,QF_EVENT_COUNT*sizeof(*events));orders[0]=0;
    memset(options,0,sizeof(*options));
    /* A nonzero first word avoids silent-handoff SEGMENT geometry. */
    for(i=0;i<QF_PCM_CAPACITY;++i)pcm[i]=(i&1U)?-1:1;
    pt_channels_init(&p->channels);
    p->orders=orders;p->events=events;p->samples=sample;
    p->order_count=p->pattern_count=p->sample_count=1;
    p->speed=which==QF_TEMPO_DELAY?2:1;p->bpm=125;
    sample->pcm=(struct pt_pcm){pcm,QF_PCM_CAPACITY,frames,8000,1,8};
    sample->volume=64;sample->loop=PT_LOOP_NONE;
    options->rate=48000;options->bits=24;options->gain_q16=65536;
    options->tracks=15; /* All extant tracks, not the earlier partial-track note. */
    options->tick_limit=100;options->frame_limit=100000;
    options->include_lead_in=(uint8_t)lead;
    if(which!=QF_EMPTY)events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    if(which==QF_LATE_PAULA_901){
        events[3*4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,9,1,0,0};
        events[4*4+3].effect=15; /* F00, parameter remains zero. */
    }else if(which==QF_TEMPO_DELAY){
        events[1*4+3].effect=15;events[1*4+3].parameter=137;
        events[2*4+3].effect=14;events[2*4+3].parameter=0xe1;
        events[3*4+3].effect=15;
    }else events[2*4+3].effect=15;
}

struct qa_memory {unsigned calls,releases,live;void *pointer[64];};
struct qa_fixture {
    struct pt_project project;struct pt_sample sample;
    struct pt_event events[64*16];uint16_t order;
    int32_t pcm[QF_PCM_CAPACITY];uint32_t slices[2];
    struct pt_render_options options;struct pt_paula_render_caps caps;
    struct pt_playback_format format;struct pt_allocator allocator,master_allocator;
    struct pt_sampler sampler;struct pt_sample_version *pin;
    struct pt_mixed_quantized_audit_inputs inputs;struct pt_mixed_plan_span contexts[30];
    struct pt_mixed_quantized_audit **owner;size_t owner_capacity;
    void *workspace,*normalizer,*result;size_t capacity,ncapacity,rcapacity;
    struct qa_memory memory,masters;
    void *alias;unsigned fail_at,hook_at,hook_release,hook_mode,hook_calls;
};
static void *qa_plain_alloc(void *context,size_t n)
{
    struct qa_memory *m=context;void *p=malloc(n);unsigned i;
    assert(p);++m->calls;++m->live;
    for(i=0;i<64&&m->pointer[i];++i){}
    assert(i<64);m->pointer[i]=p;return p;
}
static void qa_plain_release(void *context,void *p)
{
    struct qa_memory *m=context;unsigned i;
    for(i=0;i<64&&m->pointer[i]!=p;++i){}
    assert(i<64&&m->live);m->pointer[i]=NULL;--m->live;++m->releases;free(p);
}
static void qa_hook(struct qa_fixture *f)
{
    struct pt_mixed_quantized_audit_report r;
    ++f->hook_calls;
    if(f->hook_mode==1)
        assert(pt_mixed_quantized_audit_get(*f->owner,f->inputs.revision,f->inputs.generation,&r)==PT_MIXED_QAUDIT_BUSY);
    else if(f->hook_mode==2)++f->inputs.revision;
    else if(f->hook_mode==3){
        f->project.samples=(void *)(uintptr_t)1;
        assert(pt_mixed_quantized_audit_begin_in_workspace(f->workspace,f->capacity,&f->inputs,f->owner)==PT_MIXED_QAUDIT_BUSY);
    }else if(f->hook_mode==4){
        uint8_t *snapshot=malloc(sizeof(r));assert(snapshot);
        /* The complete fixture context is an admitted ordinary calloc span.
         * Its aligned base fits the full report; embedded PCM alignment alone
         * does not satisfy the report output contract. */
        assert(!((uintptr_t)f%_Alignof(struct pt_mixed_quantized_audit_report))&&sizeof(r)<=sizeof(*f));
        memcpy(snapshot,f,sizeof(r));
        assert(pt_mixed_quantized_audit_get(*f->owner,31,7,(void *)f)==PT_MIXED_QAUDIT_ALIAS);
        assert(!memcmp(snapshot,f,sizeof(r)));free(snapshot);
    }
}
static void *qa_allocate(void *context,size_t n)
{
    struct qa_fixture *f=context;void *p;
    if(f->fail_at&&f->memory.calls+1==f->fail_at){++f->memory.calls;return NULL;}
    if(f->alias){p=f->alias;f->alias=NULL;++f->memory.calls;}
    else p=qa_plain_alloc(&f->memory,n);
    if(f->hook_at&&f->memory.calls==f->hook_at){f->hook_at=0;qa_hook(f);}
    return p;
}
static void qa_release(void *context,void *p)
{
    struct qa_fixture *f=context;
    qa_plain_release(&f->memory,p);
    if(f->hook_release){f->hook_release=0;qa_hook(f);}
}
static uint8_t *qa_save(struct qa_fixture *f,size_t *size)
{
    uint8_t *p;size_t used;
    assert(pt_project_size(&f->project,size)==PT_PROJECT_OK);p=malloc(*size);assert(p);
    assert(pt_project_encode(&f->project,p,*size,&used)==PT_PROJECT_OK&&used==*size);return p;
}
static struct qa_fixture *qa_make_storage(enum qf_case which,unsigned lead,unsigned bits,unsigned card,unsigned spare)
{
    struct qa_fixture *f=calloc(1,sizeof(*f));unsigned i,n;struct pt_pcm pcm;
    struct pt_sampler_storage_span spans[6];size_t padding=sizeof(struct pt_mixed_quantized_audit_report);assert(f);
    qf_init(&f->project,&f->sample,f->events,&f->order,f->pcm,&f->options,which,lead);
    f->sample.pcm.bits=(uint8_t)bits;
    for(i=0;i<QF_PCM_CAPACITY;++i)f->pcm[i]*=(int32_t)(1U<<(bits-8));
    if(bits==24){f->pcm[2]=257;f->pcm[3]=-257;}
    if(card){for(i=0;i<4;++i){f->project.channels.track[i].route=PT_AMIGUS;f->project.channels.track[i].pan=128;}}
    f->master_allocator=(struct pt_allocator){&f->masters,qa_plain_alloc,qa_plain_release};
    pt_sampler_init(&f->sampler,&f->master_allocator,4*1024*1024);
    /* Only the spare alias fixtures adopt a real full-capacity allocation.
     * The ordinary pin path intentionally keeps its compact genuine master. */
    if(spare){
        struct pt_pattern_history history;struct pt_pattern_command command[1];
        struct pt_event_change change[1];struct pt_pcm owned=f->sample.pcm;
        int32_t *allocated;unsigned frames=owned.frames;
        uint8_t *saved,*again;size_t saved_size,again_size;
        assert(which==QF_EMPTY&&bits==8&&frames==16&&owned.channels==1);
        saved=qa_save(f,&saved_size);
        owned.data=qa_plain_alloc(&f->masters,QF_PCM_CAPACITY*sizeof(*owned.data));
        allocated=owned.data;memcpy(owned.data,f->pcm,QF_PCM_CAPACITY*sizeof(*owned.data));
        f->project.sample_count=0;
        assert(pt_pattern_history_init(&history,&f->project,command,1,change,1)==PT_EDIT_OK);
        assert(pt_sampler_append_owned(&f->sampler,&f->project,&history,&owned,
            &f->master_allocator,"")==PT_EDIT_OK);
        assert(!owned.data&&!owned.capacity&&!owned.frames&&!owned.rate&&
            !owned.channels&&!owned.bits&&history.count==1);
        assert(f->project.samples==f->sampler.table&&f->project.sample_count==1&&
            f->sampler.table_original==&f->sample&&
            f->sampler.table_bytes==PT_PROJECT_SAMPLES*sizeof(struct pt_sample));
        assert(f->project.samples[0].pcm.data==allocated&&
            f->project.samples[0].pcm.frames==frames&&
            f->project.samples[0].pcm.capacity==QF_PCM_CAPACITY&&f->sampler.current[0]);
        /* Drop the finished append command before borrow; actual current
         * ownership retains the adopted allocation and genuine expanded table. */
        pt_pattern_history_release(&history);assert(!history.count&&f->sampler.current[0]);
        assert(f->sampler.bytes>=QF_PCM_CAPACITY*sizeof(*allocated));
        again=qa_save(f,&again_size);assert(again_size==saved_size&&!memcmp(saved,again,saved_size));
        free(again);free(saved);
        if(padding<pt_render_sequence_setup_control_size())padding=pt_render_sequence_setup_control_size();
        if(padding<pt_render_sequence_control_size())padding=pt_render_sequence_control_size();
    }
    /* Establish and pin BEFORE any renderer borrow. No audit source promotion. */
    assert(pt_sampler_pin(&f->sampler,&f->project,0,f->sampler.generation,&pcm,&f->pin)==PT_EDIT_OK);
    assert(f->pin==f->sampler.current[0]&&pcm.bits==bits&&
        pcm.data==f->project.samples[0].pcm.data&&pcm.frames==f->project.samples[0].pcm.frames);
    if(spare)assert(pcm.frames==16&&pcm.capacity==QF_PCM_CAPACITY);
    f->contexts[0]=(struct pt_mixed_plan_span){f,sizeof(*f)};
    assert(pt_sampler_version_spans(f->pin,spans,6,&n));assert(n<30);
    for(i=0;i<n;++i)f->contexts[1+i]=(struct pt_mixed_plan_span){spans[i].data,spans[i].bytes};
    if(spare){assert(n<29);f->contexts[1+n++]=(struct pt_mixed_plan_span){f->sampler.table,f->sampler.table_bytes};}
    assert(padding<=SIZE_MAX-128);
    f->capacity=pt_mixed_quantized_audit_workspace_size()+padding+128;
    f->ncapacity=pt_mixed_plan_normalizer_workspace_size()+padding+128;
    f->rcapacity=pt_mixed_quantized_audit_result_size()+padding+128;
    f->owner_capacity=padding+64;
    f->owner=calloc(1,f->owner_capacity);f->workspace=calloc(1,f->capacity);
    f->normalizer=calloc(1,f->ncapacity);f->result=calloc(1,f->rcapacity);
    assert(f->owner&&f->workspace&&f->normalizer&&f->result);
    memset((uint8_t *)f->workspace+pt_mixed_quantized_audit_workspace_size(),0xa7,f->capacity-pt_mixed_quantized_audit_workspace_size());
    memset((uint8_t *)f->normalizer+pt_mixed_plan_normalizer_workspace_size(),0xb7,f->ncapacity-pt_mixed_plan_normalizer_workspace_size());
    memset((uint8_t *)f->result+pt_mixed_quantized_audit_result_size(),0xc7,f->rcapacity-pt_mixed_quantized_audit_result_size());
    f->caps=(struct pt_paula_render_caps){3546895,124,65535};f->format.bits=16;
    f->allocator=(struct pt_allocator){f,qa_allocate,qa_release};
    f->inputs=(struct pt_mixed_quantized_audit_inputs){&f->project,&f->options,&f->caps,&f->format,
        &f->allocator,f->contexts,n+1,f->normalizer,f->ncapacity,f->result,f->rcapacity,
        f->capacity+f->ncapacity+f->rcapacity+pt_render_sequence_setup_control_size()+
        pt_render_sequence_control_size(),31,f->sampler.generation};
    assert(pt_project_validate(&f->project,NULL)==PT_PROJECT_OK);return f;
}
static struct qa_fixture *qa_make(enum qf_case which,unsigned lead,unsigned bits,unsigned card)
{return qa_make_storage(which,lead,bits,card,0);}
static struct qa_fixture *qa_make_spare(void)
{return qa_make_storage(QF_EMPTY,0,8,0,1);}
/* These are real writable fixture allocations, not merely numeric aliases.
 * Admission proves the complete report/publisher/request span and alignment. */
static void qa_alias_span(struct qa_fixture *f,void *p,size_t bytes,size_t alignment)
{
    const void *base[6]={f,f->project.samples[0].pcm.data,f->workspace,f->normalizer,f->result,f->owner};
    size_t size[6]={sizeof(*f),f->project.samples[0].pcm.capacity*sizeof(int32_t),
        f->capacity,f->ncapacity,f->rcapacity,f->owner_capacity};
    uintptr_t x=(uintptr_t)p;unsigned i;
    assert(bytes&&alignment&&!(x%alignment)&&bytes<=UINTPTR_MAX-x);
    for(i=0;i<6;++i){uintptr_t b=(uintptr_t)base[i];
        if(x>=b&&x-b<=size[i]&&bytes<=size[i]-(x-b))return;
    }
    assert(!"alias extent must fit a real writable fixture allocation");
}
static void *qa_spare_span(struct qa_fixture *f,size_t bytes,size_t alignment)
{
    struct pt_pcm *pcm=&f->project.samples[0].pcm;
    void *p;
    assert(pcm->frames==16&&pcm->channels==1&&pcm->capacity==QF_PCM_CAPACITY&&
        f->pin==f->sampler.current[0]&&pcm->frames<pcm->capacity);
    assert(bytes<=(pcm->capacity-pcm->frames)*sizeof(*pcm->data));
    p=pcm->data+pcm->frames;qa_alias_span(f,p,bytes,alignment);return p;
}
static void qa_begin(struct qa_fixture *f)
{
    assert(pt_mixed_quantized_audit_begin_in_workspace(f->workspace,f->capacity,&f->inputs,f->owner)==PT_MIXED_QAUDIT_PENDING);
    assert(*f->owner&&!f->memory.calls);
}
static enum pt_mixed_quantized_audit_result qa_step(struct qa_fixture *f,unsigned work)
{return pt_mixed_quantized_audit_step(*f->owner,f->inputs.revision,f->inputs.generation,work);}
static enum pt_mixed_quantized_audit_result qa_finish(struct qa_fixture *f,unsigned work)
{
    enum pt_mixed_quantized_audit_result r;unsigned calls=0;
    do{r=qa_step(f,work);assert((*f->owner)->report.last_work<=work&&++calls<100000);}while(r==PT_MIXED_QAUDIT_PENDING);
    return r;
}
static void qa_drain(struct qa_fixture *f)
{
    unsigned tries=0;enum pt_mixed_quantized_audit_result r;
    while(*f->owner){r=pt_mixed_quantized_audit_close(f->owner);assert(r!=PT_MIXED_QAUDIT_PENDING&&++tries<=2);}
    assert(pt_mixed_quantized_audit_close(f->owner)==PT_MIXED_QAUDIT_CLOSED);
}
static void qa_drop(struct qa_fixture *f)
{
    size_t i;qa_drain(f);assert(!f->memory.live);
    for(i=0;i<pt_mixed_quantized_audit_workspace_size();++i)assert(!((uint8_t *)f->workspace)[i]);
    for(;i<f->capacity;++i)assert(((uint8_t *)f->workspace)[i]==0xa7);
    for(i=pt_mixed_plan_normalizer_workspace_size();i<f->ncapacity;++i)assert(((uint8_t *)f->normalizer)[i]==0xb7);
    for(i=pt_mixed_quantized_audit_result_size();i<f->rcapacity;++i)assert(((uint8_t *)f->result)[i]==0xc7);
    pt_sampler_unpin(f->pin);pt_sampler_release(&f->sampler);assert(!f->masters.live&&!f->sampler.bytes);
    free(f->result);free(f->normalizer);free(f->workspace);free(f->owner);free(f);
}
static void qa_table(struct qa_fixture *f,const struct qf_boundary *table,unsigned count)
{
    struct pt_mixed_quantized_audit *j;unsigned boundary=0,loops=0;
    enum pt_mixed_quantized_audit_result r;
    qa_begin(f);j=*f->owner;
    do{
        enum pt_mixed_quantized_audit_phase phase=j->report.phase;
        r=qa_step(f,31);assert(++loops<100000&&j->report.last_work<=31);
        if(phase==PT_MIXED_QAUDIT_COMPLETE&&r==PT_MIXED_QAUDIT_PENDING){
            assert(boundary<count&&j->interval.frames==table[boundary].frames&&
                j->report.frames==table[boundary].frame&&j->interval.emit==table[boundary].emit&&
                j->interval.end==table[boundary].end&&j->plan.count==table[boundary].raw_actions);
        }
        if(phase==PT_MIXED_QAUDIT_Q_GET&&r==PT_MIXED_QAUDIT_PENDING){
            assert(j->result->batch.normalized.count==table[boundary].normalized_records);
            {unsigned i;
                for(i=0;i<j->result->batch.normalized.count;++i){
                    const struct pt_mixed_plan_record *rec=j->result->batch.normalized.record+i;
                    if(rec->kind==PT_MIXED_PLAN_TRIGGER){
                        struct pt_voice voice;const uint32_t *gains;
                        assert(rec->first_action<j->plan.count&&rec->control_action!=UINT_MAX&&rec->control_action<j->plan.count);
                        voice=j->plan.action[rec->first_action].voice;
                        voice.step=j->plan.action[rec->control_action].voice.step;
                        gains=j->plan.action[rec->control_action].gain;
                        if(rec->route==PT_AMIGUS){struct pt_amigus_voice_plan oracle;
                            assert(pt_amigus_render_voice(&voice,j->options.rate,gains,&j->format,0,
                                f->sample.pcm.frames*(j->format.bits/8),&oracle));
                            assert(rec->image.start==oracle.start&&rec->image.loop==oracle.loop&&rec->image.end_exclusive==oracle.end_exclusive&&
                                rec->image.rate==oracle.rate&&rec->image.control==oracle.control&&rec->image.left==oracle.left&&rec->image.right==oracle.right);
                            assert(oracle.start==0&&oracle.loop==0&&oracle.end_exclusive==f->sample.pcm.frames*(j->format.bits/8)&&
                                oracle.rate==44739242&&oracle.control==(0x8000|(j->format.bits==16?1:0)|
                                (j->format.bits==16&&j->format.little_endian?8:0))&&oracle.left==32639&&oracle.right==32895);
                            assert(j->result->batch.levels[i].left==32639&&j->result->batch.levels[i].right==32895);
                        }else {struct pt_paula_render_plan oracle;
                            assert(pt_paula_render_voice(&voice,j->options.rate,gains,rec->slot,&j->caps,&oracle));
                            assert(rec->geometry.paula.period==oracle.period&&rec->geometry.paula.volume==oracle.volume&&
                                oracle.period==443&&oracle.volume==64);
                        }
                    }
                }
            }
        }
        if(phase==PT_MIXED_QAUDIT_COMMIT){++boundary;assert(boundary<=count);}
    }while(r==PT_MIXED_QAUDIT_PENDING);
    assert(r==PT_MIXED_QAUDIT_READY&&boundary==count&&j->report.intervals==count);
}
static void qa_same_sequence(struct qa_fixture *f,const struct qf_boundary *table,unsigned count)
{
    struct pt_render_sequence *s=NULL,*original=(*f->owner)->sequence,*twice=NULL;
    struct pt_render_interval span;struct pt_render_plan plan;uint64_t frames=0;unsigned i,calls=f->memory.calls;
    assert(pt_mixed_quantized_audit_take(*f->owner,31,f->inputs.generation,&s)==PT_MIXED_QAUDIT_TAKEN&&s==original);
    assert((*f->owner)->report.rewinds==1&&f->memory.calls==calls);
    assert(pt_mixed_quantized_audit_take(*f->owner,31,f->inputs.generation,&twice)==PT_MIXED_QAUDIT_INVALID&&!twice);
    assert(pt_mixed_quantized_audit_close(f->owner)==PT_MIXED_QAUDIT_PENDING&&*f->owner);
    for(i=0;i<count;++i){unsigned remaining;
        assert(pt_render_sequence_next(s,&span)==PT_RENDER_OK);
        assert(span.frames==table[i].frames&&span.emit==table[i].emit&&span.end==table[i].end);
        remaining=span.frames;while(remaining){unsigned n=remaining>127?127:remaining;
            assert(pt_render_sequence_consume(s,n)==PT_RENDER_OK);remaining-=n;frames+=n;}
        memset(&plan,0,sizeof(plan));assert(pt_render_sequence_complete(s,&plan)==PT_RENDER_OK);
        assert(frames==table[i].frame&&plan.count==table[i].raw_actions);
    }
    /* Release remains positive without former source reads. Workspace is still
     * live during this genuine sequence's wrapped release. */
    f->project.samples=(void *)(uintptr_t)1;
    pt_render_sequence_close(s);assert(!f->memory.live&&*f->owner);
    assert(pt_mixed_quantized_audit_close(f->owner)==PT_MIXED_QAUDIT_CLOSED&&!*f->owner);
    f->project.samples=&f->sample;
}
static void qa_schedule(void)
{
    unsigned bits,card,lead;unsigned cases=0;
    for(bits=8;bits<=24;bits+=8)for(card=0;card<2;++card)for(lead=0;lead<2;++lead){
        struct qa_fixture *f=qa_make(QF_TEMPO_DELAY,lead,bits,card);
        const struct qf_boundary *table=lead?qf_tempo_lead:qf_tempo_trimmed;
        struct pt_mixed_quantized_audit_report r;uint8_t *before,*after;size_t a,b;
        before=qa_save(f,&a);qa_table(f,table,10);
        assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,&r)==PT_MIXED_QAUDIT_READY);
        assert(r.frames==table[9].frame&&r.intervals==10&&r.samples[card][0]==1&&!r.samples[1-card][0]);
        assert((*f->owner)->origins[0].present&&(*f->owner)->origins[0].frame==table[1].frame);
        assert(!(*f->owner)->plan.count&&(*f->owner)->origins[0].present); /* DONE is not STOP. */
        qa_same_sequence(f,table,10);after=qa_save(f,&b);assert(a==b&&!memcmp(before,after,a));
        free(after);free(before);qa_drop(f);++cases;
    }
    assert(cases==12);
    {struct qa_fixture *f=qa_make(QF_EMPTY,0,8,0);struct pt_mixed_quantized_audit_report r;unsigned i;
        qa_table(f,qf_empty,3);assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,&r)==PT_MIXED_QAUDIT_READY);
        for(i=0;i<PT_PROJECT_SAMPLES;++i)assert(!r.samples[0][i]&&!r.samples[1][i]);
        for(i=0;i<16;++i)assert(!(*f->owner)->origins[i].present);
        qa_same_sequence(f,qf_empty,3);qa_drop(f);}
    puts("STRICT Q AUDIT SCHEDULE PASS:12 genuine pinned8/16/24 Paula/card tempo-delay/lead-in lifetimes, literal every-boundary frames, exact centre32639/32895 seven-field images, SAME once-rewound pointer, exact master saves and empty DONE retention; SOFTWARE_ONLY");
}
static void qa_full16(void)
{
    const struct qf_boundary table[]={{0,0,32,16,1,0},{960,960,16,16,1,0},{960,1920,0,0,1,1}};
    unsigned cache,endian,i,cases=0;
    for(cache=8;cache<=16;cache+=8)for(endian=0;endian<2;++endian){
        struct qa_fixture *f=qa_make(QF_TEMPO_DELAY,0,24,0);
        struct pt_mixed_quantized_audit_report r;
        memset(f->events,0,sizeof(f->events));f->project.channels.count=16;f->project.speed=1;
        f->options.tracks=65535;f->format.bits=cache;f->format.little_endian=endian;
        for(i=0;i<16;++i){f->project.channels.track[i].route=(uint8_t)(i<4?PT_PAULA:PT_AMIGUS);
            if(i>=4)f->project.channels.track[i].pan=128;
            f->events[i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};}
        f->events[2*16+15].effect=15;
        assert(pt_project_validate(&f->project,NULL)==PT_PROJECT_OK);
        qa_table(f,table,3);
        assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,&r)==PT_MIXED_QAUDIT_READY);
        assert(r.samples[0][0]&&r.samples[1][0]&&r.intervals==3&&r.frames==1920);
        for(i=0;i<16;++i)assert((*f->owner)->origins[i].present&&(*f->owner)->origins[i].frame==0);
        qa_same_sequence(f,table,3);qa_drop(f);++cases;
    }
    assert(cases==4);
    puts("STRICT Q AUDIT FULL16 PASS:4 genuine mixed4Paula/12card full-song8/16-endian geometries,32 raw to16 exact records, origin15 retained through empty DONE, same sequence and literal fields; SOFTWARE_ONLY");
}
static void qa_late_refusal(void)
{
    struct qa_fixture *f=qa_make(QF_LATE_PAULA_901,0,8,0);struct pt_mixed_report old;
    struct pt_mixed_preflight *gate=NULL;struct pt_mixed_quantized_audit_report r;
    enum pt_mixed_result mr;struct pt_render_sequence *take=NULL;
    assert(pt_mixed_preflight_begin(&f->project,&f->options,NULL,&f->caps,&f->format,1,1,&f->allocator,&old,&gate)==PT_MIXED_PENDING);
    do{mr=pt_mixed_preflight_step(gate,&old);}while(mr==PT_MIXED_PENDING);
    assert(mr==PT_MIXED_OK&&old.frames==qf_late[4].frame&&old.frames==3840&&old.intervals==5&&old.samples[0][0]);
    pt_mixed_preflight_close(&gate);assert(!gate&&!f->memory.live);f->memory.calls=f->memory.releases=0;
    qa_begin(f);assert(qa_finish(f,31)==PT_MIXED_QAUDIT_REFUSED);
    assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,&r)==PT_MIXED_QAUDIT_REFUSED);
    assert(r.frames==2880&&r.normalizer.reason==PT_MIXED_PLAN_REASON_PAULA&&r.normalizer.action==0&&r.normalizer.track==0);
    assert((*f->owner)->plan.action[0].voice.start==256&&(*f->owner)->plan.action[0].voice.end==4096);
    assert(!r.samples[0][0]&&!r.samples[1][0]);
    assert(pt_mixed_quantized_audit_take(*f->owner,31,f->inputs.generation,&take)==PT_MIXED_QAUDIT_INVALID&&!take);
    qa_drop(f);
    puts("STRICT Q AUDIT LATE PASS:genuine renderer-valid Paula901 offset256/length3840 old-gate success becomes strict late refusal at2880 before masks/sequence transfer; no cache/key/enqueue callback exists; SOFTWARE_ONLY");
}
static void qa_guards_and_budget(void)
{
    struct qa_fixture *f=qa_make_spare();struct pt_mixed_quantized_audit_inputs original=f->inputs;
    unsigned i,refusals=0;void *aliases[8];uint8_t *snapshot,*saved,*master,*again;
    struct pt_mixed_quantized_audit_report before;size_t sum=123,save_size,again_size;
    size_t master_bytes=f->project.samples[0].pcm.capacity*sizeof(int32_t);
    saved=qa_save(f,&save_size);master=malloc(master_bytes);assert(master);
    memcpy(master,f->project.samples[0].pcm.data,master_bytes);
    assert(!add(SIZE_MAX,1,&sum)&&sum==123);
    assert(add(SIZE_MAX-1,1,&sum)&&sum==SIZE_MAX);
    aliases[0]=qa_spare_span(f,sizeof(before),_Alignof(struct pt_mixed_quantized_audit_report));
    aliases[1]=(uint8_t *)f->workspace+pt_mixed_quantized_audit_workspace_size();
    aliases[2]=(uint8_t *)f->normalizer+pt_mixed_plan_normalizer_workspace_size();
    aliases[3]=(uint8_t *)f->result+pt_mixed_quantized_audit_result_size();
    aliases[4]=f->owner;aliases[5]=&f->inputs;aliases[6]=f->contexts;aliases[7]=&f->project;
    qa_begin(f);
    assert(pt_mixed_quantized_audit_begin_in_workspace(f->workspace,f->capacity,&f->inputs,f->owner)==PT_MIXED_QAUDIT_INVALID);
    assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,&before)==PT_MIXED_QAUDIT_PENDING);
    for(i=0;i<8;++i){
        qa_alias_span(f,aliases[i],sizeof(before),_Alignof(struct pt_mixed_quantized_audit_report));
        qa_alias_span(f,aliases[i],sizeof(struct pt_render_sequence *),_Alignof(struct pt_render_sequence *));
        snapshot=malloc(sizeof(before));assert(snapshot);memcpy(snapshot,aliases[i],sizeof(before));
        assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,aliases[i])==PT_MIXED_QAUDIT_ALIAS);
        assert(!memcmp(snapshot,aliases[i],sizeof(before)));
        assert(pt_mixed_quantized_audit_take(*f->owner,31,f->inputs.generation,aliases[i])==PT_MIXED_QAUDIT_ALIAS);
        assert(!memcmp(snapshot,aliases[i],sizeof(before)));free(snapshot);
        assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,NULL)==PT_MIXED_QAUDIT_PENDING);
        ++refusals;
    }
    snapshot=malloc(f->owner_capacity);assert(snapshot);memcpy(snapshot,f->owner,f->owner_capacity);
    assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,(void *)f->owner)==PT_MIXED_QAUDIT_ALIAS);
    assert(!memcmp(snapshot,f->owner,f->owner_capacity));
    assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,(void *)((uint8_t *)f->owner+1))==PT_MIXED_QAUDIT_INVALID);
    assert(!memcmp(snapshot,f->owner,f->owner_capacity));free(snapshot);
    {struct pt_mixed_quantized_audit *copy=*f->owner;
        assert(pt_mixed_quantized_audit_close(&copy)==PT_MIXED_QAUDIT_ALIAS&&copy==*f->owner);}
    assert(pt_mixed_quantized_audit_step(*f->owner,31,f->inputs.generation,0)==PT_MIXED_QAUDIT_INVALID);
    assert(pt_mixed_quantized_audit_step(*f->owner,31,f->inputs.generation,257)==PT_MIXED_QAUDIT_INVALID);
    qa_drain(f);
    for(i=0;i<5;++i){enum pt_mixed_quantized_audit_result r;
        size_t capacity=f->capacity;f->inputs=original;
        if(i==0)capacity=pt_mixed_quantized_audit_workspace_size()-1;
        else if(i==1)f->inputs.normalizer_capacity=pt_mixed_plan_normalizer_workspace_size()-1;
        else if(i==2)f->inputs.result_capacity=pt_mixed_quantized_audit_result_size()-1;
        else if(i==3)--f->inputs.ordinary_byte_budget;
        else {f->inputs.normalizer_storage=(void *)(uintptr_t)(UINTPTR_MAX-15);f->inputs.normalizer_capacity=64;}
        r=pt_mixed_quantized_audit_begin_in_workspace(f->workspace,capacity,&f->inputs,f->owner);
        assert(r==(i==3?PT_MIXED_QAUDIT_CAPACITY:PT_MIXED_QAUDIT_INVALID)&&!*f->owner&&!f->memory.calls);
    }
    f->inputs=original;
    for(i=0;i<4;++i){struct pt_render_options options=f->options;
        if(i==0)f->options.row_range=1;else if(i==1)f->options.pattern_only=1;
        else if(i==2)f->options.tracks=1;else f->inputs.context_count=31;
        assert(pt_mixed_quantized_audit_begin_in_workspace(f->workspace,f->capacity,&f->inputs,f->owner)==
            (i==3?PT_MIXED_QAUDIT_ALIAS:PT_MIXED_QAUDIT_INVALID));
        assert(!*f->owner&&!f->memory.calls);f->options=options;f->inputs=original;
    }
    qa_begin(f);assert(qa_finish(f,256)==PT_MIXED_QAUDIT_READY);
    assert((*f->owner)->allocations[0].data&&!(*f->owner)->allocations[0].live);
    assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,
        (*f->owner)->allocations[0].data)==PT_MIXED_QAUDIT_ALIAS);
    memcpy(&before,&(*f->owner)->report,sizeof(before));
    assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,(void *)f->owner)==PT_MIXED_QAUDIT_ALIAS);
    assert(!memcmp(&before,&(*f->owner)->report,sizeof(before)));
    assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,aliases[0])==PT_MIXED_QAUDIT_ALIAS);
    assert(pt_mixed_quantized_audit_take(*f->owner,31,f->inputs.generation,aliases[0])==PT_MIXED_QAUDIT_ALIAS);
    assert(!memcmp(master,f->project.samples[0].pcm.data,master_bytes));
    again=qa_save(f,&again_size);assert(again_size==save_size&&!memcmp(saved,again,save_size));
    free(again);free(master);free(saved);assert(refusals==8);qa_drop(f);
    puts("STRICT Q AUDIT GUARDS PASS:whole original owner/result/workspace/context/spare-master guards before mutation, copied-close refusal, five queried capacity/budget/wrap and four scope refusals, READY owner alias and bounded work; SOFTWARE_ONLY");
}
static void qa_cancel_and_expiry(void)
{
    unsigned phase;
    for(phase=PT_MIXED_QAUDIT_SETUP_BEGIN;phase<=PT_MIXED_QAUDIT_FINISHED;++phase){
        struct qa_fixture *f=qa_make(QF_TEMPO_DELAY,0,24,1);unsigned calls=0;
        qa_begin(f);
        while((*f->owner)->report.phase!=(enum pt_mixed_quantized_audit_phase)phase){
            enum pt_mixed_quantized_audit_result r=qa_step(f,31);
            assert((r==PT_MIXED_QAUDIT_PENDING||
                (phase==PT_MIXED_QAUDIT_FINISHED&&r==PT_MIXED_QAUDIT_READY))&&++calls<100000);
        }
        f->project.samples=(void *)(uintptr_t)1;f->project.extensions=(void *)(uintptr_t)1;
        assert(pt_mixed_quantized_audit_cancel(*f->owner)==PT_MIXED_QAUDIT_CANCELLED);
        qa_drain(f);f->project.samples=&f->sample;f->project.extensions=NULL;qa_drop(f);
    }
    {struct qa_fixture *f=qa_make(QF_EMPTY,0,8,0);struct pt_mixed_quantized_audit_report r;
        qa_begin(f);f->project.channels.selected=3;
        assert(pt_mixed_quantized_audit_get(*f->owner,31,f->inputs.generation,&r)==PT_MIXED_QAUDIT_PENDING);
        f->project.samples=(void *)(uintptr_t)1;
        assert(pt_mixed_quantized_audit_step(*f->owner,32,f->inputs.generation,1)==PT_MIXED_QAUDIT_STALE);
        assert(pt_mixed_quantized_audit_get(*f->owner,32,f->inputs.generation,&r)==PT_MIXED_QAUDIT_STALE);
        qa_drain(f);f->project.samples=&f->sample;qa_drop(f);}
    puts("STRICT Q AUDIT CANCEL PASS:all13 construction/local/READY phases cancel and close genuine original Q/renderer children before poisoned former table walks; revision-only stale and permitted selection; SOFTWARE_ONLY");
}
static void qa_callbacks(void)
{
    unsigned request,which;
    for(request=1;request<=2;++request)for(which=0;which<5;++which){
        struct qa_fixture *f=qa_make_spare();void *target;uint8_t *snapshot,*saved,*again;
        size_t bytes=request==1?pt_render_sequence_setup_control_size():pt_render_sequence_control_size();
        size_t alignment=request==1?pt_render_sequence_setup_control_alignment():pt_render_sequence_control_alignment();
        size_t save_size,again_size;
        qa_begin(f);
        if(request==2)while((*f->owner)->report.phase!=PT_MIXED_QAUDIT_SETUP_TAKE)
            assert(qa_step(f,256)==PT_MIXED_QAUDIT_PENDING);
        target=which==0?qa_spare_span(f,bytes,alignment):which==1?(void *)f->owner:
            which==2?(uint8_t *)f->workspace+pt_mixed_quantized_audit_workspace_size():
            which==3?(uint8_t *)f->normalizer+pt_mixed_plan_normalizer_workspace_size():
            (uint8_t *)f->result+pt_mixed_quantized_audit_result_size();
        qa_alias_span(f,target,bytes,alignment);
        snapshot=malloc(bytes);assert(snapshot);memcpy(snapshot,target,bytes);saved=qa_save(f,&save_size);
        f->alias=target;assert(qa_step(f,31)==PT_MIXED_QAUDIT_FAILED);
        assert(!memcmp(snapshot,target,bytes)&&f->memory.releases==0);
        again=qa_save(f,&again_size);assert(again_size==save_size&&!memcmp(saved,again,save_size));
        free(again);free(saved);free(snapshot);qa_drop(f); /* Never release ambiguous alias. */
    }
    for(request=1;request<=2;++request)for(which=1;which<=4;++which){
        struct qa_fixture *f=qa_make(QF_EMPTY,0,8,0);
        f->hook_at=request;f->hook_mode=which;qa_begin(f);
        if(which==4)assert(qa_finish(f,31)==PT_MIXED_QAUDIT_READY);
        else {enum pt_mixed_quantized_audit_result r=qa_finish(f,31);
            assert(r==((which==2||which==3)?PT_MIXED_QAUDIT_STALE:PT_MIXED_QAUDIT_FAILED));
            assert(f->hook_calls==1&&f->memory.releases==1);}
        qa_drain(f);f->project.samples=&f->sample;qa_drop(f);
    }
    /* Outer callback fault after setup consumption still returns a live genuine
     * sequence from setup_take; that identity MUST be retained before post veto. */
    {struct qa_fixture *f=qa_make(QF_EMPTY,0,8,0);qa_begin(f);
        while((*f->owner)->report.phase!=PT_MIXED_QAUDIT_SETUP_TAKE)
            assert(qa_step(f,256)==PT_MIXED_QAUDIT_PENDING);
        f->hook_release=1;f->hook_mode=1;
        assert(qa_step(f,31)==PT_MIXED_QAUDIT_FAILED);
        assert(!(*f->owner)->setup&&(*f->owner)->sequence&&f->memory.live==1&&f->memory.releases==1);
        qa_drop(f);}
    /* Release after transfer retires the exact ledger while owner stays BUSY. */
    {struct qa_fixture *f=qa_make(QF_EMPTY,0,8,0);struct pt_render_sequence *s=NULL;
        qa_begin(f);assert(qa_finish(f,256)==PT_MIXED_QAUDIT_READY);
        assert(pt_mixed_quantized_audit_take(*f->owner,31,f->inputs.generation,&s)==PT_MIXED_QAUDIT_TAKEN);
        f->hook_release=1;f->hook_mode=1;pt_render_sequence_close(s);
        assert(!f->memory.live&&*f->owner&&f->hook_calls==1);qa_drop(f);}
    for(request=1;request<=2;++request){struct qa_fixture *f=qa_make(QF_EMPTY,0,8,0);
        f->fail_at=request;qa_begin(f);assert(qa_finish(f,256)==PT_MIXED_QAUDIT_CAPACITY);
        assert(f->memory.calls==request);qa_drop(f);}
    puts("STRICT Q AUDIT CALLBACK PASS:10 known-alias no-release,8 allocating reentry/stale/read-only groups, genuine sequence retained after setup-release outer fault, transferred BUSY release and two exact-request NULL refusals without retries; SOFTWARE_ONLY");
}
int main(void)
{
    qa_schedule();qa_full16();qa_late_refusal();qa_guards_and_budget();qa_cancel_and_expiry();qa_callbacks();
    puts("STRICT QUANTIZED AUDIT PASS:standalone genuine every-boundary Q traversal; no composite hook/master promotion/cache/key/enqueue/device/native/timing/audio/listening authority");return 0;
}
