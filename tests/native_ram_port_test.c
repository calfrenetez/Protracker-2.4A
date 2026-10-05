/* Focused SOFTWARE adapter fixture. Genuine public queue/ledger holders;
 * production C is compiled separately. No CIA, IRQ, device or native claims.
 * The small RAM adapter is an explicitly private scaffold, never an owner
 * cast, registration shortcut or fabricated positive activation certificate.
 */
#include "native_ram_port.h"
#include "document.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_EPOCH UINT64_C(100)
#define RAM_FREQUENCY 709379U
#define RAM_RATE 48000U
#define RAM_GENERATION UINT64_C(7)
#define RAM_FRAME UINT64_C(4096)
#define RAM_STRIDE UINT64_C(960)
#define RAM_CAPACITY 128U

struct ram_model {
    uint64_t now,read[8],arm_tick,last_quiet;
    uint32_t frequency,read_frequency[8],arm_frequency;
    unsigned reads,index,clock_calls,arm_calls,quiet_calls,close_calls,last_cancel;
    int clock_result,arm_result,quiet_result,close_result;
};
struct ram_memory {unsigned allocations,releases,live;};
struct ram_counts {unsigned current,terminal,invalid,releases,live;};
struct ram_holder {
    struct ram_counts *counts;
    uint64_t token;
    uint8_t *data;
    unsigned live;
};
struct ram_case {
    struct ram_model *model;
    struct ram_memory *memory;
    struct pt_private_ram_port *port;
    struct pt_readers_activation *ledger;
    struct pt_readers_output *queue;
    struct pt_allocator allocator;
    struct pt_scheduled_grid grid;
    struct pt_readers_backend backend;
    struct ram_counts counts;
    struct pt_scheduled_batch batch;
    struct pt_readers_control command;
    struct pt_readers_owner owners[4];
    struct pt_scheduled_span spans[4];
    struct pt_readers_key keys[4],target[4];
    struct pt_readers_command_receipt command_out;
    struct pt_readers_reader_receipt reader_out;
    uint64_t session,token;
};

static int ram_key_equal(const struct pt_readers_key *a,const struct pt_readers_key *b)
{
    return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&
        a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&
        a->action==b->action&&a->slot==b->slot;
}
static uint64_t ram_first(uint64_t frame)
{
    uint64_t n;
    assert(frame<=UINT64_MAX/RAM_FREQUENCY);
    n=frame*RAM_FREQUENCY;
    assert(n/RAM_RATE<=UINT64_MAX-RAM_EPOCH-1U);
    return RAM_EPOCH+n/RAM_RATE+(n%RAM_RATE!=0);
}
static void *ram_allocate(void *context,size_t bytes)
{
    struct ram_memory *m=context;
    void *p=malloc(bytes);
    if(p){++m->allocations;++m->live;}
    return p;
}
static void ram_release(void *context,void *p)
{
    struct ram_memory *m=context;
    assert(p&&m->live);
    --m->live;++m->releases;free(p);
}
static int ram_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct ram_model *m=context;
    ++m->clock_calls;
    if(m->index<m->reads){
        *ticks=m->read[m->index];*frequency=m->read_frequency[m->index];++m->index;
    }else{*ticks=m->now;*frequency=m->frequency;}
    return m->clock_result;
}
static int ram_arm(void *context,uint64_t ticks,uint32_t frequency)
{
    struct ram_model *m=context;
    ++m->arm_calls;m->arm_tick=ticks;m->arm_frequency=frequency;
    return m->arm_result;
}
static int ram_quiet(void *context,uint64_t ticket,unsigned cancel)
{
    struct ram_model *m=context;
    ++m->quiet_calls;m->last_quiet=ticket;m->last_cancel=cancel;
    return m->quiet_result;
}
static int ram_source_end(void *context)
{
    struct ram_model *m=context;
    ++m->close_calls;return m->close_result;
}
static int ram_current(void *context,uint64_t token,uint64_t generation)
{
    struct ram_holder *h=context;
    ++h->counts->current;
    return h->live&&h->token==token&&generation==RAM_GENERATION;
}
static void ram_terminal(void *context,uint64_t token,int valid)
{
    struct ram_holder *h=context;
    assert(h->live&&h->token==token);
    ++h->counts->terminal;if(!valid)++h->counts->invalid;
}
static void ram_holder_release(void *context,uint64_t token)
{
    struct ram_holder *h=context;
    struct ram_counts *counts=h->counts;
    assert(h->live&&h->token==token&&counts->live);
    h->live=0;--counts->live;++counts->releases;
    free(h->data);free(h);
}
static struct pt_readers_control ram_holder_new(struct ram_case *c,unsigned sample,unsigned slot)
{
    struct ram_holder *h=calloc(1,sizeof(*h));
    assert(h);
    h->counts=&c->counts;h->token=++c->token;h->live=1;++c->counts.live;
    if(sample){
        unsigned i;
        h->data=malloc(RAM_CAPACITY);assert(h->data);
        for(i=0;i<RAM_CAPACITY;++i)h->data[i]=(uint8_t)(17U+slot+i);
    }
    return (struct pt_readers_control){h,sizeof(*h),h->token,ram_current,ram_holder_release,ram_terminal};
}
static uint64_t ram_session(void)
{
    static uint64_t next=UINT64_C(40000);
    assert(next!=UINT64_MAX);return ++next;
}
static struct ram_case *ram_new(void)
{
    struct ram_case *c=calloc(1,sizeof(*c));
    struct pt_private_ram_adapter adapter;
    struct pt_readers_activation_port port;
    assert(c);
    c->model=calloc(1,sizeof(*c->model));c->memory=calloc(1,sizeof(*c->memory));
    c->port=malloc(sizeof(*c->port));assert(c->model&&c->memory&&c->port);
    c->session=ram_session();c->model->now=RAM_EPOCH;c->model->frequency=RAM_FREQUENCY;
    c->model->clock_result=1;c->model->arm_result=1;c->model->close_result=1;
    adapter=(struct pt_private_ram_adapter){c->model,ram_clock,ram_arm,ram_quiet,ram_source_end};
    assert(pt_private_ram_init(c->port,c->session,RAM_GENERATION,RAM_FREQUENCY,&adapter,128,256,4));
    c->allocator=(struct pt_allocator){c->memory,ram_allocate,ram_release};
    c->grid=(struct pt_scheduled_grid){RAM_EPOCH,RAM_GENERATION,RAM_FREQUENCY,RAM_RATE};
    port=pt_private_ram_api(c->port);
    assert(pt_readers_activation_open(&c->allocator,&c->grid,c->session,&port,&c->ledger)==PT_SCHEDULED_OK);
    assert(pt_readers_activation_api(c->ledger,&c->backend)==PT_SCHEDULED_OK);
    assert(pt_readers_open(&c->allocator,&c->grid,c->session,&c->backend,2,8,&c->queue)==PT_SCHEDULED_OK);
    assert(c->memory->live==2&&!c->counts.live&&!c->port->effects);
    return c;
}
static struct pt_private_ram_command *ram_command(struct ram_case *c,uint64_t ticket)
{
    unsigned i;
    for(i=0;i<2;++i)if(c->port->command[i].live&&c->port->command[i].packet.ticket==ticket)return c->port->command+i;
    return NULL;
}
static unsigned ram_readers(const struct ram_case *c)
{
    unsigned i,n=0;
    for(i=0;i<8;++i)n+=c->port->reader[i].live;
    return n;
}
static void ram_sources_unchanged(const struct ram_case *c)
{
    unsigned i,j;
    for(i=0;i<8;++i)if(c->port->reader[i].live){
        const struct pt_private_ram_reader *r=c->port->reader+i;
        assert(r->data&&r->bytes==64);
        /* This fixture owns a genuine128-byte capacity for each held source;
         * only64 bytes are active geometry, but all capacity stays immutable. */
        for(j=0;j<RAM_CAPACITY;++j)assert(r->data[j]==(uint8_t)(17U+r->key.slot+j));
    }
}
static uint64_t ram_enqueue(struct ram_case *c,enum pt_scheduled_kind kind,unsigned mask,uint64_t frame)
{
    unsigned slot,n=0;
    uint64_t ticket=0;
    memset(&c->batch,0,sizeof(c->batch));memset(c->owners,0,sizeof(c->owners));memset(c->target,0,sizeof(c->target));
    c->batch.frame=frame;c->batch.generation=RAM_GENERATION;c->command=ram_holder_new(c,0,0);
    assert(mask&&!(mask&~15U));
    for(slot=0;slot<4;++slot)if(mask&(1U<<slot)){
        struct pt_scheduled_action *a=c->batch.action+n;
        a->kind=kind;a->slot=slot;
        if(kind!=PT_SCHEDULED_STOP){a->period=(uint16_t)(428U+slot);a->volume=(uint8_t)(32U+slot);}
        if(kind==PT_SCHEDULED_TRIGGER){
            struct ram_holder *h;
            c->owners[n].control=ram_holder_new(c,1,slot);h=c->owners[n].control.context;
            c->spans[n]=(struct pt_scheduled_span){h->data,RAM_CAPACITY};
            c->owners[n].spans=c->spans+n;c->owners[n].count=1;a->data=h->data;a->words=32;
        }else c->target[n]=c->keys[slot];
        ++n;
    }
    c->batch.count=n;
    assert(pt_readers_enqueue(c->queue,&c->batch,kind==PT_SCHEDULED_TRIGGER?NULL:c->target,&c->command,
        kind==PT_SCHEDULED_TRIGGER?c->owners:NULL,&ticket)==PT_SCHEDULED_OK);
    /* Ended declaration scratch is scrubbed, never opaque holder storage. */
    memset(&c->command,0x3c,sizeof(c->command));memset(c->owners,0x3c,sizeof(c->owners));memset(c->spans,0x3c,sizeof(c->spans));
    return ticket;
}
static void ram_publish(struct ram_case *c,uint64_t ticket,uint64_t frame)
{
    unsigned calls=c->model->clock_calls;
    struct pt_private_ram_command *command;
    assert(pt_readers_publish(c->queue,ticket)==PT_SCHEDULED_OK);
    command=ram_command(c,ticket);assert(command);
    assert(c->model->clock_calls==calls+3U&&command->packet.frame==frame);
    assert(command->packet.first==ram_first(frame)&&command->packet.last==ram_first(frame+1));
    assert(command->packet.session==c->session&&command->packet.generation==RAM_GENERATION);
    assert(c->model->arm_tick==command->packet.first-128U&&c->model->arm_frequency==RAM_FREQUENCY);
}
static void ram_script(struct ram_case *c,const uint64_t *reads,unsigned count)
{
    unsigned i;
    assert(count<=8);c->model->reads=count;c->model->index=0;
    for(i=0;i<count;++i){c->model->read[i]=reads[i];c->model->read_frequency[i]=RAM_FREQUENCY;}
}
static void ram_dispatch_good(struct ram_case *c,uint64_t ticket,unsigned aperture)
{
    struct pt_private_ram_command *command=ram_command(c,ticket);
    uint64_t first,reads[4];
    unsigned callbacks=c->counts.current,terminals=c->counts.terminal,releases=c->counts.releases;
    unsigned allocations=c->memory->allocations;
    assert(command);first=command->packet.first;
    reads[0]=aperture?command->arm_tick:first;reads[1]=first;reads[2]=first;reads[3]=first;
    ram_script(c,reads,aperture?4U:3U);
    assert(pt_private_ram_dispatch(c->port)==PT_READERS_ACTIVATION_COMMITTED);
    assert(c->port->ledger_result==PT_READERS_ACTIVATION_COMMITTED&&c->port->trace_count==(aperture?4U:3U));
    assert(c->port->trace[c->port->trace_count-2U].ticks==first&&c->port->trace[c->port->trace_count-1U].ticks==first);
    assert(c->counts.current==callbacks&&c->counts.terminal==terminals&&c->counts.releases==releases&&c->memory->allocations==allocations);
    ram_sources_unchanged(c);
    c->model->now=first;c->model->reads=c->model->index=0;
    assert(pt_readers_poll_command(c->queue,ticket,&c->command_out)==PT_SCHEDULED_PENDING);
}
static void ram_detach(struct ram_case *c,uint64_t ticket,enum pt_scheduled_result expected)
{
    struct pt_readers_command_receipt before;
    c->model->quiet_result=1;memset(&c->command_out,0xa7,sizeof(c->command_out));memcpy(&before,&c->command_out,sizeof(before));
    assert(pt_readers_poll_command(c->queue,ticket,&c->command_out)==expected&&!ram_command(c,ticket));
    if(expected==PT_SCHEDULED_BACKEND)assert(!memcmp(&before,&c->command_out,sizeof(before)));
    c->model->quiet_result=0;
}
static void ram_reader_cancel(struct ram_case *c,uint64_t ticket,unsigned action,enum pt_scheduled_result expected)
{
    struct pt_readers_reader_receipt before;
    memset(&c->reader_out,0xb8,sizeof(c->reader_out));memcpy(&before,&c->reader_out,sizeof(before));
    assert(pt_readers_cancel_reader(c->queue,ticket,action,&c->reader_out)==expected);
    if(expected==PT_SCHEDULED_BACKEND)assert(!memcmp(&before,&c->reader_out,sizeof(before)));
}
static void ram_finish(struct ram_case *c)
{
    assert(!pt_readers_commands_held(c->queue)&&!pt_readers_readers_held(c->queue)&&!c->counts.live);
    assert(c->counts.terminal==c->counts.releases&&!ram_readers(c)&&!c->port->mask);
    assert(pt_readers_stop(c->queue)==(c->counts.invalid?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK));
    assert(pt_private_ram_source_close(c->port)==1&&c->port->source_closed);
    assert(pt_readers_close(c->queue));c->queue=NULL;
    assert(pt_readers_activation_close(&c->ledger)&&!c->ledger);
    assert(!c->memory->live&&c->memory->allocations==c->memory->releases);
    free(c->port);free(c->model);free(c->memory);free(c);
}

static void ram_clock_exact_one(void)
{
    const int refused[3]={0,-1,2};
    struct ram_case *c=ram_new();
    struct pt_readers_activation_port api=pt_private_ram_api(c->port);
    struct pt_private_ram_port *before=malloc(sizeof(*before));
    unsigned i,dispatch;
    assert(before);
    for(dispatch=0;dispatch<2;++dispatch)for(i=0;i<3;++i){
        uint64_t ticks=UINT64_C(0x123456789abcdef0);
        uint32_t frequency=0x89abcdefU;
        c->port->dispatching=dispatch;c->model->clock_result=refused[i];
        memcpy(before,c->port,sizeof(*before));
        assert(api.read_clock(api.context,&ticks,&frequency)==0);
        assert(ticks==UINT64_C(0x123456789abcdef0)&&frequency==0x89abcdefU&&!memcmp(before,c->port,sizeof(*before)));
    }
    c->model->clock_result=1;c->port->dispatching=1;
    {uint64_t ticks=0;uint32_t frequency=0;
     assert(api.read_clock(api.context,&ticks,&frequency)==1&&ticks==RAM_EPOCH&&frequency==RAM_FREQUENCY&&c->port->trace_count==1);}
    for(i=0;i<2;++i){
        uint64_t ticks=UINT64_C(0x123456789abcdef0),last=c->port->last_clock;
        uint32_t frequency=0x89abcdefU;
        unsigned trace=c->port->trace_count;
        c->model->now=i?RAM_EPOCH-1U:RAM_EPOCH;
        c->model->frequency=i?RAM_FREQUENCY:RAM_FREQUENCY+1U;
        assert(api.read_clock(api.context,&ticks,&frequency)==0);
        assert(ticks==UINT64_C(0x123456789abcdef0)&&frequency==0x89abcdefU&&c->port->last_clock==last);
        assert(c->port->trace_count==trace+1U&&c->port->trace[trace].ticks==c->model->now&&c->port->trace[trace].frequency==c->model->frequency);
    }
    c->model->now=RAM_EPOCH;c->model->frequency=RAM_FREQUENCY;
    c->port->dispatching=0;
    /* Genuine public ledger API refuses known port-storage outputs before callback. */
    {unsigned calls=c->model->clock_calls;uint32_t frequency=0x1234;
     memcpy(before,c->port,sizeof(*before));
     assert(c->backend.read_clock(c->backend.context,&c->port->last_clock,&frequency)==0);
     assert(c->model->clock_calls==calls&&frequency==0x1234&&!memcmp(before,c->port,sizeof(*before)));}
    free(before);ram_finish(c);
}
static void ram_four_slot_lifecycle(void)
{
    struct ram_case *c=ram_new();
    uint64_t first,control,replacement,stop;
    struct pt_readers_key old,replacement_key,before;
    unsigned i;
    first=ram_enqueue(c,PT_SCHEDULED_TRIGGER,15,RAM_FRAME);ram_publish(c,first,RAM_FRAME);
    assert(ram_command(c,first)->packet.first==60634&&ram_command(c,first)->packet.last==60649);
    memset(&before,0xb3,sizeof(before));memcpy(&old,&before,sizeof(old));
    assert(pt_readers_reader_key(c->queue,first,0,&old)==PT_SCHEDULED_STALE&&!memcmp(&old,&before,sizeof(old)));
    c->model->now=ram_command(c,first)->packet.first-1U;
    assert(pt_readers_activation_fire(c->ledger,first)==PT_READERS_ACTIVATION_EARLY&&!c->port->effects);
    ram_dispatch_good(c,first,0);
    for(i=0;i<4;++i)assert(pt_readers_reader_key(c->queue,first,i,c->keys+i)==PT_SCHEDULED_OK&&ram_key_equal(c->keys+i,c->port->slot+i));
    assert(c->port->mask==15&&ram_readers(c)==4&&c->port->effects==4);
    assert(!pt_readers_activation_close(&c->ledger));ram_detach(c,first,PT_SCHEDULED_OK);
    assert(pt_readers_readers_held(c->queue)==4&&ram_readers(c)==4&&c->counts.live==4);
    control=ram_enqueue(c,PT_SCHEDULED_CONTROL,15,RAM_FRAME+RAM_STRIDE);ram_publish(c,control,RAM_FRAME+RAM_STRIDE);
    assert(ram_command(c,control)->packet.first==74822&&ram_command(c,control)->packet.last==74837);
    ram_dispatch_good(c,control,1);ram_detach(c,control,PT_SCHEDULED_OK);
    old=c->keys[0];replacement=ram_enqueue(c,PT_SCHEDULED_TRIGGER,1,RAM_FRAME+2U*RAM_STRIDE);
    memcpy(&before,&old,sizeof(before));
    assert(pt_readers_reader_key(c->queue,first,0,&before)==PT_SCHEDULED_STALE&&ram_key_equal(&before,&old));
    assert(ram_key_equal(c->port->slot,&old));
    ram_publish(c,replacement,RAM_FRAME+2U*RAM_STRIDE);ram_dispatch_good(c,replacement,1);
    assert(pt_readers_reader_key(c->queue,replacement,0,&replacement_key)==PT_SCHEDULED_OK&&!ram_key_equal(&replacement_key,&old));
    assert(pt_readers_reader_key(c->queue,first,0,&before)==PT_SCHEDULED_STALE);
    ram_reader_cancel(c,first,0,PT_SCHEDULED_OK);
    assert(ram_key_equal(c->port->slot,&replacement_key)&&c->port->mask==15&&ram_readers(c)==4);
    c->keys[0]=replacement_key;ram_detach(c,replacement,PT_SCHEDULED_OK);
    stop=ram_enqueue(c,PT_SCHEDULED_STOP,15,RAM_FRAME+3U*RAM_STRIDE);ram_publish(c,stop,RAM_FRAME+3U*RAM_STRIDE);
    ram_dispatch_good(c,stop,1);assert(!c->port->mask&&ram_readers(c)==4&&c->port->effects==13);
    for(i=0;i<2;++i)assert(pt_readers_poll_reader(c->queue,i?first:replacement,i?i:0,&c->reader_out)==PT_SCHEDULED_OK);
    assert(pt_readers_readers_held(c->queue)==4&&pt_readers_commands_held(c->queue)==1);
    ram_detach(c,stop,PT_SCHEDULED_OK);
    for(i=2;i<4;++i)assert(pt_readers_poll_reader(c->queue,first,i,&c->reader_out)==PT_SCHEDULED_OK);
    ram_finish(c);
}
static void ram_pending_exact_old_key(void)
{
    struct ram_case *c=ram_new();
    uint64_t first,replacement,pending;
    struct pt_readers_key successor;
    unsigned i,effects,quiet;
    first=ram_enqueue(c,PT_SCHEDULED_TRIGGER,15,RAM_FRAME);ram_publish(c,first,RAM_FRAME);ram_dispatch_good(c,first,1);
    for(i=0;i<4;++i)assert(pt_readers_reader_key(c->queue,first,i,c->keys+i)==PT_SCHEDULED_OK);
    ram_detach(c,first,PT_SCHEDULED_OK);
    replacement=ram_enqueue(c,PT_SCHEDULED_TRIGGER,1,RAM_FRAME+RAM_STRIDE);ram_publish(c,replacement,RAM_FRAME+RAM_STRIDE);ram_dispatch_good(c,replacement,1);
    assert(pt_readers_reader_key(c->queue,replacement,0,&successor)==PT_SCHEDULED_OK);c->keys[0]=successor;
    ram_detach(c,replacement,PT_SCHEDULED_OK);
    pending=ram_enqueue(c,PT_SCHEDULED_CONTROL,3,RAM_FRAME+2U*RAM_STRIDE);ram_publish(c,pending,RAM_FRAME+2U*RAM_STRIDE);
    effects=c->port->effects;quiet=c->model->quiet_calls;
    ram_reader_cancel(c,first,0,PT_SCHEDULED_OK);
    assert(c->port->armed_index>=0&&c->model->quiet_calls==quiet&&ram_key_equal(c->port->slot,&successor));
    c->model->quiet_result=1;ram_reader_cancel(c,first,1,PT_SCHEDULED_OK);
    assert(c->port->armed_index==-1&&c->model->last_quiet==pending&&c->model->last_cancel==1);
    assert(ram_command(c,pending)->finished&&!ram_command(c,pending)->packet.action[0].data&&!ram_command(c,pending)->packet.action[1].data);
    assert(pt_private_ram_dispatch(c->port)==PT_READERS_ACTIVATION_FAILED&&c->port->effects==effects);
    assert(ram_key_equal(c->port->slot,&successor)&&c->port->mask==13);
    ram_detach(c,pending,PT_SCHEDULED_OK);
    ram_reader_cancel(c,replacement,0,PT_SCHEDULED_OK);
    for(i=2;i<4;++i)ram_reader_cancel(c,first,i,PT_SCHEDULED_OK);
    ram_finish(c);
}
static void ram_dispatch_boundaries(unsigned mode)
{
    struct ram_case *c=ram_new();
    uint64_t ticket=ram_enqueue(c,PT_SCHEDULED_TRIGGER,15,RAM_FRAME),reads[4];
    struct pt_private_ram_command *command;
    unsigned i,effects=0;
    enum pt_scheduled_result cleanup=PT_SCHEDULED_OK;
    ram_publish(c,ticket,RAM_FRAME);command=ram_command(c,ticket);assert(command);
    reads[0]=command->packet.first;reads[1]=command->packet.first;reads[2]=command->packet.first;reads[3]=command->packet.first;
    if(mode==0)reads[0]=command->arm_tick-1U;
    else if(mode==1)reads[0]=command->packet.last;
    else if(mode==2){/* Actual frequency mismatch is recorded, never normalized. */}
    else if(mode==3){reads[1]=reads[0]-1U;cleanup=PT_SCHEDULED_BACKEND;}
    else if(mode==4){reads[0]=command->packet.first-1U;reads[1]=command->packet.last;}
    else if(mode==5){reads[2]=command->packet.last;effects=4;cleanup=PT_SCHEDULED_BACKEND;}
    else assert(0);
    ram_script(c,reads,3);
    if(mode==2)c->model->read_frequency[0]=RAM_FREQUENCY+1U;
    assert(pt_private_ram_dispatch(c->port)==PT_READERS_ACTIVATION_FAILED&&c->port->effects==effects);
    if(mode==5)assert(c->port->ledger_result==PT_READERS_ACTIVATION_FAILED&&ram_readers(c)==4);
    else assert(!ram_readers(c));
    if(mode==2)assert(c->port->trace_count==1&&c->port->trace[0].frequency==RAM_FREQUENCY+1U);
    if(mode==3)assert(c->port->trace_count==2&&c->port->trace[1].ticks<c->port->trace[0].ticks);
    assert(pt_private_ram_source_close(c->port)==1);
    c->model->quiet_result=1;
    memset(&c->command_out,0xa7,sizeof(c->command_out));
    assert(pt_readers_cancel_command(c->queue,ticket,&c->command_out)==cleanup);
    for(i=0;i<4;++i)ram_reader_cancel(c,ticket,i,cleanup);
    ram_finish(c);
}
static void ram_safe_arm_refusal(void)
{
    struct ram_case *c=ram_new();
    uint64_t ticket=ram_enqueue(c,PT_SCHEDULED_TRIGGER,15,RAM_FRAME);
    c->model->arm_result=0;
    assert(pt_readers_publish(c->queue,ticket)==PT_SCHEDULED_PENDING&&!ram_command(c,ticket)&&c->port->armed_index==-1&&!c->port->effects);
    assert(pt_readers_commands_held(c->queue)==1&&pt_readers_readers_held(c->queue)==4&&c->counts.live==5);
    assert(pt_readers_stop(c->queue)==PT_SCHEDULED_OK&&!c->counts.live);
    ram_finish(c);
}
static void ram_uncertain_arm_and_close(void)
{
    struct ram_case *c=ram_new();
    uint64_t ticket=ram_enqueue(c,PT_SCHEDULED_TRIGGER,15,RAM_FRAME);
    struct pt_private_ram_port *before=malloc(sizeof(*before));
    struct pt_readers_command_receipt command_before;
    struct pt_readers_reader_receipt reader_before;
    unsigned i;
    assert(before);c->model->arm_result=-1;
    assert(pt_readers_publish(c->queue,ticket)==PT_SCHEDULED_BACKEND&&ram_command(c,ticket)->uncertain);
    assert(pt_private_ram_dispatch(c->port)==PT_READERS_ACTIVATION_FAILED&&!c->port->effects);
    c->model->close_result=-1;memcpy(before,c->port,sizeof(*before));
    assert(pt_private_ram_source_close(c->port)==-1&&!memcmp(before,c->port,sizeof(*before)));
    c->model->quiet_result=-1;memset(&c->command_out,0xa7,sizeof(c->command_out));memcpy(&command_before,&c->command_out,sizeof(command_before));
    memset(&c->reader_out,0xb8,sizeof(c->reader_out));memcpy(&reader_before,&c->reader_out,sizeof(reader_before));
    assert(pt_readers_cancel_command(c->queue,ticket,&c->command_out)==PT_SCHEDULED_BACKEND&&!memcmp(&command_before,&c->command_out,sizeof(command_before)));
    assert(pt_readers_cancel_reader(c->queue,ticket,0,&c->reader_out)==PT_SCHEDULED_BACKEND&&!memcmp(&reader_before,&c->reader_out,sizeof(reader_before)));
    assert(pt_readers_commands_held(c->queue)==1&&pt_readers_readers_held(c->queue)==4&&c->counts.live==5);
    assert(!pt_readers_close(c->queue)&&!pt_readers_activation_close(&c->ledger)&&c->ledger);
    c->model->close_result=1;assert(pt_private_ram_source_close(c->port)==1);
    c->model->quiet_result=1;ram_detach(c,ticket,PT_SCHEDULED_BACKEND);
    for(i=0;i<4;++i)ram_reader_cancel(c,ticket,i,PT_SCHEDULED_BACKEND);
    assert(c->counts.invalid>0);free(before);ram_finish(c);
}
int main(void)
{
    unsigned mode;
    ram_clock_exact_one();ram_four_slot_lifecycle();ram_pending_exact_old_key();
    for(mode=0;mode<6;++mode)ram_dispatch_boundaries(mode);
    ram_safe_arm_refusal();ram_uncertain_arm_and_close();
    puts("NATIVE RAM PORT SOFTWARE PASS: genuine public holders; exact original windows; RAM-only adoption; independent quiet proofs; no IRQ or hardware qualification");
    return 0;
}
