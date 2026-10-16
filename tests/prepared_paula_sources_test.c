/* HOST-only integration: real immutable sampler pins and real Paula jobs.
 * No private holder layout, invented reference counters, source token export,
 * native output, device, reader or quiet authority is used. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../src/editor/prepared_paula_sources.h"
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
    struct arena arena[2];
    unsigned arena_count;
    uint8_t *encoded;size_t encoded_bytes;
    struct pt_sample *sample_table;
};
struct span_before {const void *data;size_t bytes;unsigned char *copy;};
struct audit {struct span_before span[MAX_SPANS];unsigned count;};
/* A forbidden callback may create uncertain ownership. These two complete
 * heap fixtures, their allocators and all genuine remaining resources stay
 * reachable until process exit. They are never forcibly closed or repaired. */
static struct fixture *volatile retained[2];
static unsigned retained_count,alias_cases,cancel_cases,stale_cases;

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
        a->capacity=prefix+256;a->data=malloc(a->capacity);assert(a->data);
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
static void cancellation_at_every_transition(void)
{
    unsigned cut;
    for(cut=0;cut<=24;++cut){struct fixture *f=setup(3*PACKED,0);
        owner_begin(f);advance(f,cut);cancel_all(f,1);finish(f);++cancel_cases;}
    assert(cancel_cases==25);
}
static void ready_and_real_hits(void)
{
    struct fixture *f=setup(3*PACKED,0);struct audit a;unsigned i,fast,chip;
    owner_begin(f);advance(f,24);audit_capture(f,&a,1);
    assert(pt_prepared_paula_sources_step(f->owner)==PT_PREPARED_PAULA_SOURCES_READY);
    audit_equal_release(&a);cancel_all(f,1);assert(!pins(f)&&f->chip_bytes==3*PACKED);
    new_arena(f,0);owner_begin(f);fast=f->fast_calls;chip=f->chip_calls;
    for(i=1;i<=9;++i){
        unsigned done=i/3,part=i%3;struct pt_prepared_paula_sources_status s;
        assert(pt_prepared_paula_sources_step(f->owner)==(i==9?PT_PREPARED_PAULA_SOURCES_READY:PT_PREPARED_PAULA_SOURCES_PENDING));
        s=status(f);assert(s.completed==done&&s.master_pins==done+(part==2));
        assert(s.chip_leases==done&&s.master_job==(part==1)&&!s.chip_job);
        assert(f->sampler.pending_count==(size_t)(part==1)&&pins(f)==done);
        assert(f->fast_calls==fast&&f->chip_calls==chip&&f->chip_bytes==3*PACKED);
        master_save_equal(f);
    }
    cancel_all(f,1);finish(f);
}
static void own_output_refusals(struct fixture *f)
{
    struct arena *a=f->arena+f->arena_count-1;size_t off,align=ALIGNMENT(struct pt_prepared_paula_sources_status);
    unsigned char *saved=malloc(a->capacity);unsigned fast=f->fast_calls,chip=f->chip_calls;
    assert(saved&&!a->mapping);memcpy(saved,a->data,a->capacity);
    for(off=0;off+sizeof(struct pt_prepared_paula_sources_status)<=a->capacity;off+=align){
        struct pt_prepared_paula_sources_status *out=(void *)(a->data+off);
        assert(pt_prepared_paula_sources_get(f->owner,out)==PT_PREPARED_PAULA_SOURCES_ALIAS);
        assert(!memcmp(saved,a->data,a->capacity));++alias_cases;
    }
    assert(f->fast_calls==fast&&f->chip_calls==chip);free(saved);
}
static void alias_and_numeric_guards(void)
{
    struct fixture *f=setup(3*PACKED,0);struct audit a;unsigned i;size_t align;
    const void *out[12];struct pt_prepared_paula_sources_status status_out,before;
    owner_begin(f);advance(f,24);own_output_refusals(f);
    out[0]=&f->sampler;out[1]=&f->bridge;out[2]=&f->document.project;
    out[3]=f->sample_table;out[4]=f->document.project.events;out[5]=f->unused+32;
    out[6]=f->pcm[0].data;out[7]=f->pcm[1].data;out[8]=f->pcm[2].data;
    out[9]=f->pin[0];out[10]=f->bridge.cache.entry[0].data;out[11]=f->bridge.cache.entry[2].data;
    for(i=0;i<12;++i){
        assert((uintptr_t)out[i]%ALIGNMENT(struct pt_prepared_paula_sources_status)==0);
        audit_capture(f,&a,1);
        assert(pt_prepared_paula_sources_get(f->owner,(void *)out[i])==PT_PREPARED_PAULA_SOURCES_ALIAS);
        audit_equal_release(&a);++alias_cases;
    }
    memset(&status_out,0x6a,sizeof(status_out));memcpy(&before,&status_out,sizeof(before));
    align=ALIGNMENT(struct pt_prepared_paula_sources_status);
    audit_capture(f,&a,1);
    assert(pt_prepared_paula_sources_get(f->owner,NULL)==PT_PREPARED_PAULA_SOURCES_ALIAS);
    assert(pt_prepared_paula_sources_get(f->owner,(void *)((unsigned char *)&status_out+1))==PT_PREPARED_PAULA_SOURCES_ALIAS);
    assert(pt_prepared_paula_sources_get(f->owner,(void *)(UINTPTR_MAX-(UINTPTR_MAX%align)))==PT_PREPARED_PAULA_SOURCES_ALIAS);
    audit_equal_release(&a);assert(!memcmp(&status_out,&before,sizeof(before)));
    /* A stale generation or descriptor must not turn an own-output refusal
     * into a write to the overlapped owner/failure/busy bytes. No owner repair. */
    ++f->sampler.generation;own_output_refusals(f);--f->sampler.generation;
    ++f->sample_table[0].pcm.rate;own_output_refusals(f);--f->sample_table[0].pcm.rate;
    assert(status(f).failure==PT_PREPARED_PAULA_SOURCES_OK);
    cancel_all(f,1);own_output_refusals(f);finish(f);
}
static void constructor_guards_and_protected_tail(void)
{
    struct fixture *f=setup(3*PACKED,0);struct arena *a=f->arena;struct audit audit;
    size_t size=pt_prepared_paula_sources_size(),align=pt_prepared_paula_sources_alignment(),bad[3];
    unsigned sample[3]={0,1,2},i,fast=f->fast_calls,chip=f->chip_calls;
    unsigned fr=f->fast_releases,cr=f->chip_releases;
    assert(size&&align&&size<=a->capacity);
    bad[0]=0;bad[1]=size/2;bad[2]=size-1;
    for(i=0;i<3;++i){a->data[bad[i]]=1;audit_capture(f,&audit,1);
        assert(pt_prepared_paula_sources_init(f->owner,a->capacity,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
        audit_equal_release(&audit);a->data[bad[i]]=0;}
    audit_capture(f,&audit,1);
    assert(pt_prepared_paula_sources_init(NULL,a->capacity,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(pt_prepared_paula_sources_init((void *)(a->data+1),a->capacity,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(pt_prepared_paula_sources_init(f->owner,size-1,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(pt_prepared_paula_sources_init(f->owner,SIZE_MAX,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(pt_prepared_paula_sources_init((void *)(UINTPTR_MAX-(UINTPTR_MAX%align)),a->capacity,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(pt_prepared_paula_sources_init(f->owner,a->capacity,NULL,0,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(pt_prepared_paula_sources_init(f->owner,a->capacity,&f->bridge,0,NULL)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(pt_prepared_paula_sources_init(f->owner,a->capacity,&f->bridge,4,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
    assert(pt_prepared_paula_sources_init((void *)&f->bridge,a->capacity,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_ALIAS);
    assert(pt_prepared_paula_sources_init(f->owner,a->capacity,&f->bridge,0,(void *)a->data)==PT_PREPARED_PAULA_SOURCES_ALIAS);
    sample[1]=0;
    assert(pt_prepared_paula_sources_init(f->owner,a->capacity,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
    sample[1]=1;sample[2]=PT_PROJECT_SAMPLES;
    assert(pt_prepared_paula_sources_init(f->owner,a->capacity,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_STALE);
    sample[2]=3;
    assert(pt_prepared_paula_sources_init(f->owner,a->capacity,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_STALE);
    audit_equal_release(&audit);
    for(i=0;i<3;++i){struct pt_sample saved=f->sample_table[0];
        if(i==0)f->sample_table[0].pcm.channels=2;
        if(i==1)f->sample_table[0].pcm.frames=0;
        if(i==2)f->sample_table[0].pcm.bits=32;
        sample[2]=2;audit_capture(f,&audit,1);
        assert(pt_prepared_paula_sources_init(f->owner,a->capacity,&f->bridge,0,sample)==PT_PREPARED_PAULA_SOURCES_INVALID);
        audit_equal_release(&audit);memcpy(f->sample_table,&saved,sizeof(saved));
    }
    assert(f->fast_calls==fast&&f->chip_calls==chip&&f->fast_releases==fr&&f->chip_releases==cr);
    owner_begin(f);cancel_all(f,1);finish(f);
    /* The last entire page belongs to capacity but is inaccessible. Prefix
     * init/step/get/cancel must never read or write that declared tail. */
    f=setup(3*PACKED,1);owner_begin(f);advance(f,24);cancel_all(f,1);finish(f);
    /* A read-only complete owner detects even transient busy writes that a
     * before/after comparison could miss. Its output aliases must be refused
     * before any owner byte changes, including when borrowed state is stale. */
    f=setup(3*PACKED,2);a=f->arena;owner_begin(f);
    {unsigned mode,j;size_t offsets[3],alignment=ALIGNMENT(struct pt_prepared_paula_sources_status);
     unsigned char *copy=malloc(a->capacity);assert(copy);memcpy(copy,a->data,a->capacity);
     offsets[0]=0;offsets[1]=(size/2)/alignment*alignment;
     offsets[2]=(a->capacity-sizeof(struct pt_prepared_paula_sources_status))/alignment*alignment;
     for(mode=0;mode<3;++mode){
         if(mode==1)++f->sampler.generation;
         if(mode==2)++f->sample_table[0].pcm.rate;
         assert(!mprotect(a->data,a->mapping,PROT_READ));
         for(j=0;j<3;++j){
             assert(pt_prepared_paula_sources_get(f->owner,(void *)(a->data+offsets[j]))==PT_PREPARED_PAULA_SOURCES_ALIAS);
             assert(!memcmp(copy,a->data,a->capacity));++alias_cases;
         }
         assert(!mprotect(a->data,a->mapping,PROT_READ|PROT_WRITE));
         if(mode==1)--f->sampler.generation;
         if(mode==2)--f->sample_table[0].pcm.rate;
     }free(copy);}
    assert(status(f).failure==PT_PREPARED_PAULA_SOURCES_OK);cancel_all(f,1);finish(f);
}
static void stale_then_owned_cleanup(void)
{
    static const unsigned cut[]={0,1,2,3,4,8,9,16,24};unsigned mode,k;
    for(mode=0;mode<4;++mode)for(k=0;k<sizeof(cut)/sizeof(cut[0]);++k){
        struct fixture *f=setup(3*PACKED,0);struct audit audit;
        struct pt_prepared_paula_sources_status out,saved;void *guard=NULL;size_t page=0;
        owner_begin(f);advance(f,cut[k]);
        if(mode==0)++f->sampler.generation;
        if(mode==1)f->document.project.channels.track[0].route=PT_AMIGUS;
        if(mode==2)++f->sample_table[0].volume;
        if(mode==3){long n=sysconf(_SC_PAGESIZE);assert(n>0);page=(size_t)n;
            guard=mmap(NULL,page,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(guard!=MAP_FAILED);
            f->document.project.samples=guard;}
        audit_capture(f,&audit,0);
        assert(pt_prepared_paula_sources_step(f->owner)==PT_PREPARED_PAULA_SOURCES_STALE);
        audit_equal_release(&audit);
        memset(&out,0x41,sizeof(out));memcpy(&saved,&out,sizeof(saved));
        assert(pt_prepared_paula_sources_get(f->owner,&out)==PT_PREPARED_PAULA_SOURCES_STALE);
        assert(!memcmp(&out,&saved,sizeof(out)));
        {struct arena *a=f->arena;unsigned char *copy=malloc(a->capacity);assert(copy);memcpy(copy,a->data,a->capacity);
         assert(pt_prepared_paula_sources_step(f->owner)==PT_PREPARED_PAULA_SOURCES_STALE);
         assert(!memcmp(copy,a->data,a->capacity));free(copy);}
        /* No stale table/generation/route/descriptor is restored to permit a
         * retry. Cancellation must use only captured genuine child owners. */
        cancel_all(f,0);
        assert(status(f).failure==PT_PREPARED_PAULA_SOURCES_STALE);
        if(mode==0)--f->sampler.generation;
        if(mode==1)f->document.project.channels.track[0].route=PT_PAULA;
        if(mode==2)--f->sample_table[0].volume;
        if(mode==3){f->document.project.samples=f->sample_table;assert(!munmap(guard,page));}
        finish(f);++stale_cases;
    }
    assert(stale_cases==36);
}
static void budget_and_allocation_failure(void)
{
    unsigned mode;
    for(mode=0;mode<2;++mode){struct fixture *f=setup(mode?3*PACKED:PACKED-1,0);
        struct pt_prepared_paula_sources_status s;unsigned calls;
        owner_begin(f);advance(f,2);calls=f->chip_calls;f->chip_fail=mode;
        assert(pt_prepared_paula_sources_step(f->owner)==PT_PREPARED_PAULA_SOURCES_CAPACITY);
        assert(f->chip_calls==calls+mode&&!f->chip_bytes&&!pins(f));
        s=status(f);assert(s.master_pins==1&&!s.master_job&&!s.chip_job&&!s.chip_leases&&s.failure==PT_PREPARED_PAULA_SOURCES_CAPACITY);
        assert(pt_prepared_paula_sources_step(f->owner)==PT_PREPARED_PAULA_SOURCES_CAPACITY);
        f->chip_fail=0;cancel_all(f,1);assert(status(f).failure==PT_PREPARED_PAULA_SOURCES_CAPACITY);finish(f);
    }
}
static void stop_and_reentry_retention(void)
{
    struct fixture *f=setup(3*PACKED,0);unsigned i;
    owner_begin(f);advance(f,24);
    assert(!pt_sampler_paula_close(&f->bridge)&&pins(f)==3&&f->chip_bytes==3*PACKED);
    assert(pt_prepared_paula_sources_step(f->owner)==PT_PREPARED_PAULA_SOURCES_STALE);
    cancel_all(f,0);assert(status(f).failure==PT_PREPARED_PAULA_SOURCES_STALE);finish(f);
    for(i=0;i<2;++i){
        unsigned pinned,chip,fr,cr,fast,allocations;size_t bytes;
        struct pt_prepared_paula_sources_status out,saved;
        f=setup(3*PACKED,0);owner_begin(f);
        if(!i){advance(f,2);f->hook=HOOK_ALLOC_GET;
            assert(pt_prepared_paula_sources_step(f->owner)==PT_PREPARED_PAULA_SOURCES_FAULT);
            assert(pins(f)==1&&f->chip_bytes==PACKED&&f->bridge.cache.entry[0].valid==0);
        }else{advance(f,24);assert(!pt_sampler_paula_close(&f->bridge));f->hook=HOOK_RELEASE_CANCEL;
            assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_FAULT);
            assert(pins(f)==2&&f->chip_bytes==2*PACKED&&f->chip_releases==1);
        }
        assert(f->hook_calls==1&&!f->sampler.pending_count&&f->sampler.live_count==3);
        pinned=pins(f);bytes=f->chip_bytes;chip=f->chip_calls;fast=f->fast_calls;
        fr=f->fast_releases;cr=f->chip_releases;allocations=0;
        {unsigned n;for(n=0;n<MAX_ALLOCATIONS;++n)allocations+=f->allocation[n].data!=NULL;}
        assert(allocations>=6&&f->sampler.bytes&&f->fast_bytes);
        memset(&out,0x75,sizeof(out));memcpy(&saved,&out,sizeof(saved));
        assert(pt_prepared_paula_sources_get(f->owner,&out)==PT_PREPARED_PAULA_SOURCES_FAULT);
        assert(!memcmp(&out,&saved,sizeof(out)));
        assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_FAULT);
        assert(pt_prepared_paula_sources_step(f->owner)==PT_PREPARED_PAULA_SOURCES_FAULT);
        assert(pins(f)==pinned&&f->chip_bytes==bytes&&f->chip_calls==chip&&f->fast_calls==fast&&f->fast_releases==fr&&f->chip_releases==cr);
        master_save_equal(f);assert(retained_count<2);retained[retained_count++]=f;
    }
    assert(retained_count==2&&retained[0]!=retained[1]);
}
int main(void)
{
    cancellation_at_every_transition();ready_and_real_hits();alias_and_numeric_guards();
    constructor_guards_and_protected_tail();stale_then_owned_cleanup();
    budget_and_allocation_failure();stop_and_reentry_retention();
    assert(alias_cases>100&&cancel_cases==25&&stale_cases==36&&retained_count==2);
    puts("PREPARED PAULA SOURCES HOST PASS:real mono8/16/24 pins/cache;load24/hit9/cancel25;save/alias/stale36/budget/retained2;NO OUTPUT AUTHORITY");
    return 0;
}
