/* Private SOURCE proposal: Root alone compiles/runs. The focused entry uses
 * genuine project/editor/sampler/factory/causal owner/queue production units.
 * Existing causal/resource test helpers are included; both old entries are
 * renamed and UNCALLED. No activation_test constructor or mirrored core body.
 * ct_port is an ordinary-RAM adapter model, not a deadline/device backend. */
#define PT_MIXED_CAUSAL_TEST_MAIN inherited_controller_stop_resource_suite_not_called
#include "mixed_readers_causal_test.c"
#undef PT_MIXED_CAUSAL_TEST_MAIN
#include <stddef.h>
#include "../src/editor/editor_mixed_causal_stop_prepare_internal.h"
#include "../src/editor/editor_mixed_causal_source_internal.h"
#include "../src/core/mixed_readers_causal_factory_internal.h"

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
static int cc_identity(const struct pt_mixed_causal_command_identity *a,const struct pt_mixed_causal_command_identity *b)
{return ct_registration(&a->registration,&b->registration)&&a->ticket==b->ticket&&a->owner==b->owner&&
    a->event==b->event&&a->binding.context==b->binding.context&&a->binding.context_bytes==b->binding.context_bytes;}
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

/* Test-only layout observation of a genuine captured local extent. This is
 * neither a shadow controller/backend implementation nor a product test seam.
 * We inspect it only during the allocator callback while its lifetime is live. */
struct cs_stop_pair_layout {unsigned index;struct pt_sampler_storage_span input,output;};
struct cs_stop_scratch_layout {
    struct pt_editor_mixed_causal_stop_batch saved;
    struct pt_sampler_mixed_causal_stop_batch lower;
    struct pt_sampler_mixed_command_handle handle;
    struct cs_stop_pair_layout pair;
};
struct cs_trial;
struct cs_bus {struct cs_trial *owner;};
struct cs_ledger {void *p;size_t n;};
struct cs_memory {
    struct cs_trial *owner;
    unsigned calls,releases,refusals,fail,hook,alias_releases;
    void *alias;
    struct cs_ledger live[80];
};
struct cs_port {
    struct cs_trial *owner;
    struct cc_port model;
    unsigned bind_calls,bind_mode,task_hook,quiet_hook;
    int bind_raw,quiet_raw;
};
struct cs_trial {
    struct pt_editor_mixed_causal_stop_prepare control;
    struct pt_editor_mixed_causal_stop_inputs input;
    struct cs_memory ordinary,masters,chip;
    struct cs_port port;
    struct cs_bus bus;
    struct fake library;
    struct fixture *card;
    struct pt_document *document;
    struct pt_editor *editor;
    struct pt_editor_mixed *binding;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_pcm pcm[PT_PROJECT_SAMPLES];
    struct pt_editor_mixed_readers_request *request;
    struct pt_editor_mixed_command_ref *command;
    struct pt_editor_mixed_reader_ref *reader;
    struct pt_editor_mixed_causal_stop_batch *stop;
    int32_t original[2][64],borrowed_before[2][64],master_before[2][64];
    void *causal_workspace,*factory_workspace;
    size_t causal_capacity,factory_capacity;
    uint8_t *saved;size_t saved_bytes;
    unsigned bits,cache_bits,little,count,owned_hook,owned_after,owned_calls,write_hook,reentered,stop_alloc_hook;
    unsigned scratch_alias_mode,scratch_alias_hits,scratch_guard_before;
    struct pt_editor_mixed_command_ref *scratch_output;
    void *scratch_live_address,*scratch_returned_alias;
    size_t scratch_live_bytes,scratch_requested_bytes;
    struct cs_stop_scratch_layout scratch_immediate_before;
    struct pt_editor_mixed_command_ref expected_unpublished_ref;
    uint64_t expected_unpublished_ticket;
};
static unsigned cs_cases;
static void cs_reenter(struct cs_trial *f)
{
    uint32_t revision=f->editor->history.revision,generation=f->editor->sampler.generation;
    enum pt_editor_mixed_readers_result before=f->control.original.first_error;
    assert(pt_editor_mixed_causal_prepare_get(&f->control.original)==(before?before:PT_EDITOR_MIXED_READERS_FAULT));
    assert(!pt_editor_mixed_causal_prepare_close(&f->control.original));
    assert(!pt_editor_prepare_change(f->editor)&&!pt_editor_dispose(f->editor));
    assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation);
    ++f->reentered;
}
static void *cs_new(struct cs_memory *m,size_t n)
{
    void *p;unsigned i;++m->calls;if(m->fail==m->calls){++m->refusals;return NULL;}
    if(m->alias){++m->refusals;return m->alias;}
    p=malloc(n);assert(p);
    for(i=0;i<80&&m->live[i].p;++i){}assert(i<80);
    m->live[i]=(struct cs_ledger){p,n};return p;
}
static void cs_free(struct cs_memory *m,void *p,size_t n,unsigned exact)
{
    unsigned i;if(p==m->alias){++m->alias_releases;return;}
    for(i=0;i<80&&m->live[i].p!=p;++i){}assert(i<80&&(!exact||m->live[i].n==n));
    m->live[i]=(struct cs_ledger){NULL,0};++m->releases;free(p);
}
static unsigned cs_live(const struct cs_memory *m)
{unsigned i,n=0;for(i=0;i<80;++i)if(m->live[i].p){assert(m->live[i].n);++n;}return n;}
static int cs_numeric_apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t av=(uintptr_t)a,bv=(uintptr_t)b;
    assert(an<=UINTPTR_MAX-av&&bn<=UINTPTR_MAX-bv);
    return av+an<=bv||bv+bn<=av;
}
static void *cs_scratch_alias(struct cs_trial *f,size_t n)
{
    struct pt_editor_mixed_causal_prepare *s=&f->control.original;
    struct pt_sampler_storage_span whole;unsigned i=f->scratch_guard_before;
    const unsigned char *base;size_t offset;void *alias;
    assert((f->scratch_alias_mode==1||f->scratch_alias_mode==2)&&s->busy&&
        s->guard_count==i+3&&i<=PT_EDITOR_MIXED_READERS_GUARDS-3);
    assert(s->guards[i].data==f->stop&&s->guards[i].bytes==sizeof(*f->stop));
    assert(s->guards[i+1].data==f->scratch_output&&s->guards[i+1].bytes==sizeof(*f->scratch_output));
    whole=s->guards[i+2];assert(whole.data&&whole.bytes==sizeof(struct cs_stop_scratch_layout));
    /* The complete scratch was zero-initialized and filled before the actual
     * allocator callback; this immediate live beforeimage is legal to read. */
    memcpy(&f->scratch_immediate_before,whole.data,sizeof(f->scratch_immediate_before));
    assert(!memcmp(&f->scratch_immediate_before.saved,f->stop,sizeof(*f->stop)));
    assert(f->scratch_immediate_before.pair.index==i&&
        f->scratch_immediate_before.pair.input.data==f->stop&&
        f->scratch_immediate_before.pair.input.bytes==sizeof(*f->stop)&&
        f->scratch_immediate_before.pair.output.data==f->scratch_output&&
        f->scratch_immediate_before.pair.output.bytes==sizeof(*f->scratch_output)&&
        !f->scratch_immediate_before.handle.address&&!f->scratch_immediate_before.handle.token);
    base=whole.data;
    offset=f->scratch_alias_mode==1?
        offsetof(struct cs_stop_scratch_layout,saved)+offsetof(struct pt_editor_mixed_causal_stop_batch,reader):
        offsetof(struct cs_stop_scratch_layout,pair)+offsetof(struct cs_stop_pair_layout,output);
    assert(offset<whole.bytes);alias=(void *)(base+offset);
    /* Actual requested allocation extent is disjoint from every earlier
     * controller guard, including both original external spans. It overlaps
     * the newly installed third whole local guard itself. */
    assert(cs_numeric_apart(alias,n,s,sizeof(*s)));
    for(i=0;i<s->guard_count-1;++i)assert(cs_numeric_apart(alias,n,s->guards[i].data,s->guards[i].bytes));
    for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)
        assert(cs_numeric_apart(alias,n,s->ordinary[i].data,s->ordinary[i].bytes));
    for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP;++i)
        assert(cs_numeric_apart(alias,n,s->chip[i].data,s->chip[i].bytes));
    assert(!cs_numeric_apart(alias,n,whole.data,whole.bytes));
    f->scratch_live_address=(void *)whole.data;f->scratch_live_bytes=whole.bytes;
    f->scratch_requested_bytes=n;f->scratch_returned_alias=alias;++f->scratch_alias_hits;
    f->scratch_alias_mode=0;return alias;
}
static void *cs_allocate(void *context,size_t n)
{
    struct cs_memory *m=context;struct cs_trial *f=m->owner;void *p;
    if(m->hook==m->calls+1){m->hook=0;cs_reenter(f);}
    if(f->stop_alloc_hook){f->stop_alloc_hook=0;++f->stop->frame;}
    assert(f->binding->preparation_context==&f->control.original&&f->binding->preparation_close);
    if(f->scratch_alias_mode){assert(m==&f->ordinary);m->alias=cs_scratch_alias(f,n);}
    p=cs_new(m,n);return p;
}
static void cs_release(void *context,void *p)
{
    struct cs_memory *m=context;struct cs_trial *f=m->owner;unsigned i;
    for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)assert(f->control.original.ordinary[i].data!=p);
    if(m->hook){m->hook=0;cs_reenter(f);}cs_free(m,p,0,0);
}
static void *cs_master_new(void *context,size_t n){return cs_new(context,n);}
static void cs_master_free(void *context,void *p){cs_free(context,p,0,0);}
static void *cs_chip_new(void *context,size_t n)
{struct cs_memory *m=context;if(m->hook){m->hook=0;cs_reenter(m->owner);}return cs_new(m,n);}
static void cs_chip_free(void *context,void *p,size_t n)
{struct cs_memory *m=context;unsigned i;for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP;++i)assert(m->owner->control.original.chip[i].data!=p);
 if(m->hook){m->hook=0;cs_reenter(m->owner);}cs_free(m,p,n,1);}
static int cs_owned(void *context)
{
    struct cs_bus *bus=context;struct cs_trial *f=bus->owner;int r=f->card->healthy==1&&f->library.library&&
        f->library.owner==&f->card->reservation&&f->library.acquired==PT_AMIGUS_WAVETABLE&&f->card->reservation.reserved;
    ++f->owned_calls;
    if(f->owned_hook&&(!f->owned_after||!--f->owned_after)){f->owned_hook=0;cs_reenter(f);}return r;
}
static int cs_write(void *context,unsigned reg,uint32_t value)
{
    struct cs_bus *bus=context;struct cs_trial *f=bus->owner;struct fixture *c=f->card;unsigned i;
    assert(cs_owned(context)&&c->reservation.access);++c->writes;
    if(reg==0x14)c->address=value;
    else{assert(reg==0x10&&!(c->address&3)&&c->address<=sizeof(c->ram)-4);
        for(i=0;i<4;++i)c->ram[c->address+i]=(uint8_t)(value>>(24-8*i));}
    if(f->write_hook){f->write_hook=0;cs_reenter(f);}return 1;
}
static int cs_bind(void *context,const struct pt_mixed_causal_registration *registration)
{
    struct cs_port *p=context;struct cs_trial *f=p->owner;
    struct pt_mixed_causal_owner *owner=f->control.original.causal;struct pt_mixed_readers_output *queue=NULL;
    assert(++p->bind_calls==1&&f->ordinary.calls==2&&cs_live(&f->ordinary)==2);
    assert(!f->control.original.pool&&!f->chip.calls&&!f->card->writes&&!p->model.base.clocks&&!p->model.base.publications);
    assert(owner&&f->control.original.queue&&registration->owner==owner&&registration->queue==f->control.original.queue);
    assert(registration->session==31&&registration->generation==17);
    assert(!pt_mixed_readers_commands_held(f->control.original.queue)&&!pt_mixed_readers_readers_held(f->control.original.queue));
    memcpy(&p->model.base.registration,registration,sizeof(*registration));
    if(p->bind_mode==1)cs_reenter(f);
    if(p->bind_mode==2){assert(!pt_mixed_causal_close(&owner)&&owner==f->control.original.causal);}
    if(p->bind_mode==3)++f->input.original.causal.session;
    if(p->bind_mode==4){assert(pt_mixed_causal_borrow_queue(owner,&queue)==PT_MIXED_READERS_BACKEND&&!queue);}
    return p->bind_raw;
}
static int cs_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct cs_port *p=context;return ct_clock(&p->model.base,ticks,frequency);}
static int cs_publish(void *context,struct pt_mixed_causal_owner *owner,
 const struct pt_mixed_causal_command_identity *identity,const struct pt_mixed_causal_packet *packet)
{struct cs_port *p=context;int r=ct_publish(&p->model.base,owner,identity,packet);
 if(p->task_hook){p->task_hook=0;cs_reenter(p->owner);}return r;}
static int cs_publish_successor(void *context,struct pt_mixed_causal_owner *owner,const struct pt_mixed_causal_publication *publication)
{struct cs_port *p=context;int r=ct_publish_successor(&p->model.base,owner,publication);
 if(p->task_hook){p->task_hook=0;cs_reenter(p->owner);}return r;}
static int cs_publish_stop(void *context,struct pt_mixed_causal_owner *owner,
 const struct pt_mixed_causal_stop_publication *publication)
{struct cs_port *p=context;int r=ss_publish_stop(&p->model,owner,publication);
 if(p->task_hook){p->task_hook=0;cs_reenter(p->owner);}return r;}
static int cs_commit(void *context,const struct pt_mixed_causal_packet *packet,struct pt_mixed_causal_actual *actual)
{struct cs_port *p=context;assert(!p->model.base.reenter);return ss_commit(&p->model,packet,actual);}
static int cs_command_quiet(void *context,const struct pt_mixed_causal_command_identity *identity,unsigned cancel)
{struct cs_port *p=context;return ss_command_quiet(&p->model,identity,cancel);}
static int cs_reader_quiet(void *context,const struct pt_mixed_causal_reader_identity *identity,unsigned cancel)
{struct cs_port *p=context;return ss_reader_quiet(&p->model,identity,cancel);}
static int cs_source_close(void *context,const struct pt_mixed_causal_registration *registration)
{struct cs_port *p=context;int r=ct_source_close(&p->model.base,registration);
 if(p->quiet_hook){p->quiet_hook=0;cs_reenter(p->owner);}return r;}
static int cs_source_quiet(void *context,const struct pt_mixed_causal_registration *registration)
{struct cs_port *p=context;
 if(p->quiet_raw!=1){++p->model.base.probes;assert(ct_registration(&p->model.base.registration,registration)&&ct_empty(&p->model.base));return p->quiet_raw;}
 return ct_source_quiet(&p->model.base,registration);}
static uint8_t *cs_save(struct cs_trial *f,size_t *n)
{
    size_t used;uint8_t *p;
    assert(pt_project_size(f->editor->project,n)==PT_PROJECT_OK);p=malloc(*n);assert(p);
    assert(pt_project_encode(f->editor->project,p,*n,&used)==PT_PROJECT_OK&&used==*n);return p;
}
static void cs_same(struct cs_trial *f)
{
    size_t n;uint8_t *p;unsigned i;
    f->editor->project->channels.selected=0;p=cs_save(f,&n);assert(n==f->saved_bytes&&!memcmp(p,f->saved,n));free(p);
    for(i=0;i<2;++i){const struct pt_pcm *actual=&f->editor->project->samples[i].pcm;
        const struct pt_pcm *pinned=f->pcm+i;
        assert(actual->bits==f->bits&&actual->data==pinned->data&&actual->capacity==pinned->capacity&&
            actual->frames==pinned->frames&&actual->channels==pinned->channels&&actual->rate==pinned->rate);
        assert(actual->capacity<=64);
        assert(!memcmp(actual->data,f->master_before[i],actual->capacity*sizeof(int32_t)));
        assert(!memcmp(f->original[i],f->borrowed_before[i],sizeof(f->original[i])));}
    for(i=0;i<64;++i)assert(f->pcm[31].data[i]==(int32_t)i);
}
static struct cs_trial *cs_make(unsigned bits,unsigned cache_bits,unsigned little)
{
    struct cs_trial *f=calloc(1,sizeof(*f));struct pt_allocator a;struct pt_amigus_reservation_api api;
    struct pt_project *p;unsigned i,j;assert(f);++cs_cases;f->bits=bits;f->cache_bits=cache_bits;f->little=little;
    f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
    f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
    f->request=calloc(16,sizeof(*f->request));f->command=calloc(2,sizeof(*f->command));f->reader=calloc(32,sizeof(*f->reader));
    f->stop=calloc(1,sizeof(*f->stop));
    assert(f->card&&f->document&&f->editor&&f->binding&&f->request&&f->command&&f->reader&&f->stop);f->bus.owner=f;f->ordinary.owner=f;f->masters.owner=f;f->chip.owner=f;f->port.owner=f;f->port.bind_raw=1;f->port.quiet_raw=1;
    a=(struct pt_allocator){&f->masters,cs_master_new,cs_master_free};
    pt_document_init(f->document,&a);assert(pt_document_new(f->document,16,SIZE_MAX)==PT_PROJECT_OK);
    p=&f->document->project;
    for(i=0;i<16;++i)p->channels.track[i].route=i<4?PT_PAULA:PT_AMIGUS;
    for(i=0;i<2;++i){int32_t values[]={127,-128,1,-1,3,-3,64,-64,0,2,-2,126};
        for(j=0;j<64;++j)f->original[i][j]=values[j%12]*(int32_t)(1U<<(bits-8));
        p->samples[i].pcm=(struct pt_pcm){f->original[i],64,6,8000,2,(uint8_t)bits};p->samples[i].volume=64;}
    assert(pt_editor_init(f->editor,p));pt_sampler_init(&f->editor->sampler,&a,SIZE_MAX);
    /* An unused already-owned master with real spare PCM capacity exercises
     * the entire capacity ledger, independently of played sample geometry. */
    {struct pt_pcm spare;int32_t *values=cs_new(&f->masters,64*sizeof(int32_t));
        for(i=0;i<64;++i)values[i]=(int32_t)i;
        spare=(struct pt_pcm){values,64,1,8000,1,24};
        assert(pt_sampler_append_owned(&f->editor->sampler,p,&f->editor->history,&spare,&a,"unused spare")==PT_EDIT_OK);
        assert(!spare.data);}
    for(i=0;i<p->sample_count;++i)assert(pt_sampler_pin(&f->editor->sampler,p,i,f->editor->sampler.generation,
        &f->pcm[i],&f->pin[i])==PT_EDIT_OK);
    /* Pinning materializes frames*channels values from borrowed input. Retain
     * independent full declared materialized capacity and borrowed-tail images;
     * borrowed capacity is not the size of the new version allocation. */
    for(i=0;i<2;++i){assert(f->pcm[i].capacity<=64);
        memcpy(f->master_before[i],f->pcm[i].data,f->pcm[i].capacity*sizeof(int32_t));
        memcpy(f->borrowed_before[i],f->original[i],sizeof(f->original[i]));}
    assert(pt_editor_mixed_attach(f->binding,f->editor));
    f->library.available=f->library.supported=f->library.count=1;f->card->healthy=1;
    api=(struct pt_amigus_reservation_api){&f->library,open_library,close_library,find,supported,reserve,release};
    assert(pt_amigus_reservation_open_resource(&f->card->reservation,&api,0,PT_AMIGUS_WAVETABLE)==PT_AMIGUS_RESERVED);
    assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,cs_owned,cs_write));
    f->input.original.binding=f->binding;f->input.original.backend=&f->card->cache;
    f->input.original.causal.allocator=(struct pt_allocator){&f->ordinary,cs_allocate,cs_release};
    f->input.original.causal.allocator_context=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
    f->input.original.causal.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};f->input.original.causal.session=31;
    f->input.original.causal.control_budget=pt_mixed_causal_control_size();
    f->input.original.causal.queue_budget=pt_mixed_readers_control_size();
    f->input.original.causal.port=(struct pt_mixed_causal_port){&f->port,sizeof(f->port),PT_MIXED_CAUSAL_PORT_VERSION,
        PT_MIXED_CAUSAL_PORT_REQUIRED,cs_clock,cs_publish,cs_commit,cs_command_quiet,cs_reader_quiet,
        cs_source_close,cs_source_quiet,cs_publish_successor};
    f->input.original.bind_original=cs_bind;
    f->port.model.base.ticks=100;f->port.model.base.frequency=709379;
    f->port.model.base.commit_raw=f->port.model.base.source_raw=1;f->port.model.publication_raw=1;
    f->input.stop=(struct pt_mixed_causal_stop_port){&f->port,sizeof(f->port),
        PT_MIXED_CAUSAL_STOP_VERSION,PT_MIXED_CAUSAL_STOP_REQUIRED,cs_publish_stop};
    f->input.original.chip_context=&f->chip;f->input.original.chip_allocate=cs_chip_new;f->input.original.chip_release=cs_chip_free;
    f->input.original.factory_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
    f->input.original.chip_budget=4096;f->input.original.contexts=(struct pt_sampler_storage_span){f,sizeof(*f)};
    f->causal_capacity=pt_mixed_causal_workspace_size()+128;
    f->factory_capacity=pt_sampler_mixed_workspace_size()+128;
    f->causal_workspace=calloc(1,f->causal_capacity);f->factory_workspace=calloc(1,f->factory_capacity);
    assert(f->causal_workspace&&f->factory_workspace);
    f->input.original.causal_workspace=f->causal_workspace;f->input.original.causal_capacity=f->causal_capacity;
    f->input.original.factory_workspace=f->factory_workspace;f->input.original.factory_capacity=f->factory_capacity;
    f->saved=cs_save(f,&f->saved_bytes);return f;
}
static void cs_open(struct cs_trial *f)
{
    enum pt_editor_mixed_readers_result r;unsigned n=0;
    assert(pt_editor_mixed_causal_stop_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(!f->ordinary.calls&&f->binding->preparation_context==&f->control.original);
    do{r=pt_editor_mixed_causal_prepare_advance_validation(&f->control.original,7);assert(++n<10000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN&&f->ordinary.calls==3&&!f->chip.calls&&!f->card->writes&&!f->port.model.base.clocks);
    assert(f->port.bind_calls==1&&f->control.original.original_binding_called&&f->control.original.original_binding_confirmed);
    assert(f->control.original.original_binding_outcome==1&&f->control.original.pool&&f->control.original.causal&&f->control.original.queue);
}
static void cs_requests(struct cs_trial *f,unsigned n)
{
    unsigned i;memset(f->request,0,16*sizeof(*f->request));f->count=n;
    for(i=0;i<n;++i){struct pt_editor_mixed_readers_request *x=f->request+i;
        x->kind=PT_MIXED_READERS_TRIGGER;x->track=i;x->sample=i%2;x->expected=f->pin[x->sample];x->channel=f->editor->project->channels.track[i].route==PT_AMIGUS?1:0;
        if(f->editor->project->channels.track[i].route==PT_PAULA){x->geometry.paula.period=428;x->geometry.paula.volume=64;}
        else{x->geometry.amigus.bits=f->cache_bits;x->geometry.amigus.little_endian=f->little;
            x->geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,64,128};}}
}
static void cs_prepare(struct cs_trial *f,unsigned ci,uint64_t frame)
{
    enum pt_editor_mixed_readers_result r;unsigned n=0,writes;
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control.original,frame,f->request,f->count,f->command+ci)==PT_EDITOR_MIXED_READERS_PENDING);
    do{writes=f->card->writes;r=pt_editor_mixed_causal_prepare_batch_advance(&f->control.original,f->command[ci]);
        assert(f->card->writes-writes<=128&&++n<2000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN);
}
static void cs_refs(struct cs_trial *f,unsigned ci,unsigned base)
{
    unsigned i;for(i=0;i<f->count;++i)assert(pt_editor_mixed_causal_prepare_reader_reference(&f->control.original,f->command[ci],i,
        f->reader+base+i)==PT_EDITOR_MIXED_READERS_OPEN);
}
static void cs_barrier(struct cs_trial *f)
{
    uint32_t revision=f->editor->history.revision,generation=f->editor->sampler.generation;
    assert(f->binding->preparation_context==&f->control.original&&f->binding->preparation_close);
    /* In a continuing success path, inspect hook ownership only. A real edit
     * request intentionally cancels work via finish; separate veto cases below
     * exercise that behavior instead of normalizing live success state. */
    assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation);
}
static void cs_drop(struct cs_trial *f)
{
    unsigned i,calls,releases,chip_releases,shutdowns,probes,unpublished_seen=0;int closed;
    f->ordinary.alias=f->chip.alias=NULL;f->ordinary.hook=f->chip.hook=0;
    f->owned_hook=f->write_hook=f->port.task_hook=f->port.quiet_hook=0;
    /* Explicit independent cancellation observations drain genuine submitted
     * records; no rerun of preparation, enqueue, publication or fire. A quiet
     * R still referenced by C may remain until the final normal close. */
    f->port.model.hold_first=f->port.model.hold_stop=0;f->port.model.quiet_mask=~0U;
    if(f->control.original.pool){
        for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i){struct pt_editor_mixed_command_record *c=f->control.original.command+i;
            if(c->handle.address&&c->transferred){enum pt_mixed_readers_result r;
                struct pt_editor_mixed_command_ref ref={i,c->serial};
                void *handle=c->handle.address;uint64_t token=c->handle.token,ticket=c->ticket;
                unsigned queued=pt_mixed_readers_commands_held(f->control.original.queue);
                r=pt_editor_mixed_causal_prepare_service_command(&f->control.original,ref,1,NULL);
                if(r==PT_MIXED_READERS_INVALID){
                    /* Only the genuine raw0/PENDING publication case owns this exact
                     * numeric expectation. Transfer has not made it a publication. */
                    assert(f->expected_unpublished_ticket&&!unpublished_seen&&ticket==f->expected_unpublished_ticket&&
                        ref.slot==f->expected_unpublished_ref.slot&&ref.serial==f->expected_unpublished_ref.serial);
                    assert(!f->control.original.first_error&&f->port.model.publication_raw==0&&f->port.model.stop_publications==1);
                    assert(handle&&token&&c->transferred&&c->serial==ref.serial&&c->handle.address==handle&&
                        c->handle.token==token&&c->ticket==ticket&&queued==1&&
                        pt_mixed_readers_commands_held(f->control.original.queue)==queued);
                    ++unpublished_seen;
                }else assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_BACKEND);
            }}
        for(i=0;i<PT_SAMPLER_MIXED_READERS;++i){struct pt_editor_mixed_reader_record *r=f->control.original.reader+i;
            if(r->handle.address&&r->ticket){enum pt_mixed_readers_result result;
                struct pt_editor_mixed_reader_ref ref={i,r->serial};
                result=pt_editor_mixed_causal_prepare_service_reader(&f->control.original,ref,1,NULL);
                /* An already consumed queued R can be retained solely by the
                 * C-holder table; closing that exact holder is a later step. */
                assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_BACKEND||result==PT_MIXED_READERS_INVALID);
            }}
    }
    if(f->expected_unpublished_ticket)assert(unpublished_seen==1&&!f->control.original.first_error&&
        pt_mixed_readers_commands_held(f->control.original.queue)==1);
    cs_same(f);closed=pt_editor_mixed_causal_prepare_close(&f->control.original);
    assert(!f->control.original.pool&&!f->control.original.causal&&!f->control.original.queue);
    if(f->expected_unpublished_ticket)assert(unpublished_seen==1&&f->port.model.stop_publications==1&&
        f->port.model.publication_raw==0&&!f->port.model.stops);
    if(!closed){
        /* An actual consumed child with close0 is terminal. This later call
         * only completes the retained outer hook; no shutdown/free repetition. */
        cs_barrier(f);calls=f->ordinary.calls;releases=f->ordinary.releases;
        chip_releases=f->chip.releases;shutdowns=f->port.model.base.shutdowns;probes=f->port.model.base.probes;
        assert(pt_editor_mixed_causal_prepare_close(&f->control.original));
        assert(calls==f->ordinary.calls&&releases==f->ordinary.releases&&chip_releases==f->chip.releases&&
            shutdowns==f->port.model.base.shutdowns&&probes==f->port.model.base.probes);
    }
    assert(pt_editor_mixed_causal_prepare_close(&f->control.original));cs_same(f);
    assert(pt_editor_mixed_detach(f->binding));
    for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    assert(pt_editor_dispose(f->editor));
    assert(pt_amigus_wavetable_cache_detach(&f->card->cache));assert(pt_amigus_reservation_close(&f->card->reservation));
    pt_document_release(f->document);
    assert(!cs_live(&f->ordinary)&&!cs_live(&f->chip)&&!cs_live(&f->masters));
    assert(f->ordinary.calls==f->ordinary.releases+f->ordinary.refusals&&
        f->chip.calls==f->chip.releases+f->chip.refusals);
    assert(!f->ordinary.alias_releases&&!f->chip.alias_releases);
    free(f->saved);free(f->causal_workspace);free(f->factory_workspace);free(f->card);free(f->document);
    free(f->stop);free(f->request);free(f->command);free(f->reader);free(f->editor);free(f->binding);free(f);
}
static uint64_t cs_admit(struct cs_trial *f,unsigned ci)
{
    uint64_t ticket=999;struct pt_editor_mixed_command_record *record;
    assert(pt_editor_mixed_causal_prepare_enqueue(&f->control.original,f->command[ci],&ticket)==PT_MIXED_READERS_OK&&ticket!=999);
    record=f->control.original.command+f->command[ci].slot;
    assert(record->ticket==ticket&&record->transferred&&record->handle.address);
    assert(pt_editor_mixed_causal_prepare_publish(&f->control.original,f->command[ci])==PT_MIXED_READERS_OK);
    return ticket;
}
static void cs_observe(struct cs_trial *f,unsigned base,unsigned count)
{
    unsigned i,releases=f->ordinary.releases,chip=f->chip.releases,commands=f->port.model.base.command_proofs;
    for(i=0;i<count;++i){struct pt_editor_mixed_reader_record *r=f->control.original.reader+f->reader[base+i].slot;
        void *original=r->handle.address;uint64_t token=r->handle.token;
        assert(original&&token&&r->ticket);
        assert(pt_editor_mixed_causal_prepare_service_reader(&f->control.original,f->reader[base+i],0,NULL)==PT_MIXED_READERS_PENDING);
        assert(r->handle.address==original&&r->handle.token==token&&r->ticket);
    }
    assert(releases==f->ordinary.releases&&chip==f->chip.releases&&commands==f->port.model.base.command_proofs);
    cs_same(f);
}
static void cs_drain_readers(struct cs_trial *f,unsigned base,unsigned count,unsigned failed)
{
    unsigned i;enum pt_mixed_readers_result expected=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
    for(i=0;i<count;++i)assert(pt_editor_mixed_causal_prepare_service_reader(&f->control.original,f->reader[base+i],1,NULL)==expected);
    /* Positive R proof with retained C references can leave the real factory
     * handle alive; only later actual NULL consumption closes registration. */
}
static void cs_drain_command(struct cs_trial *f,unsigned ci,unsigned failed)
{
    assert(pt_editor_mixed_causal_prepare_service_command(&f->control.original,f->command[ci],1,NULL)==
        (failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    assert(!f->control.original.command[f->command[ci].slot].handle.address);
}
static void cs_all_card(struct cs_trial *f)
{
    unsigned i;for(i=0;i<16;++i)f->editor->project->channels.track[i].route=PT_AMIGUS;
    free(f->saved);f->saved=cs_save(f,&f->saved_bytes);
}
/* SOURCE-only acceptance cases. Inherited entries are deliberately uncalled. */
static uint64_t cs_first(struct cs_trial *f,unsigned observed)
{
    uint64_t first;cs_open(f);cs_requests(f,16);cs_prepare(f,0,960);cs_refs(f,0,0);first=cs_admit(f,0);
    assert(f->port.model.base.publications==1&&!f->port.model.stop_publications);
    f->port.model.base.ticks=oracle(960)-1;
    assert(pt_mixed_causal_fire(f->control.original.causal,first)==PT_MIXED_CAUSAL_EARLY&&!f->port.model.base.effects);
    f->port.model.base.ticks=oracle(960);
    assert(pt_mixed_causal_fire(f->control.original.causal,first)==PT_MIXED_CAUSAL_COMMITTED);
    assert(f->port.model.base.effects==16);
    if(observed){
        /* First C actually detaches/frees; its former address is never read. */
        void *former=f->control.original.command[f->command[0].slot].handle.address;
        assert(former);
        assert(pt_editor_mixed_causal_prepare_service_command(&f->control.original,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
        assert(!f->control.original.command[f->command[0].slot].handle.address);
    }
    return first;
}
static void cs_stop_input(struct cs_trial *f,unsigned count)
{
    unsigned i;memset(f->stop,0,sizeof(*f->stop));f->stop->frame=1920;f->stop->count=count;
    for(i=0;i<count;++i)f->stop->reader[i]=f->reader[i];
}
static void cs_stop_prepare(struct cs_trial *f)
{
    enum pt_editor_mixed_readers_result r;unsigned steps=0,calls=f->ordinary.calls;
    unsigned chip=f->chip.calls,masters=f->masters.calls,writes=f->card->writes;
    assert(pt_editor_mixed_causal_stop_prepare_batch_begin(&f->control,f->stop,f->command+1)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(f->ordinary.calls==calls+1&&f->chip.calls==chip&&f->masters.calls==masters&&f->card->writes==writes);
    assert(!f->control.original.command[f->command[1].slot].count);
    do{r=pt_editor_mixed_causal_prepare_batch_advance(&f->control.original,f->command[1]);assert(++steps<2000);}
    while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN&&f->ordinary.calls==calls+1&&f->chip.calls==chip&&
        f->masters.calls==masters&&f->card->writes==writes&&pt_mixed_readers_readers_held(f->control.original.queue)==16);
}
static unsigned cs_cache_pins(struct cs_trial *f)
{unsigned i,n=0;for(i=0;i<PT_CACHE_SLOTS;++i)n+=f->card->cache.cache.entry[i].pins;return n;}
static void cs_stop_success(unsigned bits,unsigned cache,unsigned all_card,unsigned targets,unsigned order)
{
    struct cs_trial *f=cs_make(bits,cache,0);struct pt_editor_mixed_reader_record before[32];
    struct pt_editor_mixed_reader_ref refused={99,999};struct pt_mixed_readers_key slots[20];
    unsigned i,calls,chips,masters,writes,pins,selected=0;uint64_t first,second,out=777;
    if(all_card)cs_all_card(f);
    first=cs_first(f,1);memcpy(before,f->control.original.reader,sizeof(before));
    memcpy(slots,f->port.model.base.slot,sizeof(slots));
    calls=f->ordinary.calls;chips=f->chip.calls;masters=f->masters.calls;writes=f->card->writes;pins=cs_cache_pins(f);
    cs_stop_input(f,targets);cs_stop_prepare(f);second=cs_admit(f,1);
    assert(second!=first&&f->command[1].slot==f->command[0].slot&&f->command[1].serial!=f->command[0].serial);
    assert(!memcmp(before,f->control.original.reader,sizeof(before))&&cs_cache_pins(f)==pins);
    assert(pt_editor_mixed_causal_prepare_reader_reference(&f->control.original,f->command[1],0,&refused)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(refused.slot==99&&refused.serial==999);
    {struct ct_command *c=ct_command(&f->port.model.base,second);assert(c&&c->packet.frame==1920&&
        c->packet.first==oracle(1920)&&c->packet.last==oracle(1921));
        for(i=0;i<targets;++i)selected|=1U<<ct_index(c->packet.key[i].route,c->packet.key[i].slot);}
    f->port.model.base.ticks=oracle(1920)-1;
    assert(pt_mixed_causal_fire(f->control.original.causal,second)==PT_MIXED_CAUSAL_EARLY&&!f->port.model.stops);
    assert(!memcmp(slots,f->port.model.base.slot,sizeof(slots))&&!memcmp(before,f->control.original.reader,sizeof(before)));
    f->port.model.base.ticks=oracle(1920);
    assert(pt_mixed_causal_fire(f->control.original.causal,second)==PT_MIXED_CAUSAL_COMMITTED&&f->port.model.stops==targets);
    assert(f->port.model.stopped_mask==selected&&!(f->port.model.base.mask&selected));
    for(i=0;i<20;++i)if(!(selected&(1U<<i)))assert(keys_equal(slots+i,f->port.model.base.slot+i));
    assert(calls+1==f->ordinary.calls&&chips==f->chip.calls&&masters==f->masters.calls&&writes==f->card->writes);
    assert(cs_cache_pins(f)==pins&&pt_mixed_readers_readers_held(f->control.original.queue)==16);
    cs_observe(f,0,16);cs_barrier(f);
    assert(pt_editor_mixed_causal_prepare_enqueue(&f->control.original,f->command[1],&out)==PT_MIXED_READERS_INVALID&&out==777);
    assert(pt_editor_mixed_causal_stop_prepare_batch_begin(&f->control,f->stop,f->command)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control.original,2880,f->request,16,f->command)==PT_EDITOR_MIXED_READERS_CAPACITY);
    if(!order)cs_drain_command(f,1,0);
    f->port.model.quiet_mask=selected;
    for(i=0;i<targets;++i){
        assert(pt_editor_mixed_causal_prepare_service_reader(&f->control.original,f->reader[i],0,NULL)==PT_MIXED_READERS_OK);
        if(order)assert(f->control.original.reader[f->reader[i].slot].handle.address);
    }
    if(order)cs_drain_command(f,1,0);
    for(i=targets;i<16;++i)assert(pt_editor_mixed_causal_prepare_service_reader(&f->control.original,f->reader[i],0,NULL)==PT_MIXED_READERS_PENDING);
    cs_same(f);cs_drop(f);
}
static void cs_shape_refusal(unsigned mode)
{
    struct cs_trial *f=cs_make(24,16,0);struct pt_editor_mixed_command_ref out={99,999};
    unsigned calls,writes,clocks,proofs;uint64_t first;unsigned char *misaligned=NULL;
    struct pt_editor_mixed_causal_stop_batch *original=f->stop;
    if(!mode){cs_open(f);cs_requests(f,16);cs_prepare(f,0,960);cs_refs(f,0,0);first=cs_admit(f,0);assert(first);}
    else first=cs_first(f,mode!=1);
    assert(first);cs_stop_input(f,2);
    if(mode==2)f->stop->reader[1]=f->stop->reader[0];
    if(mode==3)++f->stop->reader[0].serial;
    if(mode==4)f->stop->reader[15]=f->reader[15];
    if(mode==5)f->stop->frame=960;
    if(mode==6)f->stop->frame=UINT64_MAX;
    if(mode==7){misaligned=malloc(sizeof(*f->stop)+1);assert(misaligned);
        memcpy(misaligned+1,f->stop,sizeof(*f->stop));f->stop=(void *)(misaligned+1);}
    calls=f->ordinary.calls;writes=f->card->writes;clocks=f->port.model.base.clocks;proofs=f->port.model.base.command_proofs;
    assert(pt_editor_mixed_causal_stop_prepare_batch_begin(&f->control,f->stop,&out)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(out.slot==99&&out.serial==999&&calls==f->ordinary.calls&&writes==f->card->writes&&
        clocks==f->port.model.base.clocks&&proofs==f->port.model.base.command_proofs);
    f->stop=original;free(misaligned);cs_drop(f);
}
static void cs_constructor_refusal(unsigned mode)
{
    struct cs_trial *f=cs_make(24,16,0);struct pt_editor_mixed_causal_stop_inputs saved;
    struct pt_editor_mixed_causal_stop_prepare *w=&f->control;const struct pt_editor_mixed_causal_stop_inputs *in=&f->input;
    unsigned char *misaligned=NULL;enum pt_editor_mixed_readers_result r;
    if(mode==0){misaligned=calloc(1,sizeof(*w)+1);assert(misaligned);w=(void *)(misaligned+1);}
    if(mode==1){misaligned=calloc(1,sizeof(*in)+1);assert(misaligned);memcpy(misaligned+1,in,sizeof(*in));in=(void *)(misaligned+1);}
    if(mode==2)f->input.original.contexts=(struct pt_sampler_storage_span){&f->control,sizeof(f->control.original)};
    if(mode==3)f->input.stop.context=&f->ordinary;
    if(mode==4)--f->input.stop.context_bytes;
    if(mode==5)++f->input.stop.flags;
    if(mode==6)f->input.stop.publish_stop=NULL;
    if(mode==7)f->control.self=&f->control;
    if(mode==8){struct pt_editor_mixed_causal_stop_prepare *copy;
        cs_open(f);copy=malloc(sizeof(*copy));assert(copy);memcpy(copy,&f->control,sizeof(*copy));
        r=pt_editor_mixed_causal_stop_prepare_begin(copy,&f->input);
        assert(r==PT_EDITOR_MIXED_READERS_INVALID&&!f->control.original.first_error);
        free(copy);cs_drop(f);return;}
    memcpy(&saved,&f->input,sizeof(saved));r=pt_editor_mixed_causal_stop_prepare_begin(w,in);
    assert(r==PT_EDITOR_MIXED_READERS_INVALID&&!memcmp(&saved,&f->input,sizeof(saved))&&!f->ordinary.calls&&
        !f->chip.calls&&!f->card->writes&&!f->port.bind_calls&&!f->binding->preparation_context);
    free(misaligned);memset(&f->control,0,sizeof(f->control));cs_drop(f);
}
static void cs_output_alias(unsigned mode)
{
    struct cs_trial *f=cs_make(24,16,0);void *alias;unsigned calls,writes;unsigned char before[16];
    unsigned char *misaligned=NULL;
    (void)cs_first(f,1);cs_stop_input(f,2);calls=f->ordinary.calls;writes=f->card->writes;
    if(mode==0)alias=(char *)f->causal_workspace+f->causal_capacity-16;
    else if(mode==1)alias=(char *)f->factory_workspace+f->factory_capacity-16;
    else if(mode==2)alias=(void *)(f->pcm[31].data+60);
    else if(mode==3)alias=f->control.original.reader[f->reader[0].slot].handle.address;
    else if(mode==4)alias=&f->control.factory;
    else{misaligned=malloc(17);assert(misaligned);memset(misaligned,0xa5,17);alias=misaligned+1;}
    memcpy(before,alias,sizeof(before));
    assert(pt_editor_mixed_causal_stop_prepare_batch_begin(&f->control,f->stop,alias)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(!memcmp(before,alias,sizeof(before))&&calls==f->ordinary.calls&&writes==f->card->writes&&!f->control.original.first_error);
    free(misaligned);cs_drop(f);
}
static void cs_allocate_refusal(unsigned mode)
{
    struct cs_trial *f=cs_make(24,16,0);struct pt_editor_mixed_command_ref out={99,999};
    struct pt_editor_mixed_command_record command_before[PT_SAMPLER_MIXED_COMMANDS];
    struct pt_editor_mixed_reader_record reader_before[PT_SAMPLER_MIXED_READERS];
    struct pt_sampler_storage_span guards_before[PT_EDITOR_MIXED_READERS_GUARDS];
    struct cs_ledger ledger_before[80];struct pt_sampler_mixed_causal_stop_binding binding_before;
    unsigned before,releases,refusals,live,guards,prepared,chips,masters,writes,pins;uint64_t serial;
    enum pt_editor_mixed_readers_result r;
    (void)cs_first(f,1);cs_stop_input(f,2);before=f->ordinary.calls;
    releases=f->ordinary.releases;refusals=f->ordinary.refusals;live=cs_live(&f->ordinary);
    guards=f->control.original.guard_count;prepared=f->control.original.prepared_batches;serial=f->control.original.serial;
    chips=f->chip.calls;masters=f->masters.calls;writes=f->card->writes;pins=cs_cache_pins(f);
    memcpy(command_before,f->control.original.command,sizeof(command_before));
    memcpy(reader_before,f->control.original.reader,sizeof(reader_before));
    memcpy(guards_before,f->control.original.guards,sizeof(guards_before));
    memcpy(ledger_before,f->ordinary.live,sizeof(ledger_before));memcpy(&binding_before,&f->control.factory,sizeof(binding_before));
    if(mode==0)f->ordinary.fail=before+1;
    if(mode==1)f->ordinary.alias=&f->control.factory;
    if(mode==2)f->ordinary.hook=before+1;
    if(mode==3)f->stop_alloc_hook=1;
    r=pt_editor_mixed_causal_stop_prepare_batch_begin(&f->control,f->stop,&out);
    assert(r!=PT_EDITOR_MIXED_READERS_PENDING&&out.slot==99&&out.serial==999&&f->ordinary.calls==before+1);
    if(mode==0){
        /* Ordinary NULL is recoverable capacity, not an ownership fault. */
        assert(r==PT_EDITOR_MIXED_READERS_CAPACITY&&!f->control.original.first_error&&
            f->ordinary.refusals==refusals+1&&f->ordinary.releases==releases);
    }else if(mode==1){
        /* Recognized complete-wrapper alias must be sticky FAULT; the lower
         * allocator receives NULL, never initializes/registers/releases it. */
        assert(r==PT_EDITOR_MIXED_READERS_FAULT&&f->control.original.first_error==PT_EDITOR_MIXED_READERS_FAULT&&
            f->control.original.result==PT_EDITOR_MIXED_READERS_FAULT&&
            f->ordinary.refusals==refusals+1&&f->ordinary.releases==releases&&!f->ordinary.alias_releases&&
            !memcmp(&binding_before,&f->control.factory,sizeof(binding_before)));
    }else assert(f->control.original.first_error);
    if(mode<2){
        assert(f->control.original.serial==serial&&f->control.original.prepared_batches==prepared&&
            !memcmp(command_before,f->control.original.command,sizeof(command_before))&&
            !memcmp(reader_before,f->control.original.reader,sizeof(reader_before))&&
            !memcmp(ledger_before,f->ordinary.live,sizeof(ledger_before))&&cs_live(&f->ordinary)==live&&
            f->control.original.guard_count==guards&&!memcmp(guards_before,f->control.original.guards,sizeof(guards_before))&&
            chips==f->chip.calls&&masters==f->masters.calls&&writes==f->card->writes&&pins==cs_cache_pins(f)&&
            pt_mixed_readers_readers_held(f->control.original.queue)==16&&
            !pt_mixed_readers_commands_held(f->control.original.queue));
        cs_same(f);
    }
    if(mode==3)--f->stop->frame;
    cs_drop(f);
}
/* Two additional genuine instances, separately counted from original V2 50.
 * No retired local bytes are read after batch_begin returns. The live beforeimage
 * plus actual guard-triggered NULL/no-new-handle and verified pair retirement
 * establish refusal; post-refusal local-byte observation is not claimed. */
static void cs_scratch_allocate_refusal(unsigned mode)
{
    struct cs_trial *f=cs_make(24,16,0);struct pt_editor_mixed_command_ref *out=f->command+1;
    struct pt_editor_mixed_command_record command_before[PT_SAMPLER_MIXED_COMMANDS];
    struct pt_editor_mixed_reader_record reader_before[PT_SAMPLER_MIXED_READERS];
    struct pt_sampler_storage_span guards_before[PT_EDITOR_MIXED_READERS_GUARDS];
    struct pt_editor_mixed_causal_stop_batch input_before;struct cs_ledger ledger_before[80];
    unsigned calls,releases,refusals,live,chips,masters,writes,pins,prepared,guard_count;uint64_t serial;
    assert(mode==1||mode==2);(void)cs_first(f,1);cs_stop_input(f,2);*out=(struct pt_editor_mixed_command_ref){99,999};
    /* Keep the external output in its genuine heap ref array: a deliberately
     * oversized lower allocation extent from local scratch must not overlap
     * an unrelated caller stack output before reaching the third guard. */
    calls=f->ordinary.calls;releases=f->ordinary.releases;refusals=f->ordinary.refusals;live=cs_live(&f->ordinary);
    chips=f->chip.calls;masters=f->masters.calls;writes=f->card->writes;pins=cs_cache_pins(f);
    prepared=f->control.original.prepared_batches;serial=f->control.original.serial;guard_count=f->control.original.guard_count;
    memcpy(command_before,f->control.original.command,sizeof(command_before));
    memcpy(reader_before,f->control.original.reader,sizeof(reader_before));
    memcpy(guards_before,f->control.original.guards,sizeof(guards_before));
    memcpy(&input_before,f->stop,sizeof(input_before));memcpy(ledger_before,f->ordinary.live,sizeof(ledger_before));
    f->scratch_guard_before=guard_count;f->scratch_output=out;f->scratch_alias_mode=mode;
    assert(pt_editor_mixed_causal_stop_prepare_batch_begin(&f->control,f->stop,out)==PT_EDITOR_MIXED_READERS_FAULT);
    assert(f->scratch_alias_hits==1&&!f->scratch_alias_mode&&f->scratch_live_address&&f->scratch_live_bytes&&
        f->scratch_returned_alias&&f->scratch_requested_bytes&&f->ordinary.alias==f->scratch_returned_alias);
    assert(f->control.original.first_error==PT_EDITOR_MIXED_READERS_FAULT&&
        f->control.original.result==PT_EDITOR_MIXED_READERS_FAULT&&out->slot==99&&out->serial==999);
    assert(f->ordinary.calls==calls+1&&f->ordinary.releases==releases&&f->ordinary.refusals==refusals+1&&
        !f->ordinary.alias_releases&&cs_live(&f->ordinary)==live&&!memcmp(ledger_before,f->ordinary.live,sizeof(ledger_before)));
    assert(f->control.original.serial==serial&&f->control.original.prepared_batches==prepared&&
        !memcmp(command_before,f->control.original.command,sizeof(command_before))&&
        !memcmp(reader_before,f->control.original.reader,sizeof(reader_before))&&
        !memcmp(&input_before,f->stop,sizeof(input_before))&&
        f->control.original.guard_count==guard_count&&!memcmp(guards_before,f->control.original.guards,sizeof(guards_before))&&
        !f->control.original.busy&&chips==f->chip.calls&&masters==f->masters.calls&&writes==f->card->writes&&pins==cs_cache_pins(f)&&
        pt_mixed_readers_readers_held(f->control.original.queue)==16&&!pt_mixed_readers_commands_held(f->control.original.queue));
    cs_same(f);cs_drop(f);
}
static void cs_publication_outcome(unsigned mode)
{
    struct cs_trial *f=cs_make(24,16,0);struct pt_editor_mixed_reader_record before[32];
    uint64_t second=0;enum pt_mixed_readers_result r;
    (void)cs_first(f,1);cs_stop_input(f,2);cs_stop_prepare(f);memcpy(before,f->control.original.reader,sizeof(before));
    if(mode==4){f->owned_hook=1;f->owned_after=3;}
    assert(pt_editor_mixed_causal_prepare_enqueue(&f->control.original,f->command[1],&second)==PT_MIXED_READERS_OK&&second);
    assert(f->control.original.command[f->command[1].slot].transferred&&!memcmp(before,f->control.original.reader,sizeof(before)));
    if(mode==4){assert(f->control.original.first_error&&!f->owned_hook);
        assert(pt_editor_mixed_causal_prepare_publish(&f->control.original,f->command[1])==PT_MIXED_READERS_INVALID);
        cs_drop(f);return;}
    if(mode==0)f->port.model.publication_raw=0;
    if(mode==1)f->port.model.publication_raw=-1;
    if(mode==2){f->port.model.publication_raw=0;f->port.model.publication_mutation=1;}
    if(mode==3)f->port.task_hook=1;
    r=pt_editor_mixed_causal_prepare_publish(&f->control.original,f->command[1]);
    assert(r==(mode==0?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_BACKEND));
    assert(!f->port.model.stops&&pt_mixed_readers_readers_held(f->control.original.queue)==16&&
        f->control.original.command[f->command[1].slot].transferred);
    if(mode==0){
        struct pt_editor_mixed_command_record *record=f->control.original.command+f->command[1].slot;
        assert(r==PT_MIXED_READERS_PENDING&&!f->control.original.first_error&&
            f->port.model.publication_raw==0&&f->port.model.stop_publications==1&&
            pt_mixed_readers_commands_held(f->control.original.queue)==1&&record->handle.address&&
            record->handle.token&&record->ticket==second&&record->serial==f->command[1].serial);
        f->expected_unpublished_ref=f->command[1];f->expected_unpublished_ticket=second;
    }
    cs_drop(f);
}
static void cs_issue_failure(unsigned mode)
{
    struct cs_trial *f=cs_make(24,16,0);uint64_t second;int r;unsigned writes,chips;
    (void)cs_first(f,1);cs_stop_input(f,2);cs_stop_prepare(f);second=cs_admit(f,1);
    writes=f->card->writes;chips=f->chip.calls;f->port.model.base.ticks=oracle(1920);
    if(mode==0)f->port.model.base.commit_raw=0;
    if(mode==1)f->port.model.base.commit_raw=-1;
    if(mode==2)f->port.model.base.late=1;
    if(mode==3)f->port.model.base.malformed=1;
    if(mode==4)f->port.model.base.ticks=oracle(1921);
    r=pt_mixed_causal_fire(f->control.original.causal,second);
    assert(r!=PT_MIXED_CAUSAL_COMMITTED&&writes==f->card->writes&&chips==f->chip.calls);
    assert(pt_mixed_readers_readers_held(f->control.original.queue)==16);cs_barrier(f);cs_drop(f);
}
static void cs_cancel_before_issue(unsigned mode)
{
    struct cs_trial *f=cs_make(24,8,0);uint64_t second;unsigned writes,chips,calls,masters,pins=0,commits;
    struct pt_editor_mixed_command_record command_before[PT_SAMPLER_MIXED_COMMANDS];
    struct pt_editor_mixed_reader_record reader_before[PT_SAMPLER_MIXED_READERS];
    struct ct_command *published=NULL;
    (void)cs_first(f,1);cs_stop_input(f,1);cs_stop_prepare(f);second=cs_admit(f,1);
    writes=f->card->writes;chips=f->chip.calls;calls=f->ordinary.calls;masters=f->masters.calls;commits=f->port.model.base.commits;
    if(mode==2){
        const struct pt_editor_mixed_command_record *record=f->control.original.command+f->command[1].slot;
        assert(!f->control.original.first_error&&record->handle.address&&record->handle.token&&record->transferred&&
            record->ticket==second&&record->serial==f->command[1].serial&&f->port.model.stop_publications==1&&
            pt_mixed_readers_commands_held(f->control.original.queue)==1&&pt_mixed_readers_readers_held(f->control.original.queue)==16);
        published=ct_command(&f->port.model.base,second);assert(published&&!published->finished&&!published->disabled);
        memcpy(command_before,f->control.original.command,sizeof(command_before));
        memcpy(reader_before,f->control.original.reader,sizeof(reader_before));pins=cs_cache_pins(f);cs_same(f);
    }
    if(mode<2)assert(pt_editor_mixed_causal_prepare_service_reader(&f->control.original,f->reader[mode?5:0],1,NULL)==PT_MIXED_READERS_OK);
    else assert(pt_editor_mixed_causal_prepare_cancel(&f->control.original)==PT_EDITOR_MIXED_READERS_CANCELLED);
    if(mode==2){
        /* Published C/R remain genuinely held. Queue stop is PENDING; the later
         * FAULT fail-closes activation while retaining first CANCELLED. */
        assert(f->control.original.first_error==PT_EDITOR_MIXED_READERS_CANCELLED&&
            !memcmp(command_before,f->control.original.command,sizeof(command_before))&&
            !memcmp(reader_before,f->control.original.reader,sizeof(reader_before))&&
            pt_mixed_readers_commands_held(f->control.original.queue)==1&&pt_mixed_readers_readers_held(f->control.original.queue)==16&&
            ct_command(&f->port.model.base,second)==published&&!published->finished&&!published->disabled&&
            f->port.model.stop_publications==1&&cs_cache_pins(f)==pins&&commits==f->port.model.base.commits);
        cs_same(f);
    }
    f->port.model.base.ticks=oracle(1920);
    assert(pt_mixed_causal_fire(f->control.original.causal,second)==
        (mode==2?PT_MIXED_CAUSAL_FAILED:PT_MIXED_CAUSAL_INVALID)&&!f->port.model.stops);
    if(mode==2){
        assert(f->control.original.first_error==PT_EDITOR_MIXED_READERS_CANCELLED&&
            !memcmp(command_before,f->control.original.command,sizeof(command_before))&&
            !memcmp(reader_before,f->control.original.reader,sizeof(reader_before))&&
            pt_mixed_readers_commands_held(f->control.original.queue)==1&&pt_mixed_readers_readers_held(f->control.original.queue)==16&&
            ct_command(&f->port.model.base,second)==published&&!published->finished&&!published->disabled&&
            f->port.model.stop_publications==1&&cs_cache_pins(f)==pins&&commits==f->port.model.base.commits);
        cs_same(f);
    }
    assert(writes==f->card->writes&&chips==f->chip.calls&&calls==f->ordinary.calls&&masters==f->masters.calls);cs_drop(f);
}
static void cs_source_pending(int raw)
{
    struct cs_trial *f=cs_make(24,16,0);uint64_t second;unsigned releases;
    (void)cs_first(f,1);cs_stop_input(f,1);cs_stop_prepare(f);second=cs_admit(f,1);
    f->port.model.base.ticks=oracle(1920);assert(pt_mixed_causal_fire(f->control.original.causal,second)==PT_MIXED_CAUSAL_COMMITTED);
    f->port.model.quiet_mask=~0U;cs_drain_command(f,1,0);cs_drain_readers(f,0,16,0);
    f->port.model.base.source_raw=raw;f->port.quiet_raw=0;
    assert(!pt_editor_mixed_causal_prepare_close(&f->control.original)&&!f->control.original.pool&&
        !f->control.original.queue&&f->control.original.causal&&cs_live(&f->ordinary)==1);
    cs_barrier(f);releases=f->ordinary.releases;
    assert(!pt_editor_dispose(f->editor)&&f->ordinary.releases==releases&&f->port.model.base.shutdowns==1);
    f->port.quiet_raw=1;assert(pt_editor_mixed_causal_prepare_close(&f->control.original)&&!f->control.original.causal);
    assert(f->port.model.base.shutdowns==1&&f->ordinary.releases==releases+1);cs_drop(f);
}
static void cs_foreign_nonmatching_ref(void)
{
    struct cs_trial *a=cs_make(16,8,0),*b=cs_make(8,16,0);struct pt_editor_mixed_command_ref out={99,999};
    unsigned calls,n=0;enum pt_editor_mixed_readers_result r;uint64_t first,second;
    (void)cs_first(a,1);
    /* Other controller uses the preserved default pair, genuinely issuing
     * second-batch R refs outside our first sixteen occupied records. */
    assert(pt_editor_mixed_causal_prepare_begin(&b->control.original,&b->input.original)==PT_EDITOR_MIXED_READERS_PENDING);
    do{r=pt_editor_mixed_causal_prepare_advance_validation(&b->control.original,7);assert(++n<10000);}
    while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN);cs_requests(b,16);cs_prepare(b,0,960);cs_refs(b,0,0);first=cs_admit(b,0);
    cs_prepare(b,1,1920);cs_refs(b,1,16);second=cs_admit(b,1);
    b->port.model.base.ticks=oracle(960);assert(pt_mixed_causal_fire(b->control.original.causal,first)==PT_MIXED_CAUSAL_COMMITTED);
    b->port.model.base.ticks=oracle(1920);assert(pt_mixed_causal_fire(b->control.original.causal,second)==PT_MIXED_CAUSAL_COMMITTED);
    cs_stop_input(a,1);
    /* Refs are LOCAL numeric identities. Equal {slot,serial} from another
     * controller has no distinct provenance and resolves only to OUR handle.
     * This case uses a genuine other reference that is numerically different;
     * no cross-owner authentication claim is made by the fixed public ref. */
    a->stop->reader[0]=b->reader[16];
    calls=a->ordinary.calls;assert(pt_editor_mixed_causal_stop_prepare_batch_begin(&a->control,a->stop,&out)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(out.slot==99&&out.serial==999&&calls==a->ordinary.calls);cs_drop(a);cs_drop(b);
}
#ifndef PT_EDITOR_MIXED_CAUSAL_STOP_PREPARE_TEST_MAIN
#define PT_EDITOR_MIXED_CAUSAL_STOP_PREPARE_TEST_MAIN main
#endif
int PT_EDITOR_MIXED_CAUSAL_STOP_PREPARE_TEST_MAIN(void)
{
    unsigned i;
    cs_stop_success(8,8,0,16,0);cs_stop_success(16,16,0,1,1);cs_stop_success(24,16,0,16,1);
    cs_stop_success(8,16,1,16,1);cs_stop_success(16,8,1,1,0);cs_stop_success(24,16,1,16,0);
    for(i=0;i<8;++i)cs_shape_refusal(i);
    for(i=0;i<9;++i)cs_constructor_refusal(i);
    for(i=0;i<6;++i)cs_output_alias(i);
    for(i=0;i<4;++i)cs_allocate_refusal(i);
    cs_scratch_allocate_refusal(1);cs_scratch_allocate_refusal(2);
    for(i=0;i<5;++i)cs_publication_outcome(i);
    for(i=0;i<5;++i)cs_issue_failure(i);
    for(i=0;i<3;++i)cs_cancel_before_issue(i);
    cs_source_pending(0);cs_source_pending(-1);cs_foreign_nonmatching_ref();
    assert(cs_cases==52);
    puts("EDITOR MIXED CAUSAL STOP PREPARE PASS:52 genuine controller instances;original50 plus2 actual-third-guard saved-ref/pair-tail allocator aliases;6 mixed/card depth/cache subset/all STOP schedules;8 first/ACTIVE/ref/frame/input refusals;9 complete constructor guards;6 source/workspace/holder/alignment output aliases;4 allocation/reentry/mutation refusals;5 publication/actual-transfer outcomes;5 partial/unknown/late issues;3 selected/expected/local cancellations;2 independent source quiet cases;local numeric-ref scope;original960/1920 windows,zero new readers and unchanged old tickets,exact master/save/cache custody; SOFTWARE_ONLY");
    return 0;
}
