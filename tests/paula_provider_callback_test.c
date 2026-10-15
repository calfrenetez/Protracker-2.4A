/* Exercise every typed-provider callback seam using genuine retained source
 * geometry. No hardware, future backend execution or forced retirement. */
#define PT_PROVIDER_EXTERNAL_MAIN
#include "paula_segment_provider_test.c"
enum callback_phase {CB_ENTER,CB_RESTORE,CB_CLOCK,CB_ARM,CB_REARM,CB_APPLY,
    CB_ACK,CB_CANCEL,CB_TICKET,CB_READER,CB_CLOSE,CB_SOURCE,CB_COUNT};
struct callback_case {
    struct provider_model model;
    unsigned target,fault,enabled,fired,calls[CB_COUNT];
};
static struct callback_case callback_cases[CB_COUNT][3];
static struct callback_case entry_cases[12][2];
static struct callback_case precall_cases[CB_COUNT][13][2];
static struct sequence_fixture callback_owner;
static unsigned callback_count,callback_disposed,callback_retained,entry_count,precall_count,precall_positive,precall_retained;
static struct callback_case *callback_context(void *v)
{return (struct callback_case *)(void *)provider_model_context(v);}
static void callback_fault(struct callback_case *c,unsigned phase)
{
    struct pt_paula_provider_status untouched,before;
    ++c->calls[phase];if(!c->enabled||c->fired||c->target!=phase)return;
    c->fired=1;
    if(c->fault==0)memset(c->model.storage.bytes,0,pt_paula_segment_provider_size());
    else if(c->fault==1)++c->model.original.ticket;
    else {memset(&untouched,0x6d,sizeof(untouched));memcpy(&before,&untouched,sizeof(before));
        assert(!pt_paula_segment_provider_get(c->model.adapter.context,&untouched));
        assert(!memcmp(&before,&untouched,sizeof(before)));}
}
static int cb_enter(void *v,enum pt_paula_future_entry e,uint64_t *t)
{struct callback_case *c=callback_context(v);int r=provider_model_enter(&c->model,e,t);callback_fault(c,CB_ENTER);return r;}
static int cb_restore(void *v,enum pt_paula_future_entry e,uint64_t t)
{struct callback_case *c=callback_context(v);int r=provider_model_restore(&c->model,e,t);callback_fault(c,CB_RESTORE);return r;}
static int cb_clock(void *v,uint64_t *t,uint32_t *f)
{struct callback_case *c=callback_context(v);int r=provider_model_clock(&c->model,t,f);callback_fault(c,CB_CLOCK);return r;}
static int cb_packet(void *v,enum pt_paula_provider_packet_action a,const struct pt_paula_provider_record *p)
{struct callback_case *c=callback_context(v);int r=provider_model_packet(&c->model,a,p);
 callback_fault(c,a==PT_PAULA_PROVIDER_ARM?CB_ARM:a==PT_PAULA_PROVIDER_REARM?CB_REARM:CB_APPLY);return r;}
static int cb_ticket(void *v,enum pt_paula_provider_ticket_action a,uint64_t t)
{struct callback_case *c=callback_context(v);int r=provider_model_ticket(&c->model,a,t);
 callback_fault(c,a==PT_PAULA_PROVIDER_ACK?CB_ACK:a==PT_PAULA_PROVIDER_CANCEL?CB_CANCEL:CB_TICKET);return r;}
static int cb_reader(void *v)
{struct callback_case *c=callback_context(v);int r=provider_model_reader(&c->model);callback_fault(c,CB_READER);return r;}
static int cb_close(void *v)
{struct callback_case *c=callback_context(v);int r=provider_model_close(&c->model);callback_fault(c,CB_CLOSE);return r;}
static int cb_source(void *v)
{struct callback_case *c=callback_context(v);int r=provider_model_source(&c->model);callback_fault(c,CB_SOURCE);return r;}
static void callback_init(struct callback_case *c,unsigned target,unsigned fault,
    const struct pt_paula_future_packet *q)
{
    struct provider_model *m=&c->model;struct pt_paula_provider_resources ops;
    struct pt_paula_provider_config config;unsigned i;
    memset(c,0,sizeof(*c));c->target=target;c->fault=fault;m->mode=-1;
    m->arm_return=m->enter_return=m->restore_return=m->close_return=1;
    m->frequency=709379;m->now=q->first-1;m->token=31;m->context.owner=m;
    memcpy(&m->original,q,sizeof(*q));memset(&ops,0,sizeof(ops));memset(&config,0,sizeof(config));
    config.frequency=709379;for(i=0;i<4;++i)config.frame[i]=960U*(i+1U);
    for(i=0;i<3;++i)config.label[i]=(struct pt_paula_provider_label){(uint16_t)i,
        0x100U+i,0x200U+i,0x300U+i,0x400U,0x500U,0x600U,0x100000U+0x1000U*i};
    ops.context=&m->context;ops.context_bytes=sizeof(m->context);ops.enter=cb_enter;
    ops.restore=cb_restore;ops.clock=cb_clock;ops.packet=cb_packet;ops.ticket=cb_ticket;
    ops.reader_quiet=cb_reader;ops.source_close=cb_close;ops.source_quiet=cb_source;
    assert(pt_paula_segment_provider_init(m->storage.bytes,sizeof(m->storage),&ops,&config,&m->adapter));
}
static void callback_case_run(const struct pt_paula_future_packet *q,unsigned target,unsigned fault)
{
    struct callback_case *c=&callback_cases[target][fault];struct provider_model *m=&c->model;
    struct pt_paula_provider_status status;struct pt_paula_provider_row_status row;
    enum pt_paula_future_entry entry=target==CB_REARM||target==CB_APPLY||target==CB_ACK?
        PT_PAULA_FUTURE_DELIVERY:PT_PAULA_FUTURE_TASK;
    uint64_t token,tick=UINT64_MAX;uint32_t frequency=UINT32_MAX;unsigned before,i;int r;
    callback_init(c,target,fault,q);token=provider_entry(m,PT_PAULA_FUTURE_TASK);
    if(target!=CB_ARM){
        assert(m->adapter.arm(m->adapter.context,&m->original)==1);
        provider_exit(m,PT_PAULA_FUTURE_TASK,token);
        if(target==CB_APPLY||target==CB_ACK)m->now=q->first;
        if(target!=CB_ENTER)token=provider_entry(m,entry);
    }
    if(target==CB_ACK)assert(m->adapter.apply(m->adapter.context,&m->original)==1);
    m->ticket_release=m->reader_release=m->source_release=1;
    if(target>=CB_READER)assert(m->adapter.ticket_quiet(m->adapter.context,q->ticket)==1);
    if(target>=CB_CLOSE)assert(m->adapter.reader_quiet(m->adapter.context)==1);
    if(target==CB_SOURCE)assert(m->adapter.source_close(m->adapter.context)==1);
    before=c->calls[target];c->enabled=1;
    switch(target){
    case CB_ENTER:token=UINT64_MAX;r=m->adapter.enter(m->adapter.context,entry,&token);assert(token==UINT64_MAX);break;
    case CB_RESTORE:r=m->adapter.leave(m->adapter.context,entry,token);break;
    case CB_CLOCK:r=m->adapter.clock(m->adapter.context,&tick,&frequency);assert(tick==UINT64_MAX&&frequency==UINT32_MAX);break;
    case CB_ARM:r=m->adapter.arm(m->adapter.context,&m->original);break;
    case CB_REARM:r=m->adapter.rearm(m->adapter.context,&m->original);break;
    case CB_APPLY:r=m->adapter.apply(m->adapter.context,&m->original);break;
    case CB_ACK:r=m->adapter.ack(m->adapter.context,q->ticket);break;
    case CB_CANCEL:r=m->adapter.cancel(m->adapter.context,q->ticket);break;
    case CB_TICKET:r=m->adapter.ticket_quiet(m->adapter.context,q->ticket);break;
    case CB_READER:r=m->adapter.reader_quiet(m->adapter.context);break;
    case CB_CLOSE:r=m->adapter.source_close(m->adapter.context);break;
    case CB_SOURCE:r=m->adapter.source_quiet(m->adapter.context);break;
    default:assert(0);r=0;
    }
    assert(r==-1&&c->fired&&c->calls[target]==before+1);c->enabled=0;
    assert(pt_paula_segment_provider_get(m->adapter.context,&status)&&status.failed&&status.retained==1);
    assert(pt_paula_segment_provider_row(m->adapter.context,0,&row)&&row.present);
    for(i=0;i<4;++i)if(m->borrowed[i])assert(!memcmp(m->borrowed[i],m->observed+i,sizeof(m->observed[i])));
    assert(m->original.ticket==q->ticket+(fault==1)); /* No repair of caller storage. */
    assert(!pt_paula_segment_provider_dispose(m->adapter.context));
    if(target==CB_ENTER||target==CB_RESTORE){
        assert(status.restore_hold);++callback_retained;
    }else if(fault==1){
        assert(m->adapter.leave(m->adapter.context,entry,token)==-1);
        assert(pt_paula_segment_provider_get(m->adapter.context,&status)&&status.restore_hold);
        ++callback_retained;
    }else{
        provider_exit(m,entry,token);token=provider_entry(m,PT_PAULA_FUTURE_TASK);
        assert(m->adapter.ticket_quiet(m->adapter.context,q->ticket)==1);
        assert(m->adapter.reader_quiet(m->adapter.context)==1);
        r=m->adapter.source_close(m->adapter.context);assert(r==1||(target==CB_CLOSE&&r==-1));
        assert(m->adapter.source_quiet(m->adapter.context)==1);
        provider_exit(m,PT_PAULA_FUTURE_TASK,token);
        assert(pt_paula_segment_provider_get(m->adapter.context,&status)&&status.failed&&!status.restore_hold);
        assert(pt_paula_segment_provider_dispose(m->adapter.context));++callback_disposed;
    }
    assert(pinned(&provider_owner->f)==3&&provider_owner->f.chip.bytes==2304);
    storage(&provider_owner->f);++callback_count;
}
static void packet_mutate(struct provider_model *m,unsigned field)
{
    switch(field){
    case 0:++m->original.ticket;break;
    case 1:--m->original.first;break;
    case 2:++m->original.last;break;
    case 3:++m->original.geometry.frame;break;
    case 4:++m->original.geometry.track;break;
    case 5:m->original.geometry.source_count=3;break;
    case 6:++m->original.geometry.source[0].generation;break;
    case 7:++m->original.geometry.source[0].master.capacity;break;
    case 8:--m->original.geometry.source[0].cache_bytes;break;
    case 9:m->original.geometry.source[0].descriptor=NULL;break;
    case 10:++m->original.geometry.source[0].guard_count;break;
    case 11:++m->original.geometry.source[0].cache_version;break;
    default:assert(0);
    }
}
/* These packets were accepted and restored positively before caller mutation.
 * Refuse the next whole entry without even asking the injected resource to
 * enter. Changed source identities are never followed or repaired here. */
static void entry_case_run(const struct pt_paula_future_packet *q,unsigned field,
    unsigned delivery)
{
    struct callback_case *c=&entry_cases[field][delivery];struct provider_model *m=&c->model;
    struct pt_paula_provider_status status;struct pt_paula_provider_row_status row;
    struct pt_paula_future_packet changed;unsigned calls[CB_COUNT];uint64_t token,prior_token;
    callback_init(c,CB_COUNT,0,q);token=provider_entry(m,PT_PAULA_FUTURE_TASK);
    assert(m->adapter.arm(m->adapter.context,&m->original)==1);
    provider_exit(m,PT_PAULA_FUTURE_TASK,token);
    packet_mutate(m,field);
    memcpy(&changed,&m->original,sizeof(changed));memcpy(calls,c->calls,sizeof(calls));
    prior_token=m->token;token=UINT64_MAX;
    assert(m->adapter.enter(m->adapter.context,delivery?PT_PAULA_FUTURE_DELIVERY:
        PT_PAULA_FUTURE_TASK,&token)==-1&&token==UINT64_MAX);
    assert(!memcmp(calls,c->calls,sizeof(calls))&&!m->locked&&m->token==prior_token);
    assert(!memcmp(&changed,&m->original,sizeof(changed)));
    assert(pt_paula_segment_provider_get(m->adapter.context,&status)&&status.failed&&
        !status.entered&&!status.restore_hold&&status.retained==1&&!status.quiet);
    assert(pt_paula_segment_provider_row(m->adapter.context,0,&row)&&row.present&&
        !row.quiet&&!row.original_released);
    assert(m->borrowed[0]&&!memcmp(m->borrowed[0],m->observed,sizeof(m->observed[0])));
    assert(!pt_paula_segment_provider_dispose(m->adapter.context));
    assert(pinned(&provider_owner->f)==3&&provider_owner->f.chip.bytes==2304);
    storage(&provider_owner->f);++entry_count;
}
static int precall_invoke(struct provider_model *m,unsigned target,
    const struct pt_paula_future_packet *next,uint64_t ticket,uint64_t *tick,uint32_t *frequency)
{
    switch(target){
    case CB_CLOCK:return m->adapter.clock(m->adapter.context,tick,frequency);
    case CB_ARM:return m->adapter.arm(m->adapter.context,next);
    case CB_REARM:return m->adapter.rearm(m->adapter.context,&m->original);
    case CB_APPLY:return m->adapter.apply(m->adapter.context,&m->original);
    case CB_ACK:return m->adapter.ack(m->adapter.context,ticket);
    case CB_CANCEL:return m->adapter.cancel(m->adapter.context,ticket);
    case CB_TICKET:return m->adapter.ticket_quiet(m->adapter.context,ticket);
    case CB_READER:return m->adapter.reader_quiet(m->adapter.context);
    case CB_CLOSE:return m->adapter.source_close(m->adapter.context);
    case CB_SOURCE:return m->adapter.source_quiet(m->adapter.context);
    default:assert(0);return 0;
    }
}
/* A positive control for every seam proves the requested operation was ready.
 * Negative cases mutate a live accepted packet inside the existing entry;
 * restoring that entry remains an independent once-only obligation. */
static void precall_case_run(const struct pt_paula_future_packet *q,unsigned target,unsigned field,unsigned cached)
{
    struct callback_case *c=&precall_cases[target][field][cached];struct provider_model *m=&c->model;
    struct pt_paula_provider_status before,status;struct pt_paula_provider_row_status row_before,row;
    struct pt_paula_future_packet next,changed;unsigned calls[CB_COUNT],i;uint64_t token,tick=UINT64_MAX;
    uint32_t frequency=UINT32_MAX;int r;enum pt_paula_future_entry entry=
        target==CB_REARM||target==CB_APPLY||target==CB_ACK?PT_PAULA_FUTURE_DELIVERY:PT_PAULA_FUTURE_TASK;
    callback_init(c,CB_COUNT,0,q);token=provider_entry(m,PT_PAULA_FUTURE_TASK);
    assert(m->adapter.arm(m->adapter.context,&m->original)==1);
    provider_exit(m,PT_PAULA_FUTURE_TASK,token);
    if(target==CB_APPLY||target==CB_ACK)m->now=q->first;
    token=provider_entry(m,entry);m->ticket_release=m->reader_release=m->source_release=1;
    if(target==CB_ACK)assert(m->adapter.apply(m->adapter.context,&m->original)==1);
    if(target>=CB_READER)assert(m->adapter.ticket_quiet(m->adapter.context,q->ticket)==1);
    if(target>=CB_CLOSE)assert(m->adapter.reader_quiet(m->adapter.context)==1);
    if(target==CB_SOURCE)assert(m->adapter.source_close(m->adapter.context)==1);
    memcpy(&next,q,sizeof(next));next.ticket+=100;next.first+=100;next.last+=100;next.geometry.frame=1920;
    if(cached)assert(precall_invoke(m,target,&next,q->ticket,&tick,&frequency)==1);
    assert(pt_paula_segment_provider_get(m->adapter.context,&before));
    assert(pt_paula_segment_provider_row(m->adapter.context,0,&row_before));
    if(field<12)packet_mutate(m,field);
    memcpy(&changed,&m->original,sizeof(changed));memcpy(calls,c->calls,sizeof(calls));
    r=precall_invoke(m,target,&next,q->ticket,&tick,&frequency);
    assert(!memcmp(&changed,&m->original,sizeof(changed)));
    if(field<12){
        assert(r==-1&&!memcmp(calls,c->calls,sizeof(calls))&&m->locked&&m->token==token);
        if(target==CB_CLOCK)assert(tick==UINT64_MAX&&frequency==UINT32_MAX);
        assert(pt_paula_segment_provider_get(m->adapter.context,&status));
        before.failed=1;assert(!memcmp(&before,&status,sizeof(before)));
        assert(pt_paula_segment_provider_row(m->adapter.context,0,&row)&&!memcmp(&row_before,&row,sizeof(row)));
        for(i=0;i<4;++i)if(m->borrowed[i])assert(!memcmp(m->borrowed[i],m->observed+i,sizeof(m->observed[i])));
        /* Refusal must still allow the one original restoration attempt. An
         * unclean entry does not become certified quiet or disposable. */
        assert(m->adapter.leave(m->adapter.context,entry,token)==-1&&!m->locked);
        ++calls[CB_RESTORE];assert(!memcmp(calls,c->calls,sizeof(calls)));
        assert(pt_paula_segment_provider_get(m->adapter.context,&status)&&status.failed&&status.restore_hold);
        assert(m->adapter.leave(m->adapter.context,entry,token)==-1&&!memcmp(calls,c->calls,sizeof(calls)));
        assert(!pt_paula_segment_provider_dispose(m->adapter.context));++precall_retained;
    }else{
        assert(r==1);if(!cached)++calls[target];assert(!memcmp(calls,c->calls,sizeof(calls)));
        if(entry==PT_PAULA_FUTURE_DELIVERY){provider_exit(m,entry,token);token=provider_entry(m,PT_PAULA_FUTURE_TASK);}
        assert(m->adapter.ticket_quiet(m->adapter.context,q->ticket)==1);
        if(target==CB_ARM)assert(m->adapter.ticket_quiet(m->adapter.context,next.ticket)==1);
        assert(m->adapter.reader_quiet(m->adapter.context)==1);
        assert(m->adapter.source_close(m->adapter.context)==1);
        assert(m->adapter.source_quiet(m->adapter.context)==1);
        provider_exit(m,PT_PAULA_FUTURE_TASK,token);
        assert(pt_paula_segment_provider_dispose(m->adapter.context));++precall_positive;
    }
    assert(pinned(&provider_owner->f)==3&&provider_owner->f.chip.bytes==2304);
    storage(&provider_owner->f);++precall_count;
}
static int callback_submit(void *v,const struct pt_readers_event *event,const struct pt_paula_segment_plan *plan)
{
    struct pt_paula_future_packet q;unsigned i,j;memset(&q,0,sizeof(q));
    q.ticket=event->scheduled.ticket;q.first=event->scheduled.first;q.last=event->scheduled.last;
    memcpy(&q.geometry,plan,sizeof(*plan));
    for(i=0;i<CB_COUNT;++i)for(j=0;j<3;++j)callback_case_run(&q,i,j);
    for(i=0;i<12;++i)for(j=0;j<2;++j)entry_case_run(&q,i,j);
    for(i=CB_ACK;i<CB_COUNT;++i){precall_case_run(&q,i,12,1);
        for(j=0;j<12;++j)precall_case_run(&q,i,j,1);}
    for(i=CB_CLOCK;i<CB_COUNT;++i){precall_case_run(&q,i,12,0);
        for(j=0;j<12;++j)precall_case_run(&q,i,j,0);}
    return sequence_submit(v,event,plan);
}
int main(void)
{
    assert(pt_provider_fixture_main()==0);
    sequence_init(&callback_owner,0);provider_owner=&callback_owner;
    callback_owner.f.backend.submit_plan=callback_submit;sequence_open(&callback_owner);
    assert(sequence_until(&callback_owner,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
    sequence_activate(&callback_owner,0,1);
    assert(callback_count==36&&callback_disposed==20&&callback_retained==16);
    assert(entry_count==24);
    assert(precall_count==208&&precall_positive==16&&precall_retained==192);
    assert(pinned(&callback_owner.f)==3&&callback_owner.f.chip.bytes==2304);
    puts("provider callback matrix: 36 cases; 12 seams x holder/original/reentry faults");
    puts("20 independently drained providers; 16 process-lifetime HOLD providers; source owner retained");
    puts("HOST callback and borrowed-packet checks only; holder/source mutation/native/IRQ/timing NOT QUALIFIED");
    puts("provider stale-entry matrix: 24 cases; 12 packet changes x TASK/delivery");
    puts("no resource enter, token write or source release after observed stale packet");
    puts("provider precall matrix: 208 cases; 16 ready controls and 192 stale-packet refusals");
    puts("no extra callback or quiet claim; original restoration attempted once, then HOLD");
    return 0;
}
