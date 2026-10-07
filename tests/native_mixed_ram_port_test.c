/* Genuine resources/pins/cache helpers, unchanged old fixture entry renamed
 * and NOT CALLED. The new port production C is a separate compilation unit.
 * These tests make only ordinary RAM effects; no CIA/device/native claims. */
#define PT_MIXED_READERS_TEST_MAIN pt_unused_mixed_resources_entry
#include "mixed_scheduled_readers_test.c"
#include "native_mixed_ram_port.h"

struct nr_memory {unsigned live,allocations,releases;};
struct nr_clock {
    struct pt_private_mixed_ram_port *port;
    struct pt_mixed_activation_registration registration;
    uint64_t now,script[64],active_ticket,last_ticket,arm_tick;
    uint32_t frequency,script_frequency[64];
    unsigned length,index,clocks,arms,tickets,shutdowns,probes,last_cancel,closed,hook,hooks;
    unsigned fail_ticket_call;
    int clock_result,arm_result,ticket_result,source_result,probe_result;
};
struct nr_case {
    struct trial *trial;
    struct pt_private_mixed_ram_port *port;
    struct nr_clock *clock;
    struct nr_memory *memory;
    struct pt_mixed_readers_activation *owner;
    struct pt_mixed_activation_config *config;
    void *workspace;
    size_t save_bytes;
    uint8_t *before;
    uint64_t session;
};
static unsigned nr_cases;
static uint64_t nr_session(void)
{static uint64_t next=UINT64_C(60000);return ++next;}
static void *nr_allocate(void *context,size_t bytes)
{struct nr_memory *m=context;void *p=malloc(bytes);if(p){++m->live;++m->allocations;}return p;}
static void nr_release(void *context,void *p)
{struct nr_memory *m=context;assert(p&&m->live);--m->live;++m->releases;free(p);}
static int nr_registration_same(const struct pt_mixed_activation_registration *a,const struct pt_mixed_activation_registration *b)
{return a->owner==b->owner&&a->queue==b->queue&&a->session==b->session&&a->generation==b->generation;}
static void nr_hook(struct nr_clock *c,unsigned which)
{if(c->hook==which){c->hook=0;++c->hooks;assert(pt_private_mixed_ram_dispatch(c->port)==PT_MIXED_ACTIVATION_FAILED);}}
static int nr_read_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct nr_clock *c=context;++c->clocks;nr_hook(c,1);
 if(c->index<c->length){*ticks=c->script[c->index];*frequency=c->script_frequency[c->index++];}
 else{*ticks=c->now;*frequency=c->frequency;}
 return c->clock_result;}
static int nr_arm(void *context,const struct pt_mixed_activation_registration *r,uint64_t ticket,uint64_t ticks,uint32_t frequency)
{struct nr_clock *c=context;assert(nr_registration_same(r,&c->registration)&&ticket&&frequency==709379);
 ++c->arms;c->last_ticket=ticket;c->arm_tick=ticks;nr_hook(c,2);
 if(c->arm_result!=0){c->active_ticket=ticket;}return c->arm_result;}
static int nr_ticket(void *context,const struct pt_mixed_activation_registration *r,uint64_t ticket,unsigned cancel)
{struct nr_clock *c=context;int result=c->ticket_result;assert(nr_registration_same(r,&c->registration)&&ticket&&cancel<=1);
 ++c->tickets;c->last_ticket=ticket;c->last_cancel=cancel;nr_hook(c,3);
 if(c->fail_ticket_call==c->tickets)result=-1;
 if(result==1&&c->active_ticket==ticket){c->active_ticket=0;}return result;}
static int nr_shutdown(void *context,const struct pt_mixed_activation_registration *r)
{struct nr_clock *c=context;assert(nr_registration_same(r,&c->registration));++c->shutdowns;nr_hook(c,4);
 if(c->source_result==1){assert(!c->active_ticket);c->closed=1;}return c->source_result;}
static int nr_source_probe(void *context,const struct pt_mixed_activation_registration *r)
{struct nr_clock *c=context;assert(nr_registration_same(r,&c->registration));++c->probes;nr_hook(c,5);
 if(c->probe_result==1){assert(c->closed&&!c->active_ticket);}return c->probe_result;}
static struct pt_private_mixed_ram_adapter nr_adapter(struct nr_case *c)
{return (struct pt_private_mixed_ram_adapter){c->clock,sizeof(*c->clock),nr_read_clock,nr_arm,nr_ticket,nr_shutdown,nr_source_probe};}
static struct nr_case *nr_make(unsigned bits,unsigned cache_bits,unsigned bind)
{
    struct nr_case *c=calloc(1,sizeof(*c));struct pt_private_mixed_ram_adapter adapter;
    assert(c);++nr_cases;c->trial=trial_make(bits,cache_bits);c->port=calloc(1,sizeof(*c->port));
    c->clock=calloc(1,sizeof(*c->clock));c->memory=calloc(1,sizeof(*c->memory));c->config=calloc(1,sizeof(*c->config));
    c->workspace=calloc(1,pt_mixed_activation_workspace_size());assert(c->port&&c->clock&&c->memory&&c->config&&c->workspace);
    c->session=nr_session();c->clock->now=100;c->clock->frequency=709379;
    c->clock->clock_result=c->clock->arm_result=c->clock->ticket_result=c->clock->source_result=c->clock->probe_result=1;
    c->clock->port=c->port;adapter=nr_adapter(c);
    assert(pt_private_mixed_ram_init(c->port,c->session,17,709379,&adapter,128,256,8));
    c->config->allocator=(struct pt_allocator){c->memory,nr_allocate,nr_release};
    c->config->allocator_context=(struct pt_mixed_readers_span){c->memory,sizeof(*c->memory)};
    c->config->grid=(struct pt_mixed_readers_grid){100,17,709379,48000};c->config->session=c->session;
    c->config->control_budget=pt_mixed_activation_control_size();c->config->queue_budget=pt_mixed_readers_control_size();
    c->config->port=pt_private_mixed_ram_api(c->port);
    assert(pt_mixed_activation_open(c->config,c->workspace,pt_mixed_activation_workspace_size(),&c->owner)==PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_borrow_queue(c->owner,&c->trial->queue)==PT_MIXED_READERS_OK);
    if(bind)c->clock->registration=(struct pt_mixed_activation_registration){c->owner,c->trial->queue,c->session,17};
    /* Immediately bind original genuine open/borrow before any factory, input
     * preparation, enqueue, validation, publication or source exposure. */
    if(bind)assert(pt_private_mixed_ram_bind(c->port,c->owner,c->trial->queue,c->session,17));
    assert(c->memory->live==2&&!c->clock->clocks&&!c->clock->arms);
    c->before=save(c->trial->resources,&c->save_bytes);return c;
}
static void nr_unchanged(struct nr_case *c)
{same_save(c->trial->resources,c->before,c->save_bytes);}
static void nr_drop(struct nr_case *c)
{
    assert(!c->owner&&!c->memory->live&&!c->clock->active_ticket);nr_unchanged(c);
    c->trial->queue=NULL;trial_drop(c->trial);free(c->before);free(c->workspace);free(c->config);free(c->memory);free(c->clock);free(c->port);free(c);
}
static unsigned nr_live_readers(struct nr_case *c)
{unsigned i,n=0;for(i=0;i<32;++i)n+=c->port->reader[i].live;return n;}
static struct pt_private_mixed_ram_command *nr_command(struct nr_case *c,uint64_t ticket)
{unsigned i;for(i=0;i<2;++i)if(c->port->command[i].live&&c->port->command[i].packet.ticket==ticket)return c->port->command+i;return NULL;}
static uint64_t nr_enqueue(struct nr_case *c,unsigned count,unsigned generation,uint64_t frame,unsigned all_card)
{
    struct trial *t=c->trial;uint64_t ticket=0;unsigned i;
    if(!all_card)trigger_input(t,count,generation,frame);
    else{struct pt_amigus_voice_request request={8000,1,0,64,128};struct pt_playback_format format={(uint8_t)t->resources->cache_bits,0,(uint8_t)t->resources->little,0};
        memset(t->input,0,sizeof(*t->input));holder_init(t->command+generation,100+generation);t->command[generation].queue=t->queue;
        t->input->command=control_of(t->command+generation);t->input->batch=(struct pt_mixed_readers_batch){17,frame,count,{{0}}};
        for(i=0;i<count;++i){struct pt_mixed_readers_action *a=t->input->batch.action+i;struct holder *h=t->reader+generation*16+i;
            a->route=PT_MIXED_READERS_AMIGUS;a->slot=i;a->kind=PT_MIXED_READERS_TRIGGER;
            t->input->reader[i]=source(h,t->resources,PT_MIXED_READERS_AMIGUS,i,i%2,1000+generation*16+i);h->queue=t->queue;
            assert(pt_amigus_voice_plan_prepare(t->resources->document.project.samples+i%2,&format,&request,t->input->reader[i].card.address,t->input->reader[i].card.logical_bytes,&a->geometry.amigus));
        }
    }
    assert(pt_mixed_activation_enqueue(c->owner,t->input,&ticket)==PT_MIXED_READERS_OK);return ticket;
}
static void nr_script(struct nr_case *c,const uint64_t *ticks,unsigned count)
{unsigned i;assert(count<=64);c->clock->length=count;c->clock->index=0;for(i=0;i<count;++i){c->clock->script[i]=ticks[i];c->clock->script_frequency[i]=709379;}}
static void nr_publish(struct nr_case *c,uint64_t ticket)
{assert(pt_mixed_activation_publish(c->owner,ticket)==PT_MIXED_READERS_OK);assert(nr_command(c,ticket)&&!c->port->effects);}
static void nr_dispatch(struct nr_case *c,uint64_t ticket,unsigned aperture)
{
    struct pt_private_mixed_ram_command *command=nr_command(c,ticket);uint64_t ticks[4];unsigned effects=c->port->effects,allocations=c->memory->allocations;
    assert(command);ticks[0]=aperture?command->arm_tick:command->packet.first;ticks[1]=ticks[2]=ticks[3]=command->packet.first;
    nr_script(c,ticks,aperture?4U:3U);
    assert(pt_private_mixed_ram_dispatch(c->port)==PT_MIXED_ACTIVATION_COMMITTED&&c->port->ledger_result==PT_MIXED_ACTIVATION_COMMITTED);
    assert(c->port->effects==effects+command->packet.count&&c->memory->allocations==allocations);
    assert(c->clock->index==(aperture?4U:3U));c->clock->now=command->packet.first;c->clock->length=c->clock->index=0;nr_unchanged(c);
}
static void nr_drain(struct nr_case *c,uint64_t ticket,unsigned count,unsigned order,unsigned failed)
{
    unsigned i;enum pt_mixed_readers_result result=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
    if(!order)assert(pt_mixed_activation_service_command(c->owner,ticket,1,NULL)==result);
    for(i=0;i<count;++i)assert(pt_mixed_activation_service_reader(c->owner,ticket,i,1,NULL)==result);
    if(order)assert(pt_mixed_activation_service_command(c->owner,ticket,1,NULL)==result);
}
static void nr_close(struct nr_case *c)
{assert(pt_mixed_activation_close(&c->owner)&&!c->owner&&c->port->source_closed);assert(pt_mixed_activation_close(&c->owner));}
/* fire adopts the private activation/RAM ledgers, not the queue observation.
 * A genuine noncancelling reader receipt alone observes that original adoption;
 * it must retain command, reader, callback owner and every sample/cache pin. */
static void nr_observe_active(struct nr_case *c,uint64_t ticket,unsigned action,unsigned generation)
{
    struct pt_mixed_readers_reader_receipt receipt;
    struct pt_private_mixed_ram_command *command=nr_command(c,ticket),*before=malloc(sizeof(c->port->command));
    struct nr_clock *clock_before=malloc(sizeof(*clock_before));
    struct holder *h=c->trial->reader+generation*16+action;struct pt_sample_version *pin=h->pin;
    unsigned commands=pt_mixed_readers_commands_held(c->trial->queue),readers=pt_mixed_readers_readers_held(c->trial->queue),live=nr_live_readers(c);
    assert(before&&clock_before&&command&&action<command->packet.count&&commands&&readers&&pin&&h->live&&!h->terminals&&!h->releases);
    memcpy(before,c->port->command,sizeof(c->port->command));memcpy(clock_before,c->clock,sizeof(*clock_before));
    memset(&receipt,0,sizeof(receipt));
    assert(pt_mixed_activation_service_reader(c->owner,ticket,action,0,&receipt)==PT_MIXED_READERS_PENDING);
    assert(receipt.domain==PT_MIXED_READER_DOMAIN&&receipt.state==PT_MIXED_READER_ACTIVE&&receipt.adoption==PT_MIXED_ADOPTED&&
        keys_equal(&receipt.key,command->packet.key+action)&&receipt.key.queue==c->trial->queue&&receipt.key.session==c->session&&receipt.key.generation==17&&
        receipt.reference&&receipt.binding.context==h&&receipt.binding.context_bytes==sizeof(*h)&&receipt.observed==command->packet.first&&receipt.issued==command->packet.first);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==commands&&pt_mixed_readers_readers_held(c->trial->queue)==readers&&nr_live_readers(c)==live&&
        !memcmp(before,c->port->command,sizeof(c->port->command))&&!memcmp(clock_before,c->clock,sizeof(*clock_before))&&h->pin==pin&&h->live&&!h->terminals&&!h->releases);
    nr_unchanged(c);free(clock_before);free(before);
}
static void nr_success(unsigned bits,unsigned cache_bits,unsigned order,unsigned all_card)
{
    struct nr_case *c=nr_make(bits,cache_bits,1);uint64_t ticket;unsigned i;
    ticket=nr_enqueue(c,16,0,960,all_card);nr_publish(c,ticket);nr_dispatch(c,ticket,1);
    assert(nr_live_readers(c)==16&&c->port->effects==16);
    for(i=0;i<16;++i){struct pt_mixed_readers_key key;
        nr_observe_active(c,ticket,i,0);
        assert(pt_mixed_activation_reader_key(c->owner,ticket,i,&key)==PT_MIXED_READERS_OK&&key.route==(all_card||i>=4?PT_MIXED_READERS_AMIGUS:PT_MIXED_READERS_PAULA));
    }
    if(!order){assert(pt_mixed_activation_service_command(c->owner,ticket,0,NULL)==PT_MIXED_READERS_OK);assert(!nr_command(c,ticket)&&nr_live_readers(c)==16);}
    for(i=0;i<16;++i)assert(pt_mixed_activation_service_reader(c->owner,ticket,i,1,NULL)==PT_MIXED_READERS_OK);
    if(order){assert(nr_command(c,ticket)&&!nr_live_readers(c));assert(pt_mixed_activation_service_command(c->owner,ticket,0,NULL)==PT_MIXED_READERS_OK);}
    assert(!nr_live_readers(c)&&!c->port->mask);nr_close(c);assert(c->clock->shutdowns==1&&!c->clock->probes);nr_drop(c);
}
static void nr_binding_and_empty(unsigned cancel_before)
{
    struct nr_case *c=nr_make(24,16,1);struct pt_private_mixed_ram_adapter adapter=nr_adapter(c);struct pt_private_mixed_ram_port *saved=malloc(sizeof(*saved));
    assert(saved);memcpy(saved,c->port,sizeof(*saved));
    assert(!pt_private_mixed_ram_bind(c->port,c->owner,c->trial->queue,c->session,17));
    assert(!pt_private_mixed_ram_init(c->port,c->session,17,709379,&adapter,128,256,8));assert(!memcmp(saved,c->port,sizeof(*saved)));free(saved);
    if(cancel_before){uint64_t ticket=nr_enqueue(c,16,0,960,0);assert(ticket);assert(pt_mixed_activation_stop(c->owner)==PT_MIXED_READERS_OK);}
    assert(!c->port->publishes&&!c->clock->arms&&!c->port->effects);nr_close(c);assert(c->clock->shutdowns==1&&!c->clock->probes);nr_drop(c);
}
static void nr_unbound_cleanup(void)
{
    struct nr_case *c=nr_make(8,8,0);struct pt_private_mixed_ram_adapter a=nr_adapter(c);
    /* Genuine owner/queue cleanup before exposure; this tests the universal
     * branch, not the exceptional retained failed-constructor path. */
    nr_close(c);assert(c->port->unbound_terminal&&!c->port->source_outcome&&!c->port->quiet_outcome&&!c->clock->shutdowns&&!c->clock->probes&&!c->port->bound&&!c->port->registration.owner&&!c->port->registration.queue);
    assert(!pt_private_mixed_ram_bind(c->port,(void *)(uintptr_t)1,(void *)(uintptr_t)2,c->session,17));
    assert(!pt_private_mixed_ram_init(c->port,c->session,17,709379,&a,128,256,8));
    assert(pt_private_mixed_ram_dispatch(c->port)==PT_MIXED_ACTIVATION_FAILED);nr_drop(c);
}
static void nr_init_refusals(void)
{
    struct nr_case *c=nr_make(8,8,1);struct pt_private_mixed_ram_port *dirty=calloc(1,sizeof(*dirty)),*before=malloc(sizeof(*before));
    struct pt_private_mixed_ram_adapter a=nr_adapter(c),*alias;
    assert(dirty&&before);dirty->reader[31].live=1;memcpy(before,dirty,sizeof(*before));
    assert(!pt_private_mixed_ram_init(dirty,c->session,17,709379,&a,128,256,8)&&!memcmp(dirty,before,sizeof(*before)));
    memset(dirty,0,sizeof(*dirty));a.context=dirty;a.context_bytes=sizeof(*dirty);
    assert(!pt_private_mixed_ram_init(dirty,c->session,17,709379,&a,128,256,8)&&!dirty->initialized);
    a=nr_adapter(c);alias=(void *)dirty;memcpy(alias,&a,sizeof(a));memcpy(before,dirty,sizeof(*before));
    assert(!pt_private_mixed_ram_init(dirty,c->session,17,709379,alias,128,256,8)&&!memcmp(dirty,before,sizeof(*before)));
    free(before);free(dirty);nr_close(c);nr_drop(c);
}
static void nr_arm_cases(int raw,unsigned hook,unsigned order)
{
    struct nr_case *c=nr_make(24,16,1);uint64_t ticket=nr_enqueue(c,6,0,960,0);unsigned failed=raw!=0||hook;
    c->clock->arm_result=raw;c->clock->hook=hook?2U:0U;
    assert(pt_mixed_activation_publish(c->owner,ticket)==(failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_PENDING));
    assert(!c->port->effects&&c->trial->reader[0].pin&&c->trial->reader[5].pin);
    if(failed){struct pt_private_mixed_ram_command *command=nr_command(c,ticket);
        assert(command&&command->arm_outcome==raw&&command->uncertain&&pt_mixed_readers_commands_held(c->trial->queue)==1);
        nr_drain(c,ticket,6,order,1);
    }else{assert(!nr_command(c,ticket));assert(pt_mixed_activation_stop(c->owner)==PT_MIXED_READERS_OK);}
    nr_close(c);nr_drop(c);
}
static void nr_source_cases(int raw,unsigned hook)
{
    struct nr_case *c=nr_make(8,8,1);unsigned calls;
    c->clock->source_result=raw;c->clock->hook=hook?4U:0U;
    assert(!pt_mixed_activation_close(&c->owner)&&c->owner&&c->memory->live==1&&c->port->source_attempted&&!c->port->source_closed);
    assert(c->clock->shutdowns==1&&c->port->source_outcome==raw);calls=c->clock->shutdowns;
    c->clock->probe_result=0;assert(!pt_mixed_activation_close(&c->owner)&&c->clock->probes==1&&c->clock->shutdowns==calls);
    c->clock->probe_result=2;assert(!pt_mixed_activation_close(&c->owner)&&c->clock->probes==2&&c->clock->shutdowns==calls);
    /* Distinct external software observation now proves the original source
     * absent. Probe is read-only; no second shutdown or automatic retry. */
    c->clock->closed=1;c->clock->probe_result=1;nr_close(c);
    assert(c->clock->probes==3&&c->clock->shutdowns==calls);nr_drop(c);
}
static void nr_replacement_pressure(unsigned order)
{
    struct nr_case *c=nr_make(24,16,1);uint64_t first=nr_enqueue(c,16,0,960,0),second,third=0;unsigned i;
    nr_publish(c,first);nr_dispatch(c,first,0);assert(pt_mixed_activation_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    second=nr_enqueue(c,16,1,1920,0);assert(pt_mixed_activation_publish(c->owner,second)==PT_MIXED_READERS_OK);nr_dispatch(c,second,0);
    assert(nr_live_readers(c)==32&&pt_mixed_readers_readers_held(c->trial->queue)==32);
    trigger_input(c->trial,1,2,2880);
    assert(pt_mixed_activation_enqueue(c->owner,c->trial->input,&third)==PT_MIXED_READERS_CAPACITY&&!third);unused_drop(c->trial,1,2);
    for(i=0;i<16;++i){struct pt_mixed_readers_key key;nr_observe_active(c,second,i,1);assert(pt_mixed_activation_reader_key(c->owner,second,i,&key)==PT_MIXED_READERS_OK);
        assert(keys_equal(c->port->slot+i,&key)); /* Mixed 4/12 maps to indices0..15. */
        assert(pt_mixed_activation_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);
        assert(keys_equal(c->port->slot+i,&key));
    }
    assert(nr_live_readers(c)==16);nr_drain(c,second,16,order,0);nr_close(c);nr_drop(c);
}
static uint64_t nr_control_input(struct nr_case *c,uint64_t original,unsigned generation,uint64_t frame,unsigned count,unsigned stop)
{
    struct trial *t=c->trial;unsigned i;uint64_t ticket=0;
    memset(t->input,0,sizeof(*t->input));holder_init(t->command+generation,100+generation);t->command[generation].queue=t->queue;
    t->input->command=control_of(t->command+generation);t->input->batch=(struct pt_mixed_readers_batch){17,frame,count,{{0}}};
    for(i=0;i<count;++i){struct pt_mixed_readers_action *a=t->input->batch.action+i;
        assert(pt_mixed_activation_reader_key(c->owner,original,i,t->input->target+i)==PT_MIXED_READERS_OK);
        a->route=t->input->target[i].route;a->slot=t->input->target[i].slot;a->kind=stop?PT_MIXED_READERS_STOP:PT_MIXED_READERS_CONTROL;
        if(!stop){if(a->route==PT_MIXED_READERS_PAULA){a->geometry.paula.period=400;a->geometry.paula.volume=32;}
            else{a->geometry.amigus.rate=0x04000000;a->geometry.amigus.left=123;a->geometry.amigus.right=456;}}
    }
    assert(pt_mixed_activation_enqueue(c->owner,t->input,&ticket)==PT_MIXED_READERS_OK);return ticket;
}
static void nr_control_stop(void)
{
    struct nr_case *c=nr_make(24,16,1);uint64_t original=nr_enqueue(c,6,0,960,0),control,stop;unsigned i;
    nr_publish(c,original);nr_dispatch(c,original,0);assert(pt_mixed_activation_service_command(c->owner,original,0,NULL)==PT_MIXED_READERS_OK);
    control=nr_control_input(c,original,1,1920,6,0);assert(pt_mixed_activation_publish(c->owner,control)==PT_MIXED_READERS_OK);nr_dispatch(c,control,0);
    assert(pt_mixed_activation_service_command(c->owner,control,0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<6;++i){struct pt_private_mixed_ram_reader *r=c->port->reader+i;assert(r->live&&r->active);
        if(i<4){assert(r->action.geometry.paula.data&&r->action.geometry.paula.words==32&&r->action.geometry.paula.period==400&&r->action.geometry.paula.volume==32);}
        else{assert(r->card.reservation&&r->card.cache&&r->card.logical_bytes==128&&r->action.geometry.amigus.start<r->action.geometry.amigus.end_exclusive&&r->action.geometry.amigus.rate==0x04000000&&r->action.geometry.amigus.left==123&&r->action.geometry.amigus.right==456);}}
    stop=nr_control_input(c,original,2,2880,6,1);assert(pt_mixed_activation_publish(c->owner,stop)==PT_MIXED_READERS_OK);nr_dispatch(c,stop,0);
    assert(pt_mixed_activation_service_command(c->owner,stop,0,NULL)==PT_MIXED_READERS_OK);
    assert(!c->port->mask&&nr_live_readers(c)==6);nr_unchanged(c);
    for(i=0;i<6;++i){assert(!c->port->reader[i].active&&c->trial->reader[i].pin);
        assert(pt_mixed_activation_service_reader(c->owner,original,i,1,NULL)==PT_MIXED_READERS_OK);}
    assert(!nr_live_readers(c));nr_close(c);nr_drop(c);
}
static void nr_second_armed_refusal(void)
{
    struct nr_case *c=nr_make(16,8,1);uint64_t original=nr_enqueue(c,6,0,960,0),first,second;unsigned i,arms;
    struct pt_private_mixed_ram_command *before=malloc(sizeof(c->port->command));
    assert(before);nr_publish(c,original);nr_dispatch(c,original,0);
    assert(pt_mixed_activation_service_command(c->owner,original,0,NULL)==PT_MIXED_READERS_OK);
    first=nr_control_input(c,original,1,1920,1,0);assert(pt_mixed_activation_publish(c->owner,first)==PT_MIXED_READERS_OK);
    second=nr_control_input(c,original,2,2880,1,0);memcpy(before,c->port->command,sizeof(c->port->command));arms=c->clock->arms;
    assert(pt_mixed_activation_publish(c->owner,second)==PT_MIXED_READERS_PENDING);
    assert(!memcmp(before,c->port->command,sizeof(c->port->command))&&c->clock->arms==arms&&c->clock->active_ticket==first&&!nr_command(c,second)&&nr_live_readers(c)==6);
    nr_dispatch(c,first,0);assert(c->port->ledger_result==PT_MIXED_ACTIVATION_COMMITTED);
    assert(pt_mixed_activation_stop(c->owner)==PT_MIXED_READERS_PENDING&&c->trial->command[2].releases==1);
    assert(pt_mixed_activation_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<6;++i)assert(pt_mixed_activation_service_reader(c->owner,original,i,1,NULL)==PT_MIXED_READERS_OK);
    free(before);nr_close(c);nr_drop(c);
}
static void nr_quiet_outcomes(unsigned domain,unsigned mode)
{
    struct nr_case *c=nr_make(24,16,1);uint64_t ticket=nr_enqueue(c,6,0,960,0);struct pt_private_mixed_ram_command *command;unsigned failed=mode!=0;
    enum pt_mixed_readers_result expected=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_PENDING;
    nr_publish(c,ticket);nr_dispatch(c,ticket,0);command=nr_command(c,ticket);assert(command);
    c->clock->ticket_result=mode==0?0:mode==1?2:1;if(mode==2)c->clock->hook=3;
    if(!domain)assert(pt_mixed_activation_service_command(c->owner,ticket,1,NULL)==expected);
    else assert(pt_mixed_activation_service_reader(c->owner,ticket,0,1,NULL)==expected);
    assert(nr_command(c,ticket)==command&&command->ticket_outcome==(mode==0?0:mode==1?2:1)&&nr_live_readers(c)==6&&c->trial->reader[0].pin&&c->trial->reader[5].pin);
    c->clock->ticket_result=1;nr_drain(c,ticket,6,0,failed);nr_close(c);nr_drop(c);
}
static void nr_expected_only(unsigned uncertain_second)
{
    struct nr_case *c=nr_make(24,16,1);uint64_t first=nr_enqueue(c,16,0,960,1),second;unsigned i,calls;
    struct pt_mixed_readers_key key;struct pt_private_mixed_ram_command *pending;
    enum pt_mixed_readers_result result=uncertain_second?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
    nr_publish(c,first);nr_dispatch(c,first,0);
    /* Keep first command retained: both records depend on original slot19. */
    second=nr_enqueue(c,1,1,1920,0);assert(pt_mixed_activation_publish(c->owner,second)==PT_MIXED_READERS_OK);pending=nr_command(c,second);assert(pending);
    nr_observe_active(c,first,15,0);
    assert(pt_mixed_activation_reader_key(c->owner,first,15,&key)==PT_MIXED_READERS_OK);
    assert(key.route==PT_MIXED_READERS_AMIGUS&&key.slot==15&&!keys_equal(pending->packet.key,&key));
    calls=c->clock->tickets;
    if(uncertain_second)c->clock->fail_ticket_call=calls+2;
    /* Genuine core supplies the full original reader/domain/binding identity;
     * no forged domain, fabricated private certificate or direct port probe. */
    assert(pt_mixed_activation_service_reader(c->owner,first,15,1,NULL)==result);
    assert(c->clock->tickets==calls+2);
    if(uncertain_second){assert(nr_live_readers(c)==16&&(c->port->mask&(1U<<19)));c->clock->fail_ticket_call=0;
        assert(pt_mixed_activation_service_reader(c->owner,first,15,1,NULL)==PT_MIXED_READERS_BACKEND);}
    assert(!(c->port->mask&(1U<<19))&&nr_live_readers(c)==15);
    assert(pending->disabled&&pending->finished&&!pending->armed&&c->port->armed_index==-1);
    for(i=0;i<16;++i){struct pt_mixed_readers_action a;struct pt_mixed_readers_card card;
        memset(&a,0,sizeof(a));memset(&card,0,sizeof(card));assert(!memcmp(&pending->packet.action[i].geometry,&a.geometry,sizeof(a.geometry))&&!memcmp(pending->packet.card+i,&card,sizeof(card)));}
    assert(c->trial->reader[15].pin); /* Original command still owns its borrow. */
    assert(pt_mixed_activation_service_command(c->owner,first,1,NULL)==result);
    assert(!c->trial->reader[15].pin);
    for(i=0;i<15;++i)assert(pt_mixed_activation_service_reader(c->owner,first,i,1,NULL)==result);
    nr_drain(c,second,1,1,uncertain_second);nr_close(c);nr_drop(c);
}
static void nr_registry_refusal(unsigned mutation)
{
    struct nr_case *c=nr_make(24,16,1);uint64_t ticket=nr_enqueue(c,6,0,960,0);struct pt_private_mixed_ram_command *command;uint64_t ticks[3];
    nr_publish(c,ticket);command=nr_command(c,ticket);assert(command);
    switch(mutation){
    case 0:c->port->slot[19].serial=999;break;
    case 1:command->packet.registration.session++;break;
    case 2:command->packet.key[5].serial++;break;
    case 3:command->packet.card[5].full_capacity=0;break;
    case 4:command->packet.action[0].geometry.paula.data=NULL;break;
    case 5:command->packet.action[5].geometry.amigus.end_exclusive++;break;
    case 6:command->packet.action[5].slot=0;break;
    }
    ticks[0]=ticks[1]=ticks[2]=oracle(960);nr_script(c,ticks,3);
    assert(pt_private_mixed_ram_dispatch(c->port)==PT_MIXED_ACTIVATION_FAILED&&!c->port->effects&&!nr_live_readers(c));
    /* Restore only fixture corruption of copied identity/value storage before
     * exact core cancellation; no sample/cache/owner bytes were changed. */
    memset(c->port->slot,0,sizeof(c->port->slot));command->packet.registration=c->clock->registration;
    nr_drain(c,ticket,6,0,c->port->fires!=0);nr_close(c);nr_drop(c);
}
static void nr_clock_and_late(unsigned mode)
{
    struct nr_case *c=nr_make(24,16,1);uint64_t ticket=nr_enqueue(c,6,0,960,0),ticks[4];struct pt_private_mixed_ram_command *command;
    nr_publish(c,ticket);command=nr_command(c,ticket);assert(command);
    ticks[0]=mode==0?command->packet.last:command->packet.first;ticks[1]=ticks[2]=command->packet.first;ticks[3]=command->packet.last;
    nr_script(c,ticks,mode==3?4U:3U);
    if(mode==1)c->clock->script_frequency[0]=715909;
    if(mode==2)c->clock->hook=1;
    if(mode==3){ticks[0]=command->arm_tick;ticks[1]=command->packet.first;ticks[2]=command->packet.first;ticks[3]=command->packet.last;nr_script(c,ticks,4);}
    assert(pt_private_mixed_ram_dispatch(c->port)==PT_MIXED_ACTIVATION_FAILED);
    assert(c->port->effects==(mode==3?6U:0U));
    if(mode==3)assert(c->port->ledger_result==PT_MIXED_ACTIVATION_FAILED&&nr_live_readers(c)==6);
    else assert(!nr_live_readers(c));
    c->clock->length=c->clock->index=0;c->clock->now=oracle(960);
    nr_drain(c,ticket,6,1,c->port->fires!=0);nr_close(c);nr_drop(c);
}
#define PT_NATIVE_MIXED_RAM_PORT_TEST_VERSION 1U
#ifndef PT_NATIVE_MIXED_RAM_PORT_TEST_ENTRY
#define PT_NATIVE_MIXED_RAM_PORT_TEST_ENTRY main
#endif
int PT_NATIVE_MIXED_RAM_PORT_TEST_ENTRY(void)
{
    unsigned bits,cache_bits,order,mode;
    for(bits=8;bits<=24;bits+=8)for(cache_bits=8;cache_bits<=16;cache_bits+=8)for(order=0;order<2;++order){nr_success(bits,cache_bits,order,0);nr_success(bits,cache_bits,order,1);}
    nr_binding_and_empty(0);nr_binding_and_empty(1);nr_unbound_cleanup();nr_init_refusals();
    for(order=0;order<2;++order){nr_arm_cases(0,0,order);nr_arm_cases(-1,0,order);nr_arm_cases(2,0,order);nr_arm_cases(0,1,order);nr_arm_cases(1,1,order);nr_replacement_pressure(order);}
    nr_source_cases(0,0);nr_source_cases(-1,0);nr_source_cases(2,0);nr_source_cases(1,1);
    nr_expected_only(0);nr_expected_only(1);nr_control_stop();nr_second_armed_refusal();
    for(order=0;order<2;++order)for(mode=0;mode<3;++mode)nr_quiet_outcomes(order,mode);
    for(mode=0;mode<7;++mode)nr_registry_refusal(mode);
    for(mode=0;mode<4;++mode)nr_clock_and_late(mode);
    assert(nr_cases==65);
    puts("NATIVE MIXED RAM PORT PASS:65 genuine heap cases;24 mixed4/12 or16-card8/16/24-master/cache proof-order lifetimes;32-reader replacement pressure;all20 original keys;expected-only dependent command records including second-proof uncertainty;second armed publication refusal;paired CONTROL/STOP;raw arm/source outcomes and reentry;once shutdown then independent read-only quiet;exact clocks/retained late adoption/master beforeimages;SOFTWARE_ONLY");
    return 0;
}
