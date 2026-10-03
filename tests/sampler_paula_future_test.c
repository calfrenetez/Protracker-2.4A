#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler_paula_future.h"
#include "../src/editor/sampler_internal.h"
#include "../src/core/playback_pcm.h"
#define VALUES 16400
struct block {void *data;size_t bytes;};
struct memory {struct block block[128];unsigned calls,live,fail;size_t bytes;void *last,*quarantine,*spare;size_t spare_bytes;};
static struct pt_paula_future_pool *reenter_pool;
static struct pt_paula_future_owner *reenter_owner;
static unsigned reenter_calls;
static void *allocate(void *context,size_t n)
{
    struct memory *m=context;void *p;unsigned i;++m->calls;
    if(reenter_pool) {assert(!pt_paula_future_close(reenter_pool));++reenter_calls;}
    if(reenter_owner) {assert(!pt_paula_future_owner_close(reenter_owner));++reenter_calls;}
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
    if(reenter_pool) {assert(!pt_paula_future_close(reenter_pool));++reenter_calls;}
    if(reenter_owner) {assert(!pt_paula_future_owner_close(reenter_owner));++reenter_calls;}
    for(i=0;i<128;++i)if(m->block[i].data==p)break;
    assert(i<128 && m->live);m->bytes-=m->block[i].bytes;
    if(m->quarantine==p) {assert(!m->spare);m->spare=p;m->spare_bytes=m->block[i].bytes;m->quarantine=NULL;}
    else free(p);
    m->block[i].data=NULL;--m->live;
}
static void *chip_allocate(void *c,size_t n) {return allocate(c,n);}
static void chip_release(void *c,void *p,size_t n)
{struct memory *m=c;unsigned i;for(i=0;i<128;++i)if(m->block[i].data==p)break;assert(i<128 && m->block[i].bytes==n);release(c,p);}
static size_t allocated(struct memory *m,const void *p)
{unsigned i;for(i=0;i<128;++i)if(m->block[i].data==p)return m->block[i].bytes;assert(0);return 0;}
struct image {void *object,*saved;size_t bytes;};
static struct image image(struct memory *m,void *p)
{struct image s={p,NULL,allocated(m,p)};s.saved=malloc(s.bytes);assert(s.saved);memcpy(s.saved,p,s.bytes);return s;}
static void unchanged(struct image *s)
{assert(!memcmp(s->object,s->saved,s->bytes));free(s->saved);s->saved=NULL;}
struct backend {struct pt_scheduled_event event[8];unsigned n;uint64_t now;int submit,poll,cancel;};
static int clock_read(void *c,uint64_t *ticks,uint32_t *frequency)
{struct backend *b=c;*ticks=b->now;*frequency=1000;return 1;}
static int submit(void *c,const struct pt_scheduled_event *e)
{struct backend *b=c;assert(b->n<8);if(b->submit)b->event[b->n++]=*e;return b->submit;}
static int poll(void *c,uint64_t ticket,struct pt_scheduled_receipt *r)
{
    struct backend *b=c;unsigned i;if(b->poll!=1)return b->poll;
    for(i=0;i<b->n;++i)if(b->event[i].ticket==ticket)break;assert(i<b->n);
    r->completion=PT_SCHEDULED_CANCELLED;r->observed=r->issued=b->event[i].first;return 1;
}
static int cancel(void *c,uint64_t ticket) {(void)ticket;return ((struct backend *)c)->cancel;}
struct fixture {
    struct memory fast,chip;struct pt_allocator allocator;struct pt_document document;
    struct pt_sampler sampler;struct pt_paula_future_pool *pool;struct pt_scheduled_output *queue;
    struct backend backend;
    union {int32_t values[VALUES];struct {int32_t prefix[VALUES-2];uint64_t ticket;} member;} master;
};
static void init(struct fixture *f,unsigned bits,unsigned channels,unsigned owners,size_t chipbudget)
{
    struct pt_paula_future_config c;struct pt_scheduled_grid grid={100,7,1000,100};
    struct pt_scheduled_backend b;unsigned i;memset(f,0,sizeof(*f));
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
    c=(struct pt_paula_future_config){owners,1024*1024,chipbudget,7,&f->chip,chip_allocate,chip_release};
    assert(pt_paula_future_open(&f->allocator,&f->sampler,&f->document.project,&c,&f->pool)==PT_FUTURE_OK);
    f->backend.now=100;f->backend.submit=1;
    b=(struct pt_scheduled_backend){&f->backend,sizeof(f->backend),{7,8,4},clock_read,submit,poll,cancel};
    assert(pt_scheduled_output_open(&f->allocator,&grid,&b,owners,&f->queue)==PT_SCHEDULED_OK);
}
static void finish(struct fixture *f)
{
    assert(!pt_scheduled_output_held(f->queue));assert(pt_scheduled_output_close(f->queue));
    assert(pt_paula_future_close(f->pool));pt_sampler_release(&f->sampler);pt_document_release(&f->document);
    assert(!f->fast.live && !f->chip.live && !f->fast.bytes && !f->chip.bytes && !f->sampler.bytes);
    assert(!f->fast.spare && !f->chip.spare);
}
static struct pt_paula_future_owner *begin(struct fixture *f,uint64_t frame,unsigned track,unsigned sample,unsigned channel)
{
    struct pt_paula_future_request r={track,sample,channel,NULL,0,0};struct pt_paula_future_owner *o=NULL;
    assert(pt_paula_future_begin(f->pool,f->queue,frame,&r,1,&o)==PT_FUTURE_OK);assert(o);return o;
}
static unsigned prepare(struct fixture *f,struct pt_paula_future_owner *o)
{
    enum pt_paula_future_result r;unsigned ready=77,n=0,chipcalls=f->chip.calls;
    void *master=NULL;size_t master_bytes=0,header_bytes=0;uint8_t master_image[VALUES*sizeof(int32_t)];
    do {
        unsigned i,fastcalls=f->fast.calls;uint8_t previous[8194];void *chip=NULL;size_t bytes=0,changed=0;
        struct pt_paula_future_view hidden={0},saved;hidden.token=77;saved=hidden;
        assert(pt_paula_future_view(o,0,&hidden)==PT_FUTURE_INVALID && !memcmp(&hidden,&saved,sizeof(hidden)));
        for(i=0;i<128;++i)if(f->chip.block[i].data){chip=f->chip.block[i].data;bytes=f->chip.block[i].bytes;break;}
        if(chip){assert(bytes<=sizeof(previous));memcpy(previous,chip,bytes);}
        if(master)memcpy(master_image,(uint8_t *)master+header_bytes,master_bytes);
        r=pt_paula_future_step(o,&ready);assert(++n<=52);
        if(chip) {for(i=0;i<bytes;++i)changed+=previous[i]!=((uint8_t *)chip)[i];assert(changed<=256);}
        if(master) {
            changed=0;for(i=0;i<master_bytes;++i)changed+=master_image[i]!=((uint8_t *)master)[header_bytes+i];
            assert(changed<=PT_SAMPLER_PIN_CHUNK);
        }else if(f->fast.calls!=fastcalls) {
            assert(f->fast.calls==fastcalls+1);master=f->fast.last;
            master_bytes=(size_t)f->document.project.samples[0].pcm.frames*f->document.project.samples[0].pcm.channels*sizeof(int32_t);
            assert(master_bytes<=sizeof(master_image) && allocated(&f->fast,master)>master_bytes);
            header_bytes=allocated(&f->fast,master)-master_bytes;
        }
        assert(r==PT_FUTURE_PENDING || r==PT_FUTURE_OK);
        assert(ready==(r==PT_FUTURE_OK));
    }while(r==PT_FUTURE_PENDING);
    assert(f->chip.calls<=chipcalls+1);return n;
}
static struct pt_scheduled_batch batch(struct pt_paula_future_view v,uint64_t frame)
{
    struct pt_scheduled_batch b;memset(&b,0,sizeof(b));b.generation=7;b.frame=frame;b.count=1;
    b.action[0]=(struct pt_scheduled_action){PT_SCHEDULED_TRIGGER,v.slot,v.data,(uint16_t)(v.bytes/2),428,64};return b;
}
static void refused(struct fixture *f,struct pt_paula_future_owner *o,struct pt_scheduled_batch *b,
    enum pt_scheduled_result expected)
{
    struct image holder=image(&f->fast,o),pool=image(&f->fast,f->pool),queue=image(&f->fast,f->queue);
    struct pt_scheduled_batch saved=*b;uint64_t ticket=99;size_t fast=f->fast.bytes,chip=f->chip.bytes;
    assert(pt_paula_future_enqueue(o,b,&ticket)==expected && ticket==99);
    assert(!memcmp(b,&saved,sizeof(*b)));unchanged(&holder);unchanged(&pool);unchanged(&queue);
    assert(fast==f->fast.bytes && chip==f->chip.bytes);
}
static void precision(unsigned bits,unsigned channels)
{
    struct fixture f;struct pt_paula_future_owner *a,*b;struct pt_paula_future_view av,bv;
    struct pt_scheduled_batch event;struct pt_playback_format format={8,channels-1,0,1};uint8_t gold[8194];
    int32_t saved[VALUES];uint64_t t1,t2;unsigned calls,ready;struct image holder,pool;struct pt_sampler_storage_span spans[6];
    struct pt_pcm pcm;struct pt_sample_version *pin;unsigned count=99;
    init(&f,bits,channels,4,16388);memcpy(saved,f.master.values,sizeof(saved));
    a=begin(&f,10,0,0,channels-1);assert(!pt_paula_future_close(f.pool));
    /* Alias refusal while still unpublished protects original full padding,
     * opaque holder/control, headers and job storage, with unchanged readiness. */
    holder=image(&f.fast,a);pool=image(&f.fast,f.pool);
    assert(pt_paula_future_step(a,(unsigned *)&f.master.member.ticket)==PT_FUTURE_INVALID);
    assert(pt_paula_future_step(a,(unsigned *)a)==PT_FUTURE_INVALID);
    assert(pt_paula_future_step(a,&f.sampler.generation)==PT_FUTURE_INVALID);
    unchanged(&holder);unchanged(&pool);assert(!memcmp(saved,f.master.values,sizeof(saved)));
    assert(prepare(&f,a)==(channels==1?44U:52U));assert(pt_paula_future_view(a,0,&av)==PT_FUTURE_OK && av.bytes==8194);
    assert(pt_playback_pcm_pack(&f.document.project.samples[0].pcm,&format,gold,sizeof(gold))==PT_PCM_OK);
    assert(!memcmp(av.data,gold,sizeof(gold)));
    if(channels==1) {
        const uint8_t expected[6]={128,127,18,238,1,255};assert(!memcmp(av.data,expected,6));
    }else {const uint8_t expected[3]={127,238,255};assert(!memcmp(av.data,expected,3));}
    assert(!memcmp(saved,f.master.values,sizeof(saved)) && f.document.project.samples[0].pcm.bits==bits);
    assert(pt_sampler_pin(&f.sampler,&f.document.project,0,f.sampler.generation,&pcm,&pin)==PT_EDIT_OK);
    assert(pt_sampler_version_spans(pin,spans,6,&count) && count>=2 && count<=6);
    calls=f.fast.calls;count=99;
    assert(!pt_sampler_version_spans(pin,(struct pt_sampler_storage_span *)pin,6,&count) && count==99);
    assert(!pt_sampler_version_spans(pin,spans,1,&count) && count==99);
    assert(!pt_sampler_version_spans(pin,spans,6,&f.sampler.generation));
    assert(f.fast.calls==calls);pt_sampler_unpin(pin);
    event=batch(av,10);event.action[0].data=(uint8_t *)f.document.project.samples[0].pcm.data;refused(&f,a,&event,PT_SCHEDULED_INVALID);
    event.action[0].data=(uint8_t *)f.sampler.current[0];refused(&f,a,&event,PT_SCHEDULED_INVALID);
    event=batch(av,10);event.action[0].data=av.data+2;refused(&f,a,&event,PT_SCHEDULED_INVALID);
    event=batch(av,10);event.action[0].kind=PT_SCHEDULED_CONTROL;event.action[0].data=NULL;event.action[0].words=0;
    refused(&f,a,&event,PT_SCHEDULED_INVALID);
    event.action[0]=(struct pt_scheduled_action){PT_SCHEDULED_STOP,av.slot,NULL,0,0,0};refused(&f,a,&event,PT_SCHEDULED_INVALID);
    event=batch(av,10);event.frame=11;refused(&f,a,&event,PT_SCHEDULED_INVALID);
    event=batch(av,10);holder=image(&f.fast,a);pool=image(&f.fast,f.pool);
    assert(pt_paula_future_view(a,0,(struct pt_paula_future_view *)a)==PT_FUTURE_INVALID);
    assert(pt_paula_future_view(a,0,(struct pt_paula_future_view *)av.data)==PT_FUTURE_INVALID);
    assert(pt_paula_future_view(a,0,(struct pt_paula_future_view *)f.sampler.current[0])==PT_FUTURE_INVALID);
    assert(pt_paula_future_view(a,0,(struct pt_paula_future_view *)&f.master.member.ticket)==PT_FUTURE_INVALID);
    assert(pt_paula_future_enqueue(a,&event,(uint64_t *)(UINTPTR_MAX-1))==PT_SCHEDULED_INVALID);
    assert(pt_paula_future_enqueue(a,&event,&f.master.member.ticket)==PT_SCHEDULED_INVALID);
    assert(pt_paula_future_enqueue(a,&event,(uint64_t *)av.data)==PT_SCHEDULED_INVALID);
    assert(pt_paula_future_enqueue(a,&event,(uint64_t *)a)==PT_SCHEDULED_INVALID);
    unchanged(&holder);unchanged(&pool);assert(!memcmp(saved,f.master.values,sizeof(saved)));
    calls=f.fast.calls;assert(pt_paula_future_enqueue(a,&event,&t1)==PT_SCHEDULED_OK);
    assert(pt_paula_future_cancel(a)==PT_FUTURE_BUSY && !pt_paula_future_owner_close(a));
    assert(pt_paula_future_step(a,&ready)==PT_FUTURE_INVALID);
    assert(pt_scheduled_output_publish(f.queue,t1)==PT_SCHEDULED_OK && f.fast.calls==calls);
    b=begin(&f,11,1,0,channels-1);assert(prepare(&f,b)==3);assert(pt_paula_future_view(b,0,&bv)==PT_FUTURE_OK);
    assert(av.data==bv.data && av.token!=bv.token && f.chip.live==1);
    event=batch(bv,11);assert(pt_paula_future_enqueue(b,&event,&t2)==PT_SCHEDULED_OK);
    assert(pt_scheduled_output_publish(f.queue,t2)==PT_SCHEDULED_OK);
    f.backend.poll=1;assert(pt_scheduled_output_poll(f.queue,t2)==PT_SCHEDULED_OK);
    assert(pt_paula_future_owner_close(b));assert(!memcmp(av.data,gold,sizeof(gold)));
    f.backend.poll=-1;assert(pt_scheduled_output_poll(f.queue,t1)==PT_SCHEDULED_BACKEND);
    assert(!pt_paula_future_owner_close(a) && f.chip.live==1);
    f.backend.poll=1;assert(pt_scheduled_output_poll(f.queue,t1)==PT_SCHEDULED_OK);
    assert(pt_paula_future_owner_close(a));finish(&f);
}
static void retrigger(unsigned when)
{
    struct fixture f;struct pt_paula_future_owner *a,*b,*sentinel=(void *)1;struct pt_paula_future_request request;
    struct pt_paula_future_view av,bv;struct pt_scheduled_batch e;uint64_t t1,t2;unsigned ready,calls;
    init(&f,24,2,3,8194);a=begin(&f,10,0,0,1);prepare(&f,a);assert(pt_paula_future_view(a,0,&av)==PT_FUTURE_OK);
    e=batch(av,10);assert(pt_paula_future_enqueue(a,&e,&t1)==PT_SCHEDULED_OK);
    assert(pt_scheduled_output_publish(f.queue,t1)==PT_SCHEDULED_OK);
    request=(struct pt_paula_future_request){0,0,1,a,av.token,0};
    assert(pt_paula_future_begin(f.pool,f.queue,10,&request,1,&sentinel)==PT_FUTURE_INVALID && sentinel==(void *)1);
    ++request.token;assert(pt_paula_future_begin(f.pool,f.queue,11,&request,1,&sentinel)==PT_FUTURE_INVALID);--request.token;
    request.track=1;assert(pt_paula_future_begin(f.pool,f.queue,11,&request,1,&sentinel)==PT_FUTURE_INVALID);request.track=0;
    assert(pt_paula_future_begin(f.pool,(struct pt_scheduled_output *)f.pool,11,&request,1,&sentinel)==PT_FUTURE_INVALID);
    assert(pt_paula_future_begin(f.pool,f.queue,11,&request,1,&b)==PT_FUTURE_OK);
    if(when==0) {
        f.backend.poll=1;assert(pt_scheduled_output_poll(f.queue,t1)==PT_SCHEDULED_OK);
        assert(!pt_paula_future_owner_close(a)); /* metadata/resources still borrowed */
    }
    calls=f.chip.calls;assert(pt_paula_future_step(b,&ready)==PT_FUTURE_PENDING && !ready);
    if(when==1) {
        f.backend.poll=1;assert(pt_scheduled_output_poll(f.queue,t1)==PT_SCHEDULED_OK);
        assert(!pt_paula_future_owner_close(a));
    }
    assert(pt_paula_future_step(b,&ready)==PT_FUTURE_OK && ready && f.chip.calls==calls);
    assert(pt_paula_future_view(b,0,&bv)==PT_FUTURE_OK && bv.data==av.data && bv.token!=av.token);
    if(when==2) {f.backend.poll=1;assert(pt_scheduled_output_poll(f.queue,t1)==PT_SCHEDULED_OK);}
    assert(pt_paula_future_owner_close(a)); /* ready successor has independent refs */
    e=batch(bv,11);assert(pt_paula_future_enqueue(b,&e,&t2)==PT_SCHEDULED_OK);
    assert(pt_scheduled_output_publish(f.queue,t2)==PT_SCHEDULED_OK);
    f.backend.poll=1;assert(pt_scheduled_output_poll(f.queue,t2)==PT_SCHEDULED_OK);
    assert(pt_paula_future_owner_close(b));finish(&f);
}
static void retrigger_cancel(void)
{
    struct fixture f;struct pt_paula_future_owner *a,*b,*sentinel=(void *)1;struct pt_paula_future_request request;
    struct pt_paula_future_view view;struct pt_scheduled_batch e;uint64_t ticket;unsigned ready=99;
    init(&f,24,2,3,8194);a=begin(&f,10,0,0,1);prepare(&f,a);assert(pt_paula_future_view(a,0,&view)==PT_FUTURE_OK);
    e=batch(view,10);assert(pt_paula_future_enqueue(a,&e,&ticket)==PT_SCHEDULED_OK);
    assert(pt_scheduled_output_publish(f.queue,ticket)==PT_SCHEDULED_OK);
    request=(struct pt_paula_future_request){0,0,1,a,view.token,0};
    ++f.sampler.generation;assert(pt_paula_future_begin(f.pool,f.queue,11,&request,1,&sentinel)==PT_FUTURE_STALE && sentinel==(void *)1);
    --f.sampler.generation;assert(pt_paula_future_begin(f.pool,f.queue,11,&request,1,&b)==PT_FUTURE_OK);
    assert(pt_paula_future_step(b,&ready)==PT_FUTURE_PENDING && !ready);
    f.backend.poll=1;assert(pt_scheduled_output_poll(f.queue,ticket)==PT_SCHEDULED_OK);
    assert(!pt_paula_future_owner_close(a));assert(pt_paula_future_cancel(b)==PT_FUTURE_OK);
    assert(pt_paula_future_owner_close(a) && pt_paula_future_owner_close(b));finish(&f);
}
static void undo_stale(void)
{
    struct fixture f;struct pt_paula_future_owner *o;struct pt_paula_future_view view;struct pt_scheduled_batch e;
    struct pt_pattern_history h;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    init(&f,24,1,2,8194);assert(pt_pattern_history_init(&h,&f.document.project,commands,4,changes,4)==PT_EDIT_OK);
    o=begin(&f,10,0,0,0);prepare(&f,o);assert(pt_paula_future_view(o,0,&view)==PT_FUTURE_OK);e=batch(view,10);
    assert(pt_sampler_edit(&f.sampler,&f.document.project,&h,0,PT_PCM_REVERSE,0,8193,0)==PT_EDIT_OK);
    refused(&f,o,&e,PT_SCHEDULED_STALE);assert(pt_pattern_undo(&f.document.project,&h,-1)==PT_EDIT_OK);
    refused(&f,o,&e,PT_SCHEDULED_STALE); /* undo also changes generation, never resurrects session */
    assert(pt_paula_future_owner_close(o));pt_pattern_history_release(&h);finish(&f);
}
static void span_cases(struct pt_sampler *sampler,struct pt_project *p,unsigned slot,struct memory *m)
{
    struct pt_sample_version *pin;struct pt_pcm pcm;struct pt_sampler_storage_span spans[6],saved[6];
    unsigned n,i,count;union {struct pt_sampler_storage_span spans[6];unsigned count;} both;
    assert(pt_sampler_pin(sampler,p,slot,sampler->generation,&pcm,&pin)==PT_EDIT_OK);
    assert(pt_sampler_version_spans(pin,spans,6,&n) && n>=2 && n<=6);
    for(i=0;i<n;++i) {
        unsigned before=99;size_t bytes=spans[i].bytes;void *image=malloc(bytes);assert(image);
        memcpy(image,spans[i].data,bytes);memset(saved,0x5a,sizeof(saved));
        assert(!pt_sampler_version_spans(pin,(struct pt_sampler_storage_span *)spans[i].data,6,&before) && before==99);
        assert(!memcmp(spans[i].data,image,bytes));
        count=99;assert(!pt_sampler_version_spans(pin,saved,6,(unsigned *)spans[i].data));
        assert(!memcmp(spans[i].data,image,bytes));free(image);
        assert(!pt_sampler_version_spans(pin,saved,6,(unsigned *)((uint8_t *)spans[i].data+bytes-1)));
    }
    memset(&both,0xa5,sizeof(both));memcpy(saved,both.spans,sizeof(saved));
    assert(!pt_sampler_version_spans(pin,both.spans,6,&both.count) && !memcmp(saved,both.spans,sizeof(saved)));
    count=99;assert(!pt_sampler_version_spans(pin,(struct pt_sampler_storage_span *)(UINTPTR_MAX-7),6,&count) && count==99);
    assert(!pt_sampler_version_spans(pin,saved,6,(unsigned *)(UINTPTR_MAX-1)));
    pt_sampler_unpin(pin);assert(m->live);
}
static void backing_spans(void)
{
    struct memory memory={0};struct pt_allocator a={&memory,allocate,release};struct pt_document d;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[8];struct pt_event_change changes[8];
    int32_t values[4]={-128,1,2,127};uint32_t markers[2]={0,2};struct pt_pcm adopted;int32_t *storage;
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){values,4,4,8000,1,8};d.project.samples[0].slices=markers;
    d.project.samples[0].slice_count=2;pt_sampler_init(&s,&a,1024*1024);
    assert(pt_pattern_history_init(&h,&d.project,commands,8,changes,8)==PT_EDIT_OK);
    assert(pt_sampler_attributes(&s,&d.project,&h,0,"metadata",32,1)==PT_EDIT_OK);
    span_cases(&s,&d.project,0,&memory); /* metadata-only header + flat PCM/marker backing */
    storage=allocate(&memory,64*sizeof(*storage));assert(storage);
    memset(storage,0x7f,64*sizeof(*storage));memcpy(storage,values,sizeof(values));
    adopted=(struct pt_pcm){storage,64,4,8000,1,8};
    assert(pt_sampler_append_owned(&s,&d.project,&h,&adopted,&a,"owned")==PT_EDIT_OK && !adopted.data);
    assert(d.project.samples[31].pcm.capacity==64);
    assert(pt_sampler_attributes(&s,&d.project,&h,31,"owned metadata",31,0)==PT_EDIT_OK);
    span_cases(&s,&d.project,31,&memory); /* header accounting includes separate full PCM capacity */
    assert(storage[63]==0x7f7f7f7f);pt_pattern_history_release(&h);pt_sampler_release(&s);pt_document_release(&d);
    assert(!memory.live && !memory.bytes && !s.bytes);
}
static void batch_pressure(void)
{
    struct fixture f;struct pt_paula_future_owner *a,*b,*out=(void *)1;
    struct pt_paula_future_request requests[2]={{0,0,0,NULL,0,0},{1,1,0,NULL,0,0}};
    struct pt_paula_future_view first,second;struct pt_scheduled_batch e;struct pt_scheduled_grid grid={100,7,1000,100};
    struct pt_scheduled_backend backend;struct pt_paula_future_config config;
    unsigned ready,n;uint64_t ticket;enum pt_paula_future_result result;size_t poolbytes,ownerbytes;
    init(&f,16,1,3,16388);
    assert(pt_paula_future_begin(f.pool,f.queue,10,requests,2,&a)==PT_FUTURE_OK);
    n=0;do {result=pt_paula_future_step(a,&ready);assert(++n<=88);}while(result==PT_FUTURE_PENDING);
    assert(result==PT_FUTURE_OK && n==88 && ready);
    assert(pt_paula_future_view(a,0,&first)==PT_FUTURE_OK && pt_paula_future_view(a,1,&second)==PT_FUTURE_OK);
    assert(first.data!=second.data && f.chip.live==2);
    e=batch(first,10);e.count=2;e.action[1]=batch(second,10).action[0];
    e.action[1].data=(uint8_t *)f.document.project.samples[1].pcm.data;refused(&f,a,&e,PT_SCHEDULED_INVALID);
    e.action[1]=batch(second,10).action[0];e.action[1].slot=first.slot;refused(&f,a,&e,PT_SCHEDULED_INVALID);
    assert(pt_paula_future_owner_close(a));finish(&f);
    init(&f,16,1,3,8194);requests[1].sample=0;
    assert(pt_paula_future_begin(f.pool,f.queue,10,requests,2,&a)==PT_FUTURE_OK);
    n=0;do {result=pt_paula_future_step(a,&ready);assert(++n<=47);}while(result==PT_FUTURE_PENDING);
    assert(result==PT_FUTURE_OK && n==47 && ready); /* own promotion, then same-cache HIT */
    assert(pt_paula_future_view(a,0,&first)==PT_FUTURE_OK && pt_paula_future_view(a,1,&second)==PT_FUTURE_OK);
    assert(first.data==second.data && first.slot!=second.slot && f.chip.live==1);
    assert(pt_paula_future_owner_close(a));finish(&f);
    init(&f,16,1,2,8194);assert(pt_scheduled_output_close(f.queue));
    backend=(struct pt_scheduled_backend){&f.backend,sizeof(f.backend),{7,8,4},clock_read,submit,poll,cancel};
    assert(pt_scheduled_output_open(&f.allocator,&grid,&backend,1,&f.queue)==PT_SCHEDULED_OK);
    a=begin(&f,10,0,0,0);prepare(&f,a);assert(pt_paula_future_view(a,0,&first)==PT_FUTURE_OK);
    e=batch(first,10);assert(pt_paula_future_enqueue(a,&e,&ticket)==PT_SCHEDULED_OK);
    b=begin(&f,11,1,0,0);prepare(&f,b);assert(pt_paula_future_view(b,0,&second)==PT_FUTURE_OK);
    e=batch(second,11);refused(&f,b,&e,PT_SCHEDULED_CAPACITY);
    poolbytes=allocated(&f.fast,f.pool);ownerbytes=allocated(&f.fast,b);
    assert(pt_paula_future_owner_close(b));assert(pt_scheduled_output_stop(f.queue)==PT_SCHEDULED_OK);
    assert(pt_paula_future_owner_close(a));finish(&f);
    init(&f,16,1,2,8194);assert(pt_paula_future_close(f.pool));
    config=(struct pt_paula_future_config){2,poolbytes+ownerbytes-1,8194,7,&f.chip,chip_allocate,chip_release};
    assert(pt_paula_future_open(&f.allocator,&f.sampler,&f.document.project,&config,&f.pool)==PT_FUTURE_OK);
    assert(pt_paula_future_begin(f.pool,f.queue,10,requests,1,&out)==PT_FUTURE_CAPACITY && out==(void *)1);
    assert(pt_paula_future_begin(f.pool,f.queue,10,requests,5,&out)==PT_FUTURE_INVALID && out==(void *)1);
    assert(pt_paula_future_begin(f.pool,f.queue,10,requests,1,(struct pt_paula_future_owner **)(UINTPTR_MAX-1))==PT_FUTURE_INVALID);
    finish(&f);
}
static int foreign_current(void *c,uint64_t token,uint64_t generation)
{unsigned *calls=c;++*calls;return token==99 && generation==7;}
static void foreign_release(void *c,uint64_t token) {(void)c;(void)token;assert(0);}
static void full_table_guard(void)
{
    struct fixture f;struct pt_paula_future_config config;struct pt_paula_future_owner *o;struct pt_paula_future_view view;
    struct pt_pattern_history h;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    struct pt_scheduled_batch event;uint64_t ticket;unsigned calls=0;uint16_t data=0;
    struct pt_scheduled_span span={&data,sizeof(data)};struct pt_scheduled_owner foreign={&calls,99,foreign_current,foreign_release,&span,1};
    struct image table,queue,pool,holder;
    init(&f,16,1,2,8194);assert(pt_paula_future_close(f.pool));
    assert(pt_pattern_history_init(&h,&f.document.project,commands,4,changes,4)==PT_EDIT_OK);
    assert(pt_sampler_add_slot(&f.sampler,&f.document.project,&h)==PT_EDIT_OK && f.sampler.table);
    config=(struct pt_paula_future_config){2,1024*1024,8194,7,&f.chip,chip_allocate,chip_release};
    assert(pt_paula_future_open(&f.allocator,&f.sampler,&f.document.project,&config,&f.pool)==PT_FUTURE_OK);
    o=begin(&f,10,0,0,0);prepare(&f,o);assert(pt_paula_future_view(o,0,&view)==PT_FUTURE_OK);
    event=batch(view,10);assert(pt_paula_future_enqueue(o,&event,&ticket)==PT_SCHEDULED_OK);
    event.frame=11;event.action[0].data=(uint8_t *)&data;event.action[0].words=1;
    table=image(&f.fast,f.sampler.table);queue=image(&f.fast,f.queue);pool=image(&f.fast,f.pool);holder=image(&f.fast,o);
    assert(pt_scheduled_output_enqueue(f.queue,&event,&foreign,(uint64_t *)(f.sampler.table+200))==PT_SCHEDULED_INVALID);
    assert(!calls);unchanged(&table);unchanged(&queue);unchanged(&pool);unchanged(&holder);
    assert(pt_scheduled_output_stop(f.queue)==PT_SCHEDULED_OK);assert(pt_paula_future_owner_close(o));
    pt_pattern_history_release(&h);finish(&f);
}
static void preowned_reuse(unsigned transferred)
{
    struct fixture f;struct pt_paula_future_config config;struct pt_paula_future_owner *a,*b;
    struct pt_paula_future_view view;struct pt_scheduled_batch event;struct pt_pcm pcm;struct pt_sample_version *pin;
    struct pt_pattern_history h;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    void *old,*reused,*saved;size_t bytes,offset;uint64_t ticket=0;unsigned *ready;struct image source;
    init(&f,24,1,3,8194);assert(pt_paula_future_close(f.pool));
    assert(pt_sampler_pin(&f.sampler,&f.document.project,0,f.sampler.generation,&pcm,&pin)==PT_EDIT_OK);
    old=pin;bytes=allocated(&f.fast,old);offset=(uint8_t *)pcm.data-(uint8_t *)old;pt_sampler_unpin(pin);
    config=(struct pt_paula_future_config){3,1024*1024,8194,7,&f.chip,chip_allocate,chip_release};
    assert(pt_paula_future_open(&f.allocator,&f.sampler,&f.document.project,&config,&f.pool)==PT_FUTURE_OK);
    a=begin(&f,10,0,0,0);prepare(&f,a);assert(pt_paula_future_view(a,0,&view)==PT_FUTURE_OK);
    if(transferred) {
        event=batch(view,10);assert(pt_paula_future_enqueue(a,&event,&ticket)==PT_SCHEDULED_OK);
        assert(pt_scheduled_output_publish(f.queue,ticket)==PT_SCHEDULED_OK);
    }
    b=begin(&f,11,1,0,0);
    assert(pt_pattern_history_init(&h,&f.document.project,commands,4,changes,4)==PT_EDIT_OK);
    f.fast.quarantine=old;
    assert(pt_sampler_edit(&f.sampler,&f.document.project,&h,0,PT_PCM_REVERSE,0,8193,0)==PT_EDIT_OK);
    pt_pattern_history_release(&h);assert(!f.fast.spare);
    source=image(&f.fast,old);
    assert(pt_paula_future_view(a,0,(struct pt_paula_future_view *)((uint8_t *)old+offset))==PT_FUTURE_INVALID);
    unchanged(&source);
    if(transferred) {f.backend.poll=1;assert(pt_scheduled_output_poll(f.queue,ticket)==PT_SCHEDULED_OK);}
    else assert(pt_paula_future_cancel(a)==PT_FUTURE_OK);
    assert(f.fast.spare==old);reused=allocate(&f.fast,bytes);assert(reused==old && !f.fast.spare);
    ready=(unsigned *)((uint8_t *)reused+offset);*ready=99;saved=malloc(bytes);assert(saved);memcpy(saved,reused,bytes);
    /* Recycled allocation is unrelated output. Unpinned pending descriptors,
     * initial private captures and retired/cancelled holders must not police it
     * as a false source alias. Stale cancellation still preserves the output. */
    assert(pt_paula_future_step(b,ready)==PT_FUTURE_STALE && *ready==99 && !memcmp(saved,reused,bytes));
    free(saved);release(&f.fast,reused);
    assert(pt_paula_future_owner_close(a) && pt_paula_future_owner_close(b));finish(&f);
}
static void pressure_cancel_stale(void)
{
    struct fixture f;struct pt_paula_future_owner *a,*b,*sentinel=(void *)1;struct pt_paula_future_request r={0,0,0,NULL,0,0};
    struct pt_paula_future_view view;struct pt_scheduled_batch e;unsigned ready=99,i;uint64_t ticket;enum pt_paula_future_result result;
    /* Cancellation before allocation, during master copy, and unpublished Chip
     * copy does not expose a view or leak either budget. */
    for(i=0;i<4;++i) {
        unsigned j;init(&f,16,1,2,8194);a=begin(&f,10,0,0,0);
        for(j=0;j<(i==0?0:i==1?2:i==2?11:12);++j)assert(pt_paula_future_step(a,&ready)==PT_FUTURE_PENDING);
        assert(pt_paula_future_cancel(a)==PT_FUTURE_OK);assert(pt_paula_future_view(a,0,&view)==PT_FUTURE_INVALID);
        assert(pt_paula_future_owner_close(a));finish(&f);
    }
    init(&f,8,1,1,8194);a=begin(&f,10,0,0,0);
    assert(pt_paula_future_begin(f.pool,f.queue,11,&r,1,&sentinel)==PT_FUTURE_CAPACITY && sentinel==(void *)1);
    assert(pt_paula_future_cancel(a)==PT_FUTURE_OK && pt_paula_future_owner_close(a));finish(&f);
    init(&f,16,1,2,8193);a=begin(&f,10,0,0,0);
    for(i=0;i<20;++i) {result=pt_paula_future_step(a,&ready);if(result!=PT_FUTURE_PENDING)break;}
    assert(result==PT_FUTURE_CAPACITY && f.chip.live==0);assert(pt_paula_future_owner_close(a));finish(&f);
    /* Master budget and allocator failures cancel all unpublished state. */
    init(&f,16,1,2,8194);a=begin(&f,10,0,0,0);f.sampler.budget=1;ready=99;
    assert(pt_paula_future_step(a,&ready)==PT_FUTURE_CAPACITY && ready==99);assert(pt_paula_future_owner_close(a));finish(&f);
    init(&f,16,1,2,8194);f.fast.fail=f.fast.calls+1;
    assert(pt_paula_future_begin(f.pool,f.queue,10,&r,1,&sentinel)==PT_FUTURE_CAPACITY);f.fast.fail=0;
    a=begin(&f,10,0,0,0);f.fast.fail=f.fast.calls+1;
    assert(pt_paula_future_step(a,&ready)==PT_FUTURE_CAPACITY);f.fast.fail=0;
    assert(pt_paula_future_owner_close(a));finish(&f);
    init(&f,16,1,2,8194);a=begin(&f,10,0,0,0);f.chip.fail=1;
    for(i=0;i<20;++i){result=pt_paula_future_step(a,&ready);if(result!=PT_FUTURE_PENDING)break;}
    assert(result==PT_FUTURE_CAPACITY && !f.chip.live);f.chip.fail=0;
    assert(pt_paula_future_owner_close(a));finish(&f);
    init(&f,16,1,2,8194);a=begin(&f,10,0,0,0);assert(pt_paula_future_step(a,&ready)==PT_FUTURE_PENDING);
    f.document.project.samples=NULL;ready=99;
    assert(pt_paula_future_step(a,&ready)==PT_FUTURE_STALE && ready==99);
    f.document.project.samples=f.document.storage.samples;assert(pt_paula_future_owner_close(a));finish(&f);
    init(&f,16,1,2,8194);a=begin(&f,10,0,0,0);f.document.project.samples[0].volume=1;
    assert(pt_paula_future_step(a,&ready)==PT_FUTURE_STALE);assert(pt_paula_future_owner_close(a));finish(&f);
    init(&f,16,1,2,8194);a=begin(&f,10,0,0,0);prepare(&f,a);assert(pt_paula_future_view(a,0,&view)==PT_FUTURE_OK);
    e=batch(view,10);f.document.project.channels.track[0].route=PT_AMIGUS;
    refused(&f,a,&e,PT_SCHEDULED_STALE);f.document.project.channels.track[0].route=PT_PAULA;
    ++f.sampler.generation;refused(&f,a,&e,PT_SCHEDULED_STALE);--f.sampler.generation;
    assert(pt_paula_future_enqueue(a,&e,&ticket)==PT_SCHEDULED_OK);
    ++f.sampler.generation;assert(pt_scheduled_output_publish(f.queue,ticket)==PT_SCHEDULED_STALE);--f.sampler.generation;
    assert(pt_scheduled_output_stop(f.queue)==PT_SCHEDULED_OK);assert(pt_paula_future_owner_close(a));finish(&f);
    /* Full queue refuses atomically; uncertain backend/cancel retains owners. */
    init(&f,16,1,2,8194);a=begin(&f,10,0,0,0);prepare(&f,a);assert(pt_paula_future_view(a,0,&view)==PT_FUTURE_OK);
    e=batch(view,10);assert(pt_paula_future_enqueue(a,&e,&ticket)==PT_SCHEDULED_OK);
    b=begin(&f,9,1,0,0);prepare(&f,b);assert(pt_paula_future_view(b,0,&view)==PT_FUTURE_OK);
    e=batch(view,9);refused(&f,b,&e,PT_SCHEDULED_INVALID);assert(pt_paula_future_owner_close(b));
    f.backend.submit=-1;assert(pt_scheduled_output_publish(f.queue,ticket)==PT_SCHEDULED_BACKEND);
    f.backend.cancel=-1;assert(pt_scheduled_output_stop(f.queue)==PT_SCHEDULED_BACKEND);
    assert(!pt_paula_future_owner_close(a) && !pt_paula_future_close(f.pool) && f.chip.live==1);
    f.backend.cancel=1;assert(pt_scheduled_output_stop(f.queue)==PT_SCHEDULED_OK);
    assert(pt_paula_future_owner_close(a));finish(&f);
    /* Genuine allocation/free callbacks cannot close a busy pool/holder. */
    init(&f,16,1,2,8194);reenter_pool=f.pool;a=begin(&f,10,0,0,0);reenter_owner=a;
    prepare(&f,a);assert(pt_paula_future_cancel(a)==PT_FUTURE_OK);reenter_owner=NULL;
    assert(pt_paula_future_owner_close(a));reenter_pool=NULL;assert(reenter_calls);finish(&f);
}
int main(void)
{
    unsigned bits,channels;for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)precision(bits,channels);
    retrigger(0);retrigger(1);retrigger(2);retrigger_cancel();undo_stale();backing_spans();batch_pressure();full_table_guard();preowned_reuse(0);preowned_reuse(1);pressure_cancel_stale();
    puts("PAULA FUTURE TRIGGER OWNER PASS: independent master/cache retirement; software ownership only, no DMA or hardware timing proof");return 0;
}
