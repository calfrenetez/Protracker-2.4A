/* Private HOST-only ACTIVE/adopted-reader fixture. The exact acquisition
 * donor's renamed main is NEVER executed or counted. No owner layout, owner
 * callback, manufactured pin count or hardware/timing authority is used. */
#define main pt_active_donor_main_never_called
#include "prepared_paula_lifetime_test.c"
#undef main

enum active_mode {ACTIVE_EXACT,ACTIVE_BAD_CLASS,ACTIVE_PENDING,
                  ACTIVE_WRONG_ENVELOPE,ACTIVE_UNCERTAIN};
struct active_fixture;
struct active_command_identity {
    const struct pt_readers_output *queue;
    const struct pt_readers_event *event_identity;
    struct pt_readers_binding binding;
    struct pt_readers_key key;
    uint64_t session,generation,ticket,owner;
    unsigned count;
};
struct active_reader_identity {
    const struct pt_readers_domain *reference;
    struct pt_readers_binding binding;
    struct pt_readers_key key;
};
struct active_backend {
    struct active_fixture *fixture;
    uint64_t now,first,last,frame,observed,issued;
    uint32_t frequency;
    unsigned source_generation,callback_depth;
    const struct pt_readers_event *activation_event;
    const struct pt_readers_domain *reader_reference;
    /* Actual persistent-reader geometry, independent of command storage. */
    struct pt_scheduled_action action;
    struct pt_scheduled_span spans[PT_PREPARED_PAULA_LIFETIME_DATA_SPANS];
    unsigned span_count,command_live,reader_live,activation_pending,adopted,active;
    struct active_command_identity command;
    struct active_reader_identity reader;
    enum active_mode reader_mode;
    unsigned clocks,submits,dispatches,reads,read_bytes,command_cancels,reader_cancels,polls;
    unsigned command_drops,reader_drops;
};
struct active_fixture {struct fixture *base;struct active_backend backend;unsigned stale;};
struct active_counters {
    unsigned clocks,submits,dispatches,reads,read_bytes,command_cancels,reader_cancels,polls;
};
static struct active_fixture *volatile active_retained[3];
static unsigned active_retained_count,active_completed,active_stale,active_semantic;
static unsigned active_pending,active_wrong,active_uncertain;
static unsigned active_dispatch_total,active_read_total;

static struct active_counters active_counts(const struct active_backend *b)
{
    return (struct active_counters){b->clocks,b->submits,b->dispatches,b->reads,
        b->read_bytes,b->command_cancels,b->reader_cancels,b->polls};
}
static void active_counts_equal(struct active_counters a,struct active_counters b)
{
    assert(a.clocks==b.clocks&&a.submits==b.submits&&a.dispatches==b.dispatches&&
        a.reads==b.reads&&a.read_bytes==b.read_bytes&&a.command_cancels==b.command_cancels&&
        a.reader_cancels==b.reader_cancels&&a.polls==b.polls);
}
static int active_key_equal(const struct pt_readers_key *a,const struct pt_readers_key *b)
{
    return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&
        a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&
        a->action==b->action&&a->slot==b->slot;
}
static int active_consumer_live(const struct active_backend *b)
{
    return b->adopted&&b->active&&b->reader_live&&b->reader_reference&&
        b->reader.reference&&b->action.data&&b->span_count;
}
static void active_callback_enter(struct active_backend *b)
{assert(!b->callback_depth);b->callback_depth=1;}
static void active_callback_leave(struct active_backend *b)
{assert(b->callback_depth==1);b->callback_depth=0;}
static int active_clock(void *context,uint64_t *tick,uint32_t *frequency)
{
    struct active_backend *b=context;
    assert(b&&b->fixture&&tick&&frequency);active_callback_enter(b);++b->clocks;
    *tick=b->now;*frequency=b->frequency;active_callback_leave(b);return 1;
}
static void active_original_event(const struct active_backend *b,const struct pt_readers_event *e)
{
    const struct fixture *f=b->fixture->base;const struct pt_readers_domain *d;
    const struct pt_scheduled_action *a;
    assert(e&&e->scheduled.batch.count==1&&e->reader[0]);d=e->reader[0];
    assert(e->scheduled.batch.frame==100&&e->scheduled.first==2184&&e->scheduled.last==2205);
    assert(f->config.frame==100&&f->config.grid.epoch==100&&f->config.grid.frequency==1000000&&
        f->config.grid.rate==48000&&b->frequency==1000000);
    assert(f->sampler.generation==b->source_generation);
    assert(e->queue==d->key.queue&&e->session==f->config.session&&e->session==d->key.session&&
        e->scheduled.batch.generation==7&&d->key.generation==7&&
        d->key.trigger==e->scheduled.ticket&&d->key.action==0&&d->key.slot==0);
    a=e->scheduled.batch.action;
    assert(a->kind==PT_SCHEDULED_TRIGGER&&a->slot==0&&a->data&&a->words==PACKED/2&&
        a->period==124&&a->volume==48);
    assert(e->binding.context&&e->binding.context_bytes&&d->binding.context&&
        d->binding.context_bytes&&e->binding.context!=d->binding.context);
    assert(d->spans&&d->count&&d->count<=PT_PREPARED_PAULA_LIFETIME_DATA_SPANS);
}
static int active_submit(void *context,const struct pt_readers_event *e)
{
    struct active_backend *b=context;const struct pt_readers_domain *d;unsigned i;
    active_callback_enter(b);++b->submits;
    assert(!b->command_live&&!b->reader_live&&!b->activation_event&&!b->adopted);
    active_original_event(b,e);d=e->reader[0];
    assert(b->now==100&&b->now<e->scheduled.first);
    b->command=(struct active_command_identity){e->queue,e,e->binding,d->key,
        e->session,e->scheduled.batch.generation,e->scheduled.ticket,e->command_owner,
        e->scheduled.batch.count};
    b->reader=(struct active_reader_identity){d,d->binding,d->key};
    b->activation_event=e;b->reader_reference=d;b->action=e->scheduled.batch.action[0];
    b->span_count=d->count;
    for(i=0;i<d->count;++i){assert(d->spans[i].data&&d->spans[i].bytes);b->spans[i]=d->spans[i];}
    b->first=e->scheduled.first;b->last=e->scheduled.last;b->frame=e->scheduled.batch.frame;
    b->command_live=b->reader_live=b->activation_pending=1;
    /* Publication retained the real originals; it did not activate/adopt. */
    assert(!active_consumer_live(b)&&!b->observed&&!b->issued&&!b->dispatches&&!b->reads);
    active_callback_leave(b);return 1;
}
static void active_dispatch(struct active_backend *b)
{
    const struct pt_readers_event *e=b->activation_event;const struct pt_readers_domain *d;
    const struct pt_scheduled_action *a;unsigned i,covered=0;
    assert(!b->callback_depth&&b->command_live&&b->reader_live&&b->activation_pending&&
        !b->adopted&&!b->active&&!b->dispatches&&b->now==2184);
    /* Ordinary serialized MODEL dispatch, outside every core callback. All
     * original identities/geometry/currentness validate before a model effect. */
    active_original_event(b,e);d=e->reader[0];a=e->scheduled.batch.action;
    assert(e==b->command.event_identity&&d==b->reader_reference&&d==b->reader.reference&&
        active_key_equal(&d->key,&b->reader.key)&&e->scheduled.ticket==b->command.ticket&&
        e->command_owner==b->command.owner&&e->binding.context==b->command.binding.context&&
        e->binding.context_bytes==b->command.binding.context_bytes&&
        d->binding.context==b->reader.binding.context&&d->binding.context_bytes==b->reader.binding.context_bytes);
    assert(e->scheduled.first==b->first&&e->scheduled.last==b->last&&
        e->scheduled.batch.frame==b->frame&&b->now>=b->first&&b->now<b->last);
    assert(a->kind==b->action.kind&&a->slot==b->action.slot&&a->data==b->action.data&&
        a->words==b->action.words&&a->period==b->action.period&&a->volume==b->action.volume&&
        d->count==b->span_count);
    for(i=0;i<d->count;++i){
        uintptr_t start=(uintptr_t)b->spans[i].data,pcm=(uintptr_t)b->action.data;
        assert(d->spans[i].data==b->spans[i].data&&d->spans[i].bytes==b->spans[i].bytes);
        if(pcm>=start&&b->spans[i].bytes>=PACKED&&pcm-start<=b->spans[i].bytes-PACKED)covered=1;
    }
    assert(covered);
    b->observed=b->issued=b->now;b->adopted=b->active=1;
    b->activation_event=NULL;b->activation_pending=0;++b->dispatches;++active_dispatch_total;
    assert(active_consumer_live(b)&&b->observed==2184&&b->issued==2184);
}
static void active_read_first_cache(struct active_backend *b,unsigned after_detach)
{
    const struct fixture *f=b->fixture->base;unsigned i;
    assert(!b->callback_depth&&active_consumer_live(b)&&!b->activation_event&&
        !b->activation_pending&&b->dispatches==1&&!b->reads&&b->action.words*2U==PACKED);
    if(after_detach)assert(!b->command_live&&!b->command.event_identity&&b->command_drops==1);
    /* This actual bounded read uses only independent persistent-R geometry.
     * No command event or owner/control context is traversed. */
    for(i=0;i<PACKED;++i)assert(b->action.data[i]==f->gold[0][i]);
    ++b->reads;b->read_bytes=PACKED;++active_read_total;
}
static void active_drop_command(struct active_backend *b)
{
    assert(b->command_live&&b->command.event_identity&&b->command.binding.context&&
        b->adopted&&!b->activation_event&&!b->activation_pending);
    b->command_live=0;memset(&b->command,0,sizeof(b->command));++b->command_drops;
    if(b->reader_live)assert(active_consumer_live(b));
}
static void active_drop_reader(struct active_backend *b)
{
    assert(active_consumer_live(b)&&b->reader.reference&&b->reader.binding.context);
    /* End every actual R consumer/geometry/domain/activation borrowing BEFORE
     * the retirement envelope. Remaining C identity is opaque scalars only. */
    b->active=0;b->activation_pending=0;b->activation_event=NULL;b->reader_reference=NULL;
    memset(&b->action,0,sizeof(b->action));memset(b->spans,0,sizeof(b->spans));
    b->span_count=0;b->reader_live=0;memset(&b->reader,0,sizeof(b->reader));++b->reader_drops;
}
static void active_assert_reader_dropped(const struct active_backend *b)
{
    unsigned i;
    assert(!active_consumer_live(b)&&!b->active&&!b->reader_live&&!b->reader_reference&&
        !b->reader.reference&&!b->reader.binding.context&&!b->reader.binding.context_bytes&&
        !b->activation_pending&&!b->activation_event&&!b->action.data&&!b->action.words&&
        !b->action.period&&!b->action.volume&&!b->span_count);
    for(i=0;i<PT_PREPARED_PAULA_LIFETIME_DATA_SPANS;++i)
        assert(!b->spans[i].data&&!b->spans[i].bytes);
}
static enum pt_readers_reply active_cancel_command(void *context,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{
    struct active_backend *b=context;struct pt_readers_command_receipt r;
    active_callback_enter(b);++b->command_cancels;
    assert(b->command_live&&ticket==b->command.ticket&&out&&b->adopted&&b->dispatches==1);
    memset(&r,0,sizeof(r));r.domain=PT_READERS_COMMAND_DOMAIN;r.origin=PT_READERS_BACKEND_ACTUAL;
    r.queue=b->command.queue;r.session=b->command.session;r.generation=b->command.generation;
    r.ticket=b->command.ticket;r.owner=b->command.owner;r.count=b->command.count;
    r.event=b->command.event_identity;r.context=b->command.binding.context;
    r.context_bytes=b->command.binding.context_bytes;
    r.action[0].key=b->command.key;r.action[0].command=PT_READERS_CANCELLED_AFTER;
    r.action[0].reader=b->reader_live?PT_READERS_ACTIVE:PT_READERS_RETIRED;
    r.action[0].adoption=PT_READERS_ADOPTED;
    r.action[0].observed=b->observed;r.action[0].issued=b->issued;
    active_drop_command(b);*out=r;active_callback_leave(b);return PT_READERS_COMMAND_DETACHED;
}
static enum pt_readers_reply active_cancel_reader(void *context,const struct pt_readers_domain *d,
    struct pt_readers_reader_receipt *out)
{
    struct active_backend *b=context;struct pt_readers_reader_receipt r;
    active_callback_enter(b);++b->reader_cancels;
    assert(active_consumer_live(b)&&d==b->reader_reference&&out&&
        active_key_equal(&d->key,&b->reader.key));
    if(b->reader_mode==ACTIVE_PENDING){active_callback_leave(b);return PT_READERS_PENDING;}
    if(b->reader_mode==ACTIVE_UNCERTAIN){active_callback_leave(b);return PT_READERS_UNCERTAIN;}
    memset(&r,0,sizeof(r));r.domain=PT_READERS_READER_DOMAIN;r.key=b->reader.key;
    r.reference=b->reader.reference;r.context=b->reader.binding.context;
    r.context_bytes=b->reader.binding.context_bytes;r.state=PT_READERS_RETIRED;
    r.adoption=PT_READERS_ADOPTED;r.observed=b->observed;r.issued=b->issued;
    active_drop_reader(b);active_assert_reader_dropped(b);
    if(b->reader_mode==ACTIVE_BAD_CLASS)r.state=PT_READERS_ACTIVE;
    if(b->reader_mode==ACTIVE_WRONG_ENVELOPE){assert(r.key.serial<UINT64_MAX);++r.key.serial;}
    *out=r;active_callback_leave(b);return PT_READERS_READER_RETIRED;
}
static enum pt_readers_reply active_poll_command(void *context,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{struct active_backend *b=context;(void)ticket;(void)out;++b->polls;assert(0);return PT_READERS_UNCERTAIN;}
static enum pt_readers_reply active_poll_reader(void *context,const struct pt_readers_domain *d,
    struct pt_readers_reader_receipt *out)
{struct active_backend *b=context;(void)d;(void)out;++b->polls;assert(0);return PT_READERS_UNCERTAIN;}

static struct active_fixture *active_ready(void)
{
    struct active_fixture *p=calloc(1,sizeof(*p));struct fixture *f;struct arena *a;
    assert(p);f=ready();p->base=f;p->backend.fixture=p;p->backend.now=100;
    p->backend.frequency=1000000;p->backend.source_generation=f->sampler.generation;
    config_begin(f);a=lifetime_arena(f);
    f->backend=(struct pt_readers_backend){&p->backend,sizeof(p->backend),
        {PT_SCHEDULED_REQUIRED,1,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        active_clock,active_submit,active_poll_command,active_cancel_command,
        active_poll_reader,active_cancel_reader};
    f->config.context_count=2;
    f->config.context[1]=(struct pt_prepared_paula_context_extent){p,sizeof(*p)};
    assert(pt_prepared_paula_lifetime_init(f->lifetime,a->capacity,f->owner,
        &f->config,&f->backend)==PT_PREPARED_PAULA_SOURCES_OK);
    assert(pt_prepared_paula_lifetime_enqueue_unpublished(f->lifetime)==PT_PREPARED_PAULA_SOURCES_OK);
    assert(pt_prepared_paula_lifetime_failure(f->lifetime)==PT_SCHEDULED_OK);
    assert(status(f).master_pins==3&&status(f).chip_leases==3&&pins(f)==3);
    return p;
}
static void active_claimed(struct active_fixture *p)
{
    struct fixture *f=p->base;struct active_counters c=active_counts(&p->backend);
    claimed_cancel_refuses(f);active_counts_equal(c,active_counts(&p->backend));
    assert(pins(f)==3&&f->sampler.live_count==3&&f->chip_bytes==3*PACKED);
    if(!p->stale){struct pt_prepared_paula_sources_status s=status(f);
        assert(s.master_pins==3&&s.chip_leases==3&&!s.master_job&&!s.chip_job);}
    master_save_equal(f);
}
static void active_no_early_drain(struct active_fixture *p)
{
    struct fixture *f=p->base;struct audit a;struct active_counters c=active_counts(&p->backend);
    unsigned fast=f->fast_calls,chip=f->chip_calls,fr=f->fast_releases,cr=f->chip_releases;
    audit_capture(f,&a,1);
    assert(pt_prepared_paula_lifetime_drain_step(f->lifetime)==PT_PREPARED_PAULA_SOURCES_BUSY);
    audit_equal_release(&a);no_resource_delta(f,fast,chip,fr,cr);
    active_counts_equal(c,active_counts(&p->backend));active_claimed(p);
}
static void active_publish_dispatch_stop(struct active_fixture *p)
{
    struct fixture *f=p->base;struct active_backend *b=&p->backend;
    struct audit a;struct active_counters c;
    audit_capture(f,&a,1);
    assert(pt_prepared_paula_lifetime_publish_once(f->lifetime)==PT_SCHEDULED_OK);
    audit_equal_release(&a);
    assert(b->clocks==1&&b->submits==1&&!b->dispatches&&!b->reads&&!b->adopted&&!b->active);
    c=active_counts(b);
    assert(pt_prepared_paula_lifetime_publish_once(f->lifetime)==PT_SCHEDULED_INVALID);
    active_counts_equal(c,active_counts(b));active_no_early_drain(p);
    b->now=2184;active_dispatch(b);active_claimed(p);
    c=active_counts(b);audit_capture(f,&a,1);
    assert(pt_prepared_paula_lifetime_stop_publication(f->lifetime)==PT_SCHEDULED_PENDING);
    audit_equal_release(&a);active_counts_equal(c,active_counts(b));
    assert(pt_prepared_paula_lifetime_stop_publication(f->lifetime)==PT_SCHEDULED_INVALID);
    active_counts_equal(c,active_counts(b));active_no_early_drain(p);
}
static enum pt_scheduled_result active_cancel(struct active_fixture *p,unsigned reader)
{
    struct fixture *f=p->base;struct audit a;
    struct active_counters before=active_counts(&p->backend),after;
    enum pt_scheduled_result result;unsigned fast=f->fast_calls,chip=f->chip_calls;
    unsigned fr=f->fast_releases,cr=f->chip_releases;
    audit_capture(f,&a,1);
    result=reader?pt_prepared_paula_lifetime_cancel_reader_once(f->lifetime):
        pt_prepared_paula_lifetime_cancel_command_once(f->lifetime);
    audit_equal_release(&a);no_resource_delta(f,fast,chip,fr,cr);after=active_counts(&p->backend);
    assert(after.clocks==before.clocks&&after.submits==before.submits&&
        after.dispatches==before.dispatches&&after.reads==before.reads&&
        after.read_bytes==before.read_bytes&&after.polls==before.polls);
    assert(after.command_cancels==before.command_cancels+(reader?0:1)&&
        after.reader_cancels==before.reader_cancels+(reader?1:0));
    active_claimed(p);return result;
}
static void active_repeat_refuses(struct active_fixture *p,unsigned reader)
{
    struct fixture *f=p->base;struct arena *sa=f->arena+f->arena_count-1;
    struct arena *la=f->lifetime_arena+f->lifetime_count-1;
    unsigned char *source=malloc(sa->capacity),*life=malloc(la->capacity);
    unsigned char *before=malloc(sizeof(*f)),*model=malloc(sizeof(*p));
    assert(source&&life&&before&&model);
    memcpy(source,sa->data,sa->capacity);memcpy(life,la->data,la->capacity);
    memcpy(before,f,sizeof(*f));memcpy(model,p,sizeof(*p));
    assert((reader?pt_prepared_paula_lifetime_cancel_reader_once(f->lifetime):
        pt_prepared_paula_lifetime_cancel_command_once(f->lifetime))==PT_SCHEDULED_INVALID);
    assert(!memcmp(source,sa->data,sa->capacity)&&!memcmp(life,la->data,la->capacity));
    assert(!memcmp(before,f,sizeof(*f))&&!memcmp(model,p,sizeof(*p)));
    free(source);free(life);free(before);free(model);
}
static void active_mark_stale(struct active_fixture *p)
{
    struct fixture *f=p->base;struct pt_prepared_paula_sources_status out,saved;
    assert(p->backend.adopted&&p->backend.dispatches==1&&f->sampler.generation<UINT32_MAX);
    ++f->sampler.generation;p->stale=1;memset(&out,0x6d,sizeof(out));saved=out;
    assert(pt_prepared_paula_sources_get(f->owner,&out)==PT_PREPARED_PAULA_SOURCES_STALE);
    assert(!memcmp(&out,&saved,sizeof(out)));++active_stale;
}
static void active_drain_finish(struct active_fixture *p,enum pt_scheduled_result failure)
{
    struct fixture *f=p->base;struct active_counters c=active_counts(&p->backend);
    static const unsigned expected_cache_pins[7]={2,2,1,1,0,0,0};
    const unsigned stale=p->stale;unsigned n;
    assert(!p->backend.command_live&&!p->backend.command.event_identity);
    active_assert_reader_dropped(&p->backend);
    assert(p->backend.command_drops==1&&p->backend.reader_drops==1&&
        p->backend.dispatches==1&&p->backend.reads==1&&p->backend.read_bytes==PACKED);
    assert(pt_prepared_paula_lifetime_failure(f->lifetime)==failure);
    for(n=0;n<7;++n){
        struct pt_prepared_paula_sources_status s,before;
        unsigned fast=f->fast_calls,chip=f->chip_calls,fr=f->fast_releases,cr=f->chip_releases;
        if(!stale)before=status(f);
        assert(pt_prepared_paula_lifetime_drain_step(f->lifetime)==
            (n==6?PT_PREPARED_PAULA_SOURCES_CLOSED:PT_PREPARED_PAULA_SOURCES_PENDING));
        no_resource_delta(f,fast,chip,fr,cr);active_counts_equal(c,active_counts(&p->backend));
        assert(pins(f)==expected_cache_pins[n]);
        if(!stale){s=status(f);assert(s.master_pins+s.chip_leases==
            (before.master_pins+before.chip_leases?before.master_pins+before.chip_leases-1:0));}
        assert(pt_prepared_paula_lifetime_failure(f->lifetime)==failure);master_save_equal(f);
    }
    assert(!pins(f));
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    assert(pt_prepared_paula_lifetime_failure(f->lifetime)==failure);
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    assert(pt_prepared_paula_sources_cancel_step(f->owner)==PT_PREPARED_PAULA_SOURCES_CLOSED);
    active_counts_equal(c,active_counts(&p->backend));
    lifetime_tail_equal(f->lifetime_arena+f->lifetime_count-1);
    if(stale)assert(f->sampler.generation==p->backend.source_generation+1);
    finish(f);p->base=NULL;free(p);++active_completed;
}
static void active_clean_case(unsigned reader_first,unsigned stale)
{
    struct active_fixture *p=active_ready();struct active_backend *b=&p->backend;
    active_publish_dispatch_stop(p);if(stale)active_mark_stale(p);
    if(reader_first)active_read_first_cache(b,0);
    assert(active_cancel(p,reader_first)==PT_SCHEDULED_OK);active_repeat_refuses(p,reader_first);
    active_no_early_drain(p);
    if(reader_first){
        active_assert_reader_dropped(b);assert(b->command_live&&!b->command_drops&&b->reader_drops==1);
        /* No claim that core observed ACTIVE/key permission in this order. */
    }else{
        assert(!b->command_live&&b->command_drops==1&&active_consumer_live(b));
        active_read_first_cache(b,1);
    }
    assert(active_cancel(p,!reader_first)==PT_SCHEDULED_OK);active_repeat_refuses(p,!reader_first);
    assert(b->command_cancels==1&&b->reader_cancels==1&&!b->polls);
    active_drain_finish(p,PT_SCHEDULED_OK);
}
static void active_retain(struct active_fixture *p,enum pt_scheduled_result failure)
{
    struct fixture *f=p->base;struct active_backend *b=&p->backend;
    struct active_counters c=active_counts(b);
    assert(active_retained_count<3&&!b->command_live&&b->command_drops==1&&
        b->command_cancels==1&&b->reader_cancels==1&&b->dispatches==1&&b->reads==1);
    active_no_early_drain(p);
    assert(pt_prepared_paula_lifetime_failure(f->lifetime)==failure);
    assert(pt_prepared_paula_lifetime_close(f->lifetime)==PT_PREPARED_PAULA_SOURCES_FAULT);
    active_counts_equal(c,active_counts(b));active_claimed(p);
    assert(f->control.queue&&f->fast_bytes&&f->chip_bytes==3*PACKED);
    active_retained[active_retained_count++]=p;
}
static void active_reader_fault_case(enum active_mode mode)
{
    struct active_fixture *p=active_ready();struct active_backend *b=&p->backend;
    enum pt_scheduled_result result,failure;
    active_publish_dispatch_stop(p);
    assert(active_cancel(p,0)==PT_SCHEDULED_OK);active_repeat_refuses(p,0);
    active_no_early_drain(p);active_read_first_cache(b,1);b->reader_mode=mode;
    result=active_cancel(p,1);active_repeat_refuses(p,1);
    failure=mode==ACTIVE_PENDING?PT_SCHEDULED_OK:PT_SCHEDULED_BACKEND;
    assert(result==(mode==ACTIVE_PENDING?PT_SCHEDULED_PENDING:PT_SCHEDULED_BACKEND));
    assert(pt_prepared_paula_lifetime_failure(p->base->lifetime)==failure);
    if(mode==ACTIVE_BAD_CLASS){
        active_assert_reader_dropped(b);++active_semantic;
        active_drain_finish(p,PT_SCHEDULED_BACKEND);return;
    }
    if(mode==ACTIVE_WRONG_ENVELOPE){active_assert_reader_dropped(b);++active_wrong;}
    else {assert(active_consumer_live(b)&&!b->reader_drops);
        if(mode==ACTIVE_PENDING)++active_pending;else {assert(mode==ACTIVE_UNCERTAIN);++active_uncertain;}}
    active_retain(p,failure);
}
int main(void)
{
    unsigned order,i,j;
    for(order=0;order<2;++order){active_clean_case(order,0);active_clean_case(order,1);}
    active_reader_fault_case(ACTIVE_BAD_CLASS);active_reader_fault_case(ACTIVE_PENDING);
    active_reader_fault_case(ACTIVE_WRONG_ENVELOPE);active_reader_fault_case(ACTIVE_UNCERTAIN);
    assert(active_completed==5&&active_retained_count==3&&active_stale==2&&active_semantic==1&&
        active_pending==1&&active_wrong==1&&active_uncertain==1&&active_dispatch_total==8&&active_read_total==8);
    for(i=0;i<3;++i){assert(active_retained[i]&&active_retained[i]->base);
        for(j=0;j<i;++j)assert(active_retained[i]!=active_retained[j]);}
    assert(!retained_count&&!clean_cases&&!stale_cases&&!alias_cases);
    puts("PREPARED PAULA ACTIVE HOST PASS:8 cases;5 completed;3 retained;synthetic2184 adoption;C-first/R-first ADOPTED retirement;first-cache1026B;C-first read after detach;3 genuine source pins;deferred drain;NO OUTPUT OR TIMING AUTHORITY");
    return 0;
}
