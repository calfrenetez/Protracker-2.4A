#include <limits.h>
#include <stddef.h>
#include <string.h>
#include "mixed_quantized_audit.h"
#include "../core/render_storage_internal.h"
#include "../core/document.h"

#define AUDIT_MAGIC UINT64_C(0x50544d5141554431)
#define AUDIT_EXTENSIONS 4090U
/* 5 project/table +510 PCM/slice +4090 payload +5 fixed controls
 * +1 context vector +30 context entries +1 original owner =4642. */
#define AUDIT_GUARDS 4672U
struct allocation {void *data;size_t bytes;unsigned live,transferred;};
struct result_buffer {
    struct pt_mixed_plan_normalizer *normalizer;
    struct pt_mixed_plan_quantized_batch batch;
};
struct pt_mixed_quantized_audit {
    uint64_t magic;size_t capacity;
    struct pt_mixed_quantized_audit **owner;
    const struct pt_mixed_quantized_audit_inputs *inputs;
    struct pt_mixed_quantized_audit_inputs saved;
    struct pt_project header;
    struct pt_sample samples[PT_PROJECT_SAMPLES];
    struct pt_extension extensions[AUDIT_EXTENSIONS];
    struct pt_render_options options;struct pt_paula_render_caps caps;
    struct pt_playback_format format;struct pt_allocator base,wrapped;
    struct pt_mixed_plan_span contexts[PT_MIXED_QAUDIT_CONTEXTS];
    struct pt_mixed_plan_span guards[AUDIT_GUARDS];unsigned guard_count,owner_guard;
    struct allocation allocations[2];unsigned requests,busy,failed,cancelled;
    const void *active;size_t active_bytes;
    size_t setup_bytes,sequence_bytes,setup_alignment,sequence_alignment;
    struct pt_render_sequence_setup *setup;
    struct pt_render_sequence *sequence;
    struct result_buffer *result;
    struct pt_render_plan plan;
    struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS];
    struct pt_mixed_plan_inputs qinputs;
    struct pt_mixed_plan_span qcontexts[PT_MIXED_PLAN_CONTEXTS];
    struct pt_render_interval interval;uint32_t remaining;
    struct pt_mixed_quantized_audit_report report;
};
/* Whole actual local child slots are outside every complete parent context.
 * Numeric guards precede scratch initialization, busy/stale writes and child calls. */
struct scratch {
    struct pt_render_sequence_setup *setup;
    struct pt_render_sequence *sequence;
    struct pt_render_interval interval;
    struct pt_render_plan plan;
    struct pt_render_setup_report setup_report;
    struct pt_mixed_plan_report qreport;
    struct pt_render_setup_guard guards[PT_RENDER_SETUP_GUARDS];
    unsigned ready;
};
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);
}
static int add(size_t a,size_t b,size_t *out)
{
    if(b>SIZE_MAX-a)return 0;
    *out=a+b;return 1;
}
static int shape(const struct pt_project *p)
{
    unsigned i;
    if(!span(p,sizeof(*p))||(uintptr_t)p%_Alignof(struct pt_project)||
       !p->channels.count||p->channels.count>PT_CHANNEL_LIMIT||
       !p->order_count||p->order_count>PT_PROJECT_ORDERS||
       !p->pattern_count||p->pattern_count>PT_PROJECT_PATTERNS||
       p->sample_count>PT_PROJECT_SAMPLES||p->extension_count>AUDIT_EXTENSIONS||
       !span(p->orders,p->order_count*sizeof(*p->orders))||
       (uintptr_t)p->orders%_Alignof(uint16_t)||
       !span(p->events,(size_t)p->pattern_count*64*p->channels.count*sizeof(*p->events))||
       (uintptr_t)p->events%_Alignof(struct pt_event)||
       !span(p->samples,p->sample_count*sizeof(*p->samples))||
       (uintptr_t)p->samples%_Alignof(struct pt_sample)||
       !span(p->extensions,p->extension_count*sizeof(*p->extensions))||
       (uintptr_t)p->extensions%_Alignof(struct pt_extension))return 0;
    for(i=0;i<p->sample_count;++i){const struct pt_sample *s=p->samples+i;
        if(s->pcm.capacity>SIZE_MAX/sizeof(*s->pcm.data)||s->slice_count>PT_PROJECT_SLICES||
           !span(s->pcm.data,s->pcm.capacity*sizeof(*s->pcm.data))||
           (uintptr_t)s->pcm.data%_Alignof(int32_t)||
           !span(s->slices,s->slice_count*sizeof(*s->slices))||
           (uintptr_t)s->slices%_Alignof(uint32_t))return 0;
    }
    for(i=0;i<p->extension_count;++i)
        if(!span(p->extensions[i].data,p->extensions[i].length))return 0;
    return 1;
}
static int initial_apart(const struct pt_mixed_quantized_audit_inputs *v,const void *out,size_t n)
{
    unsigned i;
    if(!span(v,sizeof(*v))||(uintptr_t)v%_Alignof(struct pt_mixed_quantized_audit_inputs)||
       !span(v->options,sizeof(*v->options))||(uintptr_t)v->options%_Alignof(struct pt_render_options)||
       !span(v->caps,sizeof(*v->caps))||(uintptr_t)v->caps%_Alignof(struct pt_paula_render_caps)||
       !span(v->format,sizeof(*v->format))||(uintptr_t)v->format%_Alignof(struct pt_playback_format)||
       !span(v->allocator,sizeof(*v->allocator))||(uintptr_t)v->allocator%_Alignof(struct pt_allocator)||
       v->context_count>PT_MIXED_QAUDIT_CONTEXTS||
       !span(v->contexts,v->context_count*sizeof(*v->contexts))||
       (uintptr_t)v->contexts%_Alignof(struct pt_mixed_plan_span)||!shape(v->project)||
       !apart(v,sizeof(*v),out,n)||!apart(v->options,sizeof(*v->options),out,n)||
       !apart(v->caps,sizeof(*v->caps),out,n)||!apart(v->format,sizeof(*v->format),out,n)||
       !apart(v->allocator,sizeof(*v->allocator),out,n)||
       !apart(v->contexts,v->context_count*sizeof(*v->contexts),out,n)||
       !pt_render_project_storage_output_disjoint(v->project,out,n))return 0;
    for(i=0;i<v->context_count;++i)
        if(!apart(v->contexts[i].data,v->contexts[i].bytes,out,n))return 0;
    return 1;
}
static int valid(const struct pt_mixed_quantized_audit *j)
{
    return span(j,sizeof(*j))&&!( (uintptr_t)j%_Alignof(struct pt_mixed_quantized_audit))&&
        j->magic==AUDIT_MAGIC&&j->capacity>=sizeof(*j)&&span(j,j->capacity)&&
        j->guard_count<=AUDIT_GUARDS&&j->owner_guard<j->guard_count&&
        j->saved.context_count<=PT_MIXED_QAUDIT_CONTEXTS&&j->requests<=2;
}
/* Captured numeric-only guard, including complete retired/unpublished allocations. */
static int output_apart(const struct pt_mixed_quantized_audit *j,const void *out,size_t n)
{
    unsigned i;
    if(!valid(j)||!apart(j,j->capacity,out,n)||
       !apart(j->saved.normalizer_storage,j->saved.normalizer_capacity,out,n)||
       !apart(j->saved.result_storage,j->saved.result_capacity,out,n))return 0;
    for(i=0;i<j->guard_count;++i)
        if(!apart(j->guards[i].data,j->guards[i].bytes,out,n))return 0;
    for(i=0;i<2;++i)
        if(!apart(j->allocations[i].data,j->allocations[i].bytes,out,n))return 0;
    return apart(j->active,j->active_bytes,out,n);
}
static int local_apart(const struct pt_mixed_quantized_audit *j,uintptr_t p,size_t n)
{return output_apart(j,(const void *)p,n);}
static int close_apart(const struct pt_mixed_quantized_audit *j,
    struct pt_mixed_quantized_audit **out)
{
    unsigned i;
    if(!valid(j)||out!=j->owner||j->guards[j->owner_guard].data!=out||
       j->guards[j->owner_guard].bytes!=sizeof(*out)||!apart(j,j->capacity,out,sizeof(*out))||
       !apart(j->saved.normalizer_storage,j->saved.normalizer_capacity,out,sizeof(*out))||
       !apart(j->saved.result_storage,j->saved.result_capacity,out,sizeof(*out))||
       !apart(j->active,j->active_bytes,out,sizeof(*out)))return 0;
    for(i=0;i<j->guard_count;++i)
        if(i!=j->owner_guard&&!apart(j->guards[i].data,j->guards[i].bytes,out,sizeof(*out)))return 0;
    for(i=0;i<2;++i)
        if(!apart(j->allocations[i].data,j->allocations[i].bytes,out,sizeof(*out)))return 0;
    return 1;
}
static void capture(struct pt_mixed_quantized_audit *j,const void *p,size_t n)
{if(n){j->guards[j->guard_count].data=p;j->guards[j->guard_count++].bytes=n;}}
static void sources(struct pt_mixed_quantized_audit *j)
{
    const struct pt_project *p=j->saved.project;unsigned i;
    capture(j,j->inputs,sizeof(*j->inputs));capture(j,p,sizeof(*p));
    capture(j,p->orders,p->order_count*sizeof(*p->orders));
    capture(j,p->events,(size_t)p->pattern_count*64*p->channels.count*sizeof(*p->events));
    capture(j,p->samples,p->sample_count*sizeof(*p->samples));
    capture(j,p->extensions,p->extension_count*sizeof(*p->extensions));
    for(i=0;i<p->sample_count;++i){
        capture(j,p->samples[i].pcm.data,p->samples[i].pcm.capacity*sizeof(int32_t));
        capture(j,p->samples[i].slices,p->samples[i].slice_count*sizeof(uint32_t));
    }
    for(i=0;i<p->extension_count;++i)capture(j,p->extensions[i].data,p->extensions[i].length);
    capture(j,j->saved.options,sizeof(j->options));capture(j,j->saved.caps,sizeof(j->caps));
    capture(j,j->saved.format,sizeof(j->format));capture(j,j->saved.allocator,sizeof(j->base));
    capture(j,j->saved.contexts,j->saved.context_count*sizeof(*j->saved.contexts));
    for(i=0;i<j->saved.context_count;++i)capture(j,j->contexts[i].data,j->contexts[i].bytes);
    j->owner_guard=j->guard_count;capture(j,j->owner,sizeof(*j->owner));
}
static int fixed_current(const struct pt_mixed_quantized_audit *j,uint32_t revision,uint32_t generation)
{
    const struct pt_project *p=j->saved.project;
    size_t k=offsetof(struct pt_project,channels)+offsetof(struct pt_channels,selected);
    return revision==j->saved.revision&&generation==j->saved.generation&&*j->owner==j&&
        !memcmp(j->inputs,&j->saved,sizeof(j->saved))&&p->channels.selected<j->header.channels.count&&
        !memcmp(p,&j->header,k)&&
        !memcmp((const uint8_t *)p+k+sizeof(p->channels.selected),
                (const uint8_t *)&j->header+k+sizeof(p->channels.selected),
                sizeof(*p)-k-sizeof(p->channels.selected))&&
        !memcmp(j->saved.options,&j->options,sizeof(j->options))&&
        !memcmp(j->saved.caps,&j->caps,sizeof(j->caps))&&
        !memcmp(j->saved.format,&j->format,sizeof(j->format))&&
        !memcmp(j->saved.allocator,&j->base,sizeof(j->base))&&
        (!j->saved.context_count||!memcmp(j->saved.contexts,j->contexts,
                j->saved.context_count*sizeof(*j->saved.contexts)));
}
static int current(const struct pt_mixed_quantized_audit *j,uint32_t revision,uint32_t generation)
{
    const struct pt_project *p=j->saved.project;
    if(!fixed_current(j,revision,generation))return 0;
    return (!p->sample_count||!memcmp(p->samples,j->samples,p->sample_count*sizeof(*p->samples)))&&
        (!p->extension_count||!memcmp(p->extensions,j->extensions,p->extension_count*sizeof(*p->extensions)));
}
static enum pt_mixed_quantized_audit_result fault(struct pt_mixed_quantized_audit *j,
    enum pt_mixed_quantized_audit_result r)
{j->failed=1;j->report.result=r;return r;}
static enum pt_mixed_quantized_audit_result enter(struct pt_mixed_quantized_audit *j)
{if(j->busy){fault(j,PT_MIXED_QAUDIT_FAILED);return PT_MIXED_QAUDIT_BUSY;}return j->report.result;}
static enum pt_mixed_quantized_audit_result post(struct pt_mixed_quantized_audit *j)
{
    if(!current(j,j->saved.revision,j->saved.generation))return fault(j,PT_MIXED_QAUDIT_STALE);
    return j->report.result;
}
static int allocation_apart(const struct pt_mixed_quantized_audit *j,const void *p,size_t n)
{return output_apart(j,p,n);}
static void release(void *context,void *p)
{
    struct pt_mixed_quantized_audit *j=context;unsigned i,was_busy;
    if(!valid(j))return;
    for(i=0;i<2;++i)if(j->allocations[i].data==p&&j->allocations[i].live)break;
    if(i==2){fault(j,PT_MIXED_QAUDIT_FAILED);return;}
    was_busy=j->busy;j->busy=1;
    /* Retire before callback. Numeric extent stays guarded, including after take.
     * No consumed child storage/source descriptor read follows this callback. */
    j->allocations[i].live=0;j->allocations[i].transferred=0;
    j->base.release(j->base.context,p);
    j->busy=was_busy;
}
static void *allocate(void *context,size_t n)
{
    struct pt_mixed_quantized_audit *j=context;void *p;unsigned i;
    size_t expected,alignment;
    if(!valid(j)||!j->busy||j->requests>=2){if(valid(j))fault(j,PT_MIXED_QAUDIT_FAILED);return NULL;}
    i=j->requests;expected=i?j->sequence_bytes:j->setup_bytes;
    alignment=i?j->sequence_alignment:j->setup_alignment;
    if(n!=expected){fault(j,PT_MIXED_QAUDIT_FAILED);return NULL;}
    ++j->requests;j->report.allocation_requests=j->requests;
    p=j->base.allocate(j->base.context,n);
    if(!p)return NULL;
    if((uintptr_t)p%alignment||!allocation_apart(j,p,n)){
        /* Known/ambiguous storage never granted ownership. Do not release it. */
        fault(j,PT_MIXED_QAUDIT_FAILED);return NULL;
    }
    j->allocations[i].data=p;j->allocations[i].bytes=n;j->allocations[i].live=1;
    if(j->failed||!current(j,j->saved.revision,j->saved.generation)){
        if(!j->failed)fault(j,PT_MIXED_QAUDIT_STALE);
        release(j,p);return NULL;
    }
    return p;
}
size_t pt_mixed_quantized_audit_workspace_size(void){return sizeof(struct pt_mixed_quantized_audit);}
size_t pt_mixed_quantized_audit_workspace_alignment(void){return _Alignof(struct pt_mixed_quantized_audit);}
size_t pt_mixed_quantized_audit_result_size(void){return sizeof(struct result_buffer);}
size_t pt_mixed_quantized_audit_result_alignment(void){return _Alignof(struct result_buffer);}
static int options_valid(const struct pt_mixed_quantized_audit_inputs *v)
{
    const struct pt_render_options *o=v->options;unsigned i,covered=v->allocator->context==NULL;
    uint32_t mask=(UINT32_C(1)<<v->project->channels.count)-1;
    if(!v->allocator->allocate||!v->allocator->release||!v->ordinary_byte_budget||
       !o->frame_limit||!o->tick_limit||(o->rate!=44100&&o->rate!=48000)||
       (o->bits!=16&&o->bits!=24)||o->tracks!=mask||o->pattern_only||o->row_range||
       o->include_lead_in>1||o->gain_q16>65536||!pt_paula_render_caps_valid(v->caps)||
       (v->format->bits!=8&&v->format->bits!=16)||v->format->channel||
       v->format->word_pad||v->format->little_endian>1)return 0;
    for(i=0;i<v->context_count;++i){uintptr_t x=(uintptr_t)v->contexts[i].data,c=(uintptr_t)v->allocator->context;
        if(v->contexts[i].bytes&&c>=x&&c-x<v->contexts[i].bytes)covered=1;
    }
    return covered;
}
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_begin_in_workspace(
    void *storage,size_t capacity,const struct pt_mixed_quantized_audit_inputs *v,
    struct pt_mixed_quantized_audit **out)
{
    struct pt_mixed_quantized_audit *j=storage;size_t total,i;
    const uint8_t *bytes=storage;
    if(!span(storage,capacity)||capacity<sizeof(*j)||(uintptr_t)storage%_Alignof(struct pt_mixed_quantized_audit)||
       !span(v,sizeof(*v))||(uintptr_t)v%_Alignof(struct pt_mixed_quantized_audit_inputs)||
       !span(out,sizeof(*out))||(uintptr_t)out%_Alignof(struct pt_mixed_quantized_audit *))return PT_MIXED_QAUDIT_INVALID;
    if(j->magic==AUDIT_MAGIC){
        if(!valid(j)||(v!=j->inputs&&!output_apart(j,v,sizeof(*v)))||
           (out==j->owner?!close_apart(j,out):!output_apart(j,out,sizeof(*out))))return PT_MIXED_QAUDIT_ALIAS;
        if(j->busy)return enter(j);
        return PT_MIXED_QAUDIT_INVALID;
    }
    if(!span(v->normalizer_storage,v->normalizer_capacity)||
       v->normalizer_capacity<pt_mixed_plan_normalizer_workspace_size()||
       (uintptr_t)v->normalizer_storage%pt_mixed_plan_normalizer_workspace_alignment()||
       !span(v->result_storage,v->result_capacity)||v->result_capacity<sizeof(struct result_buffer)||
       (uintptr_t)v->result_storage%_Alignof(struct result_buffer))return PT_MIXED_QAUDIT_INVALID;
    if(!initial_apart(v,storage,capacity)||!initial_apart(v,out,sizeof(*out))||
       !initial_apart(v,v->normalizer_storage,v->normalizer_capacity)||
       !initial_apart(v,v->result_storage,v->result_capacity)||
       !apart(storage,capacity,out,sizeof(*out))||
       !apart(storage,capacity,v->normalizer_storage,v->normalizer_capacity)||
       !apart(storage,capacity,v->result_storage,v->result_capacity)||
       !apart(out,sizeof(*out),v->normalizer_storage,v->normalizer_capacity)||
       !apart(out,sizeof(*out),v->result_storage,v->result_capacity)||
       !apart(v->normalizer_storage,v->normalizer_capacity,v->result_storage,v->result_capacity))return PT_MIXED_QAUDIT_ALIAS;
    if(*out||!options_valid(v))return PT_MIXED_QAUDIT_INVALID;
    /* All arithmetic and byte admission precede any owner/result initialization. */
    total=capacity;
    if(!add(total,v->normalizer_capacity,&total)||!add(total,v->result_capacity,&total)||
       !add(total,pt_render_sequence_setup_control_size(),&total)||
       !add(total,pt_render_sequence_control_size(),&total)||total>v->ordinary_byte_budget)return PT_MIXED_QAUDIT_CAPACITY;
    for(i=0;i<sizeof(*j);++i)if(bytes[i])return PT_MIXED_QAUDIT_INVALID;
    bytes=v->result_storage;for(i=0;i<sizeof(struct result_buffer);++i)if(bytes[i])return PT_MIXED_QAUDIT_INVALID;
    bytes=v->normalizer_storage;for(i=0;i<pt_mixed_plan_normalizer_workspace_size();++i)if(bytes[i])return PT_MIXED_QAUDIT_INVALID;
    memset(j,0,sizeof(*j));j->capacity=capacity;j->owner=out;j->inputs=v;
    memcpy(&j->saved,v,sizeof(j->saved));memcpy(&j->header,v->project,sizeof(j->header));
    memcpy(&j->options,v->options,sizeof(j->options));memcpy(&j->caps,v->caps,sizeof(j->caps));
    memcpy(&j->format,v->format,sizeof(j->format));memcpy(&j->base,v->allocator,sizeof(j->base));
    if(v->project->sample_count)memcpy(j->samples,v->project->samples,v->project->sample_count*sizeof(*j->samples));
    if(v->project->extension_count)memcpy(j->extensions,v->project->extensions,v->project->extension_count*sizeof(*j->extensions));
    if(v->context_count)memcpy(j->contexts,v->contexts,v->context_count*sizeof(*j->contexts));
    j->wrapped=(struct pt_allocator){j,allocate,release};j->result=v->result_storage;
    j->setup_bytes=pt_render_sequence_setup_control_size();j->setup_alignment=pt_render_sequence_setup_control_alignment();
    j->sequence_bytes=pt_render_sequence_control_size();j->sequence_alignment=pt_render_sequence_control_alignment();
    sources(j);j->report.result=PT_MIXED_QAUDIT_PENDING;j->report.phase=PT_MIXED_QAUDIT_SETUP_BEGIN;
    j->report.normalizer.action=j->report.normalizer.track=j->report.normalizer.kind=UINT_MAX;
    j->magic=AUDIT_MAGIC;*out=j;return PT_MIXED_QAUDIT_PENDING;
}
/* Genuine original child slots only; exact positive ledger identity, no layout cast. */
static int live_child(const struct pt_mixed_quantized_audit *j,const void *p,size_t n)
{
    unsigned i;for(i=0;i<2;++i)
        if(j->allocations[i].data==p&&j->allocations[i].bytes==n&&j->allocations[i].live)return 1;
    return 0;
}
static void renderer_guards(struct pt_mixed_quantized_audit *j,struct scratch *s)
{
    s->guards[0]=(struct pt_render_setup_guard){j,j->capacity};
    s->guards[1]=(struct pt_render_setup_guard){j->saved.normalizer_storage,j->saved.normalizer_capacity};
    s->guards[2]=(struct pt_render_setup_guard){j->saved.result_storage,j->saved.result_capacity};
    s->guards[3]=(struct pt_render_setup_guard){j->owner,sizeof(*j->owner)};
    /* Descriptor array is disjoint from the publishers inside scratch. The
     * whole scratch is already an active allocation guard; do not list it here. */
}
static void qinputs(struct pt_mixed_quantized_audit *j)
{
    unsigned i,n=0;
    j->qcontexts[n++]=(struct pt_mixed_plan_span){j,j->capacity};
    j->qcontexts[n++]=(struct pt_mixed_plan_span){j->sequence,j->sequence_bytes};
    for(i=0;i<j->saved.context_count;++i)j->qcontexts[n++]=j->contexts[i];
    j->qinputs=(struct pt_mixed_plan_inputs){j->saved.project,&j->plan,j->origins,
        &j->caps,&j->format,j->qcontexts,n,j->options.rate,j->report.frames,
        j->saved.revision,j->saved.generation};
}
static enum pt_mixed_quantized_audit_result qfailure(struct pt_mixed_quantized_audit *j,
    enum pt_mixed_plan_result r,struct scratch *s)
{
    /* Actual normalizer report is numeric-only; scratch already fully admitted. */
    if(j->result->normalizer&&pt_mixed_plan_normalizer_report(j->result->normalizer,&s->qreport)!=PT_MIXED_PLAN_ALIAS)
        j->report.normalizer=s->qreport;
    return fault(j,r==PT_MIXED_PLAN_STALE?PT_MIXED_QAUDIT_STALE:
        r==PT_MIXED_PLAN_REFUSED?PT_MIXED_QAUDIT_REFUSED:PT_MIXED_QAUDIT_FAILED);
}
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_step(
    struct pt_mixed_quantized_audit *j,uint32_t revision,uint32_t generation,unsigned work)
{
    struct scratch s;enum pt_render_setup_result sr;enum pt_render_result rr;
    enum pt_mixed_plan_result qr;enum pt_mixed_quantized_audit_result result;
    unsigned i,k;uint32_t frames;
    if(!valid(j)||!work||work>PT_MIXED_QAUDIT_WORK_MAX)return PT_MIXED_QAUDIT_INVALID;
    if(!local_apart(j,(uintptr_t)&s,sizeof(s)))return PT_MIXED_QAUDIT_ALIAS;
    if(j->busy)return enter(j);
    if(!current(j,revision,generation))return fault(j,PT_MIXED_QAUDIT_STALE);
    if(j->report.result!=PT_MIXED_QAUDIT_PENDING)return j->report.result;
    memset(&s,0,sizeof(s));j->active=&s;j->active_bytes=sizeof(s);j->busy=1;
    j->report.last_work=0;
    switch(j->report.phase){
    case PT_MIXED_QAUDIT_SETUP_BEGIN:
        renderer_guards(j,&s);
        sr=pt_render_sequence_setup_begin(j->saved.project,j->saved.options,&j->wrapped,
            revision,generation,s.guards,4,&s.setup);
        /* Adopt actual positive child identity BEFORE any outer stale/fault veto. */
        if(s.setup&&live_child(j,s.setup,j->setup_bytes))j->setup=s.setup;
        j->report.setup_result=sr;j->report.last_work=1;
        if(sr==PT_RENDER_SETUP_PENDING&&j->setup)j->report.phase=PT_MIXED_QAUDIT_SETUP_STEP;
        else if(!j->failed)fault(j,sr==PT_RENDER_SETUP_CAPACITY?PT_MIXED_QAUDIT_CAPACITY:PT_MIXED_QAUDIT_FAILED);
        break;
    case PT_MIXED_QAUDIT_SETUP_STEP:
        sr=pt_render_sequence_setup_step(j->setup,revision,generation,work);j->report.setup_result=sr;
        if(sr==PT_RENDER_SETUP_PENDING||sr==PT_RENDER_SETUP_READY){
            if(pt_render_sequence_setup_get(j->setup,revision,generation,&s.setup_report)==sr)
                j->report.last_work=s.setup_report.last_work;
            if(sr==PT_RENDER_SETUP_READY)j->report.phase=PT_MIXED_QAUDIT_SETUP_TAKE;
        }else fault(j,sr==PT_RENDER_SETUP_STALE?PT_MIXED_QAUDIT_STALE:PT_MIXED_QAUDIT_FAILED);
        break;
    case PT_MIXED_QAUDIT_SETUP_TAKE:
        renderer_guards(j,&s);s.setup=j->setup;
        sr=pt_render_sequence_setup_take(&s.setup,revision,generation,s.guards,4,&s.sequence);
        /* Local NULL is actual consumed setup even if outer callbacks faulted.
         * A genuine returned sequence remains retained for explicit cleanup. */
        if(!s.setup)j->setup=NULL;
        if(s.sequence&&live_child(j,s.sequence,j->sequence_bytes))j->sequence=s.sequence;
        j->report.setup_result=sr;j->report.last_work=1;
        if(sr==PT_RENDER_SETUP_READY&&j->sequence)j->report.phase=PT_MIXED_QAUDIT_MEASURE;
        else if(!j->failed)fault(j,sr==PT_RENDER_SETUP_STALE?PT_MIXED_QAUDIT_STALE:
            sr==PT_RENDER_SETUP_CAPACITY?PT_MIXED_QAUDIT_CAPACITY:PT_MIXED_QAUDIT_FAILED);
        break;
    case PT_MIXED_QAUDIT_MEASURE:
        rr=pt_render_sequence_prepare(j->sequence,work,&s.ready);j->report.render_result=rr;
        j->report.last_work=work;
        if(rr!=PT_RENDER_OK)fault(j,PT_MIXED_QAUDIT_FAILED);
        else if(s.ready)j->report.phase=PT_MIXED_QAUDIT_NEXT;
        break;
    case PT_MIXED_QAUDIT_NEXT:
        rr=pt_render_sequence_next(j->sequence,&s.interval);j->report.render_result=rr;
        j->report.last_work=1;
        if(rr!=PT_RENDER_OK)fault(j,PT_MIXED_QAUDIT_FAILED);
        else if(j->report.intervals==UINT64_MAX)fault(j,PT_MIXED_QAUDIT_REFUSED);
        else {j->interval=s.interval;j->remaining=s.interval.frames;++j->report.intervals;
            j->report.phase=j->remaining?PT_MIXED_QAUDIT_CONSUME:PT_MIXED_QAUDIT_COMPLETE;}
        break;
    case PT_MIXED_QAUDIT_CONSUME:
        frames=j->remaining>work?work:j->remaining;
        if(!frames||frames>=UINT64_MAX-j->report.frames){fault(j,PT_MIXED_QAUDIT_REFUSED);break;}
        rr=pt_render_sequence_consume(j->sequence,frames);j->report.render_result=rr;
        if(rr!=PT_RENDER_OK)fault(j,PT_MIXED_QAUDIT_FAILED);
        else {j->remaining-=frames;j->report.frames+=frames;j->report.last_work=frames;
            if(!j->remaining)j->report.phase=PT_MIXED_QAUDIT_COMPLETE;}
        break;
    case PT_MIXED_QAUDIT_COMPLETE:
        rr=pt_render_sequence_complete(j->sequence,&s.plan);j->report.render_result=rr;
        j->report.last_work=1;
        if(rr!=PT_RENDER_OK)fault(j,PT_MIXED_QAUDIT_FAILED);
        else {j->plan=s.plan;j->report.phase=PT_MIXED_QAUDIT_Q_BEGIN;}
        break;
    case PT_MIXED_QAUDIT_Q_BEGIN:
        qinputs(j);
        qr=pt_mixed_plan_normalizer_begin_quantized_in_workspace(j->saved.normalizer_storage,
            j->saved.normalizer_capacity,&j->qinputs,&j->result->normalizer);
        j->report.last_work=1;
        if(qr==PT_MIXED_PLAN_PENDING)j->report.phase=PT_MIXED_QAUDIT_Q_STEP;
        else qfailure(j,qr,&s);
        break;
    case PT_MIXED_QAUDIT_Q_STEP:
        qr=pt_mixed_plan_normalizer_step(j->result->normalizer,revision,generation,work);
        if(pt_mixed_plan_normalizer_report(j->result->normalizer,&s.qreport)!=PT_MIXED_PLAN_ALIAS){
            j->report.normalizer=s.qreport;j->report.last_work=s.qreport.last_work;}
        if(qr==PT_MIXED_PLAN_READY)j->report.phase=PT_MIXED_QAUDIT_Q_GET;
        else if(qr!=PT_MIXED_PLAN_PENDING)qfailure(j,qr,&s);
        break;
    case PT_MIXED_QAUDIT_Q_GET:
        qr=pt_mixed_plan_normalizer_get_quantized(j->result->normalizer,revision,generation,&j->result->batch);
        j->report.last_work=1;
        if(qr==PT_MIXED_PLAN_READY)j->report.phase=PT_MIXED_QAUDIT_Q_CLOSE;
        else qfailure(j,qr,&s);
        break;
    case PT_MIXED_QAUDIT_Q_CLOSE:
        j->report.last_work=1;
        if(!pt_mixed_plan_normalizer_close(&j->result->normalizer))fault(j,PT_MIXED_QAUDIT_FAILED);
        else j->report.phase=PT_MIXED_QAUDIT_COMMIT;
        break;
    case PT_MIXED_QAUDIT_COMMIT:
        for(i=0;i<PT_MIXED_PLAN_RECORDS;++i)j->origins[i]=j->result->batch.normalized.next[i];
        for(k=0;k<2;++k)for(i=0;i<PT_PROJECT_SAMPLES;++i)
            j->report.samples[k][i]|=j->result->batch.normalized.samples[k][i];
        j->report.last_work=1;
        if(j->interval.end){j->report.result=PT_MIXED_QAUDIT_READY;j->report.phase=PT_MIXED_QAUDIT_FINISHED;}
        else j->report.phase=PT_MIXED_QAUDIT_NEXT;
        break;
    default:fault(j,PT_MIXED_QAUDIT_FAILED);break;
    }
    if(j->report.last_work>UINT64_MAX-j->report.work)fault(j,PT_MIXED_QAUDIT_REFUSED);
    else j->report.work+=j->report.last_work;
    result=post(j);j->busy=0;j->active=NULL;j->active_bytes=0;return result;
}
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_get(
    struct pt_mixed_quantized_audit *j,uint32_t revision,uint32_t generation,
    struct pt_mixed_quantized_audit_report *out)
{
    if(!valid(j)|| (out&&(!span(out,sizeof(*out))||
       (uintptr_t)out%_Alignof(struct pt_mixed_quantized_audit_report))))return PT_MIXED_QAUDIT_INVALID;
    if(out&&!output_apart(j,out,sizeof(*out)))return PT_MIXED_QAUDIT_ALIAS;
    if(j->busy)return enter(j);
    /* Fixed-first source currentness avoids expired table reads. A stale report
     * publishes only captured numeric diagnostics, with zero masks. */
    if(!current(j,revision,generation))fault(j,PT_MIXED_QAUDIT_STALE);
    if(out){*out=j->report;if(j->report.result!=PT_MIXED_QAUDIT_READY)memset(out->samples,0,sizeof(out->samples));}
    return j->report.result;
}
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_take(
    struct pt_mixed_quantized_audit *j,uint32_t revision,uint32_t generation,struct pt_render_sequence **out)
{
    enum pt_render_result r;unsigned i;
    if(!valid(j)||!span(out,sizeof(*out))||(uintptr_t)out%_Alignof(struct pt_render_sequence *))return PT_MIXED_QAUDIT_INVALID;
    if(!output_apart(j,out,sizeof(*out)))return PT_MIXED_QAUDIT_ALIAS;
    if(*out)return PT_MIXED_QAUDIT_INVALID;
    if(j->busy)return enter(j);
    if(!current(j,revision,generation))return fault(j,PT_MIXED_QAUDIT_STALE);
    if(j->report.result!=PT_MIXED_QAUDIT_READY||!j->sequence||j->report.rewinds)return PT_MIXED_QAUDIT_INVALID;
    for(i=0;i<2;++i)if(j->allocations[i].data==j->sequence&&j->allocations[i].live)break;
    if(i==2)return fault(j,PT_MIXED_QAUDIT_FAILED);
    j->busy=1;r=pt_render_sequence_rewind(j->sequence);j->report.render_result=r;
    if(r!=PT_RENDER_OK){j->busy=0;return fault(j,PT_MIXED_QAUDIT_FAILED);}
    ++j->report.rewinds;
    if(post(j)!=PT_MIXED_QAUDIT_READY||!output_apart(j,out,sizeof(*out))){j->busy=0;return j->report.result;}
    *out=j->sequence;j->sequence=NULL;j->allocations[i].transferred=1;
    j->report.result=PT_MIXED_QAUDIT_TAKEN;j->busy=0;return PT_MIXED_QAUDIT_TAKEN;
}
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_cancel(struct pt_mixed_quantized_audit *j)
{
    if(!valid(j))return PT_MIXED_QAUDIT_INVALID;
    if(j->busy)return enter(j);
    j->cancelled=1;j->report.result=PT_MIXED_QAUDIT_CANCELLED;return j->report.result;
}
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_close(struct pt_mixed_quantized_audit **owner)
{
    struct pt_mixed_quantized_audit *j;struct scratch s;unsigned i;
    enum pt_render_setup_result sr;enum pt_mixed_quantized_audit_result result;
    if(!span(owner,sizeof(*owner))||(uintptr_t)owner%_Alignof(struct pt_mixed_quantized_audit *))return PT_MIXED_QAUDIT_INVALID;
    if(!*owner)return PT_MIXED_QAUDIT_CLOSED;
    j=*owner;if(!valid(j))return PT_MIXED_QAUDIT_INVALID;
    if(!close_apart(j,owner)||!local_apart(j,(uintptr_t)&s,sizeof(s)))return PT_MIXED_QAUDIT_ALIAS;
    if(j->busy)return enter(j);
    for(i=0;i<2;++i)if(j->allocations[i].live&&j->allocations[i].transferred)return PT_MIXED_QAUDIT_PENDING;
    memset(&s,0,sizeof(s));j->active=&s;j->active_bytes=sizeof(s);j->busy=1;
    if(j->result->normalizer&&!pt_mixed_plan_normalizer_close(&j->result->normalizer)){
        j->busy=0;j->active=NULL;j->active_bytes=0;return fault(j,PT_MIXED_QAUDIT_FAILED);}
    if(j->setup){s.setup=j->setup;sr=pt_render_sequence_setup_cancel(&s.setup);
        if(!s.setup)j->setup=NULL;
        if(sr!=PT_RENDER_SETUP_READY){j->report.setup_result=sr;fault(j,PT_MIXED_QAUDIT_FAILED);}
    }
    if(j->sequence){s.sequence=j->sequence;j->sequence=NULL;pt_render_sequence_close(s.sequence);}
    j->busy=0;j->active=NULL;j->active_bytes=0;
    for(i=0;i<2;++i)if(j->allocations[i].live)return fault(j,PT_MIXED_QAUDIT_FAILED);
    if(j->setup||j->result->normalizer)return fault(j,PT_MIXED_QAUDIT_FAILED);
    /* Callback failure can accompany actual complete consumption. Retain this
     * empty owner for one later explicit numeric close; no child release retry. */
    if(j->failed&&j->report.result!=PT_MIXED_QAUDIT_CANCELLED){
        result=j->report.result;j->failed=0;j->report.result=PT_MIXED_QAUDIT_CANCELLED;return result;}
    if(*owner!=j||!close_apart(j,owner))return fault(j,PT_MIXED_QAUDIT_FAILED);
    memset(j->result,0,sizeof(*j->result));memset(j,0,sizeof(*j));*owner=NULL;
    return PT_MIXED_QAUDIT_CLOSED;
}
