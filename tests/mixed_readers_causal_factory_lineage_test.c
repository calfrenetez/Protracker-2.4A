/* Prospective SOURCE fixture. Production owner/queue compile separately.
 * The unchanged lineage/resource support main is defined but never invoked.
 * All completion and detach facts below come from actual owner/queue calls. */
#include "mixed_readers_causal_factory_lineage_fixture_support.inc"
#include "../src/core/mixed_readers_causal_factory_internal.h"

struct fq_snapshot {
    struct ct_memory memory;struct tl_port port;
    struct holder command[4],reader[64];
    unsigned commands,readers,writes,chips;
};
static struct fq_snapshot *fq_before(struct tl_case *c)
{
    struct fq_snapshot *s=calloc(1,sizeof(*s));assert(s);
    memcpy(&s->memory,&c->memory,sizeof(s->memory));memcpy(&s->port,&c->port,sizeof(s->port));
    memcpy(s->command,c->trial->command,sizeof(s->command));memcpy(s->reader,c->trial->reader,sizeof(s->reader));
    s->commands=pt_mixed_readers_commands_held(c->trial->queue);
    s->readers=pt_mixed_readers_readers_held(c->trial->queue);
    s->writes=c->trial->resources->card.writes;s->chips=c->trial->resources->chips;return s;
}
static void fq_unchanged(struct tl_case *c,struct fq_snapshot *s)
{
    assert(!memcmp(&s->memory,&c->memory,sizeof(s->memory))&&!memcmp(&s->port,&c->port,sizeof(s->port)));
    assert(!memcmp(s->command,c->trial->command,sizeof(s->command))&&!memcmp(s->reader,c->trial->reader,sizeof(s->reader)));
    assert(s->commands==pt_mixed_readers_commands_held(c->trial->queue)&&
        s->readers==pt_mixed_readers_readers_held(c->trial->queue)&&
        s->writes==c->trial->resources->card.writes&&s->chips==c->trial->resources->chips);
    same_save(c->trial->resources,c->before,c->before_bytes);free(s);
}
static void fq_state(struct tl_case *c,uint64_t first,uint64_t control,
    int empty,int first_current,int control_current)
{
    struct fq_snapshot *s=fq_before(c);
    assert(pt_mixed_causal_factory_control_stop_original_empty(c->owner,c->trial->queue,19,17)==empty);
    assert(pt_mixed_causal_factory_control_stop_first_current(c->owner,first)==first_current);
    assert(pt_mixed_causal_factory_control_stop_control_current(c->owner,control)==control_current);
    fq_unchanged(c,s);
}
static void fq_sequence(unsigned bits,unsigned cache_bits)
{
    struct tl_case *c=tl_make(bits,cache_bits,1);uint64_t first=0,control=0,third=0;
    struct pt_mixed_causal_control_request cr;struct pt_mixed_causal_stop_request sr;
    struct fq_snapshot *s;unsigned i;
    fq_state(c,0,0,1,0,0);trigger_input(c->trial,16,0,960);
    assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&first)==PT_MIXED_READERS_OK);
    fq_state(c,first,0,0,0,0);
    assert(pt_mixed_causal_publish(c->owner,first)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(960)-1;
    assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_EARLY);fq_state(c,first,0,0,0,0);
    ++c->port.original.base.ticks;assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED);
    fq_state(c,first,0,0,1,0);
    s=fq_before(c);
    assert(!pt_mixed_causal_factory_control_stop_first_current(c->owner,first+1));
    assert(!pt_mixed_causal_factory_control_stop_control_current(c->owner,first));fq_unchanged(c,s);
    c->port.original.hold_first=1;
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_PENDING);
    fq_state(c,first,0,0,1,0);cr=tl_control_request(c,first,6);
    assert(pt_mixed_causal_control_enqueue(c->owner,&cr,&control)==PT_MIXED_READERS_OK);
    fq_state(c,first,control,0,0,0);
    assert(pt_mixed_causal_control_publish(c->owner,control)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(1920)-1;
    assert(pt_mixed_causal_fire(c->owner,control)==PT_MIXED_CAUSAL_EARLY);fq_state(c,first,control,0,0,0);
    ++c->port.original.base.ticks;assert(pt_mixed_causal_fire(c->owner,control)==PT_MIXED_CAUSAL_COMMITTED);
    /* Actual CONTROL does not imply clean C1 detach. */
    fq_state(c,first,control,0,0,0);c->port.original.hold_first=0;
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    assert(!c->trial->command[0].live&&c->trial->command[0].releases==1);
    fq_state(c,first,control,0,0,1);
    s=fq_before(c);assert(!pt_mixed_causal_factory_control_stop_control_current(c->owner,first));
    assert(!pt_mixed_causal_factory_control_stop_control_current(c->owner,control+1));fq_unchanged(c,s);
    /* Original holder is genuinely disposed. The query must not reread it. */
    CT_POISON(c->trial->command,sizeof(c->trial->command[0]));
    assert(pt_mixed_causal_factory_control_stop_control_current(c->owner,control));
    CT_UNPOISON(c->trial->command,sizeof(c->trial->command[0]));
    sr=tl_stop_request(c,control,6);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&sr,&third)==PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==2&&
        pt_mixed_readers_readers_held(c->trial->queue)==16);
    fq_state(c,first,control,0,0,0);
    assert(pt_mixed_causal_stop_after_control_publish(c->owner,third)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(2880)-1;
    assert(pt_mixed_causal_fire(c->owner,third)==PT_MIXED_CAUSAL_EARLY);fq_state(c,first,control,0,0,0);
    ++c->port.original.base.ticks;assert(pt_mixed_causal_fire(c->owner,third)==PT_MIXED_CAUSAL_COMMITTED);
    fq_state(c,first,control,0,0,0);
    for(i=0;i<16;++i)assert(c->trial->reader[i].pin&&c->trial->reader[i].live);
    tl_drain(c,first,control,third,16,1,0,0);tl_drop(c,0);
}
static void fq_wrong_capability(unsigned mode)
{
    struct tl_case *c=tl_make(24,16,0);uint64_t first,control=0;
    struct pt_mixed_causal_control_port cp={&c->port,sizeof(c->port),1,3,cc_publish_control};
    struct pt_mixed_causal_stop_port sp={&c->port,sizeof(c->port),1,3,ss_publish_stop};
    if(mode==1)assert(pt_mixed_causal_control_bind(c->owner,&cp)==PT_MIXED_READERS_OK);
    if(mode==2)assert(pt_mixed_causal_stop_bind(c->owner,&sp)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_factory_original_empty(c->owner,c->trial->queue,19,17));
    fq_state(c,1,1,0,0,0);
    if(mode==0){
        uint64_t second=0;unsigned i;
        /* The default owner needs its actual published successor before C1
         * fires. Neither CONTROL nor STOP capability is installed here. */
        trigger_input(c->trial,6,0,960);first=0;
        assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&first)==PT_MIXED_READERS_OK);
        assert(pt_mixed_causal_publish(c->owner,first)==PT_MIXED_READERS_OK);
        trigger_input(c->trial,6,1,1920);
        assert(pt_mixed_causal_enqueue_successor(c->owner,first,c->trial->input,&second)==PT_MIXED_READERS_OK);
        assert(pt_mixed_causal_publish_successor(c->owner,first,second)==PT_MIXED_READERS_OK);
        c->port.original.base.ticks=oracle(960);
        assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED);
        fq_state(c,first,0,0,0,0);
        assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
        c->port.original.base.ticks=oracle(1920);
        assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_COMMITTED);
        fq_state(c,first,second,0,0,0);
        assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==PT_MIXED_READERS_OK);
        for(i=0;i<6;++i){
            assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);
            assert(pt_mixed_causal_service_reader(c->owner,second,i,1,NULL)==PT_MIXED_READERS_OK);
        }
        assert(!pt_mixed_readers_commands_held(c->trial->queue)&&
            !pt_mixed_readers_readers_held(c->trial->queue));
        tl_drop(c,0);return;
    }
    first=tl_first(c,6);fq_state(c,first,0,0,0,0);
    if(mode==1){control=tl_control(c,first,6,1);fq_state(c,first,control,0,0,0);}
    tl_drain(c,first,control,0,6,mode==1,0,0);tl_drop(c,0);
}
static void fq_context_and_registration(void)
{
    struct tl_case *c=tl_make(24,16,0),*other=tl_make(24,16,1);
    struct pt_mixed_causal_control_stop_port p={&c->port,sizeof(c->port),1,3,cc_publish_control,tl_publish_stop},bad;
    struct pt_mixed_causal_factory_bind_result binding;struct fq_snapshot *s;unsigned i;
    for(i=0;i<6;++i){bad=p;
        if(i==0)bad.context=&c->config;
        if(i==1)--bad.context_bytes;
        if(i==2)++bad.version;
        if(i==3)++bad.flags;
        if(i==4)bad.publish_control=NULL;
        if(i==5)bad.publish_stop_after_control=NULL;
        s=fq_before(c);assert(pt_mixed_causal_control_stop_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
        assert(!pt_mixed_causal_factory_control_stop_original_empty(c->owner,c->trial->queue,19,17));fq_unchanged(c,s);
    }
    assert(pt_mixed_causal_control_stop_bind(c->owner,&p)==PT_MIXED_READERS_OK);fq_state(c,0,0,1,0,0);
    s=fq_before(c);
    assert(!pt_mixed_causal_factory_control_stop_original_empty(c->owner,other->trial->queue,19,17));
    assert(!pt_mixed_causal_factory_control_stop_original_empty(c->owner,c->trial->queue,20,17));
    assert(!pt_mixed_causal_factory_control_stop_original_empty(c->owner,c->trial->queue,19,18));
    assert(!pt_mixed_causal_factory_control_stop_original_empty(NULL,c->trial->queue,19,17));
    assert(!pt_mixed_causal_factory_control_stop_original_empty(c->owner,NULL,19,17));
    assert(!pt_mixed_causal_factory_control_stop_original_empty(c->owner,c->trial->queue,0,17));
    assert(!pt_mixed_causal_factory_control_stop_original_empty(c->owner,c->trial->queue,19,0));
    assert(!pt_mixed_causal_factory_control_stop_first_current(c->owner,0));
    assert(!pt_mixed_causal_factory_control_stop_control_current(c->owner,0));
    /* A changed caller descriptor is not an installed port. Snapshot semantics
     * remain exact; original binding still refuses a retargeted context. */
    bad=p;bad.context=&c->config;bad.context_bytes=sizeof(c->config);
    binding=pt_mixed_causal_factory_bind_original(c->owner,ct_source_close,bad.context,bad.context_bytes);
    assert(!binding.called&&!binding.current&&!binding.raw);
    memset(&p,0,sizeof(p));
    assert(pt_mixed_causal_factory_control_stop_original_empty(c->owner,c->trial->queue,19,17));fq_unchanged(c,s);
    fq_state(other,0,0,1,0,0);tl_drop(other,0);tl_drop(c,0);
}
static unsigned fq_reentry_mode,fq_reentries;
static int fq_reentrant_bind(void *context,const struct pt_mixed_causal_registration *r)
{
    struct tl_port *p=context;struct pt_mixed_causal_owner *b=(void *)r->owner;
    struct pt_mixed_readers_output *queue=(void *)r->queue;
    assert(ct_registration(r,&p->original.base.registration));++fq_reentries;
    if(fq_reentry_mode==0)assert(!pt_mixed_causal_factory_control_stop_original_empty(b,queue,r->session,r->generation));
    if(fq_reentry_mode==1)assert(!pt_mixed_causal_factory_control_stop_first_current(b,1));
    if(fq_reentry_mode==2)assert(!pt_mixed_causal_factory_control_stop_control_current(b,1));
    return 1;
}
static void fq_task_reentry(unsigned mode)
{
    struct tl_case *c=tl_make(24,16,1);struct pt_mixed_causal_factory_bind_result binding;
    struct pt_mixed_causal_diagnostic d;struct fq_snapshot *s=fq_before(c);unsigned before=fq_reentries;
    fq_reentry_mode=mode;
    binding=pt_mixed_causal_factory_bind_original(c->owner,fq_reentrant_bind,&c->port,sizeof(c->port));
    assert(binding.called&&binding.raw==1&&!binding.current&&fq_reentries==before+1);
    fq_unchanged(c,s);assert(pt_mixed_causal_diagnostic(c->owner,&d)&&d.suppressed);
    fq_state(c,1,1,0,0,0);tl_drop(c,0);
}
static void fq_failed_detach(void)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=0,control;
    trigger_input(c->trial,6,0,960);c->trial->input->command.terminal=tl_terminal_fault;
    assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&first)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_publish(c->owner,first)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(960);assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED);
    control=tl_control(c,first,6,0);fq_state(c,first,control,0,0,0);
    c->port.original.hold_first=0;tl_fail_owner=c->owner;
    /* Lower queue positively consumes C1. Its terminal callback faults outer
     * ownership, so one free slot/actual OK does not qualify STOP preparation. */
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    assert(!c->trial->command[0].live&&c->trial->command[0].releases==1&&
        pt_mixed_readers_commands_held(c->trial->queue)==1);
    fq_state(c,first,control,0,0,0);tl_drain(c,first,control,0,6,1,0,1);tl_drop(c,0);
}
static void fq_failed_control(void)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=tl_first(c,6),control=0;
    struct pt_mixed_causal_control_request r=tl_control_request(c,first,6);
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    fq_state(c,first,0,0,1,0);
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&control)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_control_publish(c->owner,control)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(1920)-1;
    assert(pt_mixed_causal_fire(c->owner,control)==PT_MIXED_CAUSAL_EARLY);fq_state(c,first,control,0,0,0);
    ++c->port.original.base.ticks;c->port.original.base.commit_raw=0;
    assert(pt_mixed_causal_fire(c->owner,control)==PT_MIXED_CAUSAL_FAILED&&!c->port.control_completed);
    fq_state(c,first,control,0,0,0);tl_drain(c,first,control,0,6,1,0,1);tl_drop(c,0);
}
static int fq_changed_registration(void *context,const struct pt_mixed_causal_registration *r)
{
    struct tl_port *p=context;struct pt_mixed_causal_registration *changed=(void *)r;
    assert(ct_registration(r,&p->original.base.registration));++changed->session;return 1;
}
static void fq_registration_fault(void)
{
    struct tl_case *c=tl_make(24,16,1);struct pt_mixed_causal_factory_bind_result binding;
    struct fq_snapshot *s=fq_before(c);
    binding=pt_mixed_causal_factory_bind_original(c->owner,fq_changed_registration,&c->port,sizeof(c->port));
    assert(binding.called&&binding.raw==1&&!binding.current);fq_unchanged(c,s);
    fq_state(c,1,1,0,0,0);tl_drop(c,0);
}
static void fq_disposed_control(void)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=tl_first(c,6),control=tl_control(c,first,6,1),third=0;
    const struct pt_mixed_readers_event *retired_event=c->port.control.identity.event;
    struct pt_mixed_causal_stop_request r;struct fq_snapshot *s;
    assert(pt_mixed_causal_service_command(c->owner,control,0,NULL)==PT_MIXED_READERS_OK);
    assert(!c->trial->command[1].live&&c->trial->command[1].releases==1);
    fq_state(c,first,control,0,0,1);s=fq_before(c);
    CT_POISON(c->trial->command,sizeof(c->trial->command[0]));
    CT_POISON(c->trial->command+1,sizeof(c->trial->command[1]));
    CT_POISON((void *)retired_event,sizeof(*retired_event));
    /* Genuine C1/C2 storage is no longer authority. Only the copied opaque
     * identities and actual completed owner state are queried. */
    assert(pt_mixed_causal_factory_control_stop_control_current(c->owner,control));
    CT_UNPOISON((void *)retired_event,sizeof(*retired_event));
    CT_UNPOISON(c->trial->command,sizeof(c->trial->command[0]));
    CT_UNPOISON(c->trial->command+1,sizeof(c->trial->command[1]));fq_unchanged(c,s);
    r=tl_stop_request(c,control,6);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_OK);
    fq_state(c,first,control,0,0,0);
    assert(pt_mixed_causal_stop_after_control_publish(c->owner,third)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(2880);
    assert(pt_mixed_causal_fire(c->owner,third)==PT_MIXED_CAUSAL_COMMITTED);
    tl_drain(c,first,control,third,6,1,1,0);tl_drop(c,0);
}
int main(void)
{
    unsigned bits,cache,mode;
    for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)fq_sequence(bits,cache);
    for(mode=0;mode<3;++mode)fq_wrong_capability(mode);
    fq_context_and_registration();for(mode=0;mode<3;++mode)fq_task_reentry(mode);
    fq_failed_detach();fq_failed_control();fq_registration_fault();fq_disposed_control();
    puts("MIXED CAUSAL COMPOSITE FACTORY QUERIES HOST PASS;actual installed capability;actual first and CONTROL completion;clean C1 detach;wrong capability/context/registration/reentry/fault refusal;no new reader/cache/upload;original frames;independent cleanup;SOURCE_OWNER_MODEL_ONLY_NOT_NATIVE_AUDIO");
    return 0;
}
