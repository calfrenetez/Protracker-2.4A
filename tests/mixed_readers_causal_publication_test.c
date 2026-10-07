/* SOURCE proposal: genuine resources and actual adopted production units.
 * The committed 26-case entry is renamed and deliberately never called.
 * Only callbacks below inject publication/allocator faults; no core mirror. */
#define PT_MIXED_CAUSAL_TEST_MAIN pf_committed_26_entry_not_called
#include "mixed_readers_causal_test.c"
#undef PT_MIXED_CAUSAL_TEST_MAIN

enum pf_mode {
    PF_CLEAN_ZERO,PF_UNKNOWN,PF_MALFORMED,PF_IDENTITY,PF_PACKET,
    PF_REENTRY,PF_FAULT,PF_ZERO_IDENTITY,PF_ZERO_PACKET,
    PF_ZERO_REENTRY,PF_ZERO_FAULT,PF_LATE,PF_MODES
};
struct pf_port {
    struct ct_port ram; /* Existing ordinary-memory port helpers only. */
    struct pt_mixed_causal_owner *bound_owner;
    unsigned bound,scope,mode,first_calls,successor_calls,hooks,pending;
    int last_raw;
};
struct pf_memory {
    void *block[2];unsigned calls,live,releases,order[2];
    unsigned fail_at,reenter_at,fault_at,queue_reentry,owner_reentry,hooks;
    const struct pt_mixed_causal_config *config;
    void *workspace;size_t capacity;
    struct pt_mixed_causal_owner **output;
};
struct pf_case {
    struct trial *trial;struct pf_port port;struct pf_memory memory;
    struct pt_mixed_causal_config config;struct pt_mixed_causal_owner *owner;
    void *workspace;uint8_t *before;size_t before_bytes;
};
struct pf_holders {
    struct holder reader[64],command[4];
};
static unsigned pf_cases;

static int pf_raw(unsigned mode)
{
    if(mode==PF_CLEAN_ZERO||mode==PF_ZERO_IDENTITY||mode==PF_ZERO_PACKET||
       mode==PF_ZERO_REENTRY||mode==PF_ZERO_FAULT)return 0;
    if(mode==PF_UNKNOWN)return -1;
    if(mode==PF_MALFORMED)return 2;
    return 1;
}
static void pf_fault_hook(struct pf_port *p,struct pt_mixed_causal_owner *owner,
    uint64_t ticket)
{
    assert(p->bound&&owner==p->bound_owner&&p->ram.registration.owner==owner);
    if(p->mode==PF_REENTRY||p->mode==PF_ZERO_REENTRY){
        ++p->hooks;
        assert(pt_mixed_causal_fire(owner,ticket)==PT_MIXED_CAUSAL_INVALID);
    }else if(p->mode==PF_FAULT||p->mode==PF_ZERO_FAULT){
        ++p->hooks;pt_mixed_causal_fail_closed(owner);
    }
}
static int pf_publish(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_command_identity *identity,
    const struct pt_mixed_causal_packet *packet)
{
    struct pf_port *p=context;int raw;
    assert(p->bound&&owner==p->bound_owner&&ct_registration(&p->ram.registration,&packet->registration));
    ++p->first_calls;
    if(p->scope!=1)return ct_publish(&p->ram,owner,identity,packet);
    raw=pf_raw(p->mode);p->last_raw=raw;
    if(raw)assert(ct_publish(&p->ram,owner,identity,packet)==1);
    else ++p->ram.publications; /* Actual clean/raw0 model acquired no refs. */
    if(p->mode==PF_IDENTITY||p->mode==PF_ZERO_IDENTITY){
        ++p->hooks;((struct pt_mixed_causal_command_identity *)(void *)identity)->owner++;
    }
    if(p->mode==PF_PACKET||p->mode==PF_ZERO_PACKET){
        struct pt_mixed_causal_packet *mutable_packet=(void *)packet;
        ++p->hooks;--mutable_packet->count;mutable_packet->expected[19].serial++;
    }
    pf_fault_hook(p,owner,packet->ticket);
    if(p->mode==PF_LATE)p->ram.ticks=packet->first;
    return raw;
}
static int pf_publish_successor(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_publication *publication)
{
    struct pf_port *p=context;int raw;
    assert(p->bound&&owner==p->bound_owner&&ct_registration(&p->ram.registration,&publication->successor.registration));
    ++p->successor_calls;
    if(p->scope!=2)return ct_publish_successor(&p->ram,owner,publication);
    raw=pf_raw(p->mode);p->last_raw=raw;
    if(raw)assert(ct_publish_successor(&p->ram,owner,publication)==1);
    else ++p->ram.publications;
    if(p->mode==PF_IDENTITY||p->mode==PF_ZERO_IDENTITY){
        struct pt_mixed_causal_publication *mutable_publication=(void *)publication;
        ++p->hooks;mutable_publication->predecessor.owner++;
        mutable_publication->successor_identity.ticket++;
    }
    if(p->mode==PF_PACKET||p->mode==PF_ZERO_PACKET){
        struct pt_mixed_causal_publication *mutable_publication=(void *)publication;
        ++p->hooks;--mutable_publication->successor.count;
        mutable_publication->first.expected[19].serial++;
        memset(&mutable_publication->successor.action[15].geometry,0,
            sizeof(mutable_publication->successor.action[15].geometry));
    }
    pf_fault_hook(p,owner,publication->successor.ticket);
    if(p->mode==PF_LATE)p->ram.ticks=publication->successor.first;
    return raw;
}
static int pf_command_quiet(void *context,
    const struct pt_mixed_causal_command_identity *identity,unsigned cancel)
{
    struct pf_port *p=context;
    assert(p->bound&&ct_registration(&p->ram.registration,&identity->registration));
    if(p->pending){++p->ram.command_proofs;return 0;}
    return ct_command_quiet(&p->ram,identity,cancel);
}
static int pf_reader_quiet(void *context,
    const struct pt_mixed_causal_reader_identity *identity,unsigned cancel)
{
    struct pf_port *p=context;
    assert(p->bound&&ct_registration(&p->ram.registration,&identity->registration));
    if(p->pending){++p->ram.reader_proofs;return 0;}
    return ct_reader_quiet(&p->ram,identity,cancel);
}
static void *pf_allocate(void *context,size_t bytes)
{
    struct pf_memory *m=context;unsigned index;void *p;
    ++m->calls;
    if(m->reenter_at==m->calls){++m->hooks;
        assert(pt_mixed_causal_open(m->config,m->workspace,m->capacity,m->output)==PT_MIXED_READERS_BACKEND);}
    if(m->fault_at==m->calls){
        assert(m->calls==2&&m->block[0]);++m->hooks;
        /* First exact allocation is initialized as the genuine owner before
         * production asks for the second queue allocation. No opaque fields. */
        pt_mixed_causal_fail_closed(m->block[0]);
    }
    if(m->fail_at==m->calls)return NULL;
    p=malloc(bytes);assert(p);
    for(index=0;index<2;++index)if(!m->block[index])break;
    assert(index<2);m->block[index]=p;++m->live;return p;
}
static void pf_release(void *context,void *pointer)
{
    struct pf_memory *m=context;unsigned index;
    for(index=0;index<2;++index)if(m->block[index]==pointer)break;
    assert(index<2&&m->live&&m->releases<2);
    m->order[m->releases++]=index;
    if(index==1&&m->queue_reentry){m->queue_reentry=0;++m->hooks;
        assert(pt_mixed_readers_stop(pointer)==PT_MIXED_READERS_BACKEND);}
    if(index==0&&m->owner_reentry){m->owner_reentry=0;++m->hooks;
        assert(pt_mixed_causal_stop(pointer)==PT_MIXED_READERS_BACKEND);}
    m->block[index]=NULL;--m->live;free(pointer);
}
static struct pf_case *pf_make(void)
{
    struct pf_case *c=calloc(1,sizeof(*c));assert(c);++pf_cases;
    c->trial=trial_make(24,16);c->before=save(c->trial->resources,&c->before_bytes);
    c->port.ram.ticks=100;c->port.ram.frequency=709379;
    c->port.ram.commit_raw=c->port.ram.source_raw=1;
    c->config.allocator=(struct pt_allocator){&c->memory,pf_allocate,pf_release};
    c->config.allocator_context=(struct pt_mixed_readers_span){&c->memory,sizeof(c->memory)};
    c->config.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};c->config.session=19;
    c->config.control_budget=pt_mixed_causal_control_size();c->config.queue_budget=pt_mixed_readers_control_size();
    c->config.port=(struct pt_mixed_causal_port){&c->port,sizeof(c->port),1,31,
        ct_clock,pf_publish,ct_commit,pf_command_quiet,pf_reader_quiet,
        ct_source_close,ct_source_quiet,pf_publish_successor};
    c->workspace=calloc(1,pt_mixed_causal_workspace_size());assert(c->workspace);
    c->memory.config=&c->config;c->memory.workspace=c->workspace;
    c->memory.capacity=pt_mixed_causal_workspace_size();c->memory.output=&c->owner;
    return c;
}
static void pf_open(struct pf_case *c)
{
    assert(pt_mixed_causal_open(&c->config,c->workspace,c->memory.capacity,&c->owner)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_borrow_queue(c->owner,&c->trial->queue)==PT_MIXED_READERS_OK&&c->memory.live==2);
    /* Original opaque registration and mutable callback owner bind immediately,
     * before factory construction, validation, admission or publication. */
    c->port.ram.registration=(struct pt_mixed_causal_registration){c->owner,c->trial->queue,19,17};
    c->port.bound_owner=c->owner;c->port.bound=1;
}
static void pf_destroy(struct pf_case *c)
{
    assert(!c->owner&&!c->memory.live);
    c->trial->queue=NULL;same_save(c->trial->resources,c->before,c->before_bytes);
    trial_drop(c->trial);free(c->before);free(c->workspace);free(c);
}
static uint64_t pf_enqueue(struct pf_case *c,uint64_t predecessor,unsigned generation)
{
    uint64_t ticket=0;
    trigger_input(c->trial,16,generation,generation?1920:960);
    if(generation)assert(pt_mixed_causal_enqueue_successor(c->owner,predecessor,c->trial->input,&ticket)==PT_MIXED_READERS_OK);
    else assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&ticket)==PT_MIXED_READERS_OK);
    return ticket;
}
static void pf_holders_copy(struct pf_case *c,struct pf_holders *snapshot)
{
    memcpy(snapshot->reader,c->trial->reader,sizeof(snapshot->reader));
    memcpy(snapshot->command,c->trial->command,sizeof(snapshot->command));
}
static void pf_holders_unchanged(struct pf_case *c,struct pf_holders *snapshot)
{
    unsigned i;
    /* Actual publish revalidates current holders; only these counters may grow. */
    for(i=0;i<64;++i){assert(c->trial->reader[i].currents>=snapshot->reader[i].currents);
        snapshot->reader[i].currents=c->trial->reader[i].currents;}
    for(i=0;i<4;++i){assert(c->trial->command[i].currents>=snapshot->command[i].currents);
        snapshot->command[i].currents=c->trial->command[i].currents;}
    assert(!memcmp(snapshot->reader,c->trial->reader,sizeof(snapshot->reader)));
    assert(!memcmp(snapshot->command,c->trial->command,sizeof(snapshot->command)));
}
static void pf_drain(struct pf_case *c,uint64_t first,uint64_t second,unsigned order,unsigned failed)
{
    enum pt_mixed_readers_result expected=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
    unsigned generation,i,n=second?2:1;
    if(!order){
        assert(pt_mixed_causal_service_command(c->owner,first,1,NULL)==expected);
        if(second)assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==expected);
        assert(pt_mixed_readers_commands_held(c->trial->queue)==0&&pt_mixed_readers_readers_held(c->trial->queue)==16*n);
        for(generation=0;generation<n;++generation)for(i=0;i<16;++i)
            assert(c->trial->reader[generation*16+i].live&&!c->trial->reader[generation*16+i].releases);
    }
    for(generation=0;generation<n;++generation)for(i=0;i<16;++i)
        assert(pt_mixed_causal_service_reader(c->owner,generation?second:first,i,1,NULL)==expected);
    if(order){
        assert(pt_mixed_readers_commands_held(c->trial->queue)==n&&pt_mixed_readers_readers_held(c->trial->queue)==16*n);
        assert(pt_mixed_causal_service_command(c->owner,first,1,NULL)==expected);
        if(second)assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==expected);
    }
    assert(!pt_mixed_readers_commands_held(c->trial->queue)&&!pt_mixed_readers_readers_held(c->trial->queue));
    for(generation=0;generation<n;++generation){
        assert(c->trial->command[generation].releases==1);
        for(i=0;i<16;++i)assert(c->trial->reader[generation*16+i].releases==1&&!c->trial->reader[generation*16+i].pin);
    }
    assert(!c->port.ram.commits&&!c->port.ram.effects&&ct_empty(&c->port.ram));
}
static void pf_publication(unsigned scope,unsigned mode,unsigned order)
{
    struct pf_case *c=pf_make();struct pf_holders *snapshot=malloc(sizeof(*snapshot));
    struct pt_mixed_causal_diagnostic diagnostic;
    struct pt_mixed_readers_key key,sentinel;
    struct pt_mixed_readers_command_receipt command,before_command;
    struct pt_mixed_readers_reader_receipt reader,before_reader;
    uint64_t first,second=0,target;unsigned held=scope==2?32:16,callbacks;
    assert(snapshot);pf_open(c);first=pf_enqueue(c,0,0);
    if(scope==2){assert(pt_mixed_causal_publish(c->owner,first)==PT_MIXED_READERS_OK);second=pf_enqueue(c,first,1);}
    target=scope==2?second:first;c->port.scope=scope;c->port.mode=mode;
    pf_holders_copy(c,snapshot);
    assert((scope==2?pt_mixed_causal_publish_successor(c->owner,first,second):pt_mixed_causal_publish(c->owner,first))==
        (mode==PF_CLEAN_ZERO?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_BACKEND));
    assert(c->port.last_raw==pf_raw(mode)&&c->port.first_calls==1&&c->port.successor_calls==(scope==2));
    assert(c->port.hooks==((mode==PF_IDENTITY||mode==PF_PACKET||mode==PF_REENTRY||mode==PF_FAULT||
        mode==PF_ZERO_IDENTITY||mode==PF_ZERO_PACKET||mode==PF_ZERO_REENTRY||mode==PF_ZERO_FAULT)?1U:0U));
    assert(pt_mixed_readers_commands_held(c->trial->queue)==(scope==2?2U:1U)&&
        pt_mixed_readers_readers_held(c->trial->queue)==held&&c->memory.live==2);
    pf_holders_unchanged(c,snapshot);same_save(c->trial->resources,c->before,c->before_bytes);
    assert(!c->port.ram.commits&&!c->port.ram.effects&&!c->port.ram.shutdowns);
    assert(!pt_mixed_causal_close(&c->owner)&&c->owner&&c->memory.live==2&&!c->port.ram.shutdowns);
    if(mode==PF_CLEAN_ZERO){
        assert(pt_mixed_causal_diagnostic(c->owner,&diagnostic)&&!diagnostic.completed&&!diagnostic.suppressed&&!diagnostic.published);
        assert(ct_command(&c->port.ram,target)==NULL);
        /* Only this explicit test action retries a positively clean refusal.
         * There is no automatic producer/port retry or deadline rebasing. */
        c->port.scope=0;
        assert((scope==2?pt_mixed_causal_publish_successor(c->owner,first,second):pt_mixed_causal_publish(c->owner,first))==PT_MIXED_READERS_OK);
        pf_drain(c,first,second,order,0);
    }else{
        memset(&sentinel,0xa5,sizeof(sentinel));memcpy(&key,&sentinel,sizeof(key));
        assert(pt_mixed_causal_reader_key(c->owner,target,0,&key)!=PT_MIXED_READERS_OK&&!memcmp(&key,&sentinel,sizeof(key)));
        callbacks=c->port.first_calls+c->port.successor_calls;
        c->port.ram.ticks=oracle(1920);
        assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_FAILED);
        if(second)assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_FAILED);
        assert(!c->port.ram.commits&&!c->port.ram.effects&&callbacks==c->port.first_calls+c->port.successor_calls);
        pf_holders_unchanged(c,snapshot);
        c->port.pending=1;
        memset(&before_command,0xa5,sizeof(before_command));memcpy(&command,&before_command,sizeof(command));
        memset(&before_reader,0xa5,sizeof(before_reader));memcpy(&reader,&before_reader,sizeof(reader));
        assert(pt_mixed_causal_service_command(c->owner,target,1,&command)==PT_MIXED_READERS_BACKEND&&
            !memcmp(&command,&before_command,sizeof(command)));
        assert(pt_mixed_causal_service_reader(c->owner,target,0,1,&reader)==PT_MIXED_READERS_BACKEND&&
            !memcmp(&reader,&before_reader,sizeof(reader)));
        assert(pt_mixed_readers_commands_held(c->trial->queue)==(scope==2?2U:1U)&&
            pt_mixed_readers_readers_held(c->trial->queue)==held);
        pf_holders_unchanged(c,snapshot);same_save(c->trial->resources,c->before,c->before_bytes);
        assert(!pt_mixed_causal_close(&c->owner)&&c->memory.live==2&&!c->port.ram.shutdowns);
        c->port.pending=0;pf_drain(c,first,second,order,1);
    }
    assert(pt_mixed_causal_close(&c->owner)&&!c->owner&&!c->memory.live&&c->port.ram.shutdowns==1);
    assert(c->memory.releases==2&&c->memory.order[0]==1&&c->memory.order[1]==0);
    assert(pt_mixed_causal_close(&c->owner));free(snapshot);pf_destroy(c);
}
static void pf_constructor(unsigned mode)
{
    struct pf_case *c=pf_make();struct pt_mixed_causal_owner *sentinel=(void *)(uintptr_t)1;
    enum pt_mixed_readers_result expected;unsigned calls,releases;
    c->owner=sentinel;
    if(mode<2){c->memory.fail_at=mode+1;expected=PT_MIXED_READERS_CAPACITY;calls=mode+1;releases=mode;}
    else{expected=PT_MIXED_READERS_BACKEND;calls=mode==2?1:2;releases=calls;
        if(mode==4)c->memory.fault_at=2;
        else c->memory.reenter_at=mode==2?1:2;
        if(mode==5)c->memory.queue_reentry=1;}
    assert(pt_mixed_causal_open(&c->config,c->workspace,c->memory.capacity,&c->owner)==expected&&c->owner==sentinel);
    assert(c->memory.calls==calls&&c->memory.releases==releases&&!c->memory.live);
    if(releases==2)assert(c->memory.order[0]==1&&c->memory.order[1]==0);
    assert(!c->port.bound&&!c->port.ram.clocks&&!c->port.first_calls&&!c->port.successor_calls&&
        !c->port.ram.shutdowns&&!c->port.ram.probes&&!c->trial->queue);
    assert(c->memory.hooks==(mode<2?0U:mode==5?2U:1U));
    same_save(c->trial->resources,c->before,c->before_bytes);
    c->owner=NULL;pf_destroy(c);
}
static void pf_consumed_release(unsigned owner_release)
{
    struct pf_case *c=pf_make();struct pt_mixed_readers_output *probe=(void *)(uintptr_t)1;
    pf_open(c);
    if(owner_release){
        c->memory.owner_reentry=1;
        assert(!pt_mixed_causal_close(&c->owner)&&!c->owner&&!c->memory.live&&c->memory.hooks==1&&c->port.ram.shutdowns==1);
        assert(pt_mixed_causal_close(&c->owner));
    }else{
        c->memory.queue_reentry=1;
        assert(!pt_mixed_causal_close(&c->owner)&&c->owner&&c->memory.live==1&&
            !c->memory.block[1]&&c->memory.hooks==1&&!c->port.ram.shutdowns);
        assert(pt_mixed_causal_borrow_queue(c->owner,&probe)==PT_MIXED_READERS_BACKEND&&probe==(void *)(uintptr_t)1);
        c->trial->queue=NULL; /* Actual queue allocator consumption was observed. */
        assert(pt_mixed_causal_close(&c->owner)&&!c->owner&&!c->memory.live&&c->port.ram.shutdowns==1);
        assert(pt_mixed_causal_close(&c->owner));
    }
    assert(c->memory.releases==2&&c->memory.order[0]==1&&c->memory.order[1]==0);
    assert(!c->port.ram.clocks&&!c->port.first_calls&&!c->port.successor_calls&&
        !c->port.ram.commits&&!c->port.ram.effects&&!c->port.ram.probes);
    pf_destroy(c);
}
#ifndef PT_MIXED_CAUSAL_PUBLICATION_TEST_MAIN
#define PT_MIXED_CAUSAL_PUBLICATION_TEST_MAIN main
#endif
int PT_MIXED_CAUSAL_PUBLICATION_TEST_MAIN(void)
{
    unsigned scope,mode,order;
    for(scope=1;scope<=2;++scope)for(mode=0;mode<PF_MODES;++mode)for(order=0;order<2;++order)
        pf_publication(scope,mode,order);
    for(mode=0;mode<6;++mode)pf_constructor(mode);
    pf_consumed_release(0);pf_consumed_release(1);assert(pf_cases==56);
    puts("MIXED CAUSAL PUBLICATION PASS:56 genuine heap cases;48 first/successor raw0/-1/malformed identity/full-copy/reentry/fault/late publication paths;independent C/R proof orders;faulting raw0 retains pins and unchanged outputs until exact later proofs;6 real allocation/reentry/fail-closed constructors;2 actual consumed-close0 release lifetimes;bound original registration;master beforeimages;SOFTWARE_ONLY");
    return 0;
}
