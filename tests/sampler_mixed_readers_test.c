/* Reuse the genuine committed typed queued/backend/8+16 card model bodies. */
#include "mixed_scheduled_readers_test.c"
#include "../src/editor/sampler_mixed_readers.h"
struct smf_trial;
struct smf_alloc {struct smf_trial *owner;
unsigned calls,releases,hook,consumed,fail;
void *alias;
struct ledger_test {void *p;
size_t n;
} live[36];
};
struct smf_chip {struct smf_trial *owner;
unsigned calls,releases,hook,consumed;
void *alias;
struct ledger_test live[32];
};
struct smf_backend {struct smf_trial *owner;
unsigned hook,consumed;
};
struct smf_trial {
 struct trial *inherited;
struct smf_backend backend;
struct smf_alloc ordinary;
struct smf_chip chip;
 struct pt_sampler_mixed_config config;
void *workspace;
size_t capacity;
struct pt_sampler_mixed_pool *pool,*captured;
 struct pt_sample_version *pins[PT_PROJECT_SAMPLES];
struct pt_pcm pcm[PT_PROJECT_SAMPLES];
 struct pt_sampler_mixed_request request[16];
struct pt_sampler_mixed_command_handle command[2];
struct pt_sampler_mixed_reader_handle reader[32];
 unsigned count;
uint32_t revision;
uint8_t *saved;
size_t saved_bytes;
};
static void smf_backend_hook(struct smf_backend *b,unsigned kind)
{if(b->hook==kind){b->hook=0;
++b->consumed;
assert(pt_sampler_mixed_stop(b->owner->pool)==PT_SAMPLER_MIXED_BUSY);
}}
static int smf_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct smf_backend *b=context;
int result=model_clock(&b->owner->inherited->model,ticks,frequency);
smf_backend_hook(b,1);
return result;
}
static int smf_submit(void *context,const struct pt_mixed_readers_event *event)
{struct smf_backend *b=context;
int result=model_submit(&b->owner->inherited->model,event);
smf_backend_hook(b,2);
return result;
}
static enum pt_mixed_readers_reply smf_command_service(void *context,uint64_t ticket,unsigned cancel,struct pt_mixed_readers_command_receipt *out)
{struct smf_backend *b=context;
enum pt_mixed_readers_reply result=model_command_service(&b->owner->inherited->model,ticket,cancel,out);
smf_backend_hook(b,3);
return result;
}
static enum pt_mixed_readers_reply smf_reader_service(void *context,const struct pt_mixed_readers_domain *domain,unsigned cancel,struct pt_mixed_readers_reader_receipt *out)
{struct smf_backend *b=context;
enum pt_mixed_readers_reply result=model_reader_service(&b->owner->inherited->model,domain,cancel,out);
smf_backend_hook(b,4);
return result;
}
static void *smf_new(void *context,size_t bytes)
{
 struct smf_alloc *a=context;
struct smf_trial *f=a->owner;
void *p;
unsigned i;
++a->calls;
 if(a->hook){unsigned hook=a->hook;
a->hook=0;
++a->consumed;
  if(hook==1)assert(pt_sampler_mixed_open(&f->config,f->revision,f->workspace,f->capacity,&f->pool)==PT_SAMPLER_MIXED_BUSY);
  else if(hook==2)assert(pt_sampler_mixed_stop(f->pool)==PT_SAMPLER_MIXED_BUSY);
  else if(hook==3){struct pt_sampler_mixed_config low=f->config;
low.control_budget=0;
assert(pt_sampler_mixed_open(&low,f->revision,f->workspace,f->capacity,&f->pool)==PT_SAMPLER_MIXED_INVALID);
}
  else {assert(hook==4);
assert(pt_sampler_mixed_begin(f->pool,f->revision,2000,f->request,f->count,(void *)f->captured)==PT_SAMPLER_MIXED_INVALID);
}
 }
 if(a->fail)return NULL;
if(a->alias)return a->alias;
p=malloc(bytes);
assert(p);
for(i=0;i<36&&a->live[i].p;++i){}
assert(i<36);
a->live[i]=(struct ledger_test){p,bytes};
return p;
}
static void smf_free(void *context,void *p)
{
 struct smf_alloc *a=context;
unsigned i;
++a->releases;
 for(i=0;i<36&&a->live[i].p!=p;++i){}
assert(i<36);
a->live[i].p=NULL;
a->live[i].n=0;
 if(a->hook){a->hook=0;
++a->consumed;
assert(pt_sampler_mixed_stop(a->owner->captured)==PT_SAMPLER_MIXED_BUSY);
}free(p);
}
static void *smf_chip_new(void *context,size_t bytes)
{
 struct smf_chip *a=context;
unsigned i;
void *p;
++a->calls;
 if(a->hook){a->hook=0;
++a->consumed;
assert(pt_sampler_mixed_stop(a->owner->pool)==PT_SAMPLER_MIXED_BUSY);
}
 if(a->alias)return a->alias;
p=malloc(bytes);
assert(p);
for(i=0;i<32&&a->live[i].p;++i){}
assert(i<32);
a->live[i]=(struct ledger_test){p,bytes};
return p;
}
static void smf_chip_free(void *context,void *p,size_t bytes)
{struct smf_chip *a=context;
unsigned i;
++a->releases;
for(i=0;i<32&&a->live[i].p!=p;++i){}
assert(i<32&&a->live[i].n==bytes);
a->live[i].p=NULL;
a->live[i].n=0;
if(a->hook){a->hook=0;
++a->consumed;
assert(pt_sampler_mixed_stop(a->owner->pool)==PT_SAMPLER_MIXED_BUSY);
}free(p);
}
static struct smf_trial *smf_make(unsigned bits,unsigned cache_bits,unsigned stereo,unsigned frames,size_t card_budget)
{
 struct smf_trial *f=calloc(1,sizeof(*f));
struct resources *r;
unsigned i,j;
struct pt_project *p;
assert(f);
 f->inherited=trial_make(bits,cache_bits);
r=f->inherited->resources;
p=&r->document.project;
 /* The inherited bridge has no cache allocations yet; factory gets a dedicated
  * empty attached backend and independently establishes every master first. */
 if(card_budget)r->card.cache.cache.budget=card_budget;
 for(i=0;i<2;++i){int32_t scale=(int32_t)(1U<<(bits-8));
int32_t values[]={127,-128,1,-1,3,-3,64,-64,0,2,-2,126};
  for(j=0;j<64;++j)r->original[i][j]=values[j%12]*scale;
  p->samples[i].pcm.channels=(uint8_t)(stereo?2:1);
p->samples[i].pcm.frames=frames;
p->samples[i].pcm.capacity=64;
p->samples[i].pcm.bits=(uint8_t)bits;
 }
 for(i=0;i<p->sample_count;++i)assert(pt_sampler_pin(&r->sampler,p,i,r->sampler.generation,&f->pcm[i],&f->pins[i])==PT_EDIT_OK);
 f->revision=37;
f->ordinary.owner=f;
f->chip.owner=f;
f->backend.owner=f;
f->inherited->config.backend=(struct pt_mixed_readers_backend){&f->backend,sizeof(f->backend),PT_MIXED_READERS_VERSION,PT_MIXED_READERS_REQUIRED,smf_clock,smf_submit,smf_command_service,smf_reader_service};
open_trial(f->inherited);
 f->config.allocator=(struct pt_allocator){&f->ordinary,smf_new,smf_free};
f->config.sampler=&r->sampler;
f->config.project=p;
f->config.queue=f->inherited->queue;
f->config.backend=&r->card.cache;
f->config.chip_context=&f->chip;
f->config.chip_allocate=smf_chip_new;
f->config.chip_release=smf_chip_free;
 f->config.control_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
f->config.chip_budget=4096;
f->config.generation=17;
f->config.maximum_commands=2;
f->config.maximum_readers=32;
 f->config.contexts[0]=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
f->config.contexts[1]=(struct pt_mixed_readers_span){&f->chip,sizeof(f->chip)};
f->config.contexts[2]=(struct pt_mixed_readers_span){&r->card,sizeof(r->card)};
f->config.contexts[3]=(struct pt_mixed_readers_span){&f->inherited->allocator,sizeof(f->inherited->allocator)};
f->config.contexts[4]=(struct pt_mixed_readers_span){&f->inherited->model,sizeof(f->inherited->model)};
f->config.contexts[5]=(struct pt_mixed_readers_span){&f->backend,sizeof(f->backend)};
f->config.context_count=6;
 f->capacity=pt_sampler_mixed_workspace_size();
f->workspace=calloc(1,f->capacity);
assert(f->workspace);
f->saved=save(r,&f->saved_bytes);
return f;
}
static void smf_open(struct smf_trial *f)
{unsigned calls=0;
enum pt_sampler_mixed_result result;
assert(pt_sampler_mixed_open(&f->config,f->revision,f->workspace,f->capacity,&f->pool)==PT_SAMPLER_MIXED_PENDING);
f->captured=f->pool;
 do{result=pt_sampler_mixed_advance_validation(f->pool,f->revision,7);
assert(++calls<10000);
}while(result==PT_SAMPLER_MIXED_PENDING);
assert(result==PT_SAMPLER_MIXED_OK);
 assert(!f->inherited->model.reads&&!f->inherited->model.submits&&!f->inherited->model.command_calls&&!f->inherited->model.reader_calls&&!f->inherited->resources->card.writes&&!f->chip.calls);
}
static void smf_verify(struct smf_trial *f)
{size_t n;
uint8_t *after=save(f->inherited->resources,&n);
unsigned i;
assert(n==f->saved_bytes&&!memcmp(after,f->saved,n));
free(after);
 for(i=0;i<2;++i){struct pt_pcm *p=&f->inherited->resources->document.project.samples[i].pcm;
assert(p->bits==f->inherited->resources->bits);
assert(!memcmp(p->data,f->inherited->resources->original[i],(size_t)p->frames*p->channels*sizeof(int32_t)));
}}
static void smf_drop(struct smf_trial *f)
{unsigned i;
if(f->pool){assert(pt_sampler_mixed_close(&f->pool)&&!f->pool);
}
assert(pt_sampler_mixed_close(&f->pool));
smf_verify(f);
 for(i=0;i<36;++i)assert(!f->ordinary.live[i].p);
for(i=0;i<32;++i)assert(!f->chip.live[i].p);
 for(i=0;i<f->inherited->resources->document.project.sample_count;++i)pt_sampler_unpin(f->pins[i]);
free(f->saved);
free(f->workspace);
trial_drop(f->inherited);
free(f);
}
static void smf_requests(struct smf_trial *f,unsigned n,unsigned little)
{unsigned i;
memset(f->request,0,sizeof(f->request));
f->count=n;
for(i=0;i<n;++i){struct pt_sampler_mixed_request *x=f->request+i;
x->kind=PT_MIXED_READERS_TRIGGER;
x->track=i;
x->sample=i%2;
x->channel=(i>=4&&f->inherited->resources->document.project.samples[x->sample].pcm.channels==2)?1:0;
x->expected=f->pins[x->sample];
if(i<4){x->geometry.paula.period=428;
x->geometry.paula.volume=64;
}else{x->geometry.amigus.bits=f->inherited->resources->cache_bits;
x->geometry.amigus.little_endian=little;
x->geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,64,128};
}}}
static void smf_prepare(struct smf_trial *f,unsigned ci,uint64_t frame)
{enum pt_sampler_mixed_result result;
unsigned steps=0,completed=999,writes;
struct model *model=&f->inherited->model;
unsigned reads=model->reads,submits=model->submits,commands=model->command_calls,readers=model->reader_calls;
 assert(pt_sampler_mixed_begin(f->pool,f->revision,frame,f->request,f->count,f->command+ci)==PT_SAMPLER_MIXED_PENDING);
 do{writes=f->inherited->resources->card.writes;
result=pt_sampler_mixed_advance(f->pool,f->revision,f->command[ci],&completed);
assert(f->inherited->resources->card.writes-writes<=128);
assert(++steps<2000);
}while(result==PT_SAMPLER_MIXED_PENDING);
 if(result!=PT_SAMPLER_MIXED_OK)fprintf(stderr,"factory prepare result=%u steps=%u completed=%u count=%u master=%u cache=%u writes=%u\n",(unsigned)result,steps,completed,f->count,f->inherited->resources->bits,f->inherited->resources->cache_bits,f->inherited->resources->card.writes);
assert(result==PT_SAMPLER_MIXED_OK&&completed==f->count);
assert(model->reads==reads&&model->submits==submits&&model->command_calls==commands&&model->reader_calls==readers);
}
static uint8_t smf_convert8(int32_t value,unsigned bits)
{if(bits>8){unsigned shift=bits-8;
int64_t v=(int64_t)value+(1LL<<(shift-1));
v=v<0?-((-v+(1LL<<shift)-1)>>shift):v>>shift;
if(v>127)v=127;
if(v< -128)v=-128;
return (uint8_t)v;
}
return (uint8_t)value;
}
static uint16_t smf_convert16(int32_t value,unsigned bits)
{int64_t v=value;
if(bits<16)v*=1LL<<(16-bits);
else if(bits>16){unsigned shift=bits-16;
v+=1LL<<(shift-1);
v=v<0?-((-v+(1LL<<shift)-1)>>shift):v>>shift;
}
if(v>32767)v=32767;
if(v< -32768)v=-32768;
return (uint16_t)v;
}
static void smf_oracles(struct smf_trial *f,uint64_t ticket,unsigned little)
{struct model_command *c=model_command(&f->inherited->model,ticket);
unsigned i,j;
struct resources *r=f->inherited->resources;
assert(c);
 assert(c->first==oracle(c->batch.frame)&&c->last==oracle(c->batch.frame+1));
 for(i=0;i<c->batch.count;++i){const struct pt_mixed_readers_action *a=c->batch.action+i;
const struct pt_mixed_readers_domain *d=c->reference->reader[i];
unsigned sample=i%2,channel=f->request[i].channel;
struct pt_pcm *pcm=&r->document.project.samples[sample].pcm;
 if(i<4){assert(a->geometry.paula.words==((pcm->frames+1)&~1U)/2);
for(j=0;j<pcm->frames;++j)assert(a->geometry.paula.data[j]==smf_convert8(pcm->data[(size_t)j*pcm->channels+channel],r->bits));
if(pcm->frames&1)assert(!a->geometry.paula.data[pcm->frames]);
}
 else{void *resource=pt_cache_data(&r->card.cache.cache,(struct pt_cache_lease){d->card.cache_slot,d->card.serial});
unsigned arena;
assert(d->card.cache==&r->card.cache.cache&&d->card.reservation==&r->card.reservation&&d->card.version==1);
for(arena=0;arena<32&&resource!=&r->card.cache.arena.block[arena];++arena){}
assert(arena<32);
assert(d->card.full_capacity==r->card.cache.arena.block[arena].reserved);
assert(d->card.logical_bytes==pcm->frames*r->cache_bits/8);
for(j=0;j<pcm->frames;++j){int32_t value=pcm->data[(size_t)j*pcm->channels+channel];
uint32_t address=d->card.address+j*r->cache_bits/8;
if(r->cache_bits==8)assert(r->card.ram[address]==smf_convert8(value,r->bits));
else{uint16_t v=smf_convert16(value,r->bits);
assert(r->card.ram[address]==(uint8_t)(little?v:v>>8));
assert(r->card.ram[address+1]==(uint8_t)(little?v>>8:v));
}}
for(j=d->card.logical_bytes;j<d->card.full_capacity;++j)assert(!r->card.ram[d->card.address+j]);
}}
}
static void smf_handles(struct smf_trial *f,unsigned ci,unsigned base)
{unsigned i;
for(i=0;i<f->count;++i)assert(pt_sampler_mixed_reader(f->pool,f->command[ci],i,f->reader+base+i)==PT_SAMPLER_MIXED_OK);
}
static void smf_close_handles(struct smf_trial *f,unsigned ci,unsigned base,unsigned n)
{unsigned i;
assert(pt_sampler_mixed_command_close(f->pool,f->command+ci)||!f->command[ci].address);
for(i=0;i<n;++i){assert(pt_sampler_mixed_reader_close(f->pool,f->reader+base+i)||!f->reader[base+i].address);
assert(!f->reader[base+i].address);
}}
static void smf_lifetime(unsigned bits,unsigned cache_bits,unsigned little,unsigned order)
{struct smf_trial *f=smf_make(bits,cache_bits,1,cache_bits==8?6:5,0);
uint64_t ticket;
unsigned i;
struct pt_mixed_readers_key key;
smf_open(f);
smf_requests(f,16,little);
smf_prepare(f,0,960);
smf_handles(f,0,0);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
assert(pt_mixed_readers_readers_held(f->inherited->queue)==16);
assert(pt_sampler_mixed_reader_key(f->pool,f->reader[0],&key)==PT_MIXED_READERS_STALE);
 assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
smf_oracles(f,ticket,little);
fire(&f->inherited->model,ticket);
 assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,0,NULL)==PT_MIXED_READERS_PENDING);
assert(pt_sampler_mixed_reader_key(f->pool,f->reader[0],&key)==PT_MIXED_READERS_OK);
assert(key.queue==f->inherited->queue&&key.trigger==ticket&&key.route==PT_MIXED_READERS_PAULA);
 if(!order)assert(pt_sampler_mixed_service_command(f->pool,ticket,0,NULL)==PT_MIXED_READERS_OK);
 for(i=0;i<16;++i)assert(pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL)==PT_MIXED_READERS_OK);
 if(order){assert(pt_mixed_readers_readers_held(f->inherited->queue)==16);
assert(pt_sampler_mixed_service_command(f->pool,ticket,0,NULL)==PT_MIXED_READERS_OK);
}
 assert(!pt_mixed_readers_commands_held(f->inherited->queue)&&!pt_mixed_readers_readers_held(f->inherited->queue));
assert(!pt_sampler_mixed_close(&f->pool));
smf_close_handles(f,0,0,16);
smf_drop(f);
}
static void smf_cancel_phase(unsigned phase)
{struct smf_trial *f=smf_make(24,16,0,64,0);
unsigned i,writes;
smf_open(f);
smf_requests(f,5,0);
assert(pt_sampler_mixed_begin(f->pool,f->revision,960,f->request,5,f->command)==PT_SAMPLER_MIXED_PENDING);
for(i=0;i<phase;++i){enum pt_sampler_mixed_result result=pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],NULL);
assert(result==PT_SAMPLER_MIXED_PENDING||result==PT_SAMPLER_MIXED_OK);
}writes=f->inherited->resources->card.writes;
assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
assert(!f->inherited->model.reads&&!f->inherited->model.submits&&!f->inherited->model.command_calls&&!f->inherited->model.reader_calls&&f->inherited->resources->card.writes==writes);
assert(pt_sampler_mixed_command_close(f->pool,f->command));
smf_drop(f);
}
static void smf_constructor(unsigned mode)
{struct smf_trial *f=smf_make(16,16,0,6,0);
struct pt_sampler_mixed_config c=f->config;
struct pt_sampler_mixed_pool *out=(void *)(uintptr_t)1;
unsigned calls=f->ordinary.calls;
size_t capacity=f->capacity;
void *workspace=f->workspace;
struct pt_sample *samples=f->config.project->samples;
struct pt_extension *extensions=f->config.project->extensions;
unsigned extension_count=f->config.project->extension_count;
 if(mode==0)c.control_budget=pt_sampler_mixed_pool_size()-1;
else if(mode==1)capacity--;
else if(mode==2)c.chip_allocate=NULL;
else if(mode==3)c.chip_release=NULL;
else if(mode==4)c.context_count=0;
else if(mode==5)c.project->samples=NULL;
else if(mode==6){c.project->extension_count=1;
c.project->extensions=NULL;
}else if(mode==7){c.contexts[6]=(struct pt_mixed_readers_span){workspace,capacity};
c.context_count=7;
}else if(mode==8)f->ordinary.alias=f->pcm[0].data+(f->pcm[0].frames-1);
else if(mode==9)f->ordinary.alias=f->inherited->resources->card.ram;
else if(mode==10)f->ordinary.hook=1;
else if(mode==11)f->ordinary.hook=3;
else {assert(mode==12);
f->ordinary.fail=1;
}
 if(mode==11){assert(pt_sampler_mixed_open(&c,f->revision,workspace,capacity,&f->pool)==PT_SAMPLER_MIXED_PENDING);
f->captured=f->pool;
assert(f->ordinary.consumed==1&&!f->ordinary.hook);
}
 else {assert(pt_sampler_mixed_open(&c,f->revision,workspace,capacity,&out)!=PT_SAMPLER_MIXED_PENDING);
assert(out==(void *)(uintptr_t)1);
if(mode<8)assert(f->ordinary.calls==calls);
if(mode==8||mode==9)assert(!f->ordinary.releases);
if(mode==10)assert(f->ordinary.consumed==1&&f->ordinary.releases==1);
}
 f->config.project->samples=samples;
f->config.project->extensions=extensions;
f->config.project->extension_count=(uint16_t)extension_count;
f->ordinary.alias=NULL;
f->ordinary.fail=0;
smf_drop(f);
}
static void smf_admission(unsigned mode)
{struct smf_trial *f=smf_make(8,8,0,6,0);
struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,999};
struct pt_sampler_mixed_command_handle *output=&out;
struct pt_sampler_mixed_request *request=f->request;
unsigned count=2,calls;
smf_open(f);
smf_requests(f,2,0);
calls=f->ordinary.calls;
 if(mode==0)output=(void *)f->captured;
else if(mode==1)output=(void *)f->pcm[0].data;
else if(mode==2)output=(void *)&f->ordinary;
else if(mode==3)request=(void *)f->captured;
else if(mode==4)f->request[0].expected=(void *)(uintptr_t)1;
else if(mode==5)f->request[1].track=0;
else if(mode==6)f->request[0].channel=2;
else if(mode==7)f->request[0].kind=PT_MIXED_READERS_CONTROL;
else if(mode==8)count=17;
else if(mode==9)f->request[0].geometry.paula.period=0;
else if(mode==10)f->ordinary.alias=f->pcm[0].data;
else if(mode==11)f->ordinary.alias=f->captured;
else if(mode==12)f->ordinary.hook=4;
else if(mode==13)f->ordinary.hook=2;
else if(mode==14)f->ordinary.fail=1;
else {assert(mode==15);
f->config.sampler->table_bytes++;
}
 if(mode==12){assert(pt_sampler_mixed_begin(f->pool,f->revision,960,request,count,f->command)==PT_SAMPLER_MIXED_PENDING);
assert(f->ordinary.consumed==1);
assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
assert(pt_sampler_mixed_command_close(f->pool,f->command));
}
 else {assert(pt_sampler_mixed_begin(f->pool,f->revision,960,request,count,output)!=PT_SAMPLER_MIXED_PENDING);
assert(out.address==(void *)(uintptr_t)1&&out.token==999);
if(mode<10||mode==15)assert(f->ordinary.calls==calls);
if(mode==10||mode==11)assert(!f->ordinary.releases);
if(mode==13)assert(f->ordinary.consumed==1&&f->ordinary.releases==1);
}
 if(mode==15)f->config.sampler->table_bytes--;
f->ordinary.alias=NULL;
f->ordinary.fail=0;
smf_drop(f);
}
static void smf_chip_fault(unsigned mode)
{struct smf_trial *f=smf_make(24,8,0,5,0);
unsigned out=777,releases;
smf_open(f);
smf_requests(f,1,0);
assert(pt_sampler_mixed_begin(f->pool,f->revision,960,f->request,1,f->command)==PT_SAMPLER_MIXED_PENDING);
assert(pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],NULL)==PT_SAMPLER_MIXED_PENDING);
 releases=f->chip.releases;
if(mode==0)f->chip.alias=f->pcm[0].data;
else if(mode==1)f->chip.alias=f->captured;
else if(mode==2)f->chip.hook=1;
else {assert(mode==3);
f->inherited->resources->card.healthy=0;
}
 assert(pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],&out)==PT_SAMPLER_MIXED_STALE&&out==777);
if(mode<2)assert(f->chip.releases==releases);
if(mode==2)assert(f->chip.consumed==1&&f->chip.releases==releases+1);
assert(!f->inherited->model.reads&&!f->inherited->model.submits);
assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
assert(pt_sampler_mixed_command_close(f->pool,f->command)||!f->command[0].address);
f->chip.alias=NULL;
f->inherited->resources->card.healthy=1;
smf_drop(f);
}
static void smf_expired(unsigned phase)
{struct smf_trial *f=smf_make(16,16,0,64,0);
struct pt_project *p=f->config.project;
struct pt_project original=*p;
struct pt_sample *table=f->config.sampler->table;
unsigned i,out=888;
smf_open(f);
smf_requests(f,5,0);
assert(pt_sampler_mixed_begin(f->pool,f->revision,960,f->request,5,f->command)==PT_SAMPLER_MIXED_PENDING);
for(i=0;i<phase;++i){enum pt_sampler_mixed_result result=pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],NULL);
assert(result==PT_SAMPLER_MIXED_PENDING||result==PT_SAMPLER_MIXED_OK);
}
 p->samples=(void *)(uintptr_t)1;
p->events=(void *)(uintptr_t)1;
p->extensions=(void *)(uintptr_t)1;
f->config.sampler->table=(void *)(uintptr_t)1;
f->config.sampler->generation++;
 assert(pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],&out)==PT_SAMPLER_MIXED_STALE&&out==888);
assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
assert(pt_sampler_mixed_command_close(f->pool,f->command)||!f->command[0].address);
assert(pt_sampler_mixed_close(&f->pool));
assert(pt_sampler_mixed_close(&f->pool));
*p=original;
f->config.sampler->table=table;
f->config.sampler->generation--;
smf_drop(f);
}
static void smf_fragmentation(void)
{struct smf_trial *f=smf_make(8,8,0,6,12);
uint64_t ticket;
struct model_command *mc;
unsigned i;
struct pt_mixed_readers_domain const *d;
smf_open(f);
smf_requests(f,5,0);
/* two distinct6-byte derived card resources */
 f->request[0]=f->request[4];
f->request[0].track=4;
f->request[0].sample=0;
f->request[0].expected=f->pins[0];
f->request[1]=f->request[0];
f->request[1].track=5;
f->request[1].sample=1;
f->request[1].expected=f->pins[1];
f->count=2;
smf_prepare(f,0,960);
assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
assert(pt_sampler_mixed_command_close(f->pool,f->command));
 /* Refill a different representation identity; eviction leaves cache slot2 but
  * actual resource allocator reuses arena block0. */
 f->request[0].geometry.amigus.little_endian=1;
f->count=1;
smf_prepare(f,0,1920);
smf_handles(f,0,0);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
mc=model_command(&f->inherited->model,ticket);
d=mc->reference->reader[0];
assert(d->card.cache_slot==2&&d->card.logical_bytes==6&&d->card.full_capacity==8&&d->card.address==0);
assert(pt_cache_data(&f->inherited->resources->card.cache.cache,(struct pt_cache_lease){d->card.cache_slot,d->card.serial})==&f->inherited->resources->card.cache.arena.block[0]);
assert(d->card.version==1&&d->card.cache==&f->inherited->resources->card.cache.cache&&d->card.serial);
 assert(pt_sampler_mixed_service_command(f->pool,ticket,1,NULL)==PT_MIXED_READERS_OK);
for(i=0;i<1;++i)assert(pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL)==PT_MIXED_READERS_OK);
smf_close_handles(f,0,0,1);
smf_drop(f);
}
static void smf_queue_effects(unsigned mode)
{struct smf_trial *f=smf_make(24,16,1,5,0);
struct model *m=&f->inherited->model;
uint64_t ticket;
unsigned i;
struct pt_mixed_readers_command_receipt out,original;
struct pt_mixed_readers_reader_receipt ro,r_original;
struct pt_mixed_readers_key key;
 smf_open(f);
smf_requests(f,5,1);
smf_prepare(f,0,960);
smf_handles(f,0,0);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
 if(mode==0)f->backend.hook=1;
else if(mode==1||mode==2)f->backend.hook=2;
if(mode==2)m->submit_result=-1;
 if(mode<=2){assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_BACKEND);
assert(f->backend.consumed==1&&!f->backend.hook);
if(mode==0){assert(!m->submits);
assert(pt_sampler_mixed_command_close(f->pool,f->command)==0);
assert(pt_mixed_readers_stop(f->inherited->queue)==PT_MIXED_READERS_BACKEND);
assert(!pt_mixed_readers_commands_held(f->inherited->queue)&&!pt_mixed_readers_readers_held(f->inherited->queue));
smf_close_handles(f,0,0,5);
smf_drop(f);
return;
}
assert(m->submits==1);
}
 else assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
 if(mode==3)f->backend.hook=3;
else if(mode==4)f->backend.hook=4;
 if(mode==5){struct model_command *c=model_command(m,ticket);
c->issued=1;
c->fired=c->batch.count;
  assert(pt_sampler_mixed_service_command(f->pool,ticket,0,NULL)==PT_MIXED_READERS_OK);
assert(pt_mixed_readers_readers_held(f->inherited->queue)==5);
assert(pt_sampler_mixed_reader_key(f->pool,f->reader[0],&key)==PT_MIXED_READERS_STALE);
  for(i=0;i<5;++i){struct model_reader *r=model_reader(m,c->key+i);
r->adopted=1;
r->state=PT_MIXED_READER_ACTIVE;
r->observed=r->issued=c->first;
}
  assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,0,NULL)==PT_MIXED_READERS_PENDING);
assert(pt_sampler_mixed_reader_key(f->pool,f->reader[0],&key)==PT_MIXED_READERS_OK);
 }else{memset(&out,0xa5,sizeof(out));
original=out;
memset(&ro,0xa5,sizeof(ro));
r_original=ro;
  if(mode==4){assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,1,&ro)==PT_MIXED_READERS_BACKEND);
assert(!memcmp(&ro,&r_original,sizeof(ro))&&f->backend.consumed==1);
}
  if(mode==3){assert(pt_sampler_mixed_service_command(f->pool,ticket,1,&out)==PT_MIXED_READERS_BACKEND);
assert(!memcmp(&out,&original,sizeof(out))&&f->backend.consumed==1);
}
  else {enum pt_mixed_readers_result r=pt_sampler_mixed_service_command(f->pool,ticket,1,NULL);
assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_BACKEND);
}
 }
 for(i=mode==4?1:0;i<5;++i){enum pt_mixed_readers_result r=pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL);
assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_BACKEND);
}
 assert(!pt_mixed_readers_commands_held(f->inherited->queue)&&!pt_mixed_readers_readers_held(f->inherited->queue));
smf_close_handles(f,0,0,5);
smf_drop(f);
}
static void smf_live_output_aliases(void)
{struct smf_trial *f=smf_make(16,16,0,5,0);
uint64_t ticket;
unsigned i,cc,rc;
const void *aliases[7];
smf_open(f);
smf_requests(f,5,0);
smf_prepare(f,0,960);
smf_handles(f,0,0);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
 aliases[0]=f->captured;
aliases[1]=f->command[0].address;
aliases[2]=f->reader[0].address;
aliases[3]=f->pcm[0].data;
aliases[4]=f->inherited->queue;
aliases[5]=&f->ordinary;
aliases[6]=f->chip.live[0].p;
 cc=f->inherited->model.command_calls;
rc=f->inherited->model.reader_calls;
 for(i=0;i<7;++i){assert(pt_sampler_mixed_service_command(f->pool,ticket,1,(void *)aliases[i])==PT_MIXED_READERS_INVALID);
assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,1,(void *)aliases[i])==PT_MIXED_READERS_INVALID);
}
assert(cc==f->inherited->model.command_calls&&rc==f->inherited->model.reader_calls);
 assert(pt_sampler_mixed_service_command(f->pool,ticket,1,NULL)==PT_MIXED_READERS_OK);
for(i=0;i<5;++i)assert(pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL)==PT_MIXED_READERS_OK);
 /* Registered RETIRED controls still protect every output before explicit close. */
 assert(pt_sampler_mixed_reader_key(f->pool,f->reader[0],(void *)f->reader[0].address)==PT_MIXED_READERS_INVALID);
smf_close_handles(f,0,0,5);
smf_drop(f);
}
static void smf_live_expired(void)
{struct smf_trial *f=smf_make(24,16,0,5,0);
struct pt_project original=*f->config.project;
struct pt_sample *table=f->config.sampler->table;
uint64_t ticket;
unsigned i;
smf_open(f);
smf_requests(f,5,0);
smf_prepare(f,0,960);
smf_handles(f,0,0);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
 f->config.project->samples=(void *)(uintptr_t)1;
f->config.project->events=(void *)(uintptr_t)1;
f->config.project->extensions=(void *)(uintptr_t)1;
f->config.sampler->table=(void *)(uintptr_t)1;
f->config.sampler->generation++;
 {struct pt_mixed_readers_command_receipt out,copy;
memset(&out,0xa5,sizeof(out));
copy=out;
assert(pt_sampler_mixed_service_command(f->pool,ticket,1,&out)==PT_MIXED_READERS_INVALID&&!memcmp(&out,&copy,sizeof(out)));
}
 assert(pt_sampler_mixed_service_command(f->pool,ticket,1,NULL)==PT_MIXED_READERS_OK);
assert(pt_mixed_readers_readers_held(f->inherited->queue)==5);
for(i=0;i<5;++i){enum pt_mixed_readers_result r=pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL);
assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_BACKEND);
}
smf_close_handles(f,0,0,5);
assert(pt_sampler_mixed_close(&f->pool));
*f->config.project=original;
f->config.sampler->table=table;
f->config.sampler->generation--;
smf_drop(f);
}
static void smf_control_stop(unsigned stop)
{struct smf_trial *f=smf_make(16,16,0,5,0);
uint64_t first,second;
struct pt_mixed_readers_key keys[5];
unsigned i;
smf_open(f);
smf_requests(f,5,0);
smf_prepare(f,0,960);
smf_handles(f,0,0);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&first)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,first)==PT_MIXED_READERS_OK);
fire(&f->inherited->model,first);
assert(pt_sampler_mixed_service_command(f->pool,first,0,NULL)==PT_MIXED_READERS_OK);
 for(i=0;i<5;++i){assert(pt_sampler_mixed_service_reader(f->pool,first,i,0,NULL)==PT_MIXED_READERS_PENDING);
assert(pt_sampler_mixed_reader_key(f->pool,f->reader[i],keys+i)==PT_MIXED_READERS_OK);
}
 for(i=0;i<5;++i){memset(f->request+i,0,sizeof(f->request[i]));
f->request[i].kind=stop?PT_MIXED_READERS_STOP:PT_MIXED_READERS_CONTROL;
f->request[i].track=i;
f->request[i].sample=i%2;
f->request[i].key=keys[i];
if(!stop){if(i<4){f->request[i].geometry.paula.period=214;
f->request[i].geometry.paula.volume=32;
}else{f->request[i].geometry.amigus.rate=1000;
f->request[i].geometry.amigus.left=300;
f->request[i].geometry.amigus.right=400;
}}}
 assert(pt_sampler_mixed_begin(f->pool,f->revision,1920,f->request,5,f->command+1)==PT_SAMPLER_MIXED_PENDING);
for(i=0;i<5;++i)assert(pt_sampler_mixed_advance(f->pool,f->revision,f->command[1],NULL)==(i==4?PT_SAMPLER_MIXED_OK:PT_SAMPLER_MIXED_PENDING));
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[1],&second)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,second)==PT_MIXED_READERS_OK);
fire(&f->inherited->model,second);
assert(pt_sampler_mixed_service_command(f->pool,second,0,NULL)==PT_MIXED_READERS_OK);
for(i=0;i<5;++i)assert(pt_sampler_mixed_service_reader(f->pool,first,i,1,NULL)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_command_close(f->pool,f->command+1));
smf_close_handles(f,0,0,5);
smf_drop(f);
}
static void smf_budget(void)
{struct smf_trial *f=smf_make(8,8,0,6,0);
struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,33};
unsigned calls;
f->config.control_budget=pt_sampler_mixed_pool_size()+pt_sampler_mixed_command_size()+pt_sampler_mixed_reader_size()-1;
smf_open(f);
smf_requests(f,1,0);
calls=f->ordinary.calls;
assert(pt_sampler_mixed_begin(f->pool,f->revision,960,f->request,1,&out)==PT_SAMPLER_MIXED_CAPACITY&&out.address==(void *)(uintptr_t)1&&out.token==33&&f->ordinary.calls==calls);
smf_drop(f);
}
static void smf_new_master_outputs(void)
{struct smf_trial *f=smf_make(16,16,0,6,0);
struct pt_pattern_history history;
struct pt_pattern_command records[4];
struct pt_event_change changes[4];
struct pt_project header;
struct pt_sample *table;
struct pt_pcm pcm,owned;
struct pt_sample_version *pin;
struct pt_sampler_mixed_command_handle copy;
unsigned added;
smf_open(f);
smf_requests(f,1,0);
smf_prepare(f,0,960);
smf_handles(f,0,0);
 assert(pt_pattern_history_init(&history,f->config.project,records,4,changes,4)==PT_EDIT_OK);
assert(pt_sampler_edit(f->config.sampler,f->config.project,&history,0,PT_PCM_REVERSE,0,6,0)==PT_EDIT_OK);
added=f->config.project->sample_count;
owned=(struct pt_pcm){malloc(32*sizeof(int32_t)),32,6,8000,1,16};
assert(owned.data);
memset(owned.data,0,32*sizeof(int32_t));
owned.data[0]=1;
assert(pt_sampler_append_owned(f->config.sampler,f->config.project,&history,&owned,&f->config.sampler->allocator,"capacity")==PT_EDIT_OK&&!owned.data);
assert(pt_sampler_pin(f->config.sampler,f->config.project,added,f->config.sampler->generation,&pcm,&pin)==PT_EDIT_OK&&pcm.capacity==32);
header=*f->config.project;
table=f->config.sampler->table;
 /* Newly-owned PCM/version capacities are protected without following old
  * project arrays, even after both project and sampler table pointers expire. */
 f->config.project->samples=(void *)(uintptr_t)1;
f->config.project->events=(void *)(uintptr_t)1;
f->config.project->extensions=(void *)(uintptr_t)1;
f->config.sampler->table=(void *)(uintptr_t)1;
 copy=f->command[0];
assert(!pt_sampler_mixed_command_close(f->pool,(void *)pcm.data));
assert(pt_sampler_mixed_reader(f->pool,f->command[0],0,(void *)(pcm.data+pcm.capacity-1))==PT_SAMPLER_MIXED_INVALID);
assert(pt_sampler_mixed_reader(f->pool,f->command[0],0,(void *)pin)==PT_SAMPLER_MIXED_INVALID);
assert(f->command[0].address==copy.address&&f->command[0].token==copy.token);
 assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
assert(pt_sampler_mixed_reader_close(f->pool,f->reader));
assert(pt_sampler_mixed_command_close(f->pool,f->command));
assert(pt_sampler_mixed_close(&f->pool));
*f->config.project=header;
f->config.sampler->table=table;
assert(pt_pattern_undo(f->config.project,&history,-1)==PT_EDIT_OK);
assert(pt_pattern_undo(f->config.project,&history,-1)==PT_EDIT_OK);
pt_sampler_unpin(pin);
pt_pattern_history_release(&history);
smf_drop(f);
}
static void smf_retired_cache(void)
{struct smf_trial *f=smf_make(16,16,0,6,0);
uint64_t ticket;
unsigned i;
struct pt_mixed_readers_key key;
smf_open(f);
smf_requests(f,5,0);
smf_prepare(f,0,960);
smf_handles(f,0,0);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
fire(&f->inherited->model,ticket);
assert(pt_sampler_mixed_service_command(f->pool,ticket,0,NULL)==PT_MIXED_READERS_OK);
assert(!pt_cache_clear(&f->inherited->resources->card.cache.cache));
assert(f->inherited->resources->card.cache.cache.entry[0].valid==2);
assert(pt_sampler_mixed_reader_key(f->pool,f->reader[4],&key)==PT_MIXED_READERS_STALE);
assert(pt_mixed_readers_readers_held(f->inherited->queue)==5);
for(i=0;i<5;++i){enum pt_mixed_readers_result result=pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL);
assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_BACKEND);
}
smf_close_handles(f,0,0,5);
smf_drop(f);
}
static void smf_persistent_master_lifetime(unsigned bits,unsigned order)
{struct smf_trial *f=smf_make(bits,16,1,5,0);
uint64_t ticket;
unsigned i,count=f->config.project->sample_count;
struct pt_project *p=f->config.project;
smf_open(f);
smf_requests(f,5,1);
smf_prepare(f,0,960);
smf_handles(f,0,0);
 for(i=0;i<count;++i){pt_sampler_unpin(f->pins[i]);
f->pins[i]=NULL;
}
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
smf_oracles(f,ticket,1);
fire(&f->inherited->model,ticket);
smf_verify(f);
 pt_sampler_release(f->config.sampler);
assert(!f->config.sampler->current[0]&&!f->config.sampler->current[1]);
assert(f->pcm[0].data[0]==f->inherited->resources->original[0][0]);
assert(f->pcm[1].data[0]==f->inherited->resources->original[1][0]);
 if(!order){enum pt_mixed_readers_result result=pt_sampler_mixed_service_command(f->pool,ticket,0,NULL);
assert(result==PT_MIXED_READERS_BACKEND||result==PT_MIXED_READERS_OK);
assert(f->pcm[0].data[0]==f->inherited->resources->original[0][0]);
}
 for(i=0;i<5;++i){enum pt_mixed_readers_result result=pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL);
assert(result==PT_MIXED_READERS_BACKEND||result==PT_MIXED_READERS_OK);
}
if(order){assert(f->pcm[0].data[0]==f->inherited->resources->original[0][0]);
{enum pt_mixed_readers_result result=pt_sampler_mixed_service_command(f->pool,ticket,0,NULL);
assert(result==PT_MIXED_READERS_BACKEND||result==PT_MIXED_READERS_OK);
}}
 smf_close_handles(f,0,0,5);
assert(pt_sampler_mixed_close(&f->pool));
/* Source project ownership was explicitly retired; restore independent original
  * PCM before any later save oracle, never read the expired promoted capacity. */
 for(i=0;i<2;++i){p->samples[i].pcm.data=f->inherited->resources->original[i];
p->samples[i].pcm.capacity=64;
}
smf_drop(f);
}
static void smf_chip_release_reentry(void)
{struct smf_trial *f=smf_make(16,8,0,5,0);
smf_open(f);
smf_requests(f,1,0);
smf_prepare(f,0,960);
assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
assert(pt_sampler_mixed_command_close(f->pool,f->command));
f->chip.hook=1;
assert(!pt_sampler_mixed_close(&f->pool)&&!f->pool);
assert(f->chip.consumed==1&&!f->chip.hook);
smf_drop(f);
}
static void smf_replacement_pressure(void)
{struct smf_trial *f=smf_make(24,16,0,6,0);
uint64_t first,second;
unsigned i,calls;
struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,999};
smf_open(f);
smf_requests(f,16,0);
smf_prepare(f,0,960);
smf_handles(f,0,0);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&first)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,first)==PT_MIXED_READERS_OK);
fire(&f->inherited->model,first);
assert(pt_sampler_mixed_service_command(f->pool,first,0,NULL)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_command_close(f->pool,f->command));
 smf_requests(f,16,0);
smf_prepare(f,0,1920);
smf_handles(f,0,16);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&second)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,second)==PT_MIXED_READERS_OK);
fire(&f->inherited->model,second);
assert(pt_sampler_mixed_service_command(f->pool,second,0,NULL)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_command_close(f->pool,f->command));
assert(pt_mixed_readers_readers_held(f->inherited->queue)==32);
 smf_requests(f,1,0);
calls=f->ordinary.calls;
assert(pt_sampler_mixed_begin(f->pool,f->revision,2880,f->request,1,&out)==PT_SAMPLER_MIXED_CAPACITY&&out.address==(void *)(uintptr_t)1&&out.token==999&&f->ordinary.calls==calls);
 for(i=0;i<16;++i)assert(pt_sampler_mixed_service_reader(f->pool,first,i,1,NULL)==PT_MIXED_READERS_OK);
assert(pt_mixed_readers_readers_held(f->inherited->queue)==16);
for(i=0;i<16;++i)assert(pt_sampler_mixed_service_reader(f->pool,second,i,1,NULL)==PT_MIXED_READERS_OK);
for(i=0;i<32;++i)assert(pt_sampler_mixed_reader_close(f->pool,f->reader+i));
smf_drop(f);
}
static void smf_malformed_domains(void)
{struct smf_trial *f=smf_make(16,16,0,6,0);
struct model *model=&f->inherited->model;
uint64_t ticket;
unsigned i;
struct pt_mixed_readers_command_receipt receipt,original;
smf_open(f);
smf_requests(f,5,0);
smf_prepare(f,0,960);
smf_handles(f,0,0);
assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
fire(model,ticket);
 memset(&receipt,0xa5,sizeof(receipt));
original=receipt;
model->malformed=1;
assert(pt_sampler_mixed_service_command(f->pool,ticket,0,&receipt)==PT_MIXED_READERS_BACKEND&&!memcmp(&receipt,&original,sizeof(receipt)));
assert(pt_mixed_readers_commands_held(f->inherited->queue)==1&&pt_mixed_readers_readers_held(f->inherited->queue)==5);
 model->malformed=2;
assert(pt_sampler_mixed_service_command(f->pool,ticket,0,NULL)==PT_MIXED_READERS_BACKEND);
assert(!pt_mixed_readers_commands_held(f->inherited->queue)&&pt_mixed_readers_readers_held(f->inherited->queue)==5);
 model->malformed=3;
assert(pt_sampler_mixed_service_reader(f->pool,ticket,0,1,NULL)==PT_MIXED_READERS_BACKEND);
assert(pt_mixed_readers_readers_held(f->inherited->queue)==5);
model->malformed=0;
for(i=0;i<5;++i)assert(pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL)==PT_MIXED_READERS_BACKEND);
assert(!pt_mixed_readers_readers_held(f->inherited->queue));
smf_close_handles(f,0,0,5);
smf_drop(f);
}
static void smf_release_reentry(void)
{struct smf_trial *f=smf_make(8,8,0,6,0);
smf_open(f);
f->ordinary.hook=2;
assert(!pt_sampler_mixed_close(&f->pool)&&!f->pool);
assert(f->ordinary.consumed==1&&!f->ordinary.hook);
assert(pt_sampler_mixed_close(&f->pool));
smf_drop(f);
}
static void __attribute__((constructor)) smf_suite(void)
{unsigned bits,cache_bits,little,order,mode;
for(bits=8;bits<=24;bits+=8)for(cache_bits=8;cache_bits<=16;cache_bits+=8)for(little=0;little<2;++little)for(order=0;order<2;++order)smf_lifetime(bits,cache_bits,little,order);
 for(mode=0;mode<13;++mode)smf_constructor(mode);
for(mode=0;mode<16;++mode)smf_admission(mode);
for(mode=0;mode<4;++mode)smf_chip_fault(mode);
for(mode=0;mode<20;++mode){smf_cancel_phase(mode);
smf_expired(mode);
}
smf_fragmentation();
for(mode=0;mode<6;++mode)smf_queue_effects(mode);
smf_live_output_aliases();
smf_live_expired();
smf_control_stop(0);
smf_control_stop(1);
smf_budget();
smf_new_master_outputs();
smf_retired_cache();
for(bits=8;bits<=24;bits+=8)for(order=0;order<2;++order)smf_persistent_master_lifetime(bits,order);
smf_chip_release_reentry();
smf_replacement_pressure();
smf_malformed_domains();
smf_release_reentry();
 puts("SAMPLER MIXED FACTORY PASS:24 genuine16-reader paired8/16/24 master lifetimes; real private validation and persistent/TEMP pins; selective signed8 Chip and8/16 card literal endian/channel/padding oracles; exact saves/grid/independent proof orders; constructor/admission/all-phase cancel/source expiry/alias/reentry/fragmented cache-slot2 arena-block0; factory queued effects/unadopted pins/CONTROL+STOP/output guards/budget;6 released caller/current persistent-pin lifetimes;32-reader replacement/pressure and malformed exact-domain drains; SOFTWARE_ONLY");
}
