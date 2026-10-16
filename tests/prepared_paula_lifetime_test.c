/* PRIVATE HOST-only draft: genuine mono8/16/24 pins/Chip jobs and the actual
 * separately compiled pt_readers queue. No private holder layouts, owner
 * callbacks, quiet/ACTIVE receipts, publication or native authority are used.
 * Two deliberately invalid/faulted contexts stay reachable until exit. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../src/editor/prepared_paula_lifetime.h"
#include "../src/editor/sampler_internal.h"
#include "../src/editor/sampler_paula_internal.h"

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#define ALIGNMENT(T) offsetof(struct {char byte;T value;},value)
enum {FRAMES=1025,PACKED=1026,VALUES=1041,MAX_ALLOCATIONS=64,MAX_SPANS=64};
enum {HOOK_NONE,HOOK_ALLOC_GET,HOOK_RELEASE_CANCEL};
struct allocation {void *data;size_t bytes;unsigned chip;};
struct arena {unsigned char *data;size_t capacity,mapping;};
struct fixture;
struct control_memory {
    struct fixture *fixture;void *queue;size_t bytes;
    unsigned calls,allocations,releases,fail,hook,hook_calls,alias_returns;
    enum pt_prepared_paula_sources_result nested;
};
struct host_backend {uint64_t now;uint32_t frequency;unsigned calls;};
struct fixture {
    struct pt_allocator allocator;
    struct pt_document document;
    struct pt_sampler sampler;
    struct pt_sampler_paula bridge;
    struct pt_sample_version *pin[3];
    struct pt_pcm pcm[3];
    int32_t source[3][VALUES],unused[64];
    uint32_t markers[3][2];
    uint8_t gold[3][PACKED];
    struct allocation allocation[MAX_ALLOCATIONS];
    size_t fast_bytes,chip_bytes,fast_limit,chip_limit;
    unsigned fast_calls,chip_calls,fast_releases,chip_releases;
    unsigned hook,hook_calls,chip_fail;
    struct pt_prepared_paula_sources *owner;
    struct arena arena[2],lifetime_arena[4];
    unsigned arena_count,lifetime_count;
    struct pt_prepared_paula_lifetime *lifetime;
    struct pt_prepared_paula_lifetime_config config;
    struct pt_readers_backend backend;
    struct control_memory control;
    struct host_backend host;
    uint8_t *encoded;size_t encoded_bytes;
    struct pt_sample *sample_table;
};
struct span_before {const void *data;size_t bytes;unsigned char *copy;};
struct audit {struct span_before span[MAX_SPANS];unsigned count;};
/* A forbidden callback may create uncertain ownership. These two complete
 * heap fixtures, their allocators and all genuine remaining resources stay
 * reachable until process exit. They are never forcibly closed or repaired. */
static struct fixture *volatile retained[2];
static unsigned retained_count,alias_cases,clean_cases,stale_cases;
static uint64_t next_session=1000;

static void callback(struct fixture *f,unsigned event)
{
    struct pt_prepared_paula_sources_status out,before;
    if(f->hook!=event)return;
    f->hook=HOOK_NONE;++f->hook_calls;
    memset(&out,0x6d,sizeof(out));memcpy(&before,&out,sizeof(before));
    if(event==HOOK_ALLOC_GET)
        assert(pt_prepared_paula_sources_get(f->owner,&out)==PT_PREPARED_PAULA_SOURCES_FAULT);
    else
        assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_FAULT);
    assert(!memcmp(&out,&before,sizeof(out)));
}
static void *allocate_record(struct fixture *f,size_t bytes,unsigned chip)
{
    unsigned i;void *p;size_t *live=chip?&f->chip_bytes:&f->fast_bytes;
    size_t limit=chip?f->chip_limit:f->fast_limit;
    if(chip)++f->chip_calls;else ++f->fast_calls;
    if((chip&&f->chip_fail)||*live>limit||bytes>limit-*live)return NULL;
    p=malloc(bytes);assert(p);
    for(i=0;i<MAX_ALLOCATIONS&&f->allocation[i].data;++i){}
    assert(i<MAX_ALLOCATIONS);
    f->allocation[i]=(struct allocation){p,bytes,chip};*live+=bytes;
    memset(p,chip?0xa5:0xcc,bytes);
    if(chip)callback(f,HOOK_ALLOC_GET);
    return p;
}
static void release_record(struct fixture *f,void *p,size_t bytes,unsigned chip)
{
    unsigned i;
    if(!p)return;
    for(i=0;i<MAX_ALLOCATIONS&&f->allocation[i].data!=p;++i){}
    assert(i<MAX_ALLOCATIONS&&f->allocation[i].chip==chip);
    if(chip){assert(f->allocation[i].bytes==bytes&&f->chip_bytes>=bytes);
        ++f->chip_releases;f->chip_bytes-=bytes;}
    else {assert(f->fast_bytes>=f->allocation[i].bytes);
        ++f->fast_releases;f->fast_bytes-=f->allocation[i].bytes;}
    memset(f->allocation+i,0,sizeof(f->allocation[i]));free(p);
    if(chip)callback(f,HOOK_RELEASE_CANCEL);
}
static void *fast_allocate(void *context,size_t bytes)
{return allocate_record(context,bytes,0);}
static void fast_release(void *context,void *p)
{release_record(context,p,0,0);}
static void *chip_allocate(void *context,size_t bytes)
{return allocate_record(context,bytes,1);}
static void chip_release(void *context,void *p,size_t bytes)
{release_record(context,p,bytes,1);}

static uint8_t signed8(int32_t value,unsigned bits)
{
    int64_t magnitude=value<0?-(int64_t)value:(int64_t)value,result;
    unsigned shift=bits-8;
    if(shift)magnitude=(magnitude+((int64_t)1<<(shift-1)))>>shift;
    result=value<0?-magnitude:magnitude;
    if(result<-128)result=-128;
    if(result>127)result=127;
    return (uint8_t)result;
}
static unsigned pins(const struct fixture *f)
{
    unsigned i,n=0;
    for(i=0;i<PT_CACHE_SLOTS;++i)n+=f->bridge.cache.entry[i].pins;
    return n;
}
static struct pt_prepared_paula_sources_status status(struct fixture *f)
{
    struct pt_prepared_paula_sources_status s;
    unsigned fast=f->fast_calls,chip=f->chip_calls,fr=f->fast_releases,cr=f->chip_releases;
    memset(&s,0x59,sizeof(s));
    assert(pt_prepared_paula_sources_get(f->owner,&s)==PT_PREPARED_PAULA_SOURCES_OK);
    assert(f->fast_calls==fast&&f->chip_calls==chip&&f->fast_releases==fr&&f->chip_releases==cr);
    return s;
}
static void master_save_equal(struct fixture *f)
{
    uint8_t *encoded;size_t bytes,written;unsigned i;
    for(i=0;i<3;++i){
        const struct pt_sample *s=f->sample_table+i;
        assert(s->pcm.data==f->pcm[i].data&&s->pcm.bits==8+8*i&&s->pcm.channels==1);
        assert(s->pcm.frames==FRAMES&&s->pcm.capacity==FRAMES);
        assert(!memcmp(s->pcm.data,f->source[i],FRAMES*sizeof(int32_t)));
        assert(s->slice_count==2&&!memcmp(s->slices,f->markers[i],sizeof(f->markers[i])));
    }
    assert(pt_project_size(&f->document.project,&bytes)==PT_PROJECT_OK&&bytes==f->encoded_bytes);
    encoded=malloc(bytes);assert(encoded);
    assert(pt_project_encode(&f->document.project,encoded,bytes,&written)==PT_PROJECT_OK&&written==bytes);
    assert(!memcmp(encoded,f->encoded,bytes));free(encoded);
}
static void new_arena(struct fixture *f,unsigned guarded)
{
    struct arena *a;size_t prefix=pt_prepared_paula_sources_size(),page;
    assert(f->arena_count<2);a=f->arena+f->arena_count++;
    if(guarded){
        long value=sysconf(_SC_PAGESIZE);assert(value>0);page=(size_t)value;
        a->capacity=((prefix+page-1)/page+1)*page;a->mapping=a->capacity;
        a->data=mmap(NULL,a->mapping,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        assert(a->data!=MAP_FAILED);
        memset(a->data,0,prefix);memset(a->data+prefix,0xb6,a->capacity-prefix);
        if(guarded==1)assert(!mprotect(a->data+a->capacity-page,page,PROT_NONE));
    }else{
        a->capacity=prefix+pt_prepared_paula_lifetime_size()+512;a->data=malloc(a->capacity);assert(a->data);
        memset(a->data,0,prefix);memset(a->data+prefix,0xb6,a->capacity-prefix);
    }
    assert((uintptr_t)a->data%pt_prepared_paula_sources_alignment()==0);
    f->owner=(struct pt_prepared_paula_sources *)a->data;
}
static void owner_begin(struct fixture *f)
{
    unsigned samples[3]={0,1,2},fast=f->fast_calls,chip=f->chip_calls;
    struct arena *a=f->arena+f->arena_count-1;
    assert(pt_prepared_paula_sources_init(f->owner,a->capacity,&f->bridge,0,samples)==PT_PREPARED_PAULA_SOURCES_OK);
    samples[0]=samples[1]=samples[2]=UINT32_MAX; /* The argument array expires. */
    assert(f->fast_calls==fast&&f->chip_calls==chip&&!f->sampler.pending_count);
}
static struct fixture *setup(size_t chip_budget,unsigned guarded)
{
    struct fixture *f=calloc(1,sizeof(*f));unsigned i,j;
    assert(f);f->fast_limit=1024*1024;f->chip_limit=3*PACKED;
    f->allocator=(struct pt_allocator){f,fast_allocate,fast_release};
    pt_document_init(&f->document,&f->allocator);
    assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<4;++i)f->document.project.channels.track[i].route=i?PT_AMIGUS:PT_PAULA;
    for(i=0;i<3;++i){
        unsigned bits=8+8*i;int32_t low=-(int32_t)((uint32_t)1<<(bits-1));
        int32_t high=-low-1;
        struct pt_sample *s=f->document.project.samples+i;
        for(j=0;j<VALUES;++j){
            uint64_t range=((uint64_t)1<<bits)-1;
            f->source[i][j]=(int32_t)(((uint64_t)j*65357+17*i)%range)+low;
        }
        f->source[i][0]=low;f->source[i][1]=high;
        f->source[i][2]=-1;f->source[i][3]=1;
        f->source[i][4]=i?(int32_t)((uint32_t)1<<(bits-9)):1;
        f->source[i][5]=-f->source[i][4];
        f->markers[i][0]=16;f->markers[i][1]=512;
        s->pcm=(struct pt_pcm){f->source[i],VALUES,FRAMES,48000,1,(uint8_t)bits};
        s->slices=f->markers[i];s->slice_count=2;s->volume=48;
        s->name[0]='A'+(char)i;s->name[1]=0;
        for(j=0;j<FRAMES;++j)f->gold[i][j]=signed8(f->source[i][j],bits);
        f->gold[i][FRAMES]=0;
        assert(f->gold[i][0]==128&&f->gold[i][1]==127&&f->gold[i][4]==1&&f->gold[i][5]==255);
    }
    for(i=0;i<64;++i)f->unused[i]=(int32_t)i-32;
    f->document.project.samples[3].pcm=(struct pt_pcm){f->unused,64,4,8000,1,8};
    assert(pt_project_validate(&f->document.project,NULL)==PT_PROJECT_OK);
    pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
    for(i=0;i<3;++i){
        assert(pt_sampler_pin(&f->sampler,&f->document.project,i,f->sampler.generation,
                              f->pcm+i,f->pin+i)==PT_EDIT_OK);
        assert(f->pin[i]&&f->pin[i]==f->sampler.current[i]&&f->pcm[i].data!=f->source[i]);
        if(i)assert(f->pin[i]!=f->pin[i-1]&&f->pcm[i].data!=f->pcm[i-1].data);
    }
    f->sample_table=f->document.project.samples;
    assert(f->sampler.live_count==3&&!f->sampler.pending_count&&f->sampler.bytes);
    assert(pt_project_validate(&f->document.project,NULL)==PT_PROJECT_OK);
    assert(pt_sampler_paula_bind(&f->bridge,&f->sampler,&f->document.project,f,
                                 chip_allocate,chip_release,chip_budget));
    assert(pt_project_size(&f->document.project,&f->encoded_bytes)==PT_PROJECT_OK);
    f->encoded=malloc(f->encoded_bytes);assert(f->encoded);
    {size_t written;assert(pt_project_encode(&f->document.project,f->encoded,f->encoded_bytes,&written)==PT_PROJECT_OK&&written==f->encoded_bytes);}
    new_arena(f,guarded);return f;
}
static void finish(struct fixture *f)
{
    unsigned i;struct pt_prepared_paula_sources_status s=status(f);
    assert(s.phase==PT_PREPARED_PAULA_SOURCES_FINISHED&&!s.master_pins&&!s.chip_leases&&!s.master_job&&!s.chip_job);
    assert(!f->sampler.pending_count&&!pins(f));master_save_equal(f);
    assert(pt_sampler_paula_close(&f->bridge));assert(!f->chip_bytes);
    for(i=0;i<3;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    pt_sampler_release(&f->sampler);assert(!f->sampler.bytes&&!f->sampler.live_count);
    pt_document_release(&f->document);assert(!f->fast_bytes);
    for(i=0;i<MAX_ALLOCATIONS;++i)assert(!f->allocation[i].data);
    for(i=0;i<f->arena_count;++i){
        if(f->arena[i].mapping)assert(!munmap(f->arena[i].data,f->arena[i].mapping));
        else free(f->arena[i].data);
    }
    assert(!f->control.queue&&f->control.allocations==f->control.releases);
    for(i=0;i<f->lifetime_count;++i){
        struct arena *a=f->lifetime_arena+i;
        assert(a->mapping&&!munmap(a->data,a->mapping));
    }
    free(f->encoded);free(f);
}
static void audit_add(struct audit *a,const void *data,size_t bytes)
{
    unsigned i=a->count;
    if(!bytes)return;
    assert(data&&i<MAX_SPANS&&bytes<1024*1024);
    a->span[i]=(struct span_before){data,bytes,malloc(bytes)};assert(a->span[i].copy);
    memcpy(a->span[i].copy,data,bytes);++a->count;
}
static void audit_capture(struct fixture *f,struct audit *a,unsigned owner)
{
    struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned i,j,count;
    memset(a,0,sizeof(*a));
    if(owner){struct arena *r=f->arena+f->arena_count-1;assert(!r->mapping);audit_add(a,r->data,r->capacity);}
    audit_add(a,&f->bridge,sizeof(f->bridge));audit_add(a,&f->sampler,sizeof(f->sampler));
    audit_add(a,&f->document,sizeof(f->document));
    audit_add(a,f->sample_table,f->document.project.sample_count*sizeof(*f->sample_table));
    audit_add(a,f->document.storage.events,f->document.storage.event_capacity*sizeof(struct pt_event));
    audit_add(a,f->document.storage.orders,f->document.storage.order_capacity*sizeof(uint16_t));
    audit_add(a,f->source,sizeof(f->source));audit_add(a,f->unused,sizeof(f->unused));
    for(i=0;i<3;++i){
        assert(pt_sampler_version_spans(f->pin[i],spans,PT_SAMPLER_VERSION_SPANS,&count));
        for(j=0;j<count;++j)audit_add(a,spans[j].data,spans[j].bytes);
    }
    for(i=0;i<PT_CACHE_SLOTS;++i)
        if(f->bridge.cache.entry[i].data)audit_add(a,f->bridge.cache.entry[i].data,f->bridge.cache.entry[i].bytes);
}
static void audit_equal_release(struct audit *a)
{
    unsigned i;for(i=0;i<a->count;++i){
        assert(!memcmp(a->span[i].data,a->span[i].copy,a->span[i].bytes));free(a->span[i].copy);
    }memset(a,0,sizeof(*a));
}
static void load_state(struct fixture *f,unsigned step)
{
    struct pt_prepared_paula_sources_status s=status(f);unsigned done=step/8,part=step%8,i;
    assert(step<=24&&s.phase==(step==24?PT_PREPARED_PAULA_SOURCES_PREPARED:PT_PREPARED_PAULA_SOURCES_PREPARING));
    assert(s.failure==PT_PREPARED_PAULA_SOURCES_OK&&s.completed==done);
    assert(s.master_pins==done+(part>=2)&&s.chip_leases==done);
    assert(s.master_job==(part==1)&&s.chip_job==(part>=3));
    assert(f->sampler.pending_count==(size_t)(part==1)&&f->sampler.live_count==3);
    assert(pins(f)==done+(part>=3)&&f->chip_bytes==(done+(part>=3))*PACKED);
    for(i=0;i<done+(part>=3);++i){
        const struct pt_cache_entry *e=f->bridge.cache.entry+i;
        unsigned copied=i<done?PACKED:256*(part-3);
        const uint8_t *p=e->data;unsigned j;
        assert(p&&e->bytes==PACKED&&e->pins==1&&e->valid==(unsigned)(i<done));
        for(j=0;j<PACKED;++j)assert(p[j]==(j<copied?f->gold[i][j]:0xa5));
    }
}
static void advance(struct fixture *f,unsigned end)
{
    unsigned i,fast=f->fast_calls,chip=f->chip_calls;
    load_state(f,0);
    for(i=1;i<=end;++i){
        f->document.project.channels.selected=(uint8_t)(i%4);
        assert(pt_prepared_paula_sources_step(f->owner)==(i==24?PT_PREPARED_PAULA_SOURCES_READY:PT_PREPARED_PAULA_SOURCES_PENDING));
        f->document.project.channels.selected=0;
        assert(f->fast_calls==fast&&f->chip_calls==chip+(i+5)/8);
        load_state(f,i);master_save_equal(f);
    }
}
static void cancel_all(struct fixture *f,unsigned observe)
{
    unsigned n=0;enum pt_prepared_paula_sources_result result;
    struct pt_prepared_paula_sources_status before,after;
    do{
        unsigned old=0;
        if(observe){before=status(f);old=before.master_pins+before.chip_leases+before.master_job+before.chip_job;}
        result=pt_prepared_paula_sources_cancel_step(f->owner);assert(++n<=9);
        assert(result==PT_PREPARED_PAULA_SOURCES_PENDING||result==PT_PREPARED_PAULA_SOURCES_CLOSED);
        if(observe){
            after=status(f);
            assert(after.master_pins+after.chip_leases+after.master_job+after.chip_job==(old?old-1:0));
            assert((result==PT_PREPARED_PAULA_SOURCES_CLOSED)==(old==0));
        }
    }while(result!=PT_PREPARED_PAULA_SOURCES_CLOSED);
    {struct arena *a=f->arena+f->arena_count-1;unsigned char *saved=malloc(pt_prepared_paula_sources_size());
     unsigned fast=f->fast_calls,chip=f->chip_calls,fr=f->fast_releases,cr=f->chip_releases;
     assert(saved);memcpy(saved,a->data,pt_prepared_paula_sources_size());
     assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_CLOSED);
     assert(!memcmp(saved,a->data,pt_prepared_paula_sources_size()));free(saved);
     assert(f->fast_calls==fast&&f->chip_calls==chip&&f->fast_releases==fr&&f->chip_releases==cr);}
}
/* The supplied HOST backend is deliberately fail-closed: it accepts no
 * publication and never claims adoption/quiet. These complete callbacks meet
 * the refusal contract, but no backend or timing qualification is inferred.
 * Every case below requires zero calls, including read_clock. */
static int host_clock(void *v,uint64_t *tick,uint32_t *hz)
{struct host_backend *b=v;++b->calls;*tick=b->now;*hz=b->frequency;return 1;}
static int host_submit(void *v,const struct pt_readers_event *e)
{struct host_backend *b=v;(void)e;++b->calls;return 0;}
static enum pt_readers_reply host_command(void *v,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{struct host_backend *b=v;(void)ticket;(void)out;++b->calls;return PT_READERS_UNCERTAIN;}
static enum pt_readers_reply host_reader(void *v,const struct pt_readers_domain *d,
    struct pt_readers_reader_receipt *out)
{struct host_backend *b=v;(void)d;(void)out;++b->calls;return PT_READERS_UNCERTAIN;}
enum {CONTROL_NONE,CONTROL_INIT_REENTRY,CONTROL_CANCEL_PRECLAIM,
      CONTROL_CLOSE_REENTRY,CONTROL_ALIAS_CONFIG,CONTROL_ALIAS_BACKEND};
static void *control_allocate(void *v,size_t bytes)
{
    struct control_memory *m=v;struct fixture *f=m->fixture;unsigned hook=m->hook;
    void *p;unsigned char *before;
    ++m->calls;m->hook=CONTROL_NONE;
    if(m->fail)return NULL;
    if(hook==CONTROL_ALIAS_CONFIG||hook==CONTROL_ALIAS_BACKEND){
        ++m->alias_returns;
        return hook==CONTROL_ALIAS_CONFIG?(void *)&f->config:(void *)&f->backend;
    }
    assert(!m->queue&&bytes);p=malloc(bytes);assert(p);
    m->queue=p;m->bytes=bytes;++m->allocations;memset(p,0x4c,bytes);
    if(hook==CONTROL_INIT_REENTRY){
        struct arena *a=f->lifetime_arena+f->lifetime_count-1;
        size_t n=pt_prepared_paula_lifetime_size();before=malloc(n);assert(before);
        memcpy(before,f->lifetime,n);++m->hook_calls;
        m->nested=pt_prepared_paula_lifetime_init(f->lifetime,a->capacity,
            f->owner,&f->config,&f->backend);
        assert(m->nested==PT_PREPARED_PAULA_SOURCES_BUSY);
        assert(!memcmp(before,f->lifetime,n));free(before);
    }else if(hook==CONTROL_CANCEL_PRECLAIM){
        ++m->hook_calls;m->nested=pt_prepared_paula_sources_cancel_step(f->owner);
        assert(m->nested==PT_PREPARED_PAULA_SOURCES_PENDING);
    }
    return p;
}
static void control_release(void *v,void *p)
{
    struct control_memory *m=v;struct fixture *f=m->fixture;unsigned hook=m->hook;
    assert(p&&p==m->queue&&m->bytes);m->hook=CONTROL_NONE;
    /* Actual allocator free precedes the forbidden nested method. The outer
     * raw pt_readers_close has consumed its object even if that method faults. */
    m->queue=NULL;m->bytes=0;++m->releases;free(p);
    if(hook==CONTROL_CLOSE_REENTRY){
        ++m->hook_calls;m->nested=pt_prepared_paula_lifetime_close(f->lifetime);
        assert(m->nested==PT_PREPARED_PAULA_SOURCES_FAULT);
    }
}
static void config_begin(struct fixture *f)
{
    memset(&f->config,0,sizeof(f->config));f->control.fixture=f;
    f->host.now=100;f->host.frequency=1000000;
    f->backend=(struct pt_readers_backend){&f->host,sizeof(f->host),
        {PT_SCHEDULED_REQUIRED,1,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        host_clock,host_submit,host_command,host_command,host_reader,host_reader};
    f->config.allocator=(struct pt_allocator){&f->control,control_allocate,control_release};
    f->config.grid=(struct pt_scheduled_grid){100,7,1000000,48000};
    assert(next_session<UINT64_MAX);f->config.session=++next_session;
    f->config.frame=100;f->config.control_budget=1024*1024;
    f->config.slot=0;f->config.period=124;f->config.volume=48;
    /* All three callback contexts genuinely lie in this complete heap fixture.
     * This is not a size inferred from a pointer. Actual retained masters and
     * Chip data are separate tracked allocations, disjoint from these controls. */
    f->config.context_count=1;
    f->config.context[0]=(struct pt_prepared_paula_context_extent){f,sizeof(*f)};
}
static struct arena *lifetime_arena(struct fixture *f)
{
    struct arena *a;size_t size=pt_prepared_paula_lifetime_size(),extra;
    long n=sysconf(_SC_PAGESIZE);assert(n>0&&f->lifetime_count<4);
    extra=sizeof(f->config)+sizeof(f->backend)+512;
    a=f->lifetime_arena+f->lifetime_count++;
    a->capacity=((size+extra+(size_t)n-1)/(size_t)n)*(size_t)n;a->mapping=a->capacity;
    a->data=mmap(NULL,a->mapping,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(a->data!=MAP_FAILED&&(uintptr_t)a->data%pt_prepared_paula_lifetime_alignment()==0);
    memset(a->data,0,size);memset(a->data+size,0xb6,a->capacity-size);
    f->lifetime=(void *)a->data;return a;
}
static void lifetime_tail_equal(const struct arena *a)
{
    size_t i;for(i=pt_prepared_paula_lifetime_size();i<a->capacity;++i)assert(a->data[i]==0xb6);
}
static void lifetime_begin(struct fixture *f)
{
    struct arena *a=lifetime_arena(f);config_begin(f);
    assert(pt_prepared_paula_lifetime_init(f->lifetime,a->capacity,f->owner,
        &f->config,&f->backend)==PT_PREPARED_PAULA_SOURCES_OK);
    lifetime_tail_equal(a);assert(!f->host.calls&&f->control.queue);
}
static struct fixture *ready(void)
{struct fixture *f=setup(3*PACKED,0);owner_begin(f);advance(f,24);return f;}
static void no_resource_delta(struct fixture *f,unsigned fast,unsigned chip,unsigned fr,unsigned cr)
{assert(f->fast_calls==fast&&f->chip_calls==chip&&f->fast_releases==fr&&f->chip_releases==cr);}
static void claimed_cancel_refuses(struct fixture *f)
{
    struct arena *a=f->arena;unsigned fast=f->fast_calls,chip=f->chip_calls;
    unsigned fr=f->fast_releases,cr=f->chip_releases;unsigned char *copy=malloc(a->capacity);
    assert(copy);memcpy(copy,a->data,a->capacity);
    assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_BUSY);
    assert(!memcmp(copy,a->data,a->capacity));free(copy);
    no_resource_delta(f,fast,chip,fr,cr);assert(!f->host.calls);
}
static void enqueue_and_stop(struct fixture *f)
{
    struct pt_prepared_paula_sources_status s;
    unsigned fast=f->fast_calls,chip=f->chip_calls,fr=f->fast_releases,cr=f->chip_releases;
    assert(pt_prepared_paula_lifetime_enqueue_unpublished(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    s=status(f);assert(s.master_pins==3&&s.chip_leases==3&&pins(f)==3);
    claimed_cancel_refuses(f);
    assert(pt_prepared_paula_lifetime_enqueue_unpublished(f->lifetime)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(pt_prepared_paula_lifetime_drain_step(f->lifetime)==PT_PREPARED_PAULA_SOURCES_BUSY);
    assert(pt_prepared_paula_lifetime_stop_local(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    /* This is the actual separately compiled core's unpublished local C->R
     * release path. No fixture callback/terminal/receipt is invoked or forged. */
    no_resource_delta(f,fast,chip,fr,cr);s=status(f);
    assert(s.master_pins==3&&s.chip_leases==3&&pins(f)==3);
    claimed_cancel_refuses(f);
    assert(pt_prepared_paula_lifetime_stop_local(f->lifetime)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(!f->host.calls);master_save_equal(f);
}
static void drain_all(struct fixture *f,unsigned observe)
{
    enum pt_prepared_paula_sources_result result;unsigned n=0;
    do{
        struct pt_prepared_paula_sources_status before,after;unsigned old=0;
        unsigned fast=f->fast_calls,chip=f->chip_calls,fr=f->fast_releases,cr=f->chip_releases;
        if(observe){before=status(f);old=before.master_pins+before.chip_leases;}
        result=pt_prepared_paula_lifetime_drain_step(f->lifetime);++n;
        assert(n<=7&&(result==PT_PREPARED_PAULA_SOURCES_PENDING||result==PT_PREPARED_PAULA_SOURCES_CLOSED));
        assert(f->fast_calls==fast&&f->chip_calls==chip&&f->fast_releases==fr&&f->chip_releases==cr);
        if(observe){after=status(f);assert(after.master_pins+after.chip_leases==(old?old-1:0));
            assert((result==PT_PREPARED_PAULA_SOURCES_CLOSED)==(old==0));}
        if(result!=PT_PREPARED_PAULA_SOURCES_CLOSED)claimed_cancel_refuses(f);
        assert(!f->host.calls);master_save_equal(f);
    }while(result!=PT_PREPARED_PAULA_SOURCES_CLOSED);
    assert(n==7&&!pins(f)&&f->chip_bytes==3*PACKED&&!f->sampler.pending_count);
    assert(pt_prepared_paula_lifetime_drain_step(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_CLOSED);
}
static void clean_transfer_and_dead_lifetime(void)
{
    struct fixture *f=ready();struct arena *a;unsigned frees;
    lifetime_begin(f);a=f->lifetime_arena;enqueue_and_stop(f);drain_all(f,1);
    frees=f->control.releases;
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    assert(f->control.releases==frees+1&&!f->control.queue);
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    assert(f->control.releases==frees+1);lifetime_tail_equal(a);
    /* Genuine positive ledger completion must detach custody. A dangling
     * claim would fault here instead of following the finished source metadata. */
    assert(!mprotect(a->data,a->mapping,PROT_NONE));f->lifetime=NULL;
    assert(status(f).phase==PT_PREPARED_PAULA_SOURCES_FINISHED);
    assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    finish(f);++clean_cases;
}
static void stale_after_transfer(void)
{
    struct fixture *f=ready();unsigned generation;struct pt_prepared_paula_sources_status out,saved;
    lifetime_begin(f);
    assert(pt_prepared_paula_lifetime_enqueue_unpublished(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    assert(f->sampler.generation<UINT32_MAX);generation=++f->sampler.generation;
    assert(pt_prepared_paula_sources_step(f->owner)==PT_PREPARED_PAULA_SOURCES_STALE);
    memset(&out,0x51,sizeof(out));saved=out;
    assert(pt_prepared_paula_sources_get(f->owner,&out)==PT_PREPARED_PAULA_SOURCES_STALE);
    assert(!memcmp(&out,&saved,sizeof(out)));claimed_cancel_refuses(f);
    assert(pt_prepared_paula_lifetime_stop_local(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    assert(pins(f)==3);drain_all(f,0);
    assert(f->sampler.generation==generation&&status(f).failure==PT_PREPARED_PAULA_SOURCES_STALE);
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    /* No old source generation/table/route is restored to permit cleanup. */
    assert(f->sampler.generation==generation);finish(f);++stale_cases;
}
static void constructor_window_and_allocation(void)
{
    unsigned mode;
    for(mode=0;mode<3;++mode){
        struct fixture *f=ready();struct arena *a=lifetime_arena(f);
        struct pt_prepared_paula_sources_status s;config_begin(f);
        f->control.fail=(mode==0);
        f->control.hook=mode==1?CONTROL_INIT_REENTRY:mode==2?CONTROL_CANCEL_PRECLAIM:CONTROL_NONE;
        assert(pt_prepared_paula_lifetime_init(f->lifetime,a->capacity,f->owner,&f->config,&f->backend)==
            (mode==0?PT_PREPARED_PAULA_SOURCES_CAPACITY:PT_PREPARED_PAULA_SOURCES_OK));
        lifetime_tail_equal(a);s=status(f);assert(!f->host.calls);
        if(mode==2){
            /* Empty queue allocation ran before any claim. Cancellation was
             * allowed to consume the first genuine lease. Never restore it. */
            assert(f->control.hook_calls==1&&s.phase==PT_PREPARED_PAULA_SOURCES_CANCELLING);
            assert(s.master_pins==3&&s.chip_leases==2&&pins(f)==2);
            assert(pt_prepared_paula_lifetime_enqueue_unpublished(f->lifetime)==PT_PREPARED_PAULA_SOURCES_STALE);
        }else assert(s.master_pins==3&&s.chip_leases==3&&pins(f)==3);
        if(mode==1)assert(f->control.hook_calls==1&&f->control.nested==PT_PREPARED_PAULA_SOURCES_BUSY);
        assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
        cancel_all(f,1);finish(f);++clean_cases;
    }
}
static void unchanged_init_refusal(struct fixture *f,struct pt_prepared_paula_lifetime *l,size_t capacity,
    const struct pt_prepared_paula_lifetime_config *config,const struct pt_readers_backend *backend,
    enum pt_prepared_paula_sources_result expected)
{
    struct audit audit;unsigned calls=f->control.calls;unsigned char *before=malloc(capacity);
    assert(before);memcpy(before,l,capacity);audit_capture(f,&audit,1);
    assert(pt_prepared_paula_lifetime_init(l,capacity,f->owner,config,backend)==expected);
    assert(!memcmp(before,l,capacity));free(before);audit_equal_release(&audit);
    assert(f->control.calls==calls);++alias_cases;
}
static void constructor_aliases_and_nonzero(void)
{
    struct fixture *f=ready();struct arena *a=lifetime_arena(f),*source=f->arena;
    struct pt_prepared_paula_lifetime_config cfg;size_t size=pt_prepared_paula_lifetime_size();
    size_t align=pt_prepared_paula_lifetime_alignment(),off;unsigned char *tail;
    config_begin(f);
    a->data[size/2]=1;
    unchanged_init_refusal(f,f->lifetime,a->capacity,&f->config,&f->backend,PT_PREPARED_PAULA_SOURCES_BUSY);
    a->data[size/2]=0;
    off=((size+ALIGNMENT(struct pt_prepared_paula_lifetime_config)-1)/ALIGNMENT(struct pt_prepared_paula_lifetime_config))*ALIGNMENT(struct pt_prepared_paula_lifetime_config);
    tail=a->data+off;assert(off+sizeof(cfg)<=a->capacity);memcpy(tail,&f->config,sizeof(cfg));
    unchanged_init_refusal(f,f->lifetime,a->capacity,(void *)tail,&f->backend,PT_PREPARED_PAULA_SOURCES_ALIAS);
    memset(a->data+size,0xb6,a->capacity-size);
    off=((size+ALIGNMENT(struct pt_readers_backend)-1)/ALIGNMENT(struct pt_readers_backend))*ALIGNMENT(struct pt_readers_backend);
    tail=a->data+off;assert(off+sizeof(f->backend)<=a->capacity);memcpy(tail,&f->backend,sizeof(f->backend));
    unchanged_init_refusal(f,f->lifetime,a->capacity,&f->config,(void *)tail,PT_PREPARED_PAULA_SOURCES_ALIAS);
    memset(a->data+size,0xb6,a->capacity-size);
    cfg=f->config;cfg.context[cfg.context_count++]=(struct pt_prepared_paula_context_extent){a->data+size,a->capacity-size};
    unchanged_init_refusal(f,f->lifetime,a->capacity,&cfg,&f->backend,PT_PREPARED_PAULA_SOURCES_ALIAS);
    cfg=f->config;cfg.context_count=0;
    unchanged_init_refusal(f,f->lifetime,a->capacity,&cfg,&f->backend,PT_PREPARED_PAULA_SOURCES_INVALID);
    off=((pt_prepared_paula_sources_size()+align-1)/align)*align;
    assert(off+size<=source->capacity);
    unchanged_init_refusal(f,(void *)(source->data+off),size,&f->config,&f->backend,PT_PREPARED_PAULA_SOURCES_ALIAS);
    assert(pt_prepared_paula_lifetime_init(f->lifetime,a->capacity,f->owner,&f->config,&f->backend)==PT_PREPARED_PAULA_SOURCES_OK);
    lifetime_tail_equal(a);
    assert(pt_prepared_paula_lifetime_enqueue_unpublished(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    /* Source get must reject the complete lifetime tail and declared callback
     * contexts before writing either output or owner bytes. */
    {const void *out[6];unsigned i;size_t output_align=ALIGNMENT(struct pt_prepared_paula_sources_status);
     out[0]=a->data;out[1]=a->data+((size+output_align-1)/output_align)*output_align;
     out[2]=&f->control;out[3]=&f->config;out[4]=&f->backend;
     out[5]=source->data+((pt_prepared_paula_sources_size()+output_align-1)/output_align)*output_align;
     for(i=0;i<6;++i){struct audit audit;unsigned char output_before[sizeof(struct pt_prepared_paula_sources_status)];
        unsigned char *copy=malloc(a->capacity);assert(copy);
        assert((uintptr_t)out[i]%output_align==0);memcpy(output_before,out[i],sizeof(output_before));
        memcpy(copy,a->data,a->capacity);audit_capture(f,&audit,1);
        assert(pt_prepared_paula_sources_get(f->owner,(void *)out[i])==PT_PREPARED_PAULA_SOURCES_ALIAS);
        assert(!memcmp(copy,a->data,a->capacity)&&!memcmp(output_before,out[i],sizeof(output_before)));
        free(copy);audit_equal_release(&audit);++alias_cases;}}
    assert(pt_prepared_paula_lifetime_stop_local(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    drain_all(f,1);assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    lifetime_tail_equal(a);finish(f);++clean_cases;
}
static void retain_fault(struct fixture *f)
{
    assert(retained_count<2&&f->sampler.live_count==3&&f->fast_bytes&&f->chip_bytes==3*PACKED);
    assert(!f->host.calls);master_save_equal(f);retained[retained_count++]=f;
}
static void refused_lease_and_known_allocation_alias(void)
{
    struct fixture *f=ready();struct pt_cache_lease own;unsigned mode;
    /* Known input aliases returned by a malicious control allocator are not
     * fresh allocations. Both original input bodies remain unchanged, and no
     * release is called on either address. The complete fault contexts remain
     * reachable within this same retained fixture. */
    for(mode=0;mode<2;++mode){
        struct arena *a=lifetime_arena(f);struct pt_prepared_paula_lifetime_config cfg;
        struct pt_readers_backend be;config_begin(f);cfg=f->config;be=f->backend;
        f->control.hook=mode?CONTROL_ALIAS_BACKEND:CONTROL_ALIAS_CONFIG;
        assert(pt_prepared_paula_lifetime_init(f->lifetime,a->capacity,f->owner,&f->config,&f->backend)==PT_PREPARED_PAULA_SOURCES_FAULT);
        assert(!memcmp(&cfg,&f->config,sizeof(cfg))&&!memcmp(&be,&f->backend,sizeof(be)));
        assert(!f->control.queue&&!f->control.allocations&&!f->control.releases);
        assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_FAULT);
        lifetime_tail_equal(a);
    }
    assert(f->control.alias_returns==2);lifetime_begin(f);
    assert(pt_prepared_paula_lifetime_enqueue_unpublished(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    /* Real HIT creates an independent genuine lease. Its first unpin is legal.
     * Its second use is an INTENTIONAL CONTRACT VIOLATION: it consumes the
     * source owner's reference. This is a refusal-retention negative case,
     * not an allowed ownership pattern. No owner lease, cache serial, count,
     * validity, pin or callback is fabricated or directly changed. */
    assert(pt_sampler_paula_acquire(&f->bridge,0,0,0,&own)==PT_CACHE_HIT&&pins(f)==4);
    assert(pt_sampler_paula_unpin(&f->bridge,own)&&pins(f)==3);
    assert(pt_sampler_paula_unpin(&f->bridge,own)&&pins(f)==2);
    assert(pt_prepared_paula_lifetime_stop_local(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    {unsigned fast=f->fast_calls,chip=f->chip_calls,fr=f->fast_releases,cr=f->chip_releases;
     struct pt_prepared_paula_sources_status out,saved;memset(&out,0x6e,sizeof(out));saved=out;
     assert(pt_prepared_paula_lifetime_drain_step(f->lifetime)==PT_PREPARED_PAULA_SOURCES_FAULT);
     assert(pt_prepared_paula_sources_get(f->owner,&out)==PT_PREPARED_PAULA_SOURCES_FAULT);
     assert(!memcmp(&out,&saved,sizeof(out)));claimed_cancel_refuses(f);
     assert(pt_prepared_paula_lifetime_drain_step(f->lifetime)==PT_PREPARED_PAULA_SOURCES_BUSY);
     assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_FAULT);
     no_resource_delta(f,fast,chip,fr,cr);assert(pins(f)==2&&f->control.queue);}
    retain_fault(f);
}
static void consumed_queue_on_release_fault(void)
{
    struct fixture *f=ready();unsigned calls,frees;struct pt_prepared_paula_sources_status s;
    lifetime_begin(f);enqueue_and_stop(f);drain_all(f,1);
    calls=f->control.calls;frees=f->control.releases;f->control.hook=CONTROL_CLOSE_REENTRY;
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_FAULT);
    assert(f->control.nested==PT_PREPARED_PAULA_SOURCES_FAULT&&f->control.hook_calls==1);
    assert(!f->control.queue&&f->control.releases==frees+1);
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_FAULT);
    assert(f->control.calls==calls&&f->control.releases==frees+1);
    s=status(f);assert(s.phase==PT_PREPARED_PAULA_SOURCES_FINISHED&&!s.master_pins&&!s.chip_leases);
    assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    assert(!pins(f));retain_fault(f);
}
int main(void)
{
    clean_transfer_and_dead_lifetime();stale_after_transfer();
    constructor_window_and_allocation();constructor_aliases_and_nonzero();
    refused_lease_and_known_allocation_alias();consumed_queue_on_release_fault();
    assert(clean_cases==5&&stale_cases==1&&alias_cases==12&&retained_count==2);
    assert(retained[0]&&retained[1]&&retained[0]!=retained[1]);
    puts("PREPARED PAULA LIFETIME HOST PASS:real mono8/16/24;unpublished 1C1R local C-then-R;bounded drain/save/aliases/stale/preclaim/reentry;retained2;NO OUTPUT AUTHORITY");
    return 0;
}
