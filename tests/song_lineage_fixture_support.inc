/* Ordinary-RAM model copied from reviewed original STOP helpers only.
 * Original producer62 and underlying43/core donor entries are UNCALLED. */
#define PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_TEST_MAIN sl_original_producer62_not_called
#include "editor_mixed_causal_song_pair_test.c"
#undef PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_TEST_MAIN
#include "../src/editor/editor_mixed_causal_lineage_prepare_internal.h"
struct sl_cc_port {
    struct ct_port base;
    struct pt_mixed_causal_command_identity first_identity;
    struct pt_mixed_causal_actual first_post;
    uint64_t first_tick,last_tick,observed,issued;
    unsigned first_completed,hold_first,controls,control_publications;
    int publication_raw;
    unsigned publication_reentry,publication_mutation;
    unsigned stops,stop_publications,stopped_mask,quiet_mask,hold_stop;
};
static int sl_cc_identity(const struct pt_mixed_causal_command_identity *a,const struct pt_mixed_causal_command_identity *b)
{return ct_registration(&a->registration,&b->registration)&&a->ticket==b->ticket&&a->owner==b->owner&&
    a->event==b->event&&a->binding.context==b->binding.context&&a->binding.context_bytes==b->binding.context_bytes;}
static int sl_cc_commit(void *context,const struct pt_mixed_causal_packet *b,struct pt_mixed_causal_actual *actual)
{
    struct sl_cc_port *p=context;struct ct_port *base=&p->base;struct ct_command *c=ct_command(base,b->ticket);
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
static int sl_cc_command_quiet(void *context,const struct pt_mixed_causal_command_identity *id,unsigned cancel)
{
    struct sl_cc_port *p=context;
    if(p->hold_first&&!cancel&&id->ticket==p->first_identity.ticket){++p->base.command_proofs;return 0;}
    return ct_command_quiet(&p->base,id,cancel);
}
static int sl_ss_commit(void *context,const struct pt_mixed_causal_packet *b,struct pt_mixed_causal_actual *actual)
{
    struct sl_cc_port *p=context;struct ct_port *base=&p->base;struct ct_command *c=ct_command(base,b->ticket);
    unsigned i;int raw;
    if(b->action[0].kind==PT_MIXED_READERS_TRIGGER)return sl_cc_commit(context,b,actual);
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
static int sl_ss_reader_quiet(void *context,const struct pt_mixed_causal_reader_identity *id,unsigned cancel)
{
    struct sl_cc_port *p=context;unsigned bit=1U<<ct_index(id->key.route,id->key.slot);
    if((p->stopped_mask&bit)&&!(p->quiet_mask&bit)){++p->base.reader_proofs;return 0;}
    if((p->stopped_mask&bit)&&(p->quiet_mask&bit))return ct_reader_quiet(context,id,1);
    return ct_reader_quiet(context,id,cancel);
}
