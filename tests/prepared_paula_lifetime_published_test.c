/* Private HOST-only published/cancel-before-activation fixture.
 * The exact committed donor supplies genuine acquisition, storage, save and
 * cleanup helpers. Its renamed main is NEVER called: none of its old cases,
 * counters, retained contexts or stdout are included in this fixture's oracle.
 * No production owner layout or owner callback is exposed or invoked here. */
#define main pt_published_donor_main_never_called
#include "prepared_paula_lifetime_test.c"
#undef main

enum pub_mode {PUB_EXACT,PUB_BAD_CLASS,PUB_PENDING,PUB_WRONG_ENVELOPE,PUB_REENTRY};
enum pub_case_kind {PUB_CLEAN,PUB_STALE,PUB_BAD_COMMAND,PUB_BAD_READER,
                    PUB_PENDING_FIRST,PUB_WRONG_FIRST,PUB_REENTER_FIRST};
struct pub_fixture;
struct pub_command_identity {
    const struct pt_readers_output *queue;
    const struct pt_readers_event *event_identity;
    struct pt_readers_binding binding;
    struct pt_readers_key key;
    uint64_t session,generation,ticket,owner;
    unsigned count;
};
struct pub_reader_identity {
    const struct pt_readers_domain *reference;
    struct pt_readers_binding binding;
    struct pt_readers_key key;
};
struct pub_backend {
    struct pub_fixture *fixture;
    uint64_t now,first,last,frame;
    uint32_t frequency;
    unsigned source_generation,refuse_submit;
    const struct pt_readers_event *activation_event;
    const struct pt_readers_domain *reader_reference;
    const uint8_t *sample_data;
    struct pt_scheduled_span spans[PT_PREPARED_PAULA_LIFETIME_DATA_SPANS];
    unsigned span_count,command_live,reader_live,activation_pending;
    struct pub_command_identity command;
    struct pub_reader_identity reader;
    enum pub_mode command_mode,reader_mode;
    unsigned clocks,submits,command_cancels,reader_cancels,polls,effects;
    unsigned command_drops,reader_drops,reentries;
    enum pt_scheduled_result nested;
};
struct pub_fixture {
    struct fixture *base;
    struct pub_backend backend;
    unsigned stale;
};
struct pub_counters {
    unsigned clocks,submits,command_cancels,reader_cancels,polls,effects;
};
static struct pub_fixture *volatile pub_retained[6];
static unsigned pub_retained_count,pub_completed,pub_stale,pub_semantic;
static unsigned pub_pending,pub_wrong,pub_reentry;
static unsigned pub_entry_guards;

static struct pub_counters pub_counts(const struct pub_backend *b)
{
    return (struct pub_counters){b->clocks,b->submits,b->command_cancels,
        b->reader_cancels,b->polls,b->effects};
}
static void pub_counts_equal(struct pub_counters a,struct pub_counters b)
{
    assert(a.clocks==b.clocks&&a.submits==b.submits&&
           a.command_cancels==b.command_cancels&&a.reader_cancels==b.reader_cancels&&
           a.polls==b.polls&&a.effects==b.effects);
}
static int pub_key_equal(const struct pt_readers_key *a,const struct pt_readers_key *b)
{
    return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&
        a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&
        a->action==b->action&&a->slot==b->slot;
}
static int pub_source_consumer_armed(const struct pub_backend *b)
{
    /* The model's only scheduled source-consumer state, separate from the
     * command envelope's opaque identity. No activation is exercised here. */
    return b->activation_pending&&b->activation_event&&b->reader_reference&&
        b->reader_live&&b->sample_data&&b->span_count;
}
static int pub_clock(void *context,uint64_t *tick,uint32_t *frequency)
{
    struct pub_backend *b=context;
    assert(b&&b->fixture&&tick&&frequency);++b->clocks;
    *tick=b->now;*frequency=b->frequency;return 1;
}
static int pub_submit(void *context,const struct pt_readers_event *e)
{
    struct pub_backend *b=context;struct fixture *f=b->fixture->base;
    const struct pt_readers_domain *d;unsigned i;
    ++b->submits;assert(!b->command_live&&!b->reader_live&&!b->activation_event);
    assert(e&&e->scheduled.batch.count==1&&e->reader[0]);d=e->reader[0];
    /* Independent fixed-grid oracle: epoch100 + ceil(frame*1000000/48000).
     * Both original boundaries remain exact; publication never moves frame100. */
    assert(e->scheduled.batch.frame==100&&e->scheduled.first==2184&&e->scheduled.last==2205);
    assert(f->config.frame==100&&f->config.grid.epoch==100&&
           f->config.grid.frequency==1000000&&f->config.grid.rate==48000);
    assert(b->now==100&&b->now<e->scheduled.first&&b->frequency==1000000);
    assert(f->sampler.generation==b->source_generation);
    assert(e->queue==d->key.queue&&e->session==f->config.session&&
           e->session==d->key.session&&e->scheduled.batch.generation==7&&
           d->key.generation==7&&d->key.trigger==e->scheduled.ticket&&
           d->key.action==0&&d->key.slot==0);
    assert(e->scheduled.batch.action[0].kind==PT_SCHEDULED_TRIGGER&&
           e->scheduled.batch.action[0].data&&e->scheduled.batch.action[0].words==PACKED/2&&
           e->scheduled.batch.action[0].period==124&&e->scheduled.batch.action[0].volume==48);
    assert(e->binding.context&&e->binding.context_bytes&&d->binding.context&&
           d->binding.context_bytes&&e->binding.context!=d->binding.context);
    assert(d->count&&d->count<=PT_PREPARED_PAULA_LIFETIME_DATA_SPANS&&d->spans);
    /* A genuine submit0 refuses with NO retained references or effects. */
    if(b->refuse_submit)return 0;
    b->command=(struct pub_command_identity){e->queue,e,e->binding,d->key,
        e->session,e->scheduled.batch.generation,e->scheduled.ticket,e->command_owner,
        e->scheduled.batch.count};
    b->reader=(struct pub_reader_identity){d,d->binding,d->key};
    b->activation_event=e;b->reader_reference=d;
    b->sample_data=e->scheduled.batch.action[0].data;b->span_count=d->count;
    for(i=0;i<d->count;++i){assert(d->spans[i].data&&d->spans[i].bytes);b->spans[i]=d->spans[i];}
    b->first=e->scheduled.first;b->last=e->scheduled.last;b->frame=e->scheduled.batch.frame;
    b->command_live=b->reader_live=b->activation_pending=1;
    assert(pub_source_consumer_armed(b));
    /* This real model now retains the actual original C and R references.
     * No owner binding is dereferenced; no callback or receipt is forged. */
    return 1;
}
static void pub_drop_command(struct pub_backend *b)
{
    assert(b->command_live&&b->command.event_identity&&b->command.binding.context);
    /* Cancel actual pending activation before giving up command borrowing.
     * The persistent reader/data obligation remains until its own retirement. */
    b->activation_pending=0;b->activation_event=NULL;b->command_live=0;
    memset(&b->command,0,sizeof(b->command));++b->command_drops;
    assert(!pub_source_consumer_armed(b));
}
static void pub_drop_reader(struct pub_backend *b)
{
    assert(b->reader_live&&b->reader_reference&&b->reader.reference&&b->sample_data&&b->span_count);
    /* R-first must remove ALL model data/span/domain/activation references.
     * Remaining C identity is only an opaque event address + scalar envelope:
     * it has no geometry or reader pointer and is never dereferenced. */
    b->activation_pending=0;b->activation_event=NULL;b->reader_reference=NULL;
    b->sample_data=NULL;memset(b->spans,0,sizeof(b->spans));b->span_count=0;
    b->reader_live=0;memset(&b->reader,0,sizeof(b->reader));++b->reader_drops;
}
static void pub_assert_reader_dropped(const struct pub_backend *b)
{
    unsigned i;assert(!b->reader_live&&!b->reader_reference&&!b->reader.reference&&
        !b->reader.binding.context&&!b->reader.binding.context_bytes&&
        !b->activation_pending&&!b->activation_event&&!b->sample_data&&!b->span_count);
    assert(!pub_source_consumer_armed(b));
    for(i=0;i<PT_PREPARED_PAULA_LIFETIME_DATA_SPANS;++i)
        assert(!b->spans[i].data&&!b->spans[i].bytes);
}
static void pub_nested(struct pub_backend *b)
{
    ++b->reentries;
    b->nested=pt_prepared_paula_lifetime_cancel_command_once(b->fixture->base->lifetime);
    assert(b->nested==PT_SCHEDULED_BACKEND);
}
static enum pt_readers_reply pub_cancel_command(void *context,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{
    struct pub_backend *b=context;struct pt_readers_command_receipt r;
    enum pub_mode mode=b->command_mode;
    ++b->command_cancels;assert(b->command_live&&ticket==b->command.ticket&&out);
    if(mode==PUB_PENDING)return PT_READERS_PENDING;
    memset(&r,0,sizeof(r));r.domain=PT_READERS_COMMAND_DOMAIN;r.origin=PT_READERS_BACKEND_ACTUAL;
    r.queue=b->command.queue;r.session=b->command.session;r.generation=b->command.generation;
    r.ticket=b->command.ticket;r.owner=b->command.owner;r.count=b->command.count;
    r.event=b->command.event_identity;r.context=b->command.binding.context;
    r.context_bytes=b->command.binding.context_bytes;
    r.action[0].key=b->command.key;r.action[0].command=PT_READERS_CANCELLED_BEFORE;
    r.action[0].reader=PT_READERS_NONE;r.action[0].adoption=PT_READERS_UNADOPTED;
    assert(!r.action[0].observed&&!r.action[0].issued);
    if(mode==PUB_REENTRY)pub_nested(b);
    pub_drop_command(b); /* Real stop/drop precedes even fault-injected envelopes. */
    assert(!b->activation_event&&!b->command_live&&!b->command.event_identity);
    if(mode==PUB_BAD_CLASS)r.action[0].command=PT_READERS_WAITING;
    if(mode==PUB_WRONG_ENVELOPE){assert(r.session<UINT64_MAX);++r.session;}
    *out=r;return PT_READERS_COMMAND_DETACHED;
}
static enum pt_readers_reply pub_cancel_reader(void *context,const struct pt_readers_domain *d,
    struct pt_readers_reader_receipt *out)
{
    struct pub_backend *b=context;struct pt_readers_reader_receipt r;
    enum pub_mode mode=b->reader_mode;
    ++b->reader_cancels;assert(b->reader_live&&d==b->reader_reference&&out);
    assert(pub_key_equal(&d->key,&b->reader.key));
    if(mode==PUB_PENDING)return PT_READERS_PENDING;
    memset(&r,0,sizeof(r));r.domain=PT_READERS_READER_DOMAIN;r.key=b->reader.key;
    r.reference=b->reader.reference;r.context=b->reader.binding.context;
    r.context_bytes=b->reader.binding.context_bytes;
    r.state=PT_READERS_RETIRED;r.adoption=PT_READERS_UNADOPTED;
    assert(!r.observed&&!r.issued);
    if(mode==PUB_REENTRY)pub_nested(b);
    pub_drop_reader(b);pub_assert_reader_dropped(b);
    if(mode==PUB_BAD_CLASS)r.state=PT_READERS_RESERVED;
    if(mode==PUB_WRONG_ENVELOPE){assert(r.key.serial<UINT64_MAX);++r.key.serial;}
    *out=r;return PT_READERS_READER_RETIRED;
}
static enum pt_readers_reply pub_poll_command(void *context,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{struct pub_backend *b=context;(void)ticket;(void)out;++b->polls;assert(0);return PT_READERS_UNCERTAIN;}
static enum pt_readers_reply pub_poll_reader(void *context,const struct pt_readers_domain *d,
    struct pt_readers_reader_receipt *out)
{struct pub_backend *b=context;(void)d;(void)out;++b->polls;assert(0);return PT_READERS_UNCERTAIN;}

static struct pub_fixture *pub_ready(void)
{
    struct pub_fixture *p=calloc(1,sizeof(*p));struct fixture *f;struct arena *a;
    assert(p);f=ready();p->base=f;p->backend.fixture=p;p->backend.now=100;
    p->backend.frequency=1000000;p->backend.source_generation=f->sampler.generation;
    config_begin(f);a=lifetime_arena(f);
    f->backend=(struct pt_readers_backend){&p->backend,sizeof(p->backend),
        {PT_SCHEDULED_REQUIRED,1,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        pub_clock,pub_submit,pub_poll_command,pub_cancel_command,pub_poll_reader,pub_cancel_reader};
    f->config.context_count=2;
    f->config.context[1]=(struct pt_prepared_paula_context_extent){p,sizeof(*p)};
    assert(pt_prepared_paula_lifetime_init(f->lifetime,a->capacity,f->owner,
        &f->config,&f->backend)==PT_PREPARED_PAULA_SOURCES_OK);
    assert(pt_prepared_paula_lifetime_enqueue_unpublished(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    assert(pt_prepared_paula_lifetime_failure(f->lifetime)==PT_SCHEDULED_OK);
    assert(!p->backend.clocks&&!p->backend.submits&&!p->backend.polls);
    assert(status(f).master_pins==3&&status(f).chip_leases==3&&pins(f)==3);
    return p;
}
static void pub_claimed(struct pub_fixture *p)
{
    struct pub_counters c=pub_counts(&p->backend);struct fixture *f=p->base;
    claimed_cancel_refuses(f);pub_counts_equal(c,pub_counts(&p->backend));
    assert(pins(f)==3&&f->sampler.live_count==3&&f->chip_bytes==3*PACKED);
    if(!p->stale){struct pt_prepared_paula_sources_status s=status(f);
        assert(s.master_pins==3&&s.chip_leases==3&&!s.master_job&&!s.chip_job);}
    master_save_equal(f);
}
static void pub_no_early_drain(struct pub_fixture *p)
{
    struct fixture *f=p->base;struct audit a;struct pub_counters c=pub_counts(&p->backend);
    unsigned fast=f->fast_calls,chip=f->chip_calls,fr=f->fast_releases,cr=f->chip_releases;
    audit_capture(f,&a,1);
    assert(pt_prepared_paula_lifetime_drain_step(f->lifetime)==PT_PREPARED_PAULA_SOURCES_BUSY);
    audit_equal_release(&a);no_resource_delta(f,fast,chip,fr,cr);
    pub_counts_equal(c,pub_counts(&p->backend));pub_claimed(p);
}
static void pub_publish_and_stop(struct pub_fixture *p)
{
    struct fixture *f=p->base;struct audit a;struct pub_counters c;
    audit_capture(f,&a,1);
    assert(pt_prepared_paula_lifetime_publish_once(f->lifetime)==PT_SCHEDULED_OK);
    audit_equal_release(&a);
    assert(p->backend.clocks==1&&p->backend.submits==1&&!p->backend.effects&&
           p->backend.command_live&&p->backend.reader_live&&p->backend.activation_pending);
    pub_claimed(p);c=pub_counts(&p->backend);
    assert(pt_prepared_paula_lifetime_publish_once(f->lifetime)==PT_SCHEDULED_INVALID);
    pub_counts_equal(c,pub_counts(&p->backend));
    audit_capture(f,&a,1);
    assert(pt_prepared_paula_lifetime_stop_publication(f->lifetime)==PT_SCHEDULED_PENDING);
    audit_equal_release(&a);pub_counts_equal(c,pub_counts(&p->backend));
    assert(pt_prepared_paula_lifetime_stop_publication(f->lifetime)==PT_SCHEDULED_INVALID);
    pub_counts_equal(c,pub_counts(&p->backend));pub_no_early_drain(p);
}
static enum pt_scheduled_result pub_cancel(struct pub_fixture *p,unsigned reader)
{
    struct fixture *f=p->base;struct audit a;struct pub_counters before=pub_counts(&p->backend),after;
    enum pt_scheduled_result result;unsigned fast=f->fast_calls,chip=f->chip_calls;
    unsigned fr=f->fast_releases,cr=f->chip_releases;
    audit_capture(f,&a,1);
    result=reader?pt_prepared_paula_lifetime_cancel_reader_once(f->lifetime):
                  pt_prepared_paula_lifetime_cancel_command_once(f->lifetime);
    audit_equal_release(&a);no_resource_delta(f,fast,chip,fr,cr);after=pub_counts(&p->backend);
    assert(after.clocks==before.clocks&&after.submits==before.submits&&
           after.polls==before.polls&&after.effects==before.effects);
    assert(after.command_cancels==before.command_cancels+(reader?0:1)&&
           after.reader_cancels==before.reader_cancels+(reader?1:0));
    pub_claimed(p);return result;
}
static void pub_repeat_refuses(struct pub_fixture *p,unsigned reader)
{
    struct fixture *f=p->base;struct arena *sa=f->arena+f->arena_count-1;
    struct arena *la=f->lifetime_arena+f->lifetime_count-1;
    unsigned char *source=malloc(sa->capacity),*life=malloc(la->capacity);
    unsigned char *before=malloc(sizeof(*f)),*backend=malloc(sizeof(p->backend));
    assert(source&&life&&before&&backend);
    memcpy(source,sa->data,sa->capacity);memcpy(life,la->data,la->capacity);
    memcpy(before,f,sizeof(*f));memcpy(backend,&p->backend,sizeof(p->backend));
    assert((reader?pt_prepared_paula_lifetime_cancel_reader_once(f->lifetime):
        pt_prepared_paula_lifetime_cancel_command_once(f->lifetime))==PT_SCHEDULED_INVALID);
    assert(!memcmp(source,sa->data,sa->capacity)&&!memcmp(life,la->data,la->capacity));
    assert(!memcmp(before,f,sizeof(*f))&&!memcmp(backend,&p->backend,sizeof(p->backend)));
    free(source);free(life);free(before);free(backend);
}
static void pub_drain_and_finish(struct pub_fixture *p,enum pt_scheduled_result failure)
{
    struct fixture *f=p->base;struct pub_counters c=pub_counts(&p->backend);
    static const unsigned expected_cache_pins[7]={2,2,1,1,0,0,0};
    const unsigned stale=p->stale;
    unsigned n;enum pt_prepared_paula_sources_result result=PT_PREPARED_PAULA_SOURCES_PENDING;
    assert(!p->backend.command_live&&!p->backend.reader_live&&!p->backend.activation_event);
    pub_assert_reader_dropped(&p->backend);
    assert(pt_prepared_paula_lifetime_failure(f->lifetime)==failure);
    for(n=0;n<7;++n){
        struct pt_prepared_paula_sources_status s,before;
        unsigned fast=f->fast_calls,chip=f->chip_calls,fr=f->fast_releases,cr=f->chip_releases;
        if(!stale)before=status(f);
        result=pt_prepared_paula_lifetime_drain_step(f->lifetime);
        assert(result==(n==6?PT_PREPARED_PAULA_SOURCES_CLOSED:PT_PREPARED_PAULA_SOURCES_PENDING));
        no_resource_delta(f,fast,chip,fr,cr);pub_counts_equal(c,pub_counts(&p->backend));
        assert(pins(f)==expected_cache_pins[n]);
        if(!stale){s=status(f);assert(s.master_pins+s.chip_leases==
            (before.master_pins+before.chip_leases?before.master_pins+before.chip_leases-1:0));}
        assert(pt_prepared_paula_lifetime_failure(f->lifetime)==failure);master_save_equal(f);
    }
    assert(result==PT_PREPARED_PAULA_SOURCES_CLOSED&&!pins(f));
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    assert(pt_prepared_paula_lifetime_failure(f->lifetime)==failure);
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    pub_counts_equal(c,pub_counts(&p->backend));
    lifetime_tail_equal(f->lifetime_arena+f->lifetime_count-1);
    assert(p->stale==stale);
    if(stale)assert(f->sampler.generation==p->backend.source_generation+1);
    finish(f);p->base=NULL;free(p);++pub_completed;
}
static void pub_retain(struct pub_fixture *p)
{
    struct fixture *f=p->base;struct pub_counters c=pub_counts(&p->backend);
    assert(pub_retained_count<6);pub_no_early_drain(p);
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_FAULT);
    pub_counts_equal(c,pub_counts(&p->backend));pub_claimed(p);
    assert(f->control.queue&&f->fast_bytes&&f->chip_bytes==3*PACKED);
    pub_retained[pub_retained_count++]=p;
}
static void pub_case(enum pub_case_kind kind,unsigned reader_first)
{
    struct pub_fixture *p=pub_ready();struct fixture *f=p->base;
    struct pub_backend *b=&p->backend;enum pt_scheduled_result first,second,failure;
    pub_publish_and_stop(p);
    if(kind==PUB_STALE){
        struct pt_prepared_paula_sources_status out,saved;
        assert(f->sampler.generation<UINT32_MAX);++f->sampler.generation;p->stale=1;
        memset(&out,0x6d,sizeof(out));saved=out;
        assert(pt_prepared_paula_sources_get(f->owner,&out)==PT_PREPARED_PAULA_SOURCES_STALE);
        assert(!memcmp(&out,&saved,sizeof(out)));++pub_stale;
    }
    if(kind==PUB_BAD_COMMAND)b->command_mode=PUB_BAD_CLASS;
    if(kind==PUB_BAD_READER)b->reader_mode=PUB_BAD_CLASS;
    if(kind==PUB_PENDING_FIRST||kind==PUB_WRONG_FIRST||kind==PUB_REENTER_FIRST){
        enum pub_mode mode=kind==PUB_PENDING_FIRST?PUB_PENDING:
            kind==PUB_WRONG_FIRST?PUB_WRONG_ENVELOPE:PUB_REENTRY;
        if(reader_first)b->reader_mode=mode;else b->command_mode=mode;
    }
    first=pub_cancel(p,reader_first);pub_repeat_refuses(p,reader_first);
    pub_no_early_drain(p);
    if(reader_first&&b->reader_mode!=PUB_PENDING)pub_assert_reader_dropped(b);
    if(kind==PUB_PENDING_FIRST){
        assert(first==PT_SCHEDULED_PENDING&&b->command_live&&b->reader_live&&b->activation_pending);
        assert(pt_prepared_paula_lifetime_failure(f->lifetime)==PT_SCHEDULED_OK);
        ++pub_pending;pub_retain(p);return;
    }
    if(kind==PUB_REENTER_FIRST){
        struct pub_counters c=pub_counts(b);
        assert(first==PT_SCHEDULED_BACKEND&&b->reentries==1&&b->nested==PT_SCHEDULED_BACKEND);
        assert((reader_first?pt_prepared_paula_lifetime_cancel_command_once(f->lifetime):
             pt_prepared_paula_lifetime_cancel_reader_once(f->lifetime))==PT_SCHEDULED_BACKEND);
        pub_counts_equal(c,pub_counts(b));assert(pt_prepared_paula_lifetime_failure(f->lifetime)==PT_SCHEDULED_BACKEND);
        ++pub_reentry;pub_retain(p);return;
    }
    second=pub_cancel(p,!reader_first);pub_repeat_refuses(p,!reader_first);
    assert(b->command_cancels==1&&b->reader_cancels==1&&!b->polls&&!b->effects);
    if(kind==PUB_WRONG_FIRST){
        assert(first==PT_SCHEDULED_BACKEND&&second==PT_SCHEDULED_BACKEND);
        assert(pt_prepared_paula_lifetime_failure(f->lifetime)==PT_SCHEDULED_BACKEND);
        ++pub_wrong;pub_retain(p);return;
    }
    failure=(kind==PUB_BAD_COMMAND||kind==PUB_BAD_READER)?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK;
    if(failure==PT_SCHEDULED_OK)assert(first==PT_SCHEDULED_OK&&second==PT_SCHEDULED_OK);
    else{
        assert(first==(reader_first?(kind==PUB_BAD_READER?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK):
            (kind==PUB_BAD_COMMAND?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK)));
        assert(second==PT_SCHEDULED_BACKEND);++pub_semantic;
    }
    assert(b->command_drops==1&&b->reader_drops==1);
    pub_drain_and_finish(p,failure);
}
enum pub_guard_kind {PUB_STALE_BEFORE_PUBLISH,PUB_SUBMIT_REFUSED,PUB_CLOCK_AT_FIRST};
static void pub_guard_case(enum pub_guard_kind kind)
{
    struct pub_fixture *p=pub_ready();struct fixture *f=p->base;
    struct pub_backend *b=&p->backend;struct audit a;struct pub_counters counts;
    enum pt_scheduled_result failure,result,stopped;
    if(kind==PUB_STALE_BEFORE_PUBLISH){
        assert(f->sampler.generation<UINT32_MAX);++f->sampler.generation;p->stale=1;
        /* No getter/step pre-latches a source failure: the new serialized
         * stage2 publication check must itself detect the changed generation. */
        failure=PT_SCHEDULED_STALE;
    }else if(kind==PUB_SUBMIT_REFUSED){b->refuse_submit=1;failure=PT_SCHEDULED_OK;}
    else {b->now=2184;failure=PT_SCHEDULED_LATE;}
    audit_capture(f,&a,1);
    result=pt_prepared_paula_lifetime_publish_once(f->lifetime);
    assert(result==(kind==PUB_SUBMIT_REFUSED?PT_SCHEDULED_PENDING:failure));
    audit_equal_release(&a);
    if(kind==PUB_STALE_BEFORE_PUBLISH){
        struct pt_prepared_paula_sources_status out,saved;
        memset(&out,0x4e,sizeof(out));saved=out;
        assert(pt_prepared_paula_sources_get(f->owner,&out)==PT_PREPARED_PAULA_SOURCES_STALE);
        assert(!memcmp(&out,&saved,sizeof(out)));
    }
    pub_claimed(p);
    assert(b->clocks==(kind==PUB_STALE_BEFORE_PUBLISH?0U:1U));
    assert(b->submits==(kind==PUB_SUBMIT_REFUSED?1U:0U));
    assert(!b->effects&&!b->command_live&&!b->reader_live&&!b->activation_pending&&
           !b->activation_event&&!b->reader_reference&&!b->sample_data&&!b->span_count);
    assert(pt_prepared_paula_lifetime_failure(f->lifetime)==failure);
    counts=pub_counts(b);
    assert(pt_prepared_paula_lifetime_publish_once(f->lifetime)==PT_SCHEDULED_INVALID);
    pub_counts_equal(counts,pub_counts(b));pub_no_early_drain(p);
    audit_capture(f,&a,1);stopped=pt_prepared_paula_lifetime_stop_publication(f->lifetime);
    assert(stopped==(kind==PUB_CLOCK_AT_FIRST?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK));
    audit_equal_release(&a);pub_counts_equal(counts,pub_counts(b));
    assert(pt_prepared_paula_lifetime_failure(f->lifetime)==failure);
    /* Only the genuine core local stop consumed these never-submitted domains.
     * No backend cancellation or fabricated quiet is involved. */
    assert(!b->command_cancels&&!b->reader_cancels&&!b->polls);
    pub_drain_and_finish(p,failure);++pub_entry_guards;
}
int main(void)
{
    unsigned order,i;
    for(order=0;order<2;++order){
        pub_case(PUB_CLEAN,order);pub_case(PUB_STALE,order);
        pub_case(PUB_BAD_COMMAND,order);pub_case(PUB_BAD_READER,order);
        pub_case(PUB_PENDING_FIRST,order);pub_case(PUB_WRONG_FIRST,order);
        pub_case(PUB_REENTER_FIRST,order);
    }
    pub_guard_case(PUB_STALE_BEFORE_PUBLISH);pub_guard_case(PUB_SUBMIT_REFUSED);
    pub_guard_case(PUB_CLOCK_AT_FIRST);
    assert(pub_completed==11&&pub_entry_guards==3&&pub_stale==2&&pub_semantic==4&&pub_retained_count==6&&
           pub_pending==2&&pub_wrong==2&&pub_reentry==2);
    for(i=0;i<6;++i){assert(pub_retained[i]&&pub_retained[i]->base);
        if(i)assert(pub_retained[i]!=pub_retained[i-1]);}
    assert(!retained_count&&!clean_cases&&!stale_cases&&!alias_cases);
    puts("PREPARED PAULA PUBLISHED HOST PASS:17 cases;11 completed;6 retained;actual submit/cancel C-first/R-first;fresh publication/refusal/late guards;unchanged frame/window;mono8/16/24 held through deferred drain;sticky semantic failure;NO OUTPUT AUTHORITY");
    return 0;
}
