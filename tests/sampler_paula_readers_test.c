#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler_paula_readers.c"
#include "../src/editor/sampler_internal.h"
#include "../src/core/playback_pcm.h"
#define VALUES 16400
struct block {void *data;size_t bytes;};
struct memory {struct block block[128];unsigned calls,live,fail;size_t bytes;void *last,*quarantine,*spare;size_t spare_bytes;};
static struct pt_paula_readers_pool *reenter_pool;
static struct pt_paula_readers_command *reenter_owner;
static unsigned reenter_calls;
static void (*allocate_hook)(void);
static void *allocate(void *context,size_t n)
{
    struct memory *m=context;void *p;unsigned i;++m->calls;
    if(allocate_hook){void (*hook)(void)=allocate_hook;allocate_hook=NULL;hook();}
    if(reenter_pool) {assert(!pt_paula_readers_close(reenter_pool));++reenter_calls;}
    if(reenter_owner) {assert(!pt_paula_readers_command_close(reenter_owner));++reenter_calls;}
    if(m->calls==m->fail)return NULL;
    if(m->spare && n==m->spare_bytes){p=m->spare;m->spare=NULL;m->spare_bytes=0;}
    else p=malloc(n);
    if(!p)return NULL;
    memset(p,0xa5,n);
    for(i=0;i<128;++i)if(!m->block[i].data)break;
    assert(i<128);m->block[i]=(struct block){p,n};++m->live;m->bytes+=n;m->last=p;return p;
}
static void release(void *context,void *p)
{
    struct memory *m=context;unsigned i;if(!p)return;
    if(reenter_pool) {assert(!pt_paula_readers_close(reenter_pool));++reenter_calls;}
    if(reenter_owner) {assert(!pt_paula_readers_command_close(reenter_owner));++reenter_calls;}
    for(i=0;i<128;++i)if(m->block[i].data==p)break;
    assert(i<128 && m->live);m->bytes-=m->block[i].bytes;
    if(m->quarantine==p) {assert(!m->spare);m->spare=p;m->spare_bytes=m->block[i].bytes;m->quarantine=NULL;}
    else free(p);
    m->block[i].data=NULL;--m->live;
}
static void *chip_allocate(void *c,size_t n) {return allocate(c,n);}
static void chip_release(void *c,void *p,size_t n)
{
 struct memory *m=c;unsigned i;
 for(i=0;i<128;++i)if(m->block[i].data==p)break;
 assert(i<128 && m->block[i].bytes==n);release(c,p);
}
static size_t allocated(struct memory *m,const void *p)
{
 unsigned i;
 for(i=0;i<128;++i)if(m->block[i].data==p)return m->block[i].bytes;
 assert(0);return 0;
}
struct image {void *object,*saved;size_t bytes;};
static struct image image(struct memory *m,void *p)
{struct image s={p,NULL,allocated(m,p)};s.saved=malloc(s.bytes);assert(s.saved);memcpy(s.saved,p,s.bytes);return s;}
static void unchanged(struct image *s)
{assert(!memcmp(s->object,s->saved,s->bytes));free(s->saved);s->saved=NULL;}
struct model_command {
 const struct pt_readers_event *borrowed;struct pt_readers_event event;
 struct pt_readers_command_receipt receipt;enum pt_readers_reply reply;unsigned fired;
 struct pt_paula_readers_command *owner;
};
struct model_reader {
 const struct pt_readers_domain *borrowed;struct pt_readers_key key;
 struct pt_readers_reader_receipt receipt;enum pt_readers_reply reply;struct pt_paula_readers_reader *owner;
};
struct backend {
 struct pt_paula_readers_pool *pool;struct model_command command[64];struct model_reader reader[16];
 unsigned commands,readers,effects,submissions;uint64_t now;struct pt_readers_key slot[4];unsigned active[4];int uncertain;
};
static struct model_command *mc(struct backend *b,uint64_t ticket)
{unsigned i;for(i=0;i<b->commands;++i)if(b->command[i].event.scheduled.ticket==ticket)return b->command+i;assert(0);return NULL;}
static struct model_reader *mr(struct backend *b,const struct pt_readers_key *key)
{unsigned i;for(i=0;i<b->readers;++i)if(same_key(&b->reader[i].key,key))return b->reader+i;assert(0);return NULL;}
static struct pt_paula_readers_reader *mapped_reader(struct backend *b,uint64_t token)
{unsigned i;for(i=0;i<b->pool->config.maximum_readers;++i)if(b->pool->readers[i]&&b->pool->readers[i]->token==token)return b->pool->readers[i];assert(0);return NULL;}
static int clock_read(void *c,uint64_t *ticks,uint32_t *frequency)
{struct backend *b=c;*ticks=b->now;*frequency=1000;return 1;}
/* Serialized host model only: these task-side callback calls are not an IRQ or
 * qualified native activation implementation. Effects require every full key. */
static int actual_condition(struct backend *b,const struct pt_readers_event *e)
{
 unsigned i;
 for(i=0;i<e->scheduled.batch.count;++i) {
   const struct pt_readers_domain *d=e->reader[i];struct pt_paula_readers_reader *r=mapped_reader(b,d->key.owner);
   if(!reader_current(r,d->key.owner,d->key.generation))return 0;
   if(e->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER&&
      (!b->active[d->key.slot]||!same_key(&b->slot[d->key.slot],&d->key)))return 0;
 }
 return 1;
}
static int submit(void *c,const struct pt_readers_event *e)
{
 struct backend *b=c;struct model_command *m;struct pt_paula_readers_command *owner=NULL;unsigned i;
 ++b->submissions;if(b->uncertain)return -1;
 if(!actual_condition(b,e))return 0;
 assert(b->commands<64);m=b->command+b->commands++;memset(m,0,sizeof(*m));m->borrowed=e;m->event=*e;
 for(i=0;i<b->pool->config.maximum_commands;++i)if(b->pool->commands[i]&&b->pool->commands[i]->token==e->command_owner)owner=b->pool->commands[i];
 assert(owner);m->owner=owner;
 m->receipt.domain=PT_READERS_COMMAND_DOMAIN;m->receipt.origin=PT_READERS_BACKEND_ACTUAL;m->receipt.queue=e->queue;
 m->receipt.session=e->session;m->receipt.generation=e->scheduled.batch.generation;m->receipt.ticket=e->scheduled.ticket;
 m->receipt.owner=e->command_owner;m->receipt.count=e->scheduled.batch.count;m->receipt.event=e;
 m->receipt.context=owner;m->receipt.context_bytes=sizeof(*owner);
 for(i=0;i<m->receipt.count;++i) {
   const struct pt_readers_domain *d=e->reader[i];m->receipt.action[i].key=d->key;
   if(e->scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER) {
     struct model_reader *r;assert(b->readers<16);r=b->reader+b->readers++;memset(r,0,sizeof(*r));
     r->borrowed=d;r->key=d->key;r->owner=mapped_reader(b,d->key.owner);
     r->receipt=(struct pt_readers_reader_receipt){PT_READERS_READER_DOMAIN,d->key,d,r->owner,sizeof(*r->owner),PT_READERS_RESERVED,PT_READERS_UNADOPTED,0,0};
   }else{m->receipt.action[i].reader=PT_READERS_ACTIVE;m->receipt.action[i].adoption=PT_READERS_ADOPTED;}
 }
 return 1;
}
static enum pt_readers_reply poll_command(void *c,uint64_t t,struct pt_readers_command_receipt *out)
{struct backend *b=c;struct model_command *m=mc(b,t);if(b->uncertain)return PT_READERS_UNCERTAIN;*out=m->receipt;
 if(m->reply==PT_READERS_COMMAND_DETACHED)m->borrowed=NULL;
 return m->reply;}
static enum pt_readers_reply cancel_command(void *c,uint64_t t,struct pt_readers_command_receipt *out)
{return poll_command(c,t,out);}
static enum pt_readers_reply poll_reader(void *c,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{struct backend *b=c;struct model_reader *r=mr(b,&d->key);if(b->uncertain)return PT_READERS_UNCERTAIN;*out=r->receipt;
 if(r->reply==PT_READERS_READER_RETIRED)r->borrowed=NULL;
 return r->reply;}
static enum pt_readers_reply cancel_reader(void *c,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{return poll_reader(c,d,out);}
static void fire(struct backend *b,uint64_t t)
{
 struct model_command *m=mc(b,t);unsigned i;
 assert(!m->fired&&b->now>=m->event.scheduled.first&&b->now<m->event.scheduled.last);
 assert(actual_condition(b,&m->event));m->fired=1;m->reply=PT_READERS_OBSERVATION;
 for(i=0;i<m->receipt.count;++i) {
   struct pt_readers_action_receipt *a=m->receipt.action+i;unsigned slot=m->event.scheduled.batch.action[i].slot;
   a->command=PT_READERS_ISSUED;a->observed=a->issued=b->now;a->adoption=PT_READERS_ADOPTED;
   if(m->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER) {
     struct model_reader *r=mr(b,&a->key);b->slot[slot]=a->key;b->active[slot]=1;
     r->receipt.state=PT_READERS_ACTIVE;r->receipt.adoption=PT_READERS_ADOPTED;r->receipt.observed=r->receipt.issued=b->now;
     r->reply=PT_READERS_OBSERVATION;
   }
   if(m->event.scheduled.batch.action[i].kind==PT_SCHEDULED_STOP){b->active[slot]=0;a->reader=PT_READERS_DRAINING;mr(b,&a->key)->receipt.state=PT_READERS_DRAINING;}
   else a->reader=PT_READERS_ACTIVE;
 }
 b->effects+=m->receipt.count;
}
static void detach(struct backend *b,uint64_t t)
{struct model_command *m=mc(b,t);unsigned i;
 for(i=0;i<m->receipt.count;++i)if(m->receipt.action[i].command==PT_READERS_WAITING){
   m->receipt.action[i].command=PT_READERS_CANCELLED_BEFORE;
   if(m->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER)m->receipt.action[i].reader=PT_READERS_NONE;
 }
 for(i=0;i<m->receipt.count;++i)if(m->receipt.action[i].command==PT_READERS_ISSUED)m->receipt.action[i].reader=mr(b,&m->receipt.action[i].key)->receipt.state;
 m->reply=PT_READERS_COMMAND_DETACHED;}
static void retired(struct backend *b,const struct pt_readers_key *key)
{struct model_reader *r=mr(b,key);if(same_key(&b->slot[key->slot],key))b->active[key->slot]=0;
 r->receipt.state=PT_READERS_RETIRED;r->reply=PT_READERS_READER_RETIRED;}
struct fixture {
 struct memory fast,chip;struct pt_allocator allocator;struct pt_document document;struct pt_sampler sampler;
 struct pt_paula_readers_pool *pool;struct pt_readers_output *queue;struct backend backend;
 union {int32_t values[VALUES];struct pt_readers_key key;struct {int32_t prefix[VALUES-2];uint64_t ticket;} member;} master;
};
static void init(struct fixture *f,unsigned bits,unsigned channels,unsigned readers,size_t chipbudget)
{
 struct pt_paula_readers_config c;struct pt_scheduled_grid grid={100,7,1000,100};struct pt_readers_backend b;unsigned i;
 memset(f,0,sizeof(*f));f->allocator=(struct pt_allocator){&f->fast,allocate,release};
 pt_document_init(&f->document,&f->allocator);assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
 for(i=0;i<VALUES;++i)f->master.values[i]=((int32_t)(i%200)-100)*(1L<<(bits-8))+(bits==8?0:(int32_t)(i&63));
 f->master.values[0]=-(1L<<(bits-1));f->master.values[1]=(1L<<(bits-1))-1;
 f->master.values[2]=bits==8?0x12:bits==16?0x1234:0x123456;f->master.values[3]=-f->master.values[2];
 f->document.project.samples[0].pcm=(struct pt_pcm){f->master.values,VALUES,8193,48000,(uint8_t)channels,(uint8_t)bits};
 f->document.project.samples[1]=f->document.project.samples[0];pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
 c=(struct pt_paula_readers_config){2,readers,1024*1024,chipbudget,7,&f->chip,chip_allocate,chip_release};
 assert(pt_paula_readers_open(&f->allocator,&f->sampler,&f->document.project,&c,&f->pool)==PT_PAULA_READERS_OK);
 f->backend.pool=f->pool;f->backend.now=100;
 b=(struct pt_readers_backend){&f->backend,sizeof(f->backend),{7,8,4},1,3,readers,clock_read,submit,poll_command,cancel_command,poll_reader,cancel_reader};
 assert(pt_readers_open(&f->allocator,&grid,1001,&b,2,readers,&f->queue)==PT_SCHEDULED_OK);
}
static void finish(struct fixture *f)
{assert(!pt_readers_commands_held(f->queue)&&!pt_readers_readers_held(f->queue));assert(pt_readers_close(f->queue));
 assert(pt_paula_readers_close(f->pool));pt_sampler_release(&f->sampler);pt_document_release(&f->document);
 assert(!f->fast.live&&!f->chip.live&&!f->fast.bytes&&!f->chip.bytes&&!f->sampler.bytes);}
static struct pt_paula_readers_command *begin(struct fixture *f,uint64_t frame,enum pt_scheduled_kind kind,unsigned track,unsigned sample,unsigned channel,const struct pt_readers_key *key)
{struct pt_paula_readers_request r;struct pt_paula_readers_command *c=NULL;memset(&r,0,sizeof(r));r.kind=kind;r.track=track;r.sample=sample;r.channel=channel;if(key)r.key=*key;
 assert(pt_paula_readers_begin(f->pool,f->queue,frame,&r,1,&c)==PT_PAULA_READERS_OK&&c);return c;}
static unsigned prepare(struct fixture *f,struct pt_paula_readers_command *c)
{
 unsigned ready=77,n=0;enum pt_paula_readers_result result;
 do {
   struct pt_paula_readers_reader *r=c->source[0].reader;uint8_t before[VALUES*sizeof(int32_t)],chipbefore[8194];
   size_t count=0,chipbytes=0;void *pcm=NULL,*chip=NULL;unsigned i,changed=0;
   if(r&&r->pin_job.value){
     count=r->pin_job.values;
     assert(count<=sizeof(before));pcm=(uint8_t *)r->pin_job.value+allocated(&f->fast,r->pin_job.value)-count;
     memcpy(before,pcm,count);
   }
   if(r&&r->chip_job.upload.cache){chip=pt_cache_data(r->chip_job.upload.cache,r->chip_job.upload.lease);chipbytes=r->chip_job.upload.bytes;assert(chipbytes<=sizeof(chipbefore));memcpy(chipbefore,chip,chipbytes);}
   result=pt_paula_readers_step(c,&ready);assert(++n<=52);
   assert(result==PT_PAULA_READERS_PENDING||result==PT_PAULA_READERS_OK);
   assert(ready==(result==PT_PAULA_READERS_OK));
   if(pcm){for(i=0;i<count;++i)changed+=before[i]!=((uint8_t *)pcm)[i];assert(changed<=4096);}
   if(chip){changed=0;for(i=0;i<chipbytes;++i)changed+=chipbefore[i]!=((uint8_t *)chip)[i];assert(changed<=256);}
 }while(result==PT_PAULA_READERS_PENDING);
 (void)f;return n;
}
static struct pt_scheduled_batch batch(struct pt_paula_readers_command *c)
{struct pt_scheduled_batch b;unsigned i;memset(&b,0,sizeof(b));b.generation=7;b.frame=c->frame;b.count=c->count;
 for(i=0;i<c->count;++i){struct command_source *s=c->source+i;struct pt_paula_readers_view v;
   if(s->request.kind==PT_SCHEDULED_TRIGGER){assert(pt_paula_readers_view(c,i,&v)==PT_PAULA_READERS_OK);b.action[i]=(struct pt_scheduled_action){PT_SCHEDULED_TRIGGER,v.slot,v.data,16,124,64};}
   else b.action[i]=(struct pt_scheduled_action){s->request.kind,(unsigned)c->pool->map[s->request.track],NULL,0,s->request.kind==PT_SCHEDULED_STOP?0:124,s->request.kind==PT_SCHEDULED_STOP?0:64};
 }
 return b;}
static uint64_t enqueue(struct pt_paula_readers_command *c)
{uint64_t t=0;struct pt_scheduled_batch b=batch(c);enum pt_scheduled_result result=pt_paula_readers_enqueue(c,&b,&t);
 if(result!=PT_SCHEDULED_OK)fprintf(stderr,"enqueue result%d frame%llu\n",result,(unsigned long long)c->frame);
 assert(result==PT_SCHEDULED_OK&&t);return t;}
static struct pt_paula_readers_reader *reader_handle(struct pt_paula_readers_command *c,unsigned i)
{struct pt_paula_readers_reader *r=NULL;assert(pt_paula_readers_reader(c,i,&r)==PT_PAULA_READERS_OK&&r);return r;}
static void observe_fire(struct fixture *f,uint64_t t)
{struct pt_readers_command_receipt out;assert(pt_readers_publish(f->queue,t)==PT_SCHEDULED_OK);
 f->backend.now=mc(&f->backend,t)->event.scheduled.first;fire(&f->backend,t);
 assert(pt_readers_poll_command(f->queue,t,&out)==PT_SCHEDULED_PENDING);}
static void detach_collect(struct fixture *f,uint64_t t)
{struct pt_readers_command_receipt out;detach(&f->backend,t);assert(pt_readers_poll_command(f->queue,t,&out)==PT_SCHEDULED_OK);}
static struct pt_readers_key active_key(struct pt_paula_readers_reader *r)
{struct pt_readers_key k;memset(&k,0,sizeof(k));assert(pt_paula_readers_reader_key(r,&k)==PT_SCHEDULED_OK);return k;}
static void retire_collect(struct fixture *f,struct pt_readers_key key)
{struct pt_readers_reader_receipt out;retired(&f->backend,&key);assert(pt_readers_poll_reader(f->queue,key.trigger,key.action,&out)==PT_SCHEDULED_OK);}
static unsigned pins(struct fixture *f)
{unsigned i,n=0;for(i=0;i<PT_CACHE_SLOTS;++i)n+=f->pool->bridge.cache.entry[i].pins;return n;}
static void persistent_controls(unsigned bits,unsigned channels)
{
 struct fixture f;struct pt_paula_readers_command *c;struct pt_paula_readers_reader *r;struct pt_readers_key key;
 struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned ns,i;void *header_image;size_t sb,cb;unsigned count;uint64_t t,old;
 int32_t before[VALUES];init(&f,bits,channels,2,16388);memcpy(before,f.master.values,sizeof(before));
 c=begin(&f,10,PT_SCHEDULED_TRIGGER,0,0,channels-1,NULL);assert(prepare(&f,c)>32);
 {struct image ci=image(&f.fast,c);unsigned *bad=(unsigned *)&c->token;assert(pt_paula_readers_step(c,bad)==PT_PAULA_READERS_INVALID);unchanged(&ci);}
 t=enqueue(c);r=reader_handle(c,0);
 {uint8_t expected[8194];struct pt_playback_format format={8,channels-1,0,1};
 assert(r->bytes==sizeof(expected)&&pt_playback_pcm_pack(&r->pcm,&format,expected,sizeof(expected))==PT_PCM_OK);
 assert(!memcmp(expected,r->data,sizeof(expected))&&expected[8193]==0);}
 observe_fire(&f,t);key=active_key(r);old=t;
 assert(!pt_paula_readers_command_close(c)&&!pt_paula_readers_reader_close(r)&&!pt_paula_readers_close(f.pool));
 detach_collect(&f,t);assert(pt_paula_readers_command_close(c));assert(pt_readers_readers_held(f.queue)==1&&pins(&f)==1);
 assert(pt_sampler_version_spans(r->pin,spans,PT_SAMPLER_VERSION_SPANS,&ns)&&ns);header_image=malloc(spans[0].bytes);assert(header_image);memcpy(header_image,spans[0].data,spans[0].bytes);
 sb=f.sampler.bytes;cb=f.chip.bytes;count=f.chip.calls;
 for(i=0;i<20;++i) {
   struct pt_readers_command_receipt stale;struct pt_readers_key again;
   c=begin(&f,20+i,PT_SCHEDULED_CONTROL,0,0,channels-1,&key);assert(prepare(&f,c)==1);
   assert(!c->source[0].reader&&!c->source[0].reader_token);t=enqueue(c);assert(pt_paula_readers_reader(c,0,(struct pt_paula_readers_reader **)&again)==PT_PAULA_READERS_INVALID);
   observe_fire(&f,t);detach_collect(&f,t);assert(pt_paula_readers_command_close(c));
   assert(pt_readers_poll_command(f.queue,old,&stale)==PT_SCHEDULED_INVALID);
   again=active_key(r);assert(same_key(&again,&key));assert(pins(&f)==1&&f.sampler.bytes==sb&&f.chip.bytes==cb&&f.chip.calls==count);
   assert(!memcmp(header_image,spans[0].data,spans[0].bytes));assert(!memcmp(before,f.master.values,sizeof(before)));
 }
 free(header_image);c=begin(&f,50,PT_SCHEDULED_STOP,0,0,channels-1,&key);assert(prepare(&f,c)==1);t=enqueue(c);observe_fire(&f,t);
 {struct pt_readers_key k=key;assert(pt_paula_readers_reader_key(r,&k)!=PT_SCHEDULED_OK&&!memcmp(&k,&key,sizeof(k)));}
 /* Reader proof first cannot release while the STOP command references it. */
 retire_collect(&f,key);assert(pins(&f)==1&&!pt_paula_readers_reader_close(r));detach_collect(&f,t);assert(pins(&f)==0&&r->released);
 assert(pt_paula_readers_command_close(c)&&pt_paula_readers_reader_close(r));assert(f.backend.effects==22);
 assert(!memcmp(before,f.master.values,sizeof(before)));finish(&f);
}
static void extra_cases(void);
int main(void)
{unsigned bits,channels;for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)persistent_controls(bits,channels);
 extra_cases();puts("SAMPLER PAULA READERS PASS: genuine master and selective Chip cache retained through 20 controls with command capacity2");return 0;}
static struct pt_paula_readers_reader *start_active(struct fixture *f,struct pt_paula_readers_command **command,struct pt_readers_key *key)
{struct pt_paula_readers_command *c=begin(f,10,PT_SCHEDULED_TRIGGER,0,0,0,NULL);struct pt_paula_readers_reader *r;uint64_t t;
 prepare(f,c);t=enqueue(c);r=reader_handle(c,0);observe_fire(f,t);*key=active_key(r);*command=c;return r;}
static void remove_active(struct fixture *f,struct pt_paula_readers_reader *r,struct pt_readers_key key)
{retire_collect(f,key);assert(pt_paula_readers_reader_close(r));}
static void retire_races(unsigned ready_first,unsigned close_first)
{
 struct fixture f;struct pt_paula_readers_command *original,*control;struct pt_paula_readers_reader *r;struct pt_readers_key key;
 struct pt_scheduled_batch b;struct image ci;uint64_t out=777;unsigned state=888,submissions;
 init(&f,24,2,2,16388);r=start_active(&f,&original,&key);
 control=begin(&f,20,PT_SCHEDULED_CONTROL,0,0,0,&key);
 if(ready_first)assert(prepare(&f,control)==1);
 b=batch(control);submissions=f.backend.submissions;
 if(close_first){detach_collect(&f,key.trigger);assert(pt_paula_readers_command_close(original));}
 retire_collect(&f,key);
 if(!close_first){assert(pins(&f)==1);detach_collect(&f,key.trigger);assert(pt_paula_readers_command_close(original));}
 assert(pins(&f)==0&&pt_paula_readers_reader_close(r));
 if(ready_first){ci=image(&f.fast,control);assert(pt_paula_readers_enqueue(control,&b,&out)==PT_SCHEDULED_STALE&&out==777);unchanged(&ci);}
 else{assert(pt_paula_readers_step(control,&state)==PT_PAULA_READERS_STALE&&state==888);}
 assert(f.backend.submissions==submissions&&!control->source[0].reader&&!control->source[0].reader_token);
 assert(pt_paula_readers_command_close(control));finish(&f);
}
static void replacement_race(void)
{
 struct fixture f;struct pt_paula_readers_command *original,*control,*replacement;struct pt_paula_readers_reader *old,*fresh;
 struct pt_readers_key key,newkey;struct pt_scheduled_batch b;struct image ci;uint64_t t,out=777;unsigned submissions;
 init(&f,16,1,2,16388);old=start_active(&f,&original,&key);detach_collect(&f,key.trigger);assert(pt_paula_readers_command_close(original));
 control=begin(&f,30,PT_SCHEDULED_CONTROL,0,0,0,&key);assert(prepare(&f,control)==1);b=batch(control);
 replacement=begin(&f,20,PT_SCHEDULED_TRIGGER,0,0,0,NULL);prepare(&f,replacement);t=enqueue(replacement);fresh=reader_handle(replacement,0);
 assert(fresh->pin==old->pin&&fresh->lease.slot==old->lease.slot&&fresh->lease.serial==old->lease.serial&&pins(&f)==2);
 observe_fire(&f,t);newkey=active_key(fresh);assert(!same_key(&key,&newkey));detach_collect(&f,t);assert(pt_paula_readers_command_close(replacement));
 /* Retiring the replacement first cannot revive the old key, even same lease. */
 remove_active(&f,fresh,newkey);assert(pins(&f)==1);ci=image(&f.fast,control);submissions=f.backend.submissions;
 assert(pt_paula_readers_enqueue(control,&b,&out)==PT_SCHEDULED_STALE&&out==777);unchanged(&ci);
 assert(f.backend.submissions==submissions);assert(pt_paula_readers_command_close(control));remove_active(&f,old,key);finish(&f);
}
static void version_race(void)
{
 struct fixture f;struct pt_pattern_history history;struct pt_pattern_command commands[4];struct pt_event_change changes[16];struct pt_paula_readers_command *original,*control;
 struct pt_paula_readers_reader *r;struct pt_readers_key key;struct pt_scheduled_batch b;struct image ci;uint64_t out=777;
 init(&f,24,1,2,16388);r=start_active(&f,&original,&key);detach_collect(&f,key.trigger);assert(pt_paula_readers_command_close(original));
 control=begin(&f,20,PT_SCHEDULED_STOP,0,0,0,&key);assert(prepare(&f,control)==1);b=batch(control);
 assert(pt_pattern_history_init(&history,&f.document.project,commands,4,changes,16)==PT_EDIT_OK);
 assert(pt_sampler_edit(&f.sampler,&f.document.project,&history,0,PT_PCM_REVERSE,0,16,0)==PT_EDIT_OK);
 assert(r->pin!=f.sampler.current[0]&&pins(&f)==1);ci=image(&f.fast,control);
 assert(pt_paula_readers_enqueue(control,&b,&out)==PT_SCHEDULED_STALE&&out==777);unchanged(&ci);
 assert(pt_paula_readers_command_close(control));remove_active(&f,r,key);pt_pattern_history_release(&history);finish(&f);
}
static void control_cancels_and_pressure(void)
{
 struct fixture f;struct pt_paula_readers_command *original,*c,*other,*refused=(void *)1;struct pt_paula_readers_reader *r;
 struct pt_readers_key key;struct pt_paula_readers_request request;unsigned kind,ready,calls;size_t sb,cb;struct image ri;
 init(&f,8,1,1,8194);r=start_active(&f,&original,&key);detach_collect(&f,key.trigger);assert(pt_paula_readers_command_close(original));
 sb=f.sampler.bytes;cb=f.chip.bytes;calls=f.chip.calls;
 for(kind=PT_SCHEDULED_CONTROL;kind<=PT_SCHEDULED_STOP;++kind) {
   c=begin(&f,20+kind,(enum pt_scheduled_kind)kind,0,0,0,&key);ri=image(&f.fast,r);
   assert(pt_paula_readers_cancel(c)==PT_PAULA_READERS_OK);unchanged(&ri);assert(pt_paula_readers_command_close(c));
   c=begin(&f,30+kind,(enum pt_scheduled_kind)kind,0,0,0,&key);assert(prepare(&f,c)==1);ri=image(&f.fast,r);
   assert(pt_paula_readers_cancel(c)==PT_PAULA_READERS_OK);unchanged(&ri);assert(pt_paula_readers_command_close(c));
   assert(pins(&f)==1&&f.sampler.bytes==sb&&f.chip.bytes==cb&&f.chip.calls==calls);
 }
 c=begin(&f,40,PT_SCHEDULED_CONTROL,0,0,0,&key);other=begin(&f,41,PT_SCHEDULED_CONTROL,0,0,0,&key);
 memset(&request,0,sizeof(request));request.kind=PT_SCHEDULED_CONTROL;request.key=key;
 calls=f.fast.calls;assert(pt_paula_readers_begin(f.pool,f.queue,42,&request,1,&refused)==PT_PAULA_READERS_CAPACITY&&refused==(void *)1&&calls==f.fast.calls);
 assert(pt_paula_readers_command_close(c)&&pt_paula_readers_command_close(other));
 request.kind=PT_SCHEDULED_TRIGGER;memset(&request.key,0,sizeof(request.key));calls=f.fast.calls;
 assert(pt_paula_readers_begin(f.pool,f.queue,42,&request,1,&refused)==PT_PAULA_READERS_CAPACITY&&refused==(void *)1&&calls==f.fast.calls);
 assert(pt_paula_readers_step(NULL,&ready)==PT_PAULA_READERS_INVALID);remove_active(&f,r,key);finish(&f);
}
static void all_phase_cancel(void)
{
 unsigned phase;
 for(phase=0;phase<=52;++phase) {
   struct fixture f;struct pt_paula_readers_command *c;unsigned i,ready=0;enum pt_paula_readers_result result=PT_PAULA_READERS_PENDING;
   int32_t before[VALUES];init(&f,24,2,2,16388);memcpy(before,f.master.values,sizeof(before));c=begin(&f,10,PT_SCHEDULED_TRIGGER,0,0,1,NULL);
   for(i=0;i<phase&&result==PT_PAULA_READERS_PENDING;++i)result=pt_paula_readers_step(c,&ready);
   assert(result==PT_PAULA_READERS_PENDING||result==PT_PAULA_READERS_OK);assert(pt_paula_readers_cancel(c)==PT_PAULA_READERS_OK);
   assert(pt_paula_readers_command_close(c));assert(!memcmp(before,f.master.values,sizeof(before))&&pins(&f)==0);finish(&f);
 }
}
static void allocation_failures(void)
{
 unsigned failure;
 for(failure=1;failure<=5;++failure) {
   struct fixture f;struct pt_paula_readers_command *c=(void *)1;struct pt_paula_readers_request request={0};enum pt_paula_readers_result result;
   unsigned ready=777,i;int32_t before[VALUES];init(&f,24,2,2,16388);memcpy(before,f.master.values,sizeof(before));
   f.fast.fail=f.fast.calls+failure;result=pt_paula_readers_begin(f.pool,f.queue,10,&request,1,&c);
   if(result==PT_PAULA_READERS_OK) {
     for(i=0;i<52;++i){result=pt_paula_readers_step(c,&ready);if(result!=PT_PAULA_READERS_PENDING)break;}
     assert(result==PT_PAULA_READERS_OK||result==PT_PAULA_READERS_CAPACITY||result==PT_PAULA_READERS_STALE);
     assert(pt_paula_readers_command_close(c));
   }else assert(result==PT_PAULA_READERS_CAPACITY&&c==(void *)1);
   assert(!memcmp(before,f.master.values,sizeof(before))&&pins(&f)==0);finish(&f);
 }
 {struct fixture f;struct pt_paula_readers_command *c;unsigned i,ready=777;enum pt_paula_readers_result result=PT_PAULA_READERS_PENDING;
 init(&f,16,1,2,8194);f.chip.fail=f.chip.calls+1;c=begin(&f,10,PT_SCHEDULED_TRIGGER,0,0,0,NULL);
 for(i=0;i<52&&result==PT_PAULA_READERS_PENDING;++i)result=pt_paula_readers_step(c,&ready);
 assert(result==PT_PAULA_READERS_CAPACITY);assert(pt_paula_readers_command_close(c));finish(&f);}
}
static void alias_guards(void)
{
 struct fixture f;struct pt_paula_readers_command *original,*control;struct pt_paula_readers_reader *r;struct pt_readers_key key,beforekey;
 struct image pi,ci,ri;struct pt_scheduled_batch b;uint64_t t=777;unsigned ready=888;int32_t before[VALUES];
 init(&f,24,2,2,16388);r=start_active(&f,&original,&key);detach_collect(&f,key.trigger);assert(pt_paula_readers_command_close(original));
 control=begin(&f,20,PT_SCHEDULED_CONTROL,0,0,0,&key);assert(prepare(&f,control)==1);b=batch(control);memcpy(before,f.master.values,sizeof(before));
 pi=image(&f.fast,f.pool);ci=image(&f.fast,control);ri=image(&f.fast,r);
 assert(pt_paula_readers_step(control,(unsigned *)&f.master.values[VALUES-1])==PT_PAULA_READERS_INVALID);
 assert(pt_paula_readers_step(control,(unsigned *)&r->token)==PT_PAULA_READERS_INVALID);
 assert(pt_paula_readers_step(control,(unsigned *)(UINTPTR_MAX-1))==PT_PAULA_READERS_INVALID);
 assert(pt_paula_readers_enqueue(control,&b,&f.master.member.ticket)==PT_SCHEDULED_INVALID);
 assert(pt_paula_readers_reader_key(r,&f.master.key)==PT_SCHEDULED_INVALID);
 assert(pt_paula_readers_reader_key(r,(struct pt_readers_key *)r->data)==PT_SCHEDULED_INVALID);
 assert(pt_paula_readers_reader_key(r,(struct pt_readers_key *)r)==PT_SCHEDULED_INVALID);
 beforekey=f.backend.slot[0];assert(pt_paula_readers_reader_key(r,f.backend.slot)==PT_SCHEDULED_INVALID);assert(!memcmp(&beforekey,f.backend.slot,sizeof(beforekey)));
 assert(pt_paula_readers_enqueue(control,&b,(uint64_t *)&b.frame)==PT_SCHEDULED_INVALID);
 assert(pt_paula_readers_reader_key(r,(struct pt_readers_key *)(UINTPTR_MAX-1))==PT_SCHEDULED_INVALID);
 unchanged(&pi);unchanged(&ci);unchanged(&ri);assert(!memcmp(before,f.master.values,sizeof(before))&&ready==888&&t==777);
 assert(pt_paula_readers_command_close(control));remove_active(&f,r,key);finish(&f);
}
static struct fixture *hook_fixture;static struct pt_readers_key hook_key;static unsigned hook_calls;
static void retirement_hook(void)
{struct pt_readers_reader_receipt out;retired(&hook_fixture->backend,&hook_key);
 assert(pt_readers_poll_reader(hook_fixture->queue,hook_key.trigger,hook_key.action,&out)==PT_SCHEDULED_OK);++hook_calls;}
static void deferred_retirement(void)
{
 struct fixture f;struct pt_paula_readers_command *original,*control=(void *)1;struct pt_paula_readers_reader *r;
 struct pt_readers_key key;struct pt_paula_readers_request request;unsigned chip_calls;size_t chip_bytes;
 init(&f,16,1,2,16388);r=start_active(&f,&original,&key);detach_collect(&f,key.trigger);assert(pt_paula_readers_command_close(original));
 memset(&request,0,sizeof(request));request.kind=PT_SCHEDULED_CONTROL;request.key=key;
 hook_fixture=&f;hook_key=key;hook_calls=0;allocate_hook=retirement_hook;chip_calls=f.chip.calls;chip_bytes=f.chip.bytes;
 assert(pt_paula_readers_begin(f.pool,f.queue,20,&request,1,&control)==PT_PAULA_READERS_INVALID&&control==(void *)1);
 assert(hook_calls==1&&r->state==RETIRED&&!r->released&&pins(&f)==1&&chip_calls==f.chip.calls&&chip_bytes==f.chip.bytes);
 assert(!pt_paula_readers_close(f.pool));assert(pt_paula_readers_reader_close(r)&&pins(&f)==0);finish(&f);hook_fixture=NULL;
}
static void address_reuse(void)
{
 struct fixture f;struct pt_paula_readers_command *original,*freshc,*refused=(void *)1;
 struct pt_paula_readers_reader *old,*fresh,*outreader=(void *)1;struct pt_readers_key key,freshkey;
 struct pt_paula_readers_request request;struct image ci,ri;uint64_t t;unsigned calls;
 init(&f,8,1,2,16388);old=start_active(&f,&original,&key);detach_collect(&f,key.trigger);
 retire_collect(&f,key);f.fast.quarantine=old;assert(pt_paula_readers_reader_close(old));
 freshc=begin(&f,20,PT_SCHEDULED_TRIGGER,0,0,0,NULL);assert(freshc->source[0].reader==old&&!f.fast.spare);
 prepare(&f,freshc);t=enqueue(freshc);fresh=reader_handle(freshc,0);assert(fresh==old&&fresh->token!=key.owner);
 observe_fire(&f,t);freshkey=active_key(fresh);ci=image(&f.fast,original);ri=image(&f.fast,fresh);
 assert(pt_paula_readers_reader(original,0,&outreader)==PT_PAULA_READERS_STALE&&outreader==(void *)1);unchanged(&ci);unchanged(&ri);
 assert(pt_paula_readers_command_close(original));memset(&request,0,sizeof(request));request.kind=PT_SCHEDULED_CONTROL;request.key=key;
 calls=f.fast.calls;assert(pt_paula_readers_begin(f.pool,f.queue,30,&request,1,&refused)==PT_PAULA_READERS_INVALID&&refused==(void *)1&&calls==f.fast.calls);
 detach_collect(&f,t);assert(pt_paula_readers_command_close(freshc));remove_active(&f,fresh,freshkey);finish(&f);
}
static void domain_failures(void);
static void reentry_cases(void);
static void forged_keys_and_metadata(void);
static void extra_cases(void)
{unsigned a,b;for(a=0;a<2;++a)for(b=0;b<2;++b)retire_races(a,b);
 replacement_race();version_race();control_cancels_and_pressure();all_phase_cancel();allocation_failures();alias_guards();deferred_retirement();address_reuse();domain_failures();reentry_cases();forged_keys_and_metadata();}
static void domain_failures(void)
{
 unsigned bad;
 for(bad=0;bad<3;++bad) {
   struct fixture f;struct pt_paula_readers_command *c;struct pt_paula_readers_reader *r;struct pt_readers_key key;
   struct pt_readers_reader_receipt out,saved;uint64_t t;enum pt_scheduled_result result;
   init(&f,24,1,2,8194);c=begin(&f,10,PT_SCHEDULED_TRIGGER,0,0,0,NULL);prepare(&f,c);t=enqueue(c);r=reader_handle(c,0);
   assert(pt_readers_publish(f.queue,t)==PT_SCHEDULED_OK);key=mr(&f.backend,&f.backend.reader[0].key)->key;
   if(bad!=0){struct pt_readers_command_receipt command;f.backend.now=mc(&f.backend,t)->event.scheduled.first;fire(&f.backend,t);assert(pt_readers_poll_command(f.queue,t,&command)==PT_SCHEDULED_PENDING);key=active_key(r);}
   detach_collect(&f,t);assert(pt_paula_readers_command_close(c));assert(pins(&f)==1&&!r->released&&!pt_paula_readers_reader_close(r));
   memset(&out,0xa5,sizeof(out));saved=out;
   if(bad==0){f.backend.uncertain=1;assert(pt_readers_poll_reader(f.queue,t,0,&out)==PT_SCHEDULED_BACKEND);f.backend.uncertain=0;assert(pins(&f)==1&&!memcmp(&out,&saved,sizeof(out)));}
   retired(&f.backend,&key);
   if(bad==1){struct model_reader *m=mr(&f.backend,&key);void *original=m->receipt.context;m->receipt.context=&f.backend;
     assert(pt_readers_poll_reader(f.queue,t,0,&out)==PT_SCHEDULED_BACKEND&&pins(&f)==1&&!memcmp(&out,&saved,sizeof(out)));m->receipt.context=original;}
   if(bad==2)mr(&f.backend,&key)->receipt.issued=mc(&f.backend,t)->event.scheduled.last;
   result=pt_readers_poll_reader(f.queue,t,0,&out);assert(result==PT_SCHEDULED_BACKEND&&!memcmp(&out,&saved,sizeof(out)));
   assert(!pins(&f)&&r->released&&r->terminal_seen&&r->terminal_valid==(bad!=2));
   assert(pt_readers_poll_reader(f.queue,t,0,&out)==PT_SCHEDULED_INVALID&&r->released);
   assert(pt_paula_readers_reader_close(r));finish(&f);
 }
}
static void reentry_cases(void)
{
 struct fixture f;struct pt_paula_readers_command *c=(void *)1;struct pt_paula_readers_request request={0};
 init(&f,16,1,2,8194);reenter_calls=0;reenter_pool=f.pool;
 assert(pt_paula_readers_begin(f.pool,f.queue,10,&request,1,&c)==PT_PAULA_READERS_INVALID&&c==(void *)1&&reenter_calls);
 reenter_pool=NULL;finish(&f);
 init(&f,16,1,2,8194);c=begin(&f,10,PT_SCHEDULED_TRIGGER,0,0,0,NULL);reenter_owner=c;reenter_calls=0;
 assert(pt_paula_readers_command_close(c)&&reenter_calls);reenter_owner=NULL;finish(&f);
 init(&f,16,1,2,8194);assert(pt_readers_close(f.queue));reenter_pool=f.pool;reenter_calls=0;
 assert(pt_paula_readers_close(f.pool)&&reenter_calls);reenter_pool=NULL;pt_sampler_release(&f.sampler);pt_document_release(&f.document);
 assert(!f.fast.live&&!f.chip.live&&!f.fast.bytes&&!f.chip.bytes);
}
static void forged_keys_and_metadata(void)
{
 struct fixture f;struct pt_paula_readers_command *original,*c,*refused=(void *)1;struct pt_paula_readers_reader *r;struct pt_readers_key key;
 struct pt_paula_readers_request request;struct pt_scheduled_batch b;struct image ri,pi;uint64_t out=777;unsigned i,calls;
 init(&f,24,2,2,16388);r=start_active(&f,&original,&key);detach_collect(&f,key.trigger);assert(pt_paula_readers_command_close(original));
 for(i=0;i<8;++i) {
   memset(&request,0,sizeof(request));request.kind=PT_SCHEDULED_CONTROL;request.key=key;
   switch(i){case 0:++request.key.serial;break;case 1:++request.key.session;break;case 2:++request.key.owner;break;
     case 3:++request.key.trigger;break;case 4:++request.key.generation;break;case 5:++request.key.action;break;
     case 6:++request.key.slot;break;default:request.key.queue=(void *)1;break;}
   calls=f.fast.calls;ri=image(&f.fast,r);pi=image(&f.fast,f.pool);
   assert(pt_paula_readers_begin(f.pool,f.queue,20,&request,1,&refused)==PT_PAULA_READERS_INVALID&&refused==(void *)1&&calls==f.fast.calls);
   unchanged(&ri);unchanged(&pi);
 }
 c=begin(&f,20,PT_SCHEDULED_CONTROL,0,0,0,&key);assert(prepare(&f,c)==1);b=batch(c);
 {struct pt_sample *samples=f.document.project.samples;f.document.project.samples=NULL;
 assert(pt_paula_readers_enqueue(c,&b,&out)==PT_SCHEDULED_STALE&&out==777);f.document.project.samples=samples;}
 assert(pt_paula_readers_command_close(c));remove_active(&f,r,key);finish(&f);
}
