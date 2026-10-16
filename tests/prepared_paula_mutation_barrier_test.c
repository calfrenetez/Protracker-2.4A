/* Private HOST fixture: real editor, sampler masters, Paula cache jobs and
 * unpublished queue. No donor main, live opaque copies or invented receipts. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../src/editor/prepared_paula_mutation_barrier.h"
#include "../src/editor/editor.h"
#include "../src/editor/sampler_internal.h"
#include "../src/editor/sampler_paula_internal.h"
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
enum {FRAMES=513,VALUES=521,PACKED=514,RECORDS=64,SPANS=48,SERVICE_LIMIT=128};
enum {NO_HOOK,CHIP_ALLOC_REENTRY,QUEUE_ALLOC_REENTRY,CHIP_FREE_REENTRY};
struct allocation {void *data;size_t bytes;unsigned chip;};
struct workspace {void *data;size_t capacity,prefix;};
struct fixture {
    struct pt_allocator allocator;struct pt_document document;struct pt_editor *editor;
    struct pt_sampler_paula bridge;
    struct pt_sample_version *pin[3];struct pt_pcm pcm[3];
    int32_t source[3][VALUES];uint32_t markers[3][2];uint8_t gold[3][PACKED];
    struct pt_sample saved[3];uint8_t *encoded;size_t encoded_bytes;
    struct allocation allocation[RECORDS];size_t fast_bytes,chip_bytes;
    unsigned fast_calls,fast_frees,chip_calls,chip_frees,backend_calls;
    void *queue;size_t queue_bytes;unsigned queue_calls,queue_frees;
    struct workspace work[3];struct pt_prepared_paula_mutation_barrier *barrier;
    struct pt_prepared_paula_sources *sources;struct pt_prepared_paula_lifetime *lifetime;
    struct pt_prepared_paula_mutation_config config;
    unsigned hook,reentries,alias,alias_hits,steps,ended;
    /* Tail of the COMPLETE real backend/queue callback context. A returned
     * Chip interval here does not overlap its first byte or any new owner. */
    uint8_t context_tail[PACKED+64];
};
struct span_image {const void *data;size_t bytes;uint8_t *copy;};
struct image {struct span_image span[SPANS];unsigned count;};
struct counts {size_t fast,chip;unsigned fa,ff,ca,cf,qa,qf,backend;};
static struct fixture *volatile retained[6];
static unsigned retained_count,completed,cases,mutation_groups;
static uint64_t session=5000;

static struct counts counts(const struct fixture *f)
{return (struct counts){f->fast_bytes,f->chip_bytes,f->fast_calls,f->fast_frees,
    f->chip_calls,f->chip_frees,f->queue_calls,f->queue_frees,f->backend_calls};}
static void counts_equal(struct counts a,struct counts b)
{assert(a.fast==b.fast&&a.chip==b.chip&&a.fa==b.fa&&a.ff==b.ff&&a.ca==b.ca&&
    a.cf==b.cf&&a.qa==b.qa&&a.qf==b.qf&&a.backend==b.backend);}
static void image_add(struct image *a,const void *p,size_t n)
{
    struct span_image *s;
    if(!n)return;
    assert(p&&a->count<SPANS);s=a->span+a->count++;
    *s=(struct span_image){p,n,malloc(n)};assert(s->copy);memcpy(s->copy,p,n);
}
static void image_equal(struct image *a)
{
    unsigned i;for(i=0;i<a->count;++i){assert(!memcmp(a->span[i].data,a->span[i].copy,a->span[i].bytes));free(a->span[i].copy);}
    memset(a,0,sizeof(*a));
}
static void child_image(struct fixture *f,struct image *a)
{
    memset(a,0,sizeof(*a));image_add(a,f->work[1].data,f->work[1].capacity);
    image_add(a,f->work[2].data,f->work[2].capacity);
}
static void reenter(struct fixture *f)
{
    struct counts before=counts(f);struct image child;
    ++f->reentries;child_image(f,&child);
    assert(!pt_editor_prepare_change(f->editor));
    assert(pt_prepared_paula_mutation_barrier_step(f->barrier)==PT_PREPARED_PAULA_MUTATION_HELD);
    assert(pt_prepared_paula_mutation_barrier_enqueue_once(f->barrier)==PT_PREPARED_PAULA_MUTATION_HELD);
    assert(pt_prepared_paula_mutation_barrier_cancel(f->barrier)==PT_PREPARED_PAULA_MUTATION_HELD);
    assert(pt_prepared_paula_mutation_barrier_detach(f->barrier)==PT_PREPARED_PAULA_MUTATION_HELD);
    image_equal(&child);counts_equal(before,counts(f));
}
static void *allocate(struct fixture *f,size_t bytes,unsigned chip)
{
    unsigned i;void *p;
    if(chip){++f->chip_calls;if(f->alias){++f->alias_hits;
        assert(f->alias==1||f->alias==2);
        return f->alias==1?f->work[0].data:f->context_tail+32;}}
    else ++f->fast_calls;
    if((chip?f->chip_bytes:f->fast_bytes)+bytes>1024*1024)return NULL;
    p=malloc(bytes);assert(p);
    for(i=0;i<RECORDS&&f->allocation[i].data;++i){}
    assert(i<RECORDS);f->allocation[i]=(struct allocation){p,bytes,chip};
    if(chip)f->chip_bytes+=bytes;else f->fast_bytes+=bytes;
    memset(p,chip?0xa5:0xcc,bytes);
    if(chip&&f->hook==CHIP_ALLOC_REENTRY){f->hook=NO_HOOK;reenter(f);}
    return p;
}
static void release(struct fixture *f,void *p,unsigned chip)
{
    unsigned i;if(!p)return;
    for(i=0;i<RECORDS&&f->allocation[i].data!=p;++i){}
    assert(i<RECORDS&&f->allocation[i].chip==chip);
    if(chip){assert(f->chip_bytes>=f->allocation[i].bytes);f->chip_bytes-=f->allocation[i].bytes;++f->chip_frees;}
    else {assert(f->fast_bytes>=f->allocation[i].bytes);f->fast_bytes-=f->allocation[i].bytes;++f->fast_frees;}
    memset(f->allocation+i,0,sizeof(f->allocation[i]));free(p);
    if(chip&&f->hook==CHIP_FREE_REENTRY){f->hook=NO_HOOK;reenter(f);}
}
static void *fast_allocate(void *context,size_t bytes){return allocate(context,bytes,0);}
static void fast_release(void *context,void *p){release(context,p,0);}
static void *chip_allocate(void *context,size_t bytes){return allocate(context,bytes,1);}
static void chip_release(void *context,void *p,size_t bytes)
{
    struct fixture *f=context;unsigned i;
    for(i=0;i<RECORDS&&f->allocation[i].data!=p;++i){}
    assert(i<RECORDS&&f->allocation[i].bytes==bytes);release(f,p,1);
}
static void *queue_allocate(void *context,size_t bytes)
{
    struct fixture *f=context;void *p=fast_allocate(f,bytes);
    assert(!f->queue&&!f->queue_bytes);++f->queue_calls;
    if(p){f->queue=p;f->queue_bytes=bytes;}
    if(f->hook==QUEUE_ALLOC_REENTRY){f->hook=NO_HOOK;reenter(f);}
    return p;
}
static void queue_release(void *context,void *p)
{
    struct fixture *f=context;assert(p&&p==f->queue&&f->queue_bytes);
    f->queue=NULL;f->queue_bytes=0;++f->queue_frees;fast_release(f,p);
}
static int backend_clock(void *context,uint64_t *t,uint32_t *frequency)
{struct fixture *f=context;(void)t;(void)frequency;++f->backend_calls;assert(0);return 0;}
static int backend_submit(void *context,const struct pt_readers_event *e)
{struct fixture *f=context;(void)e;++f->backend_calls;assert(0);return 0;}
static enum pt_readers_reply backend_command(void *context,uint64_t ticket,struct pt_readers_command_receipt *out)
{struct fixture *f=context;(void)ticket;(void)out;++f->backend_calls;assert(0);return PT_READERS_UNCERTAIN;}
static enum pt_readers_reply backend_reader(void *context,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{struct fixture *f=context;(void)d;(void)out;++f->backend_calls;assert(0);return PT_READERS_UNCERTAIN;}
static uint8_t signed8(int32_t v,unsigned bits)
{
    int64_t n=v;unsigned shift=bits-8;
    if(shift){int64_t half=INT64_C(1)<<(shift-1);n=n>=0?(n+half)>>shift:-(((-n)+half)>>shift);}
    if(n>127)n=127;
    if(n< -128)n=-128;
    return (uint8_t)(int8_t)n;
}
static unsigned pins(const struct fixture *f)
{unsigned i,n=0;for(i=0;i<PT_CACHE_SLOTS;++i)n+=f->bridge.cache.entry[i].pins;return n;}
static void master_save_equal(struct fixture *f)
{
    size_t n,written;uint8_t *out;unsigned i;
    assert(f->editor->sampler.generation==0);
    assert(!memcmp(f->saved,f->document.project.samples,sizeof(f->saved)));
    for(i=0;i<3;++i){const struct pt_sample *s=f->document.project.samples+i;
        assert(s->pcm.bits==8+8*i&&s->pcm.channels==1&&s->pcm.capacity==FRAMES&&
            s->pcm.data==f->pcm[i].data&&s->pcm.frames==FRAMES);
        assert(!memcmp(s->pcm.data,f->source[i],FRAMES*sizeof(int32_t))&&s->slice_count==2&&
            !memcmp(s->slices,f->markers[i],sizeof(f->markers[i])));}
    assert(pt_project_size(&f->document.project,&n)==PT_PROJECT_OK&&n==f->encoded_bytes);
    out=malloc(n);assert(out);
    assert(pt_project_encode(&f->document.project,out,n,&written)==PT_PROJECT_OK&&written==n&&
        !memcmp(out,f->encoded,n));free(out);
}
static void content_image(struct fixture *f,struct image *a)
{
    struct pt_project *p=&f->document.project;unsigned i,j,n;
    struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];
    memset(a,0,sizeof(*a));image_add(a,p,sizeof(*p));
    image_add(a,p->samples,p->sample_count*sizeof(*p->samples));
    image_add(a,p->orders,p->order_count*sizeof(*p->orders));
    image_add(a,p->events,p->pattern_count*PT_PROJECT_ROWS*p->channels.count*sizeof(*p->events));
    image_add(a,f->source,sizeof(f->source));image_add(a,f->markers,sizeof(f->markers));
    for(i=0;i<3;++i){assert(pt_sampler_version_spans(f->pin[i],spans,PT_SAMPLER_VERSION_SPANS,&n));
        for(j=0;j<n;++j)image_add(a,spans[j].data,spans[j].bytes);}
    image_add(a,&f->bridge,sizeof(f->bridge));
    for(i=0;i<PT_CACHE_SLOTS;++i)if(f->bridge.cache.entry[i].data)
        image_add(a,f->bridge.cache.entry[i].data,f->bridge.cache.entry[i].bytes);
    image_add(a,f->work[1].data,f->work[1].capacity);image_add(a,f->work[2].data,f->work[2].capacity);
}
static void tails_equal(struct fixture *f)
{
    unsigned i;size_t j;
    for(i=0;i<3;++i)for(j=f->work[i].prefix;j<f->work[i].capacity;++j)
        assert(((uint8_t *)f->work[i].data)[j]==0xb6);
}
static struct fixture *make(void)
{
    struct fixture *f=calloc(1,sizeof(*f));unsigned i,j;size_t n,written;long page;
    assert(f);memset(f->context_tail,0x7c,sizeof(f->context_tail));
    f->allocator=(struct pt_allocator){f,fast_allocate,fast_release};
    pt_document_init(&f->document,&f->allocator);
    assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<3;++i){unsigned bits=8+8*i;int32_t low=-(int32_t)(UINT32_C(1)<<(bits-1));
        struct pt_sample *s=f->document.project.samples+i;
        for(j=0;j<VALUES;++j)f->source[i][j]=(int32_t)(((uint64_t)j*65357+17*i)%((UINT64_C(1)<<bits)-1))+low;
        f->source[i][0]=low;f->source[i][1]=-low-1;f->source[i][2]=-1;f->source[i][3]=1;
        f->source[i][4]=i?(int32_t)(UINT32_C(1)<<(bits-9)):1;f->source[i][5]=-f->source[i][4];
        f->markers[i][0]=16;f->markers[i][1]=256;
        s->pcm=(struct pt_pcm){f->source[i],VALUES,FRAMES,48000,1,(uint8_t)bits};
        s->slices=f->markers[i];s->slice_count=2;s->volume=48;s->name[0]=(char)('A'+i);
        for(j=0;j<FRAMES;++j)f->gold[i][j]=signed8(f->source[i][j],bits);
        f->gold[i][FRAMES]=0;
    }
    f->editor=calloc(1,sizeof(*f->editor));assert(f->editor&&pt_editor_init(f->editor,&f->document.project));
    f->editor->sampler.allocator=f->allocator;f->editor->sampler.budget=1024*1024;
    for(i=0;i<3;++i){assert(pt_sampler_pin(&f->editor->sampler,&f->document.project,i,0,f->pcm+i,f->pin+i)==PT_EDIT_OK);
        assert(f->pin[i]&&f->pin[i]==f->editor->sampler.current[i]&&f->pcm[i].data!=f->source[i]);
        if(i)assert(f->pin[i]!=f->pin[i-1]&&f->pcm[i].data!=f->pcm[i-1].data);}
    memcpy(f->saved,f->document.project.samples,sizeof(f->saved));
    assert(pt_sampler_paula_bind(&f->bridge,&f->editor->sampler,&f->document.project,f,chip_allocate,chip_release,3U*PACKED));
    assert(pt_project_size(&f->document.project,&f->encoded_bytes)==PT_PROJECT_OK);
    f->encoded=malloc(f->encoded_bytes);assert(f->encoded);
    assert(pt_project_encode(&f->document.project,f->encoded,f->encoded_bytes,&written)==PT_PROJECT_OK&&written==f->encoded_bytes);
    page=sysconf(_SC_PAGESIZE);assert(page>0);
    f->work[0].prefix=pt_prepared_paula_mutation_barrier_size();f->work[1].prefix=pt_prepared_paula_sources_size();
    f->work[2].prefix=pt_prepared_paula_lifetime_size();
    for(i=0;i<3;++i){n=((f->work[i].prefix+PACKED+256+(size_t)page-1)/(size_t)page)*(size_t)page;
        f->work[i].capacity=n;f->work[i].data=mmap(NULL,n,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        assert(f->work[i].data!=MAP_FAILED);memset(f->work[i].data,0,f->work[i].prefix);
        memset((uint8_t *)f->work[i].data+f->work[i].prefix,0xb6,n-f->work[i].prefix);}
    f->barrier=f->work[0].data;f->sources=f->work[1].data;f->lifetime=f->work[2].data;
    f->config.editor=f->editor;f->config.bridge=&f->bridge;f->config.sources=f->sources;
    f->config.source_capacity=f->work[1].capacity;f->config.lifetime=f->lifetime;
    f->config.lifetime_capacity=f->work[2].capacity;f->config.track=0;
    for(i=0;i<3;++i)f->config.sample[i]=i;
    f->config.queue.allocator=(struct pt_allocator){f,queue_allocate,queue_release};
    f->config.queue.grid=(struct pt_scheduled_grid){100,7,1000000,48000};
    f->config.queue.session=++session;f->config.queue.frame=100;f->config.queue.control_budget=1024*1024;
    f->config.queue.period=124;f->config.queue.volume=48;f->config.queue.context_count=1;
    f->config.queue.context[0]=(struct pt_prepared_paula_context_extent){f,sizeof(*f)};
    f->config.backend=(struct pt_readers_backend){f,sizeof(*f),{PT_SCHEDULED_REQUIRED,1,1},
        PT_READERS_VERSION,PT_READERS_REQUIRED,1,backend_clock,backend_submit,backend_command,backend_command,backend_reader,backend_reader};
    master_save_equal(f);return f;
}
static void begin(struct fixture *f)
{
    struct counts c=counts(f);
    assert(pt_prepared_paula_mutation_barrier_begin(f->barrier,f->work[0].capacity,&f->config)==PT_PREPARED_PAULA_MUTATION_OK);
    counts_equal(c,counts(f));assert(!f->editor->before_change&&f->editor->change_ready&&f->editor->before_change_context==f->barrier);
    /* Constructor descriptor may expire; its pointees/contexts remain live. */
    memset(&f->config,0,sizeof(f->config));
    assert(pt_prepared_paula_mutation_barrier_state(f->barrier)==PT_PREPARED_PAULA_MUTATION_ATTACHED);
}
static enum pt_prepared_paula_mutation_result step(struct fixture *f)
{
    enum pt_prepared_paula_mutation_result r;
    assert(++f->steps<=SERVICE_LIMIT);r=pt_prepared_paula_mutation_barrier_step(f->barrier);
    assert(!f->backend_calls);return r;
}
static void blocked_mutations(struct fixture *f)
{
    struct image a;struct counts c;unsigned revision=f->editor->history.revision,generation=f->editor->sampler.generation;
    struct pt_editor *e=f->editor;
    assert(!e->number_field&&!e->name_entry&&!e->workflow.transaction);
    e->panel=0;e->editing=1;e->field=0;e->octave=1;
    e->pattern=e->row=0;e->project->channels.selected=0;
    content_image(f,&a);c=counts(f);
    assert(!pt_editor_prepare_change(e));
    assert(!strcmp(e->status,"PLAYBACK STOP UNCONFIRMED - RETRY BEFORE CHANGING PROJECT"));
    assert(pt_editor_key(e,0x31,0)==PT_UI_NONE);
    assert(!strcmp(e->status,"INVALID EVENT - NO CHANGE"));
    assert(pt_editor_key(e,0x31,8)==PT_UI_NONE);
    assert(!strcmp(e->status,"UNDO CONFLICT - EDIT PRESERVED"));
    e->panel=5;e->sample=1;pt_editor_sample_all(e);
    assert(pt_editor_key(e,0x4c,0x10)==PT_UI_NONE);
    assert(!strcmp(e->status,"SAMPLE EDIT REFUSED - NO CHANGE"));
    assert(pt_editor_key(e,0x0c,9)==PT_UI_NONE);
    assert(!strcmp(e->status,"SLOT ADD REFUSED - NO CHANGE"));
    assert(!pt_editor_dispose(e));
    assert(!strcmp(e->status,"PLAYBACK STOP UNCONFIRMED - RETRY BEFORE CHANGING PROJECT"));
    assert(e->history.revision==revision&&e->sampler.generation==generation);
    image_equal(&a);counts_equal(c,counts(f));master_save_equal(f);tails_equal(f);++mutation_groups;
}
static void cache_gold(struct fixture *f)
{
    unsigned i,j,seen=0;
    for(i=0;i<PT_CACHE_SLOTS;++i){const struct pt_cache_entry *e=f->bridge.cache.entry+i;
        if(!e->data)continue;
        assert(e->bytes==PACKED&&e->valid==1&&e->pins==1&&e->key&&e->serial);
        for(j=0;j<3&&memcmp(e->data,f->gold[j],PACKED);++j){}
        assert(j<3&&!(seen&(1U<<j)));seen|=1U<<j;
    }
    assert(seen==7&&pins(f)==3&&f->chip_bytes==3*PACKED);
}
static void finish(struct fixture *f,unsigned active_barrier)
{
    unsigned i;struct counts c;enum pt_prepared_paula_mutation_result r;
    if(active_barrier){
        for(;;){r=step(f);assert(r!=PT_PREPARED_PAULA_MUTATION_HELD);
            if(r==PT_PREPARED_PAULA_MUTATION_CLEANED)break;
            assert(r==PT_PREPARED_PAULA_MUTATION_PENDING);master_save_equal(f);}
        assert(!f->chip_bytes&&!pins(f)&&!f->queue&&f->queue_calls==f->queue_frees);
        blocked_mutations(f);c=counts(f);
        assert(pt_prepared_paula_mutation_barrier_detach(f->barrier)==PT_PREPARED_PAULA_MUTATION_CLOSED);
        counts_equal(c,counts(f));assert(!f->editor->before_change&&!f->editor->change_ready&&!f->editor->before_change_context);
        assert(!mprotect(f->work[1].data,f->work[1].capacity,PROT_NONE));
        assert(!mprotect(f->work[2].data,f->work[2].capacity,PROT_NONE));
        assert(pt_prepared_paula_mutation_barrier_state(f->barrier)==PT_PREPARED_PAULA_MUTATION_FINISHED);
        assert(pt_prepared_paula_mutation_barrier_failure(f->barrier)==PT_PREPARED_PAULA_SOURCES_OK);
        assert(pt_prepared_paula_mutation_barrier_detach(f->barrier)==PT_PREPARED_PAULA_MUTATION_CLOSED);
        assert(!mprotect(f->work[0].data,f->work[0].capacity,PROT_NONE));
    }else {assert(pt_sampler_paula_close(&f->bridge));}
    assert(pt_editor_prepare_change(f->editor));
    f->editor->panel=0;f->editor->editing=1;f->editor->row=f->editor->pattern=0;
    {unsigned revision=f->editor->history.revision;pt_editor_key(f->editor,0x31,0);
        assert(f->editor->history.revision!=revision&&f->document.project.events[0].kind==PT_NOTE_PERIOD);}
    assert(pt_editor_dispose(f->editor));
    for(i=0;i<3;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    assert(!f->editor->sampler.bytes&&!f->editor->sampler.live_count&&!f->editor->sampler.pending_count);
    pt_document_release(&f->document);assert(!f->fast_bytes&&!f->chip_bytes&&!f->backend_calls);
    for(i=0;i<RECORDS;++i)assert(!f->allocation[i].data);
    for(i=0;i<3;++i)assert(!munmap(f->work[i].data,f->work[i].capacity));
    free(f->encoded);free(f->editor);free(f);++completed;
}
enum cut {CUT_ATTACHED,CUT_PIN,CUT_CHIP,CUT_SOURCES_READY,CUT_EMPTY,CUT_QUEUED};
static void cancel_cut(enum cut cut)
{
    struct fixture *f=make();unsigned reached=cut==CUT_ATTACHED;
    begin(f);
    if(!reached){
        /* Exact successful constructor call before observing source metadata.
         * No observation of an alleged/uninitialized opaque owner. */
        assert(step(f)==PT_PREPARED_PAULA_MUTATION_PENDING);
        assert(pt_prepared_paula_mutation_barrier_state(f->barrier)==PT_PREPARED_PAULA_MUTATION_PREPARING);
    }
    while(!reached){enum pt_prepared_paula_mutation_result r=step(f);
        assert(r==PT_PREPARED_PAULA_MUTATION_PENDING||r==PT_PREPARED_PAULA_MUTATION_READY);
        if(cut==CUT_PIN)reached=f->editor->sampler.pending_count!=0;
        if(cut==CUT_CHIP)reached=f->chip_calls!=0;
        if(cut==CUT_SOURCES_READY){struct pt_prepared_paula_sources_status status;
            assert(!f->queue&&!f->queue_calls);
            assert(pt_prepared_paula_sources_get(f->sources,&status)==PT_PREPARED_PAULA_SOURCES_OK);
            reached=status.phase==PT_PREPARED_PAULA_SOURCES_PREPARED;
            if(reached){assert(status.failure==PT_PREPARED_PAULA_SOURCES_OK&&status.completed==3&&
                status.master_pins==3&&status.chip_leases==3&&!status.master_job&&!status.chip_job);
                cache_gold(f);}}
        if(cut==CUT_EMPTY||cut==CUT_QUEUED)reached=r==PT_PREPARED_PAULA_MUTATION_READY;
        master_save_equal(f);
    }
    if(cut==CUT_EMPTY||cut==CUT_QUEUED)cache_gold(f);
    if(cut==CUT_QUEUED){struct counts c=counts(f);
        assert(pt_prepared_paula_mutation_barrier_enqueue_once(f->barrier)==PT_PREPARED_PAULA_MUTATION_QUEUED);
        counts_equal(c,counts(f));assert(pins(f)==3&&f->queue_calls==1);}
    blocked_mutations(f);finish(f,1);++cases;
}
static void foreign_guard(void *context){unsigned *n=context;++*n;}
static int foreign_ready(void *context){unsigned *n=context;++*n;return 1;}
static int recording_busy(void *context){unsigned *n=context;++*n;return 1;}
static void admission_refuses(struct fixture *f,struct pt_prepared_paula_mutation_config *cfg,
    void *storage,size_t capacity)
{
    struct image a;struct counts c=counts(f);unsigned i;uint8_t *editor=malloc(sizeof(*f->editor));
    assert(editor);memcpy(editor,f->editor,sizeof(*f->editor));memset(&a,0,sizeof(a));
    for(i=0;i<3;++i)image_add(&a,f->work[i].data,f->work[i].capacity);
    assert(pt_prepared_paula_mutation_barrier_begin(storage,capacity,cfg)!=PT_PREPARED_PAULA_MUTATION_OK);
    image_equal(&a);counts_equal(c,counts(f));assert(!memcmp(editor,f->editor,sizeof(*f->editor)));free(editor);
    master_save_equal(f);
}
static void admission_case(void)
{
    struct fixture *f=make();struct pt_prepared_paula_mutation_config cfg=f->config;unsigned calls=0;
    admission_refuses(f,&cfg,NULL,f->work[0].capacity);
    admission_refuses(f,&cfg,(uint8_t *)f->work[0].data+1,f->work[0].capacity-1);
    admission_refuses(f,&cfg,f->work[0].data,SIZE_MAX);
    cfg.sources=f->work[0].data;admission_refuses(f,&cfg,f->barrier,f->work[0].capacity);cfg=f->config;
    cfg.sample[2]=cfg.sample[1];admission_refuses(f,&cfg,f->barrier,f->work[0].capacity);cfg=f->config;
    f->bridge.project=NULL;admission_refuses(f,&cfg,f->barrier,f->work[0].capacity);f->bridge.project=&f->document.project;
    pt_editor_change_guard(f->editor,foreign_guard,&calls);admission_refuses(f,&cfg,f->barrier,f->work[0].capacity);
    assert(!calls);pt_editor_change_guard(f->editor,NULL,NULL);
    assert(pt_editor_change_barrier(f->editor,foreign_ready,&calls));admission_refuses(f,&cfg,f->barrier,f->work[0].capacity);
    assert(!calls&&pt_editor_change_barrier(f->editor,NULL,NULL));
    f->editor->playback.active=1;admission_refuses(f,&cfg,f->barrier,f->work[0].capacity);f->editor->playback.active=0;
    f->editor->workflow.busy=1;admission_refuses(f,&cfg,f->barrier,f->work[0].capacity);f->editor->workflow.busy=0;
    f->editor->workflow.recording_busy=recording_busy;f->editor->workflow.recording_context=&calls;
    admission_refuses(f,&cfg,f->barrier,f->work[0].capacity);assert(!calls);
    f->editor->workflow.recording_busy=NULL;admission_refuses(f,&cfg,f->barrier,f->work[0].capacity);
    f->editor->workflow.recording_context=NULL;finish(f,0);++cases;
}
static void retain(struct fixture *f)
{
    struct counts before=counts(f);
    assert(retained_count<6&&f->reentries<=1&&pt_prepared_paula_mutation_barrier_state(f->barrier)==PT_PREPARED_PAULA_MUTATION_UNCERTAIN);
    assert(pt_prepared_paula_mutation_barrier_step(f->barrier)==PT_PREPARED_PAULA_MUTATION_HELD);
    assert(pt_prepared_paula_mutation_barrier_detach(f->barrier)==PT_PREPARED_PAULA_MUTATION_HELD);
    counts_equal(before,counts(f));assert(!f->backend_calls&&f->editor->sampler.live_count==3);
    master_save_equal(f);retained[retained_count++]=f;++cases;
}
static void reentry_case(unsigned hook)
{
    struct fixture *f=make();enum pt_prepared_paula_mutation_result r;
    begin(f);f->hook=hook;
    if(hook==CHIP_FREE_REENTRY){
        do {r=step(f);assert(r==PT_PREPARED_PAULA_MUTATION_PENDING||r==PT_PREPARED_PAULA_MUTATION_READY);}while(r!=PT_PREPARED_PAULA_MUTATION_READY);
        assert(pt_prepared_paula_mutation_barrier_enqueue_once(f->barrier)==PT_PREPARED_PAULA_MUTATION_QUEUED);
        assert(pt_prepared_paula_mutation_barrier_cancel(f->barrier)==PT_PREPARED_PAULA_MUTATION_PENDING);
    }
    do {r=step(f);assert(r!=PT_PREPARED_PAULA_MUTATION_CLEANED);}while(r!=PT_PREPARED_PAULA_MUTATION_HELD);
    assert(f->reentries==1);if(hook==CHIP_FREE_REENTRY)assert(f->chip_frees>=1);
    retain(f);
}
static void alias_case(unsigned mode)
{
    struct fixture *f=make();enum pt_prepared_paula_mutation_result r;uint8_t before[sizeof(f->context_tail)];
    memcpy(before,f->context_tail,sizeof(before));begin(f);f->alias=mode;
    do {r=step(f);}while(r==PT_PREPARED_PAULA_MUTATION_PENDING);
    assert(r==PT_PREPARED_PAULA_MUTATION_HELD&&f->alias_hits==1&&!f->chip_frees);
    assert(!memcmp(before,f->context_tail,sizeof(before)));
    tails_equal(f);retain(f);
    assert(!memcmp(before,f->context_tail,sizeof(before))&&!f->chip_frees);
}
static void stolen_hook_case(void)
{
    struct fixture *f=make();enum pt_prepared_paula_mutation_result r;
    begin(f);do {r=step(f);}while(r==PT_PREPARED_PAULA_MUTATION_PENDING);
    assert(r==PT_PREPARED_PAULA_MUTATION_READY);
    /* Deliberate forbidden hook replacement: test module refusal only. Never
     * use that foreign hook to mutate, and never repair it for cleanup. */
    f->editor->change_ready=foreign_ready;f->editor->before_change_context=&f->ended;
    assert(pt_prepared_paula_mutation_barrier_step(f->barrier)==PT_PREPARED_PAULA_MUTATION_HELD);
    assert(f->editor->change_ready==foreign_ready&&f->editor->before_change_context==&f->ended&&!f->ended);
    retain(f);
}
int main(void)
{
    unsigned i,j;
    admission_case();for(i=CUT_ATTACHED;i<=CUT_QUEUED;++i)cancel_cut((enum cut)i);
    reentry_case(CHIP_ALLOC_REENTRY);reentry_case(QUEUE_ALLOC_REENTRY);reentry_case(CHIP_FREE_REENTRY);
    alias_case(1);alias_case(2);stolen_hook_case();
    assert(cases==13&&completed==7&&retained_count==6&&mutation_groups>=12);
    for(i=0;i<6;++i){assert(retained[i]);for(j=0;j<i;++j)assert(retained[i]!=retained[j]);}
    puts("PREPARED PAULA MUTATION HOST PASS:13 cases;7 completed;6 retained;real editor mono8/16/24;exclusive veto;deferred actual source/queue/cache cleanup;separate detach;project/master/save preserved;NO OUTPUT OR NATIVE AUTHORITY");
    return 0;
}
