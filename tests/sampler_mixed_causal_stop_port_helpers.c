/* Exact unchanged port-model function bodies from STOP45 fixture. No fixture
 * entry copied/called; these are ordinary RAM callbacks, not device evidence. */
#define PT_MIXED_CAUSAL_TEST_MAIN sf_inherited_default_suite_not_called
#include "mixed_readers_causal_test.c"
#undef PT_MIXED_CAUSAL_TEST_MAIN
#include "../src/core/mixed_readers_causal_control_internal.h"
#include "../src/core/mixed_readers_causal_stop_internal.h"
static unsigned ss_cases;
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
static void cc_actual_first(struct cc_case *c,uint64_t first)
{c->port.base.ticks=ct_command(&c->port.base,first)->packet.first;
 assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED);}
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
