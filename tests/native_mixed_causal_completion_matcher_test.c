#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN cm_baseline34_main
#include "native_mixed_causal_ram_port_test.c"
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN
#ifndef PT_CAUSAL_COMPLETION_MATCHER_TEST_MAIN
#define PT_CAUSAL_COMPLETION_MATCHER_TEST_MAIN main
#endif

/* Extra matcher checks run AFTER the unchanged literal34-case baseline above.
 * Real owner/port/controller state; no internal-owner type mirror or field edit. */
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN
static unsigned cm_checks,cm_cases;
static void cm_expect(struct cp_trial *f,
 const struct pt_mixed_causal_first_completion_match *request,int expected)
{
    size_t bytes=pt_mixed_causal_control_size();
    unsigned char *saved=malloc(bytes);
    struct pt_private_mixed_causal_ram_port *p=&cr_current->port,*port_saved=malloc(sizeof(*p));
    struct pt_mixed_causal_first_completion_match original;
    assert(saved&&port_saved&&f->control.causal);
    memcpy(saved,f->control.causal,bytes);memcpy(port_saved,p,sizeof(*p));memcpy(&original,request,sizeof(original));
    assert(pt_mixed_causal_first_completion_matches(f->control.causal,*request)==expected);
    /* Regular matches/mismatches leave complete genuine owner and port unchanged,
     * no callback/clock/service/release: not just a counter-only certificate. */
    assert(!memcmp(saved,f->control.causal,bytes)&&!memcmp(port_saved,p,sizeof(*p)));
    assert(!memcmp(&original,request,sizeof(original)));++cm_checks;
    free(port_saved);free(saved);
}
static struct pt_mixed_causal_first_completion_match cm_original(struct pt_private_mixed_causal_ram_port *p)
{
    struct pt_mixed_causal_first_completion_match r;
    memset(&r,0,sizeof(r));r.predecessor=p->command[0].identity;r.successor=p->command[1].identity;
    r.serial=p->command[1].serial;r.first_tick=p->command[0].packet.first;r.last_tick=p->command[0].packet.last;
    /* Before completion this is explicitly a prediction: matcher MUST reject. */
    r.observed=r.issued=r.first_tick;r.post.active_mask=r.post.adopted_mask=p->command[1].packet.expected_mask;
    memcpy(r.post.slot,p->command[1].packet.expected,sizeof(r.post.slot));return r;
}
static void cm_actual(struct cp_trial *f,struct pt_mixed_causal_first_completion_match *r)
{
    struct pt_mixed_causal_diagnostic d;struct pt_private_mixed_causal_ram_port *p=&cr_current->port;
    memset(&d,0,sizeof(d));assert(pt_mixed_causal_diagnostic(f->control.causal,&d));
    assert(d.completed&&d.admitted&&d.published&&!d.suppressed&&d.commit_called&&d.commit_outcome==1);
    assert(d.first==r->predecessor.ticket&&d.successor==r->successor.ticket&&d.serial==r->serial);
    r->observed=d.observed;r->issued=d.issued;r->post.active_mask=r->post.adopted_mask=p->mask;
    memcpy(r->post.slot,p->slot,sizeof(r->post.slot));
    assert(p->command[1].predecessor_completed&&r->observed==p->command[1].predecessor_observed&&
           r->issued==p->command[1].predecessor_issued);
}
static void cm_forge_identity(struct pt_mixed_causal_command_identity *i,unsigned field)
{
    switch(field){
    case 0:i->registration.owner=NULL;break;case 1:i->registration.queue=NULL;break;
    case 2:++i->registration.session;break;case 3:++i->registration.generation;break;
    case 4:++i->ticket;break;case 5:++i->owner;break;case 6:i->event=NULL;break;
    case 7:i->binding.context=NULL;break;case 8:++i->binding.context_bytes;break;
    default:assert(0);
    }
}
static void cm_forge_key(struct pt_mixed_readers_key *k,unsigned field,
 const struct pt_mixed_readers_output *queue)
{
    switch(field){
    case 0:k->queue=k->queue?NULL:queue;break;case 1:++k->session;break;
    case 2:++k->generation;break;case 3:++k->trigger;break;case 4:++k->owner;break;
    case 5:++k->serial;break;case 6:++k->action;break;case 7:++k->route;break;
    case 8:++k->slot;break;default:assert(0);
    }
}
static void cm_rejections(struct cp_trial *f,const struct pt_mixed_causal_first_completion_match *good)
{
    struct pt_mixed_causal_first_completion_match bad;unsigned i,j;
    for(i=0;i<2;++i)for(j=0;j<9;++j){memcpy(&bad,good,sizeof(bad));
        cm_forge_identity(i?&bad.successor:&bad.predecessor,j);cm_expect(f,&bad,0);}
    for(i=0;i<8;++i){memcpy(&bad,good,sizeof(bad));switch(i){
        case 0:++bad.first_tick;break;case 1:++bad.last_tick;break;
        case 2:bad.first_tick=bad.last_tick;break;case 3:++bad.observed;break;
        case 4:++bad.issued;break;case 5:bad.observed=bad.first_tick-1;break;
        case 6:bad.issued=bad.observed-1;break;case 7:bad.issued=bad.last_tick;break;
        default:assert(0);}cm_expect(f,&bad,0);}
    memcpy(&bad,good,sizeof(bad));++bad.serial;cm_expect(f,&bad,0);
    memcpy(&bad,good,sizeof(bad));bad.serial=0;cm_expect(f,&bad,0);
    memcpy(&bad,good,sizeof(bad));bad.post.active_mask^=1U;cm_expect(f,&bad,0);
    memcpy(&bad,good,sizeof(bad));bad.post.adopted_mask^=1U;cm_expect(f,&bad,0);
    memcpy(&bad,good,sizeof(bad));bad.post.active_mask|=1U<<20;bad.post.adopted_mask=bad.post.active_mask;cm_expect(f,&bad,0);
    /* Every scalar in all20 slots, including zero/untouched slots16..19. */
    for(i=0;i<20;++i)for(j=0;j<9;++j){memcpy(&bad,good,sizeof(bad));
        cm_forge_key(bad.post.slot+i,j,f->control.queue);cm_expect(f,&bad,0);}
}
static void cm_success(void)
{
    struct cp_trial *f=cr_make(24,16,0);struct cr_before *before=cr_capture(f);
    struct cr_state *s=&cr_current->state;struct pt_private_mixed_causal_ram_port *p=&cr_current->port;
    struct pt_mixed_causal_first_completion_match r;uint64_t first,second;
    cp_open(f);cp_requests(f,16);cp_prepare(f,0,960);cp_refs(f,0,0);first=cp_admit(f,0);
    cp_prepare(f,1,1920);cp_refs(f,1,16);second=cp_admit(f,1);r=cm_original(p);
    cm_expect(f,&r,0);s->now=oracle(960)-1;
    assert(pt_private_mixed_causal_ram_dispatch(p,first)==PT_MIXED_CAUSAL_EARLY);cm_expect(f,&r,0);
    assert(!p->effects&&!cr_readers(p)&&s->arm[0].live&&s->arm[1].live);
    s->now=oracle(960);assert(pt_private_mixed_causal_ram_dispatch(p,first)==PT_MIXED_CAUSAL_COMMITTED);
    assert(p->effects==16&&cr_readers(p)==16);cm_actual(f,&r);cm_expect(f,&r,1);cm_rejections(f,&r);
    s->poison_command=f->control.command[f->command[0].slot].handle.address;assert(s->poison_command);
    cp_drain_command(f,0,0);assert(s->poisoned==1&&!s->poison_command&&!p->command[0].owner&&!p->command[0].live);
    assert(s->arm[1].live&&p->command[1].predecessor_completed);cm_expect(f,&r,1);
    s->now=oracle(1920);assert(pt_private_mixed_causal_ram_dispatch(p,second)==PT_MIXED_CAUSAL_COMMITTED);
    assert(p->effects==32&&cr_readers(p)==32);cm_expect(f,&r,0);
    cp_drain_command(f,1,0);cp_drain_readers(f,0,16,0);cp_drain_readers(f,16,16,0);
    cr_same(f,before);cr_closed(f,before);++cm_cases;
}
static void cm_successor_cancelled(void)
{
    struct cp_trial *f=cr_make(24,16,0);struct cr_before *before=cr_capture(f);
    struct cr_state *s=&cr_current->state;struct pt_private_mixed_causal_ram_port *p=&cr_current->port;
    struct pt_mixed_causal_first_completion_match r;uint64_t first;
    cp_open(f);cp_requests(f,16);cp_prepare(f,0,960);cp_refs(f,0,0);first=cp_admit(f,0);
    cp_prepare(f,1,1920);cp_refs(f,1,16);assert(cp_admit(f,1));r=cm_original(p);
    s->now=oracle(960);assert(pt_private_mixed_causal_ram_dispatch(p,first)==PT_MIXED_CAUSAL_COMMITTED);
    cm_actual(f,&r);cm_expect(f,&r,1);cp_drain_command(f,1,0);cm_expect(f,&r,0);
    cp_drain_command(f,0,0);cm_expect(f,&r,0);
    cp_drain_readers(f,0,16,0);cp_drain_readers(f,16,16,0);cr_same(f,before);cr_closed(f,before);++cm_cases;
}
int PT_CAUSAL_COMPLETION_MATCHER_TEST_MAIN(void)
{
    assert(cm_baseline34_main()==0&&cr_cases==34);
    cm_success();cm_successor_cancelled();assert(cm_cases==2&&cr_cases==36&&cm_checks==219);
    puts("CAUSAL COMPLETION MATCHER PASS:2 added genuine cases;219 immutable-owner/port by-value checks;actual first commit before/after poisoned disposed C;18 identity fields,8 original-window/observed-issued,2 serial,3 masks,180 all20-key forgeries;before completion and successor issued/cancelled reject;baseline34 preserved separately; HOST_ONLY");
    return 0;
}
