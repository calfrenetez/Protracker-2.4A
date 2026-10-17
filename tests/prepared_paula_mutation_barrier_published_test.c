/* Standalone HOST model: real editor plus authentic published C/R custody.
 * No donor include/main, opaque owner copy, manufactured pin or quiet flag. */
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

enum { FRAMES=513, VALUES=521, PACKED=514, RECORDS=64, SPANS=64, SERVICE_LIMIT=128 };
enum mode { EXACT, BAD_CLASS, PENDING, WRONG_ENVELOPE, REENTER, UNCERTAIN };
struct allocation { void *data; size_t bytes; unsigned chip; };
struct workspace { void *data; size_t capacity,prefix; };
struct command_identity {
    const struct pt_readers_output *queue;
    const struct pt_readers_event *event;
    struct pt_readers_binding binding;
    struct pt_readers_key key;
    uint64_t session,generation,ticket,owner;
    unsigned count;
};
struct reader_identity {
    const struct pt_readers_domain *reference;
    struct pt_readers_binding binding;
    struct pt_readers_key key;
};
struct backend {
    uint64_t now,first,last,frame,observed,issued;
    uint32_t frequency,source_generation;
    unsigned depth,command_live,reader_live,activation_pending,adopted,active;
    const struct pt_readers_event *activation_event;
    const struct pt_readers_domain *reader_reference;
    struct command_identity command;
    struct reader_identity reader;
    struct pt_scheduled_action action;
    struct pt_scheduled_span spans[PT_PREPARED_PAULA_LIFETIME_DATA_SPANS];
    unsigned span_count,clocks,submits,cancel_c,cancel_r,polls,drops_c,drops_r;
    unsigned dispatches,reads,read_bytes,reentries,refuse_submit;
    enum mode command_mode,reader_mode;
};
struct fixture {
    struct pt_allocator allocator; struct pt_document document; struct pt_editor *editor;
    struct pt_sampler_paula bridge;
    struct pt_sample_version *pin[3]; struct pt_pcm pcm[3];
    int32_t source[3][VALUES]; uint32_t markers[3][2]; uint8_t gold[3][PACKED];
    struct pt_sample saved[3]; uint8_t *encoded; size_t encoded_bytes;
    struct allocation allocation[RECORDS]; size_t fast_bytes,chip_bytes;
    unsigned fast_calls,fast_frees,chip_calls,chip_frees;
    void *queue; size_t queue_bytes; unsigned queue_calls,queue_frees;
    struct workspace work[3]; struct pt_prepared_paula_mutation_barrier *barrier;
    struct pt_prepared_paula_sources *sources; struct pt_prepared_paula_lifetime *lifetime;
    struct pt_prepared_paula_mutation_config config;
    uint64_t session; unsigned expected_generation,steps;
    struct backend backend;
    uint8_t context_tail[PACKED+64];
};
struct span_image { const void *data; size_t bytes; uint8_t *copy; };
struct image { struct span_image span[SPANS]; unsigned count; };
struct counts { size_t fast,chip; unsigned fa,ff,ca,cf,qa,qf,callbacks; };
static struct fixture *volatile published_retained[5], *volatile adopted_retained[1];
static unsigned published_cases,published_completed,published_held;
static unsigned adopted_cases,adopted_completed,adopted_held,mutation_groups;
static uint64_t next_session=9000;

static struct counts counts(const struct fixture *f)
{
    const struct backend *b=&f->backend;
    return (struct counts){f->fast_bytes,f->chip_bytes,f->fast_calls,f->fast_frees,
        f->chip_calls,f->chip_frees,f->queue_calls,f->queue_frees,
        b->clocks+b->submits+b->cancel_c+b->cancel_r+b->polls};
}
static void resource_equal(struct counts a,struct counts b)
{ assert(a.fast==b.fast&&a.chip==b.chip&&a.fa==b.fa&&a.ff==b.ff&&a.ca==b.ca&&a.cf==b.cf&&a.qa==b.qa&&a.qf==b.qf); }
static void counts_equal(struct counts a,struct counts b)
{ resource_equal(a,b); assert(a.callbacks==b.callbacks); }
static void image_add(struct image *a,const void *p,size_t n)
{
    struct span_image *s; if(!n)return;
    assert(p&&a->count<SPANS);s=a->span+a->count++;
    *s=(struct span_image){p,n,malloc(n)};assert(s->copy);memcpy(s->copy,p,n);
}
static void image_equal(struct image *a)
{
    unsigned i;for(i=0;i<a->count;++i){assert(!memcmp(a->span[i].data,a->span[i].copy,a->span[i].bytes));free(a->span[i].copy);}
    memset(a,0,sizeof(*a));
}
static void *allocate(struct fixture *f,size_t bytes,unsigned chip)
{
    unsigned i;const size_t budget=1024U*1024U;
    size_t used=chip?f->chip_bytes:f->fast_bytes;void *p;
    if(used>budget||bytes>budget-used)return NULL;
    p=malloc(bytes);assert(p&&bytes);
    for(i=0;i<RECORDS&&f->allocation[i].data;++i){}
    assert(i<RECORDS);f->allocation[i]=(struct allocation){p,bytes,chip};
    if(chip){f->chip_bytes+=bytes;++f->chip_calls;}else{f->fast_bytes+=bytes;++f->fast_calls;}
    return p;
}
static void release(struct fixture *f,void *p,unsigned chip)
{
    unsigned i;for(i=0;i<RECORDS&&f->allocation[i].data!=p;++i){}
    assert(p&&i<RECORDS&&f->allocation[i].chip==chip);
    if(chip){assert(f->chip_bytes>=f->allocation[i].bytes);f->chip_bytes-=f->allocation[i].bytes;++f->chip_frees;}
    else{assert(f->fast_bytes>=f->allocation[i].bytes);f->fast_bytes-=f->allocation[i].bytes;++f->fast_frees;}
    memset(f->allocation+i,0,sizeof(f->allocation[i]));free(p);
}
static void *fast_allocate(void *context,size_t bytes){return allocate(context,bytes,0);}
static void fast_release(void *context,void *p){release(context,p,0);}
static void *chip_allocate(void *context,size_t bytes){return allocate(context,bytes,1);}
static void chip_release(void *context,void *p,size_t bytes)
{
    struct fixture *f=context;unsigned i;for(i=0;i<RECORDS&&f->allocation[i].data!=p;++i){}
    assert(i<RECORDS&&f->allocation[i].bytes==bytes);release(f,p,1);
}
static void *queue_allocate(void *context,size_t bytes)
{
    struct fixture *f=context;void *p=fast_allocate(f,bytes);
    assert(!f->queue&&!f->queue_bytes);f->queue=p;f->queue_bytes=bytes;++f->queue_calls;return p;
}
static void queue_release(void *context,void *p)
{
    struct fixture *f=context;assert(p==f->queue&&f->queue_bytes);
    f->queue=NULL;f->queue_bytes=0;++f->queue_frees;fast_release(f,p);
}
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
    assert(f->editor->sampler.generation==f->expected_generation);
    assert(!memcmp(f->saved,f->document.project.samples,sizeof(f->saved)));
    for(i=0;i<3;++i){const struct pt_sample *s=f->document.project.samples+i;
        assert(s->pcm.bits==8+8*i&&s->pcm.channels==1&&s->pcm.capacity==FRAMES&&s->pcm.data==f->pcm[i].data&&s->pcm.frames==FRAMES);
        assert(!memcmp(s->pcm.data,f->source[i],FRAMES*sizeof(int32_t))&&s->slice_count==2&&!memcmp(s->slices,f->markers[i],sizeof(f->markers[i])));}
    assert(pt_project_size(&f->document.project,&n)==PT_PROJECT_OK&&n==f->encoded_bytes);
    out=malloc(n);assert(out);
    assert(pt_project_encode(&f->document.project,out,n,&written)==PT_PROJECT_OK&&written==n&&!memcmp(out,f->encoded,n));free(out);
}
static void cache_gold(struct fixture *f)
{
    unsigned i,j,seen=0;
    for(i=0;i<PT_CACHE_SLOTS;++i){const struct pt_cache_entry *e=f->bridge.cache.entry+i;if(!e->data)continue;
        assert(e->bytes==PACKED&&e->valid==1&&e->pins==1&&e->key&&e->serial);
        for(j=0;j<3&&memcmp(e->data,f->gold[j],PACKED);++j){}
        assert(j<3&&!(seen&(1U<<j)));seen|=1U<<j;}
    assert(seen==7&&pins(f)==3&&f->chip_bytes==3*PACKED);
}
static void tails_equal(struct fixture *f)
{unsigned i;size_t j;for(i=0;i<3;++i)for(j=f->work[i].prefix;j<f->work[i].capacity;++j)assert(((uint8_t *)f->work[i].data)[j]==0xb6);}
static void content_image(struct fixture *f,struct image *a)
{
    struct pt_project *p=&f->document.project;unsigned i,j,n;
    struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];memset(a,0,sizeof(*a));
    image_add(a,p,sizeof(*p));image_add(a,p->samples,p->sample_count*sizeof(*p->samples));
    image_add(a,p->orders,p->order_count*sizeof(*p->orders));
    image_add(a,p->events,p->pattern_count*PT_PROJECT_ROWS*p->channels.count*sizeof(*p->events));
    image_add(a,f->source,sizeof(f->source));image_add(a,f->markers,sizeof(f->markers));
    for(i=0;i<3;++i){assert(pt_sampler_version_spans(f->pin[i],spans,PT_SAMPLER_VERSION_SPANS,&n));
        for(j=0;j<n;++j)image_add(a,spans[j].data,spans[j].bytes);}
    image_add(a,&f->bridge,sizeof(f->bridge));
    for(i=0;i<PT_CACHE_SLOTS;++i)if(f->bridge.cache.entry[i].data)image_add(a,f->bridge.cache.entry[i].data,f->bridge.cache.entry[i].bytes);
    image_add(a,f->work[1].data,f->work[1].capacity);image_add(a,f->work[2].data,f->work[2].capacity);
    image_add(a,&f->backend,sizeof(f->backend));
}
static void blocked_mutations(struct fixture *f)
{
    struct pt_editor *e=f->editor;struct image a;struct counts c;
    unsigned revision=e->history.revision,generation=e->sampler.generation;
    assert(!e->number_field&&!e->name_entry&&!e->workflow.transaction);
    e->panel=0;e->editing=1;e->field=0;e->octave=1;e->pattern=e->row=0;e->project->channels.selected=0;
    content_image(f,&a);c=counts(f);
    assert(!pt_editor_prepare_change(e));assert(!strcmp(e->status,"PLAYBACK STOP UNCONFIRMED - RETRY BEFORE CHANGING PROJECT"));
    assert(pt_editor_key(e,0x31,0)==PT_UI_NONE);assert(!strcmp(e->status,"INVALID EVENT - NO CHANGE"));
    assert(pt_editor_key(e,0x31,8)==PT_UI_NONE);assert(!strcmp(e->status,"UNDO CONFLICT - EDIT PRESERVED"));
    e->panel=5;e->sample=1;pt_editor_sample_all(e);
    assert(pt_editor_key(e,0x4c,0x10)==PT_UI_NONE);assert(!strcmp(e->status,"SAMPLE EDIT REFUSED - NO CHANGE"));
    assert(pt_editor_key(e,0x0c,9)==PT_UI_NONE);assert(!strcmp(e->status,"SLOT ADD REFUSED - NO CHANGE"));
    assert(!pt_editor_dispose(e));assert(!strcmp(e->status,"PLAYBACK STOP UNCONFIRMED - RETRY BEFORE CHANGING PROJECT"));
    assert(e->history.revision==revision&&e->sampler.generation==generation);
    image_equal(&a);counts_equal(c,counts(f));master_save_equal(f);tails_equal(f);++mutation_groups;
}
static int key_equal(const struct pt_readers_key *a,const struct pt_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->serial==b->serial&&a->owner==b->owner&&a->action==b->action&&a->slot==b->slot;}
static void callback_enter(struct backend *b){assert(!b->depth);++b->depth;}
static void callback_leave(struct backend *b){assert(b->depth==1);--b->depth;}
static int consumer_live(const struct backend *b)
{return b->adopted&&b->active&&b->reader_live&&b->reader_reference&&b->reader.reference&&b->action.data&&b->span_count;}
static void original_event(struct fixture *f,const struct pt_readers_event *e)
{
    const struct pt_readers_domain *d;const struct pt_scheduled_action *a;
    assert(e&&e->scheduled.batch.count==1&&e->reader[0]);d=e->reader[0];a=e->scheduled.batch.action;
    assert(e->scheduled.batch.frame==100&&e->scheduled.first==2184&&e->scheduled.last==2205);
    assert(f->editor->sampler.generation==f->backend.source_generation);
    assert(e->queue==d->key.queue&&e->session==f->session&&d->key.session==f->session&&e->scheduled.batch.generation==7&&d->key.generation==7);
    assert(d->key.trigger==e->scheduled.ticket&&d->key.action==0&&d->key.slot==0);
    assert(a->kind==PT_SCHEDULED_TRIGGER&&a->slot==0&&a->data&&a->words==PACKED/2&&a->period==124&&a->volume==48);
    assert(e->binding.context&&e->binding.context_bytes&&d->binding.context&&d->binding.context_bytes&&e->binding.context!=d->binding.context);
    assert(d->spans&&d->count&&d->count<=PT_PREPARED_PAULA_LIFETIME_DATA_SPANS);
}
static int backend_clock(void *context,uint64_t *tick,uint32_t *frequency)
{struct fixture *f=context;struct backend *b=&f->backend;callback_enter(b);++b->clocks;*tick=b->now;*frequency=b->frequency;callback_leave(b);return 1;}
static int backend_submit(void *context,const struct pt_readers_event *e)
{
    struct fixture *f=context;struct backend *b=&f->backend;const struct pt_readers_domain *d;unsigned i;
    callback_enter(b);++b->submits;assert(!b->command_live&&!b->reader_live&&!b->activation_event&&!b->adopted);
    original_event(f,e);d=e->reader[0];assert(b->now==100&&b->now<e->scheduled.first&&b->frequency==1000000);
    if(b->refuse_submit){callback_leave(b);return 0;}
    b->command=(struct command_identity){e->queue,e,e->binding,d->key,e->session,e->scheduled.batch.generation,e->scheduled.ticket,e->command_owner,e->scheduled.batch.count};
    b->reader=(struct reader_identity){d,d->binding,d->key};b->activation_event=e;b->reader_reference=d;b->action=e->scheduled.batch.action[0];
    b->span_count=d->count;for(i=0;i<d->count;++i){assert(d->spans[i].data&&d->spans[i].bytes);b->spans[i]=d->spans[i];}
    b->first=e->scheduled.first;b->last=e->scheduled.last;b->frame=e->scheduled.batch.frame;
    b->command_live=b->reader_live=b->activation_pending=1;
    assert(!b->adopted&&!b->active&&!b->dispatches&&!b->reads&&!b->observed&&!b->issued);callback_leave(b);return 1;
}
static void adopt(struct fixture *f)
{
    struct backend *b=&f->backend;const struct pt_readers_event *e=b->activation_event;const struct pt_readers_domain *d;unsigned i,covered=0;
    assert(!b->depth&&b->command_live&&b->reader_live&&b->activation_pending&&!b->adopted&&!b->active&&!b->dispatches&&b->now==2184);
    original_event(f,e);d=e->reader[0];
    assert(e==b->command.event&&d==b->reader.reference&&d==b->reader_reference&&key_equal(&d->key,&b->reader.key));
    assert(e->scheduled.ticket==b->command.ticket&&e->command_owner==b->command.owner&&e->binding.context==b->command.binding.context&&e->binding.context_bytes==b->command.binding.context_bytes);
    assert(d->binding.context==b->reader.binding.context&&d->binding.context_bytes==b->reader.binding.context_bytes&&d->count==b->span_count);
    assert(e->scheduled.first==b->first&&e->scheduled.last==b->last&&b->now>=b->first&&b->now<b->last);
    assert(e->scheduled.batch.action[0].kind==b->action.kind&&e->scheduled.batch.action[0].slot==b->action.slot&&
        e->scheduled.batch.action[0].data==b->action.data&&e->scheduled.batch.action[0].words==b->action.words&&
        e->scheduled.batch.action[0].period==b->action.period&&e->scheduled.batch.action[0].volume==b->action.volume);
    for(i=0;i<d->count;++i){uintptr_t start=(uintptr_t)b->spans[i].data,pcm=(uintptr_t)b->action.data;
        assert(d->spans[i].data==b->spans[i].data&&d->spans[i].bytes==b->spans[i].bytes);
        if(pcm>=start&&b->spans[i].bytes>=PACKED&&pcm-start<=b->spans[i].bytes-PACKED)covered=1;}
    assert(covered);b->observed=b->issued=b->now;b->adopted=b->active=1;b->activation_event=NULL;b->activation_pending=0;++b->dispatches;
    assert(consumer_live(b));
}
static void read_cache(struct fixture *f,unsigned after_c)
{
    struct backend *b=&f->backend;unsigned i;
    assert(!b->depth&&consumer_live(b)&&!b->activation_event&&!b->activation_pending&&b->dispatches==1&&!b->reads&&b->action.words*2U==PACKED);
    if(after_c)assert(!b->command_live&&!b->command.event&&b->drops_c==1);
    /* Only original persistent-R geometry is traversed, never a C event/context. */
    for(i=0;i<PACKED;++i)assert(b->action.data[i]==f->gold[0][i]);
    ++b->reads;b->read_bytes=PACKED;
}
static void drop_command(struct backend *b)
{
    assert(b->command_live&&b->command.event&&b->command.binding.context);
    b->activation_pending=0;b->activation_event=NULL;b->command_live=0;memset(&b->command,0,sizeof(b->command));++b->drops_c;
    if(b->adopted&&b->reader_live)assert(consumer_live(b));
}
static void drop_reader(struct backend *b)
{
    assert(b->reader_live&&b->reader_reference&&b->reader.reference&&b->action.data&&b->span_count);
    b->active=0;b->activation_pending=0;b->activation_event=NULL;b->reader_reference=NULL;
    memset(&b->action,0,sizeof(b->action));memset(b->spans,0,sizeof(b->spans));b->span_count=0;b->reader_live=0;memset(&b->reader,0,sizeof(b->reader));++b->drops_r;
}
static void reader_gone(const struct backend *b)
{
    unsigned i;assert(!consumer_live(b)&&!b->active&&!b->reader_live&&!b->reader_reference&&!b->reader.reference&&!b->reader.binding.context&&!b->reader.binding.context_bytes);
    assert(!b->activation_pending&&!b->activation_event&&!b->action.data&&!b->action.words&&!b->action.period&&!b->action.volume&&!b->span_count);
    for(i=0;i<PT_PREPARED_PAULA_LIFETIME_DATA_SPANS;++i)assert(!b->spans[i].data&&!b->spans[i].bytes);
}
static void callback_reenter(struct fixture *f)
{
    struct counts before=counts(f);struct image a;struct backend *b=&f->backend;
    memset(&a,0,sizeof(a));image_add(&a,f->work[1].data,f->work[1].capacity);image_add(&a,f->work[2].data,f->work[2].capacity);
    ++b->reentries;assert(!pt_editor_prepare_change(f->editor));
    assert(pt_prepared_paula_mutation_barrier_cancel_command_once(f->barrier)==PT_SCHEDULED_BACKEND);
    assert(pt_prepared_paula_mutation_barrier_cancel_reader_once(f->barrier)==PT_SCHEDULED_BACKEND);
    assert(pt_prepared_paula_mutation_barrier_step(f->barrier)==PT_PREPARED_PAULA_MUTATION_HELD);
    image_equal(&a);counts_equal(before,counts(f));
}
static enum pt_readers_reply backend_command(void *context,uint64_t ticket,struct pt_readers_command_receipt *out)
{
    struct fixture *f=context;struct backend *b=&f->backend;struct pt_readers_command_receipt r;enum mode m=b->command_mode;
    callback_enter(b);++b->cancel_c;assert(b->command_live&&ticket==b->command.ticket&&out);
    if(m==PENDING||m==UNCERTAIN){callback_leave(b);return m==PENDING?PT_READERS_PENDING:PT_READERS_UNCERTAIN;}
    memset(&r,0,sizeof(r));r.domain=PT_READERS_COMMAND_DOMAIN;r.origin=PT_READERS_BACKEND_ACTUAL;
    r.queue=b->command.queue;r.session=b->command.session;r.generation=b->command.generation;r.ticket=b->command.ticket;r.owner=b->command.owner;r.count=b->command.count;
    r.event=b->command.event;r.context=b->command.binding.context;r.context_bytes=b->command.binding.context_bytes;r.action[0].key=b->command.key;
    r.action[0].command=b->adopted?PT_READERS_CANCELLED_AFTER:PT_READERS_CANCELLED_BEFORE;
    r.action[0].reader=b->adopted?(b->reader_live?PT_READERS_ACTIVE:PT_READERS_RETIRED):PT_READERS_NONE;
    r.action[0].adoption=b->adopted?PT_READERS_ADOPTED:PT_READERS_UNADOPTED;r.action[0].observed=b->observed;r.action[0].issued=b->issued;
    if(m==REENTER)callback_reenter(f);
    drop_command(b);if(m==BAD_CLASS)r.action[0].command=PT_READERS_WAITING;
    if(m==WRONG_ENVELOPE){assert(r.session<UINT64_MAX);++r.session;}
    *out=r;callback_leave(b);return PT_READERS_COMMAND_DETACHED;
}
static enum pt_readers_reply backend_reader(void *context,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{
    struct fixture *f=context;struct backend *b=&f->backend;struct pt_readers_reader_receipt r;enum mode m=b->reader_mode;
    callback_enter(b);++b->cancel_r;assert(b->reader_live&&d==b->reader_reference&&out&&key_equal(&d->key,&b->reader.key));
    if(m==PENDING||m==UNCERTAIN){callback_leave(b);return m==PENDING?PT_READERS_PENDING:PT_READERS_UNCERTAIN;}
    memset(&r,0,sizeof(r));r.domain=PT_READERS_READER_DOMAIN;r.key=b->reader.key;r.reference=b->reader.reference;
    r.context=b->reader.binding.context;r.context_bytes=b->reader.binding.context_bytes;r.state=PT_READERS_RETIRED;
    r.adoption=b->adopted?PT_READERS_ADOPTED:PT_READERS_UNADOPTED;r.observed=b->observed;r.issued=b->issued;
    if(m==REENTER)callback_reenter(f);
    drop_reader(b);reader_gone(b);if(m==BAD_CLASS)r.state=PT_READERS_RESERVED;
    if(m==WRONG_ENVELOPE){assert(r.key.serial<UINT64_MAX);++r.key.serial;}
    *out=r;callback_leave(b);return PT_READERS_READER_RETIRED;
}
static enum pt_readers_reply poll_command(void *context,uint64_t ticket,struct pt_readers_command_receipt *out)
{struct fixture *f=context;(void)ticket;(void)out;++f->backend.polls;assert(0);return PT_READERS_UNCERTAIN;}
static enum pt_readers_reply poll_reader(void *context,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{struct fixture *f=context;(void)d;(void)out;++f->backend.polls;assert(0);return PT_READERS_UNCERTAIN;}
static struct fixture *ready(void)
{
    struct fixture *f=calloc(1,sizeof(*f));unsigned i,j;size_t n,written;long page;
    enum pt_prepared_paula_mutation_result r;
    assert(f);memset(f->context_tail,0x7c,sizeof(f->context_tail));
    f->allocator=(struct pt_allocator){f,fast_allocate,fast_release};pt_document_init(&f->document,&f->allocator);
    assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<3;++i){unsigned bits=8+8*i;int32_t low=-(int32_t)(UINT32_C(1)<<(bits-1));struct pt_sample *s=f->document.project.samples+i;
        for(j=0;j<VALUES;++j)f->source[i][j]=(int32_t)(((uint64_t)j*65357+17*i)%((UINT64_C(1)<<bits)-1))+low;
        f->source[i][0]=low;f->source[i][1]=-low-1;f->source[i][2]=-1;f->source[i][3]=1;
        f->source[i][4]=i?(int32_t)(UINT32_C(1)<<(bits-9)):1;f->source[i][5]=-f->source[i][4];
        f->markers[i][0]=16;f->markers[i][1]=256;s->pcm=(struct pt_pcm){f->source[i],VALUES,FRAMES,48000,1,(uint8_t)bits};
        s->slices=f->markers[i];s->slice_count=2;s->volume=48;s->name[0]=(char)('A'+i);
        for(j=0;j<FRAMES;++j)f->gold[i][j]=signed8(f->source[i][j],bits);
        f->gold[i][FRAMES]=0;}
    f->editor=calloc(1,sizeof(*f->editor));assert(f->editor&&pt_editor_init(f->editor,&f->document.project));
    f->editor->sampler.allocator=f->allocator;f->editor->sampler.budget=1024*1024;
    for(i=0;i<3;++i){assert(pt_sampler_pin(&f->editor->sampler,&f->document.project,i,0,f->pcm+i,f->pin+i)==PT_EDIT_OK);
        assert(f->pin[i]&&f->pin[i]==f->editor->sampler.current[i]&&f->pcm[i].data!=f->source[i]);
        if(i)assert(f->pin[i]!=f->pin[i-1]&&f->pcm[i].data!=f->pcm[i-1].data);}
    memcpy(f->saved,f->document.project.samples,sizeof(f->saved));f->expected_generation=f->editor->sampler.generation;
    assert(pt_sampler_paula_bind(&f->bridge,&f->editor->sampler,&f->document.project,f,chip_allocate,chip_release,3U*PACKED));
    assert(pt_project_size(&f->document.project,&f->encoded_bytes)==PT_PROJECT_OK);f->encoded=malloc(f->encoded_bytes);assert(f->encoded);
    assert(pt_project_encode(&f->document.project,f->encoded,f->encoded_bytes,&written)==PT_PROJECT_OK&&written==f->encoded_bytes);
    page=sysconf(_SC_PAGESIZE);assert(page>0);
    f->work[0].prefix=pt_prepared_paula_mutation_barrier_size();f->work[1].prefix=pt_prepared_paula_sources_size();f->work[2].prefix=pt_prepared_paula_lifetime_size();
    for(i=0;i<3;++i){n=((f->work[i].prefix+PACKED+256+(size_t)page-1)/(size_t)page)*(size_t)page;
        f->work[i].capacity=n;f->work[i].data=mmap(NULL,n,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(f->work[i].data!=MAP_FAILED);
        memset(f->work[i].data,0,f->work[i].prefix);memset((uint8_t *)f->work[i].data+f->work[i].prefix,0xb6,n-f->work[i].prefix);}
    f->barrier=f->work[0].data;f->sources=f->work[1].data;f->lifetime=f->work[2].data;
    f->config.editor=f->editor;f->config.bridge=&f->bridge;f->config.sources=f->sources;f->config.source_capacity=f->work[1].capacity;
    f->config.lifetime=f->lifetime;f->config.lifetime_capacity=f->work[2].capacity;f->config.track=0;
    for(i=0;i<3;++i)f->config.sample[i]=i;
    assert(next_session<UINT64_MAX);f->session=++next_session;
    f->config.queue.allocator=(struct pt_allocator){f,queue_allocate,queue_release};f->config.queue.grid=(struct pt_scheduled_grid){100,7,1000000,48000};
    f->config.queue.session=f->session;f->config.queue.frame=100;f->config.queue.control_budget=1024*1024;
    f->config.queue.period=124;f->config.queue.volume=48;f->config.queue.context_count=1;
    f->config.queue.context[0]=(struct pt_prepared_paula_context_extent){f,sizeof(*f)};
    f->config.backend=(struct pt_readers_backend){f,sizeof(*f),{PT_SCHEDULED_REQUIRED,1,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        backend_clock,backend_submit,poll_command,backend_command,poll_reader,backend_reader};
    f->backend.now=100;f->backend.frequency=1000000;f->backend.source_generation=f->expected_generation;
    {struct counts before=counts(f);assert(pt_prepared_paula_mutation_barrier_begin(f->barrier,f->work[0].capacity,&f->config)==PT_PREPARED_PAULA_MUTATION_OK);counts_equal(before,counts(f));}
    memset(&f->config,0,sizeof(f->config));
    do {assert(++f->steps<=SERVICE_LIMIT);r=pt_prepared_paula_mutation_barrier_step(f->barrier);
        assert(r==PT_PREPARED_PAULA_MUTATION_PENDING||r==PT_PREPARED_PAULA_MUTATION_READY);master_save_equal(f);
    }while(r!=PT_PREPARED_PAULA_MUTATION_READY);
    cache_gold(f);
    {struct counts before=counts(f);assert(pt_prepared_paula_mutation_barrier_enqueue_once(f->barrier)==PT_PREPARED_PAULA_MUTATION_QUEUED);counts_equal(before,counts(f));}
    assert(f->queue&&f->queue_calls==1&&!f->queue_frees&&!f->backend.clocks&&!f->backend.submits);
    assert(pt_prepared_paula_mutation_barrier_publication_failure(f->barrier)==PT_SCHEDULED_OK);return f;
}
static enum pt_prepared_paula_mutation_result service(struct fixture *f)
{assert(++f->steps<=SERVICE_LIMIT);return pt_prepared_paula_mutation_barrier_step(f->barrier);}
static enum pt_scheduled_result publish(struct fixture *f)
{
    struct counts before=counts(f);enum pt_scheduled_result r=pt_prepared_paula_mutation_barrier_publish_once(f->barrier);
    resource_equal(before,counts(f));cache_gold(f);master_save_equal(f);tails_equal(f);return r;
}
static void repeated_publish(struct fixture *f)
{
    struct counts c=counts(f);struct image a;content_image(f,&a);
    assert(pt_prepared_paula_mutation_barrier_publish_once(f->barrier)==PT_SCHEDULED_INVALID);
    image_equal(&a);counts_equal(c,counts(f));
}
static void stop_after_veto(struct fixture *f)
{
    struct counts c;blocked_mutations(f);c=counts(f);
    assert(service(f)==PT_PREPARED_PAULA_MUTATION_PENDING);counts_equal(c,counts(f));cache_gold(f);master_save_equal(f);
}
static void no_early_drain(struct fixture *f)
{
    struct counts c=counts(f);struct image a;content_image(f,&a);
    assert(service(f)==PT_PREPARED_PAULA_MUTATION_BUSY);image_equal(&a);counts_equal(c,counts(f));cache_gold(f);master_save_equal(f);
}
static enum pt_scheduled_result collect(struct fixture *f,unsigned reader)
{
    struct counts c=counts(f);struct backend *b=&f->backend;unsigned cc=b->cancel_c,rc=b->cancel_r;
    enum pt_scheduled_result r=reader?pt_prepared_paula_mutation_barrier_cancel_reader_once(f->barrier):pt_prepared_paula_mutation_barrier_cancel_command_once(f->barrier);
    resource_equal(c,counts(f));assert(b->cancel_c==cc+(reader?0:1)&&b->cancel_r==rc+(reader?1:0));
    assert(!b->polls&&!b->depth);cache_gold(f);master_save_equal(f);tails_equal(f);return r;
}
static void repeat_collect(struct fixture *f,unsigned reader)
{
    struct counts c=counts(f);struct image a;content_image(f,&a);
    assert((reader?pt_prepared_paula_mutation_barrier_cancel_reader_once(f->barrier):pt_prepared_paula_mutation_barrier_cancel_command_once(f->barrier))==PT_SCHEDULED_INVALID);
    image_equal(&a);counts_equal(c,counts(f));
}
static void stale(struct fixture *f)
{
    assert(f->editor->sampler.generation==f->expected_generation&&f->expected_generation<UINT32_MAX);
    ++f->editor->sampler.generation;++f->expected_generation;master_save_equal(f);
}
static void finish(struct fixture *f,enum pt_scheduled_result failure,unsigned adopted_family)
{
    static const unsigned expected_pins[7]={2,2,1,1,0,0,0};
    struct counts c;struct backend model;unsigned i;
    memcpy(&model,&f->backend,sizeof(model));
    assert(!f->backend.command_live&&!f->backend.command.event);reader_gone(&f->backend);
    assert(pt_prepared_paula_mutation_barrier_publication_failure(f->barrier)==failure);
    for(i=0;i<7;++i){c=counts(f);assert(service(f)==PT_PREPARED_PAULA_MUTATION_PENDING);counts_equal(c,counts(f));
        assert(pins(f)==expected_pins[i]);master_save_equal(f);tails_equal(f);
        assert(pt_prepared_paula_mutation_barrier_publication_failure(f->barrier)==failure);}
    assert(!pins(f)&&f->queue&&f->chip_bytes==3*PACKED);
    assert(service(f)==PT_PREPARED_PAULA_MUTATION_PENDING);assert(!f->queue&&f->queue_calls==f->queue_frees);
    assert(service(f)==PT_PREPARED_PAULA_MUTATION_CLEANED);assert(!f->chip_bytes&&!pins(f));
    assert(!memcmp(&model,&f->backend,sizeof(model)));master_save_equal(f);blocked_mutations(f);
    assert(pt_prepared_paula_mutation_barrier_failure(f->barrier)==PT_PREPARED_PAULA_SOURCES_OK);
    assert(!mprotect(f->work[1].data,f->work[1].capacity,PROT_NONE));assert(!mprotect(f->work[2].data,f->work[2].capacity,PROT_NONE));
    c=counts(f);assert(pt_prepared_paula_mutation_barrier_state(f->barrier)==PT_PREPARED_PAULA_MUTATION_DETACHABLE);
    assert(pt_prepared_paula_mutation_barrier_publication_failure(f->barrier)==failure);
    assert(service(f)==PT_PREPARED_PAULA_MUTATION_CLEANED);counts_equal(c,counts(f));
    assert(pt_prepared_paula_mutation_barrier_detach(f->barrier)==PT_PREPARED_PAULA_MUTATION_CLOSED);counts_equal(c,counts(f));
    assert(!f->editor->before_change&&!f->editor->change_ready&&!f->editor->before_change_context);
    master_save_equal(f);assert(pt_editor_prepare_change(f->editor));
    f->editor->panel=0;f->editor->editing=1;f->editor->row=f->editor->pattern=0;
    {unsigned revision=f->editor->history.revision;pt_editor_key(f->editor,0x31,0);
        assert(f->editor->history.revision!=revision&&f->document.project.events[0].kind==PT_NOTE_PERIOD);}
    assert(pt_editor_dispose(f->editor));for(i=0;i<3;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    assert(!f->editor->sampler.bytes&&!f->editor->sampler.live_count&&!f->editor->sampler.pending_count);
    pt_document_release(&f->document);assert(!f->fast_bytes&&!f->chip_bytes&&!f->queue&&!f->backend.polls);
    for(i=0;i<RECORDS;++i)assert(!f->allocation[i].data);
    free(f->encoded);f->encoded=NULL;free(f->editor);f->editor=NULL;
    /* Children and editor have ended: terminal observations must use own storage. */
    assert(pt_prepared_paula_mutation_barrier_state(f->barrier)==PT_PREPARED_PAULA_MUTATION_FINISHED);
    assert(pt_prepared_paula_mutation_barrier_failure(f->barrier)==PT_PREPARED_PAULA_SOURCES_OK);
    assert(pt_prepared_paula_mutation_barrier_publication_failure(f->barrier)==failure);
    assert(pt_prepared_paula_mutation_barrier_detach(f->barrier)==PT_PREPARED_PAULA_MUTATION_CLOSED);
    for(i=0;i<3;++i)assert(!munmap(f->work[i].data,f->work[i].capacity));
    free(f);if(adopted_family)++adopted_completed;else ++published_completed;
}
static void retain(struct fixture *f,enum pt_scheduled_result failure,unsigned outer_fault,unsigned adopted_family)
{
    struct counts c=counts(f);enum pt_prepared_paula_mutation_result r;
    assert(f->queue&&f->fast_bytes&&f->chip_bytes==3*PACKED&&pins(f)==3);
    assert(pt_prepared_paula_mutation_barrier_publication_failure(f->barrier)==failure);
    assert(pt_prepared_paula_mutation_barrier_failure(f->barrier)==(outer_fault?PT_PREPARED_PAULA_SOURCES_FAULT:PT_PREPARED_PAULA_SOURCES_OK));
    r=service(f);assert(r==(outer_fault?PT_PREPARED_PAULA_MUTATION_HELD:PT_PREPARED_PAULA_MUTATION_BUSY));
    assert(pt_prepared_paula_mutation_barrier_detach(f->barrier)==(outer_fault?PT_PREPARED_PAULA_MUTATION_HELD:PT_PREPARED_PAULA_MUTATION_BUSY));
    counts_equal(c,counts(f));master_save_equal(f);cache_gold(f);tails_equal(f);
    blocked_mutations(f);
    if(adopted_family){assert(adopted_held<1);adopted_retained[adopted_held++]=f;}
    else{assert(published_held<5);published_retained[published_held++]=f;}
}
enum published_kind { CLEAN_C,CLEAN_R,STALE_PUBLISHED,BAD_C,BAD_R,STALE_BEFORE,
    SUBMIT_ZERO,CLOCK_FIRST,PENDING_C,PENDING_R,WRONG_R,REENTRY_C,REENTRY_R };
static void published_case(enum published_kind kind)
{
    struct fixture *f=ready();struct backend *b=&f->backend;enum pt_scheduled_result first,second,failure=PT_SCHEDULED_OK;
    unsigned reader_first=kind==CLEAN_R||kind==BAD_R||kind==PENDING_R||kind==WRONG_R||kind==REENTRY_R;
    ++published_cases;
    if(kind==STALE_BEFORE)stale(f);
    if(kind==SUBMIT_ZERO)b->refuse_submit=1;
    if(kind==CLOCK_FIRST)b->now=2184;
    assert(publish(f)==(kind==STALE_BEFORE?PT_SCHEDULED_STALE:kind==SUBMIT_ZERO?PT_SCHEDULED_PENDING:kind==CLOCK_FIRST?PT_SCHEDULED_LATE:PT_SCHEDULED_OK));
    repeated_publish(f);
    assert(!b->dispatches&&!b->reads&&!b->adopted&&!b->active&&!b->observed&&!b->issued);
    if(kind==STALE_BEFORE||kind==SUBMIT_ZERO||kind==CLOCK_FIRST){
        assert(b->clocks==(kind==STALE_BEFORE?0U:1U)&&b->submits==(kind==SUBMIT_ZERO?1U:0U));
        assert(!b->command_live&&!b->command.event&&!b->reader_live&&!b->reader_reference&&!b->activation_event&&!b->span_count);
        failure=kind==STALE_BEFORE?PT_SCHEDULED_STALE:kind==CLOCK_FIRST?PT_SCHEDULED_LATE:PT_SCHEDULED_OK;
        stop_after_veto(f);assert(!b->cancel_c&&!b->cancel_r&&!b->drops_c&&!b->drops_r);finish(f,failure,0);return;
    }
    assert(b->clocks==1&&b->submits==1&&b->command_live&&b->reader_live&&b->activation_pending);
    if(kind==STALE_PUBLISHED)stale(f);
    stop_after_veto(f);no_early_drain(f);
    if(kind==BAD_C)b->command_mode=BAD_CLASS;
    if(kind==BAD_R)b->reader_mode=BAD_CLASS;
    if(kind==PENDING_C)b->command_mode=PENDING;
    if(kind==PENDING_R)b->reader_mode=PENDING;
    if(kind==WRONG_R)b->reader_mode=WRONG_ENVELOPE;
    if(kind==REENTRY_C)b->command_mode=REENTER;
    if(kind==REENTRY_R)b->reader_mode=REENTER;
    first=collect(f,reader_first);
    if(kind==REENTRY_C||kind==REENTRY_R){
        assert(first==PT_SCHEDULED_BACKEND&&b->reentries==1);
        assert(pt_prepared_paula_mutation_barrier_state(f->barrier)==PT_PREPARED_PAULA_MUTATION_UNCERTAIN);
        assert(pt_prepared_paula_mutation_barrier_publication_failure(f->barrier)==PT_SCHEDULED_OK);
        retain(f,PT_SCHEDULED_OK,1,0);return;
    }
    repeat_collect(f,reader_first);
    if(kind==PENDING_C||kind==PENDING_R){
        assert(first==PT_SCHEDULED_PENDING&&b->command_live&&b->reader_live&&b->activation_pending&&!b->drops_c&&!b->drops_r);
        retain(f,PT_SCHEDULED_OK,0,0);return;
    }
    if(kind==WRONG_R){assert(first==PT_SCHEDULED_BACKEND&&b->command_live&&b->drops_r==1);reader_gone(b);retain(f,PT_SCHEDULED_BACKEND,0,0);return;}
    if(kind==BAD_C||kind==BAD_R){failure=PT_SCHEDULED_BACKEND;assert(first==PT_SCHEDULED_BACKEND);}
    else assert(first==PT_SCHEDULED_OK);
    if(reader_first){reader_gone(b);assert(b->command_live&&!b->drops_c&&b->drops_r==1);}
    else assert(!b->command_live&&b->drops_c==1&&b->reader_live);
    no_early_drain(f);second=collect(f,!reader_first);repeat_collect(f,!reader_first);
    assert(second==failure&&b->cancel_c==1&&b->cancel_r==1&&b->drops_c==1&&b->drops_r==1&&!b->polls);
    reader_gone(b);finish(f,failure,0);
}
enum adopted_kind { ADOPTED_C,ADOPTED_R,ADOPTED_STALE,ADOPTED_UNCERTAIN };
static void adopted_case(enum adopted_kind kind)
{
    struct fixture *f=ready();struct backend *b=&f->backend;unsigned reader_first=kind==ADOPTED_R;
    ++adopted_cases;assert(publish(f)==PT_SCHEDULED_OK);repeated_publish(f);
    b->now=2184;adopt(f);if(kind==ADOPTED_STALE)stale(f);
    if(reader_first)read_cache(f,0);
    stop_after_veto(f);no_early_drain(f);
    assert(collect(f,reader_first)==PT_SCHEDULED_OK);repeat_collect(f,reader_first);
    no_early_drain(f);
    if(reader_first){reader_gone(b);assert(b->command_live&&!b->drops_c&&b->drops_r==1);}
    else {assert(!b->command_live&&b->drops_c==1&&consumer_live(b));read_cache(f,1);}
    if(kind==ADOPTED_UNCERTAIN){
        b->reader_mode=UNCERTAIN;assert(collect(f,1)==PT_SCHEDULED_BACKEND);repeat_collect(f,1);
        assert(consumer_live(b)&&!b->drops_r&&b->cancel_c==1&&b->cancel_r==1&&b->dispatches==1&&b->reads==1&&b->read_bytes==PACKED);
        retain(f,PT_SCHEDULED_BACKEND,0,1);return;
    }
    assert(collect(f,!reader_first)==PT_SCHEDULED_OK);repeat_collect(f,!reader_first);
    assert(b->cancel_c==1&&b->cancel_r==1&&b->drops_c==1&&b->drops_r==1&&b->dispatches==1&&b->reads==1&&b->read_bytes==PACKED&&!b->polls);
    reader_gone(b);finish(f,PT_SCHEDULED_OK,1);
}
int main(void)
{
    unsigned i,j;
    for(i=CLEAN_C;i<=REENTRY_R;++i)published_case((enum published_kind)i);
    for(i=ADOPTED_C;i<=ADOPTED_UNCERTAIN;++i)adopted_case((enum adopted_kind)i);
    assert(published_cases==13&&published_completed==8&&published_held==5);
    assert(adopted_cases==4&&adopted_completed==3&&adopted_held==1&&mutation_groups==34);
    for(i=0;i<5;++i){assert(published_retained[i]);for(j=0;j<i;++j)assert(published_retained[i]!=published_retained[j]);}
    assert(adopted_retained[0]);for(i=0;i<5;++i)assert(adopted_retained[0]!=published_retained[i]);
    puts("PREPARED PAULA PUBLISHED MUTATION HOST PASS:13 published(8 completed,5 retained);4 adopted(3 completed,1 retained);real editor veto;typed C/R;R-only post-C read;captured cleanup;NO OUTPUT OR NATIVE AUTHORITY");
    return 0;
}
