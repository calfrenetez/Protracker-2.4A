#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler_paula_lineage.h"
#include "../src/editor/sampler_internal.h"
#include "../src/core/playback_pcm.h"
#define VALUES 16400
struct block {void *data;size_t bytes;};
struct memory {struct block block[128];unsigned calls,live,fail;size_t bytes;void *last,*quarantine,*spare;size_t spare_bytes;};
static struct pt_paula_lineage_pool *reenter_pool;
static struct pt_paula_lineage_owner *reenter_owner;
static unsigned reenter_calls;
static void *allocate(void *context,size_t n)
{
    struct memory *m=context;void *p;unsigned i;++m->calls;
    if(reenter_pool) {assert(!pt_paula_lineage_close(reenter_pool));++reenter_calls;}
    if(reenter_owner) {assert(!pt_paula_lineage_owner_close(reenter_owner));++reenter_calls;}
    if(m->calls==m->fail)return NULL;
    if(m->spare && n<=m->spare_bytes){p=m->spare;m->spare=NULL;m->spare_bytes=0;}
    else p=malloc(n);
    if(!p)return NULL;
    memset(p,0xa5,n);
    for(i=0;i<128;++i)if(!m->block[i].data)break;
    assert(i<128);m->block[i]=(struct block){p,n};++m->live;m->bytes+=n;m->last=p;return p;
}
static void release(void *context,void *p)
{
    struct memory *m=context;unsigned i;if(!p)return;
    if(reenter_pool) {assert(!pt_paula_lineage_close(reenter_pool));++reenter_calls;}
    if(reenter_owner) {assert(!pt_paula_lineage_owner_close(reenter_owner));++reenter_calls;}
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
struct record {struct pt_lineage_event event;struct pt_lineage_receipt receipt;int retired;};
struct backend {struct record record[8];unsigned n,effects;uint64_t now;struct pt_lineage_key slot[4];unsigned active[4];int uncertain;};
static int equal(const struct pt_lineage_key *a,const struct pt_lineage_key *b)
{return a->queue==b->queue && a->session==b->session && a->generation==b->generation && a->ticket==b->ticket &&
 a->owner==b->owner && a->serial==b->serial && a->action==b->action && a->slot==b->slot;}
static struct record *record(struct backend *b,uint64_t ticket)
{
 unsigned i;
 for(i=0;i<b->n;++i)if(b->record[i].event.scheduled.ticket==ticket)return b->record+i;
 assert(0);return NULL;
}
static int clock_read(void *c,uint64_t *ticks,uint32_t *frequency)
{struct backend *b=c;*ticks=b->now;*frequency=1000;return 1;}
static int submit(void *c,const struct pt_lineage_event *e)
{
 struct backend *b=c;struct record *r;unsigned i;assert(b->n<8);r=b->record+b->n++;memset(r,0,sizeof(*r));r->event=*e;
 r->receipt.queue=e->queue;r->receipt.session=e->session;r->receipt.generation=e->scheduled.batch.generation;
 r->receipt.ticket=e->scheduled.ticket;r->receipt.owner=e->owner;r->receipt.count=e->scheduled.batch.count;
 for(i=0;i<r->receipt.count;++i)if(e->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER) {
   r->receipt.action[i].key=e->key[i];r->receipt.action[i].reader=e->scheduled.batch.action[i].kind==PT_SCHEDULED_STOP?PT_LINEAGE_STOP_PENDING:PT_LINEAGE_ACTIVE;
 }
 return b->uncertain?-1:1;
}
static enum pt_lineage_reply poll(void *c,uint64_t ticket,struct pt_lineage_receipt *out)
{struct backend *b=c;struct record *r=record(b,ticket);if(b->uncertain)return PT_LINEAGE_UNCERTAIN;*out=r->receipt;return r->retired?PT_LINEAGE_ALL_RETIRED:PT_LINEAGE_OBSERVATION;}
static enum pt_lineage_reply cancel(void *c,uint64_t ticket,struct pt_lineage_receipt *out)
{
 struct backend *b=c;struct record *r=record(b,ticket);unsigned i;if(b->uncertain)return PT_LINEAGE_UNCERTAIN;
 for(i=0;i<r->receipt.count;++i)if(r->receipt.action[i].command==PT_LINEAGE_WAITING)r->receipt.action[i].command=PT_LINEAGE_CANCELLED_BEFORE;
 *out=r->receipt;return r->retired?PT_LINEAGE_ALL_RETIRED:PT_LINEAGE_OBSERVATION;
}
static int fire(struct backend *b,uint64_t ticket)
{
 struct record *r=record(b,ticket);unsigned i;assert(b->now>=r->event.scheduled.first && b->now<r->event.scheduled.last);
 for(i=0;i<r->receipt.count;++i)if(r->event.scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER &&
    (!b->active[r->event.key[i].slot] || !equal(r->event.key+i,b->slot+r->event.key[i].slot))) {
   unsigned j;for(j=0;j<r->receipt.count;++j){r->receipt.action[j].command=PT_LINEAGE_CANCELLED_BEFORE;
     r->receipt.action[j].reader=b->active[r->event.key[j].slot] && equal(r->event.key+j,b->slot+r->event.key[j].slot)?PT_LINEAGE_ACTIVE:PT_LINEAGE_RETIRED;}return 0;
 }
 for(i=0;i<r->receipt.count;++i) {
   struct pt_lineage_action_receipt *a=r->receipt.action+i;unsigned slot=r->event.scheduled.batch.action[i].slot;
   a->command=PT_LINEAGE_ISSUED;a->observed=a->issued=b->now;a->key=r->event.key[i];
   if(r->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER){b->slot[slot]=a->key;b->active[slot]=1;}
   if(r->event.scheduled.batch.action[i].kind==PT_SCHEDULED_STOP){b->active[slot]=0;a->reader=PT_LINEAGE_DRAINING;}
   else a->reader=PT_LINEAGE_ACTIVE;
 }
 b->effects+=r->receipt.count;return 1;
}
static void drain(struct backend *b){unsigned i;for(i=0;i<4;++i)b->active[i]=0;}
static void retire(struct backend *b,uint64_t ticket)
{
 struct record *r=record(b,ticket);unsigned i;for(i=0;i<r->receipt.count;++i){struct pt_lineage_action_receipt *a=r->receipt.action+i;
   if(a->command==PT_LINEAGE_WAITING)a->command=PT_LINEAGE_CANCELLED_BEFORE;
   if(a->reader!=PT_LINEAGE_NONE){assert(!b->active[a->key.slot]);a->reader=PT_LINEAGE_RETIRED;}}
 r->retired=1;
}
struct fixture {
    struct memory fast,chip;struct pt_allocator allocator;struct pt_document document;
    struct pt_sampler sampler;struct pt_paula_lineage_pool *pool;struct pt_lineage_output *queue;
    struct backend backend;
    union {int32_t values[VALUES];struct {int32_t prefix[VALUES-2];uint64_t ticket;} member;} master;
};
static void init(struct fixture *f,unsigned bits,unsigned channels,unsigned owners,size_t chipbudget)
{
    struct pt_paula_lineage_config c;struct pt_scheduled_grid grid={100,7,1000,100};
    struct pt_lineage_backend b;unsigned i;memset(f,0,sizeof(*f));
    f->allocator=(struct pt_allocator){&f->fast,allocate,release};
    pt_document_init(&f->document,&f->allocator);assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<VALUES;++i)f->master.values[i]=((int32_t)(i%200)-100)*(1L<<(bits-8))+(bits==8?0:(int32_t)(i&63));
    f->master.values[0]=-(1L<<(bits-1));f->master.values[1]=(1L<<(bits-1))-1;
    f->master.values[2]=bits==8?0x12:bits==16?0x1234:0x123456;
    f->master.values[3]=-f->master.values[2];
    f->master.values[4]=1L<<(bits-8?bits-9:0);f->master.values[5]=-f->master.values[4];
    f->document.project.samples[0].pcm=(struct pt_pcm){f->master.values,VALUES,8193,48000,(uint8_t)channels,(uint8_t)bits};
    f->document.project.samples[1]=f->document.project.samples[0];
    pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
    c=(struct pt_paula_lineage_config){owners,1024*1024,chipbudget,7,&f->chip,chip_allocate,chip_release};
    assert(pt_paula_lineage_open(&f->allocator,&f->sampler,&f->document.project,&c,&f->pool)==PT_PAULA_LINEAGE_OK);
    f->backend.now=100;
    b=(struct pt_lineage_backend){&f->backend,sizeof(f->backend),{7,8,4},1,1,clock_read,submit,poll,cancel};
    assert(pt_lineage_open(&f->allocator,&grid,1001,&b,owners,&f->queue)==PT_SCHEDULED_OK);
}
static void finish(struct fixture *f)
{
    assert(!pt_lineage_held(f->queue));assert(pt_lineage_close(f->queue));
    assert(pt_paula_lineage_close(f->pool));pt_sampler_release(&f->sampler);pt_document_release(&f->document);
    assert(!f->fast.live && !f->chip.live && !f->fast.bytes && !f->chip.bytes && !f->sampler.bytes);
    assert(!f->fast.spare && !f->chip.spare);
}
static struct pt_paula_lineage_owner *begin(struct fixture *f,uint64_t frame,unsigned track,unsigned sample,unsigned channel)
{
 struct pt_paula_lineage_request r;struct pt_paula_lineage_owner *o=NULL;memset(&r,0,sizeof(r));
 r.track=track;r.sample=sample;r.channel=channel;
 assert(pt_paula_lineage_begin(f->pool,f->queue,frame,&r,1,&o)==PT_PAULA_LINEAGE_OK && o);return o;
}
static unsigned prepare(struct fixture *f,struct pt_paula_lineage_owner *o)
{
 unsigned ready=77,n=0;enum pt_paula_lineage_result result;void *master=NULL;size_t masterbytes=0,headerbytes=0;
 uint8_t before[VALUES*sizeof(int32_t)];
 do {
  unsigned i,changed=0,calls=f->fast.calls;void *chip=NULL;size_t chipbytes=0;uint8_t chipbefore[8194];
  struct pt_paula_lineage_view hidden={0},saved;hidden.token=77;saved=hidden;
  assert(pt_paula_lineage_view(o,0,&hidden)==PT_PAULA_LINEAGE_INVALID && !memcmp(&hidden,&saved,sizeof(hidden)));
  for(i=0;i<128;++i)if(f->chip.block[i].data){chip=f->chip.block[i].data;chipbytes=f->chip.block[i].bytes;break;}
  if(chip){assert(chipbytes<=sizeof(chipbefore));memcpy(chipbefore,chip,chipbytes);}
  if(master)memcpy(before,(uint8_t *)master+headerbytes,masterbytes);
  result=pt_paula_lineage_step(o,&ready);assert(++n<=52);
  if(chip){for(i=0;i<chipbytes;++i)changed+=chipbefore[i]!=((uint8_t *)chip)[i];assert(changed<=256);}
  if(master){changed=0;for(i=0;i<masterbytes;++i)changed+=before[i]!=((uint8_t *)master)[headerbytes+i];assert(changed<=4096);}
  else if(f->fast.calls!=calls){assert(f->fast.calls==calls+1);master=f->fast.last;
   masterbytes=(size_t)f->document.project.samples[0].pcm.frames*f->document.project.samples[0].pcm.channels*sizeof(int32_t);
   headerbytes=allocated(&f->fast,master)-masterbytes;}
  assert(result==PT_PAULA_LINEAGE_PENDING || result==PT_PAULA_LINEAGE_OK);assert(ready==(result==PT_PAULA_LINEAGE_OK));
 }while(result==PT_PAULA_LINEAGE_PENDING);
 return n;
}
static struct pt_scheduled_batch batch(struct pt_paula_lineage_view v,uint64_t frame,enum pt_scheduled_kind kind)
{
 struct pt_scheduled_batch b;memset(&b,0,sizeof(b));b.generation=7;b.frame=frame;b.count=1;
 b.action[0]=(struct pt_scheduled_action){kind,v.slot,kind==PT_SCHEDULED_TRIGGER?v.data:NULL,
     kind==PT_SCHEDULED_TRIGGER?(uint16_t)(v.bytes/2):0,kind==PT_SCHEDULED_STOP?0:428,kind==PT_SCHEDULED_STOP?0:64};return b;
}
static struct pt_paula_lineage_owner *command(struct fixture *f,struct pt_paula_lineage_owner *original,
 struct pt_paula_lineage_view view,struct pt_lineage_key key,uint64_t frame,enum pt_scheduled_kind kind)
{
 struct pt_paula_lineage_request request={kind,view.track,view.sample,view.channel,original,view.token,0,{0}};
 struct pt_paula_lineage_owner *o=NULL;request.key=key;
 assert(pt_paula_lineage_begin(f->pool,f->queue,frame,&request,1,&o)==PT_PAULA_LINEAGE_OK && o);return o;
}
static uint64_t enqueue(struct fixture *f,struct pt_paula_lineage_owner *o,struct pt_paula_lineage_view v,uint64_t frame,enum pt_scheduled_kind kind)
{
 struct pt_scheduled_batch e=batch(v,frame,kind);uint64_t ticket=99;
 assert(pt_paula_lineage_enqueue(o,&e,&ticket)==PT_SCHEDULED_OK && ticket!=99);
 assert(pt_paula_lineage_cancel(o)==PT_PAULA_LINEAGE_BUSY && !pt_paula_lineage_owner_close(o));
 assert(pt_lineage_publish(f->queue,ticket)==PT_SCHEDULED_OK);return ticket;
}
static void collected(struct fixture *f,struct pt_paula_lineage_owner *o,uint64_t ticket)
{
 struct pt_lineage_receipt receipt;retire(&f->backend,ticket);assert(pt_lineage_poll(f->queue,ticket,&receipt)==PT_SCHEDULED_OK);
 assert(pt_paula_lineage_owner_close(o));
}
static void aliases(struct fixture *f,struct pt_paula_lineage_owner *o,struct pt_paula_lineage_view v,uint64_t frame,enum pt_scheduled_kind kind)
{
 struct image holder=image(&f->fast,o),pool=image(&f->fast,f->pool),queue=image(&f->fast,f->queue);
 struct pt_scheduled_batch e=batch(v,frame,kind),saved=e;unsigned ready=77;
 assert(pt_paula_lineage_step(o,(unsigned *)&f->master.member.ticket)==PT_PAULA_LINEAGE_INVALID);
 assert(pt_paula_lineage_step(o,(unsigned *)o)==PT_PAULA_LINEAGE_INVALID);
 assert(pt_paula_lineage_step(o,&f->sampler.generation)==PT_PAULA_LINEAGE_INVALID);
 assert(pt_paula_lineage_step(o,(unsigned *)(UINTPTR_MAX-1))==PT_PAULA_LINEAGE_INVALID);
 assert(pt_paula_lineage_step(o,(unsigned *)f->sampler.current[0])==PT_PAULA_LINEAGE_INVALID);
 assert(pt_paula_lineage_view(o,0,(struct pt_paula_lineage_view *)v.data)==PT_PAULA_LINEAGE_INVALID);
 assert(pt_paula_lineage_enqueue(o,&e,(uint64_t *)v.data)==PT_SCHEDULED_INVALID);
 assert(pt_paula_lineage_enqueue(o,&e,&f->master.member.ticket)==PT_SCHEDULED_INVALID);
 assert(pt_paula_lineage_enqueue(o,&e,(uint64_t *)o)==PT_SCHEDULED_INVALID);
 assert(pt_paula_lineage_enqueue(o,&e,(uint64_t *)(UINTPTR_MAX-1))==PT_SCHEDULED_INVALID);
 assert(ready==77 && !memcmp(&e,&saved,sizeof(e)));unchanged(&holder);unchanged(&pool);unchanged(&queue);
}
static void precision_and_commands(unsigned bits,unsigned channels)
{
 struct fixture f;struct pt_paula_lineage_owner *a,*c,*stop;struct pt_paula_lineage_view av,cv,sv;
 struct pt_lineage_key key,sentinel;struct pt_lineage_receipt receipt;struct pt_playback_format format={8,channels-1,0,1};
 uint8_t gold[8194];int32_t saved[VALUES];uint64_t ta,tc,ts;unsigned fastcalls,chipcalls;
 init(&f,bits,channels,4,8194);memcpy(saved,f.master.values,sizeof(saved));
 a=begin(&f,10,0,0,channels-1);assert(prepare(&f,a)==(channels==1?44U:52U));
 assert(pt_paula_lineage_view(a,0,&av)==PT_PAULA_LINEAGE_OK);
 assert(pt_playback_pcm_pack(&f.document.project.samples[0].pcm,&format,gold,sizeof(gold))==PT_PCM_OK && !memcmp(av.data,gold,sizeof(gold)));
 if(channels==1){const uint8_t expected[6]={128,127,18,238,1,255};assert(!memcmp(av.data,expected,6));}
 else {const uint8_t expected[3]={127,238,255};assert(!memcmp(av.data,expected,3));}
 aliases(&f,a,av,10,PT_SCHEDULED_TRIGGER);ta=enqueue(&f,a,av,10,PT_SCHEDULED_TRIGGER);
 memset(&sentinel,0x5a,sizeof(sentinel));key=sentinel;
 assert(pt_paula_lineage_reader_key(a,0,&key)==PT_SCHEDULED_STALE && !memcmp(&key,&sentinel,sizeof(key)));
 f.backend.now=record(&f.backend,ta)->event.scheduled.first;fastcalls=f.fast.calls;chipcalls=f.chip.calls;
 assert(fire(&f.backend,ta) && f.fast.calls==fastcalls && f.chip.calls==chipcalls);
 assert(pt_lineage_poll(f.queue,ta,&receipt)==PT_SCHEDULED_PENDING && pt_paula_lineage_reader_key(a,0,&key)==PT_SCHEDULED_OK);
 {struct backend before=f.backend;struct image holder=image(&f.fast,a),pool=image(&f.fast,f.pool),queue=image(&f.fast,f.queue);
  assert(pt_paula_lineage_reader_key(a,0,f.backend.slot)==PT_SCHEDULED_INVALID && !memcmp(&before,&f.backend,sizeof(before)));
  unchanged(&holder);unchanged(&pool);unchanged(&queue);}
 c=command(&f,a,av,key,20,PT_SCHEDULED_CONTROL);assert(prepare(&f,c)==2 && f.chip.calls==chipcalls);
 assert(pt_paula_lineage_view(c,0,&cv)==PT_PAULA_LINEAGE_OK && cv.data==av.data && cv.token!=av.token);
 aliases(&f,c,cv,20,PT_SCHEDULED_CONTROL);tc=enqueue(&f,c,cv,20,PT_SCHEDULED_CONTROL);
 assert(pt_paula_lineage_reader_key(c,0,&sentinel)==PT_SCHEDULED_INVALID);
 assert(pt_lineage_cancel(f.queue,tc,&receipt)==PT_SCHEDULED_PENDING && receipt.action[0].command==PT_LINEAGE_CANCELLED_BEFORE);
 assert(pt_paula_lineage_reader_key(a,0,&key)==PT_SCHEDULED_OK && f.chip.live==1);
 stop=command(&f,a,av,key,30,PT_SCHEDULED_STOP);assert(prepare(&f,stop)==2);
 assert(pt_paula_lineage_view(stop,0,&sv)==PT_PAULA_LINEAGE_OK && sv.data==av.data);ts=enqueue(&f,stop,sv,30,PT_SCHEDULED_STOP);
 assert(pt_lineage_poll(f.queue,ts,&receipt)==PT_SCHEDULED_PENDING && receipt.action[0].reader==PT_LINEAGE_STOP_PENDING);
 assert(pt_paula_lineage_reader_key(a,0,&key)==PT_SCHEDULED_STALE);
 f.backend.now=record(&f.backend,ts)->event.scheduled.first;assert(fire(&f.backend,ts));
 assert(pt_lineage_poll(f.queue,ts,&receipt)==PT_SCHEDULED_PENDING && receipt.action[0].reader==PT_LINEAGE_DRAINING);
 assert(!pt_paula_lineage_owner_close(a) && !pt_paula_lineage_owner_close(c) && !pt_paula_lineage_owner_close(stop));
 drain(&f.backend);collected(&f,stop,ts);assert(f.chip.live==1);collected(&f,a,ta);assert(f.chip.live==1);collected(&f,c,tc);
 assert(!memcmp(saved,f.master.values,sizeof(saved)));finish(&f);
}
static void origin_retirement(unsigned when)
{
 struct fixture f;struct pt_paula_lineage_owner *a,*c;struct pt_paula_lineage_view av,cv;struct pt_lineage_key key;
 struct pt_lineage_receipt receipt;uint64_t ta,ticket=99;unsigned ready=77,i;
 init(&f,24,2,3,8194);a=begin(&f,10,0,0,1);prepare(&f,a);assert(pt_paula_lineage_view(a,0,&av)==PT_PAULA_LINEAGE_OK);
 ta=enqueue(&f,a,av,10,PT_SCHEDULED_TRIGGER);f.backend.now=record(&f.backend,ta)->event.scheduled.first;assert(fire(&f.backend,ta));
 assert(pt_lineage_poll(f.queue,ta,&receipt)==PT_SCHEDULED_PENDING && pt_paula_lineage_reader_key(a,0,&key)==PT_SCHEDULED_OK);
 c=command(&f,a,av,key,20,PT_SCHEDULED_CONTROL);
 for(i=0;i<when;++i)assert(pt_paula_lineage_step(c,&ready)==(i==1?PT_PAULA_LINEAGE_OK:PT_PAULA_LINEAGE_PENDING));
 drain(&f.backend);retire(&f.backend,ta);assert(pt_lineage_poll(f.queue,ta,&receipt)==PT_SCHEDULED_OK);
 if(when<2){assert(!pt_paula_lineage_owner_close(a));ready=77;assert(pt_paula_lineage_step(c,&ready)==PT_PAULA_LINEAGE_STALE && ready==77);}
 else {
  struct image holder,pool;struct pt_scheduled_batch e;
  assert(pt_paula_lineage_owner_close(a));assert(pt_paula_lineage_view(c,0,&cv)==PT_PAULA_LINEAGE_OK);e=batch(cv,20,PT_SCHEDULED_CONTROL);
  holder=image(&f.fast,c);pool=image(&f.fast,f.pool);
  assert(pt_paula_lineage_enqueue(c,&e,&ticket)==PT_SCHEDULED_STALE && ticket==99);unchanged(&holder);unchanged(&pool);
 }
 assert(pt_paula_lineage_cancel(c)==PT_PAULA_LINEAGE_OK && pt_paula_lineage_owner_close(c));
 if(when<2)assert(pt_paula_lineage_owner_close(a));
 finish(&f);
}
static void forged_targets(void)
{
 struct fixture f;struct pt_paula_lineage_owner *a,*sentinel=(void *)1;struct pt_paula_lineage_view av;
 struct pt_lineage_key key;struct pt_lineage_receipt receipt;struct pt_paula_lineage_request r;uint64_t ta;unsigned mode;
 init(&f,16,1,3,8194);a=begin(&f,10,0,0,0);prepare(&f,a);assert(pt_paula_lineage_view(a,0,&av)==PT_PAULA_LINEAGE_OK);
 ta=enqueue(&f,a,av,10,PT_SCHEDULED_TRIGGER);f.backend.now=record(&f.backend,ta)->event.scheduled.first;assert(fire(&f.backend,ta));
 assert(pt_lineage_poll(f.queue,ta,&receipt)==PT_SCHEDULED_PENDING && pt_paula_lineage_reader_key(a,0,&key)==PT_SCHEDULED_OK);
 {struct image pool=image(&f.fast,f.pool);unsigned calls=f.fast.calls;
  assert(pt_paula_lineage_begin(f.pool,f.queue,20,(const struct pt_paula_lineage_request *)(UINTPTR_MAX-1),1,&sentinel)==PT_PAULA_LINEAGE_INVALID && sentinel==(void *)1 && f.fast.calls==calls);
  unchanged(&pool);}
 for(mode=0;mode<11;++mode) {
  struct image pool=image(&f.fast,f.pool),holder=image(&f.fast,a),queue=image(&f.fast,f.queue);unsigned calls=f.fast.calls;
  r=(struct pt_paula_lineage_request){PT_SCHEDULED_CONTROL,0,0,0,a,av.token,0,{0}};r.key=key;
  switch(mode){case 0:r.key.queue=(void *)1;break;case 1:++r.key.session;break;case 2:++r.key.generation;break;
   case 3:++r.key.ticket;break;case 4:++r.key.action;break;case 5:++r.key.owner;break;case 6:++r.key.serial;break;
   case 7:++r.key.slot;break;case 8:r.previous=(void *)1;break;case 9:++r.token;break;default:r.track=1;}
  assert(pt_paula_lineage_begin(f.pool,f.queue,20,&r,1,&sentinel)==PT_PAULA_LINEAGE_INVALID && sentinel==(void *)1 && f.fast.calls==calls);
  unchanged(&pool);unchanged(&holder);unchanged(&queue);
 }
 drain(&f.backend);collected(&f,a,ta);finish(&f);
}
static void failure_and_cancellation(void)
{
 unsigned mode;for(mode=0;mode<3;++mode){struct fixture f;struct pt_paula_lineage_owner *o,*sentinel=(void *)1;unsigned ready=77;
  struct pt_paula_lineage_request r={0};init(&f,24,2,2,8194);
  if(mode==0){f.fast.fail=f.fast.calls+1;assert(pt_paula_lineage_begin(f.pool,f.queue,10,&r,1,&sentinel)==PT_PAULA_LINEAGE_CAPACITY && sentinel==(void *)1);}
  else {
   o=begin(&f,10,0,0,1);if(mode==1)f.fast.fail=f.fast.calls+1;else f.chip.fail=f.chip.calls+1;
   {unsigned steps=0;while(pt_paula_lineage_step(o,&ready)==PT_PAULA_LINEAGE_PENDING){assert(ready==0 && ++steps<=52);}}
   assert(ready!=1);assert(pt_paula_lineage_cancel(o)==PT_PAULA_LINEAGE_OK && pt_paula_lineage_owner_close(o));
  }
  finish(&f);
 }
 for(mode=0;mode<52;++mode){struct fixture f;struct pt_paula_lineage_owner *o;unsigned i,ready=77;
  init(&f,24,2,2,8194);o=begin(&f,10,0,0,1);
  for(i=0;i<mode;++i)assert(pt_paula_lineage_step(o,&ready)==PT_PAULA_LINEAGE_PENDING);
  assert(pt_paula_lineage_cancel(o)==PT_PAULA_LINEAGE_OK && pt_paula_lineage_owner_close(o));finish(&f);
 }
}
static void finite_pressure_and_uncertainty(void)
{
 struct fixture f;struct pt_paula_lineage_owner *o[8],*sentinel=(void *)1;struct pt_paula_lineage_view v;
 struct pt_lineage_key key;struct pt_lineage_receipt receipt;uint64_t t[8];unsigned i;
 init(&f,8,1,8,8194);o[0]=begin(&f,10,0,0,0);prepare(&f,o[0]);assert(pt_paula_lineage_view(o[0],0,&v)==PT_PAULA_LINEAGE_OK);
 t[0]=enqueue(&f,o[0],v,10,PT_SCHEDULED_TRIGGER);f.backend.now=record(&f.backend,t[0])->event.scheduled.first;assert(fire(&f.backend,t[0]));
 assert(pt_lineage_poll(f.queue,t[0],&receipt)==PT_SCHEDULED_PENDING && pt_paula_lineage_reader_key(o[0],0,&key)==PT_SCHEDULED_OK);
 for(i=1;i<8;++i){struct pt_paula_lineage_view cv;o[i]=command(&f,o[0],v,key,10+i,PT_SCHEDULED_CONTROL);prepare(&f,o[i]);
  assert(pt_paula_lineage_view(o[i],0,&cv)==PT_PAULA_LINEAGE_OK);t[i]=enqueue(&f,o[i],cv,10+i,PT_SCHEDULED_CONTROL);
  assert(pt_lineage_cancel(f.queue,t[i],&receipt)==PT_SCHEDULED_PENDING && !pt_paula_lineage_owner_close(o[i]));}
 {struct pt_paula_lineage_request r={0};struct image pool=image(&f.fast,f.pool);unsigned calls=f.fast.calls;
  assert(pt_paula_lineage_begin(f.pool,f.queue,30,&r,1,&sentinel)==PT_PAULA_LINEAGE_CAPACITY && sentinel==(void *)1 && f.fast.calls==calls);unchanged(&pool);}
 f.backend.uncertain=1;assert(pt_lineage_poll(f.queue,t[0],&receipt)==PT_SCHEDULED_BACKEND && f.chip.live==1);
 f.backend.uncertain=0;drain(&f.backend);
 for(i=8;i>0;--i){retire(&f.backend,t[i-1]);assert(pt_lineage_poll(f.queue,t[i-1],&receipt)==PT_SCHEDULED_BACKEND);assert(pt_paula_lineage_owner_close(o[i-1]));}
 finish(&f);
}
static void two_targets_and_actual_replacement(void)
{
 struct fixture f;struct pt_paula_lineage_owner *a[2],*command_owner;struct pt_paula_lineage_view v[2],cv;
 struct pt_lineage_key key[2];struct pt_lineage_receipt receipt;struct pt_paula_lineage_request requests[2];
 struct pt_scheduled_batch e;uint64_t t[2],command_ticket;unsigned i,ready=77,fastcalls,chipcalls,effects;
 init(&f,24,2,3,16388);
 for(i=0;i<2;++i){a[i]=begin(&f,10+i,i,i,1);prepare(&f,a[i]);assert(pt_paula_lineage_view(a[i],0,v+i)==PT_PAULA_LINEAGE_OK);
  t[i]=enqueue(&f,a[i],v[i],10+i,PT_SCHEDULED_TRIGGER);f.backend.now=record(&f.backend,t[i])->event.scheduled.first;
  assert(fire(&f.backend,t[i]) && pt_lineage_poll(f.queue,t[i],&receipt)==PT_SCHEDULED_PENDING);
  assert(pt_paula_lineage_reader_key(a[i],0,key+i)==PT_SCHEDULED_OK);
  requests[i]=(struct pt_paula_lineage_request){i?PT_SCHEDULED_STOP:PT_SCHEDULED_CONTROL,i,i,1,a[i],v[i].token,0,{0}};
  requests[i].key=key[i];}
 assert(pt_paula_lineage_begin(f.pool,f.queue,20,requests,2,&command_owner)==PT_PAULA_LINEAGE_OK);
 for(i=0;i<4;++i)assert(pt_paula_lineage_step(command_owner,&ready)==(i==3?PT_PAULA_LINEAGE_OK:PT_PAULA_LINEAGE_PENDING));
 assert(pt_paula_lineage_view(command_owner,0,&cv)==PT_PAULA_LINEAGE_OK && cv.data==v[0].data);
 memset(&e,0,sizeof(e));e.generation=7;e.frame=20;e.count=2;
 /* Action order deliberately differs from preparation order; mapping is stable slot plus captured source. */
 e.action[0]=(struct pt_scheduled_action){PT_SCHEDULED_STOP,1,NULL,0,0,0};
 e.action[1]=(struct pt_scheduled_action){PT_SCHEDULED_CONTROL,0,NULL,0,428,32};
 assert(pt_paula_lineage_enqueue(command_owner,&e,&command_ticket)==PT_SCHEDULED_OK);
 assert(pt_lineage_publish(f.queue,command_ticket)==PT_SCHEDULED_OK);
 effects=f.backend.effects;fastcalls=f.fast.calls;chipcalls=f.chip.calls;
 /* Simulate an independently replaced exact reader after task snapshots. Same
  * PCM/cache addresses remain; only backend actual identity changed. */
 ++f.backend.slot[1].serial;f.backend.now=record(&f.backend,command_ticket)->event.scheduled.first;
 assert(!fire(&f.backend,command_ticket) && f.backend.effects==effects && f.fast.calls==fastcalls && f.chip.calls==chipcalls);
 drain(&f.backend);collected(&f,command_owner,command_ticket);
 assert(f.chip.live==2);collected(&f,a[1],t[1]);collected(&f,a[0],t[0]);finish(&f);
}
static void stale_headers_and_reentry(void)
{
 unsigned mode;for(mode=0;mode<4;++mode){struct fixture f;struct pt_paula_lineage_owner *o;struct pt_sample *samples;unsigned ready=77;
  init(&f,16,1,2,8194);o=begin(&f,10,0,0,0);
  if(mode==0){samples=f.document.project.samples;f.document.project.samples=NULL;
   assert(pt_paula_lineage_step(o,&ready)==PT_PAULA_LINEAGE_STALE && ready==77);f.document.project.samples=samples;}
  else if(mode==1){++f.sampler.generation;assert(pt_paula_lineage_step(o,&ready)==PT_PAULA_LINEAGE_STALE && ready==77);}
  else if(mode==2){f.document.project.channels.track[0].route=PT_MIDI;
   assert(pt_paula_lineage_step(o,&ready)==PT_PAULA_LINEAGE_STALE && ready==77);}
  else {reenter_owner=o;assert(pt_paula_lineage_step(o,&ready)==PT_PAULA_LINEAGE_STALE && ready==77);reenter_owner=NULL;assert(reenter_calls);}
  assert(pt_paula_lineage_cancel(o)==PT_PAULA_LINEAGE_OK && pt_paula_lineage_owner_close(o));finish(&f);
 }
}
static void concurrent_same_source_and_terminal_failure(void)
{
 struct fixture f;struct pt_paula_lineage_owner *a,*b,*sentinel=(void *)1;struct pt_paula_lineage_view av,bv;
 struct pt_scheduled_batch e;struct pt_lineage_receipt receipt,saved;struct pt_paula_lineage_request request={0};
 uint64_t ta;int32_t before[VALUES];unsigned calls;
 init(&f,24,2,3,8194);memcpy(before,f.master.values,sizeof(before));
 a=begin(&f,10,0,0,1);b=begin(&f,11,1,0,1);prepare(&f,a);assert(prepare(&f,b)==3);
 assert(pt_paula_lineage_view(a,0,&av)==PT_PAULA_LINEAGE_OK && pt_paula_lineage_view(b,0,&bv)==PT_PAULA_LINEAGE_OK && av.data==bv.data && av.token!=bv.token);
 assert(pt_paula_lineage_cancel(b)==PT_PAULA_LINEAGE_OK && pt_paula_lineage_owner_close(b));
 assert(f.chip.live==1 && !memcmp(before,f.master.values,sizeof(before)));
 e=batch(av,10,PT_SCHEDULED_TRIGGER);assert(pt_paula_lineage_enqueue(a,&e,&ta)==PT_SCHEDULED_OK);
 assert(pt_lineage_publish(f.queue,ta)==PT_SCHEDULED_OK);f.backend.now=record(&f.backend,ta)->event.scheduled.first;assert(fire(&f.backend,ta));
 assert(pt_lineage_poll(f.queue,ta,&receipt)==PT_SCHEDULED_PENDING);drain(&f.backend);retire(&f.backend,ta);
 /* Exact independently valid retirement envelope, invalid timing classification:
  * wrapper latches its own failure, yet releases this exact ticket only once. */
 record(&f.backend,ta)->receipt.action[0].issued=record(&f.backend,ta)->event.scheduled.last;
 memset(&receipt,0x5a,sizeof(receipt));saved=receipt;calls=f.fast.calls;
 assert(pt_lineage_poll(f.queue,ta,&receipt)==PT_SCHEDULED_BACKEND && !memcmp(&receipt,&saved,sizeof(receipt)));
 /* Retirement releases the last lease; the now-evictable cached allocation
  * remains until pool close. Closing the holder removes exactly its control. */
 assert(f.chip.live==1 && !pt_lineage_held(f.queue));
 {unsigned live=f.fast.live;assert(pt_paula_lineage_owner_close(a) && f.fast.live+1==live);}
 assert(pt_paula_lineage_begin(f.pool,f.queue,20,&request,1,&sentinel)==PT_PAULA_LINEAGE_STALE && sentinel==(void *)1 && f.fast.calls==calls);
 assert(pt_lineage_poll(f.queue,ta,&receipt)==PT_SCHEDULED_INVALID);
 assert(!memcmp(before,f.master.values,sizeof(before)));finish(&f);
}
int main(void)
{
 unsigned bits,channels,when;for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)precision_and_commands(bits,channels);
 for(when=0;when<3;++when)origin_retirement(when);
 forged_targets();failure_and_cancellation();finite_pressure_and_uncertainty();two_targets_and_actual_replacement();stale_headers_and_reentry();concurrent_same_source_and_terminal_failure();
 puts("SAMPLER PAULA LINEAGE PASS: independent master/cache command ownership, genuine typed original reader and per-ticket retirement; software contract only, no DMA or timing proof");return 0;
}
