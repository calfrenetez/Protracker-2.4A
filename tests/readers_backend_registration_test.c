/* Public-header-only backend registration test with genuine sampler/song owners.
 * scheduled_readers.c, the reader adapter and song producer are separate TUs.
 * No private queue casts, owner callbacks or inherited test C are used.
 * All substantial fixture storage is heap-owned. The synthetic clock/receipts
 * prove software ownership only, never native placement or hardware activation.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/core/scheduled_readers.h"
#include "../src/editor/paula_readers_song.h"

#define COMPACT_FRAMES 2048U
#define COMPACT_VALUES (COMPACT_FRAMES+8U)
#define COMPACT_BLOCKS 96U
#define COMPACT_STEPS 4096U
#define COMPACT_RATE 48000U
#define COMPACT_SOURCE_RATE 8287U
#define COMPACT_FREQUENCY 1000000U
#define COMPACT_EPOCH 100U
#define COMPACT_START 10000U
#define COMPACT_TICK_FRAMES 960U

/* The exact same fixture entry accepts ordinary and representation adapters.
 * Host main supplies malloc; the separate optional Exec wrapper supplies the
 * existing Fast master allocator and reserve-checked Chip allocator. Neither
 * callback selection nor a successful compiler build proves native placement.
 */
struct pt_compact_song_memory {
    struct pt_allocator ordinary;
    void *chip_context;
    void *(*chip_allocate)(void *,size_t);
    void (*chip_release)(void *,void *,size_t);
};
struct compact_block {void *data;size_t bytes;unsigned chip;};
struct compact_command {
    const struct pt_readers_event *event;
    struct pt_readers_command_receipt receipt;
    unsigned live,fired,detach,song_index;
};
struct compact_reader {
    const struct pt_readers_domain *reference;
    struct pt_readers_reader_receipt receipt;
    unsigned live,retire,song_index;
};
struct compact_backend {
    struct compact_command command[2];
    struct compact_reader reader;
    struct pt_readers_key slot[4];
    unsigned active[4],submissions,effects,detached,retired,clock_calls;
    unsigned command_polls,reader_polls;
    uint64_t now,detached_ticket;
    uintptr_t detached_event;
    unsigned reused,submit_uncertain,forge_command,forge_reader;
};
/* Complete mutable allocator/Chip callback extent. It is a dedicated heap
 * object, genuinely disjoint from source, configuration, status and owner/out
 * storage in compact_state. Keep it alive until every source/owner is closed. */
struct compact_allocation_context {
    struct pt_compact_song_memory memory;
    struct compact_block block[COMPACT_BLOCKS];
    unsigned ordinary_live,chip_live,ordinary_calls,chip_calls;
    size_t ordinary_bytes,chip_bytes,ordinary_peak,chip_peak;
    unsigned context_owned;
};
struct compact_state {
    struct compact_allocation_context *allocation;
    struct pt_allocator allocator;
    struct pt_document document;
    struct pt_sampler sampler;
    struct pt_pattern_history history;
    struct pt_pattern_command history_command[1];
    struct pt_event_change history_change[1];
    struct pt_paula_readers_song_config config;
    struct pt_paula_readers_song_status status,before_status;
    struct pt_paula_readers_song *song;
    struct compact_backend backend;
    struct pt_sample sample_before;
    int32_t *master,*master_before;
    unsigned slot,bits,phases,steps,controls,scratch_discarded,pressure;
    uint64_t last_frame;
    struct pt_readers_output *obsolete_queue;
    struct pt_readers_command_receipt command_out,command_before;
    struct pt_readers_reader_receipt reader_out,reader_before;
};

static void *compact_allocate_kind(struct compact_allocation_context *s,size_t bytes,unsigned chip)
{
    void *p;unsigned i;
    assert(bytes);
    p=chip?s->memory.chip_allocate(s->memory.chip_context,bytes):
        s->memory.ordinary.allocate(s->memory.ordinary.context,bytes);
    if(!p)return NULL;
    for(i=0;i<COMPACT_BLOCKS;++i)if(!s->block[i].data)break;
    assert(i<COMPACT_BLOCKS);
    s->block[i]=(struct compact_block){p,bytes,chip};
    memset(p,0xa5,bytes);
    if(chip){++s->chip_calls;++s->chip_live;s->chip_bytes+=bytes;
        if(s->chip_bytes>s->chip_peak)s->chip_peak=s->chip_bytes;}
    else{++s->ordinary_calls;++s->ordinary_live;s->ordinary_bytes+=bytes;
        if(s->ordinary_bytes>s->ordinary_peak)s->ordinary_peak=s->ordinary_bytes;}
    return p;
}
static void *compact_allocate(void *c,size_t bytes)
{return compact_allocate_kind(c,bytes,0);}
static void *compact_chip_allocate(void *c,size_t bytes)
{return compact_allocate_kind(c,bytes,1);}
static void compact_release_kind(struct compact_allocation_context *s,void *p,size_t bytes,unsigned chip)
{
    unsigned i;
    if(!p)return;
    for(i=0;i<COMPACT_BLOCKS;++i)if(s->block[i].data==p)break;
    assert(i<COMPACT_BLOCKS&&s->block[i].chip==chip);
    if(chip){assert(bytes==s->block[i].bytes&&s->chip_live);
        --s->chip_live;s->chip_bytes-=bytes;
        s->memory.chip_release(s->memory.chip_context,p,bytes);}
    else{assert(s->ordinary_live);--s->ordinary_live;
        s->ordinary_bytes-=s->block[i].bytes;
        s->memory.ordinary.release(s->memory.ordinary.context,p);}
    memset(s->block+i,0,sizeof(s->block[i]));
}
static void compact_release(void *c,void *p)
{compact_release_kind(c,p,0,0);}
static void compact_chip_release(void *c,void *p,size_t bytes)
{compact_release_kind(c,p,bytes,1);}

/* Compare only public keys and the model's actual slot registration. Opaque
 * bindings are receipt identities, never callbacks or ACTIVE permission.
 * Genuine producer masters stay immutable while these holders are retained. */
static int compact_key_equal(const struct pt_readers_key *a,const struct pt_readers_key *b)
{
    return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&
        a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&
        a->action==b->action&&a->slot==b->slot;
}
static int compact_condition(struct compact_backend *b,const struct pt_readers_event *e)
{
    unsigned i;
    if(!e->binding.context||!e->binding.context_bytes)return 0;
    for(i=0;i<e->scheduled.batch.count;++i){
        const struct pt_readers_domain *d=e->reader[i];
        if(!d||!d->binding.context||!d->binding.context_bytes||
           d->key.queue!=e->queue||d->key.session!=e->session||
           d->key.generation!=e->scheduled.batch.generation)return 0;
        if(e->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER&&
           (!b->active[d->key.slot]||!compact_key_equal(&b->slot[d->key.slot],&d->key)))return 0;
    }
    return 1;
}
static int compact_clock(void *c,uint64_t *ticks,uint32_t *frequency)
{
    struct compact_backend *b=c;++b->clock_calls;
    *ticks=b->now;*frequency=COMPACT_FREQUENCY;return 1;
}
static int compact_submit(void *c,const struct pt_readers_event *e)
{
    struct compact_backend *b=c;struct compact_command *m=NULL;
    unsigned i;
    ++b->submissions;
    if(b->now>=e->scheduled.first||!compact_condition(b,e))return 0;
    for(i=0;i<2;++i)if(!b->command[i].live){m=b->command+i;break;}
    assert(m&&e->scheduled.batch.count==1);
    if((uintptr_t)e==b->detached_event){
        assert(e->scheduled.ticket!=b->detached_ticket);++b->reused;
    }
    memset(m,0,sizeof(*m));m->live=1;m->event=e;
    m->receipt.domain=PT_READERS_COMMAND_DOMAIN;m->receipt.origin=PT_READERS_BACKEND_ACTUAL;
    m->receipt.queue=e->queue;m->receipt.session=e->session;
    m->receipt.generation=e->scheduled.batch.generation;m->receipt.ticket=e->scheduled.ticket;
    m->receipt.owner=e->command_owner;m->receipt.count=e->scheduled.batch.count;
    m->receipt.event=e;m->receipt.context=e->binding.context;
    m->receipt.context_bytes=e->binding.context_bytes;
    for(i=0;i<m->receipt.count;++i){
        const struct pt_readers_domain *d=e->reader[i];
        m->receipt.action[i].key=d->key;
        if(e->scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER){
            assert(!b->reader.live);memset(&b->reader,0,sizeof(b->reader));
            b->reader.live=1;b->reader.reference=d;
            b->reader.receipt=(struct pt_readers_reader_receipt){PT_READERS_READER_DOMAIN,
                d->key,d,d->binding.context,d->binding.context_bytes,
                PT_READERS_RESERVED,PT_READERS_UNADOPTED,0,0};
        }else{
            assert(b->reader.live&&compact_key_equal(&b->reader.receipt.key,&d->key));
            m->receipt.action[i].reader=PT_READERS_ACTIVE;
            m->receipt.action[i].adoption=PT_READERS_ADOPTED;
        }
    }
    return b->submit_uncertain?-1:1;
}
static enum pt_readers_reply compact_poll_command(void *c,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{
    struct compact_backend *b=c;unsigned i;struct compact_command *m=NULL;
    ++b->command_polls;
    for(i=0;i<2;++i)if(b->command[i].live&&b->command[i].receipt.ticket==ticket)
        m=b->command+i;
    assert(m);
    if(!m->fired)return PT_READERS_PENDING;
    if(b->forge_command){
        /* Deliberately invalid positive proof: retain every actual reference.
         * A later exact proof must still be able to drain sticky failure. */
        *out=m->receipt;
        if(b->forge_command==1)out->context=&b->now;
        else ++out->context_bytes;
        return PT_READERS_COMMAND_DETACHED;
    }
    if(m->detach){
        if(b->reader.live&&b->reader.retire)m->receipt.action[0].reader=PT_READERS_RETIRED;
        *out=m->receipt;
        /* Positive command quiescence forgets ALL command-owned pointers.
         * The still-live reader registration is a separate reference domain. */
        b->detached_event=(uintptr_t)m->event;b->detached_ticket=m->receipt.ticket;
        memset(m,0,sizeof(*m));++b->detached;return PT_READERS_COMMAND_DETACHED;
    }
    *out=m->receipt;return PT_READERS_OBSERVATION;
}
static enum pt_readers_reply compact_poll_reader(void *c,const struct pt_readers_domain *d,
    struct pt_readers_reader_receipt *out)
{
    struct compact_backend *b=c;struct compact_reader *r=&b->reader;
    ++b->reader_polls;
    assert(r->live&&r->reference==d&&compact_key_equal(&r->receipt.key,&d->key));
    if(r->receipt.adoption!=PT_READERS_ADOPTED)return PT_READERS_PENDING;
    if(b->forge_reader){
        *out=r->receipt;out->state=PT_READERS_RETIRED;
        if(b->forge_reader==1)out->context=&b->now;
        else ++out->context_bytes;
        return PT_READERS_READER_RETIRED;
    }
    *out=r->receipt;
    if(r->retire){
        /* Commands may still exist: preserve only numeric retired identity in
         * their already-issued receipt; drop every persistent domain pointer. */
        unsigned i;
        for(i=0;i<2;++i)if(b->command[i].live&&
            compact_key_equal(&b->command[i].receipt.action[0].key,&r->receipt.key))
            b->command[i].receipt.action[0].reader=PT_READERS_RETIRED;
        b->active[r->receipt.key.slot]=0;
        memset(b->slot+r->receipt.key.slot,0,sizeof(b->slot[0]));
        memset(r,0,sizeof(*r));++b->retired;return PT_READERS_READER_RETIRED;
    }
    return PT_READERS_OBSERVATION;
}

static void compact_fire(struct compact_state *s,struct compact_command *m)
{
    struct compact_backend *b=&s->backend;const struct pt_readers_event *e=m->event;
    struct pt_readers_action_receipt *a=m->receipt.action;unsigned ordinary=s->allocation->ordinary_calls;
    unsigned chip=s->allocation->chip_calls;enum pt_scheduled_kind kind=e->scheduled.batch.action[0].kind;
    assert(m->live&&!m->fired&&compact_condition(b,e));
    b->now=e->scheduled.first;
    assert(b->now>=e->scheduled.first&&b->now<e->scheduled.last);
    m->fired=1;a->command=PT_READERS_ISSUED;a->observed=a->issued=b->now;
    a->adoption=PT_READERS_ADOPTED;
    if(kind==PT_SCHEDULED_TRIGGER){
        b->slot[a->key.slot]=a->key;b->active[a->key.slot]=1;
        b->reader.receipt.state=PT_READERS_ACTIVE;
        b->reader.receipt.adoption=PT_READERS_ADOPTED;
        b->reader.receipt.observed=b->reader.receipt.issued=b->now;
    }
    if(kind==PT_SCHEDULED_STOP){b->active[a->key.slot]=0;
        b->reader.receipt.state=PT_READERS_DRAINING;a->reader=PT_READERS_DRAINING;}
    else a->reader=PT_READERS_ACTIVE;
    ++b->effects;
    assert(s->allocation->ordinary_calls==ordinary&&s->allocation->chip_calls==chip);
}

static void compact_source_unchanged(struct compact_state *s)
{
    const struct pt_sample *sample=s->document.project.samples+s->slot;
    assert(!memcmp(sample,&s->sample_before,sizeof(*sample)));
    assert(sample->pcm.data==s->master&&sample->pcm.capacity==COMPACT_VALUES);
    assert(!memcmp(s->master,s->master_before,COMPACT_VALUES*sizeof(int32_t)));
}
static struct compact_state *compact_source(const struct pt_compact_song_memory *memory,unsigned bits)
{
    static uint64_t session=3000;
    struct compact_state *s;struct compact_allocation_context *allocation;
    struct pt_project *p;struct pt_pcm pcm;unsigned i;
    assert(memory&&memory->ordinary.allocate&&memory->ordinary.release&&
        memory->chip_allocate&&memory->chip_release);
    allocation=memory->ordinary.allocate(memory->ordinary.context,sizeof(*allocation));assert(allocation);
    memset(allocation,0,sizeof(*allocation));allocation->memory=*memory;allocation->context_owned=1;
    s=compact_allocate(allocation,sizeof(*s));assert(s);
    memset(s,0,sizeof(*s));s->allocation=allocation;s->bits=bits;
    s->allocator=(struct pt_allocator){allocation,compact_allocate,compact_release};
    pt_document_init(&s->document,&s->allocator);
    assert(pt_document_new(&s->document,4,1024U*1024U)==PT_PROJECT_OK);
    p=&s->document.project;p->speed=1;p->bpm=125;
    pt_sampler_init(&s->sampler,&s->allocator,1024U*1024U);
    assert(pt_pattern_history_init(&s->history,p,s->history_command,1,s->history_change,1)==PT_EDIT_OK);
    s->master=compact_allocate(s->allocation,COMPACT_VALUES*sizeof(int32_t));assert(s->master);
    for(i=0;i<COMPACT_FRAMES;++i)s->master[i]=((int32_t)(i%200)-100)*
        (INT32_C(1)<<(bits-8))+(bits==8?0:(int32_t)(i&63));
    s->master[0]=(INT32_C(1)<<(bits-1))-1;s->master[1]=-(INT32_C(1)<<(bits-1));
    s->master[2]=bits==8?0x12:bits==16?0x1234:0x123456;
    s->master[3]=-s->master[2];
    for(i=COMPACT_FRAMES;i<COMPACT_VALUES;++i)s->master[i]=INT32_C(0x5a6b7c8d);
    /* Rounded PAL period428 reference rate: exact control conversion rounds
     * fine-slide period427/426 back to427/426, independently asserted below. */
    pcm=(struct pt_pcm){s->master,COMPACT_VALUES,COMPACT_FRAMES,COMPACT_SOURCE_RATE,1,(uint8_t)bits};
    s->slot=p->sample_count;
    assert(pt_sampler_append_owned(&s->sampler,p,&s->history,&pcm,&s->allocator,
        "compact immutable master")==PT_EDIT_OK&&!pcm.data&&s->sampler.current[s->slot]);
    p->events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,(uint8_t)(s->slot+1),14,0x11,0,0};
    p->events[4].effect=12;p->events[4].parameter=32;
    p->events[8].effect=14;p->events[8].parameter=0x11;
    p->events[15].effect=15;p->events[15].parameter=0;
    memcpy(&s->sample_before,p->samples+s->slot,sizeof(s->sample_before));
    s->master_before=compact_allocate(s->allocation,COMPACT_VALUES*sizeof(int32_t));assert(s->master_before);
    memcpy(s->master_before,s->master,COMPACT_VALUES*sizeof(int32_t));
    s->config.render.rate=COMPACT_RATE;s->config.render.bits=24;s->config.render.tracks=1;
    s->config.render.gain_q16=65536;s->config.render.tick_limit=16;s->config.render.frame_limit=48000;
    s->config.caps=(struct pt_paula_render_caps){3546895,124,65535};
    s->config.readers=(struct pt_paula_readers_config){2,8,1024U*1024U,16384,7,
        allocation,compact_chip_allocate,compact_chip_release};
    s->config.grid=(struct pt_scheduled_grid){COMPACT_EPOCH,7,COMPACT_FREQUENCY,COMPACT_RATE};
    s->config.backend=(struct pt_readers_backend){&s->backend,sizeof(s->backend),{7,8,4},
        PT_READERS_VERSION,PT_READERS_REQUIRED,8,compact_clock,compact_submit,
        compact_poll_command,compact_poll_command,compact_poll_reader,compact_poll_reader};
    assert(session!=UINT64_MAX);s->config.session=++session;s->config.absolute_start=COMPACT_START;
    s->config.control_budget=2U*1024U*1024U;s->backend.now=COMPACT_EPOCH;
    return s;
}
static void compact_source_close(struct compact_state *s)
{
    struct compact_allocation_context *allocation=s->allocation;
    struct pt_allocator base=allocation->memory.ordinary;
    compact_source_unchanged(s);assert(!s->song&&!s->backend.reader.live);
    assert(!s->backend.command[0].live&&!s->backend.command[1].live&&!s->allocation->chip_live);
    compact_release(s->allocation,s->master_before);s->master_before=NULL;
    pt_pattern_history_release(&s->history);pt_sampler_release(&s->sampler);
    pt_document_release(&s->document);
    assert(!s->sampler.bytes&&!allocation->chip_live&&!allocation->chip_bytes);
    /* The fixture control is the last tracked ordinary arena. No callback
     * context may be freed while sampler/source/producer owners still exist. */
    assert(allocation->ordinary_live==1&&allocation->context_owned);
    compact_release(allocation,s);
    assert(!allocation->ordinary_live&&!allocation->ordinary_bytes&&
        !allocation->chip_live&&!allocation->chip_bytes);
    allocation->context_owned=0;base.release(base.context,allocation);
}
static enum pt_paula_readers_song_result compact_begin(struct compact_state *s)
{
    size_t n=pt_paula_readers_song_begin_workspace_size();
    size_t alignment=pt_paula_readers_song_begin_workspace_alignment(),capacity;
    void *raw,*workspace;uintptr_t address;enum pt_paula_readers_song_result result;
    assert(n&&alignment&&n<=SIZE_MAX-alignment);
    capacity=n+alignment;raw=compact_allocate(s->allocation,capacity);assert(raw);
    address=(uintptr_t)raw;assert(address<=UINTPTR_MAX-(alignment-1));
    address+=(alignment-address%alignment)%alignment;workspace=(void *)address;
    assert((size_t)((uint8_t *)workspace-(uint8_t *)raw)+n<=capacity);
    result=pt_paula_readers_song_begin_in_workspace(&s->allocator,&s->sampler,
        &s->document.project,&s->config,1,workspace,n,&s->song);
    /* Detect any escaped constructor scratch reference during later actual
     * startup/audit/renderer/pool use: overwrite and release on this return. */
    memset(raw,0x3c,capacity);compact_release(s->allocation,raw);++s->scratch_discarded;
    assert(!s->backend.submissions&&!s->backend.effects&&!s->allocation->chip_calls);
    return result;
}
static enum pt_paula_readers_song_result compact_step(struct compact_state *s)
{
    enum pt_paula_readers_song_result result,observed;
    assert(++s->steps<=COMPACT_STEPS);
    observed=pt_paula_readers_song_get(s->song,1,&s->status);
    assert(observed==PT_PAULA_READERS_SONG_PENDING||observed==PT_PAULA_READERS_SONG_WAIT_ACTIVE||
        observed==PT_PAULA_READERS_SONG_WAIT_PRESSURE||observed==PT_PAULA_READERS_SONG_DONE);
    s->phases|=1U<<s->status.phase;
    result=pt_paula_readers_song_step(s->song,1,256,&s->status);
    assert(result==PT_PAULA_READERS_SONG_PENDING||result==PT_PAULA_READERS_SONG_DONE||
        result==PT_PAULA_READERS_SONG_WAIT_ACTIVE||result==PT_PAULA_READERS_SONG_WAIT_PRESSURE);
    compact_source_unchanged(s);return result;
}
static void compact_until_publish(struct compact_state *s)
{
    unsigned n=0;
    while(s->status.phase!=PT_PAULA_READERS_SONG_PUBLISH){
        assert(++n<=COMPACT_STEPS&&compact_step(s)==PT_PAULA_READERS_SONG_PENDING);
    }
}
static struct compact_command *compact_publish(struct compact_state *s,unsigned ordinal)
{
    static const uint64_t expected_window[4][2]={
        {208434,208455},{228434,228455},{248434,248455},{268434,268455}};
    struct compact_command *m=NULL;unsigned i,old=s->status.published_mask,added;
    const struct pt_scheduled_action *a;const struct pt_scheduled_event *e;
    uint64_t first_numerator,last_numerator;
    assert(pt_paula_readers_song_publish_next(s->song,1)==PT_SCHEDULED_OK);
    assert(pt_paula_readers_song_get(s->song,1,&s->status)==PT_PAULA_READERS_SONG_PENDING||s->status.done);
    added=s->status.published_mask&~old;assert(added==1||added==2);
    for(i=0;i<2;++i)if(s->backend.command[i].live&&!s->backend.command[i].fired)m=s->backend.command+i;
    assert(m);m->song_index=added==1?0:1;e=&m->event->scheduled;a=e->batch.action;
    assert(ordinal<4);
    assert(e->batch.count==1&&e->batch.generation==7&&e->batch.frame==COMPACT_START+(uint64_t)ordinal*COMPACT_TICK_FRAMES);
    /* Original frame thresholds are ceilings, never a floor or a rebased epoch.
     * The asserted frames bound both products below 12,882,000,000. */
    first_numerator=e->batch.frame*COMPACT_FREQUENCY;
    last_numerator=(e->batch.frame+1)*COMPACT_FREQUENCY;
    assert(e->first==COMPACT_EPOCH+first_numerator/COMPACT_RATE+(first_numerator%COMPACT_RATE!=0));
    assert(e->last==COMPACT_EPOCH+last_numerator/COMPACT_RATE+(last_numerator%COMPACT_RATE!=0));
    assert(e->first==expected_window[ordinal][0]&&e->last==expected_window[ordinal][1]&&e->first<e->last);
    assert(a->slot==0);
    if(!ordinal){assert(a->kind==PT_SCHEDULED_TRIGGER&&a->words==COMPACT_FRAMES/2&&a->period==427&&a->volume==64);
        assert(((const int8_t *)a->data)[0]==127&&((const int8_t *)a->data)[1]==-128);
        assert(((const int8_t *)a->data)[2]==18&&((const int8_t *)a->data)[3]==-18);}
    else if(ordinal<3){assert(a->kind==PT_SCHEDULED_CONTROL&&!a->data&&!a->words&&a->volume==32);
        assert(a->period==(ordinal==1?427:426));++s->controls;}
    else assert(a->kind==PT_SCHEDULED_STOP&&!a->data&&!a->words&&!a->period&&!a->volume);
    s->last_frame=e->batch.frame;return m;
}
static void compact_observe(struct compact_state *s,struct compact_command *m)
{
    compact_fire(s,m);
    assert(pt_paula_readers_song_service_command(s->song,m->song_index,0,NULL)==PT_SCHEDULED_PENDING);
    assert(pt_paula_readers_song_service_reader(s->song,0,0,NULL)==PT_SCHEDULED_PENDING);
    compact_source_unchanged(s);
}
static void compact_detach(struct compact_state *s,struct compact_command *m)
{
    unsigned index=m->song_index;m->detach=1;
    assert(pt_paula_readers_song_service_command(s->song,index,0,NULL)==PT_SCHEDULED_OK);
    assert(!m->live);compact_source_unchanged(s);
}
static void compact_retire(struct compact_state *s)
{
    assert(s->backend.reader.live);s->backend.reader.retire=1;
    s->backend.reader.receipt.state=PT_READERS_RETIRED;
    assert(pt_paula_readers_song_service_reader(s->song,0,0,NULL)==PT_SCHEDULED_OK);
    assert(!s->backend.reader.live);compact_source_unchanged(s);
}
static void compact_success(const struct pt_compact_song_memory *memory,unsigned bits)
{
    struct compact_state *s=compact_source(memory,bits);struct compact_command *first,*second,*third,*stop;
    uint64_t frame;unsigned i,before_calls,chip_before,submissions;
    assert(compact_begin(s)==PT_PAULA_READERS_SONG_PENDING&&s->song&&s->scratch_discarded==1);
    compact_until_publish(s);assert(!s->backend.submissions&&!s->backend.effects);
    first=compact_publish(s,0);
    /* Submit acceptance is not actual adoption/ACTIVE. No next control until
     * the model positively acts in the original window and reports its key. */
    for(i=0;i<COMPACT_STEPS;++i)if(compact_step(s)==PT_PAULA_READERS_SONG_WAIT_ACTIVE)break;
    assert(i<COMPACT_STEPS&&!s->backend.effects);
    frame=s->status.boundary_frame;
    assert(compact_step(s)==PT_PAULA_READERS_SONG_WAIT_ACTIVE&&s->status.boundary_frame==frame);
    compact_observe(s,first);compact_until_publish(s);second=compact_publish(s,1);compact_observe(s,second);
    for(i=0;i<COMPACT_STEPS;++i)if(compact_step(s)==PT_PAULA_READERS_SONG_WAIT_PRESSURE)break;
    assert(i<COMPACT_STEPS&&s->status.command_mask==3&&s->status.reader_mask==1);
    frame=s->status.boundary_frame;before_calls=s->allocation->ordinary_calls;chip_before=s->allocation->chip_calls;
    assert(compact_step(s)==PT_PAULA_READERS_SONG_WAIT_PRESSURE&&s->status.boundary_frame==frame);
    assert(s->allocation->ordinary_calls==before_calls&&s->allocation->chip_calls==chip_before);++s->pressure;
    compact_detach(s,first);assert(s->backend.reader.live&&s->allocation->chip_live);
    compact_until_publish(s);third=compact_publish(s,2);compact_observe(s,third);
    compact_detach(s,second);compact_detach(s,third);
    for(i=0;i<COMPACT_STEPS;++i)if(compact_step(s)==PT_PAULA_READERS_SONG_DONE)break;
    assert(i<COMPACT_STEPS);
    assert(s->status.done&&s->status.phase==PT_PAULA_READERS_SONG_END&&
        s->status.terminal_frame==COMPACT_START+3U*COMPACT_TICK_FRAMES&&
        !s->status.command_mask&&s->status.reader_mask==1&&s->controls==2&&s->pressure==1);
    submissions=s->backend.submissions;
    assert(compact_step(s)==PT_PAULA_READERS_SONG_DONE&&s->backend.submissions==submissions);
    assert(!pt_paula_readers_song_close(&s->song)&&s->song&&s->backend.reader.live);
    assert(pt_paula_readers_song_terminal_stop(s->song,1)==PT_PAULA_READERS_SONG_PENDING);
    assert(pt_paula_readers_song_get(s->song,1,&s->status)==PT_PAULA_READERS_SONG_PENDING&&
        s->status.phase==PT_PAULA_READERS_SONG_KEYS);
    compact_until_publish(s);stop=compact_publish(s,3);
    compact_observe(s,stop);assert(!s->backend.active[0]&&s->allocation->chip_live);
    assert(!pt_paula_readers_song_close(&s->song));
    if(bits==16){
        /* Exact reader retirement alone cannot free storage still referenced
         * by the independently pending STOP command. */
        const struct pt_readers_output *q=stop->event->queue;
        compact_retire(s);
        assert(pt_readers_readers_held(q)==1&&pt_readers_commands_held(q)==1);
        assert(!pt_paula_readers_song_close(&s->song));compact_detach(s,stop);
        assert(!pt_readers_readers_held(q)&&!pt_readers_commands_held(q));
    }else{
        compact_detach(s,stop);assert(s->backend.reader.live&&s->allocation->chip_live);
        assert(!pt_paula_readers_song_close(&s->song));compact_retire(s);
    }
    assert(pt_paula_readers_song_get(s->song,1,&s->status)==PT_PAULA_READERS_SONG_DONE);
    assert(!s->status.command_mask&&!s->status.reader_mask&&s->backend.detached==4&&s->backend.retired==1&&s->backend.reused>=1);
    assert(pt_paula_readers_song_close(&s->song)&&!s->song);
    assert(s->phases&(1U<<PT_PAULA_READERS_SONG_INITIAL));
    assert(s->phases&(1U<<PT_PAULA_READERS_SONG_AUDIT));
    assert(s->phases&(1U<<PT_PAULA_READERS_SONG_FORECAST));
    printf("COMPACT SONG mono%u PASS: scratch released; original frame grid; 2 controls; STOP; independent domains; steps=%u\n",bits,s->steps);
    compact_source_close(s);
}
static void compact_protocol_refusal(const struct pt_compact_song_memory *memory)
{
    struct compact_state *s=compact_source(memory,8);
    struct pt_readers_backend old=s->config.backend;
    unsigned calls=s->allocation->ordinary_calls,chip=s->allocation->chip_calls;
    old.version=1;
    /* A refused output sentinel is never used as a genuine owner. */
    s->obsolete_queue=(void *)(uintptr_t)9;
    assert(PT_READERS_VERSION==2);
    assert(pt_readers_open(&s->allocator,&s->config.grid,s->config.session,&old,2,8,
        &s->obsolete_queue)==PT_SCHEDULED_UNSUPPORTED&&s->obsolete_queue==(void *)(uintptr_t)9);
    assert(s->allocation->ordinary_calls==calls&&s->allocation->chip_calls==chip&&
        !s->backend.submissions&&!s->backend.effects);
    compact_source_close(s);
}
static void compact_forged_binding(const struct pt_compact_song_memory *memory,unsigned mode)
{
    struct compact_state *s=compact_source(memory,24);struct compact_command *m;
    const struct pt_readers_output *q;unsigned i,ordinary,chip,command_polls,reader_polls;
    assert(mode<5&&compact_begin(s)==PT_PAULA_READERS_SONG_PENDING);
    compact_until_publish(s);
    if(mode==4){
        s->backend.submit_uncertain=1;
        assert(pt_paula_readers_song_publish_next(s->song,1)==PT_SCHEDULED_BACKEND);
        m=s->backend.command;assert(m->live&&m->event);m->song_index=0;
    }else m=compact_publish(s,0);
    q=m->event->queue;
    assert(pt_readers_commands_held(q)==1&&pt_readers_readers_held(q)==1);
    assert(!pt_paula_readers_song_close(&s->song)&&s->song);
    compact_fire(s,m);
    ordinary=s->allocation->ordinary_live;chip=s->allocation->chip_live;assert(chip);
    memset(&s->command_out,0xa7,sizeof(s->command_out));
    memcpy(&s->command_before,&s->command_out,sizeof(s->command_out));
    memset(&s->reader_out,0xb8,sizeof(s->reader_out));
    memcpy(&s->reader_before,&s->reader_out,sizeof(s->reader_out));
    if(mode<2||mode==4){
        s->backend.forge_command=mode==1?2:1;
        assert(pt_paula_readers_song_service_command(s->song,0,0,&s->command_out)==PT_SCHEDULED_BACKEND);
        s->backend.forge_command=0;
    }else{
        /* Observe the genuine original trigger before the forged reader proof. */
        assert(pt_paula_readers_song_service_command(s->song,0,0,NULL)==PT_SCHEDULED_PENDING);
        s->backend.forge_reader=mode==2?1:2;
        assert(pt_paula_readers_song_service_reader(s->song,0,0,&s->reader_out)==PT_SCHEDULED_BACKEND);
        s->backend.forge_reader=0;
    }
    assert(!memcmp(&s->command_out,&s->command_before,sizeof(s->command_out))&&
        !memcmp(&s->reader_out,&s->reader_before,sizeof(s->reader_out)));
    assert(pt_readers_commands_held(q)==1&&pt_readers_readers_held(q)==1&&
        s->allocation->ordinary_live==ordinary&&s->allocation->chip_live==chip);
    assert(m->live&&s->backend.reader.live&&!pt_paula_readers_song_close(&s->song));
    /* The public binding identifies protected control storage, not writable
     * output storage. Refusals precede both service poll callbacks; opaque
     * holders are not inspected or copied. External receipt before-images and
     * the full original source before-image are checked separately. */
    i=s->backend.clock_calls;command_polls=s->backend.command_polls;
    reader_polls=s->backend.reader_polls;
    assert(pt_paula_readers_song_service_command(s->song,0,0,
        (struct pt_readers_command_receipt *)m->event->binding.context)==PT_SCHEDULED_INVALID);
    assert(pt_paula_readers_song_service_reader(s->song,0,0,
        (struct pt_readers_reader_receipt *)s->backend.reader.reference->binding.context)==PT_SCHEDULED_INVALID);
    assert(pt_paula_readers_song_get(s->song,1,
        (struct pt_paula_readers_song_status *)s->master)==PT_PAULA_READERS_SONG_INVALID);
    assert(s->backend.clock_calls==i&&s->backend.command_polls==command_polls&&
        s->backend.reader_polls==reader_polls);
    assert(!memcmp(&s->command_out,&s->command_before,sizeof(s->command_out))&&
        !memcmp(&s->reader_out,&s->reader_before,sizeof(s->reader_out)));
    compact_source_unchanged(s);
    /* Sticky failure does not discard independently valid release proofs.
     * Command detachment alone must preserve the genuine master/cache reader. */
    m->detach=1;
    assert(pt_paula_readers_song_service_command(s->song,0,0,&s->command_out)==PT_SCHEDULED_BACKEND);
    assert(!m->live&&!pt_readers_commands_held(q)&&pt_readers_readers_held(q)==1&&
        s->backend.reader.live&&s->allocation->chip_live);
    s->backend.reader.retire=1;s->backend.reader.receipt.state=PT_READERS_RETIRED;
    assert(pt_paula_readers_song_service_reader(s->song,0,0,&s->reader_out)==PT_SCHEDULED_BACKEND);
    assert(!pt_readers_commands_held(q)&&!pt_readers_readers_held(q)&&!s->backend.reader.live);
    assert(!memcmp(&s->command_out,&s->command_before,sizeof(s->command_out))&&
        !memcmp(&s->reader_out,&s->reader_before,sizeof(s->reader_out)));
    assert(pt_paula_readers_song_cancel(s->song)==PT_PAULA_READERS_SONG_PENDING);
    assert(pt_paula_readers_song_close(&s->song)&&!s->song);
    compact_source_close(s);
}
int pt_readers_backend_registration_test(const struct pt_compact_song_memory *memory)
{
    unsigned mode;
    compact_success(memory,8);compact_success(memory,16);compact_success(memory,24);
    compact_protocol_refusal(memory);
    for(mode=0;mode<5;++mode)compact_forged_binding(memory,mode);
    puts("READERS BACKEND REGISTRATION PASS: public opaque bindings; genuine independent software ownership; no activation or hardware timing proof");
    return 0;
}

#ifndef PT_COMPACT_NO_MAIN
static void *compact_host_allocate(void *c,size_t n)
{(void)c;return malloc(n);}
static void compact_host_release(void *c,void *p)
{(void)c;free(p);}
static void compact_host_chip_release(void *c,void *p,size_t n)
{(void)n;compact_host_release(c,p);}
int main(void)
{
    const struct pt_compact_song_memory memory={{NULL,compact_host_allocate,compact_host_release},
        NULL,compact_host_allocate,compact_host_chip_release};
    return pt_readers_backend_registration_test(&memory);
}
#endif
