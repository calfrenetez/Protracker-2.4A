/* Private HOST after-first CONTROL groundwork. Real production C compiles
 * separately; unchanged committed fixture helpers supply genuine holders,
 * master versions, selective Chip caches and evictable card cache leases. */
#define PT_MIXED_CAUSAL_TEST_MAIN cc_original_fixture_not_called
#include "mixed_readers_causal_test.c"
#include "../src/core/mixed_readers_causal_control_internal.h"
struct cc_port {
    struct ct_port base;
    struct pt_mixed_causal_command_identity first_identity;
    struct pt_mixed_causal_actual first_post;
    uint64_t first_tick,last_tick,observed,issued;
    unsigned first_completed,hold_first,controls,control_publications;
    int publication_raw;
    unsigned publication_reentry,publication_mutation;
    unsigned stops,stop_publications,stopped_mask,quiet_mask,hold_stop;
};
struct cc_case {
    struct trial *trial;struct cc_port port;struct ct_memory memory;
    struct pt_mixed_causal_config config;struct pt_mixed_causal_owner *owner;
    void *workspace;uint8_t *before;size_t before_bytes;
};
static unsigned cc_cases;
static struct pt_mixed_causal_owner *cc_reentry_owner;
static struct pt_mixed_causal_control_request *cc_mutate_request;
static int cc_current(void *context,uint64_t token,uint64_t generation)
{
    if(cc_reentry_owner){struct pt_mixed_causal_diagnostic d;struct pt_mixed_causal_owner *b=cc_reentry_owner;
        cc_reentry_owner=NULL;assert(!pt_mixed_causal_diagnostic(b,&d));}
    if(cc_mutate_request){cc_mutate_request->frame++;cc_mutate_request=NULL;}
    return held_current(context,token,generation);
}
static int cc_identity(const struct pt_mixed_causal_command_identity *a,const struct pt_mixed_causal_command_identity *b)
{return ct_registration(&a->registration,&b->registration)&&a->ticket==b->ticket&&a->owner==b->owner&&
    a->event==b->event&&a->binding.context==b->binding.context&&a->binding.context_bytes==b->binding.context_bytes;}
static int cc_publish_control(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_control_publication *d)
{
    struct cc_port *p=context;unsigned i;int raw=p->publication_raw;
    ++p->control_publications;
    /* Own geometry-free first completion and own actual registry are authority;
     * owner copied completion is compared, never substituted as port evidence. */
    if(!p->first_completed||owner!=p->base.registration.owner||!d->serial||
       !cc_identity(&p->first_identity,&d->predecessor)||
       d->first_tick!=p->first_tick||d->last_tick!=p->last_tick||d->observed!=p->observed||d->issued!=p->issued||
       d->observed<d->first_tick||d->issued<d->observed||d->issued>=d->last_tick||
       d->first_post.active_mask!=p->base.mask||d->first_post.adopted_mask!=p->base.mask||
       !ct_expected(&p->base,&d->packet))return 0;
    for(i=0;i<20;++i)if(!keys_equal(d->first_post.slot+i,p->first_post.slot+i)||
        !keys_equal(d->first_post.slot+i,p->base.slot+i))return 0;
    for(i=0;i<d->packet.count;++i)if(d->packet.action[i].kind!=PT_MIXED_READERS_CONTROL||
        d->packet.key[i].trigger!=p->first_identity.ticket||!ct_reader(&p->base,d->packet.key+i))return 0;
    if(raw!=0&&!ct_publish(&p->base,owner,&d->successor,&d->packet))return 0;
    if(p->publication_reentry){struct pt_mixed_causal_diagnostic diag;p->publication_reentry=0;
        assert(!pt_mixed_causal_diagnostic(owner,&diag));}
    if(p->publication_mutation){struct pt_mixed_causal_control_publication *bad=(void *)d;
        p->publication_mutation=0;bad->first_post.slot[19].serial=1234;}
    return raw;
}
static int cc_commit(void *context,const struct pt_mixed_causal_packet *b,struct pt_mixed_causal_actual *actual)
{
    struct cc_port *p=context;struct ct_port *base=&p->base;struct ct_command *c=ct_command(base,b->ticket);
    unsigned i;int raw;
    if(b->action[0].kind==PT_MIXED_READERS_TRIGGER){
        struct pt_mixed_causal_command_identity id=c?c->identity:(struct pt_mixed_causal_command_identity){0};
        uint64_t before=base->ticks;raw=ct_commit(base,b,actual);
        if(raw==1){p->first_identity=id;p->first_post=*actual;p->first_tick=b->first;p->last_tick=b->last;
            p->observed=before;p->issued=base->ticks;p->first_completed=1;}
        return raw;
    }
    ++base->commits;raw=base->commit_raw;
    if(!c||c->finished||c->disabled||!p->first_completed||!ct_expected(base,b)||memcmp(&c->packet,b,sizeof(*b))||
       base->ticks<b->first||base->ticks>=b->last||!raw)return 0;
    for(i=0;i<b->count;++i)if(b->action[i].kind!=PT_MIXED_READERS_CONTROL||!ct_reader(base,b->key+i))return 0;
    for(i=0;i<b->count;++i){struct ct_reader *r=ct_reader(base,b->key+i);
        /* Only numeric controls change. Source/geometry/cache/card/key and live
         * reader identity persist; no reader allocation or retrigger. */
        if(b->action[i].route==PT_MIXED_READERS_PAULA){r->action.geometry.paula.period=b->action[i].geometry.paula.period;
            r->action.geometry.paula.volume=b->action[i].geometry.paula.volume;}
        else{r->action.geometry.amigus.rate=b->action[i].geometry.amigus.rate;r->action.geometry.amigus.left=b->action[i].geometry.amigus.left;
            r->action.geometry.amigus.right=b->action[i].geometry.amigus.right;}
        ++p->controls;if(raw<0)break;
    }
    c->finished=1;ct_geometry_drop(c);actual->active_mask=actual->adopted_mask=base->mask;memcpy(actual->slot,base->slot,sizeof(base->slot));
    if(base->malformed){actual->slot[19].serial=777;actual->active_mask|=1U<<19;actual->adopted_mask=actual->active_mask;}
    if(base->bad_adoption)actual->adopted_mask&=~1U;
    if(base->late)base->ticks=b->last;
    if(base->reenter){base->reenter=0;assert(pt_mixed_causal_fire((void *)base->registration.owner,b->ticket)==PT_MIXED_CAUSAL_INVALID);}
    return raw;
}
static int cc_command_quiet(void *context,const struct pt_mixed_causal_command_identity *id,unsigned cancel)
{
    struct cc_port *p=context;
    if(p->hold_first&&!cancel&&id->ticket==p->first_identity.ticket){++p->base.command_proofs;return 0;}
    return ct_command_quiet(&p->base,id,cancel);
}
static struct cc_case *cc_make(unsigned bits,unsigned cache_bits,unsigned bind)
{
    struct cc_case *c=calloc(1,sizeof(*c));struct pt_mixed_causal_control_port port;assert(c);++cc_cases;
    c->trial=trial_make(bits,cache_bits);c->before=save(c->trial->resources,&c->before_bytes);
    c->port.base.ticks=100;c->port.base.frequency=709379;c->port.base.commit_raw=c->port.base.source_raw=1;c->port.publication_raw=1;
    c->config.allocator=(struct pt_allocator){&c->memory,ct_allocate,ct_release};
    c->config.allocator_context=(struct pt_mixed_readers_span){&c->memory,sizeof(c->memory)};
    c->config.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};c->config.session=19;
    c->config.control_budget=pt_mixed_causal_control_size();c->config.queue_budget=pt_mixed_readers_control_size();
    c->config.port=(struct pt_mixed_causal_port){&c->port,sizeof(c->port),1,31,ct_clock,ct_publish,cc_commit,
        cc_command_quiet,ct_reader_quiet,ct_source_close,ct_source_quiet,ct_publish_successor};
    c->workspace=calloc(1,pt_mixed_causal_workspace_size());assert(c->workspace);
    assert(pt_mixed_causal_open(&c->config,c->workspace,pt_mixed_causal_workspace_size(),&c->owner)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_borrow_queue(c->owner,&c->trial->queue)==PT_MIXED_READERS_OK&&c->memory.live==2);
    c->port.base.registration=(struct pt_mixed_causal_registration){c->owner,c->trial->queue,19,17};
    port=(struct pt_mixed_causal_control_port){&c->port,sizeof(c->port),1,3,cc_publish_control};
    if(bind)assert(pt_mixed_causal_control_bind(c->owner,&port)==PT_MIXED_READERS_OK);
    return c;
}
static uint64_t cc_first(struct cc_case *c,unsigned count)
{
    uint64_t ticket=0;trigger_input(c->trial,count,0,960);
    assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&ticket)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_publish(c->owner,ticket)==PT_MIXED_READERS_OK);return ticket;
}
static void cc_actual_first(struct cc_case *c,uint64_t first)
{c->port.base.ticks=ct_command(&c->port.base,first)->packet.first;
 assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED);}
static struct pt_mixed_causal_control_request cc_request(struct cc_case *c,uint64_t first,unsigned count)
{
    struct pt_mixed_causal_control_request r;unsigned i;memset(&r,0,sizeof(r));
    holder_init(c->trial->command+1,101);c->trial->command[1].queue=c->trial->queue;
    r.predecessor=first;r.frame=1920;r.count=count;r.command=control_of(c->trial->command+1);
    for(i=0;i<count;++i){struct pt_mixed_causal_control_action *a=r.action+i;a->route=i<4?1:2;a->slot=i<4?i:i-4;
        if(a->route==1){a->period=(uint16_t)(214+i);a->volume=(uint8_t)(32+i);}
        else{a->rate=0x10000U+i;a->left=(uint8_t)i;a->right=(uint8_t)(250-i);}}
    return r;
}
static void cc_retire(struct cc_case *c,uint64_t first,uint64_t second,unsigned count,unsigned first_disposed,unsigned failed)
{
    unsigned i;enum pt_mixed_readers_result expected=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
    if(!first_disposed)assert(pt_mixed_causal_service_command(c->owner,first,1,NULL)==expected);
    if(second)assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==expected);
    for(i=0;i<count;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==expected);
    assert(!pt_mixed_readers_commands_held(c->trial->queue)&&!pt_mixed_readers_readers_held(c->trial->queue));
}
static void cc_drop(struct cc_case *c,unsigned unknown_source)
{
    same_save(c->trial->resources,c->before,c->before_bytes);
    if(unknown_source){c->port.base.source_raw=-1;assert(!pt_mixed_causal_close(&c->owner)&&c->owner&&c->memory.live==1);
        c->trial->queue=NULL;assert(pt_mixed_causal_close(&c->owner)&&c->port.base.shutdowns==1&&c->port.base.probes==1);}
    else assert(pt_mixed_causal_close(&c->owner)&&c->port.base.shutdowns==1);
    assert(!c->owner&&!c->memory.live&&pt_mixed_causal_close(&c->owner));c->trial->queue=NULL;
    same_save(c->trial->resources,c->before,c->before_bytes);trial_drop(c->trial);
    free(c->before);free(c->workspace);free(c);
}
static void cc_unused_command(struct cc_case *c)
{struct holder *h=c->trial->command+1;held_terminal(h,h->token,1);held_release(h,h->token);}
static void cc_success(unsigned bits,unsigned cache_bits,unsigned mode)
{
    struct cc_case *c=cc_make(bits,cache_bits,1);uint64_t first=cc_first(c,16),second=0,out=777;
    struct pt_mixed_causal_control_request r=cc_request(c,first,mode==2?1:16),saved=r;
    struct pt_mixed_readers_key keys[16],key;struct pt_cache_lease leases[16];struct pt_sample_version *pins[16];
    struct ct_reader readers[32];struct pt_mixed_readers_command_receipt receipt;
    struct pt_mixed_causal_first_completion_match old_match={0};unsigned i,writes,chips,calls;
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    cc_actual_first(c,first);memcpy(readers,c->port.base.reader,sizeof(readers));
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&out)!=PT_MIXED_READERS_OK&&out==777);
    assert(!pt_mixed_causal_first_completion_matches(c->owner,old_match));
    if(mode==2){assert(pt_mixed_causal_service_reader(c->owner,first,0,0,NULL)==PT_MIXED_READERS_PENDING);}
    else{c->port.hold_first=mode==1;
        assert(pt_mixed_causal_service_command(c->owner,first,0,&receipt)==(mode==1?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_OK));
        assert(receipt.action[0].command==PT_MIXED_COMMAND_ISSUED&&receipt.action[0].reader==PT_MIXED_READER_ACTIVE);}
    if(mode==0){assert(!c->trial->command[0].live);CT_POISON(c->trial->command,sizeof(c->trial->command[0]));}
    for(i=0;i<16;++i){keys[i]=c->port.base.slot[ct_index(i<4?1:2,i<4?i:i-4)];leases[i]=c->trial->reader[i].lease;pins[i]=c->trial->reader[i].pin;}
    writes=c->trial->resources->card.writes;chips=c->trial->resources->chips;calls=c->memory.calls;
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&second)==PT_MIXED_READERS_OK&&second&&second!=first);
    assert(!memcmp(&r,&saved,sizeof(r))&&pt_mixed_readers_readers_held(c->trial->queue)==16);
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    assert(pt_mixed_causal_publish_successor(c->owner,first,second)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_control_publish(c->owner,second)==PT_MIXED_READERS_OK);
    assert(ct_command(&c->port.base,second)->packet.frame==1920&&c->port.control_publications==1);
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_EARLY&&!c->port.controls);
    assert(!memcmp(readers,c->port.base.reader,sizeof(readers)));
    c->port.base.ticks=ct_command(&c->port.base,second)->packet.first;
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_COMMITTED&&c->port.controls==r.count&&c->port.base.effects==16);
    for(i=0;i<16;++i){unsigned index=ct_index(i<4?1:2,i<4?i:i-4);struct ct_reader *active=ct_reader(&c->port.base,keys+i);
        assert(active&&keys_equal(keys+i,c->port.base.slot+index)&&pins[i]==c->trial->reader[i].pin&&
            !memcmp(leases+i,&c->trial->reader[i].lease,sizeof(leases[i]))&&c->trial->reader[i].live);
        if(i<r.count){if(i<4)assert(active->action.geometry.paula.data==readers[i].action.geometry.paula.data&&
            active->action.geometry.paula.words==readers[i].action.geometry.paula.words&&active->action.geometry.paula.period==r.action[i].period);
            else assert(active->action.geometry.amigus.start==readers[i].action.geometry.amigus.start&&
                active->action.geometry.amigus.end_exclusive==readers[i].action.geometry.amigus.end_exclusive&&active->action.geometry.amigus.rate==r.action[i].rate);}
        assert(!memcmp(&active->card,&readers[i].card,sizeof(active->card)));}
    assert(calls==c->memory.calls&&writes==c->trial->resources->card.writes&&chips==c->trial->resources->chips);
    assert(pt_mixed_causal_service_command(c->owner,second,0,NULL)==PT_MIXED_READERS_OK);
    if(mode==0){assert(pt_mixed_causal_reader_key(c->owner,first,0,&key)==PT_MIXED_READERS_OK&&keys_equal(&key,keys));}
    c->port.hold_first=0;cc_retire(c,first,0,16,mode==0,0);
    if(mode==0)CT_UNPOISON(c->trial->command,sizeof(c->trial->command[0]));
    cc_drop(c,mode==1);
}
static void cc_guards(void)
{
    struct cc_case *c=cc_make(24,16,0);struct pt_mixed_causal_control_port p={&c->port,sizeof(c->port),1,3,cc_publish_control},bad=p;
    struct pt_mixed_causal_control_request r,saved;struct pt_mixed_causal_diagnostic before,after;
    uint64_t first,out=777;unsigned i;
    bad.version=2;assert(pt_mixed_causal_control_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    bad=p;bad.context_bytes--;assert(pt_mixed_causal_control_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    bad=p;bad.publish_control=NULL;assert(pt_mixed_causal_control_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_control_bind(c->owner,&p)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_control_bind(c->owner,&p)==PT_MIXED_READERS_INVALID);
    first=cc_first(c,16);cc_actual_first(c,first);
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=cc_request(c,first,2);saved=r;
    assert(pt_mixed_causal_diagnostic(c->owner,&before));
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&r.frame)==PT_MIXED_READERS_INVALID&&!memcmp(&r,&saved,sizeof(r)));
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&c->trial->reader[0].token)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,(void *)c->trial->reader[0].spans[0].data)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,(void *)((uint8_t *)&c->port+sizeof(c->port)-8))==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_control_enqueue(c->owner,(void *)&c->trial->reader[0],&out)==PT_MIXED_READERS_INVALID&&out==777);
    for(i=0;i<5;++i){r=saved;if(i==0)r.action[1]=r.action[0];if(i==1)r.action[0].rate=1;
        if(i==2)r.action[15].volume=1;if(i==3)r.frame=960;if(i==4)r.predecessor++;
        assert(pt_mixed_causal_control_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);}
    r=saved;r.command.context=&r;r.command.context_bytes=sizeof(r);
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    assert(pt_mixed_causal_diagnostic(c->owner,&after)&&!memcmp(&before,&after,sizeof(before)));
    cc_unused_command(c);cc_retire(c,first,0,16,1,0);cc_drop(c,0);
}
static void cc_expected_only(unsigned unknown)
{
    struct cc_case *c=cc_make(16,8,1);uint64_t first=cc_first(c,6),second=0;struct pt_mixed_causal_control_request r;
    unsigned i;cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=cc_request(c,first,1);assert(pt_mixed_causal_control_enqueue(c->owner,&r,&second)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_control_publish(c->owner,second)==PT_MIXED_READERS_OK);
    if(unknown){c->port.base.unknown_ticket=second;
        assert(pt_mixed_causal_service_reader(c->owner,first,5,1,NULL)==PT_MIXED_READERS_BACKEND);
        assert(c->trial->reader[5].live&&c->trial->reader[5].pin&&!c->trial->reader[5].releases);
        c->port.base.unknown_ticket=0;}
    assert(pt_mixed_causal_service_reader(c->owner,first,5,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    assert(ct_command(&c->port.base,second)->disabled&&!c->trial->reader[5].live);
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_INVALID);
    assert(!c->port.controls);
    assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    for(i=0;i<5;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    cc_drop(c,0);
}
static void cc_failure(unsigned mode)
{
    struct cc_case *c=cc_make(24,16,1);uint64_t first=cc_first(c,6),second=0;struct pt_mixed_causal_control_request r;
    struct pt_mixed_causal_diagnostic d;unsigned i;
    cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=cc_request(c,first,6);
    if(mode==7||mode==8){r.command.current=cc_current;if(mode==7)cc_reentry_owner=c->owner;else cc_mutate_request=&r;}
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&second)==PT_MIXED_READERS_OK&&second&&c->trial->command[1].live);
    if(mode>=7){assert(pt_mixed_causal_diagnostic(c->owner,&d)&&d.suppressed);
        assert(pt_mixed_causal_control_publish(c->owner,second)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_causal_stop(c->owner)==PT_MIXED_READERS_BACKEND&&!c->trial->command[1].live);
        for(i=0;i<6;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_BACKEND);
        cc_drop(c,0);return;}
    assert(pt_mixed_causal_control_publish(c->owner,second)==PT_MIXED_READERS_OK);
    c->port.base.ticks=ct_command(&c->port.base,second)->packet.first;
    if(mode==0)c->port.base.commit_raw=-1;if(mode==1)c->port.base.commit_raw=0;
    if(mode==2)c->port.base.malformed=1;if(mode==3)c->port.base.bad_adoption=1;
    if(mode==4)c->port.base.late=1;if(mode==5)c->port.base.reenter=1;
    if(mode==6)c->port.base.ticks=ct_command(&c->port.base,second)->packet.last;
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_FAILED);
    assert(pt_mixed_causal_diagnostic(c->owner,&d)&&d.suppressed);
    for(i=0;i<6;++i)assert(c->trial->reader[i].live&&c->trial->reader[i].pin);
    cc_retire(c,first,second,6,1,1);cc_drop(c,0);
}
static void cc_publication_failure(unsigned mode)
{
    struct cc_case *c=cc_make(8,16,1);uint64_t first=cc_first(c,6),second=0;struct pt_mixed_causal_control_request r;
    cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=cc_request(c,first,6);assert(pt_mixed_causal_control_enqueue(c->owner,&r,&second)==PT_MIXED_READERS_OK);
    c->port.publication_raw=mode==0?0:-1;if(mode==1){c->port.publication_raw=0;c->port.publication_reentry=1;}
    if(mode==2){c->port.publication_raw=1;c->port.publication_mutation=1;}
    assert(pt_mixed_causal_control_publish(c->owner,second)==(mode==0?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_BACKEND));
    assert(!c->port.controls&&c->trial->reader[0].live);
    if(mode==0){unsigned i;struct pt_mixed_causal_diagnostic d;
        assert(pt_mixed_causal_diagnostic(c->owner,&d)&&!d.suppressed&&!d.published);
        assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_causal_stop(c->owner)==PT_MIXED_READERS_PENDING&&!c->trial->command[1].live);
        for(i=0;i<6;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);
    }else cc_retire(c,first,second,6,1,1);
    cc_drop(c,0);
}
int ss_control_fixture_not_called(void)
{
    unsigned bits,cache_bits,mode;
    for(bits=8;bits<=24;bits+=8)for(cache_bits=8;cache_bits<=16;cache_bits+=8)for(mode=0;mode<3;++mode)cc_success(bits,cache_bits,mode);
    cc_guards();cc_expected_only(0);cc_expected_only(1);
    for(mode=0;mode<9;++mode)cc_failure(mode);
    for(mode=0;mode<4;++mode)cc_publication_failure(mode);
    assert(cc_cases==34);
    puts("MIXED CAUSAL AFTER FIRST CONTROL HOST PASS:34 genuine heap cases;18 mixed4/12 or subset 8/16/24-master cache8/16 same-reader lifetimes;actual first and queue ACTIVE C/R observations;disposed poisoned first C;typed guarded internally derived keys;distinct capability and port completion;expected-only quiet;partial/zero/malformed/late/reentry/uncertain retention;actual enqueue transfer authority;master beforeimages;NO_NATIVE_APERTURE_QUALIFICATION");
    return 0;
}
#include "../src/core/mixed_readers_causal_stop_internal.h"
static unsigned ss_cases;
static struct pt_mixed_causal_stop_request *ss_mutate_request;
static int ss_current(void *context,uint64_t token,uint64_t generation)
{
    if(ss_mutate_request){ss_mutate_request->frame++;ss_mutate_request=NULL;}
    return cc_current(context,token,generation);
}
static int ss_publish_stop(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_stop_publication *d)
{
    struct cc_port *p=context;unsigned i;int raw=p->publication_raw;
    ++p->stop_publications;
    if(!p->first_completed||owner!=p->base.registration.owner||d->serial!=1||
       !cc_identity(&p->first_identity,&d->predecessor)||
       d->first_tick!=p->first_tick||d->last_tick!=p->last_tick||d->observed!=p->observed||d->issued!=p->issued||
       d->observed<d->first_tick||d->issued<d->observed||d->issued>=d->last_tick||
       d->first_post.active_mask!=p->base.mask||d->first_post.adopted_mask!=p->base.mask||
       !ct_expected(&p->base,&d->packet))return 0;
    for(i=0;i<20;++i)if(!keys_equal(d->first_post.slot+i,p->first_post.slot+i)||
       !keys_equal(d->first_post.slot+i,p->base.slot+i))return 0;
    for(i=0;i<d->packet.count;++i){const struct pt_mixed_readers_action *a=d->packet.action+i;
        int zero=a->route==PT_MIXED_READERS_PAULA?
            !a->geometry.paula.data&&!a->geometry.paula.words&&!a->geometry.paula.period&&!a->geometry.paula.volume:
            !a->geometry.amigus.start&&!a->geometry.amigus.loop&&!a->geometry.amigus.end_exclusive&&
            !a->geometry.amigus.rate&&!a->geometry.amigus.control&&!a->geometry.amigus.left&&!a->geometry.amigus.right;
        if(a->kind!=PT_MIXED_READERS_STOP||!zero||
           d->packet.key[i].trigger!=p->first_identity.ticket||!ct_reader(&p->base,d->packet.key+i))return 0;}
    if(raw!=0&&!ct_publish(&p->base,owner,&d->successor,&d->packet))return 0;
    if(p->publication_reentry){struct pt_mixed_causal_diagnostic diag;p->publication_reentry=0;
        assert(!pt_mixed_causal_diagnostic(owner,&diag));}
    if(p->publication_mutation){struct pt_mixed_causal_stop_publication *bad=(void *)d;
        p->publication_mutation=0;bad->first_post.slot[19].serial=1234;}
    return raw;
}
static int ss_commit(void *context,const struct pt_mixed_causal_packet *b,struct pt_mixed_causal_actual *actual)
{
    struct cc_port *p=context;struct ct_port *base=&p->base;struct ct_command *c=ct_command(base,b->ticket);
    unsigned i;int raw;
    if(b->action[0].kind==PT_MIXED_READERS_TRIGGER)return cc_commit(context,b,actual);
    ++base->commits;raw=base->commit_raw;
    if(!c||c->finished||c->disabled||!p->first_completed||!ct_expected(base,b)||memcmp(&c->packet,b,sizeof(*b))||
       base->ticks<b->first||base->ticks>=b->last||!raw)return 0;
    /* Device voice activity is distinct from persistent ct_reader ownership. */
    for(i=0;i<b->count;++i)if(b->action[i].kind!=PT_MIXED_READERS_STOP||!ct_reader(base,b->key+i))return 0;
    for(i=0;i<b->count;++i){unsigned index=ct_index(b->action[i].route,b->action[i].slot),bit=1U<<index;
        base->mask&=~bit;memset(base->slot+index,0,sizeof(base->slot[index]));p->stopped_mask|=bit;++p->stops;
        if(raw<0)break;}
    c->finished=1;ct_geometry_drop(c);actual->active_mask=actual->adopted_mask=base->mask;memcpy(actual->slot,base->slot,sizeof(base->slot));
    if(base->malformed){actual->slot[19].serial=777;actual->active_mask|=1U<<19;actual->adopted_mask=actual->active_mask;}
    if(base->bad_adoption)actual->adopted_mask^=1U;
    if(base->late)base->ticks=b->last;
    if(base->reenter){base->reenter=0;assert(pt_mixed_causal_fire((void *)base->registration.owner,b->ticket)==PT_MIXED_CAUSAL_INVALID);}
    return raw;
}
static int ss_command_quiet(void *context,const struct pt_mixed_causal_command_identity *id,unsigned cancel)
{
    struct cc_port *p=context;
    if(p->hold_stop&&!cancel&&id->ticket!=p->first_identity.ticket){++p->base.command_proofs;return 0;}
    return cc_command_quiet(context,id,cancel);
}
static int ss_reader_quiet(void *context,const struct pt_mixed_causal_reader_identity *id,unsigned cancel)
{
    struct cc_port *p=context;unsigned bit=1U<<ct_index(id->key.route,id->key.slot);
    if((p->stopped_mask&bit)&&!(p->quiet_mask&bit)){++p->base.reader_proofs;return 0;}
    if((p->stopped_mask&bit)&&(p->quiet_mask&bit))return ct_reader_quiet(context,id,1);
    return ct_reader_quiet(context,id,cancel);
}
static struct cc_case *ss_make(unsigned bits,unsigned cache_bits,unsigned bind)
{
    struct cc_case *c=calloc(1,sizeof(*c));struct pt_mixed_causal_stop_port port;assert(c);++ss_cases;
    c->trial=trial_make(bits,cache_bits);c->before=save(c->trial->resources,&c->before_bytes);
    c->port.base.ticks=100;c->port.base.frequency=709379;c->port.base.commit_raw=c->port.base.source_raw=1;c->port.publication_raw=1;
    c->config.allocator=(struct pt_allocator){&c->memory,ct_allocate,ct_release};
    c->config.allocator_context=(struct pt_mixed_readers_span){&c->memory,sizeof(c->memory)};
    c->config.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};c->config.session=19;
    c->config.control_budget=pt_mixed_causal_control_size();c->config.queue_budget=pt_mixed_readers_control_size();
    c->config.port=(struct pt_mixed_causal_port){&c->port,sizeof(c->port),1,31,ct_clock,ct_publish,ss_commit,
        ss_command_quiet,ss_reader_quiet,ct_source_close,ct_source_quiet,ct_publish_successor};
    c->workspace=calloc(1,pt_mixed_causal_workspace_size());assert(c->workspace);
    assert(pt_mixed_causal_open(&c->config,c->workspace,pt_mixed_causal_workspace_size(),&c->owner)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_borrow_queue(c->owner,&c->trial->queue)==PT_MIXED_READERS_OK&&c->memory.live==2);
    c->port.base.registration=(struct pt_mixed_causal_registration){c->owner,c->trial->queue,19,17};
    port=(struct pt_mixed_causal_stop_port){&c->port,sizeof(c->port),1,3,ss_publish_stop};
    if(bind)assert(pt_mixed_causal_stop_bind(c->owner,&port)==PT_MIXED_READERS_OK);
    return c;
}
static uint64_t ss_first(struct cc_case *c,unsigned all_card)
{
    uint64_t first=0;unsigned i;trigger_input(c->trial,16,0,960);
    if(all_card)for(i=0;i<4;++i){struct holder *h=c->trial->reader+i;
        struct pt_amigus_voice_request request={8000,1,0,64,128};struct pt_playback_format format={(uint8_t)c->trial->resources->cache_bits,0,0,0};
        struct pt_mixed_readers_action *a=c->trial->input->batch.action+i;
        held_terminal(h,h->token,1);held_release(h,h->token);
        memset(a,0,sizeof(*a));a->route=2;a->slot=12+i;a->kind=PT_MIXED_READERS_TRIGGER;
        c->trial->input->reader[i]=source(h,c->trial->resources,2,12+i,i%2,1000+i);h->queue=c->trial->queue;
        assert(pt_amigus_voice_plan_prepare(c->trial->resources->document.project.samples+i%2,&format,&request,
            c->trial->input->reader[i].card.address,c->trial->input->reader[i].card.logical_bytes,&a->geometry.amigus));}
    assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&first)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_publish(c->owner,first)==PT_MIXED_READERS_OK);return first;
}
static struct pt_mixed_causal_stop_request ss_request(struct cc_case *c,uint64_t first,unsigned count,unsigned all_card)
{
    struct pt_mixed_causal_stop_request r;unsigned i;memset(&r,0,sizeof(r));
    holder_init(c->trial->command+1,101);c->trial->command[1].queue=c->trial->queue;
    r.predecessor=first;r.frame=1920;r.count=count;r.command=control_of(c->trial->command+1);
    for(i=0;i<count;++i){r.action[i].route=all_card?2:i<4?1:2;r.action[i].slot=i<4?(all_card?12+i:i):i-4;}
    return r;
}
static void ss_success(unsigned bits,unsigned cache_bits,unsigned mode,unsigned all_card)
{
    struct cc_case *c=ss_make(bits,cache_bits,1);uint64_t first=ss_first(c,all_card),second=0,out=777;
    struct pt_mixed_causal_stop_request r=ss_request(c,first,mode==2?1:16,all_card),saved;
    struct pt_mixed_readers_key keys[16],key;struct pt_cache_lease leases[16];struct pt_sample_version *pins[16];
    struct ct_reader readers[32];struct pt_mixed_readers_command_receipt receipt;
    struct pt_mixed_readers_reader_receipt reader_receipt;
    unsigned i,writes,chips,calls,mask=c->port.base.mask;
    memcpy(&saved,&r,sizeof(saved));
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    cc_actual_first(c,first);mask=c->port.base.mask;memcpy(readers,c->port.base.reader,sizeof(readers));
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&out)!=PT_MIXED_READERS_OK&&out==777);
    if(mode==2)assert(pt_mixed_causal_service_reader(c->owner,first,0,0,NULL)==PT_MIXED_READERS_PENDING);
    else{c->port.hold_first=mode==1;assert(pt_mixed_causal_service_command(c->owner,first,0,&receipt)==
        (mode==1?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_OK));}
    if(mode==0)CT_POISON(c->trial->command,sizeof(c->trial->command[0]));
    for(i=0;i<16;++i){keys[i]=readers[i].key;leases[i]=c->trial->reader[i].lease;pins[i]=c->trial->reader[i].pin;}
    writes=c->trial->resources->card.writes;chips=c->trial->resources->chips;calls=c->memory.calls;
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&second)==PT_MIXED_READERS_OK&&second!=first&&second);
    assert(!memcmp(&r,&saved,sizeof(r))&&pt_mixed_readers_readers_held(c->trial->queue)==16);
    assert(c->port.base.mask==mask&&!c->port.stops&&!memcmp(readers,c->port.base.reader,sizeof(readers)));
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    assert(pt_mixed_causal_control_publish(c->owner,second)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_publish_successor(c->owner,first,second)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_stop_publish(c->owner,second)==PT_MIXED_READERS_OK&&c->port.stop_publications==1);
    assert(!c->port.stops&&c->port.base.mask==mask&&!memcmp(readers,c->port.base.reader,sizeof(readers)));
    c->port.base.ticks=ct_command(&c->port.base,second)->packet.first-1;
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_EARLY&&!c->port.stops);
    c->port.base.ticks++;
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_COMMITTED&&c->port.stops==r.count&&c->port.base.effects==16);
    for(i=0;i<16;++i){unsigned index=ct_index(keys[i].route,keys[i].slot);struct ct_reader *held=ct_reader(&c->port.base,keys+i);
        assert(held&&!memcmp(held,readers+i,sizeof(*held))&&pins[i]==c->trial->reader[i].pin&&
            !memcmp(leases+i,&c->trial->reader[i].lease,sizeof(leases[i]))&&c->trial->reader[i].live);
        if(i<r.count)assert(!(c->port.base.mask&(1U<<index))&&!c->port.base.slot[index].queue);
        else assert((c->port.base.mask&(1U<<index))&&keys_equal(keys+i,c->port.base.slot+index));
        assert(pt_mixed_causal_service_reader(c->owner,first,i,0,&reader_receipt)==PT_MIXED_READERS_PENDING);
        assert(reader_receipt.observed==c->port.observed&&reader_receipt.issued==c->port.issued&&
            reader_receipt.state==(i<r.count?PT_MIXED_READER_DRAINING:PT_MIXED_READER_ACTIVE));}
    assert(calls==c->memory.calls&&writes==c->trial->resources->card.writes&&chips==c->trial->resources->chips);
    same_save(c->trial->resources,c->before,c->before_bytes);
    c->port.hold_stop=mode==1;
    assert(pt_mixed_causal_service_command(c->owner,second,0,&receipt)==(mode==1?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_OK));
    assert(receipt.action[0].reader==PT_MIXED_READER_DRAINING&&receipt.action[0].observed==oracle(1920)&&receipt.action[0].issued==oracle(1920));
    if(mode!=1)CT_POISON(c->trial->command+1,sizeof(c->trial->command[1]));
    c->port.quiet_mask=c->port.stopped_mask;
    for(i=0;i<r.count;++i){assert(pt_mixed_causal_service_reader(c->owner,first,i,0,NULL)==PT_MIXED_READERS_OK);
        assert(c->trial->reader[i].live==(mode!=0)&&c->trial->reader[i].releases==(mode==0));}
    if(mode==1){assert(c->trial->command[1].live);c->port.hold_stop=0;
        assert(pt_mixed_causal_service_command(c->owner,second,0,NULL)==PT_MIXED_READERS_OK);}
    else CT_UNPOISON(c->trial->command+1,sizeof(c->trial->command[1]));
    if(mode==2)assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    if(mode==1){c->port.hold_first=0;assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);}
    for(i=0;i<r.count;++i)assert(!c->trial->reader[i].live&&c->trial->reader[i].releases==1);
    for(i=r.count;i<16;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    assert(pt_mixed_causal_reader_key(c->owner,first,0,&key)!=PT_MIXED_READERS_OK);
    if(mode==0)CT_UNPOISON(c->trial->command,sizeof(c->trial->command[0]));
    cc_drop(c,mode==1);
}
static void ss_guards(void)
{
    struct cc_case *c=ss_make(24,16,0);struct pt_mixed_causal_stop_port p={&c->port,sizeof(c->port),1,3,ss_publish_stop},bad;
    struct pt_mixed_causal_control_port cp={&c->port,sizeof(c->port),1,3,cc_publish_control};
    struct pt_mixed_causal_stop_request r,saved;struct pt_mixed_causal_diagnostic before,after;
    uint64_t first,out=777;unsigned i;
    bad=p;bad.version=2;assert(pt_mixed_causal_stop_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    bad=p;bad.context_bytes--;assert(pt_mixed_causal_stop_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    bad=p;bad.publish_stop=NULL;assert(pt_mixed_causal_stop_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    bad=p;bad.flags=1;assert(pt_mixed_causal_stop_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_stop_bind(c->owner,&p)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_stop_bind(c->owner,&p)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_control_bind(c->owner,&cp)==PT_MIXED_READERS_INVALID);
    first=cc_first(c,16);cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=ss_request(c,first,2,0);memcpy(&saved,&r,sizeof(saved));assert(pt_mixed_causal_diagnostic(c->owner,&before));
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&r.frame)==PT_MIXED_READERS_INVALID&&!memcmp(&r,&saved,sizeof(r)));
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&c->trial->reader[0].token)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,(void *)c->trial->reader[0].spans[0].data)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,(void *)((uint8_t *)&c->port+sizeof(c->port)-8))==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_stop_enqueue(c->owner,(void *)&c->trial->reader[0],&out)==PT_MIXED_READERS_INVALID&&out==777);
    for(i=0;i<6;++i){memcpy(&r,&saved,sizeof(r));if(i==0)r.action[1]=r.action[0];if(i==1)r.action[0].slot=4;
        if(i==2)r.action[15].route=1;if(i==3)r.frame=960;if(i==4)r.predecessor++;if(i==5)r.action[0].route=3;
        assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);}
    memcpy(&r,&saved,sizeof(r));r.command.context=&r;r.command.context_bytes=sizeof(r);
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    assert(pt_mixed_causal_diagnostic(c->owner,&after)&&!memcmp(&before,&after,sizeof(before)));
    cc_unused_command(c);cc_retire(c,first,0,16,1,0);cc_drop(c,0);
}
static void ss_control_refusal(void)
{
    struct cc_case *c=cc_make(16,8,1);struct pt_mixed_causal_stop_port p={&c->port,sizeof(c->port),1,3,ss_publish_stop};
    uint64_t first,out=777;struct pt_mixed_causal_stop_request r;++ss_cases;
    assert(pt_mixed_causal_stop_bind(c->owner,&p)==PT_MIXED_READERS_INVALID);
    first=cc_first(c,6);cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=ss_request(c,first,6,0);assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    cc_unused_command(c);cc_retire(c,first,0,6,1,0);cc_drop(c,0);
}
static void ss_failure(unsigned mode)
{
    struct cc_case *c=ss_make(24,16,1);uint64_t first=cc_first(c,6),second=0;struct pt_mixed_causal_stop_request r;
    struct pt_mixed_causal_diagnostic d;unsigned i;
    cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=ss_request(c,first,6,0);
    if(mode==7||mode==8){r.command.current=ss_current;if(mode==7)cc_reentry_owner=c->owner;else ss_mutate_request=&r;}
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&second)==PT_MIXED_READERS_OK&&second&&c->trial->command[1].live);
    if(mode>=7){assert(pt_mixed_causal_diagnostic(c->owner,&d)&&d.suppressed);
        assert(pt_mixed_causal_stop_publish(c->owner,second)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_causal_stop(c->owner)==PT_MIXED_READERS_BACKEND&&!c->trial->command[1].live);
        assert(!c->port.stops&&c->port.base.mask==0x3FU);
        for(i=0;i<6;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_BACKEND);
        cc_drop(c,0);return;}
    assert(pt_mixed_causal_stop_publish(c->owner,second)==PT_MIXED_READERS_OK);
    c->port.base.ticks=ct_command(&c->port.base,second)->packet.first;
    if(mode==0)c->port.base.commit_raw=-1;if(mode==1)c->port.base.commit_raw=0;
    if(mode==2)c->port.base.malformed=1;if(mode==3)c->port.base.bad_adoption=1;
    if(mode==4)c->port.base.late=1;if(mode==5)c->port.base.reenter=1;
    if(mode==6)c->port.base.ticks=ct_command(&c->port.base,second)->packet.last;
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_FAILED);
    assert(pt_mixed_causal_diagnostic(c->owner,&d)&&d.suppressed);
    for(i=0;i<6;++i)assert(c->trial->reader[i].live&&c->trial->reader[i].pin&&!c->trial->reader[i].releases);
    assert(c->port.stops==(mode==0?1:mode==1||mode==6?0:6));
    if(c->port.stops)assert(pt_mixed_causal_service_reader(c->owner,first,0,1,NULL)==PT_MIXED_READERS_BACKEND&&c->trial->reader[0].live);
    /* Separate model device quiet permits later explicit proof; failed outcome
     * remains BACKEND even though true proof can safely drain exact ownership. */
    c->port.quiet_mask=c->port.stopped_mask;cc_retire(c,first,second,6,1,1);cc_drop(c,0);
}
static void ss_publication_failure(unsigned mode)
{
    struct cc_case *c=ss_make(8,16,1);uint64_t first=cc_first(c,6),second=0;struct pt_mixed_causal_stop_request r;
    cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=ss_request(c,first,6,0);assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&second)==PT_MIXED_READERS_OK);
    c->port.publication_raw=mode==0?0:-1;if(mode==1){c->port.publication_raw=0;c->port.publication_reentry=1;}
    if(mode==2){c->port.publication_raw=1;c->port.publication_mutation=1;}
    assert(pt_mixed_causal_stop_publish(c->owner,second)==(mode==0?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_BACKEND));
    assert(!c->port.stops&&c->port.base.mask==0x3FU&&c->trial->reader[0].live);
    if(mode==0){unsigned i;struct pt_mixed_causal_diagnostic d;
        assert(pt_mixed_causal_diagnostic(c->owner,&d)&&!d.suppressed&&!d.published);
        assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_causal_stop(c->owner)==PT_MIXED_READERS_PENDING&&!c->trial->command[1].live);
        for(i=0;i<6;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);
    }else{unsigned i;
        assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==PT_MIXED_READERS_BACKEND);
        assert(!c->trial->command[1].live&&c->port.base.mask==0x3FU&&!c->port.stops);
        for(i=0;i<6;++i)assert(c->trial->reader[i].live&&c->trial->reader[i].pin&&!c->trial->reader[i].releases);
        same_save(c->trial->resources,c->before,c->before_bytes);
        for(i=0;i<6;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_BACKEND);
    }
    cc_drop(c,mode==3);
}
static void ss_expected_only(unsigned unknown,unsigned selected)
{
    struct cc_case *c=ss_make(16,8,1);uint64_t first=cc_first(c,6),second=0;struct pt_mixed_causal_stop_request r;
    unsigned i,victim=selected?0:5;cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=ss_request(c,first,1,0);assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&second)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_stop_publish(c->owner,second)==PT_MIXED_READERS_OK);
    if(unknown){c->port.base.unknown_ticket=second;
        assert(pt_mixed_causal_service_reader(c->owner,first,victim,1,NULL)==PT_MIXED_READERS_BACKEND);
        assert(c->trial->reader[victim].live&&c->trial->reader[victim].pin&&!c->trial->reader[victim].releases);c->port.base.unknown_ticket=0;}
    assert(pt_mixed_causal_service_reader(c->owner,first,victim,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    assert(ct_command(&c->port.base,second)->disabled&&c->trial->reader[victim].live==selected);
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_INVALID&&!c->port.stops);
    assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    assert(!c->trial->reader[victim].live&&c->trial->reader[victim].releases==1);
    for(i=0;i<6;++i)if(i!=victim)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    cc_drop(c,0);
}
static void ss_port_registry_refusal(unsigned mode)
{
    struct cc_case *c=ss_make(24,8,1);uint64_t first=cc_first(c,6),second=0;struct pt_mixed_causal_stop_request r;unsigned i;
    cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=ss_request(c,first,1,0);assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&second)==PT_MIXED_READERS_OK);
    if(mode==0)c->port.first_completed=0;else c->port.base.slot[19].serial=999;
    assert(pt_mixed_causal_stop_publish(c->owner,second)==PT_MIXED_READERS_PENDING&&c->port.stop_publications==1);
    assert(!c->port.stops&&!ct_command(&c->port.base,second)&&c->trial->command[1].live);
    c->port.first_completed=1;memset(c->port.base.slot+19,0,sizeof(c->port.base.slot[19]));
    assert(pt_mixed_causal_stop(c->owner)==PT_MIXED_READERS_PENDING&&!c->trial->command[1].live);
    for(i=0;i<6;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);cc_drop(c,0);
}
static void ss_late_publication(void)
{
    struct cc_case *c=ss_make(24,16,1);uint64_t first=cc_first(c,6),out=777;struct pt_mixed_causal_stop_request r;
    cc_actual_first(c,first);assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    r=ss_request(c,first,6,0);c->port.base.ticks=oracle(r.frame);
    /* Admission is clock-free; its genuine transfer remains authoritative.
     * Original deadline is enforced at publication, without time rebasing. */
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_OK&&out!=777&&c->trial->command[1].live);
    assert(pt_mixed_causal_stop_publish(c->owner,out)==PT_MIXED_READERS_LATE&&!c->port.stop_publications);
    assert(r.frame==1920&&!c->port.stops&&c->trial->command[1].live);
    assert(pt_mixed_causal_stop(c->owner)==PT_MIXED_READERS_BACKEND&&!c->trial->command[1].live);
    cc_retire(c,first,0,6,1,1);cc_drop(c,0);
}
int main(void)
{
    unsigned bits,cache_bits,mode;
    for(bits=8;bits<=24;bits+=8)for(cache_bits=8;cache_bits<=16;cache_bits+=8){
        for(mode=0;mode<3;++mode)ss_success(bits,cache_bits,mode,0);ss_success(bits,cache_bits,0,1);}
    ss_guards();ss_control_refusal();for(mode=0;mode<9;++mode)ss_failure(mode);
    for(mode=0;mode<4;++mode)ss_publication_failure(mode);
    ss_expected_only(0,0);ss_expected_only(1,0);ss_expected_only(0,1);ss_port_registry_refusal(0);ss_port_registry_refusal(1);ss_late_publication();
    assert(ss_cases==45);
    puts("MIXED CAUSAL AFTER FIRST STOP HOST PASS:45 genuine heap cases;24 mixed4/12 subset or16card 8/16/24-master cache8/16 lifetimes;distinct empty-owner capability;actual completed first and ACTIVE C/R observations;poisoned disposed C;zero new reader/cache/upload;selected slots clear and untargeted unchanged;independent DRAINING R/C/source quiet;exact EARLY/original frames;expected-only pending C;partial/zero/malformed/late/reentry/uncertain retention;actual enqueue transfer;master beforeimages;NO_NATIVE_APERTURE_OR_DEVICE_STOP_QUALIFICATION");
    return 0;
}
