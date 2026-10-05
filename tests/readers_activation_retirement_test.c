/* Focused genuine public regression. Reuse only the small heap-owned RAM
 * fixture scaffold; its reference entry is compiled but NEVER invoked here.
 * No private core C, owner layout/cast or invented positive receipt history. */
#define main ram_reference_entry_not_run
#include "native_ram_port_test.c"
#undef main

static void retirement_assert_never_issued(const struct pt_readers_command_receipt *receipt,unsigned count)
{
    unsigned i;
    assert(receipt->count==count);
    for(i=0;i<count;++i){
        const struct pt_readers_action_receipt *a=receipt->action+i;
        assert(a->command==PT_READERS_CANCELLED_BEFORE&&a->reader==PT_READERS_NONE&&a->adoption==PT_READERS_UNADOPTED);
        assert(!a->observed&&!a->issued);
    }
}
static void retirement_reserved_before_command(void)
{
    struct ram_case *c=ram_new();
    uint64_t ticket=ram_enqueue(c,PT_SCHEDULED_TRIGGER,3,RAM_FRAME);
    struct pt_readers_key key,before;
    unsigned quiet;
    ram_publish(c,ticket,RAM_FRAME);
    assert(pt_readers_poll_command(c->queue,ticket,&c->command_out)==PT_SCHEDULED_PENDING);
    assert(c->command_out.action[0].command==PT_READERS_WAITING&&!c->port->effects);
    c->model->quiet_result=1;
    ram_reader_cancel(c,ticket,0,PT_SCHEDULED_OK);
    assert(c->reader_out.state==PT_READERS_RETIRED&&c->reader_out.adoption==PT_READERS_UNADOPTED&&!c->reader_out.observed&&!c->reader_out.issued);
    assert(c->model->last_quiet==ticket&&c->model->last_cancel==1&&c->port->armed_index==-1);
    assert(ram_command(c,ticket)->finished&&!ram_command(c,ticket)->packet.action[0].data&&!ram_command(c,ticket)->packet.action[1].data);
    assert(pt_private_ram_dispatch(c->port)==PT_READERS_ACTIVATION_FAILED&&!c->port->effects);
    assert(pt_readers_commands_held(c->queue)==1&&pt_readers_readers_held(c->queue)==2&&c->counts.live==3&&!c->counts.terminal&&!c->counts.releases);
    c->model->quiet_result=0;
    assert(pt_readers_poll_command(c->queue,ticket,&c->command_out)==PT_SCHEDULED_PENDING);
    retirement_assert_never_issued(&c->command_out,2);
    assert(c->counts.live==3&&!c->counts.terminal&&!c->counts.releases);
    /* The command observation cannot revive the retired domain. Its backend
     * registration has ended, so do not re-poll that ended receipt domain. */
    memset(&key,0xb3,sizeof(key));memcpy(&before,&key,sizeof(before));
    assert(pt_readers_reader_key(c->queue,ticket,0,&key)==PT_SCHEDULED_STALE&&!memcmp(&key,&before,sizeof(key)));
    assert(pt_readers_readers_held(c->queue)==2&&c->counts.live==3&&!c->counts.releases);
    quiet=c->model->quiet_calls;
    ram_reader_cancel(c,ticket,1,PT_SCHEDULED_OK);
    assert(c->model->quiet_calls==quiet&&!c->port->effects);
    assert(pt_readers_poll_command(c->queue,ticket,&c->command_out)==PT_SCHEDULED_PENDING);
    retirement_assert_never_issued(&c->command_out,2);
    assert(c->counts.live==3&&!c->counts.terminal&&!c->counts.releases);
    ram_detach(c,ticket,PT_SCHEDULED_OK);
    retirement_assert_never_issued(&c->command_out,2);
    assert(c->counts.terminal==3&&c->counts.releases==3&&!c->counts.invalid&&!c->counts.live);
    ram_finish(c);
}
static uint64_t retirement_mixed_enqueue(struct ram_case *c,enum pt_scheduled_kind continuing)
{
    struct ram_holder *h;
    uint64_t ticket=0;
    memset(&c->batch,0,sizeof(c->batch));memset(c->owners,0,sizeof(c->owners));memset(c->target,0,sizeof(c->target));
    c->batch.frame=RAM_FRAME+RAM_STRIDE;c->batch.generation=RAM_GENERATION;c->batch.count=2;
    c->command=ram_holder_new(c,0,0);c->owners[0].control=ram_holder_new(c,1,0);h=c->owners[0].control.context;
    c->spans[0]=(struct pt_scheduled_span){h->data,RAM_CAPACITY};c->owners[0].spans=c->spans;c->owners[0].count=1;
    c->batch.action[0]=(struct pt_scheduled_action){PT_SCHEDULED_TRIGGER,0,h->data,32,428,32};
    c->batch.action[1]=(struct pt_scheduled_action){continuing,1,NULL,0,0,0};
    if(continuing==PT_SCHEDULED_CONTROL){c->batch.action[1].period=429;c->batch.action[1].volume=33;}
    c->target[1]=c->keys[1];
    assert(pt_readers_enqueue(c->queue,&c->batch,c->target,&c->command,c->owners,&ticket)==PT_SCHEDULED_OK);
    memset(&c->command,0x3c,sizeof(c->command));memset(c->owners,0x3c,sizeof(c->owners));memset(c->spans,0x3c,sizeof(c->spans));
    return ticket;
}
static void retirement_mixed_cancel(enum pt_scheduled_kind continuing)
{
    struct ram_case *c=ram_new();
    uint64_t first=ram_enqueue(c,PT_SCHEDULED_TRIGGER,2,RAM_FRAME),mixed;
    struct pt_readers_key before;
    ram_publish(c,first,RAM_FRAME);ram_dispatch_good(c,first,1);
    assert(pt_readers_reader_key(c->queue,first,0,c->keys+1)==PT_SCHEDULED_OK);
    ram_detach(c,first,PT_SCHEDULED_OK);
    mixed=retirement_mixed_enqueue(c,continuing);ram_publish(c,mixed,RAM_FRAME+RAM_STRIDE);
    c->model->quiet_result=1;ram_reader_cancel(c,mixed,0,PT_SCHEDULED_OK);
    assert(c->port->effects==1&&c->port->mask==2&&ram_key_equal(c->port->slot+1,c->keys+1));
    c->model->quiet_result=0;
    assert(pt_readers_poll_command(c->queue,mixed,&c->command_out)==PT_SCHEDULED_PENDING);
    assert(c->command_out.action[0].command==PT_READERS_CANCELLED_BEFORE&&c->command_out.action[0].reader==PT_READERS_NONE&&c->command_out.action[0].adoption==PT_READERS_UNADOPTED);
    assert(c->command_out.action[1].command==PT_READERS_CANCELLED_BEFORE&&c->command_out.action[1].reader==PT_READERS_ACTIVE&&c->command_out.action[1].adoption==PT_READERS_ADOPTED);
    assert(!c->command_out.action[0].observed&&!c->command_out.action[0].issued&&!c->command_out.action[1].observed&&!c->command_out.action[1].issued);
    ram_detach(c,mixed,PT_SCHEDULED_OK);
    assert(pt_readers_poll_reader(c->queue,first,0,&c->reader_out)==PT_SCHEDULED_PENDING&&c->reader_out.state==PT_READERS_ACTIVE);
    memcpy(&before,c->keys+1,sizeof(before));
    if(continuing==PT_SCHEDULED_STOP){
        assert(pt_readers_reader_key(c->queue,first,0,&before)==PT_SCHEDULED_STALE&&ram_key_equal(&before,c->keys+1));
    }else assert(pt_readers_reader_key(c->queue,first,0,&before)==PT_SCHEDULED_OK&&ram_key_equal(&before,c->keys+1));
    ram_reader_cancel(c,first,0,PT_SCHEDULED_OK);
    assert(!c->counts.invalid);ram_finish(c);
}
static void retirement_issued_stays_adopted(void)
{
    struct ram_case *c=ram_new();
    uint64_t ticket=ram_enqueue(c,PT_SCHEDULED_TRIGGER,1,RAM_FRAME);
    ram_publish(c,ticket,RAM_FRAME);ram_dispatch_good(c,ticket,1);
    ram_reader_cancel(c,ticket,0,PT_SCHEDULED_OK);
    assert(c->reader_out.state==PT_READERS_RETIRED&&c->reader_out.adoption==PT_READERS_ADOPTED);
    assert(pt_readers_poll_command(c->queue,ticket,&c->command_out)==PT_SCHEDULED_PENDING);
    assert(c->command_out.action[0].command==PT_READERS_ISSUED&&c->command_out.action[0].reader==PT_READERS_RETIRED&&c->command_out.action[0].adoption==PT_READERS_ADOPTED);
    assert(c->command_out.action[0].observed==60634&&c->command_out.action[0].issued==60634);
    ram_detach(c,ticket,PT_SCHEDULED_OK);assert(!c->counts.invalid);ram_finish(c);
}

/* Transparent public backend observer: this negative case replays an ended
 * genuine WAITING observation verbatim. It never edits holders or invents an
 * ACTIVE/adoption receipt. The exact event remains alive until detachment. */
struct retirement_observer {
    struct pt_readers_backend actual;
    struct pt_readers_command_receipt waiting;
    unsigned have_waiting,replay;
};
static int retirement_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct retirement_observer *o=context;return o->actual.read_clock(o->actual.context,ticks,frequency);}
static int retirement_submit(void *context,const struct pt_readers_event *event)
{struct retirement_observer *o=context;return o->actual.submit(o->actual.context,event);}
static enum pt_readers_reply retirement_command_poll(void *context,uint64_t ticket,struct pt_readers_command_receipt *out)
{
    struct retirement_observer *o=context;
    enum pt_readers_reply reply;
    if(o->replay){assert(o->have_waiting&&o->waiting.ticket==ticket);memcpy(out,&o->waiting,sizeof(*out));return PT_READERS_OBSERVATION;}
    reply=o->actual.poll_command(o->actual.context,ticket,out);
    if(reply==PT_READERS_OBSERVATION&&out->action[0].command==PT_READERS_WAITING){memcpy(&o->waiting,out,sizeof(*out));o->have_waiting=1;}
    return reply;
}
static enum pt_readers_reply retirement_command_cancel(void *context,uint64_t ticket,struct pt_readers_command_receipt *out)
{struct retirement_observer *o=context;return o->actual.cancel_command(o->actual.context,ticket,out);}
static enum pt_readers_reply retirement_reader_poll(void *context,const struct pt_readers_domain *domain,struct pt_readers_reader_receipt *out)
{struct retirement_observer *o=context;return o->actual.poll_reader(o->actual.context,domain,out);}
static enum pt_readers_reply retirement_reader_cancel(void *context,const struct pt_readers_domain *domain,struct pt_readers_reader_receipt *out)
{struct retirement_observer *o=context;return o->actual.cancel_reader(o->actual.context,domain,out);}
static struct ram_case *retirement_observed_new(struct retirement_observer *observer)
{
    struct ram_case *c=calloc(1,sizeof(*c));
    struct pt_private_ram_adapter adapter;
    struct pt_readers_activation_port port;
    assert(c);
    c->model=calloc(1,sizeof(*c->model));c->memory=calloc(1,sizeof(*c->memory));c->port=malloc(sizeof(*c->port));
    assert(c->model&&c->memory&&c->port);
    c->session=ram_session();c->model->now=RAM_EPOCH;c->model->frequency=RAM_FREQUENCY;
    c->model->clock_result=1;c->model->arm_result=1;c->model->close_result=1;
    adapter=(struct pt_private_ram_adapter){c->model,ram_clock,ram_arm,ram_quiet,ram_source_end};
    assert(pt_private_ram_init(c->port,c->session,RAM_GENERATION,RAM_FREQUENCY,&adapter,128,256,4));
    c->allocator=(struct pt_allocator){c->memory,ram_allocate,ram_release};
    c->grid=(struct pt_scheduled_grid){RAM_EPOCH,RAM_GENERATION,RAM_FREQUENCY,RAM_RATE};port=pt_private_ram_api(c->port);
    assert(pt_readers_activation_open(&c->allocator,&c->grid,c->session,&port,&c->ledger)==PT_SCHEDULED_OK);
    assert(pt_readers_activation_api(c->ledger,&observer->actual)==PT_SCHEDULED_OK);
    c->backend=observer->actual;c->backend.context=observer;c->backend.context_bytes=sizeof(*observer);
    c->backend.read_clock=retirement_clock;c->backend.submit=retirement_submit;
    c->backend.poll_command=retirement_command_poll;c->backend.cancel_command=retirement_command_cancel;
    c->backend.poll_reader=retirement_reader_poll;c->backend.cancel_reader=retirement_reader_cancel;
    assert(pt_readers_open(&c->allocator,&c->grid,c->session,&c->backend,2,8,&c->queue)==PT_SCHEDULED_OK);
    return c;
}
static void retirement_stale_waiting_refuses(void)
{
    struct retirement_observer *observer=calloc(1,sizeof(*observer));
    struct ram_case *c;
    struct pt_readers_command_receipt before;
    uint64_t ticket;
    assert(observer);c=retirement_observed_new(observer);
    ticket=ram_enqueue(c,PT_SCHEDULED_TRIGGER,3,RAM_FRAME);ram_publish(c,ticket,RAM_FRAME);
    assert(pt_readers_poll_command(c->queue,ticket,&c->command_out)==PT_SCHEDULED_PENDING&&observer->have_waiting);
    c->model->quiet_result=1;ram_reader_cancel(c,ticket,0,PT_SCHEDULED_OK);
    c->model->quiet_result=0;observer->replay=1;
    memset(&c->command_out,0xa7,sizeof(c->command_out));memcpy(&before,&c->command_out,sizeof(before));
    assert(pt_readers_poll_command(c->queue,ticket,&c->command_out)==PT_SCHEDULED_BACKEND&&!memcmp(&before,&c->command_out,sizeof(before)));
    assert(pt_readers_commands_held(c->queue)==1&&pt_readers_readers_held(c->queue)==2&&c->counts.live==3&&!c->counts.releases);
    observer->replay=0;ram_detach(c,ticket,PT_SCHEDULED_BACKEND);
    ram_reader_cancel(c,ticket,1,PT_SCHEDULED_BACKEND);
    assert(!pt_readers_commands_held(c->queue)&&!pt_readers_readers_held(c->queue)&&!c->counts.live&&c->counts.terminal==c->counts.releases);
    /* A stale observation latches failure, but later independent, valid
     * exact proofs may have valid terminal classifications and consume once. */
    assert(pt_readers_stop(c->queue)==PT_SCHEDULED_BACKEND&&pt_private_ram_source_close(c->port)==1);
    assert(pt_readers_close(c->queue));c->queue=NULL;
    assert(pt_readers_activation_close(&c->ledger)&&!c->memory->live&&c->memory->allocations==c->memory->releases);
    free(c->port);free(c->model);free(c->memory);free(c);free(observer);
}
int main(void)
{
    retirement_reserved_before_command();
    retirement_mixed_cancel(PT_SCHEDULED_CONTROL);retirement_mixed_cancel(PT_SCHEDULED_STOP);
    retirement_issued_stays_adopted();retirement_stale_waiting_refuses();
    puts("READERS TRIGGER RETIREMENT PASS: independent reserved-reader proof; pending command retained; genuine cancelled snapshot; exact quiet release; software only");
    return 0;
}
