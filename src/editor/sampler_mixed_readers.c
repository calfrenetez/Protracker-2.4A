#include "../core/amigus_trigger_levels.h"
#include "sampler_mixed_readers.h"
#include "sampler_mixed_readers_internal.h"
#include "sampler_mixed_causal_internal.h"
#include "sampler_mixed_causal_stop_internal.h"
#include "sampler_mixed_causal_lineage_internal.h"
#include "../core/mixed_readers_causal_lineage_internal.h"
#include "../core/mixed_readers_causal_stop_internal.h"
#include "../core/mixed_readers_causal_factory_internal.h"
#include "../core/mixed_scheduled_readers_internal.h"
#include "sampler_internal.h"
#include "sampler_paula_internal.h"
#include "sampler_wavetable_internal.h"
#include "project_snapshot.h"
#include <string.h>
#include <limits.h>
#define GUARDS 8192U
enum {VALIDATING=1,BINDING,OPEN,PREPARING,READY,LIVE,RETIRED,CANCELLED,FAILED};
enum {PIN=0,DERIVED_BEGIN,DERIVED_STEP,ACTION};
struct ledger {void *data;
size_t bytes;
};
struct source {struct pt_sampler_mixed_request request;
struct pt_sampler_mixed_trigger_levels levels;
struct pt_sampler_mixed_reader *reader;
uint64_t token;
};
struct pt_sampler_mixed_command {
 struct pt_sampler_mixed_pool *pool;
uint64_t token,frame,ticket;
unsigned state,count,index,phase,terminal,valid,causal_stop,causal_control;
 struct source source[PT_SAMPLER_MIXED_ACTIONS];
struct pt_mixed_readers_inputs inputs;
};
struct pt_sampler_mixed_reader {
 struct pt_sampler_mixed_pool *pool;
uint64_t token,ticket,frame;
unsigned action,state,route,slot,track,sample,channel,leased,released,key_seen,terminal,valid,arena_index;
 struct pt_sample_version *pin;
struct pt_pcm pcm;
struct pt_sample original;
struct pt_cache_lease lease;
 struct pt_sampler_paula_job chip;
struct pt_sampler_upload_job card;
uint8_t staging[PT_PLAYBACK_UPLOAD_CHUNK];
 const uint8_t *data;
size_t bytes;
struct pt_playback_format format;
struct pt_mixed_readers_card resource;
 struct pt_mixed_readers_key key;
struct pt_mixed_readers_span spans[PT_MIXED_READERS_SPANS];
unsigned span_count;
};
struct pt_sampler_mixed_pool {
 struct pt_sampler_mixed_config config;
struct pt_project header;
uint32_t revision;
unsigned generation,state,busy,failed,closing,queue_call;
 struct pt_project_validation validation;
struct pt_sampler_paula paula;
struct pt_sampler_wavetable wave;
 struct pt_amigus_wavetable_cache backend_header;
struct pt_amigus_reservation reservation_header;
 struct pt_sample *table,*table_original;
size_t table_bytes;
struct pt_allocator sampler_allocator;
 struct pt_sample_version *masters[PT_PROJECT_SAMPLES];
struct pt_sample samples[PT_PROJECT_SAMPLES];
 struct pt_mixed_readers_span guards[GUARDS];
unsigned guard_count;
int8_t slot[PT_CHANNEL_LIMIT];
 struct pt_sampler_mixed_command *commands[PT_SAMPLER_MIXED_COMMANDS];
struct pt_sampler_mixed_reader *readers[PT_SAMPLER_MIXED_READERS];
 struct ledger chip[PT_CACHE_SLOTS];
size_t bytes,chip_bytes;
uint64_t serial;
 struct pt_sampler_mixed_request scratch[PT_SAMPLER_MIXED_ACTIONS];
struct pt_sampler_mixed_trigger_levels level_scratch[PT_SAMPLER_MIXED_ACTIONS];
unsigned *release_fault;
 struct pt_sampler_mixed_causal_binding *causal_binding;
 struct pt_sampler_mixed_causal_binding causal_original;
 unsigned causal_phase;uint64_t causal_first,causal_successor;
 struct pt_sampler_mixed_causal_stop_binding *stop_binding;
 struct pt_sampler_mixed_causal_stop_binding stop_original;
 struct pt_sampler_mixed_causal_lineage_binding *lineage_binding;
 struct pt_sampler_mixed_causal_lineage_binding lineage_original;
 unsigned lineage_stage,lineage_enqueue_attempt,lineage_root_detached,lineage_root_closed;
 uint64_t lineage_root_token,lineage_control,lineage_control_frame,lineage_stop;
};
struct workspace {struct pt_sampler_mixed_pool pool;
struct pt_sampler_mixed_config original;
unsigned busy,failed;
};
static int span(const void *a,size_t n){return !n||(a&&(uintptr_t)a<=UINTPTR_MAX-n);
}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));
}
static int add(struct pt_sampler_mixed_pool *p,const void *data,size_t bytes)
{if(!span(data,bytes)||p->guard_count>=GUARDS)return 0;
if(bytes){p->guards[p->guard_count].data=data;
p->guards[p->guard_count++].bytes=bytes;
}
return 1;
}
static int sample_apart(const struct pt_sample *s,const void *o,size_t n)
{return s->pcm.capacity<=SIZE_MAX/sizeof(int32_t)&&apart(o,n,s->pcm.data,s->pcm.capacity*sizeof(int32_t))&&apart(o,n,s->slices,(size_t)s->slice_count*sizeof(uint32_t));
}
static int initial_apart(const struct pt_sampler_mixed_config *c,const void *o,size_t n)
{
 const struct pt_project *p=c->project;
unsigned i;
size_t events;
 if(!span(o,n)||!p||!c->sampler||!c->queue||!c->backend||!c->backend->reservation||p->sample_count>PT_PROJECT_SAMPLES||p->channels.count>PT_CHANNEL_LIMIT||p->pattern_count>PT_PROJECT_PATTERNS||p->order_count>PT_PROJECT_ORDERS||p->extension_count>4090)return 0;
 if(!apart(o,n,p,sizeof(*p))||!apart(o,n,c->sampler,sizeof(*c->sampler))||!apart(o,n,c->queue,pt_mixed_readers_control_size())||!apart(o,n,c->backend,sizeof(*c->backend))||!apart(o,n,c->backend->reservation,sizeof(*c->backend->reservation)))return 0;
 events=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
 if(!apart(o,n,p->samples,(size_t)p->sample_count*sizeof(*p->samples))||!apart(o,n,p->orders,(size_t)p->order_count*sizeof(*p->orders))||!apart(o,n,p->events,events*sizeof(*p->events))||!apart(o,n,p->extensions,(size_t)p->extension_count*sizeof(*p->extensions)))return 0;
 if(!apart(o,n,c->sampler->table,c->sampler->table_bytes)||!apart(o,n,c->sampler->table_original,c->sampler->table_original?(size_t)p->sample_count*sizeof(*p->samples):0))return 0;
 for(i=0;i<c->context_count;++i)if(!apart(o,n,c->contexts[i].data,c->contexts[i].bytes))return 0;
 for(i=0;i<p->sample_count;++i)if(!sample_apart(p->samples+i,o,n))return 0;
 for(i=0;i<p->extension_count;++i)if(!apart(o,n,p->extensions[i].data,p->extensions[i].length))return 0;
 for(i=0;i<PT_PROJECT_SAMPLES;++i)if(!pt_sampler_version_output_disjoint(c->sampler->current[i],o,n))return 0;
 return 1;
}
static int output_apart(struct pt_sampler_mixed_pool *p,const void *o,size_t n)
{
 unsigned i;
if(!n||!span(o,n)||!apart(o,n,p,sizeof(*p))||!pt_sampler_output_disjoint(p->config.sampler,o,n))return 0;
 for(i=0;i<p->guard_count;++i)if(!apart(o,n,p->guards[i].data,p->guards[i].bytes))return 0;
 for(i=0;i<p->config.maximum_commands;++i)if(p->commands[i]&&!apart(o,n,p->commands[i],sizeof(*p->commands[i])))return 0;
 for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i]&&!apart(o,n,p->readers[i],sizeof(*p->readers[i])))return 0;
 for(i=0;i<PT_CACHE_SLOTS;++i)if(!apart(o,n,p->chip[i].data,p->chip[i].bytes))return 0;
 return 1;
}
/* Numeric scratch address: the guard reads no uninitialized child bytes. */
static int output_address_apart(struct pt_sampler_mixed_pool *p,uintptr_t address,size_t bytes)
{return output_apart(p,(const void *)address,bytes);
}
static int context_covered(const struct pt_sampler_mixed_config *c,const void *v)
{unsigned i;
uintptr_t x=(uintptr_t)v;
if(!v)return 1;
for(i=0;i<c->context_count;++i){uintptr_t y=(uintptr_t)c->contexts[i].data;
if(span(c->contexts[i].data,c->contexts[i].bytes)&&x>=y&&x-y<c->contexts[i].bytes)return 1;
}
return 0;
}
enum {CB_BOUND=1,CB_OPENING,CB_LIVE,CB_REFUSED,CB_CONSUMED};
static int causal_inside(struct pt_mixed_readers_span s,const void *v,size_t n)
{return n&&span(s.data,s.bytes)&&span(v,n)&&s.bytes>=n&&
 (uintptr_t)v>=(uintptr_t)s.data&&(uintptr_t)v-(uintptr_t)s.data<=s.bytes-n;}
static int causal_fields_same(const struct pt_sampler_mixed_causal_binding *a,
 const struct pt_sampler_mixed_causal_binding *b)
{return a->self==b->self&&a->owner==b->owner&&a->queue==b->queue&&
 a->session==b->session&&a->generation==b->generation&&
 a->contexts.data==b->contexts.data&&a->contexts.bytes==b->contexts.bytes;}
static int causal_current(const struct pt_sampler_mixed_pool *p)
{return !p->causal_binding||(p->causal_binding->self==p->causal_binding&&
 causal_fields_same(p->causal_binding,&p->causal_original)&&
 p->causal_binding->phase==p->causal_phase&&
 (!p->stop_binding||(p->stop_binding->self==p->stop_binding&&
 p->stop_original.self==p->stop_binding&&
 &p->stop_binding->original==p->causal_binding))&&
 (!p->lineage_binding||(p->lineage_binding->self==p->lineage_binding&&
 p->lineage_original.self==p->lineage_binding&&
 &p->lineage_binding->original==p->causal_binding)));}
int pt_sampler_mixed_causal_bind(struct pt_sampler_mixed_causal_binding *b,
 struct pt_mixed_causal_owner *owner,struct pt_mixed_readers_output *queue,
 uint64_t session,uint64_t generation,struct pt_mixed_readers_span contexts)
{
 const unsigned char *v;size_t i;
 if(!b||!owner||!queue||!session||!generation||
    (uintptr_t)b%_Alignof(struct pt_sampler_mixed_causal_binding)||
    !span(b,sizeof(*b))||!causal_inside(contexts,b,sizeof(*b))||
    !apart(contexts.data,contexts.bytes,owner,pt_mixed_causal_control_size())||
    !apart(contexts.data,contexts.bytes,queue,pt_mixed_readers_control_size()))return 0;
 v=(const unsigned char *)b;
 for(i=0;i<sizeof(*b);++i)if(v[i])return 0;
 if(!pt_mixed_causal_factory_original_empty(owner,queue,session,generation))return 0;
 b->self=b;b->owner=owner;b->queue=queue;b->session=session;b->generation=generation;
 b->contexts=contexts;b->phase=CB_BOUND;return 1;
}
int pt_sampler_mixed_causal_stop_bind(struct pt_sampler_mixed_causal_stop_binding *b,
 struct pt_mixed_causal_owner *owner,struct pt_mixed_readers_output *queue,
 uint64_t session,uint64_t generation,struct pt_mixed_readers_span contexts)
{
 const unsigned char *v;size_t i;
 if(!b||!owner||!queue||!session||!generation||
    (uintptr_t)b%_Alignof(struct pt_sampler_mixed_causal_stop_binding)||
    !span(b,sizeof(*b))||!causal_inside(contexts,b,sizeof(*b))||
    !apart(contexts.data,contexts.bytes,owner,pt_mixed_causal_control_size())||
    !apart(contexts.data,contexts.bytes,queue,pt_mixed_readers_control_size()))return 0;
 v=(const unsigned char *)b;for(i=0;i<sizeof(*b);++i)if(v[i])return 0;
 if(!pt_mixed_causal_factory_stop_original_empty(owner,queue,session,generation))return 0;
 for(i=0;i<sizeof(*b);++i)if(v[i])return 0;
 b->original.self=&b->original;b->original.owner=owner;b->original.queue=queue;
 b->original.session=session;b->original.generation=generation;
 b->original.contexts=contexts;b->original.phase=CB_BOUND;b->self=b;return 1;
}
int pt_sampler_mixed_causal_lineage_bind(struct pt_sampler_mixed_causal_lineage_binding *b,
 struct pt_mixed_causal_owner *owner,struct pt_mixed_readers_output *queue,
 uint64_t session,uint64_t generation,struct pt_mixed_readers_span contexts)
{
 const unsigned char *v;size_t i;
 if(!b||!owner||!queue||!session||!generation||
    (uintptr_t)b%_Alignof(struct pt_sampler_mixed_causal_lineage_binding)||
    !span(b,sizeof(*b))||!causal_inside(contexts,b,sizeof(*b))||
    !apart(contexts.data,contexts.bytes,owner,pt_mixed_causal_control_size())||
    !apart(contexts.data,contexts.bytes,queue,pt_mixed_readers_control_size()))return 0;
 v=(const unsigned char *)b;for(i=0;i<sizeof(*b);++i)if(v[i])return 0;
 if(!pt_mixed_causal_factory_control_stop_original_empty(owner,queue,session,generation))return 0;
 for(i=0;i<sizeof(*b);++i)if(v[i])return 0;
 b->original.self=&b->original;b->original.owner=owner;b->original.queue=queue;
 b->original.session=session;b->original.generation=generation;
 b->original.contexts=contexts;b->original.phase=CB_BOUND;b->self=b;return 1;
}
static int causal_admit(const struct pt_sampler_mixed_config *c,
 struct pt_sampler_mixed_causal_binding *b,struct pt_sampler_mixed_causal_stop_binding *stop,
 struct pt_sampler_mixed_causal_lineage_binding *lineage)
{
 unsigned i,covered=0,owner_covered=0;
 if(stop&&lineage)return 0;
 if(lineage){
  if((uintptr_t)lineage%_Alignof(struct pt_sampler_mixed_causal_lineage_binding))return 0;
  for(i=0;i<c->context_count;++i)if(causal_inside(c->contexts[i],lineage,sizeof(*lineage)))covered=1;
  if(!covered||&lineage->original!=b||lineage->self!=lineage)return 0;
  covered=0;
 }
 if(stop){
  if((uintptr_t)stop%_Alignof(struct pt_sampler_mixed_causal_stop_binding))return 0;
  for(i=0;i<c->context_count;++i)if(causal_inside(c->contexts[i],stop,sizeof(*stop)))covered=1;
  if(!covered||&stop->original!=b||stop->self!=stop)return 0;
  covered=0;
 }
 if(!b)return 1;
 /* Complete context coverage precedes its first field read. Contexts include
  * original whole parent capacities; no narrower binding-slot exemption. */
 for(i=0;i<c->context_count;++i)if(causal_inside(c->contexts[i],b,sizeof(*b)))covered=1;
 if(!covered||b->self!=b||b->phase!=CB_BOUND||!b->owner||b->queue!=c->queue||
    !b->session||b->generation!=c->generation||
    !causal_inside(b->contexts,b,sizeof(*b)))return 0;
 covered=0;
 for(i=0;i<c->context_count;++i)if(c->contexts[i].data==b->contexts.data&&
    c->contexts[i].bytes==b->contexts.bytes)covered=1;
 /* The private factory must protect the complete genuine owner too, even for
  * a direct caller outside the facade. Existing initial_apart/capture walk all
  * these full spans, before workspace/output writes or allocator callbacks. */
 for(i=0;i<c->context_count;++i)if(causal_inside(c->contexts[i],b->owner,
    pt_mixed_causal_control_size()))owner_covered=1;
 return covered&&owner_covered&&(lineage?
 pt_mixed_causal_factory_control_stop_original_empty(b->owner,b->queue,b->session,b->generation):stop?
 pt_mixed_causal_factory_stop_original_empty(b->owner,b->queue,b->session,b->generation):
 pt_mixed_causal_factory_original_empty(b->owner,b->queue,b->session,b->generation));
}
static void causal_refuse(struct pt_sampler_mixed_causal_binding *b)
{if(b)b->phase=CB_REFUSED;}
static int fixed_current(struct pt_sampler_mixed_pool *p,uint32_t revision)
{
 struct pt_project h;
struct pt_sampler *s=p->config.sampler;
unsigned i;
 if(!causal_current(p)||p->failed||p->closing||revision!=p->revision||s->generation!=p->generation||s->table!=p->table||s->table_original!=p->table_original||s->table_bytes!=p->table_bytes||s->allocator.context!=p->sampler_allocator.context||s->allocator.allocate!=p->sampler_allocator.allocate||s->allocator.release!=p->sampler_allocator.release)return 0;
 memcpy(&h,&p->header,sizeof(h));
h.channels.selected=p->config.project->channels.selected;
 if(!pt_project_snapshot_equal(p->config.project,&h))return 0;
 for(i=0;i<p->header.sample_count;++i)if(s->current[i]!=p->masters[i])return 0;
 return 1;
}
static int backend_fixed(struct pt_sampler_mixed_pool *p)
{
 struct pt_amigus_wavetable_cache *b=p->config.backend,*h=&p->backend_header;
struct pt_amigus_reservation *r;
 if(b->reservation!=h->reservation||b->closing||b->faulted||b->context!=h->context||b->owned!=h->owned||b->write32!=h->write32||b->cache.context!=h->cache.context||b->cache.allocate!=h->cache.allocate||b->cache.release!=h->cache.release||b->cache.budget!=h->cache.budget||b->arena.base!=h->arena.base||b->arena.capacity!=h->arena.capacity||b->arena.context!=h->arena.context||b->arena.owned!=h->arena.owned||b->arena.write32!=h->arena.write32)return 0;
 r=b->reservation;
return r&&memcmp(r,&p->reservation_header,sizeof(*r))==0;
}
static int current(struct pt_sampler_mixed_pool *p,uint32_t revision,unsigned owned)
{if(!fixed_current(p,revision)||!backend_fixed(p))return 0;
if(owned&&!pt_amigus_wavetable_cache_current(p->config.backend))return 0;
return fixed_current(p,revision)&&backend_fixed(p);
}
static int reentry(struct pt_sampler_mixed_pool *p)
{if(p->busy){p->failed=1;
if(p->release_fault)*p->release_fault=1;
return 1;
}
return 0;
}
static int capture(struct pt_sampler_mixed_pool *p)
{
 struct pt_project *x=p->config.project;
struct pt_sampler *s=p->config.sampler;
unsigned i,j;
struct pt_sampler_storage_span v[PT_SAMPLER_VERSION_SPANS];
unsigned count;
 if(!add(p,x,sizeof(*x))||!add(p,s,sizeof(*s))||!add(p,p->config.queue,pt_mixed_readers_control_size())||!add(p,p->config.backend,sizeof(*p->config.backend))||!add(p,p->config.backend->reservation,sizeof(*p->config.backend->reservation))||!add(p,x->samples,(size_t)x->sample_count*sizeof(*x->samples))||!add(p,x->orders,(size_t)x->order_count*sizeof(*x->orders))||!add(p,x->events,(size_t)x->pattern_count*PT_PROJECT_ROWS*x->channels.count*sizeof(*x->events))||!add(p,x->extensions,(size_t)x->extension_count*sizeof(*x->extensions))||!add(p,s->table,s->table_bytes)||!add(p,s->table_original,s->table_original?(size_t)x->sample_count*sizeof(*x->samples):0))return 0;
 for(i=0;i<p->config.context_count;++i)if(!add(p,p->config.contexts[i].data,p->config.contexts[i].bytes))return 0;
 for(i=0;i<x->sample_count;++i){struct pt_sample *a=x->samples+i;
if(a->pcm.capacity>SIZE_MAX/sizeof(int32_t)||!add(p,a->pcm.data,a->pcm.capacity*sizeof(int32_t))||!add(p,a->slices,(size_t)a->slice_count*sizeof(uint32_t))||!s->current[i])return 0;
p->masters[i]=s->current[i];
memcpy(p->samples+i,a,sizeof(*a));
}
 for(i=0;i<x->extension_count;++i)if(!add(p,x->extensions[i].data,x->extensions[i].length))return 0;
 for(i=0;i<PT_PROJECT_SAMPLES;++i)if(s->current[i]){if(!pt_sampler_version_spans(s->current[i],v,PT_SAMPLER_VERSION_SPANS,&count))return 0;
for(j=0;j<count;++j)if(!add(p,v[j].data,v[j].bytes))return 0;
}
 return 1;
}
size_t pt_sampler_mixed_workspace_size(void){return sizeof(struct workspace);
}
size_t pt_sampler_mixed_workspace_alignment(void){return _Alignof(struct workspace);
}
size_t pt_sampler_mixed_pool_size(void){return sizeof(struct pt_sampler_mixed_pool);
}
size_t pt_sampler_mixed_command_size(void){return sizeof(struct pt_sampler_mixed_command);
}
size_t pt_sampler_mixed_reader_size(void){return sizeof(struct pt_sampler_mixed_reader);
}
static int config_valid(const struct pt_sampler_mixed_config *c)
{
 unsigned i;
struct pt_amigus_wavetable_cache *b;
 if(!c||!c->allocator.allocate||!c->allocator.release||!c->chip_allocate||!c->chip_release||!c->sampler||!c->sampler->allocator.allocate||!c->sampler->allocator.release||!c->project||!c->queue||!c->backend||!c->generation||!c->maximum_commands||c->maximum_commands>PT_SAMPLER_MIXED_COMMANDS||!c->maximum_readers||c->maximum_readers>PT_SAMPLER_MIXED_READERS||c->context_count>PT_SAMPLER_MIXED_CONTEXTS||c->control_budget<sizeof(struct pt_sampler_mixed_pool)||!c->chip_budget)return 0;
 if(pt_mixed_readers_commands_held(c->queue)||pt_mixed_readers_readers_held(c->queue))return 0;
 b=c->backend;
if(!b->reservation||!b->owned||!b->write32||b->closing||b->faulted||b->cache.bytes||!b->reservation->access||!b->reservation->reserved||b->reservation->resource!=PT_AMIGUS_WAVETABLE)return 0;
 for(i=0;i<PT_CACHE_SLOTS;++i)if(b->cache.entry[i].data||b->arena.block[i].reserved)return 0;
 for(i=0;i<c->context_count;++i)if(!c->contexts[i].bytes||!span(c->contexts[i].data,c->contexts[i].bytes))return 0;
 return context_covered(c,c->allocator.context)&&context_covered(c,c->sampler->allocator.context)&&context_covered(c,c->sampler->progress_context)&&context_covered(c,c->chip_context)&&context_covered(c,b->context)&&context_covered(c,b->arena.context)&&context_covered(c,b->reservation->api.context);
}
static enum pt_sampler_mixed_result mixed_open_private(const struct pt_sampler_mixed_config *c,uint32_t revision,void *storage,size_t capacity,struct pt_sampler_mixed_causal_binding *binding,struct pt_sampler_mixed_causal_stop_binding *stop,struct pt_sampler_mixed_causal_lineage_binding *lineage,struct pt_sampler_mixed_pool **out)
{
 struct workspace *w=storage;
struct pt_sampler_mixed_pool *p;
unsigned char *v;
size_t i;
unsigned ok;
 if(!config_valid(c)||!storage||capacity<sizeof(*w)||(uintptr_t)storage%_Alignof(struct workspace)||!span(storage,capacity)||!span(out,sizeof(*out))||!apart(storage,capacity,c,sizeof(*c))||!apart(storage,capacity,out,sizeof(*out))||!apart(out,sizeof(*out),c,sizeof(*c))||!initial_apart(c,storage,capacity)||!initial_apart(c,out,sizeof(*out))||!initial_apart(c,c,sizeof(*c))||!causal_admit(c,binding,stop,lineage))return PT_SAMPLER_MIXED_INVALID;
 if(w->busy){w->failed=1;
return PT_SAMPLER_MIXED_BUSY;
}
 v=storage;
for(i=0;i<sizeof(*w);++i)if(v[i])return PT_SAMPLER_MIXED_INVALID;
 if(binding){binding->phase=CB_OPENING;w->pool.causal_binding=binding;
 memcpy(&w->pool.causal_original,binding,sizeof(*binding));w->pool.causal_phase=CB_OPENING;}
 if(stop){w->pool.stop_binding=stop;memcpy(&w->pool.stop_original,stop,sizeof(*stop));}
 if(lineage){w->pool.lineage_binding=lineage;memcpy(&w->pool.lineage_original,lineage,sizeof(*lineage));}
 w->busy=1;
memcpy(&w->original,c,sizeof(*c));
memcpy(&w->pool.config,c,sizeof(*c));
memcpy(&w->pool.header,c->project,sizeof(*c->project));
memcpy(&w->pool.backend_header,c->backend,sizeof(*c->backend));
memcpy(&w->pool.reservation_header,c->backend->reservation,sizeof(*c->backend->reservation));
w->pool.revision=revision;
w->pool.generation=c->sampler->generation;
w->pool.table=c->sampler->table;
w->pool.table_original=c->sampler->table_original;
w->pool.table_bytes=c->sampler->table_bytes;
w->pool.sampler_allocator=c->sampler->allocator;
 if(!capture(&w->pool)){causal_refuse(binding);memset(w,0,sizeof(*w));
return PT_SAMPLER_MIXED_INVALID;
}
 p=c->allocator.allocate(c->allocator.context,sizeof(*p));
 ok=p&&output_apart(&w->pool,p,sizeof(*p))&&apart(p,sizeof(*p),w,capacity)&&apart(p,sizeof(*p),c,sizeof(*c))&&apart(p,sizeof(*p),out,sizeof(*out));
 if(!ok){causal_refuse(binding);memset(w,0,sizeof(*w));
return p?PT_SAMPLER_MIXED_INVALID:PT_SAMPLER_MIXED_CAPACITY;
}
 if((uintptr_t)p%_Alignof(struct pt_sampler_mixed_pool)||w->failed||memcmp(c,&w->original,sizeof(*c))||!fixed_current(&w->pool,revision)||!backend_fixed(&w->pool)){w->original.allocator.release(w->original.allocator.context,p);
causal_refuse(binding);memset(w,0,sizeof(*w));
return PT_SAMPLER_MIXED_STALE;
}
 memcpy(p,&w->pool,sizeof(*p));
p->bytes=sizeof(*p);
p->state=VALIDATING;
 if(pt_project_validation_begin(&p->validation,p->config.project,revision,p->generation)!=PT_PROJECT_OK){p->config.allocator.release(p->config.allocator.context,p);
causal_refuse(binding);memset(w,0,sizeof(*w));
return PT_SAMPLER_MIXED_INVALID;
}
 if(binding){binding->phase=CB_LIVE;p->causal_phase=CB_LIVE;}
 *out=p;
memset(w,0,sizeof(*w));
return PT_SAMPLER_MIXED_PENDING;
}
enum pt_sampler_mixed_result pt_sampler_mixed_open(const struct pt_sampler_mixed_config *c,
 uint32_t revision,void *storage,size_t capacity,struct pt_sampler_mixed_pool **out)
{return mixed_open_private(c,revision,storage,capacity,NULL,NULL,NULL,out);}
enum pt_sampler_mixed_result pt_sampler_mixed_causal_open(const struct pt_sampler_mixed_config *c,
 uint32_t revision,void *storage,size_t capacity,struct pt_sampler_mixed_causal_binding *binding,
 struct pt_sampler_mixed_pool **out)
{if(!binding)return PT_SAMPLER_MIXED_INVALID;
 return mixed_open_private(c,revision,storage,capacity,binding,NULL,NULL,out);}
enum pt_sampler_mixed_result pt_sampler_mixed_causal_stop_open(const struct pt_sampler_mixed_config *c,
 uint32_t revision,void *storage,size_t capacity,struct pt_sampler_mixed_causal_stop_binding *binding,
 struct pt_sampler_mixed_pool **out)
{if(!binding||(uintptr_t)binding%_Alignof(struct pt_sampler_mixed_causal_stop_binding)||
    !out||(uintptr_t)out%_Alignof(struct pt_sampler_mixed_pool *))return PT_SAMPLER_MIXED_INVALID;
 return mixed_open_private(c,revision,storage,capacity,&binding->original,binding,NULL,out);}
enum pt_sampler_mixed_result pt_sampler_mixed_causal_lineage_open(const struct pt_sampler_mixed_config *c,
 uint32_t revision,void *storage,size_t capacity,struct pt_sampler_mixed_causal_lineage_binding *binding,
 struct pt_sampler_mixed_pool **out)
{if(!binding||(uintptr_t)binding%_Alignof(struct pt_sampler_mixed_causal_lineage_binding)||
    !out||(uintptr_t)out%_Alignof(struct pt_sampler_mixed_pool *))return PT_SAMPLER_MIXED_INVALID;
 return mixed_open_private(c,revision,storage,capacity,&binding->original,NULL,binding,out);}
static void *chip_allocate(void *context,size_t bytes)
{
 struct pt_sampler_mixed_pool *p=context;
void *data;
unsigned i;
 if(p->failed||bytes>p->config.chip_budget-p->chip_bytes)return NULL;
 for(i=0;i<PT_CACHE_SLOTS&&p->chip[i].data;++i){}
if(i==PT_CACHE_SLOTS)return NULL;
 data=p->config.chip_allocate(p->config.chip_context,bytes);
 if(!data||!output_apart(p,data,bytes))return NULL;
 if(!fixed_current(p,p->revision)||!backend_fixed(p)){p->config.chip_release(p->config.chip_context,data,bytes);
return NULL;
}
 p->chip[i].data=data;
p->chip[i].bytes=bytes;
p->chip_bytes+=bytes;
return data;
}
static void chip_release(void *context,void *data,size_t bytes)
{
 struct pt_sampler_mixed_pool *p=context;
unsigned i;
for(i=0;i<PT_CACHE_SLOTS;++i)if(p->chip[i].data==data&&p->chip[i].bytes==bytes)break;
 if(i==PT_CACHE_SLOTS){p->failed=1;
return;
}
p->chip[i].data=NULL;
p->chip[i].bytes=0;
p->chip_bytes-=bytes;
p->config.chip_release(p->config.chip_context,data,bytes);
}
enum pt_sampler_mixed_result pt_sampler_mixed_advance_validation(struct pt_sampler_mixed_pool *p,uint32_t revision,unsigned work)
{
 enum pt_project_result r;
unsigned i,card=0;
 if(!p||!work||work>PT_PROJECT_VALIDATION_WORK_MAX||p->state>OPEN)return PT_SAMPLER_MIXED_INVALID;
 if(reentry(p))return PT_SAMPLER_MIXED_BUSY;
if(!fixed_current(p,revision)||!backend_fixed(p)){p->failed=1;
return PT_SAMPLER_MIXED_STALE;
}
 if(p->state==OPEN)return PT_SAMPLER_MIXED_OK;
 p->busy=1;
 if(p->state==VALIDATING){r=pt_project_validation_step(&p->validation,revision,p->generation,work);
if(r==PT_PROJECT_OK)p->state=BINDING;
else if(r!=PT_PROJECT_PENDING)p->failed=1;
p->busy=0;
return p->failed?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_PENDING;
}
 if(pt_project_validation_get(&p->validation,revision,p->generation,NULL)!=PT_PROJECT_OK||!current(p,revision,1)){p->failed=1;
p->busy=0;
return PT_SAMPLER_MIXED_STALE;
}
 p->paula.sampler=p->config.sampler;
p->paula.project=p->config.project;
p->paula.table=p->header.samples;
p->paula.count=p->header.sample_count;
p->paula.generation=p->generation;
p->paula.channels=p->header.channels.count;
p->paula.version=1;
pt_cache_init(&p->paula.cache,p,chip_allocate,chip_release,p->config.chip_budget);
 p->wave.sampler=p->config.sampler;
p->wave.project=p->config.project;
p->wave.backend=p->config.backend;
p->wave.table=p->header.samples;
p->wave.count=p->header.sample_count;
p->wave.generation=p->generation;
p->wave.version=1;
 if(pt_channels_paula_map(&p->header.channels,NULL,p->slot)!=PT_CHANNEL_OK){p->failed=1;
p->busy=0;
return PT_SAMPLER_MIXED_INVALID;
}
 for(i=0;i<p->header.channels.count;++i){p->paula.routes[i]=p->header.channels.track[i].route;
if(p->header.channels.track[i].route==PT_AMIGUS)p->slot[i]=(int8_t)card++;
}
 p->state=OPEN;
p->busy=0;
return PT_SAMPLER_MIXED_OK;
}
static struct pt_sampler_mixed_command *command(struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_command_handle h)
{unsigned i;
for(i=0;i<p->config.maximum_commands;++i)if(p->commands[i]==h.address&&h.address&&p->commands[i]->token==h.token)return p->commands[i];
return NULL;
}
static struct pt_sampler_mixed_reader *reader(struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_reader_handle h)
{unsigned i;
for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i]==h.address&&h.address&&p->readers[i]->token==h.token)return p->readers[i];
return NULL;
}
static int key_same(const struct pt_mixed_readers_key *a,const struct pt_mixed_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->route==b->route&&a->slot==b->slot;
}
static int key_zero(const struct pt_mixed_readers_key *k)
{return !k->queue&&!k->session&&!k->generation&&!k->trigger&&!k->owner&&!k->serial&&!k->action&&!k->route&&!k->slot;
}
static unsigned route(struct pt_sampler_mixed_pool *p,unsigned track)
{return p->header.channels.track[track].route==PT_PAULA?PT_MIXED_READERS_PAULA:PT_MIXED_READERS_AMIGUS;
}
static int voice_zero(const struct pt_amigus_voice_request *v)
{return !v->rate_numerator&&!v->rate_denominator&&!v->offset&&!v->volume&&!v->pan;}
/* Semantic unused fields only: no memcmp of C padding. The unused union
 * has a canonical amigus view containing every declared geometry field. */
static int request_zero(const struct pt_sampler_mixed_request *x)
{return !x->kind&&!x->track&&!x->sample&&!x->channel&&!x->expected&&key_zero(&x->key)&&
 !x->geometry.amigus.bits&&!x->geometry.amigus.little_endian&&
 voice_zero(&x->geometry.amigus.trigger)&&!x->geometry.amigus.rate&&
 !x->geometry.amigus.left&&!x->geometry.amigus.right;
}
static int levels_valid(const struct pt_sampler_mixed_trigger_levels *l,unsigned rt,
    const struct pt_sampler_mixed_request *x)
{
 if(!l)return 1; /* Legacy entry's internally canonical vector. */
 if(l->mode==PT_SAMPLER_MIXED_TRIGGER_LEGACY)return !l->left&&!l->right;
 return l->mode==PT_SAMPLER_MIXED_TRIGGER_QUANTIZED&&
   rt==PT_MIXED_READERS_AMIGUS&&x->kind==PT_MIXED_READERS_TRIGGER&&
   !x->geometry.amigus.trigger.volume&&!x->geometry.amigus.trigger.pan;
}
static int request_valid(struct pt_sampler_mixed_pool *p,const struct pt_sampler_mixed_request *x,unsigned n,uint64_t frame,unsigned *triggers,const struct pt_sampler_mixed_trigger_levels *levels)
{
 unsigned i,seen=0;
*triggers=0;
if(!n||n>PT_SAMPLER_MIXED_ACTIONS||frame==UINT64_MAX)return 0;
 if(levels)for(i=n;i<PT_SAMPLER_MIXED_ACTIONS;++i)
 if(!request_zero(x+i)||levels[i].mode!=PT_SAMPLER_MIXED_TRIGGER_LEGACY||levels[i].left||levels[i].right)return 0;
 for(i=0;i<n;++i){unsigned rt,sl;
if(p->causal_binding&&x[i].kind!=PT_MIXED_READERS_TRIGGER)return 0;
if(x[i].track>=p->header.channels.count||x[i].sample>=p->header.sample_count)return 0;
rt=route(p,x[i].track);
sl=(unsigned)p->slot[x[i].track];
if(p->header.channels.track[x[i].track].route!=PT_PAULA&&p->header.channels.track[x[i].track].route!=PT_AMIGUS)return 0;
if(!levels_valid(levels?levels+i:NULL,rt,x+i))return 0;
if(seen&(1U<<x[i].track))return 0;
seen|=1U<<x[i].track;
 if(x[i].kind==PT_MIXED_READERS_TRIGGER){const struct pt_sample *a=p->samples+x[i].sample;
if(!x[i].expected||x[i].expected!=p->masters[x[i].sample]||!key_zero(&x[i].key)||x[i].channel>=a->pcm.channels)return 0;
if(rt==PT_MIXED_READERS_PAULA){if(!x[i].geometry.paula.period||x[i].geometry.paula.volume>64||a->loop!=PT_LOOP_NONE||!a->pcm.frames||a->pcm.frames>131070)return 0;
}else if((x[i].geometry.amigus.bits!=8&&x[i].geometry.amigus.bits!=16)||x[i].geometry.amigus.little_endian>1||x[i].geometry.amigus.trigger.volume>64||x[i].geometry.amigus.trigger.pan>256||x[i].geometry.amigus.rate||x[i].geometry.amigus.left||x[i].geometry.amigus.right||!x[i].geometry.amigus.trigger.rate_numerator||!x[i].geometry.amigus.trigger.rate_denominator)return 0;
(*triggers)++;
}
 else {unsigned j;
struct pt_sampler_mixed_reader *r=NULL;
if(x[i].expected||(x[i].kind!=PT_MIXED_READERS_CONTROL&&x[i].kind!=PT_MIXED_READERS_STOP)||x[i].key.queue!=p->config.queue||x[i].key.route!=rt||x[i].key.slot!=sl)return 0;
for(j=0;j<p->config.maximum_readers;++j)if(p->readers[j]&&p->readers[j]->state==LIVE&&p->readers[j]->key_seen&&key_same(&p->readers[j]->key,&x[i].key)){r=p->readers[j];
break;
}
if(!r||r->track!=x[i].track||r->sample!=x[i].sample||r->channel!=x[i].channel||r->frame>=frame)return 0;
if(rt==PT_MIXED_READERS_AMIGUS&&(x[i].geometry.amigus.bits||x[i].geometry.amigus.little_endian||!voice_zero(&x[i].geometry.amigus.trigger)))return 0;
if(x[i].kind==PT_MIXED_READERS_STOP&&((rt==PT_MIXED_READERS_PAULA&&(x[i].geometry.paula.period||x[i].geometry.paula.volume))||(rt==PT_MIXED_READERS_AMIGUS&&(x[i].geometry.amigus.rate||x[i].geometry.amigus.left||x[i].geometry.amigus.right))))return 0;
if(x[i].kind==PT_MIXED_READERS_CONTROL){if(rt==PT_MIXED_READERS_PAULA){if(!x[i].geometry.paula.period||x[i].geometry.paula.volume>64)return 0;
}else if(!x[i].geometry.amigus.rate||x[i].geometry.amigus.rate>0x40000000UL)return 0;
}}
 }
return 1;
}
static void *allocate(struct pt_sampler_mixed_pool *p,size_t n,const void *input,size_t input_bytes,const void *out,size_t out_bytes)
{
 void *v;
if(p->failed||n>p->config.control_budget-p->bytes)return NULL;
v=p->config.allocator.allocate(p->config.allocator.context,n);
 if(!v||!output_apart(p,v,n)||!apart(v,n,input,input_bytes)||!apart(v,n,out,out_bytes))return NULL;
 if((uintptr_t)v%_Alignof(struct pt_sampler_mixed_reader)||!fixed_current(p,p->revision)||!backend_fixed(p)){p->config.allocator.release(p->config.allocator.context,v);
return NULL;
}
p->bytes+=n;
memset(v,0,n);
return v;
}
static void resources(struct pt_sampler_mixed_reader *r)
{
 struct pt_sampler_mixed_pool *p=r->pool;
if(r->released)return;
 pt_sampler_paula_job_cancel(&r->chip);
pt_sampler_upload_cancel(&r->card);
 if(r->leased){if(r->route==PT_MIXED_READERS_PAULA)pt_cache_unpin(&p->paula.cache,r->lease);
else pt_cache_unpin(&p->config.backend->cache,r->lease);
}
 r->leased=0;
pt_sampler_unpin(r->pin);
r->pin=NULL;
r->released=1;
}
static void reader_free(struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_reader *r)
{unsigned i;
resources(r);
for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i]==r)p->readers[i]=NULL;
p->bytes-=sizeof(*r);
p->config.allocator.release(p->config.allocator.context,r);
}
static void command_free(struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_command *c)
{unsigned i;
for(i=0;i<p->config.maximum_commands;++i)if(p->commands[i]==c)p->commands[i]=NULL;
p->bytes-=sizeof(*c);
p->config.allocator.release(p->config.allocator.context,c);
}
static void partial(struct pt_sampler_mixed_command *c,unsigned dispose)
{unsigned i;
struct pt_sampler_mixed_pool *p=c->pool;
for(i=0;i<c->count;++i)if(c->source[i].reader){struct pt_sampler_mixed_reader_handle h={c->source[i].reader,c->source[i].token};
struct pt_sampler_mixed_reader *r=reader(p,h);
if(r&&r->state!=LIVE&&r->state!=RETIRED){resources(r);
r->state=CANCELLED;
if(dispose){reader_free(p,r);
c->source[i].reader=NULL;
c->source[i].token=0;
}}}}
static enum pt_sampler_mixed_result begin_private(struct pt_sampler_mixed_pool *p,uint32_t revision,
 uint64_t frame,const struct pt_sampler_mixed_request *x,unsigned n,
 const struct pt_sampler_mixed_trigger_levels *levels,const void *input,size_t input_bytes,
 struct pt_sampler_mixed_command_handle *out)
{
 unsigned i,ci,rc=0,free_readers=0,triggers;
size_t request_bytes;
struct pt_sampler_mixed_command *c;
 if(!p||n>PT_SAMPLER_MIXED_ACTIONS||!n)return PT_SAMPLER_MIXED_INVALID;
request_bytes=(size_t)n*sizeof(*x);
 if(!output_apart(p,input,input_bytes)||!output_apart(p,out,sizeof(*out))||
 !apart(out,sizeof(*out),input,input_bytes)||!fixed_current(p,revision)||p->state!=OPEN||
 !request_valid(p,x,n,frame,&triggers,levels))return PT_SAMPLER_MIXED_INVALID;
 if(p->causal_binding&&(p->causal_successor||(p->stop_binding&&p->causal_first)||
 (p->lineage_binding&&p->lineage_stage)))return PT_SAMPLER_MIXED_CAPACITY;
 for(ci=0;ci<p->config.maximum_commands&&p->commands[ci];++ci){}
for(i=0;i<p->config.maximum_readers;++i)if(!p->readers[i])free_readers++;
 if(ci==p->config.maximum_commands||free_readers<triggers||p->serial>UINT64_MAX-triggers-1||sizeof(*c)>p->config.control_budget-p->bytes||triggers>(p->config.control_budget-p->bytes-sizeof(*c))/sizeof(struct pt_sampler_mixed_reader))return PT_SAMPLER_MIXED_CAPACITY;
 if(reentry(p))return PT_SAMPLER_MIXED_BUSY;
p->busy=1;
/* This private lifetime stage is consumed before the first callback/allocation,
 * never supplied by the caller and never reset when a command is cancelled. */
if(p->lineage_binding)p->lineage_stage=1;
memcpy(p->scratch,x,request_bytes);
/* Every legacy admission resets the WHOLE vector, including slots reused after
 * an exact command. New admission copies every previously validated tail. */
if(levels)memcpy(p->level_scratch,levels,sizeof(p->level_scratch));
else memset(p->level_scratch,0,sizeof(p->level_scratch));
 if(!current(p,revision,1)){p->failed=1;
p->busy=0;
return PT_SAMPLER_MIXED_STALE;
}
 c=allocate(p,sizeof(*c),input,input_bytes,out,sizeof(*out));
if(!c){p->busy=0;
return p->failed?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_CAPACITY;
}
 c->pool=p;
c->token=++p->serial;
c->state=PREPARING;
c->frame=frame;
c->count=n;
p->commands[ci]=c;
if(p->lineage_binding)p->lineage_root_token=c->token;
 for(i=0;i<n;++i){struct pt_sampler_mixed_reader *r;
c->source[i].request=p->scratch[i];
c->source[i].levels=p->level_scratch[i];
if(p->scratch[i].kind!=PT_MIXED_READERS_TRIGGER)continue;
for(rc=0;rc<p->config.maximum_readers&&p->readers[rc];++rc){}r=allocate(p,sizeof(*r),input,input_bytes,out,sizeof(*out));
if(!r){partial(c,1);
command_free(p,c);
p->busy=0;
return p->failed?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_CAPACITY;
}r->pool=p;
r->token=++p->serial;
r->state=PREPARING;
r->frame=frame;
r->track=p->scratch[i].track;
r->sample=p->scratch[i].sample;
r->channel=p->scratch[i].channel;
r->route=route(p,r->track);
r->slot=(unsigned)p->slot[r->track];
memcpy(&r->original,p->samples+r->sample,sizeof(r->original));
p->readers[rc]=r;
c->source[i].reader=r;
c->source[i].token=r->token;
}
 out->address=c;
out->token=c->token;
p->busy=0;
return PT_SAMPLER_MIXED_PENDING;
}
enum pt_sampler_mixed_result pt_sampler_mixed_begin(struct pt_sampler_mixed_pool *p,uint32_t revision,
 uint64_t frame,const struct pt_sampler_mixed_request *x,unsigned n,struct pt_sampler_mixed_command_handle *out)
{
 if(!p||!n||n>PT_SAMPLER_MIXED_ACTIONS)return PT_SAMPLER_MIXED_INVALID;
 return begin_private(p,revision,frame,x,n,NULL,x,(size_t)n*sizeof(*x),out);
}
enum pt_sampler_mixed_result pt_sampler_mixed_begin_quantized(struct pt_sampler_mixed_pool *p,uint32_t revision,
 uint64_t frame,const struct pt_sampler_mixed_quantized_batch *batch,struct pt_sampler_mixed_command_handle *out)
{
 /* Numeric complete-input/output guards precede the first batch field read. */
 if(!p||(uintptr_t)batch%_Alignof(struct pt_sampler_mixed_quantized_batch)||
 !output_apart(p,batch,sizeof(*batch))||!output_apart(p,out,sizeof(*out))||
 !apart(batch,sizeof(*batch),out,sizeof(*out)))return PT_SAMPLER_MIXED_INVALID;
 return begin_private(p,revision,frame,batch->action,batch->count,batch->levels,batch,sizeof(*batch),out);
}
enum pt_sampler_mixed_result pt_sampler_mixed_causal_stop_begin(struct pt_sampler_mixed_pool *p,
 uint32_t revision,const struct pt_sampler_mixed_causal_stop_batch *batch,
 struct pt_sampler_mixed_command_handle *out)
{
 struct pt_sampler_mixed_causal_stop_batch saved;
 struct pt_sampler_mixed_request request[PT_SAMPLER_MIXED_ACTIONS];
 struct pt_mixed_readers_key key;
 struct pt_sampler_mixed_reader *r;
 struct pt_sampler_mixed_command *c;
 unsigned i,j,ci,mask=0,index;enum pt_mixed_readers_result result;
 /* WHOLE original caller, output, genuine holders and local capacities are
  * guarded before writes/getters/allocator callbacks; transformed scratch
  * never hides an alias into the original fixed handle vector. */
 if(!p||(uintptr_t)batch%_Alignof(struct pt_sampler_mixed_causal_stop_batch)||
    (uintptr_t)out%_Alignof(struct pt_sampler_mixed_command_handle)||
    !output_apart(p,batch,sizeof(*batch))||!output_apart(p,out,sizeof(*out))||
    !apart(batch,sizeof(*batch),out,sizeof(*out))||
    !output_address_apart(p,(uintptr_t)&saved,sizeof(saved))||
    !output_address_apart(p,(uintptr_t)request,sizeof(request))||
    !output_address_apart(p,(uintptr_t)&key,sizeof(key))||
    !apart(&saved,sizeof(saved),batch,sizeof(*batch))||!apart(request,sizeof(request),batch,sizeof(*batch))||
    !apart(&key,sizeof(key),batch,sizeof(*batch))||!apart(&saved,sizeof(saved),out,sizeof(*out))||
    !apart(request,sizeof(request),out,sizeof(*out))||!apart(&key,sizeof(key),out,sizeof(*out))||
    !fixed_current(p,revision)||p->state!=OPEN||!p->stop_binding||!p->causal_first||p->causal_successor||
    !batch->count||batch->count>PT_SAMPLER_MIXED_ACTIONS||batch->frame==UINT64_MAX)return PT_SAMPLER_MIXED_INVALID;
 for(i=0;i<PT_SAMPLER_MIXED_ACTIONS;++i){
  if(i>=batch->count){if(batch->reader[i].address||batch->reader[i].token)return PT_SAMPLER_MIXED_INVALID;continue;}
  r=reader(p,batch->reader[i]);
  if(!r||r->state!=LIVE||r->ticket!=p->causal_first||r->frame>=batch->frame)return PT_SAMPLER_MIXED_INVALID;
  index=r->route==PT_MIXED_READERS_PAULA?r->slot:r->slot+4;
  if(index>=20||(mask&(1U<<index)))return PT_SAMPLER_MIXED_INVALID;
  mask|=1U<<index;
 }
 for(ci=0;ci<p->config.maximum_commands&&p->commands[ci];++ci){}
 if(ci==p->config.maximum_commands||p->serial==UINT64_MAX||
    sizeof(*c)>p->config.control_budget-p->bytes)return PT_SAMPLER_MIXED_CAPACITY;
 if(reentry(p))return PT_SAMPLER_MIXED_BUSY;
 p->busy=1;p->queue_call=1;memcpy(&saved,batch,sizeof(saved));memset(request,0,sizeof(request));
 j=pt_mixed_causal_factory_stop_first_current(p->causal_original.owner,p->causal_first);
 if(memcmp(batch,&saved,sizeof(saved)))p->failed=1;
 if(!j||p->failed||!fixed_current(p,revision)||!backend_fixed(p)){
  p->queue_call=0;p->busy=0;return p->failed?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_INVALID;}
 for(i=0;i<saved.count;++i){
  r=reader(p,saved.reader[i]);
  result=pt_mixed_causal_reader_key(p->causal_original.owner,r->ticket,r->action,&key);
  if(memcmp(batch,&saved,sizeof(saved)))p->failed=1;
  if(result!=PT_MIXED_READERS_OK||p->failed||!fixed_current(p,revision)||!backend_fixed(p)||
     reader(p,saved.reader[i])!=r||r->state!=LIVE||key.trigger!=p->causal_first||
     key.action!=r->action||key.owner!=r->token||key.session!=p->causal_original.session||
     key.generation!=p->config.generation||key.queue!=p->config.queue||key.route!=r->route||key.slot!=r->slot){
   p->queue_call=0;p->busy=0;return p->failed?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_INVALID;}
  r->key=key;r->key_seen=1;request[i].kind=PT_MIXED_READERS_STOP;request[i].track=r->track;
  request[i].sample=r->sample;request[i].channel=r->channel;request[i].key=key;
 }
 p->queue_call=0;
 if(!current(p,revision,1)||memcmp(batch,&saved,sizeof(saved))){p->failed=1;p->busy=0;return PT_SAMPLER_MIXED_STALE;}
 /* All local scratch extents remain guarded across the actual allocator;
  * no recognized alias is initialized or released. */
 c=p->config.allocator.allocate(p->config.allocator.context,sizeof(*c));
 if(!c){p->busy=0;return p->failed?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_CAPACITY;}
 if(!output_apart(p,c,sizeof(*c))||!apart(c,sizeof(*c),batch,sizeof(*batch))||
    !apart(c,sizeof(*c),out,sizeof(*out))||!apart(c,sizeof(*c),&saved,sizeof(saved))||
    !apart(c,sizeof(*c),request,sizeof(request))||!apart(c,sizeof(*c),&key,sizeof(key))){
  p->busy=0;return PT_SAMPLER_MIXED_INVALID;}
 if((uintptr_t)c%_Alignof(struct pt_sampler_mixed_command)||memcmp(batch,&saved,sizeof(saved))||
    !fixed_current(p,revision)||!backend_fixed(p)){
  p->failed=1;p->config.allocator.release(p->config.allocator.context,c);
  p->busy=0;return PT_SAMPLER_MIXED_STALE;}
 p->bytes+=sizeof(*c);memset(c,0,sizeof(*c));c->pool=p;c->token=++p->serial;c->state=PREPARING;c->causal_stop=1;c->frame=saved.frame;c->count=saved.count;
 for(j=0;j<saved.count;++j)memcpy(&c->source[j].request,request+j,sizeof(request[j]));
 p->commands[ci]=c;out->address=c;out->token=c->token;p->busy=0;return PT_SAMPLER_MIXED_PENDING;
}
/* Shared private construction preserves the complete typed caller extent and
 * both local aggregate capacities. Public callers choose no mode flag. */
static enum pt_sampler_mixed_result lineage_begin(struct pt_sampler_mixed_pool *p,
 uint32_t revision,const void *input,unsigned is_stop,struct pt_sampler_mixed_command_handle *out)
{
 union {struct pt_sampler_mixed_causal_lineage_control_batch control;
        struct pt_sampler_mixed_causal_lineage_stop_batch stop;} saved;
 struct pt_sampler_mixed_request request[PT_SAMPLER_MIXED_ACTIONS];
 struct pt_mixed_readers_key key;
 struct pt_sampler_mixed_reader *r;
 struct pt_sampler_mixed_command *c;
 size_t input_bytes=is_stop?sizeof(saved.stop):sizeof(saved.control);
 size_t alignment=is_stop?_Alignof(struct pt_sampler_mixed_causal_lineage_stop_batch):
    _Alignof(struct pt_sampler_mixed_causal_lineage_control_batch);
 uint64_t frame;unsigned count,i,j,ci,index,mask=0;enum pt_mixed_readers_result result;
 if(!p||(uintptr_t)input%alignment||(uintptr_t)out%_Alignof(struct pt_sampler_mixed_command_handle)||
    !output_apart(p,input,input_bytes)||!output_apart(p,out,sizeof(*out))||
    !apart(input,input_bytes,out,sizeof(*out))||
    !output_address_apart(p,(uintptr_t)&saved,sizeof(saved))||
    !output_address_apart(p,(uintptr_t)request,sizeof(request))||
    !output_address_apart(p,(uintptr_t)&key,sizeof(key))||
    !apart(&saved,sizeof(saved),input,input_bytes)||!apart(request,sizeof(request),input,input_bytes)||
    !apart(&key,sizeof(key),input,input_bytes)||!apart(&saved,sizeof(saved),out,sizeof(*out))||
    !apart(request,sizeof(request),out,sizeof(*out))||!apart(&key,sizeof(key),out,sizeof(*out))||
    !apart(&saved,sizeof(saved),request,sizeof(request))||!apart(&saved,sizeof(saved),&key,sizeof(key))||
    !apart(request,sizeof(request),&key,sizeof(key))||!fixed_current(p,revision)||p->state!=OPEN||
    !p->lineage_binding||!p->causal_first||p->lineage_stop||
    p->lineage_stage!=(is_stop?2U:1U)||
    (is_stop&&(!p->lineage_control||!p->lineage_root_detached||!p->lineage_root_closed)))
    return PT_SAMPLER_MIXED_INVALID;
 memcpy(&saved,input,input_bytes);memset(request,0,sizeof(request));
 frame=is_stop?saved.stop.frame:saved.control.frame;count=is_stop?saved.stop.count:saved.control.count;
 if(!count||count>PT_SAMPLER_MIXED_ACTIONS||frame==UINT64_MAX||
 (is_stop&&frame<=p->lineage_control_frame))return PT_SAMPLER_MIXED_INVALID;
 for(i=0;i<PT_SAMPLER_MIXED_ACTIONS;++i){
  struct pt_sampler_mixed_reader_handle h=is_stop?saved.stop.reader[i]:saved.control.action[i].reader;
  const struct pt_sampler_mixed_causal_lineage_control_action *a=is_stop?NULL:saved.control.action+i;
  if(i>=count){
   if(h.address||h.token||(!is_stop&&(a->period||a->volume||a->rate||a->left||a->right))){
    return PT_SAMPLER_MIXED_INVALID;
   }
   continue;
  }
  r=reader(p,h);
  if(!r||r->state!=LIVE||r->ticket!=p->causal_first||r->frame>=frame)return PT_SAMPLER_MIXED_INVALID;
  index=r->route==PT_MIXED_READERS_PAULA?r->slot:r->slot+4;
  if(index>=20||(mask&(1U<<index)))return PT_SAMPLER_MIXED_INVALID;
  mask|=1U<<index;
  if(!is_stop){
   if(r->route==PT_MIXED_READERS_PAULA){if(!a->period||a->volume>64||a->rate||a->left||a->right)return PT_SAMPLER_MIXED_INVALID;}
   else if(a->period||a->volume||!a->rate||a->rate>0x40000000UL)return PT_SAMPLER_MIXED_INVALID;
  }
 }
 for(ci=0;ci<p->config.maximum_commands&&p->commands[ci];++ci){}
 if(ci==p->config.maximum_commands||p->serial==UINT64_MAX||sizeof(*c)>p->config.control_budget-p->bytes)
    return PT_SAMPLER_MIXED_CAPACITY;
 if(reentry(p))return PT_SAMPLER_MIXED_BUSY;
 p->busy=1;p->queue_call=1;
 j=is_stop?pt_mixed_causal_factory_control_stop_control_current(p->causal_original.owner,p->lineage_control):
    pt_mixed_causal_factory_control_stop_first_current(p->causal_original.owner,p->causal_first);
 if(memcmp(input,&saved,input_bytes))p->failed=1;
 if(!j||p->failed||!fixed_current(p,revision)||!backend_fixed(p)){
  p->queue_call=0;p->busy=0;return p->failed?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_INVALID;}
 for(i=0;i<count;++i){
  struct pt_sampler_mixed_reader_handle h=is_stop?saved.stop.reader[i]:saved.control.action[i].reader;
  r=reader(p,h);
  if(!r||r->state!=LIVE||r->ticket!=p->causal_first){p->queue_call=0;p->busy=0;return PT_SAMPLER_MIXED_INVALID;}
  result=pt_mixed_causal_reader_key(p->causal_original.owner,r->ticket,r->action,&key);
  if(memcmp(input,&saved,input_bytes))p->failed=1;
  if(result!=PT_MIXED_READERS_OK||p->failed||!fixed_current(p,revision)||!backend_fixed(p)||
     reader(p,h)!=r||r->state!=LIVE||key.trigger!=p->causal_first||key.action!=r->action||
     key.owner!=r->token||key.session!=p->causal_original.session||key.generation!=p->config.generation||
     key.queue!=p->config.queue||key.route!=r->route||key.slot!=r->slot){
   p->queue_call=0;p->busy=0;return p->failed?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_INVALID;}
  r->key=key;r->key_seen=1;request[i].kind=is_stop?PT_MIXED_READERS_STOP:PT_MIXED_READERS_CONTROL;
  request[i].track=r->track;request[i].sample=r->sample;request[i].channel=r->channel;request[i].key=key;
  if(!is_stop){const struct pt_sampler_mixed_causal_lineage_control_action *a=saved.control.action+i;
   if(r->route==PT_MIXED_READERS_PAULA){request[i].geometry.paula.period=a->period;request[i].geometry.paula.volume=a->volume;}
   else{request[i].geometry.amigus.rate=a->rate;request[i].geometry.amigus.left=a->left;request[i].geometry.amigus.right=a->right;}}
 }
 p->queue_call=0;
 if(!current(p,revision,1)||memcmp(input,&saved,input_bytes)){p->failed=1;p->busy=0;return PT_SAMPLER_MIXED_STALE;}
 /* Monotonic attempt boundary. A NULL/alias/mutation/cancel cannot reconstruct
  * this stage. The pool separately consumes each actual lower enqueue attempt. */
 p->lineage_stage=is_stop?3U:2U;
 c=p->config.allocator.allocate(p->config.allocator.context,sizeof(*c));
 if(!c){p->busy=0;return p->failed?PT_SAMPLER_MIXED_STALE:PT_SAMPLER_MIXED_CAPACITY;}
 if(!output_apart(p,c,sizeof(*c))||!apart(c,sizeof(*c),input,input_bytes)||
    !apart(c,sizeof(*c),out,sizeof(*out))||!apart(c,sizeof(*c),&saved,sizeof(saved))||
    !apart(c,sizeof(*c),request,sizeof(request))||!apart(c,sizeof(*c),&key,sizeof(key))){
  p->busy=0;return PT_SAMPLER_MIXED_INVALID;}
 if((uintptr_t)c%_Alignof(struct pt_sampler_mixed_command)||memcmp(input,&saved,input_bytes)||
    !fixed_current(p,revision)||!backend_fixed(p)){
  p->failed=1;p->config.allocator.release(p->config.allocator.context,c);p->busy=0;return PT_SAMPLER_MIXED_STALE;}
 p->bytes+=sizeof(*c);memset(c,0,sizeof(*c));c->pool=p;c->token=++p->serial;c->state=PREPARING;
 c->causal_stop=is_stop;c->causal_control=!is_stop;c->frame=frame;c->count=count;
 for(i=0;i<count;++i)memcpy(&c->source[i].request,request+i,sizeof(request[i]));
 p->commands[ci]=c;out->address=c;out->token=c->token;p->busy=0;return PT_SAMPLER_MIXED_PENDING;
}
enum pt_sampler_mixed_result pt_sampler_mixed_causal_lineage_control_begin(struct pt_sampler_mixed_pool *p,
 uint32_t revision,const struct pt_sampler_mixed_causal_lineage_control_batch *batch,
 struct pt_sampler_mixed_command_handle *out)
{return lineage_begin(p,revision,batch,0,out);}
enum pt_sampler_mixed_result pt_sampler_mixed_causal_lineage_stop_begin(struct pt_sampler_mixed_pool *p,
 uint32_t revision,const struct pt_sampler_mixed_causal_lineage_stop_batch *batch,
 struct pt_sampler_mixed_command_handle *out)
{return lineage_begin(p,revision,batch,1,out);}
static int source_current(struct pt_sampler_mixed_reader *r)
{
 struct pt_sampler_mixed_pool *p=r->pool;
struct pt_pcm pcm;
struct pt_sample_version *pin;
const uint8_t *data;
size_t bytes;
void *resource;
 if(!r->pin||!r->leased||pt_sampler_pin_current(p->config.sampler,p->config.project,r->sample,p->generation,r->pin,&pcm,&pin)!=PT_EDIT_OK)return 0;
pt_sampler_unpin(pin);
 if(r->route==PT_MIXED_READERS_PAULA)return pt_sampler_paula_prepared_location_validated(&p->paula,r->track,r->lease,&data,&bytes)&&data==r->data&&bytes==r->bytes;
 resource=pt_cache_data(&p->config.backend->cache,r->lease);
 if(resource&&resource==&p->config.backend->arena.block[r->arena_index]){struct pt_amigus_ram_block *b=resource;
return b->address==r->resource.address&&b->bytes==r->resource.logical_bytes&&b->reserved==r->resource.full_capacity&&b->written==b->bytes&&!b->fill&&!b->failed&&p->config.backend->cache.entry[r->lease.slot].valid==1&&p->config.backend->cache.entry[r->lease.slot].version==r->resource.version&&p->config.backend->cache.entry[r->lease.slot].serial==r->resource.serial;
}
return 0;
}
static int holder_current(void *context,uint64_t token,uint64_t generation)
{
 struct pt_sampler_mixed_reader *r=context;
struct pt_sampler_mixed_pool *p=r->pool;
int ok;
 if(r->token!=token||generation!=p->config.generation||(p->busy&&!p->queue_call))return 0;
 ok=current(p,p->revision,1)&&pt_sampler_paula_prepared_current(&p->paula)&&pt_sampler_wavetable_prepared_metadata_current(&p->wave)&&source_current(r);
return ok&&!p->failed;
}
static int command_current(void *context,uint64_t token,uint64_t generation)
{struct pt_sampler_mixed_command *c=context;
struct pt_sampler_mixed_pool *p=c->pool;
return token==c->token&&generation==p->config.generation&&(!p->busy||p->queue_call)&&current(p,p->revision,1);
}
static void command_terminal(void *context,uint64_t token,int valid)
{struct pt_sampler_mixed_command *c=context;
if(c->token!=token){c->pool->failed=1;
return;
}
c->terminal=1;
c->valid=(unsigned)(valid==1);
}
static void command_release(void *context,uint64_t token)
{struct pt_sampler_mixed_command *c=context;
if(c->token!=token||!c->terminal){c->pool->failed=1;
return;
}
c->state=RETIRED;
}
static void reader_terminal(void *context,uint64_t token,int valid)
{struct pt_sampler_mixed_reader *r=context;
if(r->token!=token){r->pool->failed=1;
return;
}r->terminal=1;
r->valid=(unsigned)(valid==1);
}
static void reader_release(void *context,uint64_t token)
{struct pt_sampler_mixed_reader *r=context;
if(r->token!=token||!r->terminal){r->pool->failed=1;
return;
}resources(r);
r->state=RETIRED;
}
static int reader_spans(struct pt_sampler_mixed_reader *r)
{
 struct pt_sampler_mixed_pool *p=r->pool;
struct pt_sampler_storage_span v[PT_SAMPLER_VERSION_SPANS];
unsigned n,i;
r->span_count=0;
 if(!pt_sampler_version_spans(r->pin,v,PT_SAMPLER_VERSION_SPANS,&n))return 0;
for(i=0;i<n;++i){r->spans[r->span_count].data=v[i].data;
r->spans[r->span_count++].bytes=v[i].bytes;
}
 r->spans[r->span_count].data=p;
r->spans[r->span_count++].bytes=sizeof(*p);
 if(r->route==PT_MIXED_READERS_PAULA){r->spans[r->span_count].data=r->data;
r->spans[r->span_count++].bytes=r->bytes;
}
 else {r->spans[r->span_count].data=p->config.backend;
r->spans[r->span_count++].bytes=sizeof(*p->config.backend);
r->spans[r->span_count].data=p->backend_header.reservation;
r->spans[r->span_count++].bytes=sizeof(*p->backend_header.reservation);
}
 return 1;
}
static int build_action(struct pt_sampler_mixed_command *c,unsigned i)
{
 struct pt_sampler_mixed_pool *p=c->pool;
struct source *s=c->source+i;
struct pt_sampler_mixed_request *x=&s->request;
struct pt_mixed_readers_action *a=c->inputs.batch.action+i;
struct pt_sampler_mixed_reader *r=s->reader;
 a->kind=x->kind;
a->route=route(p,x->track);
a->slot=(unsigned)p->slot[x->track];
 if(x->kind!=PT_MIXED_READERS_TRIGGER){c->inputs.target[i]=x->key;
if(x->kind==PT_MIXED_READERS_CONTROL){if(a->route==PT_MIXED_READERS_PAULA){a->geometry.paula.period=x->geometry.paula.period;
a->geometry.paula.volume=x->geometry.paula.volume;
}else{a->geometry.amigus.rate=x->geometry.amigus.rate;
a->geometry.amigus.left=x->geometry.amigus.left;
a->geometry.amigus.right=x->geometry.amigus.right;
}}
return 1;
}
 if(a->route==PT_MIXED_READERS_PAULA){const uint8_t *data;
size_t bytes;
if(!pt_sampler_paula_prepared_current(&p->paula)||!pt_sampler_paula_prepared_location_validated(&p->paula,r->track,r->lease,&data,&bytes)||!bytes||(bytes&1)||bytes/2>65535)return 0;
r->data=data;
r->bytes=bytes;
a->geometry.paula.data=data;
a->geometry.paula.words=(uint16_t)(bytes/2);
a->geometry.paula.period=x->geometry.paula.period;
a->geometry.paula.volume=x->geometry.paula.volume;
}
 else {struct pt_amigus_wavetable_cache *b=p->config.backend;
void *v=pt_cache_data(&b->cache,r->lease);
unsigned j;
uint32_t address,bytes;
if(!v)return 0;
for(j=0;j<PT_CACHE_SLOTS&&v!=&b->arena.block[j];++j){}
if(j==PT_CACHE_SLOTS||!pt_amigus_sample_ram_location(&b->arena,v,&address,&bytes)||b->arena.block[j].reserved<bytes)return 0;
if(s->levels.mode==PT_SAMPLER_MIXED_TRIGGER_QUANTIZED){
 struct pt_amigus_trigger_levels_request request;
 struct pt_amigus_voice_plan plan;
 /* Guard complete local child objects against all captured/owned contexts and
  * each other BEFORE initialization. No existing typed-action member is passed
  * as D1 output. The real held original/format and matched resource alone lower. */
 if(!output_address_apart(p,(uintptr_t)&request,sizeof(request))||
 !output_address_apart(p,(uintptr_t)&plan,sizeof(plan))||
 !apart((const void *)(uintptr_t)&request,sizeof(request),(const void *)(uintptr_t)&plan,sizeof(plan)))return 0;
 request.geometry=x->geometry.amigus.trigger;
 request.left=s->levels.left;
 request.right=s->levels.right;
 if(!pt_amigus_trigger_levels_prepare(&r->original,&r->format,&request,address,bytes,&plan))return 0;
 a->geometry.amigus=plan;
}else if(s->levels.mode!=PT_SAMPLER_MIXED_TRIGGER_LEGACY||
 !pt_amigus_voice_plan_prepare(&r->original,&r->format,&x->geometry.amigus.trigger,address,bytes,&a->geometry.amigus))return 0;
r->resource.reservation=b->reservation;
r->resource.cache=&b->cache;
r->resource.version=b->cache.entry[r->lease.slot].version;
r->resource.serial=r->lease.serial;
r->resource.cache_slot=r->lease.slot;
r->arena_index=j;
r->resource.bits=r->format.bits;
r->resource.little_endian=r->format.little_endian;
r->resource.source_channel=r->channel;
r->resource.address=address;
r->resource.logical_bytes=bytes;
r->resource.full_capacity=b->arena.block[j].reserved;
}
 if(!reader_spans(r))return 0;
c->inputs.reader[i].control.context=r;
c->inputs.reader[i].control.context_bytes=sizeof(*r);
c->inputs.reader[i].control.token=r->token;
c->inputs.reader[i].control.current=holder_current;
c->inputs.reader[i].control.terminal=reader_terminal;
c->inputs.reader[i].control.release=reader_release;
c->inputs.reader[i].spans=r->spans;
c->inputs.reader[i].count=r->span_count;
c->inputs.reader[i].card=r->resource;
r->state=READY;
return 1;
}
enum pt_sampler_mixed_result pt_sampler_mixed_advance(struct pt_sampler_mixed_pool *p,uint32_t revision,struct pt_sampler_mixed_command_handle h,unsigned *out)
{
 struct pt_sampler_mixed_command *c;
struct pt_sampler_mixed_reader *r;
struct source *s;
enum pt_cache_result cr;
enum pt_sampler_mixed_result result=PT_SAMPLER_MIXED_PENDING;
 if(!p||(out&&!output_apart(p,out,sizeof(*out)))||!(c=command(p,h)))return PT_SAMPLER_MIXED_INVALID;
 if(c->state==CANCELLED||c->state==FAILED||c->state==RETIRED||c->state==LIVE)return PT_SAMPLER_MIXED_INVALID;
 if(reentry(p))return PT_SAMPLER_MIXED_BUSY;
if(!current(p,revision,0)){p->failed=1;
return PT_SAMPLER_MIXED_STALE;
}
if(c->state==READY){if(out)*out=c->index;
return PT_SAMPLER_MIXED_OK;
}
 p->busy=1;
if(!current(p,revision,1))goto fail;
 s=c->source+c->index;
r=s->reader;
 if(!r){if(!build_action(c,c->index))goto fail;
c->index++;
}
 else if(c->phase==PIN){if(pt_sampler_pin_current(p->config.sampler,p->config.project,r->sample,p->generation,s->request.expected,&r->pcm,&r->pin)!=PT_EDIT_OK)goto fail;
c->phase=DERIVED_BEGIN;
}
 else if(c->phase==DERIVED_BEGIN){if(r->route==PT_MIXED_READERS_PAULA)cr=pt_sampler_paula_job_begin(&r->chip,&p->paula,r->track,r->sample,r->channel,r->pin,&r->lease);
else{r->format.bits=s->request.geometry.amigus.bits;
r->format.channel=r->channel;
r->format.little_endian=s->request.geometry.amigus.little_endian;
r->format.word_pad=0;
cr=pt_sampler_upload_begin_prepared(&r->card,&p->wave,r->sample,p->generation,1,r->pin,&r->format,&r->lease);
}
if(cr==PT_CACHE_HIT||cr==PT_CACHE_LOAD){r->leased=1;
c->phase=ACTION;
}else if(cr==PT_CACHE_PENDING)c->phase=DERIVED_STEP;
else goto fail;
}
 else if(c->phase==DERIVED_STEP){if(r->route==PT_MIXED_READERS_PAULA)cr=pt_sampler_paula_job_step(&r->chip,&r->lease);
else cr=pt_sampler_upload_step(&r->card,r->staging,sizeof(r->staging),&r->lease);
if(cr==PT_CACHE_HIT||cr==PT_CACHE_LOAD){r->leased=1;
c->phase=ACTION;
}else if(cr!=PT_CACHE_PENDING)goto fail;
}
 else {if(!build_action(c,c->index))goto fail;
c->index++;
c->phase=PIN;
}
 if(!current(p,revision,0))goto fail;
if(c->index==c->count){c->state=READY;
result=PT_SAMPLER_MIXED_OK;
}
p->busy=0;
if(out)*out=c->index;
return result;
fail: c->state=FAILED;
partial(c,0);
p->failed=1;
p->busy=0;
return PT_SAMPLER_MIXED_STALE;
}
enum pt_mixed_readers_result pt_sampler_mixed_enqueue(struct pt_sampler_mixed_pool *p,uint32_t revision,struct pt_sampler_mixed_command_handle h,uint64_t *out)
{
 struct pt_sampler_mixed_command *c;
enum pt_mixed_readers_result result;
uint64_t ticket;
unsigned i;
struct pt_mixed_causal_stop_request stop;
struct pt_mixed_causal_control_request control;
 if(!p||!output_apart(p,out,sizeof(*out))||!(c=command(p,h))||c->state!=READY)return PT_MIXED_READERS_INVALID;
 if(c->causal_stop&&((uintptr_t)out%_Alignof(uint64_t)||
 !output_address_apart(p,(uintptr_t)&stop,sizeof(stop))||
 !apart(&stop,sizeof(stop),out,sizeof(*out))))return PT_MIXED_READERS_INVALID;
 if(c->causal_control&&((uintptr_t)out%_Alignof(uint64_t)||
 !output_address_apart(p,(uintptr_t)&control,sizeof(control))||
 !apart(&control,sizeof(control),out,sizeof(*out))))return PT_MIXED_READERS_INVALID;
 if(reentry(p))return PT_MIXED_READERS_BACKEND;
if(!current(p,revision,0))return PT_MIXED_READERS_STALE;
 p->busy=1;
p->queue_call=1;
c->inputs.batch.generation=p->config.generation;
c->inputs.batch.frame=c->frame;
c->inputs.batch.count=c->count;
c->inputs.command.context=c;
c->inputs.command.context_bytes=sizeof(*c);
c->inputs.command.token=c->token;
c->inputs.command.current=command_current;
c->inputs.command.terminal=command_terminal;
c->inputs.command.release=command_release;
 if(p->lineage_binding){
    if(!p->causal_first&&!c->causal_control&&!c->causal_stop&&c->token==p->lineage_root_token&&!p->lineage_enqueue_attempt){
       p->lineage_enqueue_attempt=1;result=pt_mixed_causal_enqueue(p->causal_original.owner,&c->inputs,out);}
    else if(p->causal_first&&c->causal_control&&!p->lineage_control&&!p->lineage_stop&&p->lineage_enqueue_attempt==1){
       p->lineage_enqueue_attempt=2;memset(&control,0,sizeof(control));control.predecessor=p->causal_first;control.frame=c->frame;
       control.count=c->count;memcpy(&control.command,&c->inputs.command,sizeof(control.command));
       for(i=0;i<c->count;++i){struct pt_mixed_causal_control_action *a=control.action+i;
          const struct pt_mixed_readers_action *source=c->inputs.batch.action+i;
          a->route=source->route;a->slot=source->slot;
          if(a->route==PT_MIXED_READERS_PAULA){a->period=source->geometry.paula.period;a->volume=source->geometry.paula.volume;}
          else{a->rate=source->geometry.amigus.rate;a->left=source->geometry.amigus.left;a->right=source->geometry.amigus.right;}}
       result=pt_mixed_causal_control_enqueue(p->causal_original.owner,&control,out);
    }else if(p->lineage_control&&c->causal_stop&&!p->lineage_stop&&p->lineage_root_closed&&p->lineage_root_detached&&p->lineage_enqueue_attempt==2){
       p->lineage_enqueue_attempt=3;memset(&stop,0,sizeof(stop));stop.predecessor=p->lineage_control;stop.frame=c->frame;
       stop.count=c->count;memcpy(&stop.command,&c->inputs.command,sizeof(stop.command));
       for(i=0;i<c->count;++i){stop.action[i].route=c->inputs.batch.action[i].route;stop.action[i].slot=c->inputs.batch.action[i].slot;}
       result=pt_mixed_causal_stop_after_control_enqueue(p->causal_original.owner,&stop,out);
    }else result=PT_MIXED_READERS_INVALID;
    /* Actual lower OK/ticket is retained even after an outer ownership fault. */
    if(result==PT_MIXED_READERS_OK){ticket=*out;
       if(!p->causal_first)p->causal_first=ticket;
       else if(c->causal_control){p->lineage_control=ticket;p->lineage_control_frame=c->frame;}
       else p->lineage_stop=ticket;}
 }else if(p->causal_binding){
    if(p->causal_successor)result=PT_MIXED_READERS_INVALID;
    else if(!p->causal_first)result=pt_mixed_causal_enqueue(p->causal_original.owner,&c->inputs,out);
    else if(p->stop_binding&&c->causal_stop){
       memset(&stop,0,sizeof(stop));stop.predecessor=p->causal_first;stop.frame=c->frame;
       stop.count=c->count;memcpy(&stop.command,&c->inputs.command,sizeof(stop.command));
       for(i=0;i<c->count;++i){stop.action[i].route=c->inputs.batch.action[i].route;stop.action[i].slot=c->inputs.batch.action[i].slot;}
       result=pt_mixed_causal_stop_enqueue(p->causal_original.owner,&stop,out);
    }else if(p->stop_binding)result=PT_MIXED_READERS_INVALID;
    else result=pt_mixed_causal_enqueue_successor(p->causal_original.owner,p->causal_first,&c->inputs,out);
    if(result==PT_MIXED_READERS_OK){ticket=*out;
       if(!p->causal_first)p->causal_first=ticket;else p->causal_successor=ticket;}
 }else result=pt_mixed_readers_enqueue(p->config.queue,&c->inputs,&ticket);
p->queue_call=0;
 if(result==PT_MIXED_READERS_OK){c->ticket=ticket;
c->state=LIVE;
for(i=0;i<c->count;++i)if(c->source[i].reader){struct pt_sampler_mixed_reader *r=c->source[i].reader;
r->ticket=ticket;
r->action=i;
r->state=LIVE;
}
*out=ticket;
}
p->busy=0;
return result==PT_MIXED_READERS_OK?result:(p->failed?PT_MIXED_READERS_BACKEND:result);
}
enum pt_sampler_mixed_result pt_sampler_mixed_reader(struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_command_handle h,unsigned i,struct pt_sampler_mixed_reader_handle *out)
{struct pt_sampler_mixed_command *c;
struct pt_sampler_mixed_reader *r;
if(!p||!output_apart(p,out,sizeof(*out))||!(c=command(p,h))||i>=c->count||!(r=c->source[i].reader)||!reader(p,(struct pt_sampler_mixed_reader_handle){r,c->source[i].token}))return PT_SAMPLER_MIXED_INVALID;
if(reentry(p))return PT_SAMPLER_MIXED_BUSY;
out->address=r;
out->token=r->token;
return PT_SAMPLER_MIXED_OK;
}
enum pt_mixed_readers_result pt_sampler_mixed_reader_key(struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_reader_handle h,struct pt_mixed_readers_key *out)
{struct pt_sampler_mixed_reader *r;
struct pt_mixed_readers_key key;
enum pt_mixed_readers_result result;
if(!p||!output_apart(p,out,sizeof(*out))||!(r=reader(p,h))||r->state!=LIVE)return PT_MIXED_READERS_INVALID;
if(reentry(p))return PT_MIXED_READERS_BACKEND;
if(!fixed_current(p,p->revision))return PT_MIXED_READERS_STALE;
p->busy=1;
p->queue_call=1;
result=p->causal_binding?pt_mixed_causal_reader_key(p->causal_original.owner,r->ticket,r->action,&key):
 pt_mixed_readers_reader_key(p->config.queue,r->ticket,r->action,&key);
p->queue_call=0;
if(result==PT_MIXED_READERS_OK&&!p->failed){r->key=key;
r->key_seen=1;
*out=key;
}
p->busy=0;
return p->failed?PT_MIXED_READERS_BACKEND:result;
}
enum pt_mixed_readers_result pt_sampler_mixed_reader_readiness(
 struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_reader_handle h)
{
 struct pt_sampler_mixed_reader *r;enum pt_mixed_readers_result result;
 if(!p||!h.address||!h.token||!(r=reader(p,h))||r->state!=LIVE||!r->ticket||
    r->action>=PT_MIXED_READERS_ACTIONS)return PT_MIXED_READERS_INVALID;
 if(reentry(p))return PT_MIXED_READERS_BACKEND;
 if(p->failed)return PT_MIXED_READERS_BACKEND;
 if(p->closing||p->state!=OPEN)return PT_MIXED_READERS_INVALID;
 if(!fixed_current(p,p->revision)||!backend_fixed(p))return PT_MIXED_READERS_STALE;
 p->busy=1;p->queue_call=1;
 result=p->causal_binding?pt_mixed_causal_factory_reader_readiness(p->causal_original.owner,r->ticket,r->action):
 pt_mixed_readers_reader_readiness(p->config.queue,r->ticket,r->action);
 p->queue_call=0;
 /* Real holder-current callbacks may fault or change fixed controls. Do not
  * publish clean waiting or OK after that callback outcome. No key/key_seen
  * is initialized; the actual batch getter still performs that separate work. */
 if(p->failed)result=PT_MIXED_READERS_BACKEND;
 else if(!fixed_current(p,p->revision)||!backend_fixed(p))result=PT_MIXED_READERS_STALE;
 else if(reader(p,h)!=r||r->state!=LIVE)result=PT_MIXED_READERS_INVALID;
 p->busy=0;return result;
}
static int service_ready(struct pt_sampler_mixed_pool *p,const void *out,size_t n)
{if(!p)return 0;
if(out&&!output_apart(p,out,n))return 0;
if(reentry(p))return 0;
if(out&&!fixed_current(p,p->revision))return 0;
p->busy=1;
p->queue_call=1;
return 1;
}
enum pt_mixed_readers_result pt_sampler_mixed_publish(struct pt_sampler_mixed_pool *p,uint64_t ticket)
{enum pt_mixed_readers_result result;
if(!service_ready(p,NULL,0))return PT_MIXED_READERS_BACKEND;
if(!fixed_current(p,p->revision)){p->busy=0;
p->queue_call=0;
return PT_MIXED_READERS_STALE;
}if(p->causal_binding){
 if(ticket==p->causal_first)result=pt_mixed_causal_publish(p->causal_original.owner,ticket);
 else if(p->lineage_binding&&ticket==p->lineage_control&&ticket)
 result=pt_mixed_causal_control_publish(p->causal_original.owner,ticket);
 else if(p->lineage_binding&&ticket==p->lineage_stop&&ticket)
 result=pt_mixed_causal_stop_after_control_publish(p->causal_original.owner,ticket);
 else if(ticket==p->causal_successor&&ticket)result=p->stop_binding?
 pt_mixed_causal_stop_publish(p->causal_original.owner,ticket):
 pt_mixed_causal_publish_successor(p->causal_original.owner,p->causal_first,ticket);
 else result=PT_MIXED_READERS_INVALID;
}else result=pt_mixed_readers_publish(p->config.queue,ticket);
p->queue_call=0;
p->busy=0;
return p->failed?PT_MIXED_READERS_BACKEND:result;
}
enum pt_mixed_readers_result pt_sampler_mixed_service_command(struct pt_sampler_mixed_pool *p,uint64_t ticket,unsigned cancel,struct pt_mixed_readers_command_receipt *out)
{struct pt_mixed_readers_command_receipt receipt;
enum pt_mixed_readers_result result;
if(cancel>1||!service_ready(p,out,sizeof(*out)))return PT_MIXED_READERS_INVALID;
result=p->causal_binding?pt_mixed_causal_service_command(p->causal_original.owner,ticket,cancel,out?&receipt:NULL):
 pt_mixed_readers_service_command(p->config.queue,ticket,cancel,out?&receipt:NULL);
p->queue_call=0;
if(p->lineage_binding&&ticket==p->causal_first&&!cancel&&result==PT_MIXED_READERS_OK)
 p->lineage_root_detached=1;
if(out&&result==PT_MIXED_READERS_OK&&!p->failed&&fixed_current(p,p->revision)&&output_apart(p,out,sizeof(*out)))*out=receipt;
p->busy=0;
return p->failed?PT_MIXED_READERS_BACKEND:result;
}
enum pt_mixed_readers_result pt_sampler_mixed_service_reader(struct pt_sampler_mixed_pool *p,uint64_t ticket,unsigned action,unsigned cancel,struct pt_mixed_readers_reader_receipt *out)
{struct pt_mixed_readers_reader_receipt receipt;
enum pt_mixed_readers_result result;
if(cancel>1||action>=PT_SAMPLER_MIXED_ACTIONS||!service_ready(p,out,sizeof(*out)))return PT_MIXED_READERS_INVALID;
result=p->causal_binding?pt_mixed_causal_service_reader(p->causal_original.owner,ticket,action,cancel,out?&receipt:NULL):
 pt_mixed_readers_service_reader(p->config.queue,ticket,action,cancel,out?&receipt:NULL);
p->queue_call=0;
if(out&&result==PT_MIXED_READERS_OK&&!p->failed&&fixed_current(p,p->revision)&&output_apart(p,out,sizeof(*out)))*out=receipt;
p->busy=0;
return p->failed?PT_MIXED_READERS_BACKEND:result;
}
enum pt_sampler_mixed_result pt_sampler_mixed_cancel(struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_command_handle h)
{struct pt_sampler_mixed_command *c;
if(!p||!(c=command(p,h)))return PT_SAMPLER_MIXED_INVALID;
if(c->state==LIVE)return PT_SAMPLER_MIXED_BUSY;
if(c->state==RETIRED||c->state==CANCELLED)return PT_SAMPLER_MIXED_OK;
if(reentry(p))return PT_SAMPLER_MIXED_BUSY;
p->busy=1;
partial(c,0);
c->state=CANCELLED;
p->busy=0;
return PT_SAMPLER_MIXED_OK;
}
enum pt_sampler_mixed_result pt_sampler_mixed_stop(struct pt_sampler_mixed_pool *p)
{unsigned i;
if(!p)return PT_SAMPLER_MIXED_INVALID;
if(reentry(p))return PT_SAMPLER_MIXED_BUSY;
p->busy=1;
p->closing=1;
pt_project_validation_cancel(&p->validation);
for(i=0;i<p->config.maximum_commands;++i)if(p->commands[i]&&p->commands[i]->state!=LIVE&&p->commands[i]->state!=RETIRED){partial(p->commands[i],0);
p->commands[i]->state=CANCELLED;
}
p->busy=0;
return PT_SAMPLER_MIXED_OK;
}
int pt_sampler_mixed_reader_close(struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_reader_handle *h)
{struct pt_sampler_mixed_reader *r;
if(!span(h,sizeof(*h))||!h)return 0;
if(!h->address&&!h->token)return 1;
if(!p||!output_apart(p,h,sizeof(*h))||!(r=reader(p,*h))||(r->state!=RETIRED&&r->state!=CANCELLED))return 0;
if(reentry(p))return 0;
p->busy=1;
h->address=NULL;
h->token=0;
reader_free(p,r);
p->busy=0;
return !p->failed;
}
static int retirement_guard_has(const struct pt_sampler_mixed_pool *p,const void *v,size_t n)
{
 unsigned i;
 for(i=0;i<p->guard_count;++i){
    if(p->guards[i].data==v&&p->guards[i].bytes==n)return 1;
 }
 return 0;
}
static int retirement_extent_apart(const void *slot,size_t n,const void *v,size_t bytes)
{return (!v&&!bytes)||(v&&bytes&&span(v,bytes)&&apart(slot,n,v,bytes));}
/* This is admission of one narrowly private original retirement handle SLOT,
 * never an exception to public output_apart or a caller release certificate.
 * No sampler_output_disjoint/current-version body is followed after staleness.
 * Card cache data are opaque RAM-block identities, not CPU pointers of logical
 * sample length: their full arena/cache controls are inside captured backend.
 * Paula cache allocations and Chip ledgers use their actual full CPU capacity. */
static int retirement_slot_apart(struct pt_sampler_mixed_pool *p,
 const struct pt_sampler_mixed_reader_handle *slot)
{
 unsigned i,commands=0,readers=0;size_t n=sizeof(*slot),total;
 if(!p||!span(p,sizeof(*p))||(uintptr_t)p%_Alignof(struct pt_sampler_mixed_pool)||
    !slot||!span(slot,n)||(uintptr_t)slot%_Alignof(struct pt_sampler_mixed_reader_handle)||
    !apart(slot,n,p,sizeof(*p))||!p->guard_count||p->guard_count>GUARDS||
    !p->config.maximum_commands||p->config.maximum_commands>PT_SAMPLER_MIXED_COMMANDS||
    !p->config.maximum_readers||p->config.maximum_readers>PT_SAMPLER_MIXED_READERS||
    p->config.context_count>PT_SAMPLER_MIXED_CONTEXTS||p->header.sample_count>PT_PROJECT_SAMPLES||
    p->header.channels.count>PT_CHANNEL_LIMIT||p->header.pattern_count>PT_PROJECT_PATTERNS||
    p->header.order_count>PT_PROJECT_ORDERS||p->header.extension_count>4090||
    !p->serial||p->bytes<sizeof(*p)||p->bytes>p->config.control_budget||
    p->chip_bytes>p->config.chip_budget||!p->config.allocator.release)return 0;
 /* All captured extents are admitted before any owned-pointer header read. */
 for(i=0;i<p->guard_count;++i)if(!p->guards[i].data||!p->guards[i].bytes||
    !retirement_extent_apart(slot,n,p->guards[i].data,p->guards[i].bytes))return 0;
 for(i=0;i<PT_SAMPLER_MIXED_CONTEXTS;++i)
    if(i<p->config.context_count?(!p->config.contexts[i].data||!p->config.contexts[i].bytes||
       !retirement_extent_apart(slot,n,p->config.contexts[i].data,p->config.contexts[i].bytes)):
       (p->config.contexts[i].data||p->config.contexts[i].bytes))return 0;
 if(!retirement_guard_has(p,p->config.queue,pt_mixed_readers_control_size())||
    !retirement_guard_has(p,p->config.backend,sizeof(*p->config.backend))||
    !retirement_guard_has(p,p->backend_header.reservation,sizeof(*p->backend_header.reservation))||
    !retirement_guard_has(p,p->config.sampler,sizeof(*p->config.sampler))||
    !retirement_guard_has(p,p->config.project,sizeof(*p->config.project)))return 0;
 for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(p->commands[i]){
    if(i>=p->config.maximum_commands||!span(p->commands[i],sizeof(*p->commands[i]))||
       (uintptr_t)p->commands[i]%_Alignof(struct pt_sampler_mixed_command)||
       !apart(slot,n,p->commands[i],sizeof(*p->commands[i]))){return 0;}
    ++commands;
 }
 for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(p->readers[i]){
    if(i>=p->config.maximum_readers||!span(p->readers[i],sizeof(*p->readers[i]))||
       (uintptr_t)p->readers[i]%_Alignof(struct pt_sampler_mixed_reader)||
       !apart(slot,n,p->readers[i],sizeof(*p->readers[i]))){return 0;}
    ++readers;
 }
 total=sizeof(*p)+commands*sizeof(struct pt_sampler_mixed_command)+readers*sizeof(struct pt_sampler_mixed_reader);
 if(total!=p->bytes)return 0;
 for(i=0;i<PT_CACHE_SLOTS;++i){const struct pt_cache_entry *c=p->paula.cache.entry+i;
    if(!retirement_extent_apart(slot,n,p->chip[i].data,p->chip[i].bytes)||
       !retirement_extent_apart(slot,n,c->data,c->bytes)||c->valid>2)return 0;
 }
 return pt_mixed_readers_retirement_slot_disjoint(p->config.queue,slot,n);
}
/* Readable captured fixed headers only; never dereference former sample tables,
 * version bodies, event arrays, extension arrays or unknown cache RAM pointers. */
static int retirement_headers_current(struct pt_sampler_mixed_pool *p)
{
 struct pt_sampler *s=p->config.sampler;struct pt_project h;unsigned i;
 if(s->generation!=p->generation||s->table!=p->table||s->table_original!=p->table_original||
    s->table_bytes!=p->table_bytes||s->allocator.context!=p->sampler_allocator.context||
    s->allocator.allocate!=p->sampler_allocator.allocate||s->allocator.release!=p->sampler_allocator.release)return 0;
 memcpy(&h,&p->header,sizeof(h));h.channels.selected=p->config.project->channels.selected;
 if(!pt_project_snapshot_equal(p->config.project,&h)||!backend_fixed(p))return 0;
 for(i=0;i<p->header.sample_count;++i)if(s->current[i]!=p->masters[i])return 0;
 return 1;
}
struct pt_mixed_reader_retirement pt_sampler_mixed_retire_original_reader(
 struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_reader_handle *slot,unsigned cancel)
{
 struct pt_mixed_reader_retirement out={PT_MIXED_READERS_INVALID,0};
 struct pt_sampler_mixed_reader *r;struct pt_sampler_mixed_reader_handle original;unsigned stale;
 if(cancel>1||!retirement_slot_apart(p,slot))return out;
 original=*slot;
 if(!original.address||!original.token||original.token>p->serial||!(r=reader(p,original))||
    r->pool!=p||!r->ticket||r->action>=PT_MIXED_READERS_ACTIONS||
    (r->state!=LIVE&&r->state!=RETIRED)||r->terminal>1||r->released>1||r->valid>1||
    r->leased>1||r->span_count>PT_MIXED_READERS_SPANS||
    (r->state==RETIRED&&(!r->terminal||!r->released))||
    (r->state==LIVE&&(r->terminal||r->released)))return out;
 if(reentry(p)){out.result=PT_MIXED_READERS_BACKEND;return out;}
 stale=(unsigned)!retirement_headers_current(p);p->busy=1;
 if(r->state==RETIRED){out.retirement_consumed=1;
    out.result=r->valid?PT_MIXED_READERS_OK:PT_MIXED_READERS_BACKEND;
 }else{p->queue_call=1;
    out=p->causal_binding?pt_mixed_causal_factory_retire_original(p->causal_original.owner,r->ticket,r->action,cancel):
     pt_mixed_readers_retire_original(p->config.queue,r->ticket,r->action,cancel);
    p->queue_call=0;
 }
 /* Core consumed R may still be retained by C; only actual terminal release
  * makes this holder closeable. Clear the real admitted slot BEFORE free.
  * Do not follow r after its allocator callback, even if that callback faults. */
 if(r->state==RETIRED&&r->terminal==1&&r->released==1){
    if(!out.retirement_consumed){out.result=PT_MIXED_READERS_BACKEND;out.retirement_consumed=1;}
    slot->address=NULL;slot->token=0;reader_free(p,r);
 }
 if(p->failed)out.result=PT_MIXED_READERS_BACKEND;
 else if(stale||!retirement_headers_current(p))out.result=PT_MIXED_READERS_STALE;
 p->busy=0;return out;
}
int pt_sampler_mixed_command_close(struct pt_sampler_mixed_pool *p,struct pt_sampler_mixed_command_handle *h)
{struct pt_sampler_mixed_command *c;
unsigned i,root_close=0;
if(!span(h,sizeof(*h))||!h)return 0;
if(!h->address&&!h->token)return 1;
if(!p||!output_apart(p,h,sizeof(*h))||!(c=command(p,*h))||c->state==LIVE)return 0;
if(reentry(p))return 0;
p->busy=1;
if(c->state!=RETIRED){partial(c,0);
for(i=0;i<c->count;++i)if(c->source[i].reader){struct pt_sampler_mixed_reader *r=reader(p,(struct pt_sampler_mixed_reader_handle){c->source[i].reader,c->source[i].token});
if(r&&r->state!=LIVE&&r->state!=RETIRED)reader_free(p,r);
}}
root_close=p->lineage_binding&&c->token==p->lineage_root_token&&
 c->ticket==p->causal_first&&c->state==RETIRED&&c->terminal&&c->valid&&p->lineage_root_detached;
h->address=NULL;
h->token=0;
command_free(p,c);
/* Observe actual caller NULL consumption after the release callback; never
 * touch the disposed C again, and retain no pointer for later preparation. */
if(root_close&&!h->address&&!h->token)p->lineage_root_closed=1;
p->busy=0;
return !p->failed;
}
int pt_sampler_mixed_close(struct pt_sampler_mixed_pool **slot)
{struct pt_sampler_mixed_pool *p;
struct pt_allocator allocator;
unsigned i,fault=0;
if(!span(slot,sizeof(*slot))||!slot)return 0;
if(!*slot)return 1;
p=*slot;
if(!output_apart(p,slot,sizeof(*slot))||reentry(p))return 0;
p->closing=1;
pt_project_validation_cancel(&p->validation);
for(i=0;i<p->config.maximum_commands;++i)if(p->commands[i])return 0;
for(i=0;i<p->config.maximum_readers;++i)if(p->readers[i])return 0;
p->busy=1;
p->release_fault=&fault;
if(!pt_cache_clear(&p->paula.cache)){p->release_fault=NULL;
p->busy=0;
return 0;
}/* Private identity is retired, but the borrowed backend stays attached. */pt_cache_invalidate(&p->config.backend->cache,1);
if(!pt_cache_clear(&p->config.backend->cache)){p->release_fault=NULL;
p->busy=0;
return 0;
}allocator=p->config.allocator;
if(p->causal_binding)p->causal_binding->phase=CB_CONSUMED;
p->release_fault=&fault;
*slot=NULL;
allocator.release(allocator.context,p);
return !fault;
}
