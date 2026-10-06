/* Combined genuine factory + owned queue + copied ordinary-memory paired port.
 * Independent module fixtures also execute; this new constructor qualifies the
 * actual wiring, not a physical backend, timer, DMA, timing or listening. */
#include "sampler_mixed_readers_test.c"
#include "../src/core/mixed_readers_activation.h"
struct sma_command {struct pt_mixed_activation_packet packet;
unsigned live,committed;
};
struct sma_reader {struct pt_mixed_readers_key key;
struct pt_mixed_readers_action action;
struct pt_mixed_readers_card card;
unsigned live;
};
struct sma_port {
 uint64_t ticks;
unsigned reads,publishes,commits,command_calls,reader_calls,close_calls,quiet_calls,mask;
 unsigned reader_allow,partial;
int close_result,quiet_result;
 struct pt_mixed_readers_activation *callback_owner;
 struct pt_mixed_activation_registration registration;
 struct pt_mixed_readers_key slots[20];
struct sma_command command[2];
struct sma_reader reader[32];
};
struct sma_alloc {unsigned calls,releases,live;
};
struct sma_trial {
 struct smf_trial *factory;
struct pt_mixed_readers_output *original_queue;
 struct pt_mixed_readers_activation *activation;
struct sma_port port;
struct sma_alloc allocator;
 void *workspace;
};
static unsigned sma_slot(const struct pt_mixed_readers_key *k)
{assert(k->route==PT_MIXED_READERS_PAULA||k->route==PT_MIXED_READERS_AMIGUS);
return k->route==PT_MIXED_READERS_PAULA?k->slot:4+k->slot;
}
static struct sma_command *sma_command(struct sma_port *p,uint64_t ticket)
{unsigned i;
for(i=0;
i<2;
++i)if(p->command[i].live&&p->command[i].packet.ticket==ticket)return p->command+i;
return NULL;
}
static struct sma_reader *sma_reader(struct sma_port *p,const struct pt_mixed_readers_key *key)
{unsigned i;
for(i=0;
i<32;
++i)if(p->reader[i].live&&keys_equal(&p->reader[i].key,key))return p->reader+i;
return NULL;
}
static void *sma_allocate(void *context,size_t bytes)
{struct sma_alloc *a=context;
void *p=malloc(bytes);
assert(p);
a->calls++;
a->live++;
return p;
}
static void sma_release(void *context,void *pointer)
{struct sma_alloc *a=context;
assert(a->live);
a->live--;
a->releases++;
free(pointer);
}
static int sma_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct sma_port *p=context;
p->reads++;
*ticks=p->ticks;
*frequency=709379;
return 1;
}
static int sma_registration_same(const struct pt_mixed_activation_registration *,const struct pt_mixed_activation_registration *);
static int sma_publish(void *context,struct pt_mixed_readers_activation *owner,const struct pt_mixed_activation_packet *packet)
{
 struct sma_port *p=context;
unsigned i,j;
struct sma_command *c=NULL;
 p->publishes++;
assert(owner==packet->registration.owner&&packet->registration.queue);
 assert(packet->registration.session==31&&packet->registration.generation==17);
 if(p->registration.owner)assert(sma_registration_same(&p->registration,&packet->registration));
 assert(packet->expected_mask==p->mask&&packet->count<=16&&p->ticks<packet->first);
 for(i=0;
i<20;
++i)assert(keys_equal(p->slots+i,packet->expected+i));
 for(i=0;
i<2;
++i)if(!p->command[i].live){c=p->command+i;
break;
}
assert(c);
 memcpy(&c->packet,packet,sizeof(c->packet));
c->live=1;
c->committed=0;
p->registration=packet->registration;
p->callback_owner=owner;
 for(i=0;
i<packet->count;
++i)if(packet->action[i].kind==PT_MIXED_READERS_TRIGGER){
  struct sma_reader *r=NULL;
for(j=0;
j<32;
++j)if(!p->reader[j].live){r=p->reader+j;
break;
}
assert(r);
  r->key=packet->key[i];
r->action=packet->action[i];
r->card=packet->card[i];
r->live=1;
 }
else assert(sma_reader(p,packet->key+i));
 return 1;
}
static int sma_commit(void *context,const struct pt_mixed_activation_packet *packet,struct pt_mixed_activation_actual *actual)
{
 struct sma_port *p=context;
struct sma_command *c=sma_command(p,packet->ticket);
unsigned i,limit,touched=0;
 p->commits++;
assert(c&&!c->committed&&p->ticks>=packet->first&&p->ticks<packet->last);
 assert(packet->expected_mask==p->mask);
for(i=0;
i<20;
++i)assert(keys_equal(p->slots+i,packet->expected+i));
 assert(sma_registration_same(&p->registration,&packet->registration));
 assert(!memcmp(&c->packet,packet,sizeof(*packet)));
 /* Validate the entire copied batch before the first possible effect. */
 for(i=0;
i<packet->count;
++i){
  unsigned slot=sma_slot(packet->key+i);
  const struct pt_mixed_readers_action *a=packet->action+i;
  struct sma_reader *r=sma_reader(p,packet->key+i);
  assert(slot<20&&!(touched&(1U<<slot))&&r);
touched|=1U<<slot;
  assert(a->route==packet->key[i].route&&a->slot==packet->key[i].slot);
  if(a->kind!=PT_MIXED_READERS_TRIGGER)assert(keys_equal(p->slots+slot,packet->key+i));
  else if(a->route==PT_MIXED_READERS_PAULA)assert(r->action.geometry.paula.data&&r->action.geometry.paula.words);
  else assert(r->card.logical_bytes&&r->card.full_capacity>=r->card.logical_bytes&&r->card.serial);
 }
 limit=p->partial?1:packet->count;
 for(i=0;
i<limit;
++i){const struct pt_mixed_readers_action *a=packet->action+i;
unsigned slot=sma_slot(packet->key+i);
  assert(slot<20&&sma_reader(p,packet->key+i));
  if(a->kind==PT_MIXED_READERS_STOP){assert(keys_equal(p->slots+slot,packet->key+i));
memset(p->slots+slot,0,sizeof(p->slots[slot]));
p->mask&=~(1U<<slot);
}
  else {if(a->kind==PT_MIXED_READERS_CONTROL){struct sma_reader *r=sma_reader(p,packet->key+i);
assert(keys_equal(p->slots+slot,packet->key+i));
    if(a->route==PT_MIXED_READERS_PAULA){r->action.geometry.paula.period=a->geometry.paula.period;
r->action.geometry.paula.volume=a->geometry.paula.volume;
}
    else {r->action.geometry.amigus.rate=a->geometry.amigus.rate;
r->action.geometry.amigus.left=a->geometry.amigus.left;
r->action.geometry.amigus.right=a->geometry.amigus.right;
}
   }
p->slots[slot]=packet->key[i];
p->mask|=1U<<slot;
}
 }
 c->committed=1;
memset(actual,0,sizeof(*actual));
actual->active_mask=actual->adopted_mask=p->mask;
memcpy(actual->slot,p->slots,sizeof(p->slots));
 return p->partial?-1:1;
}
static int sma_command_quiet(void *context,const struct pt_mixed_activation_command_identity *identity,unsigned cancel)
{
 struct sma_port *p=context;
struct sma_command *c=sma_command(p,identity->ticket);
p->command_calls++;
 assert(c&&sma_registration_same(&identity->registration,&p->registration)&&sma_registration_same(&identity->registration,&c->packet.registration));
 if(!cancel&&!c->committed)return 0;
memset(c,0,sizeof(*c));
return 1;
}
static int sma_reader_quiet(void *context,const struct pt_mixed_activation_reader_identity *identity,unsigned cancel)
{
 struct sma_port *p=context;
struct sma_reader *r=sma_reader(p,&identity->key);
unsigned i,j,slot=sma_slot(&identity->key);
 p->reader_calls++;
assert(r&&sma_registration_same(&identity->registration,&p->registration));
 if(!p->reader_allow)return 0;
 if(!cancel&&(p->mask&(1U<<slot))&&keys_equal(p->slots+slot,&identity->key))return 0;
 for(i=0;
i<2;
++i)if(p->command[i].live)for(j=0;
j<p->command[i].packet.count;
++j)
  if(keys_equal(p->command[i].packet.key+j,&identity->key)){
   struct pt_mixed_activation_registration registration=p->command[i].packet.registration;
   uint64_t ticket=p->command[i].packet.ticket;
   if(!cancel)return 0;
   /* Reader quiet cancels the whole waiting packet and erases geometry;
    * its independent command identity survives until command quiet. */
   memset(&p->command[i].packet,0,sizeof(p->command[i].packet));
   p->command[i].packet.registration=registration;
p->command[i].packet.ticket=ticket;
   p->command[i].committed=1;
break;
  }
 if(keys_equal(p->slots+slot,&identity->key)){memset(p->slots+slot,0,sizeof(p->slots[slot]));
p->mask&=~(1U<<slot);
}
 memset(r,0,sizeof(*r));
return 1;
}
static int sma_registration_same(const struct pt_mixed_activation_registration *a,const struct pt_mixed_activation_registration *b)
{return a->owner==b->owner&&a->queue==b->queue&&a->session==b->session&&a->generation==b->generation;
}
static int sma_source_close(void *context,const struct pt_mixed_activation_registration *registration)
{
 struct sma_port *p=context;
unsigned i;
p->close_calls++;
assert(p->close_calls==1&&registration->owner&&registration->queue&&registration->session==31&&registration->generation==17);
 if(p->registration.owner)assert(sma_registration_same(&p->registration,registration));
else p->registration=*registration;
 for(i=0;
i<2;
++i)assert(!p->command[i].live);
for(i=0;
i<32;
++i)assert(!p->reader[i].live);
 if(p->close_result==1)p->callback_owner=NULL;
return p->close_result;
}
static int sma_source_quiet(void *context,const struct pt_mixed_activation_registration *registration)
{
 struct sma_port *p=context;
unsigned i;
 p->quiet_calls++;
 assert(p->close_calls==1&&sma_registration_same(&p->registration,registration));
 if(p->quiet_result==1){
  assert(!p->callback_owner);
  for(i=0;
i<2;
++i)assert(!p->command[i].live);
  for(i=0;
i<32;
++i)assert(!p->reader[i].live);
 }
 return p->quiet_result;
}
static struct sma_trial *sma_make(unsigned bits,unsigned cache_bits,unsigned frames)
{
 struct sma_trial *t=calloc(1,sizeof(*t));
struct pt_mixed_activation_config config;
struct smf_trial *f;
assert(t);
 f=t->factory=smf_make(bits,cache_bits,1,frames,0);
t->original_queue=f->inherited->queue;
 memset(&config,0,sizeof(config));
config.allocator=(struct pt_allocator){&t->allocator,sma_allocate,sma_release};
 config.allocator_context=(struct pt_mixed_readers_span){&t->allocator,sizeof(t->allocator)};
config.grid=f->inherited->config.grid;
config.session=31;
 config.control_budget=pt_mixed_activation_control_size();
config.queue_budget=pt_mixed_readers_control_size();
 config.port=(struct pt_mixed_activation_port){&t->port,sizeof(t->port),PT_MIXED_ACTIVATION_PORT_VERSION,PT_MIXED_ACTIVATION_PORT_REQUIRED,sma_clock,sma_publish,sma_commit,sma_command_quiet,sma_reader_quiet,sma_source_close,sma_source_quiet};
 t->port.ticks=100;
t->port.reader_allow=1;
t->port.close_result=t->port.quiet_result=1;
 t->workspace=calloc(1,pt_mixed_activation_workspace_size());
assert(t->workspace);
 assert(pt_mixed_activation_open(&config,t->workspace,pt_mixed_activation_workspace_size(),&t->activation)==PT_MIXED_READERS_OK);
 assert(t->allocator.calls==2&&t->allocator.live==2&&!t->port.reads&&!t->port.publishes);
 assert(pt_mixed_activation_borrow_queue(t->activation,&f->inherited->queue)==PT_MIXED_READERS_OK);
 f->config.queue=f->inherited->queue;
 f->config.contexts[f->config.context_count++]=(struct pt_mixed_readers_span){t->activation,pt_mixed_activation_control_size()};
 f->config.contexts[f->config.context_count++]=(struct pt_mixed_readers_span){&t->allocator,sizeof(t->allocator)};
 f->config.contexts[f->config.context_count++]=(struct pt_mixed_readers_span){&t->port,sizeof(t->port)};
 return t;
}
static void sma_drop(struct sma_trial *t)
{
 struct smf_trial *f=t->factory;
assert(!f->pool&&!t->activation&&!t->port.callback_owner&&!t->allocator.live&&t->allocator.calls==t->allocator.releases);
 f->inherited->queue=t->original_queue;
smf_drop(f);
free(t->workspace);
free(t);
}
static void sma_prepare(struct sma_trial *t,unsigned ci,uint64_t frame)
{unsigned reads=t->port.reads,publishes=t->port.publishes,commits=t->port.commits,cc=t->port.command_calls,rc=t->port.reader_calls;
smf_prepare(t->factory,ci,frame);
assert(t->port.reads==reads&&t->port.publishes==publishes&&t->port.commits==commits&&t->port.command_calls==cc&&t->port.reader_calls==rc);
}
static void sma_literals(struct sma_trial *t,uint64_t ticket,unsigned little)
{
 struct sma_command *c=sma_command(&t->port,ticket);
struct resources *r=t->factory->inherited->resources;
unsigned i,j;
assert(c);
 for(i=0;
i<c->packet.count;
++i){const struct pt_mixed_readers_action *a=c->packet.action+i;
const struct pt_mixed_readers_card *card=c->packet.card+i;
struct pt_pcm *pcm=&r->document.project.samples[i%2].pcm;
unsigned channel=i<4?0:1;
  if(i<4){assert(a->geometry.paula.words==((pcm->frames+1)&~1U)/2);
for(j=0;
j<pcm->frames;
++j)assert(a->geometry.paula.data[j]==smf_convert8(pcm->data[(size_t)j*2+channel],r->bits));
if(pcm->frames&1)assert(!a->geometry.paula.data[pcm->frames]);
}
  else {void *resource=pt_cache_data(&r->card.cache.cache,(struct pt_cache_lease){card->cache_slot,card->serial});
unsigned block;
   for(block=0;
block<32&&resource!=&r->card.cache.arena.block[block];
++block){}
assert(block<32&&card->full_capacity==r->card.cache.arena.block[block].reserved);
   assert(card->logical_bytes==pcm->frames*r->cache_bits/8&&card->source_channel==1&&card->bits==r->cache_bits&&card->little_endian==little);
   for(j=0;
j<pcm->frames;
++j){uint32_t address=card->address+j*r->cache_bits/8;
int32_t v=pcm->data[(size_t)j*2+channel];
if(r->cache_bits==8)assert(r->card.ram[address]==smf_convert8(v,r->bits));
else{uint16_t value=smf_convert16(v,r->bits);
assert(r->card.ram[address]==(uint8_t)(little?value:value>>8));
assert(r->card.ram[address+1]==(uint8_t)(little?value>>8:value));
}
}
  }
 }
}
static void sma_trigger(struct sma_trial *t,unsigned little,uint64_t *ticket)
{
 struct smf_trial *f=t->factory;
unsigned i;
struct pt_mixed_readers_key key;
smf_open(f);
smf_requests(f,16,little);
sma_prepare(t,0,960);
smf_handles(f,0,0);
 /* The factory readers now own independent pins; caller expected pins expire. */
 for(i=0;
i<f->config.project->sample_count;
++i){pt_sampler_unpin(f->pins[i]);
f->pins[i]=NULL;
}
 assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],ticket)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_reader_key(f->pool,f->reader[0],&key)==PT_MIXED_READERS_STALE);
 assert(pt_sampler_mixed_publish(f->pool,*ticket)==PT_MIXED_READERS_OK);
sma_literals(t,*ticket,little);
 assert(t->port.publishes==1&&!t->port.commits&&pt_mixed_readers_readers_held(f->inherited->queue)==16);
 assert(pt_sampler_mixed_reader_key(f->pool,f->reader[0],&key)==PT_MIXED_READERS_STALE);
 t->port.ticks=oracle(960)-1;
assert(pt_mixed_activation_fire(t->activation,*ticket)==PT_MIXED_ACTIVATION_EARLY&&!t->port.commits&&!t->port.mask);
 t->port.ticks=oracle(960);
assert(pt_mixed_activation_fire(t->activation,*ticket)==(t->port.partial?PT_MIXED_ACTIVATION_FAILED:PT_MIXED_ACTIVATION_COMMITTED));
}
static void sma_lifetime(unsigned bits,unsigned cache_bits,unsigned little)
{
 struct sma_trial *t=sma_make(bits,cache_bits,cache_bits==8?6:5);
struct smf_trial *f=t->factory;
uint64_t ticket,next;
struct pt_mixed_readers_key key[16];
unsigned i,stop;
 sma_trigger(t,little,&ticket);
assert(t->port.commits==1&&t->port.mask==0xffffU);
 assert(pt_sampler_mixed_service_command(f->pool,ticket,0,NULL)==PT_MIXED_READERS_OK);
 for(i=0;
i<16;
++i){assert(pt_sampler_mixed_service_reader(f->pool,ticket,i,0,NULL)==PT_MIXED_READERS_PENDING);
assert(pt_sampler_mixed_reader_key(f->pool,f->reader[i],key+i)==PT_MIXED_READERS_OK);
assert(key[i].trigger==ticket&&keys_equal(key+i,t->port.slots+i));
}
 assert(pt_sampler_mixed_command_close(f->pool,f->command)&&pt_mixed_readers_readers_held(f->inherited->queue)==16);
 for(stop=0;
stop<2;
++stop){for(i=0;
i<16;
++i){struct pt_sampler_mixed_request *x=f->request+i;
memset(x,0,sizeof(*x));
x->kind=stop?PT_MIXED_READERS_STOP:PT_MIXED_READERS_CONTROL;
x->track=i;
x->sample=i%2;
x->channel=i<4?0:1;
x->key=key[i];
if(!stop){if(i<4){x->geometry.paula.period=214;
x->geometry.paula.volume=32;
}
else{x->geometry.amigus.rate=1000;
x->geometry.amigus.left=300;
x->geometry.amigus.right=400;
}
}
}
  sma_prepare(t,1,stop?2880:1920);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[1],&next)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,next)==PT_MIXED_READERS_OK);
  t->port.ticks=oracle(stop?2880:1920);
assert(pt_mixed_activation_fire(t->activation,next)==PT_MIXED_ACTIVATION_COMMITTED);
  assert(pt_sampler_mixed_service_command(f->pool,next,0,NULL)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_command_close(f->pool,f->command+1));
  if(!stop)for(i=0;
i<16;
++i){struct pt_mixed_readers_key original;
struct sma_reader *r=sma_reader(&t->port,key+i);
assert(r&&pt_sampler_mixed_reader_key(f->pool,f->reader[i],&original)==PT_MIXED_READERS_OK&&keys_equal(&original,key+i));
   if(i<4)assert(r->action.geometry.paula.data&&r->action.geometry.paula.words&&r->action.geometry.paula.period==214&&r->action.geometry.paula.volume==32);
   else assert(r->action.geometry.amigus.end_exclusive&&r->action.geometry.amigus.rate==1000&&r->action.geometry.amigus.left==300&&r->action.geometry.amigus.right==400);
  }
 }
 assert(!t->port.mask&&pt_mixed_readers_readers_held(f->inherited->queue)==16);
 t->port.reader_allow=0;
assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,1,NULL)==PT_MIXED_READERS_PENDING);
assert(!pt_sampler_mixed_reader_close(f->pool,f->reader)&&f->reader[0].address);
 t->port.reader_allow=1;
for(i=0;
i<16;
++i){assert(pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_reader_close(f->pool,f->reader+i));
}
 assert(!pt_mixed_readers_commands_held(f->inherited->queue)&&!pt_mixed_readers_readers_held(f->inherited->queue));
smf_verify(f);
 assert(pt_sampler_mixed_close(&f->pool));
assert(pt_mixed_activation_close(&t->activation)&&!t->activation&&t->port.close_calls==1&&!t->port.quiet_calls);
sma_drop(t);
}
static void sma_partial(void)
{
 struct sma_trial *t=sma_make(24,16,5);
struct smf_trial *f=t->factory;
uint64_t ticket;
unsigned i;
t->port.partial=1;
sma_trigger(t,1,&ticket);
 assert(t->port.mask==1&&pt_mixed_readers_commands_held(f->inherited->queue)==1&&pt_mixed_readers_readers_held(f->inherited->queue)==16);
 t->port.reader_allow=0;
assert(pt_sampler_mixed_service_command(f->pool,ticket,1,NULL)==PT_MIXED_READERS_BACKEND);
 assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,1,NULL)==PT_MIXED_READERS_BACKEND&&pt_mixed_readers_readers_held(f->inherited->queue)==16);
 assert(!pt_sampler_mixed_reader_close(f->pool,f->reader)&&f->reader[0].address);
 t->port.reader_allow=1;
for(i=0;
i<16;
++i){assert(pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL)==PT_MIXED_READERS_BACKEND);
assert(pt_sampler_mixed_reader_close(f->pool,f->reader+i));
}
 assert(pt_sampler_mixed_command_close(f->pool,f->command));
assert(!pt_mixed_readers_readers_held(f->inherited->queue));
smf_verify(f);
assert(pt_sampler_mixed_close(&f->pool));
 t->port.close_result=-1;
assert(!pt_mixed_activation_close(&t->activation)&&t->activation&&t->allocator.live==1&&t->port.close_calls==1);
 t->port.quiet_result=0;
assert(!pt_mixed_activation_close(&t->activation)&&t->activation&&t->port.close_calls==1&&t->port.quiet_calls==1);
 /* An independently observed MODEL source stopped between explicit calls.
  * The later source_quiet callback only observes that state. */
 t->port.callback_owner=NULL;
t->port.quiet_result=1;
 assert(pt_mixed_activation_close(&t->activation)&&!t->activation&&t->port.close_calls==1&&t->port.quiet_calls==2);
assert(pt_mixed_activation_close(&t->activation));
sma_drop(t);
}
static void sma_reader_first(void)
{
 struct sma_trial *t=sma_make(16,16,6);
 struct smf_trial *f=t->factory;
 uint64_t ticket;
 unsigned i;
 smf_open(f);
smf_requests(f,5,0);
sma_prepare(t,0,960);
smf_handles(f,0,0);
 assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
 assert(!t->port.commits);
 for(i=0;
i<5;
++i){
  assert(pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL)==PT_MIXED_READERS_OK);
  assert(pt_mixed_readers_readers_held(f->inherited->queue)==5);
  assert(!pt_sampler_mixed_reader_close(f->pool,f->reader+i));
 }
 assert(!t->port.mask&&!t->port.commits&&sma_command(&t->port,ticket));
 assert(pt_sampler_mixed_service_command(f->pool,ticket,1,NULL)==PT_MIXED_READERS_OK);
 assert(!pt_mixed_readers_readers_held(f->inherited->queue));
 for(i=0;
i<5;
++i)assert(pt_sampler_mixed_reader_close(f->pool,f->reader+i));
 assert(pt_sampler_mixed_command_close(f->pool,f->command));
 assert(pt_sampler_mixed_close(&f->pool));
 assert(pt_mixed_activation_close(&t->activation));
sma_drop(t);
}
static void sma_replacement(void)
{
 struct sma_trial *t=sma_make(24,16,6);
 struct smf_trial *f=t->factory;
 struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,999};
 struct pt_mixed_readers_key replacement[20];
 uint64_t first,second;
 unsigned i,reads,calls;
 smf_open(f);
 smf_requests(f,16,0);
 sma_prepare(t,0,960);
 smf_handles(f,0,0);
 assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&first)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_publish(f->pool,first)==PT_MIXED_READERS_OK);
 t->port.ticks=oracle(960);
 assert(pt_mixed_activation_fire(t->activation,first)==PT_MIXED_ACTIVATION_COMMITTED);
 assert(pt_sampler_mixed_service_command(f->pool,first,0,NULL)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_command_close(f->pool,f->command));
 smf_requests(f,16,1);
 sma_prepare(t,0,1920);
 smf_handles(f,0,16);
 assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&second)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_publish(f->pool,second)==PT_MIXED_READERS_OK);
 t->port.ticks=oracle(1920);
 assert(pt_mixed_activation_fire(t->activation,second)==PT_MIXED_ACTIVATION_COMMITTED);
 assert(pt_sampler_mixed_service_command(f->pool,second,0,NULL)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_command_close(f->pool,f->command));
 assert(pt_mixed_readers_readers_held(f->inherited->queue)==32);
 memcpy(replacement,t->port.slots,sizeof(replacement));
 smf_requests(f,1,0);
 reads=t->port.reads;
calls=f->ordinary.calls;
 assert(pt_sampler_mixed_begin(f->pool,f->revision,2880,f->request,1,&out)==PT_SAMPLER_MIXED_CAPACITY);
 assert(out.address==(void *)(uintptr_t)1&&out.token==999&&calls==f->ordinary.calls&&reads==t->port.reads);
 for(i=0;
i<16;
++i){
  struct pt_mixed_readers_key key;
  assert(pt_sampler_mixed_service_reader(f->pool,first,i,1,NULL)==PT_MIXED_READERS_OK);
  assert(!memcmp(replacement,t->port.slots,sizeof(replacement))&&t->port.mask==0xffffU);
  assert(pt_sampler_mixed_service_reader(f->pool,second,i,0,NULL)==PT_MIXED_READERS_PENDING);
  assert(pt_sampler_mixed_reader_key(f->pool,f->reader[16+i],&key)==PT_MIXED_READERS_OK);
  assert(key.trigger==second&&keys_equal(&key,replacement+i));
 }
 assert(pt_mixed_readers_readers_held(f->inherited->queue)==16);
 for(i=0;
i<16;
++i)assert(pt_sampler_mixed_service_reader(f->pool,second,i,1,NULL)==PT_MIXED_READERS_OK);
 for(i=0;
i<32;
++i)assert(pt_sampler_mixed_reader_close(f->pool,f->reader+i));
 assert(!t->port.mask);
 smf_verify(f);
 assert(pt_sampler_mixed_close(&f->pool));
 assert(pt_mixed_activation_close(&t->activation));
 sma_drop(t);
}
static void sma_fragmentation(void)
{
 struct sma_trial *t=sma_make(8,8,6);
 struct smf_trial *f=t->factory;
 struct resources *r=f->inherited->resources;
 const struct pt_mixed_readers_card *card;
 struct sma_command *command;
 uint64_t ticket;
 unsigned i;
 r->card.cache.cache.budget=12;
 smf_open(f);
 smf_requests(f,5,0);
 f->request[0]=f->request[4];
f->request[0].track=4;
 f->request[0].sample=0;
f->request[0].expected=f->pins[0];
 f->request[1]=f->request[0];
f->request[1].track=5;
 f->request[1].sample=1;
f->request[1].expected=f->pins[1];
f->count=2;
 sma_prepare(t,0,960);
 assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
 assert(pt_sampler_mixed_command_close(f->pool,f->command));
 f->request[0].geometry.amigus.little_endian=1;
f->count=1;
 sma_prepare(t,0,1920);
smf_handles(f,0,0);
 assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
 command=sma_command(&t->port,ticket);
assert(command&&command->packet.count==1);
 card=&command->packet.card[0];
 assert(card->cache_slot==2&&card->logical_bytes==6&&card->full_capacity==8&&card->address==0);
 assert(pt_cache_data(&r->card.cache.cache,(struct pt_cache_lease){card->cache_slot,card->serial})==&r->card.cache.arena.block[0]);
 for(i=0;
i<6;
++i)assert(r->card.ram[i]==smf_convert8(f->pcm[0].data[i*2+1],8));
 assert(command->packet.action[0].geometry.amigus.end_exclusive==6);
 t->port.ticks=oracle(1920);
 assert(pt_mixed_activation_fire(t->activation,ticket)==PT_MIXED_ACTIVATION_COMMITTED&&t->port.mask==(1U<<4));
 assert(pt_sampler_mixed_service_command(f->pool,ticket,0,NULL)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,1,NULL)==PT_MIXED_READERS_OK);
 assert(pt_sampler_mixed_command_close(f->pool,f->command));
 assert(pt_sampler_mixed_reader_close(f->pool,f->reader));
 smf_verify(f);
assert(pt_sampler_mixed_close(&f->pool));
 assert(pt_mixed_activation_close(&t->activation));
sma_drop(t);
}
static void sma_unpublished_aliases(void)
{
 struct sma_trial *t=sma_make(16,16,6);
 struct smf_trial *f=t->factory;
 struct pt_mixed_readers_span aliases[6];
 unsigned i,reads,allocations;
 smf_open(f);
smf_requests(f,5,0);
sma_prepare(t,0,960);
smf_handles(f,0,0);
 aliases[0]=(struct pt_mixed_readers_span){t->activation,pt_mixed_activation_control_size()};
 aliases[1]=(struct pt_mixed_readers_span){&t->port,sizeof(t->port)};
 aliases[2]=(struct pt_mixed_readers_span){f->inherited->queue,pt_mixed_readers_control_size()};
 aliases[3]=(struct pt_mixed_readers_span){f->command[0].address,pt_sampler_mixed_command_size()};
 aliases[4]=(struct pt_mixed_readers_span){f->reader[0].address,pt_sampler_mixed_reader_size()};
 aliases[5]=(struct pt_mixed_readers_span){f->pcm[0].data,f->pcm[0].frames*f->pcm[0].channels*sizeof(*f->pcm[0].data)};
 reads=t->port.reads;
allocations=f->ordinary.calls;
 for(i=0;
i<6;
++i){
  void *copy=malloc(aliases[i].bytes);
assert(copy);
memcpy(copy,aliases[i].data,aliases[i].bytes);
  assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],(void *)aliases[i].data)==PT_MIXED_READERS_INVALID);
  assert(!memcmp(copy,aliases[i].data,aliases[i].bytes));
free(copy);
  assert(t->port.reads==reads&&!t->port.publishes&&!t->port.commits&&f->ordinary.calls==allocations);
 }
 assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
 for(i=0;
i<5;
++i)assert(pt_sampler_mixed_reader_close(f->pool,f->reader+i));
 assert(pt_sampler_mixed_command_close(f->pool,f->command));
 assert(pt_sampler_mixed_close(&f->pool));
 assert(pt_mixed_activation_close(&t->activation));
sma_drop(t);
}
static void __attribute__((constructor)) sma_suite(void)
{unsigned bits,cache_bits,little;
for(bits=8;
bits<=24;
bits+=8)for(cache_bits=8;
cache_bits<=16;
cache_bits+=8)for(little=0;
little<2;
++little)sma_lifetime(bits,cache_bits,little);
sma_partial();
sma_reader_first();
sma_replacement();
sma_fragmentation();
sma_unpublished_aliases();
 puts("SAMPLER MIXED ACTIVATION PASS:12 genuine16-reader8/16/24 master/Chip8/card8+16 paired lifetimes; copied whole-packet exact-window activation, CONTROL and STOP; caller-pin expiry, literal precision/endian/channel/padding and exact saves; independent quiet/persistent retention, partial effects and one shutdown with later independent source proof;32-reader replacement pressure preserves new keys, fragmented slot2/block0 and six unpublished full-span output guards; SOFTWARE_ONLY");
}
