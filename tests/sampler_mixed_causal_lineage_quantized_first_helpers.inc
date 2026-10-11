/* Private SOURCE fixture, NOT_RUN. Production sampler/core compile separately.
 * Unchanged genuine default/STOP/lineage helpers retain explicitly renamed,
 * UNCALLED entry points. No runtime counts or native authority are inferred. */
#include "sampler_mixed_causal_lineage_port_helpers.inc"
#include "../src/editor/sampler_mixed_causal_lineage_internal.h"
#include "../src/editor/sampler_mixed_readers_internal.h"
#include "../src/core/mixed_readers_causal_factory_internal.h"
struct sl_case;
struct sl_ledger {void *data;size_t bytes;};
struct sl_memory {struct sl_case *owner;unsigned calls,releases,refusals,fail,hook;void *alias;struct sl_ledger live[40];};
struct sl_client {
 struct pt_sampler_mixed_config config;struct pt_sampler_mixed_pool *pool;
 struct pt_sampler_mixed_request trigger[16];
 struct pt_sampler_mixed_command_handle command[2];
 struct pt_sampler_mixed_reader_handle reader[16];
 struct pt_sampler_mixed_causal_lineage_stop_batch stop;
 struct pt_sampler_mixed_causal_lineage_control_batch control;
};
struct sl_case {
 struct tl_case *base;struct sl_client *client;
 struct sl_memory ordinary,chip;struct pt_sampler_mixed_causal_lineage_binding binding;
 unsigned char stop_open_scratch[sizeof(struct pt_sampler_mixed_causal_lineage_binding)+_Alignof(struct pt_sampler_mixed_causal_lineage_binding)];
 struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_pcm pcm[PT_PROJECT_SAMPLES];
 int32_t *master_before[PT_PROJECT_SAMPLES];
 void *workspace;size_t capacity;unsigned owned_hook,owned_after,owned_calls,later_stage,unknown_source;uint32_t revision;
};
static unsigned sl_cases;
static void *sl_new(void *context,size_t bytes)
{
 struct sl_memory *m=context;struct sl_case *f=m->owner;unsigned i;void *p;++m->calls;
 if(m->hook){unsigned hook=m->hook;m->hook=0;
  if(hook==1){assert(pt_sampler_mixed_stop(f->client->pool)==PT_SAMPLER_MIXED_BUSY);}
  else {assert(hook==2);if(f->later_stage==2)++f->client->control.frame;else ++f->client->stop.frame;}}
 if(m->fail){++m->refusals;return NULL;}if(m->alias){++m->refusals;return m->alias;}
 p=malloc(bytes);assert(p);for(i=0;i<40&&m->live[i].data;++i){}assert(i<40);
 m->live[i]=(struct sl_ledger){p,bytes};return p;
}
static void sl_free(void *context,void *p)
{struct sl_memory *m=context;unsigned i;for(i=0;i<40&&m->live[i].data!=p;++i){}assert(i<40);
 m->live[i]=(struct sl_ledger){NULL,0};++m->releases;free(p);}
static void sl_chip_free(void *context,void *p,size_t bytes)
{struct sl_memory *m=context;unsigned i;for(i=0;i<40&&m->live[i].data!=p;++i){}assert(i<40&&m->live[i].bytes==bytes);sl_free(context,p);}
static int sl_owned(void *context)
{
 struct sl_case *f=context;int result=bus_owned(&f->base->trial->resources->card);++f->owned_calls;
 if(f->owned_hook&&(!f->owned_after||!--f->owned_after)){unsigned hook=f->owned_hook;f->owned_hook=0;
  if(hook==1){if(f->later_stage==2)++f->client->control.frame;else ++f->client->stop.frame;}
  else {struct pt_mixed_causal_diagnostic d;assert(hook==2);assert(!pt_mixed_causal_diagnostic(f->base->owner,&d));}}
 return result;
}
static int sl_write(void *context,unsigned reg,uint32_t value)
{struct sl_case *f=context;return bus_write(&f->base->trial->resources->card,reg,value);}
static struct sl_case *sl_make(unsigned bits,unsigned cache_bits,unsigned all_card,unsigned bind)
{
 struct sl_case *f=calloc(1,sizeof(*f));struct resources *r;struct pt_sampler_mixed_config *c;unsigned i;
 assert(f);++sl_cases;f->client=calloc(1,sizeof(*f->client));assert(f->client);
 f->base=tl_make(bits,cache_bits,bind);r=f->base->trial->resources;c=&f->client->config;
 if(all_card)for(i=0;i<16;++i)r->document.project.channels.track[i].route=PT_AMIGUS;
 f->ordinary.owner=f->chip.owner=f;f->revision=37;
 for(i=0;i<r->document.project.sample_count;++i)assert(pt_sampler_pin(&r->sampler,&r->document.project,i,r->sampler.generation,f->pcm+i,f->pin+i)==PT_EDIT_OK);
 for(i=0;i<r->document.project.sample_count;++i){size_t n=f->pcm[i].capacity*sizeof(int32_t);
  if(n){f->master_before[i]=malloc(n);assert(f->master_before[i]);memcpy(f->master_before[i],f->pcm[i].data,n);}}
 r->card.cache.context=f;r->card.cache.owned=sl_owned;r->card.cache.write32=sl_write;
 r->card.cache.arena.context=f;r->card.cache.arena.owned=sl_owned;r->card.cache.arena.write32=sl_write;
 c->allocator=(struct pt_allocator){&f->ordinary,sl_new,sl_free};c->sampler=&r->sampler;c->project=&r->document.project;
 c->queue=f->base->trial->queue;c->backend=&r->card.cache;c->chip_context=&f->chip;c->chip_allocate=sl_new;c->chip_release=sl_chip_free;
 c->control_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
 c->chip_budget=4096;c->generation=17;c->maximum_commands=2;c->maximum_readers=32;
 c->contexts[0]=(struct pt_mixed_readers_span){f,sizeof(*f)};
 c->contexts[1]=(struct pt_mixed_readers_span){&f->base->port,sizeof(f->base->port)};
 c->contexts[2]=(struct pt_mixed_readers_span){&f->base->memory,sizeof(f->base->memory)};
 c->contexts[3]=(struct pt_mixed_readers_span){f->base->owner,pt_mixed_causal_control_size()};
 c->contexts[4]=(struct pt_mixed_readers_span){&r->card,sizeof(r->card)};c->context_count=5;
 f->capacity=pt_sampler_mixed_workspace_size()+128;f->workspace=calloc(1,f->capacity);assert(f->workspace);
 /* tl_make's historical beforeimage preceded the optional route change. */
 free(f->base->before);f->base->before=save(r,&f->base->before_bytes);
 return f;
}
static void sl_open(struct sl_case *f)
{
 struct sl_client *v=f->client;enum pt_sampler_mixed_result result;unsigned steps=0;
 assert(pt_sampler_mixed_causal_lineage_bind(&f->binding,f->base->owner,v->config.queue,19,17,(struct pt_mixed_readers_span){f,sizeof(*f)}));
 assert(pt_sampler_mixed_causal_lineage_open(&v->config,f->revision,f->workspace,f->capacity,&f->binding,&v->pool)==PT_SAMPLER_MIXED_PENDING);
 do{result=pt_sampler_mixed_advance_validation(v->pool,f->revision,7);assert(++steps<10000);}while(result==PT_SAMPLER_MIXED_PENDING);
 assert(result==PT_SAMPLER_MIXED_OK&&!f->chip.calls&&!f->base->trial->resources->card.writes);
}
static void sl_advance(struct sl_case *f,unsigned command)
{
 enum pt_sampler_mixed_result result;unsigned steps=0,done=999,writes;
 do{writes=f->base->trial->resources->card.writes;result=pt_sampler_mixed_advance(f->client->pool,f->revision,f->client->command[command],&done);
  assert(f->base->trial->resources->card.writes-writes<=128&&++steps<2000);}while(result==PT_SAMPLER_MIXED_PENDING);
 assert(result==PT_SAMPLER_MIXED_OK&&done==(f->later_stage==2?f->client->control.count:(f->later_stage==3?f->client->stop.count:16)));
}
static uint64_t sl_first(struct sl_case *f,unsigned observed)
{
 struct sl_client *v=f->client;struct resources *r=f->base->trial->resources;uint64_t ticket=0;unsigned i;
 for(i=0;i<16;++i){struct pt_sampler_mixed_request *a=v->trigger+i;a->kind=PT_MIXED_READERS_TRIGGER;a->track=i;a->sample=i%2;a->expected=f->pin[i%2];
  if(r->document.project.channels.track[i].route==PT_PAULA){a->geometry.paula.period=428;a->geometry.paula.volume=64;}
  else{a->geometry.amigus.bits=r->cache_bits;a->geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,64,128};}}
 assert(pt_sampler_mixed_begin(v->pool,f->revision,960,v->trigger,16,v->command)==PT_SAMPLER_MIXED_PENDING);sl_advance(f,0);
 for(i=0;i<16;++i)assert(pt_sampler_mixed_reader(v->pool,v->command[0],i,v->reader+i)==PT_SAMPLER_MIXED_OK);
 assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[0],&ticket)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_publish(v->pool,ticket)==PT_MIXED_READERS_OK);
 assert(ct_command(&f->base->port.original.base,ticket)->packet.frame==960&&ct_command(&f->base->port.original.base,ticket)->packet.first==oracle(960)&&ct_command(&f->base->port.original.base,ticket)->packet.last==oracle(961));
 if(observed){f->base->port.original.base.ticks=oracle(960);assert(pt_mixed_causal_fire(f->base->owner,ticket)==PT_MIXED_CAUSAL_COMMITTED);
 f->base->port.original.hold_first=1;assert(pt_sampler_mixed_service_command(v->pool,ticket,0,NULL)==PT_MIXED_READERS_PENDING);}
 return ticket;
}
static void sl_batch(struct sl_case *f,unsigned count)
{unsigned i;memset(&f->client->stop,0,sizeof(f->client->stop));f->client->stop.frame=2880;f->client->stop.count=count;
 for(i=0;i<count;++i)f->client->stop.reader[i]=f->client->reader[i];}
static void sl_drop(struct sl_case *f)
{
 unsigned i;int ok;if(f->client->pool){ok=pt_sampler_mixed_close(&f->client->pool);assert(ok||!f->client->pool);}
 assert(!f->client->pool);
 for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(f->pin[i]);free(f->master_before[i]);}
 for(i=0;i<40;++i)assert(!f->ordinary.live[i].data&&!f->chip.live[i].data);
 assert(f->ordinary.calls==f->ordinary.releases+f->ordinary.refusals&&f->chip.calls==f->chip.releases+f->chip.refusals);
 tl_drop(f->base,f->unknown_source);free(f->workspace);free(f->client);free(f);
}
static void sl_same(struct sl_case *f)
{
 struct resources *r=f->base->trial->resources;unsigned i;
 same_save(r,f->base->before,f->base->before_bytes);
 for(i=0;i<r->document.project.sample_count;++i){const struct pt_pcm *a=&r->document.project.samples[i].pcm,*b=f->pcm+i;
  assert(a->data==b->data&&a->capacity==b->capacity&&a->frames==b->frames&&a->rate==b->rate&&
     a->channels==b->channels&&a->bits==b->bits&&r->sampler.current[i]==f->pin[i]);
  if(a->capacity)assert(!memcmp(a->data,f->master_before[i],a->capacity*sizeof(int32_t)));}
}
static void sl_control_batch(struct sl_case *f,unsigned count)
{
 unsigned i;memset(&f->client->control,0,sizeof(f->client->control));
 f->client->control.frame=1920;f->client->control.count=count;
 for(i=0;i<count;++i){struct pt_sampler_mixed_causal_lineage_control_action *a=f->client->control.action+i;
  a->reader=f->client->reader[i];
  if(f->base->trial->resources->document.project.channels.track[i].route==PT_PAULA){a->period=(uint16_t)(214+i);a->volume=(uint8_t)(32+i);}
  else{a->rate=0x10000U+i;a->left=(uint8_t)i;a->right=(uint8_t)(250-i);}}
}
static uint64_t sl_control(struct sl_case *f,unsigned count,unsigned fire)
{
 struct sl_client *v=f->client;uint64_t control=0;
 f->later_stage=2;sl_control_batch(f,count);
 assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,v->command+1)==PT_SAMPLER_MIXED_PENDING);
 sl_advance(f,1);assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[1],&control)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_publish(v->pool,control)==PT_MIXED_READERS_OK);
 assert(ct_command(&f->base->port.original.base,control)->packet.frame==1920&&
    ct_command(&f->base->port.original.base,control)->packet.first==oracle(1920)&&
    ct_command(&f->base->port.original.base,control)->packet.last==oracle(1921));
 if(fire){f->base->port.original.base.ticks=oracle(1920);
  assert(pt_mixed_causal_fire(f->base->owner,control)==PT_MIXED_CAUSAL_COMMITTED&&f->base->port.control_completed);}
 return control;
}
static void sl_root_close(struct sl_case *f,uint64_t first)
{
 f->base->port.original.hold_first=0;
 assert(pt_sampler_mixed_service_command(f->client->pool,first,0,NULL)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_command_close(f->client->pool,f->client->command)&&!f->client->command[0].address);
}
static uint64_t sl_stop(struct sl_case *f,unsigned count)
{
 uint64_t stop=0;f->later_stage=3;sl_batch(f,count);
 assert(pt_sampler_mixed_causal_lineage_stop_begin(f->client->pool,f->revision,&f->client->stop,
    f->client->command)==PT_SAMPLER_MIXED_PENDING);sl_advance(f,0);
 assert(pt_sampler_mixed_enqueue(f->client->pool,f->revision,f->client->command[0],&stop)==PT_MIXED_READERS_OK);
 return stop;
}
static void sl_drain(struct sl_case *f,uint64_t first,uint64_t control,uint64_t stop,unsigned first_detached,unsigned control_detached,unsigned stop_detached,unsigned failed)
{
 unsigned i;enum pt_mixed_readers_result wanted=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
 struct sl_client *v=f->client;
 f->ordinary.alias=f->chip.alias=NULL;f->ordinary.fail=f->chip.fail=0;
 f->ordinary.hook=f->chip.hook=f->owned_hook=0;
 f->base->port.original.hold_first=f->base->port.hold_control=f->base->port.hold_third=0;
 f->base->port.original.base.unknown_ticket=0;f->base->port.original.quiet_mask=~0U;
 /* A transfer is not a publication. Explicit genuine local queue cancellation
  * drains any unissued C before published-domain services; no quiet is invented. */
 if((control&&!control_detached&&!ct_command(&f->base->port.original.base,control))||
    (stop&&!stop_detached&&!ct_command(&f->base->port.original.base,stop))){
  enum pt_mixed_readers_result result=pt_mixed_causal_stop(f->base->owner);
  assert(result==PT_MIXED_READERS_PENDING||result==PT_MIXED_READERS_BACKEND);}
 if(first&&!first_detached)assert(pt_sampler_mixed_service_command(v->pool,first,1,NULL)==wanted);
 if(control&&!control_detached&&ct_command(&f->base->port.original.base,control))
    assert(pt_sampler_mixed_service_command(v->pool,control,1,NULL)==wanted);
 if(stop&&!stop_detached&&ct_command(&f->base->port.original.base,stop))
    assert(pt_sampler_mixed_service_command(v->pool,stop,1,NULL)==wanted);
 for(i=0;i<16;++i)if(v->reader[i].address){struct pt_mixed_reader_retirement actual=
    pt_sampler_mixed_retire_original_reader(v->pool,v->reader+i,1);
  assert(actual.retirement_consumed&&actual.result==wanted&&!v->reader[i].address);}
 for(i=0;i<2;++i)if(v->command[i].address){int ok=pt_sampler_mixed_command_close(v->pool,v->command+i);
  assert(ok||!v->command[i].address);}
 assert(!pt_mixed_readers_commands_held(v->config.queue)&&!pt_mixed_readers_readers_held(v->config.queue));
 sl_same(f);
}
static void sl_success(unsigned bits,unsigned cache,unsigned all_card,unsigned reader_first)
{
 struct sl_case *f=sl_make(bits,cache,all_card,1);struct sl_client *v=f->client;
 struct resources *r=f->base->trial->resources;uint64_t first,control,stop,out=777;
 struct pt_sampler_mixed_command_handle expired;struct pt_mixed_readers_key key[16];
 struct pt_amigus_ram_block blocks[PT_CACHE_SLOTS];struct pt_cache_entry entries[PT_CACHE_SLOTS];
 struct sl_ledger chips[40];unsigned i,calls,chip_calls,writes,count=reader_first?6:16;
 sl_open(f);first=sl_first(f,1);expired=v->command[0];
 for(i=0;i<16;++i)assert(pt_sampler_mixed_reader_key(v->pool,v->reader[i],key+i)==PT_MIXED_READERS_OK);
 memcpy(blocks,r->card.cache.arena.block,sizeof(blocks));memcpy(entries,r->card.cache.cache.entry,sizeof(entries));memcpy(chips,f->chip.live,sizeof(chips));
 calls=f->ordinary.calls;chip_calls=f->chip.calls;writes=r->card.writes;
 control=sl_control(f,count,1);
 assert(f->ordinary.calls==calls+1&&f->chip.calls==chip_calls&&r->card.writes==writes&&
    pt_mixed_readers_commands_held(v->config.queue)==2&&pt_mixed_readers_readers_held(v->config.queue)==16);
 sl_same(f);sl_root_close(f,first);
 /* Expired pointer is never dereferenced. Genuine token/address lookup refuses
  * it even when C3 later reuses the original logical command slot. */
 assert(pt_sampler_mixed_cancel(v->pool,expired)==PT_SAMPLER_MIXED_INVALID);
 stop=sl_stop(f,count);
 assert(v->command[0].token!=expired.token&&f->ordinary.calls==calls+2&&
    f->chip.calls==chip_calls&&r->card.writes==writes&&pt_mixed_readers_commands_held(v->config.queue)==2);
 assert(pt_sampler_mixed_cancel(v->pool,expired)==PT_SAMPLER_MIXED_INVALID);
 assert(pt_sampler_mixed_reader(v->pool,v->command[0],0,(struct pt_sampler_mixed_reader_handle *)&key[0])==PT_SAMPLER_MIXED_INVALID);
 assert(pt_sampler_mixed_publish(v->pool,stop)==PT_MIXED_READERS_OK);
 assert(f->base->port.root.identity.event==f->base->port.third_identity.event);
 assert(ct_command(&f->base->port.original.base,stop)->packet.frame==2880&&
    ct_command(&f->base->port.original.base,stop)->packet.first==oracle(2880)&&
    ct_command(&f->base->port.original.base,stop)->packet.last==oracle(2881));
 f->base->port.original.base.ticks=oracle(2880)-1;
 assert(pt_mixed_causal_fire(f->base->owner,stop)==PT_MIXED_CAUSAL_EARLY&&!f->base->port.original.stops);
 f->base->port.original.base.ticks++;
 assert(pt_mixed_causal_fire(f->base->owner,stop)==PT_MIXED_CAUSAL_COMMITTED&&f->base->port.original.stops==count);
 assert(!memcmp(blocks,r->card.cache.arena.block,sizeof(blocks))&&!memcmp(entries,r->card.cache.cache.entry,sizeof(entries))&&
    !memcmp(chips,f->chip.live,sizeof(chips))&&f->chip.calls==chip_calls&&r->card.writes==writes);
 for(i=0;i<16;++i){unsigned index=ct_index(key[i].route,key[i].slot);
  if(i<count)assert(!(f->base->port.original.base.mask&(1U<<index)));
  else assert(keys_equal(key+i,f->base->port.original.base.slot+index));}
 sl_same(f);
 assert(pt_sampler_mixed_begin(v->pool,f->revision,3840,v->trigger,16,&expired)==PT_SAMPLER_MIXED_CAPACITY);
 assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,&expired)==PT_SAMPLER_MIXED_INVALID);
 assert(pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,&expired)==PT_SAMPLER_MIXED_INVALID);
 assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[0],&out)==PT_MIXED_READERS_INVALID&&out==777);
 if(reader_first){
  struct pt_mixed_readers_reader_receipt actual,saved;
  memset(&actual,0xa5,sizeof(actual));memcpy(&saved,&actual,sizeof(saved));
  assert(pt_sampler_mixed_service_reader(v->pool,first,0,0,&actual)==PT_MIXED_READERS_PENDING&&!memcmp(&actual,&saved,sizeof(saved)));
  f->base->port.original.quiet_mask=1U<<ct_index(key[0].route,key[0].slot);
  assert(pt_sampler_mixed_service_reader(v->pool,first,0,0,NULL)==PT_MIXED_READERS_OK);
  assert(!pt_sampler_mixed_reader_close(v->pool,v->reader));
  assert(pt_sampler_mixed_service_command(v->pool,control,0,NULL)==PT_MIXED_READERS_OK);
  assert(!pt_sampler_mixed_reader_close(v->pool,v->reader));
  assert(pt_sampler_mixed_service_command(v->pool,stop,0,NULL)==PT_MIXED_READERS_OK);
  assert(pt_sampler_mixed_reader_close(v->pool,v->reader)&&!v->reader[0].address);
  sl_drain(f,first,control,stop,1,1,1,0);
 }else{
  assert(pt_sampler_mixed_service_command(v->pool,stop,0,NULL)==PT_MIXED_READERS_OK);
  assert(pt_sampler_mixed_service_reader(v->pool,first,0,0,NULL)==PT_MIXED_READERS_PENDING);
  sl_drain(f,first,control,stop,1,0,1,0);
 }
 f->unknown_source=reader_first;sl_drop(f);
}
static void sl_detachment(unsigned dispose_control)
{
 struct sl_case *f=sl_make(24,16,0,1);struct sl_client *v=f->client;
 struct pt_sampler_mixed_command_handle out={NULL,777},expired;uint64_t first,control,stop;
 unsigned calls;sl_open(f);first=sl_first(f,1);control=sl_control(f,6,1);sl_batch(f,6);calls=f->ordinary.calls;
 assert(pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,&out)==PT_SAMPLER_MIXED_INVALID&&!out.address&&out.token==777&&f->ordinary.calls==calls);
 f->base->port.original.hold_first=0;
 assert(pt_sampler_mixed_service_command(v->pool,first,0,NULL)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,&out)==PT_SAMPLER_MIXED_INVALID&&f->ordinary.calls==calls);
 expired=v->command[0];assert(pt_sampler_mixed_command_close(v->pool,v->command)&&!v->command[0].address);
 if(dispose_control){const struct pt_mixed_readers_event *event=f->base->port.control.identity.event;
  assert(pt_sampler_mixed_service_command(v->pool,control,0,NULL)==PT_MIXED_READERS_OK);
  assert(pt_sampler_mixed_command_close(v->pool,v->command+1)&&!v->command[1].address);
  CT_POISON((void *)event,sizeof(*event));stop=sl_stop(f,6);
  assert(pt_sampler_mixed_publish(v->pool,stop)==PT_MIXED_READERS_OK);CT_UNPOISON((void *)event,sizeof(*event));
 }else{stop=sl_stop(f,6);assert(pt_sampler_mixed_publish(v->pool,stop)==PT_MIXED_READERS_OK);}
 assert(pt_sampler_mixed_cancel(v->pool,expired)==PT_SAMPLER_MIXED_INVALID);
 sl_drain(f,first,control,stop,1,dispose_control,0,0);sl_drop(f);
}
static void sl_wrong_capability(unsigned mode)
{
 struct sl_case *f=sl_make(16,8,0,0);struct pt_sampler_mixed_causal_lineage_binding before;
 struct pt_mixed_causal_control_port c={&f->base->port,sizeof(f->base->port),1,3,cc_publish_control};
 struct pt_mixed_causal_stop_port s={&f->base->port,sizeof(f->base->port),1,3,ss_publish_stop};
 if(mode==1)assert(pt_mixed_causal_control_bind(f->base->owner,&c)==PT_MIXED_READERS_OK);
 if(mode==2)assert(pt_mixed_causal_stop_bind(f->base->owner,&s)==PT_MIXED_READERS_OK);
 memcpy(&before,&f->binding,sizeof(before));
 assert(!pt_sampler_mixed_causal_lineage_bind(&f->binding,f->base->owner,f->client->config.queue,19,17,
    (struct pt_mixed_readers_span){f,sizeof(*f)})&&!memcmp(&before,&f->binding,sizeof(before)));
 sl_drop(f);
}
static void sl_control_guards(unsigned mode)
{
 struct sl_case *f=sl_make(24,16,0,1);struct sl_client *v=f->client;
 struct pt_sampler_mixed_command_handle out={NULL,777};uint64_t first;unsigned calls;
 sl_open(f);first=sl_first(f,mode!=0);sl_control_batch(f,6);f->later_stage=2;calls=f->ordinary.calls;
 if(mode==0){assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,&out)==PT_SAMPLER_MIXED_INVALID);}
 if(mode==1)v->control.action[1].reader=v->control.action[0].reader;
 if(mode==2)v->control.action[0].reader.token++;
 if(mode==3)v->control.action[15].reader=v->reader[15];
 if(mode==4)v->control.frame=960;
 if(mode==5)v->control.action[0].rate=1;
 if(mode==6)v->control.action[4].period=1;
 if(mode==7)v->control.action[15].right=1;
 if(mode==8)v->control.count=17;
 if(mode>=1&&mode<=8)assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,&out)==PT_SAMPLER_MIXED_INVALID);
 if(mode==9){struct pt_sampler_mixed_causal_lineage_control_batch before=v->control;
  assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,(void *)&v->control.action[15])==PT_SAMPLER_MIXED_INVALID&&!memcmp(&before,&v->control,sizeof(before)));}
 if(mode==10)assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,(void *)f,&out)==PT_SAMPLER_MIXED_INVALID);
 if(mode==11)assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,(void *)v->reader[0].address)==PT_SAMPLER_MIXED_INVALID);
 if(mode==12){unsigned char bytes[sizeof(out)+8],before[sizeof(bytes)];memset(bytes,0xa5,sizeof(bytes));memcpy(before,bytes,sizeof(bytes));
  assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,(void *)(bytes+1))==PT_SAMPLER_MIXED_INVALID&&!memcmp(before,bytes,sizeof(bytes)));}
 assert(!out.address&&out.token==777&&f->ordinary.calls==calls);sl_drain(f,first,0,0,0,0,0,0);sl_drop(f);
}
static void sl_stop_guards(unsigned mode)
{
 struct sl_case *f=sl_make(24,16,0,1);struct sl_client *v=f->client;
 struct pt_sampler_mixed_command_handle out={NULL,777};uint64_t first,control;unsigned calls;
 sl_open(f);first=sl_first(f,1);control=sl_control(f,6,mode!=0);sl_root_close(f,first);sl_batch(f,6);f->later_stage=3;calls=f->ordinary.calls;
 if(mode==1)v->stop.reader[1]=v->stop.reader[0];
 if(mode==2)v->stop.reader[0].token++;
 if(mode==3)v->stop.reader[15]=v->reader[15];
 if(mode==4)v->stop.frame=1920;
 if(mode<=4)assert(pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,&out)==PT_SAMPLER_MIXED_INVALID);
 if(mode==5){struct pt_sampler_mixed_causal_lineage_stop_batch before=v->stop;
  assert(pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,(void *)&v->stop.reader[15])==PT_SAMPLER_MIXED_INVALID&&!memcmp(&before,&v->stop,sizeof(before)));}
 if(mode==6)assert(pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,(void *)f,&out)==PT_SAMPLER_MIXED_INVALID);
 if(mode==7)assert(pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,(void *)v->reader[0].address)==PT_SAMPLER_MIXED_INVALID);
 assert(!out.address&&out.token==777&&f->ordinary.calls==calls);sl_drain(f,first,control,0,1,0,0,0);sl_drop(f);
}
static void sl_allocation_fault(unsigned stage,unsigned mode)
{
 struct sl_case *f=sl_make(24,16,0,1);struct sl_client *v=f->client;
 struct pt_sampler_mixed_command_handle out={NULL,777};uint64_t first,control=0;unsigned calls,releases;
 enum pt_sampler_mixed_result wanted;sl_open(f);first=sl_first(f,1);
 if(stage==3){control=sl_control(f,6,1);sl_root_close(f,first);sl_batch(f,6);}else sl_control_batch(f,6);
 f->later_stage=stage;calls=f->ordinary.calls;releases=f->ordinary.releases;
 if(mode==0){f->ordinary.fail=1;wanted=PT_SAMPLER_MIXED_CAPACITY;}
 else if(mode==1){f->ordinary.alias=stage==2?(void *)&v->control:(void *)&v->stop;wanted=PT_SAMPLER_MIXED_INVALID;}
 else if(mode==2){f->ordinary.hook=2;wanted=PT_SAMPLER_MIXED_STALE;}
 else if(mode==3){f->ordinary.hook=1;wanted=PT_SAMPLER_MIXED_STALE;}
 else{f->owned_hook=1;wanted=PT_SAMPLER_MIXED_STALE;}
 assert((stage==2?pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,&out):
    pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,&out))==wanted);
 assert(!out.address&&out.token==777&&pt_mixed_readers_readers_held(v->config.queue)==16);
 assert(f->ordinary.calls==calls+(mode<4)&&f->ordinary.releases==releases+(mode==2||mode==3));
 if(mode<=1){f->ordinary.fail=0;f->ordinary.alias=NULL;
  assert((stage==2?pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,&out):
    pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,&out))==PT_SAMPLER_MIXED_INVALID&&f->ordinary.calls==calls+1);}
 sl_same(f);sl_drain(f,first,control,0,stage==3,0,0,mode>=2);sl_drop(f);
}
static void sl_publication_fault(unsigned stage,unsigned mode)
{
 struct sl_case *f=sl_make(24,16,0,1);struct sl_client *v=f->client;
 uint64_t first,control=0,stop=0,ticket;enum pt_mixed_readers_result actual;
 sl_open(f);first=sl_first(f,1);
 if(stage==2){f->later_stage=2;sl_control_batch(f,6);
  assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,v->command+1)==PT_SAMPLER_MIXED_PENDING);
  sl_advance(f,1);assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[1],&control)==PT_MIXED_READERS_OK);ticket=control;
  f->base->port.original.publication_raw=mode?1:0;
  f->base->port.original.publication_mutation=mode==1;f->base->port.original.publication_reentry=mode==2;
 }else{control=sl_control(f,6,1);sl_root_close(f,first);stop=sl_stop(f,6);ticket=stop;
  f->base->port.third_raw=mode?1:0;f->base->port.third_mutation=mode==1;f->base->port.third_reentry=mode==2;}
 actual=pt_sampler_mixed_publish(v->pool,ticket);
 assert(actual==(mode?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_PENDING));
 assert(!f->base->port.original.stops&&f->base->port.original.base.mask==0xffffU);
 sl_same(f);sl_drain(f,first,control,stop,stage==3,0,0,mode!=0);sl_drop(f);
}
static void sl_constructor_guard(unsigned mode)
{
 struct sl_case *f=sl_make(24,16,0,1);struct sl_client *v=f->client;
 struct pt_sampler_mixed_pool *out=(void *)(uintptr_t)777;
 struct pt_sampler_mixed_causal_lineage_binding *b=&f->binding;
 unsigned char output[sizeof(out)+_Alignof(struct pt_sampler_mixed_pool *)],before[sizeof(output)],*o=output;
 unsigned char *slot=f->stop_open_scratch;unsigned calls=f->ordinary.calls;
 assert(pt_sampler_mixed_causal_lineage_bind(b,f->base->owner,v->config.queue,19,17,
    (struct pt_mixed_readers_span){f,sizeof(*f)}));
 while((uintptr_t)slot%_Alignof(struct pt_sampler_mixed_causal_lineage_binding))++slot;
 if(mode==0){++slot;memcpy(slot,b,sizeof(*b));b=(void *)slot;}
 if(mode==1){if(!((uintptr_t)o%_Alignof(struct pt_sampler_mixed_pool *)))++o;}
 if(mode==2){memcpy(slot,b,sizeof(*b));b=(void *)slot;}
 if(mode==3)b->original.generation++;
 if(mode==4)v->config.contexts[0].bytes=sizeof(f->ordinary);
 if(mode==5)b->original.contexts.bytes--;
 memset(output,0xa5,sizeof(output));memcpy(before,output,sizeof(output));
 assert(pt_sampler_mixed_causal_lineage_open(&v->config,f->revision,f->workspace,f->capacity,b,
    mode==1?(void *)o:&out)==PT_SAMPLER_MIXED_INVALID);
 assert(out==(void *)(uintptr_t)777&&!memcmp(before,output,sizeof(output))&&f->ordinary.calls==calls);
 {size_t i;for(i=0;i<f->capacity;++i)assert(!((unsigned char *)f->workspace)[i]);}
 sl_drop(f);
}
static void sl_lower_actual_transfer(unsigned stage)
{
 struct sl_case *f=sl_make(24,16,0,1);struct sl_client *v=f->client;
 uint64_t first,control=0,stop=0,out=777;unsigned index=stage==2?1:0;
 sl_open(f);first=sl_first(f,1);
 if(stage==3){control=sl_control(f,2,1);sl_root_close(f,first);sl_batch(f,2);f->later_stage=3;
  assert(pt_sampler_mixed_causal_lineage_stop_begin(v->pool,f->revision,&v->stop,v->command)==PT_SAMPLER_MIXED_PENDING);
 }else{sl_control_batch(f,2);f->later_stage=2;
  assert(pt_sampler_mixed_causal_lineage_control_begin(v->pool,f->revision,&v->control,v->command+1)==PT_SAMPLER_MIXED_PENDING);}
 sl_advance(f,index);
 /* Two genuine selected ACTIVE holder getters precede the actual command
  * holder callback. Fault that lower callback while preserving its raw1. */
 f->owned_hook=2;f->owned_after=3;
 assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[index],&out)==PT_MIXED_READERS_OK&&out!=777&&out&&!f->owned_hook);
 if(stage==2)control=out;else stop=out;
 assert(!pt_sampler_mixed_command_close(v->pool,v->command+index)&&v->command[index].address);
 assert(pt_sampler_mixed_publish(v->pool,out)==PT_MIXED_READERS_INVALID);
 sl_same(f);sl_drain(f,first,control,stop,stage==3,0,0,1);sl_drop(f);
}
int sq_original74_fixture_not_called(void)
{
 unsigned bits,cache,card,order,mode,stage;
 for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(card=0;card<2;++card)for(order=0;order<2;++order)
    sl_success(bits,cache,card,order);
 sl_detachment(0);sl_detachment(1);
 for(mode=0;mode<3;++mode)sl_wrong_capability(mode);
 for(mode=0;mode<13;++mode)sl_control_guards(mode);
 for(mode=0;mode<8;++mode)sl_stop_guards(mode);
 for(stage=2;stage<=3;++stage)for(mode=0;mode<5;++mode)sl_allocation_fault(stage,mode);
 for(stage=2;stage<=3;++stage)for(mode=0;mode<3;++mode)sl_publication_fault(stage,mode);
 for(mode=0;mode<6;++mode)sl_constructor_guard(mode);
 sl_lower_actual_transfer(2);sl_lower_actual_transfer(3);
 assert(sl_cases==74);
 printf("SAMPLER CAUSAL LINEAGE FACTORY HOST COMPLETE cases=%u;24 mixed4/12 or16card 8/16/24-master cache8/16 lifetimes;exact960/1920/2880;two live C and32R bound;actual C1 service and NULL close;disposed opaque C/token reuse;CONTROL/STOP zero new R/pin/cache/upload;complete typed guards;consumed allocator attempts;publication uncertainty;independent C/R/SOURCE;full actual-capacity masters and exact save/cache beforeimages;NOT_NATIVE_CONTROLLER_DEVICE_AUDIO\n",sl_cases);
 return 0;
}
