#include "sampler_paula_readers.h"
#include "sampler_internal.h"
#include "sampler_paula_internal.h"
#include "project_snapshot.h"
#include <string.h>
#include <limits.h>
enum {PREPARING=1,READY,LIVE,RETIRED,CANCELLED,FAILED};
enum {PIN_BEGIN,PIN_STEP,CHIP_BEGIN,CHIP_STEP};
struct pt_paula_readers_reader {
    struct pt_paula_readers_pool *pool;struct pt_readers_output *queue;
    uint64_t token,frame,trigger;unsigned action,state,leased,released,terminal_seen,terminal_valid,key_seen;
    struct pt_readers_key key;
    unsigned track,sample,channel,slot;
    struct pt_sample original;struct pt_sample_version *pin;struct pt_pcm pcm;struct pt_cache_lease lease;
    const uint8_t *data;size_t bytes;
    struct pt_sampler_pin_job pin_job;struct pt_sampler_paula_job chip_job;
    struct pt_scheduled_span spans[PT_SCHEDULED_SPANS];unsigned span_count;
};
struct command_source {
    struct pt_paula_readers_request request;
    /* Trigger only, registered pointer/token checked before every dereference. */
    struct pt_paula_readers_reader *reader;uint64_t reader_token;
};
struct pt_paula_readers_command {
    struct pt_paula_readers_pool *pool;struct pt_readers_output *queue;
    uint64_t token,frame,ticket;unsigned count,index,phase,state,terminal_seen,terminal_valid;
    struct command_source source[PT_PAULA_READERS_ACTIONS];struct pt_scheduled_batch batch;
    unsigned lowered;struct pt_paula_render_plan lower[PT_PAULA_READERS_ACTIONS];
};
struct pt_paula_readers_pool {
    struct pt_allocator allocator;struct pt_sampler *sampler;struct pt_project *project,header;
    struct pt_paula_readers_config config;struct pt_sampler_paula bridge;
    struct pt_sample original[PT_PROJECT_SAMPLES];uint8_t original_external[PT_PROJECT_SAMPLES];int8_t map[PT_CHANNEL_LIMIT];
    struct pt_paula_readers_command *commands[PT_PAULA_READERS_COMMANDS],*callback;
    struct pt_paula_readers_reader *readers[PT_PAULA_READERS_PERSISTENT];
    const struct pt_scheduled_batch *callback_batch;
    size_t bytes;uint64_t serial;unsigned generation,busy,closing,failed,queue_call;
};
static int valid_span(const void *p,size_t n)
{return !n || (p && (uintptr_t)p<=UINTPTR_MAX-n);}
static int disjoint(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return valid_span(a,n)&&valid_span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int sample_disjoint(const struct pt_sample *s,const void *out,size_t bytes)
{return s->pcm.capacity<=SIZE_MAX/sizeof(int32_t)&&disjoint(out,bytes,s->pcm.data,s->pcm.capacity*sizeof(int32_t))&&
 disjoint(out,bytes,s->slices,(size_t)s->slice_count*sizeof(uint32_t));}
static int project_fixed_disjoint(const struct pt_project *p,const void *out,size_t bytes)
{
 size_t events;unsigned i;
 if(!p||p->sample_count>PT_PROJECT_SAMPLES||p->pattern_count>PT_PROJECT_PATTERNS||p->order_count>PT_PROJECT_ORDERS||
    p->channels.count>PT_CHANNEL_LIMIT||!disjoint(out,bytes,p,sizeof(*p))||
    !disjoint(out,bytes,p->samples,(size_t)p->sample_count*sizeof(*p->samples))||
    !disjoint(out,bytes,p->orders,(size_t)p->order_count*sizeof(*p->orders))||
    !disjoint(out,bytes,p->extensions,(size_t)p->extension_count*sizeof(*p->extensions)))return 0;
 events=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
 if(!disjoint(out,bytes,p->events,events*sizeof(*p->events)))return 0;
 for(i=0;i<p->extension_count;++i)if(!disjoint(out,bytes,p->extensions[i].data,p->extensions[i].length))return 0;
 return 1;
}
static int project_disjoint(const struct pt_project *p,const void *out,size_t bytes)
{unsigned i;if(!project_fixed_disjoint(p,out,bytes))return 0;for(i=0;i<p->sample_count;++i)if(!sample_disjoint(p->samples+i,out,bytes))return 0;return 1;}
static int command_registered(struct pt_paula_readers_pool *p,const struct pt_paula_readers_command *c)
{unsigned i;for(i=0;i<p->config.maximum_commands;++i)if(p->commands[i]==c)return 1;return 0;}
static int reader_registered(struct pt_paula_readers_pool *p,const struct pt_paula_readers_reader *r)
{unsigned i;for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i]==r)return 1;return 0;}
static int reentry(struct pt_paula_readers_pool *p)
{if(p->busy){p->failed=1;return 1;}return 0;}
static int current_pool(struct pt_paula_readers_pool *p)
{
 struct pt_project header;
 if(p->closing||p->failed||p->sampler->generation!=p->generation||!pt_sampler_paula_prepared_current(&p->bridge))return 0;
 memcpy(&header,&p->header,sizeof(header));header.channels.selected=p->project->channels.selected;
 return pt_project_snapshot_equal(p->project,&header);
}
static int output_disjoint(struct pt_paula_readers_pool *p,const void *out,size_t bytes)
{
 unsigned i;
 if(!bytes||!valid_span(out,bytes)||!disjoint(out,bytes,p,sizeof(*p))||!disjoint(out,bytes,p->project,sizeof(*p->project))||
    !project_fixed_disjoint(&p->header,out,bytes)||!pt_sampler_output_disjoint(p->sampler,out,bytes))return 0;
 if(current_pool(p)&&!project_disjoint(p->project,out,bytes))return 0;
 for(i=0;i<p->header.sample_count;++i)if(p->original_external[i]&&!sample_disjoint(p->original+i,out,bytes))return 0;
 for(i=0;i<PT_CACHE_SLOTS;++i)if(!disjoint(out,bytes,p->bridge.cache.entry[i].data,p->bridge.cache.entry[i].bytes))return 0;
 for(i=0;i<p->config.maximum_commands;++i)if(p->commands[i]&&!disjoint(out,bytes,p->commands[i],sizeof(*p->commands[i])))return 0;
 for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i]) {
   struct pt_paula_readers_reader *r=p->readers[i];
   if(!disjoint(out,bytes,r,sizeof(*r))||!pt_sampler_pin_job_output_disjoint(&r->pin_job,out,bytes)||
      !pt_sampler_version_output_disjoint(r->pin,out,bytes)||!pt_sampler_version_output_disjoint(r->chip_job.pin,out,bytes))return 0;
 }
 return 1;
}
static int same_key(const struct pt_readers_key *a,const struct pt_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&
 a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
static int zero_key(const struct pt_readers_key *k)
{return !k->queue&&!k->session&&!k->generation&&!k->trigger&&!k->owner&&!k->serial&&!k->action&&!k->slot;}
static int source_current(struct pt_paula_readers_reader *r)
{
 struct pt_paula_readers_pool *p=r->pool;struct pt_pcm pcm;struct pt_sample_version *pin;const uint8_t *data;size_t bytes;
 if(!r->pin||!r->leased||pt_sampler_pin_current(p->sampler,p->project,r->sample,p->generation,r->pin,&pcm,&pin)!=PT_EDIT_OK)return 0;
 pt_sampler_unpin(pin);
 return pt_sampler_paula_prepared_location_validated(&p->bridge,r->track,r->lease,&data,&bytes)&&data==r->data&&bytes==r->bytes;
}
/* Resolve the immutable full key by registration first; a caller never carries
 * a dereferenceable predecessor pointer. Current callbacks do NOT recurse here. */
static struct pt_paula_readers_reader *resolve(struct pt_paula_readers_pool *p,struct pt_readers_output *q,
 const struct pt_paula_readers_request *x,uint64_t frame)
{
 struct pt_paula_readers_reader *r=NULL;struct pt_readers_key actual;enum pt_scheduled_result result;unsigned i;
 if(x->key.queue!=q||x->key.generation!=p->config.generation||x->key.slot!=(unsigned)p->map[x->track])return NULL;
 for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i]) {
   struct pt_paula_readers_reader *v=p->readers[i];
   if(v->state==LIVE&&v->queue==q&&v->token==x->key.owner&&v->trigger==x->key.trigger&&v->action==x->key.action) {r=v;break;}
 }
 if(!r||!r->key_seen||!same_key(&r->key,&x->key)||r->frame>=frame||r->track!=x->track||r->sample!=x->sample||r->channel!=x->channel)return NULL;
 /* No external current/holder callback before the real core validates the key. */
 p->queue_call=1;result=pt_readers_reader_key(q,r->trigger,r->action,&actual);p->queue_call=0;
 if(result!=PT_SCHEDULED_OK||p->failed||!same_key(&actual,&x->key)||!source_current(r))return NULL;
 return r;
}
static int request_valid(struct pt_paula_readers_pool *p,struct pt_readers_output *q,uint64_t frame,
 const struct pt_paula_readers_request *x,unsigned n)
{
 unsigned i,seen=0;
 if(!q||frame==UINT64_MAX||!n||n>PT_PAULA_READERS_ACTIONS)return 0;
 for(i=0;i<n;++i) {
   if(x[i].track>=p->header.channels.count||p->map[x[i].track]<0||x[i].sample>=p->header.sample_count||
      x[i].channel>=p->project->samples[x[i].sample].pcm.channels||(seen&(1U<<p->map[x[i].track])))return 0;
   seen|=1U<<p->map[x[i].track];
   if(x[i].kind==PT_SCHEDULED_TRIGGER){if(!zero_key(&x[i].key))return 0;}
   else if((x[i].kind!=PT_SCHEDULED_CONTROL&&x[i].kind!=PT_SCHEDULED_STOP)||!resolve(p,q,x+i,frame))return 0;
 }
 return 1;
}
static int same_sample(const struct pt_sample *a,const struct pt_sample *b)
{return !memcmp(a->name,b->name,sizeof(a->name))&&a->pcm.data==b->pcm.data&&a->pcm.capacity==b->pcm.capacity&&
 a->pcm.frames==b->pcm.frames&&a->pcm.rate==b->pcm.rate&&a->pcm.channels==b->pcm.channels&&a->pcm.bits==b->pcm.bits&&
 a->slices==b->slices&&a->slice_count==b->slice_count&&a->loop_start==b->loop_start&&a->loop_end==b->loop_end&&
 a->crossfade==b->crossfade&&a->loop==b->loop&&a->volume==b->volume&&a->interpolation==b->interpolation&&a->finetune==b->finetune;}
static int prepared_current(struct pt_paula_readers_reader *r)
{
 struct pt_paula_readers_pool *p=r->pool;unsigned i;
 if(same_sample(&r->original,p->project->samples+r->sample))return 1;
 for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i]) {
   struct pt_paula_readers_reader *v=p->readers[i];struct pt_pcm pcm;struct pt_sample_version *pin;
   if(v->pin&&v->sample==r->sample&&same_sample(&v->original,&r->original)&&
      pt_sampler_pin_current(p->sampler,p->project,r->sample,p->generation,v->pin,&pcm,&pin)==PT_EDIT_OK) {pt_sampler_unpin(pin);return 1;}
 }
 return 0;
}
static void resources(struct pt_paula_readers_reader *r)
{
 struct pt_paula_readers_pool *p=r->pool;
 if(r->released)return;
 pt_sampler_paula_job_cancel(&r->chip_job);pt_sampler_pin_job_cancel(&r->pin_job);
 if(r->leased)pt_sampler_paula_unpin(&p->bridge,r->lease);
 pt_sampler_unpin(r->pin);r->pin=NULL;r->leased=0;r->data=NULL;r->bytes=0;r->released=1;
 memset(&r->pcm,0,sizeof(r->pcm));memset(&r->original,0,sizeof(r->original));
}
static void dispose_reader(struct pt_paula_readers_reader *r)
{struct pt_paula_readers_pool *p=r->pool;unsigned i;resources(r);for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i]==r)p->readers[i]=NULL;
 p->bytes-=sizeof(*r);p->allocator.release(p->allocator.context,r);}
static void partial_resources(struct pt_paula_readers_command *c)
{
 unsigned i;struct pt_paula_readers_pool *p=c->pool;
 for(i=0;i<c->count;++i)if(c->source[i].reader) {
   struct pt_paula_readers_reader *r=c->source[i].reader;
   if(reader_registered(p,r)&&r->token==c->source[i].reader_token&&r->state!=LIVE&&r->state!=RETIRED)dispose_reader(r);
   c->source[i].reader=NULL;c->source[i].reader_token=0;
 }
}
static struct pt_paula_readers_reader *trigger_reader(struct pt_paula_readers_command *c,unsigned i)
{struct pt_paula_readers_reader *r=c->source[i].reader;return r&&reader_registered(c->pool,r)&&r->token==c->source[i].reader_token?r:NULL;}
static int batch_valid(struct pt_paula_readers_command *c,const struct pt_scheduled_batch *b)
{
 unsigned i,j,seen=0;struct pt_paula_readers_pool *p=c->pool;
 if(!b||b->generation!=p->config.generation||b->frame!=c->frame||b->count!=c->count)return 0;
 for(i=0;i<b->count;++i) {
   const struct pt_scheduled_action *a=b->action+i;struct command_source *s;
   if(a->slot>=4||(seen&(1U<<a->slot))||a->volume>64)return 0;
   seen|=1U<<a->slot;
   for(j=0;j<c->count;++j)if(p->map[c->source[j].request.track]==(int)a->slot)break;
   if(j==c->count)return 0;
   s=c->source+j;if(a->kind!=s->request.kind)return 0;
   if(c->lowered) {
     const struct pt_paula_render_plan *v=c->lower+j;
     if(a->period!=v->period||a->volume!=v->volume)return 0;
     if(a->kind==PT_SCHEDULED_TRIGGER) {
       struct pt_paula_readers_reader *r=trigger_reader(c,j);
       if(!r||v->offset>r->bytes||v->length>r->bytes-v->offset||
          a->data!=r->data+v->offset||a->words!=v->length/2)return 0;
     }
   }
   if(a->kind==PT_SCHEDULED_TRIGGER) {
     struct pt_paula_readers_reader *r=trigger_reader(c,j);uintptr_t x=(uintptr_t)a->data,y;size_t n=(size_t)a->words*2;
     if(!r)return 0;
     y=(uintptr_t)r->data;
     if(!a->words||!a->period||(x&1)||x<y||x-y>r->bytes||n>r->bytes-(x-y))return 0;
   }else if(a->kind==PT_SCHEDULED_CONTROL) {if(a->data||a->words||!a->period)return 0;}
   else if(a->kind==PT_SCHEDULED_STOP) {if(a->data||a->words||a->period||a->volume)return 0;}
   else return 0;
 }
 return 1;
}
static int command_current(void *context,uint64_t token,uint64_t generation)
{
 struct pt_paula_readers_command *c=context;struct pt_paula_readers_pool *p=c->pool;
 if((p->busy&&!p->queue_call)||!command_registered(p,c)||token!=c->token||generation!=p->config.generation||
    (c->state!=READY&&c->state!=LIVE)||!current_pool(p))return 0;
 return batch_valid(c,p->callback==c&&p->callback_batch?p->callback_batch:&c->batch);
}
static int reader_current(void *context,uint64_t token,uint64_t generation)
{
 struct pt_paula_readers_reader *r=context;struct pt_paula_readers_pool *p=r->pool;
 return !(p->busy&&!p->queue_call)&&reader_registered(p,r)&&token==r->token&&generation==p->config.generation&&
   (r->state==READY||r->state==LIVE)&&current_pool(p)&&source_current(r);
}
static void command_terminal(void *context,uint64_t token,int valid)
{struct pt_paula_readers_command *c=context;struct pt_paula_readers_pool *p=c->pool;
 if(!command_registered(p,c)||token!=c->token||c->state!=LIVE)return;
 c->terminal_seen=1;c->terminal_valid=valid!=0;if(!valid)p->failed=1;}
static void command_retired(void *context,uint64_t token)
{struct pt_paula_readers_command *c=context;if(command_registered(c->pool,c)&&c->token==token&&c->state==LIVE)c->state=RETIRED;}
static void reader_terminal(void *context,uint64_t token,int valid)
{struct pt_paula_readers_reader *r=context;struct pt_paula_readers_pool *p=r->pool;
 if(!reader_registered(p,r)||token!=r->token||r->state!=LIVE)return;
 r->terminal_seen=1;r->terminal_valid=valid!=0;if(!valid)p->failed=1;}
static void reader_retired(void *context,uint64_t token)
{struct pt_paula_readers_reader *r=context;struct pt_paula_readers_pool *p=r->pool;
 if(!reader_registered(p,r)||token!=r->token||r->state!=LIVE)return;
 r->state=RETIRED;if(p->busy)return;
 p->busy=1;resources(r);p->busy=0;}
static int add_span(struct pt_paula_readers_reader *r,const void *data,size_t bytes)
{if(!bytes)return 1;if(!valid_span(data,bytes)||r->span_count==PT_SCHEDULED_SPANS)return 0;
 r->spans[r->span_count++]=(struct pt_scheduled_span){data,bytes};return 1;}
static int reader_spans(struct pt_paula_readers_reader *r)
{
 struct pt_paula_readers_pool *p=r->pool;struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned i,n;
 r->span_count=0;
 /* Mutable reader and command controls are deliberately absent from resources. */
 if(!add_span(r,p,sizeof(*p))||!add_span(r,p->sampler,sizeof(*p->sampler))||!add_span(r,p->project,sizeof(*p->project))||
    !add_span(r,p->sampler->table,p->sampler->table_bytes)||
    !add_span(r,p->header.samples,(size_t)p->header.sample_count*sizeof(*p->header.samples)))return 0;
 if(!pt_sampler_version_spans(r->pin,spans,PT_SAMPLER_VERSION_SPANS,&n))return 0;
 for(i=0;i<n;++i)if(!add_span(r,spans[i].data,spans[i].bytes))return 0;
 if(!add_span(r,r->data,r->bytes))return 0;
 return !p->original_external[r->sample]||
   (add_span(r,p->original[r->sample].pcm.data,p->original[r->sample].pcm.capacity*sizeof(int32_t))&&
    add_span(r,p->original[r->sample].slices,(size_t)p->original[r->sample].slice_count*sizeof(uint32_t)));
}
enum pt_paula_readers_result pt_paula_readers_open(const struct pt_allocator *a,struct pt_sampler *sampler,
 struct pt_project *project,const struct pt_paula_readers_config *config,struct pt_paula_readers_pool **out)
{
 struct pt_paula_readers_pool *p;struct pt_allocator allocator;struct pt_paula_readers_config c;unsigned i;
 if(!a||!a->allocate||!a->release||!sampler||!config||!out||!config->chip_allocate||!config->chip_release||!config->generation||
    !config->maximum_commands||config->maximum_commands>PT_PAULA_READERS_COMMANDS||!config->maximum_readers||
    config->maximum_readers>PT_PAULA_READERS_PERSISTENT||pt_project_validate(project,NULL)!=PT_PROJECT_OK)return PT_PAULA_READERS_INVALID;
 if(!project_disjoint(project,out,sizeof(*out))||!pt_sampler_output_disjoint(sampler,out,sizeof(*out))||
    !disjoint(out,sizeof(*out),a,sizeof(*a))||!disjoint(out,sizeof(*out),config,sizeof(*config)))return PT_PAULA_READERS_INVALID;
 allocator=*a;c=*config;
 if(c.control_budget<sizeof(*p))return PT_PAULA_READERS_CAPACITY;
 p=allocator.allocate(allocator.context,sizeof(*p));if(!p)return PT_PAULA_READERS_CAPACITY;
 if(!project_disjoint(project,p,sizeof(*p))||!pt_sampler_output_disjoint(sampler,p,sizeof(*p))||
    !disjoint(p,sizeof(*p),out,sizeof(*out))||!disjoint(p,sizeof(*p),a,sizeof(*a))||!disjoint(p,sizeof(*p),config,sizeof(*config))) {
   allocator.release(allocator.context,p);return PT_PAULA_READERS_INVALID;
 }
 memset(p,0,sizeof(*p));p->allocator=allocator;p->config=c;p->sampler=sampler;p->project=project;p->header=*project;
 memcpy(p->original,project->samples,project->sample_count*sizeof(*project->samples));
 for(i=0;i<project->sample_count;++i)p->original_external[i]=sampler->current[i]==NULL;
 p->generation=sampler->generation;p->bytes=sizeof(*p);
 if(pt_channels_paula_map(&project->channels,NULL,p->map)!=PT_CHANNEL_OK||
    !pt_sampler_paula_bind(&p->bridge,sampler,project,c.chip_context,c.chip_allocate,c.chip_release,c.chip_budget)) {
   allocator.release(allocator.context,p);return PT_PAULA_READERS_INVALID;
 }
 *out=p;return PT_PAULA_READERS_OK;
}
/* Extra borrowed inputs are checked again after every allocator callback,
 * before any returned arena is initialized. Staged copies are protected too. */
struct lower_input {const void *source,*copy;size_t bytes;};
static int inputs_current(const struct lower_input *in,unsigned n)
{
 unsigned i;
 for(i=0;i<n;++i) {
   if(memcmp(in[i].source,in[i].copy,in[i].bytes))return 0;
 }
 return 1;
}
static int inputs_apart(const struct lower_input *in,unsigned n,const void *out,size_t bytes)
{
 unsigned i;
 if(!disjoint(out,bytes,in,n*sizeof(*in)))return 0;
 for(i=0;i<n;++i) {
   if(!disjoint(out,bytes,in[i].source,in[i].bytes)||!disjoint(out,bytes,in[i].copy,in[i].bytes))return 0;
 }
 return 1;
}
static enum pt_paula_readers_result readers_begin(struct pt_paula_readers_pool *p,struct pt_readers_output *q,
 uint64_t frame,const struct pt_paula_readers_request *r,unsigned n,struct pt_paula_readers_command **out,
 const struct lower_input *inputs,unsigned input_count,const struct pt_paula_render_plan *lower)
{
 struct pt_paula_readers_command *c;struct pt_paula_readers_request request[PT_PAULA_READERS_ACTIONS];unsigned i,k,free_readers=0,triggers=0;
 size_t need;
 if(!p||reentry(p)||p->closing||!r||!n||n>PT_PAULA_READERS_ACTIONS||!out||!output_disjoint(p,out,sizeof(*out))||
    !output_disjoint(p,r,n*sizeof(*r))||!disjoint(out,sizeof(*out),r,n*sizeof(*r)))return PT_PAULA_READERS_INVALID;
 if(!current_pool(p))return PT_PAULA_READERS_STALE;
 memcpy(request,r,n*sizeof(*r));p->busy=1;
 if(!request_valid(p,q,frame,request,n)){p->busy=0;return PT_PAULA_READERS_INVALID;}
 for(k=0;k<p->config.maximum_commands;++k)if(!p->commands[k])break;
 for(i=0;i<n;++i)triggers+=request[i].kind==PT_SCHEDULED_TRIGGER;
 for(i=0;i<p->config.maximum_readers;++i)free_readers+=p->readers[i]==NULL;
 need=sizeof(*c)+(size_t)triggers*sizeof(struct pt_paula_readers_reader);
 if(k==p->config.maximum_commands||triggers>free_readers||p->serial>UINT64_MAX-1-triggers||
    p->bytes>p->config.control_budget||need>p->config.control_budget-p->bytes){p->busy=0;return PT_PAULA_READERS_CAPACITY;}
 c=p->allocator.allocate(p->allocator.context,sizeof(*c));
 if(!c){p->busy=0;return PT_PAULA_READERS_CAPACITY;}
 if(!current_pool(p)||!inputs_current(inputs,input_count)||!request_valid(p,q,frame,request,n)||
    !inputs_apart(inputs,input_count,c,sizeof(*c))||!disjoint(c,sizeof(*c),request,sizeof(request))||
    !output_disjoint(p,c,sizeof(*c))||
    !disjoint(c,sizeof(*c),out,sizeof(*out))||!disjoint(c,sizeof(*c),r,n*sizeof(*r))) {
   p->allocator.release(p->allocator.context,c);p->busy=0;return PT_PAULA_READERS_INVALID;
 }
 memset(c,0,sizeof(*c));c->pool=p;c->queue=q;c->frame=frame;c->token=++p->serial;c->count=n;c->state=PREPARING;
 if(lower){c->lowered=1;memcpy(c->lower,lower,n*sizeof(*lower));}
 p->commands[k]=c;p->bytes+=sizeof(*c);
 for(i=0;i<n;++i) {
   c->source[i].request=request[i];
   if(request[i].kind==PT_SCHEDULED_TRIGGER) {
     struct pt_paula_readers_reader *v=p->allocator.allocate(p->allocator.context,sizeof(*v));unsigned j;
     if(!v){partial_resources(c);p->commands[k]=NULL;p->bytes-=sizeof(*c);p->allocator.release(p->allocator.context,c);p->busy=0;return PT_PAULA_READERS_CAPACITY;}
     if(!current_pool(p)||!inputs_current(inputs,input_count)||!request_valid(p,q,frame,request,n)||
        !inputs_apart(inputs,input_count,v,sizeof(*v))||!disjoint(v,sizeof(*v),request,sizeof(request))||
        !output_disjoint(p,v,sizeof(*v))||
        !disjoint(v,sizeof(*v),out,sizeof(*out))||!disjoint(v,sizeof(*v),r,n*sizeof(*r))) {
       p->allocator.release(p->allocator.context,v);partial_resources(c);p->commands[k]=NULL;p->bytes-=sizeof(*c);
       p->allocator.release(p->allocator.context,c);p->busy=0;return PT_PAULA_READERS_INVALID;
     }
     memset(v,0,sizeof(*v));v->pool=p;v->queue=q;v->token=++p->serial;v->frame=frame;v->state=PREPARING;
     v->track=request[i].track;v->sample=request[i].sample;v->channel=request[i].channel;v->slot=(unsigned)p->map[v->track];
     v->original=p->project->samples[v->sample];
     for(j=0;j<p->config.maximum_readers;++j)if(!p->readers[j])break;
     p->readers[j]=v;p->bytes+=sizeof(*v);c->source[i].reader=v;c->source[i].reader_token=v->token;
   }
 }
 p->busy=0;*out=c;return PT_PAULA_READERS_OK;
}
enum pt_paula_readers_result pt_paula_readers_begin(struct pt_paula_readers_pool *p,struct pt_readers_output *q,
 uint64_t frame,const struct pt_paula_readers_request *r,unsigned n,struct pt_paula_readers_command **out)
{return readers_begin(p,q,frame,r,n,out,NULL,0,NULL);}
static int lower_voice_same(const struct pt_voice *a,const struct pt_voice *b)
{
 return a->pcm==b->pcm&&a->repeat_pcm==b->repeat_pcm&&a->phase==b->phase&&a->cycle==b->cycle&&
 a->start==b->start&&a->end==b->end&&a->loop_start==b->loop_start&&a->loop_end==b->loop_end&&
 a->loop==b->loop&&a->looped==b->looped&&a->linear==b->linear&&a->active==b->active&&a->segment==b->segment;
}
static struct pt_paula_readers_reader *lower_original(struct pt_paula_readers_pool *p,
 struct pt_readers_output *q,unsigned track,const struct pt_readers_key *key,uint64_t frame,
 struct pt_paula_readers_request *request)
{
 unsigned i;
 for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i]) {
   struct pt_paula_readers_reader *r=p->readers[i];
   if(r->state==LIVE&&r->key_seen&&r->queue==q&&r->track==track&&same_key(&r->key,key)) {
     request->sample=r->sample;request->channel=r->channel;request->key=*key;
     return resolve(p,q,request,frame);
   }
 }
 return NULL;
}
enum pt_paula_readers_result pt_paula_readers_lower_begin(struct pt_paula_readers_pool *p,
 struct pt_readers_output *q,uint64_t frame,uint32_t rate,const struct pt_render_plan *plan,
 const struct pt_paula_render_caps *caps,const struct pt_readers_key keys[PT_CHANNEL_LIMIT],
 struct pt_paula_readers_command **out)
{
 struct pt_render_plan copy;struct pt_paula_render_caps limits;struct pt_readers_key keycopy[PT_CHANNEL_LIMIT];
 struct pt_paula_readers_request request[4];struct pt_paula_render_plan lower[4];
 struct lower_input inputs[4];unsigned input_count=0,i,j,n=0;int first[4],control[4];
 if(!p||reentry(p)||p->closing||!q||frame==UINT64_MAX||!out||!valid_span(plan,sizeof(*plan))||
    !valid_span(caps,sizeof(*caps))||(keys&&!valid_span(keys,sizeof(keycopy)))||
    (rate!=44100&&rate!=48000))return PT_PAULA_READERS_INVALID;
 if(!current_pool(p))return PT_PAULA_READERS_STALE;
 if(!output_disjoint(p,out,sizeof(*out))||!output_disjoint(p,plan,sizeof(*plan))||
    !output_disjoint(p,caps,sizeof(*caps))||(keys&&!output_disjoint(p,keys,sizeof(keycopy)))||
    !disjoint(out,sizeof(*out),plan,sizeof(*plan))||!disjoint(out,sizeof(*out),caps,sizeof(*caps))||
    !disjoint(plan,sizeof(*plan),caps,sizeof(*caps))||
    (keys&&(!disjoint(out,sizeof(*out),keys,sizeof(keycopy))||!disjoint(plan,sizeof(*plan),keys,sizeof(keycopy))||
     !disjoint(caps,sizeof(*caps),keys,sizeof(keycopy)))))return PT_PAULA_READERS_INVALID;
 memcpy(&copy,plan,sizeof(copy));memcpy(&limits,caps,sizeof(limits));if(keys)memcpy(keycopy,keys,sizeof(keycopy));
 if(copy.count>PT_RENDER_ACTIONS||!pt_paula_render_caps_valid(&limits))
   return PT_PAULA_READERS_INVALID;
 for(i=0;i<4;++i)first[i]=control[i]=-1;
 for(i=0;i<copy.count;++i) {
   const struct pt_render_action *a=copy.action+i;unsigned slot;
   if(a->channel>=p->header.channels.count||
      (a->kind!=PT_RENDER_TRIGGER&&a->kind!=PT_RENDER_SEGMENT&&a->kind!=PT_RENDER_REPEAT&&
       a->kind!=PT_RENDER_STOP&&a->kind!=PT_RENDER_CONTROL))
     return PT_PAULA_READERS_INVALID;
   if(p->map[a->channel]<0)continue;
   slot=(unsigned)p->map[a->channel];
   if(a->kind==PT_RENDER_SEGMENT||a->kind==PT_RENDER_REPEAT)return PT_PAULA_READERS_INVALID;
   if(first[slot]<0){first[slot]=(int)i;continue;}
   if(copy.action[first[slot]].kind!=PT_RENDER_TRIGGER||a->kind!=PT_RENDER_CONTROL||control[slot]>=0||
      !lower_voice_same(&copy.action[first[slot]].voice,&a->voice))return PT_PAULA_READERS_INVALID;
   control[slot]=(int)i;
 }
 memset(request,0,sizeof(request));memset(lower,0,sizeof(lower));p->busy=1;
 for(i=0;i<4;++i)if(first[i]>=0) {
   const struct pt_render_action *a=copy.action+first[i];struct pt_voice voice=a->voice;
   const uint32_t *gain=a->gain;struct pt_paula_readers_request *r=request+n;
   r->track=a->channel;
   if(a->kind==PT_RENDER_TRIGGER) {
     for(j=0;j<p->header.sample_count;++j)if(voice.pcm==&p->project->samples[j].pcm)break;
     if(j==p->header.sample_count){p->busy=0;return PT_PAULA_READERS_INVALID;}
     r->kind=PT_SCHEDULED_TRIGGER;r->sample=j;r->channel=0;
     if(control[i]>=0){voice.step=copy.action[control[i]].voice.step;gain=copy.action[control[i]].gain;}
     if(!pt_paula_render_voice(&voice,rate,gain,i,&limits,lower+n)){p->busy=0;return PT_PAULA_READERS_INVALID;}
   }else {
     r->kind=a->kind==PT_RENDER_CONTROL?PT_SCHEDULED_CONTROL:PT_SCHEDULED_STOP;
     if(!keys||!lower_original(p,q,r->track,keycopy+r->track,frame,r)){p->busy=0;return PT_PAULA_READERS_INVALID;}
     if(r->kind==PT_SCHEDULED_CONTROL&&!pt_paula_render_control(voice.step,rate,gain,i,&limits,
        &lower[n].period,&lower[n].volume)){p->busy=0;return PT_PAULA_READERS_INVALID;}
   }
   ++n;
 }
 p->busy=0;if(!n){*out=NULL;return PT_PAULA_READERS_OK;}
 inputs[input_count++]=(struct lower_input){plan,&copy,sizeof(copy)};
 inputs[input_count++]=(struct lower_input){caps,&limits,sizeof(limits)};
 if(keys)inputs[input_count++]=(struct lower_input){keys,keycopy,sizeof(keycopy)};
 /* Protect the staged normalized values against an overlapping allocator arena. */
 inputs[input_count++]=(struct lower_input){lower,lower,sizeof(lower)};
 return readers_begin(p,q,frame,request,n,out,inputs,input_count,lower);
}
static enum pt_paula_readers_result fail(struct pt_paula_readers_command *c,enum pt_paula_readers_result result)
{partial_resources(c);c->state=FAILED;return result;}
static int sources_current(struct pt_paula_readers_command *c)
{
 unsigned i;struct pt_paula_readers_pool *p=c->pool;
 for(i=0;i<c->count;++i)if(c->source[i].request.kind==PT_SCHEDULED_TRIGGER) {
   struct pt_paula_readers_reader *r=trigger_reader(c,i);if(!r||!source_current(r))return 0;
 }else if(!resolve(p,c->queue,&c->source[i].request,c->frame))return 0;
 return 1;
}
enum pt_paula_readers_result pt_paula_readers_step(struct pt_paula_readers_command *c,unsigned *ready)
{
 struct pt_paula_readers_pool *p;struct command_source *s;struct pt_paula_readers_reader *r;
 enum pt_edit_result er;enum pt_cache_result cr;enum pt_paula_readers_result result=PT_PAULA_READERS_PENDING;unsigned done;
 if(!c||!(p=c->pool)||reentry(p)||!command_registered(p,c)||!ready||!output_disjoint(p,ready,sizeof(*ready))||
    (c->state!=PREPARING&&c->state!=READY))return PT_PAULA_READERS_INVALID;
 p->busy=1;
 if(!current_pool(p)){result=fail(c,PT_PAULA_READERS_STALE);goto end;}
 if(c->state==READY) {
   if(!sources_current(c)){result=fail(c,PT_PAULA_READERS_STALE);goto end;}
   *ready=1;result=PT_PAULA_READERS_OK;goto end;
 }
 s=c->source+c->index;
 if(s->request.kind!=PT_SCHEDULED_TRIGGER) {
   if(!resolve(p,c->queue,&s->request,c->frame)){result=fail(c,PT_PAULA_READERS_STALE);goto end;}
   ++c->index;
 }else {
   r=trigger_reader(c,c->index);
   if(!r){result=fail(c,PT_PAULA_READERS_STALE);goto end;}
   if(c->phase==PIN_BEGIN) {
     if(!prepared_current(r)){result=fail(c,PT_PAULA_READERS_STALE);goto end;}
     er=pt_sampler_pin_job_begin(&r->pin_job,p->sampler,p->project,r->sample,p->generation);
     if(er!=PT_EDIT_OK){result=fail(c,er==PT_EDIT_CAPACITY?PT_PAULA_READERS_CAPACITY:PT_PAULA_READERS_STALE);goto end;}
     c->phase=PIN_STEP;
   }else if(c->phase==PIN_STEP) {
     er=pt_sampler_pin_job_step(&r->pin_job,PT_SAMPLER_PIN_CHUNK,&r->pcm,&r->pin,&done);
     if(er!=PT_EDIT_OK){result=fail(c,PT_PAULA_READERS_STALE);goto end;}
     if(done)c->phase=CHIP_BEGIN;
   }else if(c->phase==CHIP_BEGIN) {
     cr=pt_sampler_paula_job_begin(&r->chip_job,&p->bridge,r->track,r->sample,r->channel,r->pin,&r->lease);
     if(cr==PT_CACHE_PENDING)c->phase=CHIP_STEP;
     else if(cr==PT_CACHE_HIT)r->leased=1;
     else{result=fail(c,cr==PT_CACHE_CAPACITY?PT_PAULA_READERS_CAPACITY:cr==PT_CACHE_BUSY?PT_PAULA_READERS_BUSY:PT_PAULA_READERS_STALE);goto end;}
   }else {
     cr=pt_sampler_paula_job_step(&r->chip_job,&r->lease);
     if(cr==PT_CACHE_LOAD)r->leased=1;
     else if(cr!=PT_CACHE_PENDING){result=fail(c,PT_PAULA_READERS_STALE);goto end;}
   }
   if(p->failed){result=fail(c,PT_PAULA_READERS_STALE);goto end;}
   if(r->leased) {
     if(!pt_sampler_paula_prepared_location(&p->bridge,r->track,r->lease,&r->data,&r->bytes)||!reader_spans(r)) {
       result=fail(c,PT_PAULA_READERS_STALE);goto end;
     }
     r->state=READY;++c->index;c->phase=PIN_BEGIN;
   }
 }
 if(c->index==c->count) {
   if(!sources_current(c)){result=fail(c,PT_PAULA_READERS_STALE);goto end;}
   c->state=READY;result=PT_PAULA_READERS_OK;
 }
 *ready=c->state==READY;
end:p->busy=0;return result;
}
static struct pt_paula_readers_view view_value(struct pt_paula_readers_reader *r)
{return (struct pt_paula_readers_view){r->data,r->bytes,r->pcm.frames,r->track,r->sample,r->channel,r->slot,r->token};}
enum pt_paula_readers_result pt_paula_readers_view(struct pt_paula_readers_command *c,unsigned index,struct pt_paula_readers_view *out)
{
 struct pt_paula_readers_pool *p;struct pt_paula_readers_reader *r;
 if(!c||!(p=c->pool)||reentry(p)||!command_registered(p,c)||!out||!output_disjoint(p,out,sizeof(*out))||index>=c->count||
    (c->state!=READY&&c->state!=LIVE))return PT_PAULA_READERS_INVALID;
 if(c->source[index].request.kind!=PT_SCHEDULED_TRIGGER)return PT_PAULA_READERS_INVALID;
 r=trigger_reader(c,index);if(!r||!current_pool(p)||!source_current(r))return PT_PAULA_READERS_STALE;
 *out=view_value(r);return PT_PAULA_READERS_OK;
}
enum pt_scheduled_result pt_paula_readers_enqueue(struct pt_paula_readers_command *c,const struct pt_scheduled_batch *b,uint64_t *out)
{
 struct pt_paula_readers_pool *p;struct pt_scheduled_batch candidate;struct pt_readers_key targets[PT_PAULA_READERS_ACTIONS];
 struct pt_readers_owner owners[PT_PAULA_READERS_ACTIONS];struct pt_readers_control command;enum pt_scheduled_result result;unsigned i,j;
 if(!c||!(p=c->pool)||reentry(p)||!command_registered(p,c)||c->state!=READY||!b||!out||!output_disjoint(p,out,sizeof(*out))||
    !output_disjoint(p,b,sizeof(*b))||!disjoint(out,sizeof(*out),b,sizeof(*b)))return PT_SCHEDULED_INVALID;
 if(!current_pool(p))return PT_SCHEDULED_STALE;
 candidate=*b;p->busy=1;
 if(!sources_current(c)){p->busy=0;return PT_SCHEDULED_STALE;}
 if(!batch_valid(c,&candidate)){p->busy=0;return PT_SCHEDULED_INVALID;}
 memset(targets,0,sizeof(targets));memset(owners,0,sizeof(owners));
 for(i=0;i<candidate.count;++i) {
   for(j=0;j<c->count;++j)if((unsigned)p->map[c->source[j].request.track]==candidate.action[i].slot)break;
   if(candidate.action[i].kind==PT_SCHEDULED_TRIGGER) {
     struct pt_paula_readers_reader *r=trigger_reader(c,j);
     owners[i]=(struct pt_readers_owner){{r,sizeof(*r),r->token,reader_current,reader_retired,reader_terminal},r->spans,r->span_count};
   }else targets[i]=c->source[j].request.key;
 }
 command=(struct pt_readers_control){c,sizeof(*c),c->token,command_current,command_retired,command_terminal};
 p->callback=c;p->callback_batch=&candidate;p->queue_call=1;
 result=pt_readers_enqueue(c->queue,&candidate,targets,&command,owners,out);
 p->queue_call=0;p->callback=NULL;p->callback_batch=NULL;
 if(result==PT_SCHEDULED_OK) {
   c->batch=candidate;c->state=LIVE;c->ticket=*out;
   for(i=0;i<candidate.count;++i)if(candidate.action[i].kind==PT_SCHEDULED_TRIGGER) {
     for(j=0;j<c->count;++j)if((unsigned)p->map[c->source[j].request.track]==candidate.action[i].slot)break;
     c->source[j].reader->state=LIVE;c->source[j].reader->trigger=*out;c->source[j].reader->action=i;
   }
 }
 p->busy=0;return result;
}
enum pt_scheduled_result pt_paula_readers_lower_enqueue(struct pt_paula_readers_command *c,uint64_t *out)
{
 struct pt_scheduled_batch b;struct pt_paula_readers_pool *p;unsigned i;
 if(!c||!(p=c->pool)||reentry(p)||!command_registered(p,c)||c->state!=READY||!c->lowered||
    !out||!output_disjoint(p,out,sizeof(*out)))return PT_SCHEDULED_INVALID;
 if(!current_pool(p))return PT_SCHEDULED_STALE;
 memset(&b,0,sizeof(b));b.frame=c->frame;b.generation=p->config.generation;b.count=c->count;
 for(i=0;i<c->count;++i) {
   struct command_source *s=c->source+i;struct pt_scheduled_action *a=b.action+i;
   const struct pt_paula_render_plan *v=c->lower+i;
   a->kind=s->request.kind;a->slot=(unsigned)p->map[s->request.track];a->period=v->period;a->volume=v->volume;
   if(a->kind==PT_SCHEDULED_TRIGGER) {
     struct pt_paula_readers_reader *r=trigger_reader(c,i);
     if(!r||!source_current(r)||v->offset>r->bytes||v->length>r->bytes-v->offset)return PT_SCHEDULED_STALE;
     a->data=r->data+v->offset;a->words=v->length/2;
   }
 }
 return pt_paula_readers_enqueue(c,&b,out);
}
enum pt_paula_readers_result pt_paula_readers_reader(struct pt_paula_readers_command *c,unsigned index,struct pt_paula_readers_reader **out)
{
 struct pt_paula_readers_pool *p;struct pt_paula_readers_reader *r;
 if(!c||!(p=c->pool)||reentry(p)||!command_registered(p,c)||!out||!output_disjoint(p,out,sizeof(*out))||
    index>=c->count||(c->state!=LIVE&&c->state!=RETIRED)||c->source[index].request.kind!=PT_SCHEDULED_TRIGGER)return PT_PAULA_READERS_INVALID;
 r=trigger_reader(c,index);if(!r)return PT_PAULA_READERS_STALE;
 *out=r;return PT_PAULA_READERS_OK;
}
enum pt_paula_readers_result pt_paula_readers_reader_view(struct pt_paula_readers_reader *r,struct pt_paula_readers_view *out)
{
 struct pt_paula_readers_pool *p;
 if(!r||!(p=r->pool)||reentry(p)||!reader_registered(p,r)||r->state!=LIVE||!out||!output_disjoint(p,out,sizeof(*out)))return PT_PAULA_READERS_INVALID;
 if(!current_pool(p)||!source_current(r))return PT_PAULA_READERS_STALE;
 *out=view_value(r);return PT_PAULA_READERS_OK;
}
enum pt_scheduled_result pt_paula_readers_reader_key(struct pt_paula_readers_reader *r,struct pt_readers_key *out)
{
 struct pt_paula_readers_pool *p;enum pt_scheduled_result result;
 if(!r||!(p=r->pool)||reentry(p)||!reader_registered(p,r)||r->state!=LIVE||!out||!output_disjoint(p,out,sizeof(*out)))return PT_SCHEDULED_INVALID;
 if(!current_pool(p))return PT_SCHEDULED_STALE;
 p->busy=1;p->queue_call=1;result=pt_readers_reader_key(r->queue,r->trigger,r->action,out);p->queue_call=0;
 if(result==PT_SCHEDULED_OK){r->key=*out;r->key_seen=1;}
 p->busy=0;return result;
}
enum pt_paula_readers_result pt_paula_readers_cancel(struct pt_paula_readers_command *c)
{
 struct pt_paula_readers_pool *p;
 if(!c||!(p=c->pool)||reentry(p)||!command_registered(p,c))return PT_PAULA_READERS_INVALID;
 if(c->state==LIVE)return PT_PAULA_READERS_BUSY;
 p->busy=1;partial_resources(c);c->state=CANCELLED;p->busy=0;return PT_PAULA_READERS_OK;
}
int pt_paula_readers_command_close(struct pt_paula_readers_command *c)
{
 struct pt_paula_readers_pool *p;unsigned i;
 if(!c||!(p=c->pool)||reentry(p)||!command_registered(p,c)||c->state==LIVE)return 0;
 p->busy=1;partial_resources(c);for(i=0;i<p->config.maximum_commands;++i)if(p->commands[i]==c)p->commands[i]=NULL;
 p->bytes-=sizeof(*c);p->allocator.release(p->allocator.context,c);p->busy=0;return 1;
}
int pt_paula_readers_reader_close(struct pt_paula_readers_reader *r)
{
 struct pt_paula_readers_pool *p;
 if(!r||!(p=r->pool)||reentry(p)||!reader_registered(p,r)||r->state!=RETIRED)return 0;
 p->busy=1;dispose_reader(r);p->busy=0;return 1;
}
int pt_paula_readers_close(struct pt_paula_readers_pool *p)
{
 unsigned i;struct pt_allocator a;
 if(!p||reentry(p))return 0;
 for(i=0;i<p->config.maximum_commands;++i)if(p->commands[i])return 0;
 for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i])return 0;
 p->busy=1;p->closing=1;if(!pt_sampler_paula_close(&p->bridge)){p->busy=0;return 0;}
 a=p->allocator;a.release(a.context,p);return 1;
}

/* Initial setup is deliberately separate from the legacy synchronous entry.
 * This private bridge constructor is reachable only after this workspace's
 * actual checked validation completion; it is not a public trusted flag. */
static void preparation_bridge(struct pt_sampler_paula *b,struct pt_sampler *s,
    struct pt_project *p,const struct pt_paula_readers_config *c)
{
    unsigned i;
    b->sampler=s;b->project=p;b->version=1;
    pt_cache_init(&b->cache,c->chip_context,c->chip_allocate,c->chip_release,c->chip_budget);
    b->table=p->samples;b->count=p->sample_count;b->generation=s->generation;b->channels=p->channels.count;
    for(i=0;i<PT_CHANNEL_LIMIT;++i)b->routes[i]=p->channels.track[i].route;
}
static enum pt_paula_readers_result preparation_result(enum pt_project_result r)
{
    if(r==PT_PROJECT_OK)return PT_PAULA_READERS_OK;
    if(r==PT_PROJECT_PENDING)return PT_PAULA_READERS_PENDING;
    if(r==PT_PROJECT_STALE)return PT_PAULA_READERS_STALE;
    return PT_PAULA_READERS_INVALID;
}
/* Fixed control checks and validator current/header checks precede all borrowed
 * descriptor/span walks, including after arrays have been replaced/freed. */
static enum pt_paula_readers_result preparation_current(const struct pt_paula_readers_preparation *j,
    uint32_t revision)
{
    struct pt_sampler *s=j->sampler;
    if(j->failed)return PT_PAULA_READERS_INVALID;
    if(!s || s->generation!=j->generation || s->table!=j->table ||
       s->table_original!=j->table_original || s->table_bytes!=j->table_bytes ||
       s->allocator.context!=j->sampler_allocator.context ||
       s->allocator.allocate!=j->sampler_allocator.allocate || s->allocator.release!=j->sampler_allocator.release)
        return PT_PAULA_READERS_STALE;
    return preparation_result(pt_project_validation_get(&j->validation,revision,s->generation,NULL));
}
static int preparation_apart(const struct pt_paula_readers_preparation *j,const void *out,size_t n)
{
    return disjoint(out,n,j,sizeof(*j)) &&
        disjoint(out,n,j->allocator_source,sizeof(*j->allocator_source)) &&
        disjoint(out,n,j->config_source,sizeof(*j->config_source)) &&
        pt_sampler_output_disjoint(j->sampler,out,n) && project_disjoint(j->validation.project,out,n);
}
static int preparation_reentry(struct pt_paula_readers_preparation *j)
{
    if(j->busy){j->failed=1;return 1;}return 0;
}
enum pt_paula_readers_result pt_paula_readers_prepare_begin(struct pt_paula_readers_preparation *j,
    const struct pt_allocator *a,struct pt_sampler *s,struct pt_project *p,
    const struct pt_paula_readers_config *c,uint32_t revision)
{
    struct pt_paula_readers_preparation next;enum pt_project_result r;
    if(!valid_span(j,sizeof(*j)) || !valid_span(a,sizeof(*a)) ||
       !valid_span(s,sizeof(*s)) || !valid_span(p,sizeof(*p)) || !valid_span(c,sizeof(*c)))
        return PT_PAULA_READERS_INVALID;
    /* Reject known source aliases before inspecting workspace contents. */
    if(!disjoint(j,sizeof(*j),a,sizeof(*a)) || !disjoint(j,sizeof(*j),c,sizeof(*c)) ||
       !pt_sampler_output_disjoint(s,j,sizeof(*j)))return PT_PAULA_READERS_INVALID;
    /* The validator first checks source geometry/full extents. A staged local
     * begin can be moved; it retains no pointer to its own workspace. */
    memset(&next,0,sizeof(next));
    r=pt_project_validation_begin(&next.validation,p,revision,s->generation);
    if(r!=PT_PROJECT_OK)return preparation_result(r);
    if(!project_disjoint(p,j,sizeof(*j)))return PT_PAULA_READERS_INVALID;
    if(preparation_reentry(j))return PT_PAULA_READERS_BUSY;
    if(j->state)return PT_PAULA_READERS_BUSY;
    if(!a->allocate || !a->release || !s->allocator.allocate || !s->allocator.release ||
       !c->chip_allocate || !c->chip_release || !c->generation ||
       !c->maximum_commands || c->maximum_commands>PT_PAULA_READERS_COMMANDS ||
       !c->maximum_readers || c->maximum_readers>PT_PAULA_READERS_PERSISTENT)
        return PT_PAULA_READERS_INVALID;
    next.allocator=*a;next.sampler_allocator=s->allocator;next.config=*c;
    next.allocator_source=a;next.config_source=c;
    next.sampler=s;next.generation=s->generation;next.table=s->table;
    next.table_original=s->table_original;next.table_bytes=s->table_bytes;next.state=1;
    memcpy(j,&next,sizeof(next));return PT_PAULA_READERS_OK;
}
enum pt_paula_readers_result pt_paula_readers_prepare_step(struct pt_paula_readers_preparation *j,
    uint32_t revision,unsigned work)
{
    enum pt_paula_readers_result r;
    if(!valid_span(j,sizeof(*j)))return PT_PAULA_READERS_INVALID;
    if(preparation_reentry(j))return PT_PAULA_READERS_BUSY;
    if(!j->state || !work || work>PT_PROJECT_VALIDATION_WORK_MAX)return PT_PAULA_READERS_INVALID;
    r=preparation_current(j,revision);
    if(r!=PT_PAULA_READERS_PENDING && r!=PT_PAULA_READERS_OK)return r;
    if(r==PT_PAULA_READERS_OK)return r;
    r=preparation_result(pt_project_validation_step(&j->validation,revision,j->generation,work));
    if(r==PT_PAULA_READERS_OK)j->state=2;
    return r;
}
enum pt_paula_readers_result pt_paula_readers_prepare_transfer(struct pt_paula_readers_preparation *j,
    uint32_t revision,struct pt_paula_readers_pool **out)
{
    struct pt_paula_readers_pool *p;enum pt_paula_readers_result r;int8_t map[PT_CHANNEL_LIMIT];unsigned i;
    if(!valid_span(j,sizeof(*j)) || !valid_span(out,sizeof(*out)))return PT_PAULA_READERS_INVALID;
    if(!disjoint(out,sizeof(*out),j,sizeof(*j)))return PT_PAULA_READERS_INVALID;
    if(preparation_reentry(j))return PT_PAULA_READERS_BUSY;
    if(!j->state)return PT_PAULA_READERS_INVALID;
    r=preparation_current(j,revision);
    if(r!=PT_PAULA_READERS_OK)return r;
    if(!preparation_apart(j,out,sizeof(*out)))return PT_PAULA_READERS_INVALID;
    if(j->config.control_budget<sizeof(*p))return PT_PAULA_READERS_CAPACITY;
    if(pt_channels_paula_map(&j->validation.project->channels,NULL,map)!=PT_CHANNEL_OK)
        return PT_PAULA_READERS_INVALID;
    j->busy=1;
    p=j->allocator.allocate(j->allocator.context,sizeof(*p));
    /* No old table is traversed until the original fixed identity is checked
     * again after the callback. Do not rebase onto callback-mutated inputs. */
    r=preparation_current(j,revision);
    if(!p){j->busy=0;return r==PT_PAULA_READERS_OK?PT_PAULA_READERS_CAPACITY:r;}
    if(r!=PT_PAULA_READERS_OK || !preparation_apart(j,out,sizeof(*out)) ||
       !preparation_apart(j,p,sizeof(*p)) || !disjoint(p,sizeof(*p),out,sizeof(*out))) {
        j->allocator.release(j->allocator.context,p);j->busy=0;
        return j->failed?PT_PAULA_READERS_INVALID:(r==PT_PAULA_READERS_OK?PT_PAULA_READERS_INVALID:r);
    }
    memset(p,0,sizeof(*p));p->allocator=j->allocator;p->config=j->config;
    p->sampler=j->sampler;p->project=(struct pt_project *)j->validation.project;
    memcpy(&p->header,p->project,sizeof(p->header));
    if(p->project->sample_count)
        memcpy(p->original,p->project->samples,p->project->sample_count*sizeof(*p->original));
    for(i=0;i<p->project->sample_count;++i)p->original_external[i]=j->sampler->current[i]==NULL;
    p->generation=j->generation;p->bytes=sizeof(*p);memcpy(p->map,map,sizeof(map));
    preparation_bridge(&p->bridge,j->sampler,p->project,&j->config);
    *out=p;memset(j,0,sizeof(*j));return PT_PAULA_READERS_OK;
}
enum pt_paula_readers_result pt_paula_readers_prepare_cancel(struct pt_paula_readers_preparation *j)
{
    if(!valid_span(j,sizeof(*j)))return PT_PAULA_READERS_INVALID;
    if(preparation_reentry(j))return PT_PAULA_READERS_BUSY;
    memset(j,0,sizeof(*j));return PT_PAULA_READERS_OK;
}
