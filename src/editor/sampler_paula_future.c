#include "sampler_paula_future.h"
#include "sampler_internal.h"
#include "sampler_paula_internal.h"
#include "project_snapshot.h"
#include <string.h>
#include <limits.h>
struct source {
    struct pt_paula_future_request request;struct pt_sample sample;
    struct pt_sample_version *pin;struct pt_pcm pcm;struct pt_cache_lease lease;
    const uint8_t *data;size_t bytes;unsigned leased;
    struct pt_paula_future_owner *previous;unsigned previous_source;
};
struct pt_paula_future_owner {
    struct pt_paula_future_pool *pool;struct pt_scheduled_output *queue;
    uint64_t token,frame,ticket;unsigned count,index,phase,state,borrowers,released;
    struct source source[PT_PAULA_FUTURE_SOURCES];
    struct pt_sampler_pin_job pin_job;struct pt_sampler_paula_job chip_job;
    struct pt_scheduled_batch batch;
    struct pt_scheduled_span spans[PT_SCHEDULED_SPANS];unsigned span_count;
};
struct pt_paula_future_pool {
    struct pt_allocator allocator;struct pt_sampler *sampler;struct pt_project *project,header;
    struct pt_paula_future_config config;struct pt_sampler_paula bridge;
    struct pt_sample original[PT_PROJECT_SAMPLES];uint8_t original_external[PT_PROJECT_SAMPLES];int8_t map[PT_CHANNEL_LIMIT];
    struct pt_paula_future_owner *owners[PT_PAULA_FUTURE_OWNERS],*callback;
    const struct pt_scheduled_batch *callback_batch;
    size_t bytes;uint64_t serial;unsigned generation,busy,closing;
};
enum {PREPARING=1,READY,LIVE,RETIRED,CANCELLED,FAILED};
enum {PIN_BEGIN,PIN_STEP,CHIP_BEGIN,CHIP_STEP};
static int valid_span(const void *p,size_t n)
{return !n || (p && (uintptr_t)p<=UINTPTR_MAX-n);}
static int disjoint(const void *a,size_t n,const void *b,size_t m)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return valid_span(a,n) && valid_span(b,m) && (!n || !m || (x<=y?n<=y-x:m<=x-y));
}
static int sample_disjoint(const struct pt_sample *s,const void *out,size_t bytes)
{
    return s->pcm.capacity<=SIZE_MAX/sizeof(int32_t) &&
        disjoint(out,bytes,s->pcm.data,s->pcm.capacity*sizeof(int32_t)) &&
        disjoint(out,bytes,s->slices,(size_t)s->slice_count*sizeof(uint32_t));
}
static int project_fixed_disjoint(const struct pt_project *p,const void *out,size_t bytes)
{
    size_t events;unsigned i;
    if(!p || p->sample_count>PT_PROJECT_SAMPLES || p->pattern_count>PT_PROJECT_PATTERNS ||
       p->order_count>PT_PROJECT_ORDERS || p->channels.count>PT_CHANNEL_LIMIT ||
       !disjoint(out,bytes,p,sizeof(*p)) ||
       !disjoint(out,bytes,p->samples,(size_t)p->sample_count*sizeof(*p->samples)) ||
       !disjoint(out,bytes,p->orders,(size_t)p->order_count*sizeof(*p->orders)) ||
       !disjoint(out,bytes,p->extensions,(size_t)p->extension_count*sizeof(*p->extensions)))return 0;
    events=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
    if(!disjoint(out,bytes,p->events,events*sizeof(*p->events)))return 0;
    for(i=0;i<p->extension_count;++i)if(!disjoint(out,bytes,p->extensions[i].data,p->extensions[i].length))return 0;
    return 1;
}
static int project_disjoint(const struct pt_project *p,const void *out,size_t bytes)
{
    unsigned i;if(!project_fixed_disjoint(p,out,bytes))return 0;
    for(i=0;i<p->sample_count;++i)if(!sample_disjoint(p->samples+i,out,bytes))return 0;
    return 1;
}
static int registered(struct pt_paula_future_pool *p,const struct pt_paula_future_owner *o)
{
    unsigned i;for(i=0;i<p->config.maximum_owners;++i)if(p->owners[i]==o)return 1;
    return 0;
}
static int current_pool(struct pt_paula_future_pool *p)
{
    struct pt_project header;
    if(p->closing || p->sampler->generation!=p->generation ||
       !pt_sampler_paula_prepared_current(&p->bridge))return 0;
    memcpy(&header,&p->header,sizeof(header));header.channels.selected=p->project->channels.selected;
    return pt_project_snapshot_equal(p->project,&header);
}
/* Captured original spans are used even on stale calls: never traverse a changed
 * project table or released former owner. Borrowed originals live through close. */
static int output_disjoint(struct pt_paula_future_pool *p,const void *out,size_t bytes)
{
    unsigned i,j;
    if(!valid_span(out,bytes) || !bytes || !disjoint(out,bytes,p,sizeof(*p)) ||
       !disjoint(out,bytes,p->project,sizeof(*p->project)) ||
       !project_fixed_disjoint(&p->header,out,bytes) || !pt_sampler_output_disjoint(p->sampler,out,bytes))return 0;
    if(current_pool(p) && !project_disjoint(p->project,out,bytes))return 0;
    for(i=0;i<p->header.sample_count;++i)if(p->original_external[i] && !sample_disjoint(p->original+i,out,bytes))return 0;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(!disjoint(out,bytes,p->bridge.cache.entry[i].data,p->bridge.cache.entry[i].bytes))return 0;
    for(i=0;i<p->config.maximum_owners;++i)if(p->owners[i]) {
        struct pt_paula_future_owner *o=p->owners[i];
        if(!disjoint(out,bytes,o,sizeof(*o)) || !pt_sampler_pin_job_output_disjoint(&o->pin_job,out,bytes) ||
           !pt_sampler_version_output_disjoint(o->chip_job.pin,out,bytes))return 0;
        for(j=0;j<o->count;++j)if(!pt_sampler_version_output_disjoint(o->source[j].pin,out,bytes))return 0;
    }
    return 1;
}
static int source_current(struct pt_paula_future_owner *o,unsigned i)
{
    struct source *s=o->source+i;struct pt_pcm pcm;struct pt_sample_version *pin;
    const uint8_t *data;size_t bytes;struct pt_paula_future_pool *p=o->pool;
    if(!s->pin || !s->leased || pt_sampler_pin_current(p->sampler,p->project,s->request.sample,
        p->generation,s->pin,&pcm,&pin)!=PT_EDIT_OK)return 0;
    pt_sampler_unpin(pin);
    return pt_sampler_paula_prepared_location_validated(&p->bridge,s->request.track,s->lease,&data,&bytes) &&
        data==s->data && bytes==s->bytes;
}
static int batch_valid(struct pt_paula_future_owner *o,const struct pt_scheduled_batch *b)
{
    unsigned i,j,seen=0;struct pt_paula_future_pool *p=o->pool;
    if(!b || b->generation!=p->config.generation || b->frame!=o->frame || b->count!=o->count)return 0;
    for(i=0;i<b->count;++i) {
        const struct pt_scheduled_action *a=b->action+i;struct source *s;
        if(a->slot>=4 || (seen&(1U<<a->slot)) || a->volume>64)return 0;
        seen|=1U<<a->slot;
        for(j=0;j<o->count;++j)if(p->map[o->source[j].request.track]==(int)a->slot)break;
        if(j==o->count)return 0;
        s=o->source+j;
        if(a->kind==PT_SCHEDULED_TRIGGER) {
            uintptr_t x=(uintptr_t)a->data,y=(uintptr_t)s->data;size_t n=(size_t)a->words*2;
            if(!a->words || !a->period || (x&1) || x<y || x-y>s->bytes || n>s->bytes-(x-y))return 0;
        }else return 0; /* Active-reader CONTROL/STOP lineage is not implemented. */
    }
    return 1;
}
static void resources(struct pt_paula_future_owner *o);
static void drop_borrow(struct source *s)
{
    struct pt_paula_future_owner *previous=s->previous;
    if(!previous)return;
    s->previous=NULL;--previous->borrowers;
    if(previous->state==RETIRED && !previous->borrowers)resources(previous);
}
static void resources(struct pt_paula_future_owner *o)
{
    unsigned i;struct pt_paula_future_pool *p=o->pool;
    if(o->released)return;
    pt_sampler_paula_job_cancel(&o->chip_job);pt_sampler_pin_job_cancel(&o->pin_job);
    for(i=0;i<o->count;++i) {
        struct source *s=o->source+i;
        if(s->leased)pt_sampler_paula_unpin(&p->bridge,s->lease);
        pt_sampler_unpin(s->pin);s->pin=NULL;s->leased=0;s->data=NULL;s->bytes=0;drop_borrow(s);
        memset(&s->sample,0,sizeof(s->sample));
    }
    o->released=1;
}
static int owner_current(void *context,uint64_t token,uint64_t generation)
{
    struct pt_paula_future_owner *o=context;struct pt_paula_future_pool *p=o->pool;unsigned i;
    if((p->busy && p->callback!=o) || !registered(p,o) || token!=o->token ||
       generation!=p->config.generation || (o->state!=READY && o->state!=LIVE) || !current_pool(p))return 0;
    for(i=0;i<o->count;++i)if(!source_current(o,i))return 0;
    return batch_valid(o,p->callback==o?p->callback_batch:&o->batch);
}
static void owner_retired(void *context,uint64_t token)
{
    struct pt_paula_future_owner *o=context;struct pt_paula_future_pool *p=o->pool;
    if(!registered(p,o) || token!=o->token || o->state!=LIVE)return;
    /* A forbidden callback reentry cannot discard a confirmed retirement or
     * release storage being borrowed by preparation. Defer until close/drop. */
    o->state=RETIRED;if(p->busy)return;
    p->busy=1;if(!o->borrowers)resources(o);p->busy=0;
}
static int add_span(struct pt_paula_future_owner *o,const void *data,size_t bytes)
{
    if(!bytes)return 1;
    if(!valid_span(data,bytes) || o->span_count==PT_SCHEDULED_SPANS)return 0;
    o->spans[o->span_count++]=(struct pt_scheduled_span){data,bytes};return 1;
}
static int owner_spans(struct pt_paula_future_owner *o)
{
    struct pt_paula_future_pool *p=o->pool;struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];
    unsigned i,j,n;o->span_count=0;
    if(!add_span(o,o,sizeof(*o)) || !add_span(o,p,sizeof(*p)) || !add_span(o,p->sampler,sizeof(*p->sampler)) ||
       !add_span(o,p->project,sizeof(*p->project)) ||
       !add_span(o,p->sampler->table,p->sampler->table_bytes) ||
       !add_span(o,p->header.samples,(size_t)p->header.sample_count*sizeof(*p->header.samples)))return 0;
    for(i=0;i<o->count;++i) {
        struct source *s=o->source+i;
        if(!pt_sampler_version_spans(s->pin,spans,PT_SAMPLER_VERSION_SPANS,&n))return 0;
        for(j=0;j<n;++j)if(!add_span(o,spans[j].data,spans[j].bytes))return 0;
        if(!add_span(o,s->data,s->bytes))return 0;
        if(p->original_external[s->request.sample] &&
           (!add_span(o,p->original[s->request.sample].pcm.data,p->original[s->request.sample].pcm.capacity*sizeof(int32_t)) ||
            !add_span(o,p->original[s->request.sample].slices,(size_t)p->original[s->request.sample].slice_count*sizeof(uint32_t))))return 0;
    }
    return 1;
}
enum pt_paula_future_result pt_paula_future_open(const struct pt_allocator *a,
    struct pt_sampler *sampler,struct pt_project *project,const struct pt_paula_future_config *c,
    struct pt_paula_future_pool **out)
{
    struct pt_paula_future_pool *p;unsigned i;
    if(!a || !a->allocate || !a->release || !sampler || !c || !out || !c->chip_allocate ||
       !c->chip_release || !c->generation || !c->maximum_owners || c->maximum_owners>PT_PAULA_FUTURE_OWNERS ||
       pt_project_validate(project,NULL)!=PT_PROJECT_OK)return PT_FUTURE_INVALID;
    if(!project_disjoint(project,out,sizeof(*out)) || !pt_sampler_output_disjoint(sampler,out,sizeof(*out)) ||
       !disjoint(out,sizeof(*out),a,sizeof(*a)) || !disjoint(out,sizeof(*out),c,sizeof(*c)))return PT_FUTURE_INVALID;
    if(c->control_budget<sizeof(*p))return PT_FUTURE_CAPACITY;
    p=a->allocate(a->context,sizeof(*p));if(!p)return PT_FUTURE_CAPACITY;
    if(!project_disjoint(project,p,sizeof(*p)) || !pt_sampler_output_disjoint(sampler,p,sizeof(*p)) ||
       !disjoint(p,sizeof(*p),out,sizeof(*out)) || !disjoint(p,sizeof(*p),a,sizeof(*a)) || !disjoint(p,sizeof(*p),c,sizeof(*c))) {
        a->release(a->context,p);return PT_FUTURE_INVALID;
    }
    memset(p,0,sizeof(*p));p->allocator=*a;p->config=*c;p->sampler=sampler;p->project=project;
    memcpy(&p->header,project,sizeof(p->header));memcpy(p->original,project->samples,project->sample_count*sizeof(*project->samples));
    for(i=0;i<project->sample_count;++i)p->original_external[i]=sampler->current[i]==NULL;
    p->generation=sampler->generation;p->bytes=sizeof(*p);
    if(pt_channels_paula_map(&project->channels,NULL,p->map)!=PT_CHANNEL_OK ||
       !pt_sampler_paula_bind(&p->bridge,sampler,project,c->chip_context,c->chip_allocate,c->chip_release,c->chip_budget)) {
        a->release(a->context,p);return PT_FUTURE_INVALID;
    }
    *out=p;return PT_FUTURE_OK;
}
static int same_sample(const struct pt_sample *a,const struct pt_sample *b)
{
    return !memcmp(a->name,b->name,sizeof(a->name)) && a->pcm.data==b->pcm.data &&
        a->pcm.capacity==b->pcm.capacity && a->pcm.frames==b->pcm.frames && a->pcm.rate==b->pcm.rate &&
        a->pcm.channels==b->pcm.channels && a->pcm.bits==b->pcm.bits && a->slices==b->slices &&
        a->slice_count==b->slice_count && a->loop_start==b->loop_start && a->loop_end==b->loop_end &&
        a->crossfade==b->crossfade && a->loop==b->loop && a->volume==b->volume &&
        a->interpolation==b->interpolation && a->finetune==b->finetune;
}
static int prepared_source_current(struct pt_paula_future_owner *o,struct source *s)
{
    struct pt_paula_future_pool *p=o->pool;unsigned i,j;
    if(same_sample(&s->sample,p->project->samples+s->request.sample))return 1;
    /* A prior request may have atomically promoted this same original source.
     * Accept only a registered independently held current version of the exact
     * captured descriptor; never infer content identity or scan PCM here. */
    for(i=0;i<p->config.maximum_owners;++i)if(p->owners[i])
        for(j=0;j<p->owners[i]->count;++j) {
            struct source *held=p->owners[i]->source+j;struct pt_pcm pcm;struct pt_sample_version *pin;
            if(held->pin && held->request.sample==s->request.sample && same_sample(&held->sample,&s->sample) &&
               pt_sampler_pin_current(p->sampler,p->project,s->request.sample,p->generation,held->pin,&pcm,&pin)==PT_EDIT_OK) {
                pt_sampler_unpin(pin);return 1;
            }
        }
    return 0;
}
static int request_valid(struct pt_paula_future_pool *p,struct pt_scheduled_output *q,uint64_t frame,
    const struct pt_paula_future_request *r,unsigned n)
{
    unsigned i,j,seen=0;
    if(!q || frame==UINT64_MAX || !r || !n || n>PT_PAULA_FUTURE_SOURCES)return 0;
    for(i=0;i<n;++i) {
        const struct pt_paula_future_request *x=r+i;struct pt_paula_future_owner *v=x->previous;struct source *s;
        if(x->track>=p->header.channels.count || p->map[x->track]<0 ||
           x->sample>=p->header.sample_count || x->channel>=p->project->samples[x->sample].pcm.channels ||
           (seen&(1U<<p->map[x->track])))return 0;
        seen|=1U<<p->map[x->track];
        if(!v) {if(x->token || x->source)return 0;continue;}
        if(!registered(p,v) || v->state!=LIVE || v->token!=x->token || v->queue!=q || v->frame>=frame ||
           x->source>=v->count || v->borrowers==UINT_MAX)return 0;
        s=v->source+x->source;
        if(s->request.track!=x->track || s->request.sample!=x->sample || s->request.channel!=x->channel || !source_current(v,x->source))return 0;
        for(j=0;j<v->batch.count;++j)if(v->batch.action[j].slot==(unsigned)p->map[x->track])break;
        if(j==v->batch.count || v->batch.action[j].kind==PT_SCHEDULED_STOP)return 0;
    }
    return 1;
}
enum pt_paula_future_result pt_paula_future_begin(struct pt_paula_future_pool *p,
    struct pt_scheduled_output *q,uint64_t frame,const struct pt_paula_future_request *r,unsigned n,
    struct pt_paula_future_owner **out)
{
    struct pt_paula_future_owner *o;unsigned i,k;
    if(!p || p->busy || p->closing || !n || n>PT_PAULA_FUTURE_SOURCES || !r || !out || !output_disjoint(p,out,sizeof(*out)) ||
       !output_disjoint(p,r,(size_t)n*sizeof(*r)) ||
       !disjoint(out,sizeof(*out),r,(size_t)n*sizeof(*r)))return PT_FUTURE_INVALID;
    if(!current_pool(p))return PT_FUTURE_STALE;
    if(!request_valid(p,q,frame,r,n))return PT_FUTURE_INVALID;
    for(k=0;k<p->config.maximum_owners;++k)if(!p->owners[k])break;
    if(k==p->config.maximum_owners || p->serial==UINT64_MAX || p->bytes>p->config.control_budget ||
       sizeof(*o)>p->config.control_budget-p->bytes)return PT_FUTURE_CAPACITY;
    p->busy=1;o=p->allocator.allocate(p->allocator.context,sizeof(*o));
    if(!o){p->busy=0;return PT_FUTURE_CAPACITY;}
    if(!current_pool(p) || !request_valid(p,q,frame,r,n) ||
       !output_disjoint(p,o,sizeof(*o)) || !disjoint(o,sizeof(*o),out,sizeof(*out)) ||
       !disjoint(o,sizeof(*o),r,n*sizeof(*r))) {
        p->allocator.release(p->allocator.context,o);p->busy=0;return PT_FUTURE_INVALID;
    }
    memset(o,0,sizeof(*o));o->pool=p;o->queue=q;o->frame=frame;o->token=++p->serial;o->count=n;o->state=PREPARING;
    for(i=0;i<n;++i) {
        struct source *s=o->source+i;s->request=r[i];s->sample=p->project->samples[r[i].sample];
        if(r[i].previous) {s->previous=r[i].previous;s->previous_source=r[i].source;++s->previous->borrowers;}
    }
    p->owners[k]=o;p->bytes+=sizeof(*o);p->busy=0;*out=o;return PT_FUTURE_OK;
}
static enum pt_paula_future_result fail(struct pt_paula_future_owner *o,enum pt_paula_future_result r)
{resources(o);o->state=FAILED;return r;}
enum pt_paula_future_result pt_paula_future_step(struct pt_paula_future_owner *o,unsigned *ready)
{
    struct pt_paula_future_pool *p;struct source *s;enum pt_edit_result er;enum pt_cache_result cr;unsigned done,i;
    enum pt_paula_future_result result=PT_FUTURE_PENDING;
    if(!o || !(p=o->pool) || p->busy || !registered(p,o) || !ready || !output_disjoint(p,ready,sizeof(*ready)))return PT_FUTURE_INVALID;
    if(o->state!=PREPARING && o->state!=READY)return PT_FUTURE_INVALID;
    p->busy=1;
    if(!current_pool(p)){result=fail(o,PT_FUTURE_STALE);goto end;}
    if(o->state==READY) {
        for(i=0;i<o->count;++i)if(!source_current(o,i)){result=fail(o,PT_FUTURE_STALE);goto end;}
        *ready=1;result=PT_FUTURE_OK;goto end;
    }
    s=o->source+o->index;
    if(o->phase==PIN_BEGIN) {
        if(!prepared_source_current(o,s)) {
            result=fail(o,PT_FUTURE_STALE);goto end;
        }
        if(s->previous) {
            struct source *from=s->previous->source+s->previous_source;
            er=pt_sampler_pin_current(p->sampler,p->project,s->request.sample,p->generation,from->pin,&s->pcm,&s->pin);
            if(er!=PT_EDIT_OK){result=fail(o,PT_FUTURE_STALE);goto end;}
            o->phase=CHIP_BEGIN;
        }else {
            er=pt_sampler_pin_job_begin(&o->pin_job,p->sampler,p->project,s->request.sample,p->generation);
            if(er!=PT_EDIT_OK){result=fail(o,er==PT_EDIT_CAPACITY?PT_FUTURE_CAPACITY:PT_FUTURE_STALE);goto end;}
            o->phase=PIN_STEP;
        }
    }else if(o->phase==PIN_STEP) {
        er=pt_sampler_pin_job_step(&o->pin_job,PT_SAMPLER_PIN_CHUNK,&s->pcm,&s->pin,&done);
        if(er!=PT_EDIT_OK){result=fail(o,PT_FUTURE_STALE);goto end;}
        if(done)o->phase=CHIP_BEGIN;
    }else if(o->phase==CHIP_BEGIN) {
        cr=pt_sampler_paula_job_begin(&o->chip_job,&p->bridge,s->request.track,s->request.sample,s->request.channel,s->pin,&s->lease);
        if(cr==PT_CACHE_PENDING)o->phase=CHIP_STEP;
        else if(cr==PT_CACHE_HIT)s->leased=1;
        else {result=fail(o,cr==PT_CACHE_CAPACITY?PT_FUTURE_CAPACITY:cr==PT_CACHE_BUSY?PT_FUTURE_BUSY:PT_FUTURE_STALE);goto end;}
    }else {
        cr=pt_sampler_paula_job_step(&o->chip_job,&s->lease);
        if(cr==PT_CACHE_LOAD)s->leased=1;
        else if(cr!=PT_CACHE_PENDING){result=fail(o,PT_FUTURE_STALE);goto end;}
    }
    if(s->leased) {
        if(!pt_sampler_paula_prepared_location(&p->bridge,s->request.track,s->lease,&s->data,&s->bytes) ||
           (s->previous && (s->lease.slot!=s->previous->source[s->previous_source].lease.slot ||
            s->lease.serial!=s->previous->source[s->previous_source].lease.serial))) {result=fail(o,PT_FUTURE_STALE);goto end;}
        drop_borrow(s);++o->index;o->phase=PIN_BEGIN;
        if(o->index==o->count) {
            for(i=0;i<o->count;++i)if(!source_current(o,i)){result=fail(o,PT_FUTURE_STALE);goto end;}
            if(!owner_spans(o)){result=fail(o,PT_FUTURE_INVALID);goto end;}
            o->state=READY;result=PT_FUTURE_OK;
        }
    }
    *ready=o->state==READY;
end:p->busy=0;return result;
}
enum pt_paula_future_result pt_paula_future_view(struct pt_paula_future_owner *o,unsigned index,
    struct pt_paula_future_view *out)
{
    struct pt_paula_future_pool *p;struct source *s;struct pt_paula_future_view view;
    if(!o || !(p=o->pool) || p->busy || !registered(p,o) || !out || !output_disjoint(p,out,sizeof(*out)) ||
       index>=o->count || (o->state!=READY && o->state!=LIVE))return PT_FUTURE_INVALID;
    if(!current_pool(p) || !source_current(o,index))return PT_FUTURE_STALE;
    s=o->source+index;view=(struct pt_paula_future_view){s->data,s->bytes,s->pcm.frames,s->request.track,
        s->request.sample,s->request.channel,(unsigned)p->map[s->request.track],o->token};
    *out=view;return PT_FUTURE_OK;
}
enum pt_scheduled_result pt_paula_future_enqueue(struct pt_paula_future_owner *o,
    const struct pt_scheduled_batch *b,uint64_t *out)
{
    struct pt_paula_future_pool *p;struct pt_scheduled_owner owner;struct pt_scheduled_batch candidate;
    enum pt_scheduled_result result;unsigned i;
    if(!o || !(p=o->pool) || p->busy || !registered(p,o) || o->state!=READY || !b || !out ||
       !output_disjoint(p,out,sizeof(*out)) || !output_disjoint(p,b,sizeof(*b)) ||
       !disjoint(out,sizeof(*out),b,sizeof(*b)))return PT_SCHEDULED_INVALID;
    if(!current_pool(p))return PT_SCHEDULED_STALE;
    for(i=0;i<o->count;++i)if(!source_current(o,i))return PT_SCHEDULED_STALE;
    if(!batch_valid(o,b))return PT_SCHEDULED_INVALID;
    /* Use a local owner/batch copy so refused enqueue never alters holder state. */
    owner=(struct pt_scheduled_owner){o,o->token,owner_current,owner_retired,o->spans,o->span_count};
    candidate=*b;p->busy=1;p->callback=o;p->callback_batch=&candidate;
    result=pt_scheduled_output_enqueue(o->queue,b,&owner,out);
    p->callback=NULL;p->callback_batch=NULL;p->busy=0;
    if(result==PT_SCHEDULED_OK){o->batch=candidate;o->state=LIVE;o->ticket=*out;}
    return result;
}
enum pt_paula_future_result pt_paula_future_cancel(struct pt_paula_future_owner *o)
{
    struct pt_paula_future_pool *p;
    if(!o || !(p=o->pool) || p->busy || !registered(p,o))return PT_FUTURE_INVALID;
    if(o->state==LIVE || o->borrowers)return PT_FUTURE_BUSY;
    p->busy=1;resources(o);o->state=CANCELLED;p->busy=0;return PT_FUTURE_OK;
}
int pt_paula_future_owner_close(struct pt_paula_future_owner *o)
{
    struct pt_paula_future_pool *p;unsigned i;
    if(!o || !(p=o->pool) || p->busy || !registered(p,o) || o->state==LIVE || o->borrowers)return 0;
    p->busy=1;resources(o);for(i=0;i<p->config.maximum_owners;++i)if(p->owners[i]==o)p->owners[i]=NULL;
    p->bytes-=sizeof(*o);p->allocator.release(p->allocator.context,o);p->busy=0;return 1;
}
int pt_paula_future_close(struct pt_paula_future_pool *p)
{
    unsigned i;struct pt_allocator a;
    if(!p || p->busy)return 0;
    for(i=0;i<p->config.maximum_owners;++i)if(p->owners[i])return 0;
    p->busy=1;p->closing=1;if(!pt_sampler_paula_close(&p->bridge)){p->busy=0;return 0;}
    a=p->allocator;a.release(a.context,p);return 1;
}
