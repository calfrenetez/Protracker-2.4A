/* Genuine factory production unit plus unchanged ordinary-RAM STOP port/resource
 * helpers. The inherited STOP/default entries are renamed and UNCALLED here;
 * separate executables provide their actual regression evidence. */
#include "sampler_mixed_causal_stop_port_helpers.c"
#include "../src/editor/sampler_mixed_causal_stop_internal.h"
#include "../src/editor/sampler_mixed_readers_internal.h"
#include "../src/core/mixed_readers_causal_factory_internal.h"
struct sf_case;
struct sf_ledger {void *data;size_t bytes;};
struct sf_memory {struct sf_case *owner;unsigned calls,releases,refusals,fail,hook;void *alias;struct sf_ledger live[40];};
struct sf_client {
 struct pt_sampler_mixed_config config;struct pt_sampler_mixed_pool *pool;
 struct pt_sampler_mixed_request trigger[16];
 struct pt_sampler_mixed_command_handle command[2];
 struct pt_sampler_mixed_reader_handle reader[16];
 struct pt_sampler_mixed_causal_stop_batch stop;
};
struct sf_case {
 struct cc_case *base;struct sf_client *client;
 struct sf_memory ordinary,chip;struct pt_sampler_mixed_causal_stop_binding binding;
 unsigned char stop_open_scratch[sizeof(struct pt_sampler_mixed_causal_stop_binding)+_Alignof(struct pt_sampler_mixed_causal_stop_binding)];
 struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_pcm pcm[PT_PROJECT_SAMPLES];
 void *workspace;size_t capacity;unsigned owned_hook,owned_after,owned_calls;uint32_t revision;
};
static unsigned sf_cases;
static void *sf_new(void *context,size_t bytes)
{
 struct sf_memory *m=context;struct sf_case *f=m->owner;unsigned i;void *p;++m->calls;
 if(m->hook){unsigned hook=m->hook;m->hook=0;
  if(hook==1){assert(pt_sampler_mixed_stop(f->client->pool)==PT_SAMPLER_MIXED_BUSY);}
  else {assert(hook==2);++f->client->stop.frame;}}
 if(m->fail){++m->refusals;return NULL;}if(m->alias){++m->refusals;return m->alias;}
 p=malloc(bytes);assert(p);for(i=0;i<40&&m->live[i].data;++i){}assert(i<40);
 m->live[i]=(struct sf_ledger){p,bytes};return p;
}
static void sf_free(void *context,void *p)
{struct sf_memory *m=context;unsigned i;for(i=0;i<40&&m->live[i].data!=p;++i){}assert(i<40);
 m->live[i]=(struct sf_ledger){NULL,0};++m->releases;free(p);}
static void sf_chip_free(void *context,void *p,size_t bytes)
{struct sf_memory *m=context;unsigned i;for(i=0;i<40&&m->live[i].data!=p;++i){}assert(i<40&&m->live[i].bytes==bytes);sf_free(context,p);}
static int sf_owned(void *context)
{
 struct sf_case *f=context;int result=bus_owned(&f->base->trial->resources->card);++f->owned_calls;
 if(f->owned_hook&&(!f->owned_after||!--f->owned_after)){unsigned hook=f->owned_hook;f->owned_hook=0;
  if(hook==1)++f->client->stop.frame;
  else {struct pt_mixed_causal_diagnostic d;assert(hook==2);assert(!pt_mixed_causal_diagnostic(f->base->owner,&d));}}
 return result;
}
static int sf_write(void *context,unsigned reg,uint32_t value)
{struct sf_case *f=context;return bus_write(&f->base->trial->resources->card,reg,value);}
static struct sf_case *sf_make(unsigned bits,unsigned cache_bits,unsigned all_card,unsigned bind)
{
 struct sf_case *f=calloc(1,sizeof(*f));struct resources *r;struct pt_sampler_mixed_config *c;unsigned i;
 assert(f);++sf_cases;f->client=calloc(1,sizeof(*f->client));assert(f->client);
 f->base=ss_make(bits,cache_bits,bind);r=f->base->trial->resources;c=&f->client->config;
 if(all_card)for(i=0;i<16;++i)r->document.project.channels.track[i].route=PT_AMIGUS;
 f->ordinary.owner=f->chip.owner=f;f->revision=37;
 for(i=0;i<r->document.project.sample_count;++i)assert(pt_sampler_pin(&r->sampler,&r->document.project,i,r->sampler.generation,f->pcm+i,f->pin+i)==PT_EDIT_OK);
 r->card.cache.context=f;r->card.cache.owned=sf_owned;r->card.cache.write32=sf_write;
 r->card.cache.arena.context=f;r->card.cache.arena.owned=sf_owned;r->card.cache.arena.write32=sf_write;
 c->allocator=(struct pt_allocator){&f->ordinary,sf_new,sf_free};c->sampler=&r->sampler;c->project=&r->document.project;
 c->queue=f->base->trial->queue;c->backend=&r->card.cache;c->chip_context=&f->chip;c->chip_allocate=sf_new;c->chip_release=sf_chip_free;
 c->control_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
 c->chip_budget=4096;c->generation=17;c->maximum_commands=2;c->maximum_readers=32;
 c->contexts[0]=(struct pt_mixed_readers_span){f,sizeof(*f)};
 c->contexts[1]=(struct pt_mixed_readers_span){&f->base->port,sizeof(f->base->port)};
 c->contexts[2]=(struct pt_mixed_readers_span){&f->base->memory,sizeof(f->base->memory)};
 c->contexts[3]=(struct pt_mixed_readers_span){f->base->owner,pt_mixed_causal_control_size()};
 c->contexts[4]=(struct pt_mixed_readers_span){&r->card,sizeof(r->card)};c->context_count=5;
 f->capacity=pt_sampler_mixed_workspace_size()+128;f->workspace=calloc(1,f->capacity);assert(f->workspace);
 /* ss_make's historical beforeimage preceded the optional route change. */
 free(f->base->before);f->base->before=save(r,&f->base->before_bytes);
 return f;
}
static void sf_open(struct sf_case *f)
{
 struct sf_client *v=f->client;enum pt_sampler_mixed_result result;unsigned steps=0;
 assert(pt_sampler_mixed_causal_stop_bind(&f->binding,f->base->owner,v->config.queue,19,17,(struct pt_mixed_readers_span){f,sizeof(*f)}));
 assert(pt_sampler_mixed_causal_stop_open(&v->config,f->revision,f->workspace,f->capacity,&f->binding,&v->pool)==PT_SAMPLER_MIXED_PENDING);
 do{result=pt_sampler_mixed_advance_validation(v->pool,f->revision,7);assert(++steps<10000);}while(result==PT_SAMPLER_MIXED_PENDING);
 assert(result==PT_SAMPLER_MIXED_OK&&!f->chip.calls&&!f->base->trial->resources->card.writes);
}
static void sf_advance(struct sf_case *f,unsigned command)
{
 enum pt_sampler_mixed_result result;unsigned steps=0,done=999,writes;
 do{writes=f->base->trial->resources->card.writes;result=pt_sampler_mixed_advance(f->client->pool,f->revision,f->client->command[command],&done);
  assert(f->base->trial->resources->card.writes-writes<=128&&++steps<2000);}while(result==PT_SAMPLER_MIXED_PENDING);
 assert(result==PT_SAMPLER_MIXED_OK&&done==(command?f->client->stop.count:16));
}
static uint64_t sf_first(struct sf_case *f,unsigned observed)
{
 struct sf_client *v=f->client;struct resources *r=f->base->trial->resources;uint64_t ticket=0;unsigned i;
 for(i=0;i<16;++i){struct pt_sampler_mixed_request *a=v->trigger+i;a->kind=PT_MIXED_READERS_TRIGGER;a->track=i;a->sample=i%2;a->expected=f->pin[i%2];
  if(r->document.project.channels.track[i].route==PT_PAULA){a->geometry.paula.period=428;a->geometry.paula.volume=64;}
  else{a->geometry.amigus.bits=r->cache_bits;a->geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,64,128};}}
 assert(pt_sampler_mixed_begin(v->pool,f->revision,960,v->trigger,16,v->command)==PT_SAMPLER_MIXED_PENDING);sf_advance(f,0);
 for(i=0;i<16;++i)assert(pt_sampler_mixed_reader(v->pool,v->command[0],i,v->reader+i)==PT_SAMPLER_MIXED_OK);
 assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[0],&ticket)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_publish(v->pool,ticket)==PT_MIXED_READERS_OK);
 assert(ct_command(&f->base->port.base,ticket)->packet.frame==960&&ct_command(&f->base->port.base,ticket)->packet.first==oracle(960)&&ct_command(&f->base->port.base,ticket)->packet.last==oracle(961));
 if(observed){cc_actual_first(f->base,ticket);assert(pt_sampler_mixed_service_command(v->pool,ticket,0,NULL)==PT_MIXED_READERS_OK);}
 return ticket;
}
static void sf_batch(struct sf_case *f,unsigned count)
{unsigned i;memset(&f->client->stop,0,sizeof(f->client->stop));f->client->stop.frame=1920;f->client->stop.count=count;
 for(i=0;i<count;++i)f->client->stop.reader[i]=f->client->reader[i];}
static void sf_drain(struct sf_case *f,uint64_t first,uint64_t second,unsigned failed)
{
 unsigned i;enum pt_mixed_readers_result expect=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
 f->ordinary.alias=f->chip.alias=NULL;f->ordinary.hook=f->chip.hook=f->owned_hook=0;
 f->base->port.hold_first=f->base->port.hold_stop=0;f->base->port.quiet_mask=~0U;
 if(second){enum pt_mixed_readers_result got=pt_sampler_mixed_service_command(f->client->pool,second,1,NULL);
  if(got!=expect)fprintf(stderr,"STOP drain case=%u C result=%u expected=%u held=%u\n",sf_cases,(unsigned)got,(unsigned)expect,pt_mixed_readers_commands_held(f->base->trial->queue));assert(got==expect);}
 if(pt_mixed_readers_commands_held(f->base->trial->queue))assert(pt_sampler_mixed_service_command(f->client->pool,first,1,NULL)==expect);
 for(i=0;i<16;++i)if(f->client->reader[i].address){
  struct pt_mixed_reader_retirement out=pt_sampler_mixed_retire_original_reader(f->client->pool,f->client->reader+i,1);
  assert(out.retirement_consumed&&out.result==expect&&!f->client->reader[i].address);}
 for(i=0;i<2;++i)if(f->client->command[i].address){int ok=pt_sampler_mixed_command_close(f->client->pool,f->client->command+i);assert(ok||!f->client->command[i].address);}
 assert(!pt_mixed_readers_commands_held(f->base->trial->queue)&&!pt_mixed_readers_readers_held(f->base->trial->queue));
}
static void sf_drop(struct sf_case *f)
{
 unsigned i;int ok;if(f->client->pool){ok=pt_sampler_mixed_close(&f->client->pool);assert(ok||!f->client->pool);}
 assert(!f->client->pool);
 for(i=0;i<PT_PROJECT_SAMPLES;++i)pt_sampler_unpin(f->pin[i]);
 for(i=0;i<40;++i)assert(!f->ordinary.live[i].data&&!f->chip.live[i].data);
 assert(f->ordinary.calls==f->ordinary.releases+f->ordinary.refusals&&f->chip.calls==f->chip.releases+f->chip.refusals);
 cc_drop(f->base,0);free(f->workspace);free(f->client);free(f);
}
static void sf_success(unsigned bits,unsigned cache_bits,unsigned all_card,unsigned order)
{
 struct sf_case *f=sf_make(bits,cache_bits,all_card,1);struct sf_client *v=f->client;struct resources *r=f->base->trial->resources;
 uint64_t first,second=0,out=777;unsigned i,alloc,chips,writes,leases=0;struct pt_mixed_readers_key key;
 struct ct_reader before[32];void *old_command;
 sf_open(f);first=sf_first(f,1);old_command=v->command[0].address;
 assert(pt_sampler_mixed_command_close(v->pool,v->command)&&!v->command[0].address);
 /* The actual original C was freed; no tombstone path may dereference it. */
 (void)old_command;memcpy(before,f->base->port.base.reader,sizeof(before));sf_batch(f,order?1:16);
 for(i=0;i<PT_CACHE_SLOTS;++i)leases+=r->card.cache.cache.entry[i].pins;
 alloc=f->ordinary.calls;chips=f->chip.calls;writes=r->card.writes;
 assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,v->command+1)==PT_SAMPLER_MIXED_PENDING);
 assert(f->ordinary.calls==alloc+1&&f->chip.calls==chips&&r->card.writes==writes&&pt_mixed_readers_readers_held(v->config.queue)==16);
 assert(pt_sampler_mixed_reader(v->pool,v->command[1],0,(struct pt_sampler_mixed_reader_handle *)&key)==PT_SAMPLER_MIXED_INVALID);
 sf_advance(f,1);
 {unsigned char ticket[sizeof(uint64_t)+8],saved[sizeof(ticket)];memset(ticket,0xa5,sizeof(ticket));memcpy(saved,ticket,sizeof(saved));
  assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[1],(void *)(ticket+1))==PT_MIXED_READERS_INVALID&&!memcmp(ticket,saved,sizeof(ticket)));}
 assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[1],&second)==PT_MIXED_READERS_OK&&second!=first);
 assert(pt_sampler_mixed_publish(v->pool,second)==PT_MIXED_READERS_OK);
 assert(v->stop.frame==1920&&ct_command(&f->base->port.base,second)->packet.frame==1920&&ct_command(&f->base->port.base,second)->packet.first==oracle(1920)&&ct_command(&f->base->port.base,second)->packet.last==oracle(1921));
 f->base->port.base.ticks=oracle(1920)-1;
 assert(pt_mixed_causal_fire(f->base->owner,second)==PT_MIXED_CAUSAL_EARLY&&!f->base->port.stops);
 assert(!memcmp(before,f->base->port.base.reader,sizeof(before)));
 f->base->port.base.ticks=oracle(1920);
 assert(pt_mixed_causal_fire(f->base->owner,second)==PT_MIXED_CAUSAL_COMMITTED&&f->base->port.stops==v->stop.count);
 assert(r->card.writes==writes&&f->chip.calls==chips&&f->ordinary.calls==alloc+1);
 assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,v->command)==PT_SAMPLER_MIXED_INVALID);
 assert(pt_sampler_mixed_begin(v->pool,f->revision,2880,v->trigger,16,v->command)==PT_SAMPLER_MIXED_CAPACITY);
 assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[1],&out)==PT_MIXED_READERS_INVALID&&out==777);
 if(order){struct pt_mixed_readers_reader_receipt receipt,saved;
  memset(&receipt,0xa5,sizeof(receipt));memcpy(&saved,&receipt,sizeof(saved));
  assert(pt_sampler_mixed_service_reader(v->pool,first,0,0,&receipt)==PT_MIXED_READERS_PENDING&&!memcmp(&receipt,&saved,sizeof(saved)));
  f->base->port.quiet_mask=1U<<ct_index(v->stop.reader[0].address?before[0].key.route:0,before[0].key.slot);
  assert(pt_sampler_mixed_service_reader(v->pool,first,0,0,NULL)==PT_MIXED_READERS_OK);
  assert(!pt_sampler_mixed_reader_close(v->pool,v->reader)); /* genuine STOP C still owns R reference */
 }else{assert(pt_sampler_mixed_service_command(v->pool,second,0,NULL)==PT_MIXED_READERS_OK);
  for(i=0;i<16;++i)assert(pt_sampler_mixed_service_reader(v->pool,first,i,0,NULL)==PT_MIXED_READERS_PENDING);}
 {unsigned now=0;for(i=0;i<PT_CACHE_SLOTS;++i)now+=r->card.cache.cache.entry[i].pins;assert(now==leases);}
 same_save(r,f->base->before,f->base->before_bytes);sf_drain(f,first,order?second:0,0);sf_drop(f);
}
static void sf_guards(unsigned mode)
{
 struct sf_case *f=sf_make(24,16,0,1);struct sf_client *v=f->client;struct pt_sampler_mixed_command_handle before={NULL,777};
 uint64_t first;unsigned calls;sf_open(f);first=sf_first(f,mode!=0);sf_batch(f,2);calls=f->ordinary.calls;
 if(mode==0){assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,&before)==PT_SAMPLER_MIXED_INVALID);
  cc_actual_first(f->base,first);assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,&before)==PT_SAMPLER_MIXED_INVALID);}
 if(mode==1)v->stop.reader[1]=v->stop.reader[0];
 if(mode==2)v->stop.reader[0].token++;
 if(mode==3)v->stop.reader[15]=v->reader[15];
 if(mode==4)v->stop.frame=960;
 if(mode>=1&&mode<=4)assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,&before)==PT_SAMPLER_MIXED_INVALID);
 if(mode==5){struct pt_sampler_mixed_causal_stop_batch saved;memcpy(&saved,&v->stop,sizeof(saved));
  assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,(void *)&v->stop.reader[15])==PT_SAMPLER_MIXED_INVALID&&!memcmp(&saved,&v->stop,sizeof(saved)));}
 if(mode==6)assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,(void *)v->reader[0].address)==PT_SAMPLER_MIXED_INVALID);
 if(mode==7)assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,(void *)f,&before)==PT_SAMPLER_MIXED_INVALID);
 if(mode==8)assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,(void *)f)==PT_SAMPLER_MIXED_INVALID);
 if(mode==9)assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,(void *)((unsigned char *)&v->stop+1),&before)==PT_SAMPLER_MIXED_INVALID);
 if(mode==10){unsigned char output[sizeof(before)+8],saved[sizeof(output)];memset(output,0xa5,sizeof(output));memcpy(saved,output,sizeof(saved));
  assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,(void *)(output+1))==PT_SAMPLER_MIXED_INVALID&&!memcmp(output,saved,sizeof(output)));}
 assert(!before.address&&before.token==777&&f->ordinary.calls==calls);sf_drain(f,first,0,0);sf_drop(f);
}
static void sf_refuse_owner(unsigned mode)
{
 struct sf_case *f=sf_make(16,8,0,0);struct pt_mixed_causal_stop_port stop={&f->base->port,sizeof(f->base->port),1,3,ss_publish_stop};
 struct pt_mixed_causal_control_port control={&f->base->port,sizeof(f->base->port),1,3,cc_publish_control};
 struct pt_sampler_mixed_causal_stop_binding saved;
 if(mode)assert(pt_mixed_causal_control_bind(f->base->owner,&control)==PT_MIXED_READERS_OK);
 memcpy(&saved,&f->binding,sizeof(saved));
 assert(!pt_sampler_mixed_causal_stop_bind(&f->binding,f->base->owner,f->client->config.queue,19,17,(struct pt_mixed_readers_span){f,sizeof(*f)})&&!memcmp(&saved,&f->binding,sizeof(saved)));
 if(!mode){assert(pt_mixed_causal_stop_bind(f->base->owner,&stop)==PT_MIXED_READERS_OK);sf_open(f);}
 sf_drop(f);
}
static void sf_misaligned_open(unsigned mode)
{
 struct sf_case *f=sf_make(24,16,0,1);struct sf_client *v=f->client;
 struct pt_sampler_mixed_pool *out=(void *)(uintptr_t)777;
 unsigned char before[sizeof(*f)],client_before[sizeof(*v)],*slot=f->stop_open_scratch;
 unsigned char output[sizeof(out)+_Alignof(struct pt_sampler_mixed_pool *)],output_before[sizeof(output)],*output_slot=output;
 unsigned calls=f->ordinary.calls,chip_calls=f->chip.calls,owned,i;
 assert(_Alignof(struct pt_sampler_mixed_causal_stop_binding)>1);
 assert(pt_sampler_mixed_causal_stop_bind(&f->binding,f->base->owner,v->config.queue,19,17,(struct pt_mixed_readers_span){f,sizeof(*f)}));
 owned=f->owned_calls;memset(output,0xa5,sizeof(output));memcpy(output_before,output,sizeof(output));
 if(!((uintptr_t)output_slot%_Alignof(struct pt_sampler_mixed_pool *)))++output_slot;
 assert((uintptr_t)output_slot%_Alignof(struct pt_sampler_mixed_pool *));
 if(!((uintptr_t)slot%_Alignof(struct pt_sampler_mixed_causal_stop_binding)))++slot;
 assert((uintptr_t)slot%_Alignof(struct pt_sampler_mixed_causal_stop_binding));
 assert((uintptr_t)slot>=(uintptr_t)f&&(uintptr_t)slot+sizeof(f->binding)<=(uintptr_t)f+sizeof(*f));
 memcpy(slot,&f->binding,sizeof(f->binding));memcpy(before,f,sizeof(*f));memcpy(client_before,v,sizeof(*v));
 assert(pt_sampler_mixed_causal_stop_open(&v->config,f->revision,f->workspace,f->capacity,mode?&f->binding:(void *)slot,mode?(void *)output_slot:&out)==PT_SAMPLER_MIXED_INVALID);
 assert(out==(void *)(uintptr_t)777&&!memcmp(output,output_before,sizeof(output))&&!memcmp(before,f,sizeof(*f))&&!memcmp(client_before,v,sizeof(*v)));
 assert(f->ordinary.calls==calls&&f->chip.calls==chip_calls&&f->owned_calls==owned);
 for(i=0;i<f->capacity;++i)assert(!((unsigned char *)f->workspace)[i]);
 sf_drop(f);
}
static void sf_allocation_fault(unsigned mode)
{
 struct sf_case *f=sf_make(24,16,0,1);struct sf_client *v=f->client;uint64_t first;unsigned calls,releases;
 enum pt_sampler_mixed_result expected;sf_open(f);first=sf_first(f,1);sf_batch(f,2);calls=f->ordinary.calls;releases=f->ordinary.releases;
 if(mode==0){f->ordinary.fail=1;expected=PT_SAMPLER_MIXED_CAPACITY;}
 else if(mode==1){f->ordinary.alias=&v->stop;expected=PT_SAMPLER_MIXED_INVALID;}
 else if(mode==2){f->ordinary.hook=2;expected=PT_SAMPLER_MIXED_STALE;}
 else if(mode==3){f->ordinary.hook=1;expected=PT_SAMPLER_MIXED_STALE;}
 else{f->owned_hook=1;expected=PT_SAMPLER_MIXED_STALE;}
 assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,v->command+1)==expected&&!v->command[1].address);
 assert(pt_mixed_readers_readers_held(v->config.queue)==16);
 if(mode<4)assert(f->ordinary.calls==calls+1);else assert(f->ordinary.calls==calls);
 if(mode==2||mode==3)assert(f->ordinary.releases==releases+1);else assert(f->ordinary.releases==releases);
 f->ordinary.fail=0;
 sf_drain(f,first,0,mode>=2);sf_drop(f);
}
static void sf_publication_fault(unsigned mode)
{
 struct sf_case *f=sf_make(8,16,0,1);struct sf_client *v=f->client;uint64_t first,second=0;
 sf_open(f);first=sf_first(f,1);sf_batch(f,2);assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,v->command+1)==PT_SAMPLER_MIXED_PENDING);
 sf_advance(f,1);
 if(mode==3){f->owned_hook=2;f->owned_after=3;} /* two genuine key getters first; fault actual lower enqueue holder */
 assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[1],&second)==PT_MIXED_READERS_OK&&second);
 if(mode==0)f->base->port.publication_raw=0;if(mode==1)f->base->port.publication_raw=-1;if(mode==2)f->base->port.publication_reentry=1;
 if(mode==3){assert(!f->owned_hook);assert(!pt_sampler_mixed_command_close(v->pool,v->command+1));
  assert(pt_sampler_mixed_publish(v->pool,second)==PT_MIXED_READERS_INVALID);
  assert(pt_mixed_causal_stop(f->base->owner)==PT_MIXED_READERS_BACKEND);second=0;
 }else if(!mode){assert(pt_sampler_mixed_publish(v->pool,second)==PT_MIXED_READERS_PENDING);
  assert(pt_sampler_mixed_service_command(v->pool,second,1,NULL)==PT_MIXED_READERS_INVALID);
  assert(!pt_sampler_mixed_command_close(v->pool,v->command+1));
  assert(pt_mixed_causal_stop(f->base->owner)==PT_MIXED_READERS_PENDING);second=0;
 }else assert(pt_sampler_mixed_publish(v->pool,second)==PT_MIXED_READERS_BACKEND);
 assert(pt_mixed_readers_readers_held(v->config.queue)==16&&!f->chip.releases);
 sf_drain(f,first,second,mode!=0);sf_drop(f);
}
static void sf_cancel_path(unsigned mode)
{
 struct sf_case *f=sf_make(24,8,0,1);struct sf_client *v=f->client;
 uint64_t first,second=0;unsigned writes,chips;
 sf_open(f);first=sf_first(f,1);sf_batch(f,1);writes=f->base->trial->resources->card.writes;chips=f->chip.calls;
 assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,v->command+1)==PT_SAMPLER_MIXED_PENDING);
 if(!mode){assert(pt_sampler_mixed_cancel(v->pool,v->command[1])==PT_SAMPLER_MIXED_OK);
  assert(pt_sampler_mixed_command_close(v->pool,v->command+1)&&!v->command[1].address);}
 else{sf_advance(f,1);assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[1],&second)==PT_MIXED_READERS_OK);
  if(mode==3){f->base->port.base.ticks=oracle(1920);assert(pt_sampler_mixed_publish(v->pool,second)==PT_MIXED_READERS_LATE);
   assert(!f->base->port.stop_publications&&!pt_sampler_mixed_command_close(v->pool,v->command+1));
   assert(pt_mixed_causal_stop(f->base->owner)==PT_MIXED_READERS_BACKEND);second=0;
  }else{assert(pt_sampler_mixed_publish(v->pool,second)==PT_MIXED_READERS_OK);
  assert(pt_sampler_mixed_service_reader(v->pool,first,mode==1?0:5,1,NULL)==PT_MIXED_READERS_OK);
  assert(pt_mixed_causal_fire(f->base->owner,second)==PT_MIXED_CAUSAL_INVALID&&!f->base->port.stops);
 }}
 assert(writes==f->base->trial->resources->card.writes&&chips==f->chip.calls);
 sf_drain(f,first,second,mode==3);sf_drop(f);
}
static void sf_foreign_reader(void)
{
 struct sf_case *a=sf_make(16,8,0,1),*b=sf_make(8,16,0,1);uint64_t first_a,first_b;unsigned calls;
 sf_open(a);sf_open(b);first_a=sf_first(a,1);first_b=sf_first(b,1);sf_batch(a,1);
 a->client->stop.reader[0]=b->client->reader[0];calls=a->ordinary.calls;
 assert(pt_sampler_mixed_causal_stop_begin(a->client->pool,a->revision,&a->client->stop,a->client->command+1)==PT_SAMPLER_MIXED_INVALID&&!a->client->command[1].address&&a->ordinary.calls==calls);
 sf_drain(a,first_a,0,0);sf_drain(b,first_b,0,0);sf_drop(a);sf_drop(b);
}
static void sf_active_changed(void)
{
 struct sf_case *f=sf_make(16,16,0,1);struct sf_client *v=f->client;uint64_t first,out=777;
 sf_open(f);first=sf_first(f,1);sf_batch(f,1);
 assert(pt_sampler_mixed_causal_stop_begin(v->pool,f->revision,&v->stop,v->command+1)==PT_SAMPLER_MIXED_PENDING);
 assert(pt_sampler_mixed_service_reader(v->pool,first,0,1,NULL)==PT_MIXED_READERS_OK); /* actual original key no longer ACTIVE */
 sf_advance(f,1);assert(pt_sampler_mixed_enqueue(v->pool,f->revision,v->command[1],&out)==PT_MIXED_READERS_INVALID&&out==777);
 assert(pt_sampler_mixed_cancel(v->pool,v->command[1])==PT_SAMPLER_MIXED_OK);
 assert(pt_sampler_mixed_command_close(v->pool,v->command+1));sf_drain(f,first,0,0);sf_drop(f);
}
int main(void)
{
 unsigned bits,cache,card,order,i;
 for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(card=0;card<2;++card)for(order=0;order<2;++order)sf_success(bits,cache,card,order);
 for(i=0;i<11;++i)sf_guards(i);sf_refuse_owner(0);sf_refuse_owner(1);
 for(i=0;i<5;++i)sf_allocation_fault(i);for(i=0;i<4;++i)sf_publication_fault(i);
 for(i=0;i<4;++i)sf_cancel_path(i);sf_foreign_reader();sf_active_changed();sf_misaligned_open(0);sf_misaligned_open(1);
 assert(sf_cases==55);
 puts("SAMPLER CAUSAL STOP FACTORY HOST PASS:55 genuine heap cases;24 mixed4/12 or16card 8/16/24-master cache8/16 lifetimes;distinct genuine empty STOP binding;completed first plus original ACTIVE handles;one new C and zero new R/persistent pin/cache/upload;actual transfer and independent C/R/source quiet;full fixed handle/output/context and whole-covered misaligned binding/output STOP-open guards;allocation/alias/mutation/reentry/publication uncertainty;independent requested absolute frame/window assertions and master-save beforeimages;NO_CONTROLLER_PRODUCER_NATIVE_APERTURE_OR_DEVICE_STOP_QUALIFICATION");
 return 0;
}
