#define PT_WAVETABLE_NATIVE
#include "amigus_wavetable_cache_test.c"
#include "../src/core/mixed_scheduled_readers.h"
#include "../src/editor/sampler_paula.h"
#include "../src/editor/sampler_wavetable.h"
#include "../src/editor/sampler_internal.h"
#include <stdio.h>
struct allocation {
    unsigned calls,releases,fail,hook,consumed;void *alias,*last,*workspace;
    size_t workspace_capacity;const struct pt_mixed_readers_config *config;
    struct pt_mixed_readers_output **slot,*captured;
};
static void *ordinary(void *context,size_t bytes)
{struct allocation *a=context;void *p;++a->calls;
 if(a->hook){a->hook=0;++a->consumed;assert(pt_mixed_readers_open(a->config,a->workspace,a->workspace_capacity,a->slot)==PT_MIXED_READERS_BACKEND);}
 if(a->fail)return NULL;
 p=a->alias?a->alias:malloc(bytes);a->last=p;return p;}
static void ordinary_release(void *context,void *p)
{struct allocation *a=context;++a->releases;
 if(a->hook){a->hook=0;++a->consumed;assert(pt_mixed_readers_stop(a->captured)==PT_MIXED_READERS_BACKEND);}
 free(p);}
static void *master_alloc(void *c,size_t n){(void)c;return malloc(n);}
static void master_free(void *c,void *p){(void)c;free(p);}
static void *chip_new(void *c,size_t n){unsigned *count=c;++*count;return malloc(n);}
static void chip_drop(void *c,void *p,size_t n){unsigned *count=c;(void)n;assert(*count);--*count;free(p);}
struct resources {
    struct pt_document document;struct pt_sampler sampler;
    struct pt_sampler_paula paula;struct pt_sampler_wavetable wave;
    struct fixture card;int32_t original[2][64];unsigned chips,bits,cache_bits,little,loop_mode;
};
struct holder {
    struct resources *resources;struct pt_sample_version *pin;struct pt_pcm pcm;
    struct pt_cache_lease lease;unsigned route,sample,track,live,currents,terminals,releases,valid,hook,consumed;
    uint64_t token;struct pt_mixed_readers_output *queue;
    struct pt_mixed_readers_span spans[PT_MIXED_READERS_SPANS];unsigned count;
};
static int held_current(void *context,uint64_t token,uint64_t generation)
{struct holder *h=context;struct pt_pcm pcm;struct pt_sample_version *pin;
 ++h->currents;assert(token==h->token);
 if(h->hook){h->hook=0;++h->consumed;assert(pt_mixed_readers_stop(h->queue)==PT_MIXED_READERS_BACKEND);}
 if(!h->live||generation!=17)return 0;
 if(!h->pin)return 1;
 if(pt_sampler_pin_current(&h->resources->sampler,&h->resources->document.project,h->sample,
     h->resources->sampler.generation,h->pin,&pcm,&pin)!=PT_EDIT_OK)return 0;
 pt_sampler_unpin(pin);
 if(h->route==PT_MIXED_READERS_PAULA)return pt_cache_data(&h->resources->paula.cache,h->lease)!=NULL;
 return pt_cache_data(&h->resources->card.cache.cache,h->lease)!=NULL&&bus_owned(&h->resources->card)==1;}
static void held_terminal(void *context,uint64_t token,int valid)
{struct holder *h=context;assert(h->live&&token==h->token&&!h->terminals);++h->terminals;h->valid=(unsigned)valid;}
static void held_release(void *context,uint64_t token)
{struct holder *h=context;assert(h->live&&token==h->token&&h->terminals==1&&!h->releases);
 ++h->releases;h->live=0;
 if(h->pin){
  if(h->route==PT_MIXED_READERS_PAULA)assert(pt_sampler_paula_unpin(&h->resources->paula,h->lease));
  else assert(pt_sampler_wavetable_unpin(&h->resources->wave,h->lease));
  pt_sampler_unpin(h->pin);h->pin=NULL;
 }}
static struct pt_mixed_readers_control control_of(struct holder *h)
{return (struct pt_mixed_readers_control){h,sizeof(*h),h->token,held_current,held_terminal,held_release};}
static void resource_init(struct resources *r,unsigned bits,unsigned cache_bits)
{struct pt_allocator a={NULL,master_alloc,master_free};unsigned i,j;
 memset(r,0,sizeof(*r));r->bits=bits;r->cache_bits=cache_bits;
 init(&r->card,PT_AMIGUS_WAVETABLE);assert(pt_amigus_wavetable_cache_attach(&r->card.cache,&r->card.reservation,0,4096,4096,&r->card,bus_owned,bus_write));
 pt_document_init(&r->document,&a);assert(pt_document_new(&r->document,16,SIZE_MAX)==PT_PROJECT_OK);
 for(i=0;i<16;++i)r->document.project.channels.track[i].route=i<4?PT_PAULA:PT_AMIGUS;
 for(i=0;i<2;++i){for(j=0;j<64;++j)r->original[i][j]=(int32_t)(j%60)+1+(int32_t)i;
  r->document.project.samples[i].pcm=(struct pt_pcm){r->original[i],64,64,8000,1,(uint8_t)bits};
  r->document.project.samples[i].volume=64;}
 pt_sampler_init(&r->sampler,&a,SIZE_MAX);
 assert(pt_sampler_paula_bind(&r->paula,&r->sampler,&r->document.project,&r->chips,chip_new,chip_drop,4096));
 assert(pt_sampler_wavetable_bind(&r->wave,&r->sampler,&r->document.project,&r->card.cache));}
static void resource_drop(struct resources *r)
{assert(pt_sampler_paula_close(&r->paula));assert(!r->chips);
 assert(pt_sampler_wavetable_close(&r->wave));assert(pt_amigus_reservation_close(&r->card.reservation));
 pt_sampler_release(&r->sampler);pt_document_release(&r->document);}
static uint8_t *save(struct resources *r,size_t *n)
{uint8_t *p;size_t used;assert(pt_project_size(&r->document.project,n)==PT_PROJECT_OK);p=malloc(*n);assert(p);
 assert(pt_project_encode(&r->document.project,p,*n,&used)==PT_PROJECT_OK&&used==*n);return p;}
static void same_save(struct resources *r,const uint8_t *before,size_t n)
{size_t m;uint8_t *after=save(r,&m);assert(m==n&&!memcmp(before,after,n));free(after);
 assert(!memcmp(r->original[0],r->document.project.samples[0].pcm.data,sizeof(r->original[0])));
 assert(!memcmp(r->original[1],r->document.project.samples[1].pcm.data,sizeof(r->original[1])));}
static void holder_init(struct holder *h,uint64_t token)
{memset(h,0,sizeof(*h));h->token=token;h->live=h->valid=1;}
static struct pt_mixed_readers_owner source(struct holder *h,struct resources *r,unsigned route,unsigned slot,unsigned sample,uint64_t token)
{struct pt_mixed_readers_owner o={0};struct pt_sampler_storage_span span[PT_SAMPLER_VERSION_SPANS];unsigned n,i;
 struct pt_playback_format format={(uint8_t)r->cache_bits,0,(uint8_t)r->little,0};struct pt_sampler_upload_job job={0};
 uint8_t staging[256];enum pt_cache_result result;uint32_t address,bytes;
 holder_init(h,token);h->resources=r;h->route=route;h->sample=sample;h->track=slot;
 assert(pt_sampler_pin(&r->sampler,&r->document.project,sample,r->sampler.generation,&h->pcm,&h->pin)==PT_EDIT_OK);
 assert(pt_sampler_version_spans(h->pin,span,PT_SAMPLER_VERSION_SPANS,&n));
 for(i=0;i<n;++i)h->spans[h->count++]=(struct pt_mixed_readers_span){span[i].data,span[i].bytes};
 if(route==PT_MIXED_READERS_PAULA){const uint8_t *data;size_t count;
  assert(pt_sampler_paula_acquire(&r->paula,slot,sample,0,&h->lease)==PT_CACHE_LOAD||pt_cache_data(&r->paula.cache,h->lease));
  assert(pt_sampler_paula_location(&r->paula,slot,h->lease,&data,&count));
  h->spans[h->count++]=(struct pt_mixed_readers_span){data,count};
  h->spans[h->count++]=(struct pt_mixed_readers_span){&r->paula,sizeof(r->paula)};
 }else{
  result=pt_sampler_upload_begin(&job,&r->wave,sample,&format,&h->lease);
  assert(result==PT_CACHE_PENDING||result==PT_CACHE_HIT);
  while(result==PT_CACHE_PENDING)result=pt_sampler_upload_step(&job,staging,sizeof(staging),&h->lease);
  assert(result==PT_CACHE_LOAD||result==PT_CACHE_HIT);assert(!job.pin);assert(h->pin);
  assert(pt_sampler_wavetable_location(&r->wave,h->lease,&address,&bytes));
  for(i=0;i<64;++i){uint32_t value=(uint32_t)r->original[sample][i];unsigned shift;
   if(r->cache_bits>=r->bits)value<<=r->cache_bits-r->bits;
   else{shift=r->bits-r->cache_bits;value=(value+(1U<<(shift-1)))>>shift;}
   if(r->cache_bits==8)assert(r->card.ram[address+i]==(uint8_t)value);
   else{assert(r->card.ram[address+2*i]==(uint8_t)(r->little?value:value>>8));
    assert(r->card.ram[address+2*i+1]==(uint8_t)(r->little?value>>8:value));}
  }
  o.card=(struct pt_mixed_readers_card){&r->card.reservation,&r->card.cache,r->wave.version,h->lease.serial,h->lease.slot,r->cache_bits,r->little,0,address,bytes,r->card.cache.arena.block[h->lease.slot].reserved};
  h->spans[h->count++]=(struct pt_mixed_readers_span){&r->card.cache,sizeof(r->card.cache)};
  h->spans[h->count++]=(struct pt_mixed_readers_span){&r->card.reservation,sizeof(r->card.reservation)};
 }
 o.control=control_of(h);o.spans=h->spans;o.count=h->count;return o;}
struct model_reader {const struct pt_mixed_readers_domain *reference;struct pt_mixed_readers_key key;
 unsigned adopted,state,retired;uint64_t observed,issued;};
struct model_command {const struct pt_mixed_readers_event *reference;struct pt_mixed_readers_batch batch;
 struct pt_mixed_readers_key key[16];uint64_t ticket,first,last;unsigned issued,cancelled,detached,fired;};
struct model {
 struct pt_mixed_readers_output *queue;uint64_t ticks;unsigned reads,submits,command_calls,reader_calls,hook,consumed,effects;
 int submit_result;unsigned command_pending,reader_pending,malformed,late,partial;
 struct model_command command[2];struct model_reader reader[32];struct pt_mixed_readers_key active[20];
};
static int keys_equal(const struct pt_mixed_readers_key *a,const struct pt_mixed_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->route==b->route&&a->slot==b->slot;}
static struct model_reader *model_reader(struct model *m,const struct pt_mixed_readers_key *key)
{unsigned i;for(i=0;i<32;++i)if(m->reader[i].key.serial&&keys_equal(&m->reader[i].key,key))return m->reader+i;return NULL;}
static struct model_command *model_command(struct model *m,uint64_t ticket)
{unsigned i;for(i=0;i<2;++i)if(m->command[i].ticket==ticket)return m->command+i;return NULL;}
static void model_hook(struct model *m,unsigned kind)
{if(m->hook==kind){m->hook=0;++m->consumed;assert(pt_mixed_readers_stop(m->queue)==PT_MIXED_READERS_BACKEND);}}
static int model_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct model *m=context;++m->reads;model_hook(m,1);*ticks=m->ticks;*frequency=709379;return 1;}
static uint64_t oracle(uint64_t frame)
{return 100+(frame*709379+47999)/48000;}
static int model_submit(void *context,const struct pt_mixed_readers_event *e)
{struct model *m=context;struct model_command *c;unsigned i,j;++m->submits;
 assert(e->first==oracle(e->batch.frame)&&e->last==oracle(e->batch.frame+1));assert(m->ticks<e->first);
 model_hook(m,2);if(!m->submit_result)return 0;
 for(i=0;i<2;++i){if(!m->command[i].reference)break;}
 assert(i<2);c=m->command+i;memset(c,0,sizeof(*c));
 c->reference=e;c->ticket=e->ticket;c->batch=e->batch;c->first=e->first;c->last=e->last;
 for(i=0;i<e->batch.count;++i){c->key[i]=e->reader[i]->key;
  if(e->batch.action[i].kind==PT_MIXED_READERS_TRIGGER){
   for(j=0;j<32;++j){if(!m->reader[j].key.serial)break;}
   assert(j<32);
   m->reader[j].reference=e->reader[i];m->reader[j].key=e->reader[i]->key;m->reader[j].state=PT_MIXED_READER_RESERVED;
  }}
 return m->submit_result;}
/* Autonomous software model: activation uses COPIED values/keys only, no queue,
 * original event/domain, holder callback, sampler, converter or allocator. */
static void fire(struct model *m,uint64_t ticket)
{struct model_command *c=model_command(m,ticket);unsigned i,index;struct model_reader *r;
 assert(c&&!c->cancelled&&!c->issued);m->ticks=c->first;assert(m->ticks<c->last);
 for(i=0;i<c->batch.count;++i){const struct pt_mixed_readers_action *a=c->batch.action+i;index=a->route==PT_MIXED_READERS_PAULA?a->slot:4+a->slot;
  if(a->kind!=PT_MIXED_READERS_TRIGGER)assert(keys_equal(m->active+index,c->key+i));}
 for(i=0;i<c->batch.count;++i){const struct pt_mixed_readers_action *a=c->batch.action+i;index=a->route==PT_MIXED_READERS_PAULA?a->slot:4+a->slot;
  r=model_reader(m,c->key+i);assert(r);
  if(a->kind==PT_MIXED_READERS_TRIGGER){struct model_reader *old=model_reader(m,m->active+index);if(old)old->state=PT_MIXED_READER_DRAINING;
   m->active[index]=c->key[i];r->adopted=1;r->state=PT_MIXED_READER_ACTIVE;r->observed=r->issued=m->ticks;
  }else if(a->kind==PT_MIXED_READERS_STOP){r->state=PT_MIXED_READER_DRAINING;memset(m->active+index,0,sizeof(*m->active));}
  ++m->effects;++c->fired;if(m->partial)break;
 }
 c->issued=1;}
static enum pt_mixed_readers_reply model_command_service(void *context,uint64_t ticket,unsigned cancel,struct pt_mixed_readers_command_receipt *out)
{struct model *m=context;struct model_command *c=model_command(m,ticket);const struct pt_mixed_readers_event *e;unsigned i;
 ++m->command_calls;assert(c&&c->reference);model_hook(m,3);if(m->command_pending)return PT_MIXED_PENDING;
 if(cancel){c->cancelled=1;}
 e=c->reference;memset(out,0,sizeof(*out));
 out->domain=PT_MIXED_COMMAND_DOMAIN;out->queue=e->queue;out->session=e->session;out->generation=e->batch.generation;
 out->ticket=e->ticket;out->owner=e->command_owner;out->count=e->batch.count;out->event=e;out->binding=e->binding;
 for(i=0;i<out->count;++i){struct model_reader *r=model_reader(m,c->key+i);assert(r);out->action[i].key=r->key;
  out->action[i].command=c->issued?(c->cancelled?PT_MIXED_COMMAND_CANCELLED_AFTER:PT_MIXED_COMMAND_ISSUED):(c->cancelled?PT_MIXED_COMMAND_CANCELLED_BEFORE:PT_MIXED_COMMAND_WAITING);
  out->action[i].reader=(enum pt_mixed_readers_state)(c->issued?r->state:PT_MIXED_READER_NONE);
  out->action[i].adoption=(enum pt_mixed_readers_adoption)r->adopted;
  if(c->issued&&i<c->fired)out->action[i].observed=out->action[i].issued=c->first;
  if(c->issued&&i>=c->fired)out->action[i].command=PT_MIXED_COMMAND_UNKNOWN;
 }
 if(m->malformed==1)out->owner++;
 if(m->malformed==2)out->action[0].issued=c->last;
 if(c->issued||cancel){if(m->malformed!=1)c->reference=NULL;c->detached=1;return PT_MIXED_COMMAND_DETACHED;}
 return PT_MIXED_OBSERVATION;}
static enum pt_mixed_readers_reply model_reader_service(void *context,const struct pt_mixed_readers_domain *d,unsigned cancel,struct pt_mixed_readers_reader_receipt *out)
{struct model *m=context;struct model_reader *r=model_reader(m,&d->key);unsigned index;
 ++m->reader_calls;assert(r);model_hook(m,4);if(m->reader_pending)return PT_MIXED_PENDING;
 memset(out,0,sizeof(*out));out->domain=PT_MIXED_READER_DOMAIN;out->key=d->key;out->reference=d;out->binding=d->binding;
 if(cancel){r->state=PT_MIXED_READER_RETIRED;r->retired=1;r->reference=NULL;
  index=d->key.route==PT_MIXED_READERS_PAULA?d->key.slot:4+d->key.slot;
  if(keys_equal(m->active+index,&d->key))memset(m->active+index,0,sizeof(*m->active));}
 out->state=(enum pt_mixed_readers_state)r->state;out->adoption=(enum pt_mixed_readers_adoption)r->adopted;out->observed=r->observed;out->issued=r->issued;
 if(m->malformed==3)out->key.route=0;
 if(m->malformed==4)out->state=PT_MIXED_READER_UNKNOWN;
 return cancel?PT_MIXED_READER_RETIRE_PROOF:PT_MIXED_OBSERVATION;}
struct trial {
 struct resources *resources;struct allocation allocator;struct model model;
 struct pt_mixed_readers_config config;struct pt_mixed_readers_output *queue;void *workspace;
 struct holder command[4],reader[64];struct pt_mixed_readers_inputs *input;
};
static struct trial *trial_make(unsigned bits,unsigned cache_bits)
{struct trial *t=calloc(1,sizeof(*t));assert(t);t->resources=calloc(1,sizeof(*t->resources));assert(t->resources);
 resource_init(t->resources,bits,cache_bits);t->model.ticks=100;t->model.submit_result=1;
 t->config.allocator=(struct pt_allocator){&t->allocator,ordinary,ordinary_release};
 t->config.allocator_context=(struct pt_mixed_readers_span){&t->allocator,sizeof(t->allocator)};
 t->config.backend=(struct pt_mixed_readers_backend){&t->model,sizeof(t->model),PT_MIXED_READERS_VERSION,PT_MIXED_READERS_REQUIRED,model_clock,model_submit,model_command_service,model_reader_service};
 t->config.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};t->config.session=19;t->config.control_budget=pt_mixed_readers_control_size();
 t->workspace=calloc(1,pt_mixed_readers_workspace_size());t->input=calloc(1,sizeof(*t->input));assert(t->workspace&&t->input);
 t->allocator.config=&t->config;t->allocator.workspace=t->workspace;t->allocator.workspace_capacity=pt_mixed_readers_workspace_size();t->allocator.slot=&t->queue;
 return t;}
static void open_trial(struct trial *t)
{assert(pt_mixed_readers_open(&t->config,t->workspace,pt_mixed_readers_workspace_size(),&t->queue)==PT_MIXED_READERS_OK);assert(t->queue);t->model.queue=t->allocator.captured=t->queue;}
static void trial_drop(struct trial *t)
{assert(pt_mixed_readers_close(&t->queue)&&!t->queue);resource_drop(t->resources);free(t->resources);free(t->workspace);free(t->input);free(t);}
static void trigger_input(struct trial *t,unsigned count,unsigned generation,uint64_t frame)
{unsigned i;struct pt_amigus_voice_request request={8000,1,0,64,128};struct pt_playback_format format={(uint8_t)t->resources->cache_bits,0,(uint8_t)t->resources->little,0};
 memset(t->input,0,sizeof(*t->input));holder_init(t->command+generation,100+generation);t->command[generation].queue=t->queue;
 t->input->command=control_of(t->command+generation);t->input->batch=(struct pt_mixed_readers_batch){17,frame,count,{{0}}};
 for(i=0;i<count;++i){unsigned route=i<4?PT_MIXED_READERS_PAULA:PT_MIXED_READERS_AMIGUS;unsigned slot=i<4?i:i-4;unsigned sample=t->resources->loop_mode?(i<4?0U:1U):i%2;
  struct pt_mixed_readers_action *a=t->input->batch.action+i;struct holder *h=t->reader+generation*16+i;
  a->route=route;a->slot=slot;a->kind=PT_MIXED_READERS_TRIGGER;
  t->input->reader[i]=source(h,t->resources,route,slot,sample,1000+generation*16+i);h->queue=t->queue;
  if(route==PT_MIXED_READERS_PAULA){const uint8_t *p;size_t n;assert(pt_sampler_paula_location(&t->resources->paula,slot,h->lease,&p,&n));
   a->geometry.paula.data=p;a->geometry.paula.words=(uint16_t)(n/2);a->geometry.paula.period=428;a->geometry.paula.volume=64;
  }else assert(pt_amigus_voice_plan_prepare(t->resources->document.project.samples+sample,&format,&request,
      t->input->reader[i].card.address,t->input->reader[i].card.logical_bytes,&a->geometry.amigus));
 }}
static void unused_drop(struct trial *t,unsigned count,unsigned generation)
{unsigned i;held_terminal(t->command+generation,t->command[generation].token,1);held_release(t->command+generation,t->command[generation].token);
 for(i=0;i<count;++i){struct holder *h=t->reader+generation*16+i;held_terminal(h,h->token,1);held_release(h,h->token);}}
static void drain(struct trial *t,uint64_t ticket,unsigned count,unsigned reader_first,unsigned cancel_command)
{unsigned i;if(!reader_first)assert(pt_mixed_readers_service_command(t->queue,ticket,cancel_command,NULL)==PT_MIXED_READERS_OK);
 for(i=0;i<count;++i)assert(pt_mixed_readers_service_reader(t->queue,ticket,i,1,NULL)==PT_MIXED_READERS_OK);
 if(reader_first)assert(pt_mixed_readers_service_command(t->queue,ticket,cancel_command,NULL)==PT_MIXED_READERS_OK);}
static void paired_lifetime(unsigned bits,unsigned cache_bits,unsigned order)
{struct trial *t=trial_make(bits,cache_bits);uint64_t ticket;unsigned i;size_t n;uint8_t *before=save(t->resources,&n);
 open_trial(t);trigger_input(t,16,0,960);assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
 assert(ticket==1&&pt_mixed_readers_commands_held(t->queue)==1&&pt_mixed_readers_readers_held(t->queue)==16);
 assert(pt_mixed_readers_reader_key(t->queue,ticket,0,&t->input->target[0])==PT_MIXED_READERS_STALE);
 assert(!pt_mixed_readers_close(&t->queue));assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_OK);
 assert(t->model.reads==1&&t->model.submits==1&&!t->model.effects);fire(&t->model,ticket);assert(t->model.effects==16);
 for(i=0;i<16;++i){struct pt_mixed_readers_key key;assert(pt_mixed_readers_service_reader(t->queue,ticket,i,0,NULL)==PT_MIXED_READERS_PENDING);
  assert(pt_mixed_readers_reader_key(t->queue,ticket,i,&key)==PT_MIXED_READERS_OK&&key.route==(i<4?1U:2U));}
 same_save(t->resources,before,n);drain(t,ticket,16,order,0);assert(!pt_mixed_readers_readers_held(t->queue));
 for(i=0;i<16;++i){assert(t->reader[i].releases==1&&t->reader[i].valid);}
 assert(t->command[0].releases==1);
 same_save(t->resources,before,n);free(before);trial_drop(t);}
static void constructor_admission(unsigned mode)
{struct trial *t=trial_make(8,8);struct pt_mixed_readers_output *sentinel=(void *)(uintptr_t)1;
 unsigned calls=t->allocator.calls,release=t->allocator.releases;enum pt_mixed_readers_result result;
 void *workspace=t->workspace;size_t capacity=pt_mixed_readers_workspace_size();
 t->queue=sentinel;
 switch(mode){
 case 0:t->config.control_budget--;break;
 case 1:capacity--;break;
 case 2:workspace=(uint8_t *)workspace+1;break;
 case 3:t->config.backend.flags^=1;break;
 case 4:t->config.backend.reader=NULL;break;
 case 5:t->config.allocator.allocate=NULL;break;
 case 6:t->config.allocator_context.bytes=0;break;
 case 7:t->config.backend.context_bytes=0;break;
 case 8:t->config.grid.frequency=100;break;
 case 9:t->config.session=0;break;
 case 10:t->config.allocator_context=(struct pt_mixed_readers_span){t->workspace,capacity};break;
 case 11:t->config.backend.context=t->workspace;t->config.backend.context_bytes=capacity;break;
 case 12:capacity=SIZE_MAX;break;
 case 13:t->allocator.alias=&t->config;break;
 case 14:t->allocator.alias=t->workspace;break;
 case 15:t->allocator.alias=&t->queue;break;
 case 16:t->allocator.alias=&t->allocator;break;
 case 17:t->allocator.alias=&t->model;break;
 case 18:t->allocator.fail=1;break;
 case 19:t->allocator.hook=1;break;
 }
 result=pt_mixed_readers_open(&t->config,workspace,capacity,&t->queue);
 assert(result!=PT_MIXED_READERS_OK&&t->queue==sentinel);
 if(mode<13)assert(t->allocator.calls==calls&&t->allocator.releases==release);
 else if(mode<18)assert(t->allocator.calls==calls+1&&t->allocator.releases==release);
 else if(mode==18)assert(t->allocator.calls==calls+1&&t->allocator.releases==release);
 else assert(result==PT_MIXED_READERS_BACKEND&&t->allocator.calls==calls+1&&t->allocator.releases==release+1&&t->allocator.consumed==1&&!t->allocator.hook);
 t->queue=NULL;t->allocator.alias=NULL;t->allocator.fail=t->allocator.hook=0;trial_drop(t);}
static void input_admission(unsigned bits,unsigned cache_bits,unsigned mode)
{struct trial *t=trial_make(bits,cache_bits);uint64_t ticket=123;unsigned i,currents=0;
 open_trial(t);trigger_input(t,5,0,960);
 switch(mode){
 case 0:t->input->batch.action[1].slot=0;break;
 case 1:t->input->batch.action[0].slot=4;break;
 case 2:t->input->batch.action[4].slot=16;break;
 case 3:t->input->batch.count=17;break;
 case 4:t->input->batch.generation++;break;
 case 5:t->input->reader[4].card.bits=24;break;
 case 6:t->input->reader[4].card.logical_bytes++;break;
 case 7:t->input->reader[4].card.full_capacity--;break;
 case 8:t->input->reader[4].card.address=PT_AMIGUS_RAM_ADDRESS_SPACE-4;break;
 case 9:t->input->reader[4].card.reservation=NULL;break;
 case 10:t->input->reader[4].card.serial=0;break;
 case 11:t->input->batch.action[4].geometry.amigus.control=0;break;
 case 12:t->input->batch.action[4].geometry.amigus.control|=0x10;break;
 case 13:t->input->batch.action[4].geometry.amigus.control^=1;break;
 case 14:t->input->batch.action[4].geometry.amigus.start++;break;
 case 15:t->input->batch.action[4].geometry.amigus.end_exclusive+=2;break;
 case 16:t->input->batch.action[4].geometry.amigus.rate=0x40000001;break;
 case 17:t->input->batch.action[0].geometry.paula.data++;break;
 case 18:t->input->batch.action[0].geometry.paula.words=65535;break;
 case 19:t->input->batch.action[0].geometry.paula.period=0;break;
 case 20:t->input->batch.action[0].geometry.paula.volume=65;break;
 case 21:t->input->reader[1].control=t->input->reader[0].control;break;
 case 22:t->input->command=t->input->reader[0].control;break;
 case 23:t->input->reader[0].count=129;break;
 case 24:t->input->reader[0].spans=NULL;break;
 case 25:t->input->reader[0].control.current=NULL;break;
 case 26:t->input->target[0].serial=1;break;
 case 27:t->input->reader[0].card=t->input->reader[4].card;break;
 case 28:t->input->batch.action[4].route=3;break;
 case 29:t->input->batch.action[4].kind=PT_MIXED_READERS_CONTROL;break;
 }
 assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_INVALID&&ticket==123);
 for(i=0;i<5;++i){currents+=t->reader[i].currents;}
 assert(!currents&&!t->command[0].currents);
 assert(!pt_mixed_readers_commands_held(t->queue)&&!pt_mixed_readers_readers_held(t->queue));unused_drop(t,5,0);trial_drop(t);}
static void allocation_aliases(unsigned bits,unsigned cache_bits)
{struct trial *t=trial_make(bits,cache_bits);uint64_t ticket=1;void *outs[5];unsigned i;
 open_trial(t);trigger_input(t,5,0,960);
 outs[0]=t->queue;outs[1]=&t->allocator;outs[2]=&t->model;outs[3]=(uint8_t *)t->reader[0].pcm.data+t->reader[0].pcm.capacity*sizeof(int32_t)-8;outs[4]=t->reader+4;
 for(i=0;i<5;++i)assert(pt_mixed_readers_enqueue(t->queue,t->input,outs[i])==PT_MIXED_READERS_INVALID);
 assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
 for(i=0;i<5;++i){assert(pt_mixed_readers_service_command(t->queue,ticket,0,outs[i])==PT_MIXED_READERS_INVALID);
  assert(pt_mixed_readers_service_reader(t->queue,ticket,0,0,outs[i])==PT_MIXED_READERS_INVALID);
  assert(pt_mixed_readers_reader_key(t->queue,ticket,0,outs[i])==PT_MIXED_READERS_INVALID);}
 assert(!t->model.command_calls&&!t->model.reader_calls&&!t->model.reads&&!t->model.submits);
 assert(pt_mixed_readers_stop(t->queue)==PT_MIXED_READERS_OK);trial_drop(t);}
static void callback_faults(unsigned bits,unsigned cache_bits,unsigned mode)
{struct trial *t=trial_make(bits,cache_bits);uint64_t ticket;unsigned i;
 struct pt_mixed_readers_command_receipt command_before,command_out;
 struct pt_mixed_readers_reader_receipt reader_before,reader_out;enum pt_mixed_readers_result result;
 memset(&command_before,0xa5,sizeof(command_before));command_out=command_before;
 memset(&reader_before,0x5a,sizeof(reader_before));reader_out=reader_before;
 open_trial(t);trigger_input(t,5,0,960);
 if(mode==0){t->reader[4].hook=1;assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_BACKEND);
  assert(t->reader[4].consumed==1&&!t->reader[4].hook);unused_drop(t,5,0);trial_drop(t);return;}
 assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
 if(mode==1)t->model.hook=1;
 if(mode>=2&&mode<=4){t->model.hook=2;t->model.submit_result=mode==2?0:mode==3?1:-1;}
 if(mode==5){t->model.ticks=oracle(960);}
 if(mode==6)t->model.submit_result=-1;
 result=pt_mixed_readers_publish(t->queue,ticket);
 if(mode>=1&&mode<=5){assert(result==PT_MIXED_READERS_BACKEND||result==PT_MIXED_READERS_LATE);
  if(mode!=5)assert(t->model.consumed==1&&!t->model.hook);
  if(mode==1||mode==2||mode==5){assert(t->model.submits==(mode==2?1U:0U));assert(pt_mixed_readers_stop(t->queue)==PT_MIXED_READERS_BACKEND);trial_drop(t);return;}}
 if(mode>=3&&mode<=6){assert(result==PT_MIXED_READERS_BACKEND);
  assert(pt_mixed_readers_stop(t->queue)==PT_MIXED_READERS_BACKEND);
  assert(pt_mixed_readers_service_command(t->queue,ticket,1,NULL)==PT_MIXED_READERS_BACKEND);
  for(i=0;i<5;++i)assert(pt_mixed_readers_service_reader(t->queue,ticket,i,1,NULL)==PT_MIXED_READERS_BACKEND);
  assert(!pt_mixed_readers_commands_held(t->queue)&&!pt_mixed_readers_readers_held(t->queue));trial_drop(t);return;}
 assert(result==PT_MIXED_READERS_OK);
 if(mode==12){struct model_command *c=model_command(&t->model,ticket);struct pt_mixed_readers_key key,before;uint64_t refused=99;
  /* Actual command issue does not prove reader adoption. This model keeps all
   * reader domains reserved, with no voice effect or active key permission. */
  c->issued=1;c->fired=5;t->model.ticks=c->first;
  assert(pt_mixed_readers_service_command(t->queue,ticket,0,&command_out)==PT_MIXED_READERS_OK);
  assert(!pt_mixed_readers_commands_held(t->queue)&&pt_mixed_readers_readers_held(t->queue)==5&&!t->model.effects);
  memset(&before,0xa5,sizeof(before));key=before;
  for(i=0;i<5;++i){assert(command_out.action[i].command==PT_MIXED_COMMAND_ISSUED&&command_out.action[i].adoption==PT_MIXED_UNADOPTED);
   assert(command_out.action[i].reader==PT_MIXED_READER_RESERVED&&command_out.action[i].issued==oracle(960));
   assert(pt_mixed_readers_reader_key(t->queue,ticket,i,&key)==PT_MIXED_READERS_STALE&&!memcmp(&key,&before,sizeof(key)));
   assert(pt_mixed_readers_service_reader(t->queue,ticket,i,0,&reader_out)==PT_MIXED_READERS_PENDING);
   assert(reader_out.state==PT_MIXED_READER_RESERVED&&reader_out.adoption==PT_MIXED_UNADOPTED&&!reader_out.observed&&!reader_out.issued);
   assert(t->reader[i].live&&!t->reader[i].releases);
  }
  memset(t->input,0,sizeof(*t->input));holder_init(t->command+1,101);t->command[1].queue=t->queue;
  t->input->command=control_of(t->command+1);t->input->batch.generation=17;t->input->batch.frame=1920;t->input->batch.count=1;
  t->input->target[0]=c->key[0];t->input->batch.action[0].route=PT_MIXED_READERS_PAULA;
  t->input->batch.action[0].kind=PT_MIXED_READERS_CONTROL;t->input->batch.action[0].geometry.paula.period=400;
  assert(pt_mixed_readers_enqueue(t->queue,t->input,&refused)==PT_MIXED_READERS_INVALID&&refused==99);
  t->input->batch.action[0].kind=PT_MIXED_READERS_STOP;t->input->batch.action[0].geometry.paula.period=0;
  assert(pt_mixed_readers_enqueue(t->queue,t->input,&refused)==PT_MIXED_READERS_INVALID&&refused==99);
  assert(t->command[1].live&&!t->command[1].currents&&!t->command[1].terminals);
  held_terminal(t->command+1,101,1);held_release(t->command+1,101);
  /* Separate actual adoption at the original activation tick, using copied
   * backend keys only. A later task receipt observes it; no deadline moves. */
  for(i=0;i<5;++i){struct model_reader *r=model_reader(&t->model,c->key+i);unsigned index=c->key[i].route==PT_MIXED_READERS_PAULA?c->key[i].slot:4+c->key[i].slot;
   assert(r&&!r->adopted&&t->model.ticks==c->first);r->adopted=1;r->state=PT_MIXED_READER_ACTIVE;
   r->observed=r->issued=c->first;t->model.active[index]=c->key[i];++t->model.effects;
  }
  for(i=0;i<5;++i){assert(pt_mixed_readers_service_reader(t->queue,ticket,i,0,&reader_out)==PT_MIXED_READERS_PENDING);
   assert(reader_out.state==PT_MIXED_READER_ACTIVE&&reader_out.adoption==PT_MIXED_ADOPTED&&reader_out.issued==oracle(960));
   assert(pt_mixed_readers_reader_key(t->queue,ticket,i,&key)==PT_MIXED_READERS_OK&&keys_equal(&key,c->key+i));}
  for(i=0;i<5;++i){assert(pt_mixed_readers_service_reader(t->queue,ticket,i,1,NULL)==PT_MIXED_READERS_OK);
   assert(!t->reader[i].live&&t->reader[i].releases==1&&t->reader[i].valid==1);}
  trial_drop(t);return;
 }
 if(mode==11){t->model.partial=1;}
 fire(&t->model,ticket);
 if(mode==7)t->model.hook=3;
 if(mode==8)t->model.malformed=2;
 if(mode==9){t->model.hook=4;assert(pt_mixed_readers_service_reader(t->queue,ticket,4,0,&reader_out)==PT_MIXED_READERS_BACKEND);
  assert(t->model.consumed==1&&!t->model.hook&&!memcmp(&reader_out,&reader_before,sizeof(reader_out)));}
 if(mode==11){assert(t->model.effects==1);for(i=0;i<5;++i)assert(t->reader[i].live&&!t->reader[i].releases);}
 if(mode==10){t->model.malformed=4;assert(pt_mixed_readers_service_reader(t->queue,ticket,4,1,&reader_out)==PT_MIXED_READERS_BACKEND);
  assert(!memcmp(&reader_out,&reader_before,sizeof(reader_out)));t->model.malformed=0;}
 assert(pt_mixed_readers_service_command(t->queue,ticket,0,&command_out)==PT_MIXED_READERS_BACKEND);
 assert(!memcmp(&command_out,&command_before,sizeof(command_out))&&!pt_mixed_readers_commands_held(t->queue));
 if(mode==8||mode==11)assert(t->command[0].valid==0);
 if(mode==11)assert(pt_mixed_readers_readers_held(t->queue)==5);
 if(mode==7)assert(t->model.consumed==1&&!t->model.hook);
 t->model.malformed=0;
 for(i=0;i<5;++i)if(!(mode==10&&i==4))assert(pt_mixed_readers_service_reader(t->queue,ticket,i,1,NULL)==PT_MIXED_READERS_BACKEND);
 assert(!pt_mixed_readers_readers_held(t->queue));trial_drop(t);}
static void replacement_capacity(unsigned bits,unsigned cache_bits)
{struct trial *t=trial_make(bits,cache_bits);uint64_t first,second,third=99;unsigned i;struct pt_mixed_readers_key key;
 open_trial(t);trigger_input(t,16,0,960);assert(pt_mixed_readers_enqueue(t->queue,t->input,&first)==PT_MIXED_READERS_OK);
 trigger_input(t,16,1,1920);assert(pt_mixed_readers_enqueue(t->queue,t->input,&second)==PT_MIXED_READERS_OK);
 trigger_input(t,1,2,2880);assert(pt_mixed_readers_enqueue(t->queue,t->input,&third)==PT_MIXED_READERS_CAPACITY&&third==99);unused_drop(t,1,2);
 assert(pt_mixed_readers_publish(t->queue,second)==PT_MIXED_READERS_INVALID&&!t->model.submits);
 assert(pt_mixed_readers_publish(t->queue,first)==PT_MIXED_READERS_OK);fire(&t->model,first);
 assert(pt_mixed_readers_service_command(t->queue,first,0,NULL)==PT_MIXED_READERS_OK);
 assert(pt_mixed_readers_publish(t->queue,second)==PT_MIXED_READERS_OK);fire(&t->model,second);
 assert(pt_mixed_readers_service_command(t->queue,second,0,NULL)==PT_MIXED_READERS_OK);
 assert(pt_mixed_readers_readers_held(t->queue)==32);
 trigger_input(t,1,2,2880);third=99;assert(pt_mixed_readers_enqueue(t->queue,t->input,&third)==PT_MIXED_READERS_CAPACITY&&third==99);unused_drop(t,1,2);
 for(i=0;i<16;++i){assert(pt_mixed_readers_service_reader(t->queue,second,i,0,NULL)==PT_MIXED_READERS_PENDING);
  assert(pt_mixed_readers_reader_key(t->queue,second,i,&key)==PT_MIXED_READERS_OK);
  assert(pt_mixed_readers_reader_key(t->queue,first,i,&key)==PT_MIXED_READERS_STALE);
  assert(pt_mixed_readers_service_reader(t->queue,first,i,1,NULL)==PT_MIXED_READERS_OK);
  assert(pt_mixed_readers_reader_key(t->queue,second,i,&key)==PT_MIXED_READERS_OK);
  assert(pt_mixed_readers_service_reader(t->queue,second,i,1,NULL)==PT_MIXED_READERS_OK);}
 trial_drop(t);}
static void control_stop(unsigned bits,unsigned cache_bits)
{struct trial *t=trial_make(bits,cache_bits);uint64_t first,second;struct pt_mixed_readers_key key[16];unsigned i;
 open_trial(t);trigger_input(t,6,0,960);assert(pt_mixed_readers_enqueue(t->queue,t->input,&first)==PT_MIXED_READERS_OK);
 assert(pt_mixed_readers_publish(t->queue,first)==PT_MIXED_READERS_OK);fire(&t->model,first);
 assert(pt_mixed_readers_service_command(t->queue,first,0,NULL)==PT_MIXED_READERS_OK);
 for(i=0;i<6;++i)assert(pt_mixed_readers_reader_key(t->queue,first,i,key+i)==PT_MIXED_READERS_OK);
 memset(t->input,0,sizeof(*t->input));holder_init(t->command+1,101);t->input->command=control_of(t->command+1);
 t->input->batch.generation=17;t->input->batch.frame=1920;t->input->batch.count=6;
 for(i=0;i<6;++i){struct pt_mixed_readers_action *a=t->input->batch.action+i;a->route=key[i].route;a->slot=key[i].slot;
  a->kind=i==0||i==5?PT_MIXED_READERS_STOP:PT_MIXED_READERS_CONTROL;t->input->target[i]=key[i];
  if(a->kind==PT_MIXED_READERS_CONTROL){
   if(a->route==PT_MIXED_READERS_PAULA){a->geometry.paula.period=400;a->geometry.paula.volume=32;}
   else{a->geometry.amigus.rate=0x04000000;a->geometry.amigus.left=123;a->geometry.amigus.right=456;}}}
 t->input->batch.action[5].geometry.amigus.rate=1;
 assert(pt_mixed_readers_enqueue(t->queue,t->input,&second)==PT_MIXED_READERS_INVALID);t->input->batch.action[5].geometry.amigus.rate=0;
 assert(pt_mixed_readers_enqueue(t->queue,t->input,&second)==PT_MIXED_READERS_OK);assert(pt_mixed_readers_publish(t->queue,second)==PT_MIXED_READERS_OK);
 fire(&t->model,second);assert(pt_mixed_readers_service_command(t->queue,second,0,NULL)==PT_MIXED_READERS_OK);
 for(i=0;i<6;++i){struct pt_mixed_readers_key actual;
  assert(pt_mixed_readers_reader_key(t->queue,first,i,&actual)==((i==0||i==5)?PT_MIXED_READERS_STALE:PT_MIXED_READERS_OK));
  assert(pt_mixed_readers_service_reader(t->queue,first,i,1,NULL)==PT_MIXED_READERS_OK);}
 trial_drop(t);}
static void clock_refusal_and_pending(unsigned bits,unsigned cache_bits,unsigned mode)
{struct trial *t=trial_make(bits,cache_bits);uint64_t ticket;unsigned i;struct pt_mixed_readers_command_receipt before,out;
 memset(&before,0xa5,sizeof(before));out=before;open_trial(t);trigger_input(t,5,0,960);
 assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
 t->model.ticks=150;t->model.submit_result=0;
 assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_PENDING&&t->model.submits==1);
 assert(pt_mixed_readers_commands_held(t->queue)==1&&!t->model.effects);
 if(mode==0){t->model.ticks=149;t->model.submit_result=1;
  assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_CLOCK&&t->model.submits==1);
  assert(pt_mixed_readers_stop(t->queue)==PT_MIXED_READERS_BACKEND);trial_drop(t);return;}
 t->model.ticks=151;t->model.submit_result=1;assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_OK);
 assert(model_command(&t->model,ticket)->first==oracle(960));t->model.command_pending=t->model.reader_pending=1;
 assert(pt_mixed_readers_service_command(t->queue,ticket,1,&out)==PT_MIXED_READERS_PENDING&&!memcmp(&out,&before,sizeof(out)));
 assert(pt_mixed_readers_service_reader(t->queue,ticket,4,1,NULL)==PT_MIXED_READERS_PENDING);
 assert(pt_mixed_readers_commands_held(t->queue)==1&&pt_mixed_readers_readers_held(t->queue)==5&&!pt_mixed_readers_close(&t->queue));
 t->model.command_pending=t->model.reader_pending=0;
 if(mode==2){t->model.malformed=1;assert(pt_mixed_readers_service_command(t->queue,ticket,1,&out)==PT_MIXED_READERS_BACKEND);
  assert(!memcmp(&out,&before,sizeof(out))&&pt_mixed_readers_commands_held(t->queue)==1);
  t->model.malformed=0;assert(pt_mixed_readers_service_command(t->queue,ticket,1,NULL)==PT_MIXED_READERS_BACKEND);
  for(i=0;i<5;++i)assert(pt_mixed_readers_service_reader(t->queue,ticket,i,1,NULL)==PT_MIXED_READERS_BACKEND);
 }else{drain(t,ticket,5,mode==3,1);}
 assert(!pt_mixed_readers_commands_held(t->queue)&&!pt_mixed_readers_readers_held(t->queue));trial_drop(t);}
static void card_geometry_and_padding(unsigned bits,unsigned cache_bits,unsigned little,unsigned loop)
{struct trial *t=trial_make(bits,cache_bits);uint64_t ticket;unsigned i;struct pt_mixed_readers_config saved=t->config;
 /* Nonzero padding stays byte-exact; every named member is initialized. */
 memset(&t->config,0xa5,sizeof(t->config));t->config.allocator=saved.allocator;t->config.allocator_context=saved.allocator_context;
 t->config.grid=saved.grid;t->config.session=saved.session;t->config.control_budget=saved.control_budget;t->config.backend=saved.backend;
 t->resources->little=little;
 t->resources->loop_mode=loop;
 if(loop){t->resources->document.project.samples[1].loop=PT_LOOP_FORWARD;
  t->resources->document.project.samples[1].loop_start=8;t->resources->document.project.samples[1].loop_end=48;}
 open_trial(t);trigger_input(t,6,0,960);
 for(i=4;i<6;++i){const struct pt_amigus_voice_plan *p=&t->input->batch.action[i].geometry.amigus;
  assert(!!(p->control&1)==(cache_bits==16)&&!!(p->control&8)==(cache_bits==16&&little)&&!!(p->control&2)==!!loop);
  assert(p->end_exclusive==t->input->reader[i].card.address+(loop?48U:64U)*(cache_bits/8));
  assert(p->loop==t->input->reader[i].card.address+(loop?8U:0U)*(cache_bits/8));
 }
 assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
 assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_OK);fire(&t->model,ticket);drain(t,ticket,6,0,0);trial_drop(t);}
static void reader_envelope_refusal(unsigned bits,unsigned cache_bits)
{struct trial *t=trial_make(bits,cache_bits);uint64_t ticket;unsigned i;struct pt_mixed_readers_reader_receipt before,out;
 memset(&before,0xa5,sizeof(before));out=before;open_trial(t);trigger_input(t,5,0,960);
 assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
 assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_OK);fire(&t->model,ticket);
 assert(pt_mixed_readers_service_command(t->queue,ticket,0,NULL)==PT_MIXED_READERS_OK);
 t->model.malformed=3;assert(pt_mixed_readers_service_reader(t->queue,ticket,4,1,&out)==PT_MIXED_READERS_BACKEND);
 assert(!memcmp(&out,&before,sizeof(out))&&!t->reader[4].releases&&pt_mixed_readers_readers_held(t->queue)==5);
 t->model.malformed=0;for(i=0;i<5;++i)assert(pt_mixed_readers_service_reader(t->queue,ticket,i,1,NULL)==PT_MIXED_READERS_BACKEND);
 assert(!pt_mixed_readers_readers_held(t->queue));trial_drop(t);}
static void expired_and_release(void)
{struct trial *t=trial_make(24,16);uint64_t ticket;
 open_trial(t);trigger_input(t,5,0,960);assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
 free(t->input);t->input=NULL;free(t->workspace);t->workspace=NULL;memset(&t->config,0xa5,sizeof(t->config));
 assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_OK);fire(&t->model,ticket);drain(t,ticket,5,1,0);
 t->allocator.hook=1;assert(!pt_mixed_readers_close(&t->queue)&&!t->queue);
 assert(t->allocator.consumed==1&&!t->allocator.hook&&pt_mixed_readers_close(&t->queue));trial_drop(t);}
int main(void)
{unsigned bits,cache_bits,order,mode;
 for(mode=0;mode<20;++mode)constructor_admission(mode);
 for(bits=8;bits<=24;bits+=8)for(cache_bits=8;cache_bits<=16;cache_bits+=8){
  for(order=0;order<2;++order)paired_lifetime(bits,cache_bits,order);
  for(mode=0;mode<30;++mode)input_admission(bits,cache_bits,mode);
  allocation_aliases(bits,cache_bits);
  for(mode=0;mode<=12;++mode)callback_faults(bits,cache_bits,mode);
  replacement_capacity(bits,cache_bits);control_stop(bits,cache_bits);
  for(mode=0;mode<4;++mode)clock_refusal_and_pending(bits,cache_bits,mode);
  reader_envelope_refusal(bits,cache_bits);
  for(order=0;order<2;++order){card_geometry_and_padding(bits,cache_bits,0,order);
   if(cache_bits==16)card_geometry_and_padding(bits,cache_bits,1,order);}
 }
 expired_and_release();
 puts("MIXED READERS PASS:12 genuine16-action paired lifetimes;20 constructor/180 typed admissions;78 callback/effect/proof cases;24 explicit clock/refusal/pending/cancellation groups;6 malformed-reader envelopes;18 endian/loop/padding groups;6 full alias,32-reader capacity/replacement and control/STOP groups; expired controls/consumed close0; exact common grid/master saves; SOFTWARE_ONLY");
 return wavetable_fixture_main();}
