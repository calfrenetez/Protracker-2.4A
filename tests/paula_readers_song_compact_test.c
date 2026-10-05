/* RAM-only diagnostic, not an activation backend or the comprehensive suite.
 * Include only the actual queue implementation for the private registration
 * seam: backend receipts need genuine holder context+extent omitted by the
 * public event. Reader/song implementations are separate ordinary TUs.
 * Never fabricate, cast, copy, edit or release an owner through this seam.
 * All substantial fixture storage is heap-owned. The legacy large-stack song
 * constructor and every inherited fixture are deliberately outside this TU.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/core/scheduled_readers.c"
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
    uint64_t now;
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

/* Saved event/domain pointers must resolve the actual live registration.
 * These pure test observations do not call a queue API recursively. */
static const struct command_entry *compact_registered_command(const struct pt_readers_event *event)
{
    const struct pt_readers_output *q=event->queue;unsigned i;
    assert(q&&q->commands==2&&q->readers==8);
    for(i=0;i<q->commands;++i)if(q->command[i].held && &q->command[i].event==event)
        return q->command+i;
    assert(0);return NULL;
}
static const struct reader_entry *compact_registered_reader(const struct pt_readers_domain *d)
{
    const struct pt_readers_output *q=d->key.queue;unsigned i;
    assert(q);
    for(i=0;i<q->readers;++i)if(q->reader[i].held && &q->reader[i].domain==d&&
        equal_key(&q->reader[i].domain.key,&d->key))return q->reader+i;
    assert(0);return NULL;
}
static int compact_condition(struct compact_backend *b,const struct pt_readers_event *e)
{
    unsigned i;
    for(i=0;i<e->scheduled.batch.count;++i){
        const struct pt_readers_domain *d=e->reader[i];
        const struct reader_entry *r=compact_registered_reader(d);
        if(r->owner.control.current(r->owner.control.context,d->key.owner,d->key.generation)!=1)
            return 0;
        if(e->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER&&
           (!b->active[d->key.slot]||!equal_key(&b->slot[d->key.slot],&d->key)))return 0;
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
    const struct command_entry *registered;unsigned i;
    ++b->submissions;
    if(b->now>=e->scheduled.first||!compact_condition(b,e))return 0;
    for(i=0;i<2;++i)if(!b->command[i].live){m=b->command+i;break;}
    assert(m&&e->scheduled.batch.count==1);
    registered=compact_registered_command(e);
    memset(m,0,sizeof(*m));m->live=1;m->event=e;
    m->receipt.domain=PT_READERS_COMMAND_DOMAIN;m->receipt.origin=PT_READERS_BACKEND_ACTUAL;
    m->receipt.queue=e->queue;m->receipt.session=e->session;
    m->receipt.generation=e->scheduled.batch.generation;m->receipt.ticket=e->scheduled.ticket;
    m->receipt.owner=e->command_owner;m->receipt.count=e->scheduled.batch.count;
    m->receipt.event=e;m->receipt.context=registered->owner.context;
    m->receipt.context_bytes=registered->owner.context_bytes;
    for(i=0;i<m->receipt.count;++i){
        const struct pt_readers_domain *d=e->reader[i];
        m->receipt.action[i].key=d->key;
        if(e->scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER){
            const struct reader_entry *r=compact_registered_reader(d);
            assert(!b->reader.live);memset(&b->reader,0,sizeof(b->reader));
            b->reader.live=1;b->reader.reference=d;
            b->reader.receipt=(struct pt_readers_reader_receipt){PT_READERS_READER_DOMAIN,
                d->key,d,r->owner.control.context,r->owner.control.context_bytes,
                PT_READERS_RESERVED,PT_READERS_UNADOPTED,0,0};
        }else{
            assert(b->reader.live&&equal_key(&b->reader.receipt.key,&d->key));
            m->receipt.action[i].reader=PT_READERS_ACTIVE;
            m->receipt.action[i].adoption=PT_READERS_ADOPTED;
        }
    }
    return 1;
}
static enum pt_readers_reply compact_poll_command(void *c,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{
    struct compact_backend *b=c;unsigned i;struct compact_command *m=NULL;
    for(i=0;i<2;++i)if(b->command[i].live&&b->command[i].receipt.ticket==ticket)
        m=b->command+i;
    assert(m);
    if(!m->fired)return PT_READERS_PENDING;
    if(m->detach){
        if(b->reader.live&&b->reader.retire)m->receipt.action[0].reader=PT_READERS_RETIRED;
        *out=m->receipt;
        /* Positive command quiescence forgets ALL command-owned pointers.
         * The still-live reader registration is a separate reference domain. */
        memset(m,0,sizeof(*m));++b->detached;return PT_READERS_COMMAND_DETACHED;
    }
    *out=m->receipt;return PT_READERS_OBSERVATION;
}
static enum pt_readers_reply compact_poll_reader(void *c,const struct pt_readers_domain *d,
    struct pt_readers_reader_receipt *out)
{
    struct compact_backend *b=c;struct compact_reader *r=&b->reader;
    assert(r->live&&r->reference==d&&equal_key(&r->receipt.key,&d->key));
    if(r->receipt.adoption!=PT_READERS_ADOPTED)return PT_READERS_PENDING;
    *out=r->receipt;
    if(r->retire){
        /* Commands may still exist: preserve only numeric retired identity in
         * their already-issued receipt; drop every persistent domain pointer. */
        unsigned i;
        for(i=0;i<2;++i)if(b->command[i].live&&
            equal_key(&b->command[i].receipt.action[0].key,&r->receipt.key))
            b->command[i].receipt.action[0].reader=PT_READERS_RETIRED;
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
    assert(!s->status.command_mask&&!s->status.reader_mask&&s->backend.detached==4&&s->backend.retired==1);
    assert(pt_paula_readers_song_close(&s->song)&&!s->song);
    assert(s->phases&(1U<<PT_PAULA_READERS_SONG_INITIAL));
    assert(s->phases&(1U<<PT_PAULA_READERS_SONG_AUDIT));
    assert(s->phases&(1U<<PT_PAULA_READERS_SONG_FORECAST));
    printf("COMPACT SONG mono%u PASS: scratch released; original frame grid; 2 controls; STOP; independent domains; steps=%u\n",bits,s->steps);
    compact_source_close(s);
}
static void compact_budget_cancel(const struct pt_compact_song_memory *memory)
{
    struct compact_state *s=compact_source(memory,24);unsigned live=s->allocation->ordinary_live;
    s->config.control_budget=pt_paula_readers_song_control_size()-1;
    assert(compact_begin(s)==PT_PAULA_READERS_SONG_CAPACITY&&!s->song&&s->allocation->ordinary_live==live);
    s->config.control_budget=2U*1024U*1024U;
    /* A separately constructed queue lifetime never reuses the caller session. */
    ++s->config.session;
    assert(compact_begin(s)==PT_PAULA_READERS_SONG_PENDING&&s->song);
    assert(compact_step(s)==PT_PAULA_READERS_SONG_PENDING&&!s->backend.submissions&&!s->allocation->chip_calls);
    assert(pt_paula_readers_song_cancel(s->song)==PT_PAULA_READERS_SONG_PENDING);
    assert(pt_paula_readers_song_close(&s->song)&&!s->song&&s->allocation->ordinary_live==live);
    assert(!s->backend.submissions&&!s->backend.effects&&!s->allocation->chip_calls);
    puts("COMPACT SONG budget/cancel PASS: no backend publication or representation allocation");
    compact_source_close(s);
}
int pt_paula_readers_song_compact_test(const struct pt_compact_song_memory *memory)
{
    compact_success(memory,8);compact_success(memory,16);compact_success(memory,24);
    compact_budget_cancel(memory);
    puts("PAULA READERS SONG COMPACT PASS: caller workspace; genuine same-sequence software ownership; no DMA or hardware timing proof");
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
    return pt_paula_readers_song_compact_test(&memory);
}
#endif
