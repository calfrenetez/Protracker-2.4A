/* SOURCE-only genuine controller/factory/queue fixture. No runtime claimed.
 * Original adopted STOP helpers/main remain unchanged and UNCALLED. Product
 * units compile separately; this ordinary RAM port is no live device backend. */
#define PT_EDITOR_MIXED_CAUSAL_STOP_PREPARE_TEST_MAIN cl_original_stop_fixture_not_called
#include "editor_mixed_causal_stop_prepare_test.c"
#undef PT_EDITOR_MIXED_CAUSAL_STOP_PREPARE_TEST_MAIN
#include "../src/editor/editor_mixed_causal_lineage_prepare_internal.h"

struct cl_trial;
struct cl_port {
    struct cc_port model;
    struct cl_trial *owner;
    struct pt_mixed_causal_completed_value root,control;
    struct pt_mixed_causal_command_identity third;
    unsigned control_completed,control_publications,third_publications,binds;
    unsigned hold_control,hold_third,task_hook;
    int control_raw,third_raw,quiet_raw;
};
struct cl_trial {
    struct cs_trial old;
    struct pt_editor_mixed_causal_lineage_prepare control;
    struct pt_editor_mixed_causal_lineage_inputs input;
    struct cl_port port;
    struct pt_editor_mixed_causal_lineage_control_batch *change;
    struct pt_editor_mixed_causal_lineage_stop_batch *stop;
    struct pt_editor_mixed_command_ref *command;
    unsigned allocation_reentry,allocation_mutation,high_water,scratch_alias,scratch_hits;
    struct pt_editor_mixed_command_ref unpublished_ref;
    uint64_t unpublished_ticket;
};
static unsigned cl_cases;
static struct pt_editor_mixed_causal_prepare *cl_s(struct cl_trial *c){return &c->control.original;}
static void cl_reenter(struct cl_trial *c)
{
    struct pt_editor_mixed_causal_prepare *s=cl_s(c);unsigned before=s->reentries;
    assert(pt_editor_mixed_causal_prepare_get(s)==PT_EDITOR_MIXED_READERS_FAULT);
    assert(s->reentries==before+1&&!pt_editor_mixed_causal_prepare_close(s));
}
static void *cl_allocate(void *context,size_t n)
{
    struct cs_memory *m=context;struct cl_trial *c=(void *)m->owner;void *p;
    assert(c->old.binding->preparation_context==cl_s(c));
    if(c->allocation_reentry){c->allocation_reentry=0;cl_reenter(c);}
    if(c->allocation_mutation){c->allocation_mutation=0;++c->change->frame;}
    if(c->scratch_alias){struct pt_editor_mixed_causal_prepare *s=cl_s(c);
       struct pt_sampler_storage_span whole=s->guards[s->guard_count-1];unsigned i;
       assert(s->busy&&s->guard_count>=3&&whole.data&&whole.bytes>sizeof(*c->change));
       /* Genuine last guard is the complete still-live transformed scratch.
        * Return a numeric alias into its tail, never dereference it later. */
       m->alias=(unsigned char *)whole.data+whole.bytes-sizeof(struct cs_stop_pair_layout);
       assert(!cs_numeric_apart(m->alias,n,whole.data,whole.bytes));
       for(i=0;i<s->guard_count-1;++i)assert(cs_numeric_apart(m->alias,n,s->guards[i].data,s->guards[i].bytes));
       c->scratch_alias=0;++c->scratch_hits;
    }
    p=cs_new(m,n);if(cs_live(m)>c->high_water)c->high_water=cs_live(m);return p;
}
static void cl_release(void *context,void *p)
{
    struct cs_memory *m=context;struct cl_trial *c=(void *)m->owner;unsigned i;
    for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)assert(cl_s(c)->ordinary[i].data!=p);
    cs_free(m,p,0,0);
}
static void *cl_chip_new(void *context,size_t n){return cs_new(context,n);}
static void cl_chip_free(void *context,void *p,size_t n)
{
    struct cs_memory *m=context;struct cl_trial *c=(void *)m->owner;unsigned i;
    for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP;++i)assert(cl_s(c)->chip[i].data!=p);
    cs_free(m,p,n,1);
}
static int cl_owned(void *context)
{
    struct cs_bus *bus=context;struct cl_trial *c=(void *)bus->owner;struct cs_trial *f=&c->old;
    int r=f->card->healthy==1&&f->library.library&&f->library.owner==&f->card->reservation&&
       f->library.acquired==PT_AMIGUS_WAVETABLE&&f->card->reservation.reserved;
    ++f->owned_calls;
    if(f->owned_hook&&(!f->owned_after||!--f->owned_after)){f->owned_hook=0;cl_reenter(c);}return r;
}
static int cl_write(void *context,unsigned reg,uint32_t value)
{
    struct cs_bus *bus=context;struct cl_trial *c=(void *)bus->owner;struct fixture *f=c->old.card;unsigned i;
    assert(cl_owned(context)&&f->reservation.access);++f->writes;
    if(reg==0x14)f->address=value;
    else{assert(reg==0x10&&!(f->address&3)&&f->address<=sizeof(f->ram)-4);
       for(i=0;i<4;++i)f->ram[f->address+i]=(uint8_t)(value>>(24-8*i));}
    return 1;
}
static int cl_completed_same(const struct pt_mixed_causal_completed_value *a,
    const struct pt_mixed_causal_completed_value *b)
{
    unsigned i;
    if(!cc_identity(&a->identity,&b->identity)||a->frame!=b->frame||a->first_tick!=b->first_tick||
       a->last_tick!=b->last_tick||a->observed!=b->observed||a->issued!=b->issued||
       a->post.active_mask!=b->post.active_mask||a->post.adopted_mask!=b->post.adopted_mask)return 0;
    for(i=0;i<20;++i)if(!keys_equal(a->post.slot+i,b->post.slot+i))return 0;
    return 1;
}
static void cl_capture(struct pt_mixed_causal_completed_value *out,
    const struct pt_mixed_causal_command_identity *id,const struct pt_mixed_causal_packet *packet,
    uint64_t observed,uint64_t issued,const struct pt_mixed_causal_actual *actual)
{
    memset(out,0,sizeof(*out));out->identity=*id;out->frame=packet->frame;
    out->first_tick=packet->first;out->last_tick=packet->last;out->observed=observed;out->issued=issued;out->post=*actual;
}
static int cl_bind(void *context,const struct pt_mixed_causal_registration *registration)
{
    struct cl_port *p=context;struct cl_trial *c=p->owner;struct pt_editor_mixed_causal_prepare *s=cl_s(c);
    assert(++p->binds==1&&c->old.ordinary.calls==2&&cs_live(&c->old.ordinary)==2);
    assert(s->causal&&s->queue&&!s->pool&&registration->owner==s->causal&&registration->queue==s->queue);
    assert(registration->session==31&&registration->generation==17);
    assert(!pt_mixed_readers_commands_held(s->queue)&&!pt_mixed_readers_readers_held(s->queue));
    p->model.base.registration=*registration;return 1;
}
static int cl_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct cl_port *p=context;return ct_clock(&p->model.base,ticks,frequency);}
static int cl_publish(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_command_identity *id,const struct pt_mixed_causal_packet *packet)
{struct cl_port *p=context;return ct_publish(&p->model.base,owner,id,packet);}
static int cl_publish_successor(void *context,struct pt_mixed_causal_owner *owner,const struct pt_mixed_causal_publication *d)
{struct cl_port *p=context;return ct_publish_successor(&p->model.base,owner,d);}
static int cl_publish_control(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_control_publication *d)
{
    struct cl_port *p=context;struct cc_port *m=&p->model;unsigned i;int raw=p->control_raw;
    ++p->control_publications;
    if(!m->first_completed||owner!=m->base.registration.owner||d->serial!=1||
       !cc_identity(&d->predecessor,&p->root.identity)||
       d->first_tick!=p->root.first_tick||d->last_tick!=p->root.last_tick||
       d->observed!=p->root.observed||d->issued!=p->root.issued||
       d->first_post.active_mask!=m->base.mask||d->first_post.adopted_mask!=m->base.mask||
       !ct_expected(&m->base,&d->packet))return 0;
    for(i=0;i<20;++i)if(!keys_equal(d->first_post.slot+i,p->root.post.slot+i)||
        !keys_equal(d->first_post.slot+i,m->base.slot+i))return 0;
    for(i=0;i<d->packet.count;++i)if(d->packet.action[i].kind!=PT_MIXED_READERS_CONTROL||
        d->packet.key[i].trigger!=p->root.identity.ticket||!ct_reader(&m->base,d->packet.key+i))return 0;
    if(raw&&!ct_publish(&m->base,owner,&d->successor,&d->packet))return 0;
    if(p->task_hook){p->task_hook=0;cl_reenter(p->owner);}return raw;
}
static int cl_publish_stop(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_control_stop_publication *d)
{
    struct cl_port *p=context;struct cc_port *m=&p->model;unsigned i;int raw=p->third_raw;
    ++p->third_publications;
    if(!m->first_completed||!p->control_completed||owner!=m->base.registration.owner||d->serial!=2||
       !cl_completed_same(&d->root,&p->root)||!cl_completed_same(&d->control,&p->control)||
       d->control.identity.ticket==d->root.identity.ticket||d->control.frame<=d->root.frame||
       d->packet.frame<=d->control.frame||!ct_expected(&m->base,&d->packet)||
       d->control.post.active_mask!=m->base.mask||d->control.post.adopted_mask!=m->base.mask)return 0;
    for(i=0;i<20;++i)if(!keys_equal(d->control.post.slot+i,m->base.slot+i))return 0;
    for(i=0;i<d->packet.count;++i){const struct pt_mixed_readers_action *a=d->packet.action+i;
       int zero=a->route==PT_MIXED_READERS_PAULA?
          !a->geometry.paula.data&&!a->geometry.paula.words&&!a->geometry.paula.period&&!a->geometry.paula.volume:
          !a->geometry.amigus.start&&!a->geometry.amigus.loop&&!a->geometry.amigus.end_exclusive&&
          !a->geometry.amigus.rate&&!a->geometry.amigus.control&&!a->geometry.amigus.left&&!a->geometry.amigus.right;
       if(a->kind!=PT_MIXED_READERS_STOP||!zero||d->packet.key[i].trigger!=p->root.identity.ticket||
          !ct_reader(&m->base,d->packet.key+i))return 0;
    }
    if(raw&&!ct_publish(&m->base,owner,&d->stop,&d->packet))return 0;
    if(raw)p->third=d->stop;
    if(p->task_hook){p->task_hook=0;cl_reenter(p->owner);}return raw;
}
static int cl_commit(void *context,const struct pt_mixed_causal_packet *packet,struct pt_mixed_causal_actual *actual)
{
    struct cl_port *p=context;struct ct_command *command=ct_command(&p->model.base,packet->ticket);
    struct pt_mixed_causal_command_identity id={0};uint64_t before=p->model.base.ticks;unsigned i,matched=1;int raw;
    if(command)id=command->identity;
    if(packet->action[0].kind==PT_MIXED_READERS_STOP){if(!p->control_completed)return 0;return ss_commit(&p->model,packet,actual);}
    raw=cc_commit(&p->model,packet,actual);
    if(raw==1&&before>=packet->first&&p->model.base.ticks<packet->last&&
       actual->active_mask==p->model.base.mask&&actual->adopted_mask==p->model.base.mask){
       for(i=0;i<20;++i)if(!keys_equal(actual->slot+i,p->model.base.slot+i))matched=0;
       if(matched){if(packet->action[0].kind==PT_MIXED_READERS_TRIGGER)cl_capture(&p->root,&id,packet,before,p->model.base.ticks,actual);
          else{cl_capture(&p->control,&id,packet,before,p->model.base.ticks,actual);p->control_completed=1;}}
    }
    return raw;
}
static int cl_command_quiet(void *context,const struct pt_mixed_causal_command_identity *id,unsigned cancel)
{
    struct cl_port *p=context;
    if(!cancel&&((p->hold_control&&id->ticket==p->control.identity.ticket)||
       (p->hold_third&&id->ticket==p->third.ticket))){++p->model.base.command_proofs;return 0;}
    return cc_command_quiet(&p->model,id,cancel);
}
static int cl_reader_quiet(void *context,const struct pt_mixed_causal_reader_identity *id,unsigned cancel)
{struct cl_port *p=context;return ss_reader_quiet(&p->model,id,cancel);}
static int cl_source_close(void *context,const struct pt_mixed_causal_registration *id)
{struct cl_port *p=context;return ct_source_close(&p->model.base,id);}
static int cl_source_quiet(void *context,const struct pt_mixed_causal_registration *id)
{
    struct cl_port *p=context;
    if(p->quiet_raw!=1){++p->model.base.probes;assert(ct_registration(&p->model.base.registration,id)&&ct_empty(&p->model.base));return p->quiet_raw;}
    return ct_source_quiet(&p->model.base,id);
}

/* Original cs_make resource construction copied literally except allocation
 * of the containing cl_trial, lineage-aware owned/write callback identities,
 * and the appended opt-in setup. No
 * borrowed master body is moved or freed before its genuine lifetime ends. */
static struct cl_trial *cl_make(unsigned bits,unsigned cache_bits,unsigned little)
{
    struct cl_trial *c=calloc(1,sizeof(*c));struct cs_trial *f=(void *)c;struct pt_allocator a;struct pt_amigus_reservation_api api;
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
    assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,cl_owned,cl_write));
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
    f->saved=cs_save(f,&f->saved_bytes);
    ++cl_cases;c->port.owner=c;c->port.model.base.ticks=100;c->port.model.base.frequency=709379;
    c->port.model.base.commit_raw=c->port.model.base.source_raw=1;
    c->port.control_raw=c->port.third_raw=c->port.quiet_raw=1;
    c->change=calloc(1,sizeof(*c->change));c->stop=calloc(1,sizeof(*c->stop));
    c->command=calloc(3,sizeof(*c->command));assert(c->change&&c->stop&&c->command);
    memcpy(&c->input.original,&f->input.original,sizeof(c->input.original));
    c->input.original.causal.allocator=(struct pt_allocator){&f->ordinary,cl_allocate,cl_release};
    c->input.original.causal.port=(struct pt_mixed_causal_port){&c->port,sizeof(c->port),
        PT_MIXED_CAUSAL_PORT_VERSION,PT_MIXED_CAUSAL_PORT_REQUIRED,cl_clock,cl_publish,cl_commit,
        cl_command_quiet,cl_reader_quiet,cl_source_close,cl_source_quiet,cl_publish_successor};
    c->input.original.bind_original=cl_bind;
    c->input.original.chip_allocate=cl_chip_new;c->input.original.chip_release=cl_chip_free;
    c->input.original.contexts=(struct pt_sampler_storage_span){c,sizeof(*c)};
    c->input.lineage=(struct pt_mixed_causal_control_stop_port){&c->port,sizeof(c->port),
        PT_MIXED_CAUSAL_CONTROL_STOP_VERSION,PT_MIXED_CAUSAL_CONTROL_STOP_REQUIRED,
        cl_publish_control,cl_publish_stop};
    return c;
}
static void cl_open(struct cl_trial *c)
{
    enum pt_editor_mixed_readers_result r;unsigned n=0;
    assert(pt_editor_mixed_causal_lineage_prepare_begin(&c->control,&c->input)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(!c->old.ordinary.calls&&c->old.binding->preparation_context==cl_s(c));
    do{r=pt_editor_mixed_causal_prepare_advance_validation(cl_s(c),7);assert(++n<10000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN&&c->old.ordinary.calls==3&&c->port.binds==1&&
       cl_s(c)->original_binding_confirmed&&!c->old.chip.calls&&!c->old.card->writes&&!c->port.model.base.clocks);
}
static void cl_advance(struct cl_trial *c,unsigned index)
{
    enum pt_editor_mixed_readers_result r;unsigned n=0,writes=c->old.card->writes;
    do{r=pt_editor_mixed_causal_prepare_batch_advance(cl_s(c),c->command[index]);assert(++n<2000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN);
    if(index)assert(writes==c->old.card->writes);
}
static uint64_t cl_enqueue(struct cl_trial *c,unsigned index)
{
    uint64_t ticket=777;struct pt_editor_mixed_command_record *record;
    assert(pt_editor_mixed_causal_prepare_enqueue(cl_s(c),c->command[index],&ticket)==PT_MIXED_READERS_OK&&ticket!=777);
    record=cl_s(c)->command+c->command[index].slot;
    assert(record->handle.address&&record->transferred&&record->ticket==ticket);return ticket;
}
static uint64_t cl_first(struct cl_trial *c,unsigned commit)
{
    unsigned i;uint64_t ticket;cl_open(c);cs_requests(&c->old,16);
    assert(pt_editor_mixed_causal_prepare_batch_begin(cl_s(c),960,c->old.request,16,c->command)==PT_EDITOR_MIXED_READERS_PENDING);
    cl_advance(c,0);
    for(i=0;i<16;++i)assert(pt_editor_mixed_causal_prepare_reader_reference(cl_s(c),c->command[0],i,c->old.reader+i)==PT_EDITOR_MIXED_READERS_OPEN);
    ticket=cl_enqueue(c,0);assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[0])==PT_MIXED_READERS_OK);
    c->port.model.base.ticks=oracle(960)-1;assert(pt_mixed_causal_fire(cl_s(c)->causal,ticket)==PT_MIXED_CAUSAL_EARLY);
    if(commit){c->port.model.base.ticks=oracle(960);assert(pt_mixed_causal_fire(cl_s(c)->causal,ticket)==PT_MIXED_CAUSAL_COMMITTED);
       assert(c->port.root.frame==960&&c->port.root.first_tick==oracle(960)&&c->port.root.last_tick==oracle(961)&&
          c->port.root.observed==oracle(960)&&c->port.root.issued==oracle(960));
       c->port.model.hold_first=1;
       assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[0],0,NULL)==PT_MIXED_READERS_PENDING);}
    return ticket;
}
static void cl_control_input(struct cl_trial *c,unsigned count)
{
    unsigned i;memset(c->change,0,sizeof(*c->change));c->change->frame=1920;c->change->count=count;
    for(i=0;i<count;++i){struct pt_editor_mixed_causal_lineage_control_action *a=c->change->action+i;
       a->reader=c->old.reader[i];
       if(c->old.editor->project->channels.track[i].route==PT_PAULA){a->period=(uint16_t)(214+i);a->volume=(uint8_t)(32+i);}
       else{a->rate=0x10000U+i;a->left=(uint8_t)i;a->right=(uint8_t)(250-i);}
    }
}
static void cl_stop_input(struct cl_trial *c,unsigned count)
{
    unsigned i;memset(c->stop,0,sizeof(*c->stop));c->stop->frame=2880;c->stop->count=count;
    for(i=0;i<count;++i)c->stop->reader[i]=c->old.reader[i];
}
static void cl_later_prepare(struct cl_trial *c,unsigned stop)
{
    struct cs_trial *f=&c->old;unsigned ordinary=f->ordinary.calls,chip=f->chip.calls,master=f->masters.calls,writes=f->card->writes,pins=cs_cache_pins(f);
    unsigned index=stop?2:1;enum pt_editor_mixed_readers_result r;
    r=stop?pt_editor_mixed_causal_lineage_stop_prepare_batch_begin(&c->control,c->stop,c->command+index):
       pt_editor_mixed_causal_lineage_control_prepare_batch_begin(&c->control,c->change,c->command+index);
    assert(r==PT_EDITOR_MIXED_READERS_PENDING&&f->ordinary.calls==ordinary+1);
    assert(!cl_s(c)->command[c->command[index].slot].count);
    cl_advance(c,index);
    assert(f->ordinary.calls==ordinary+1&&chip==f->chip.calls&&master==f->masters.calls&&writes==f->card->writes&&pins==cs_cache_pins(f));
}
static uint64_t cl_control(struct cl_trial *c,unsigned count,unsigned commit)
{
    uint64_t ticket;unsigned i;struct ct_command *packet;cl_control_input(c,count);c->port.model.hold_first=1;cl_later_prepare(c,0);
    ticket=cl_enqueue(c,1);assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[1])==PT_MIXED_READERS_OK);
    packet=ct_command(&c->port.model.base,ticket);assert(packet&&packet->packet.frame==1920&&
       packet->packet.first==oracle(1920)&&packet->packet.last==oracle(1921));
    c->port.model.base.ticks=oracle(1920)-1;assert(pt_mixed_causal_fire(cl_s(c)->causal,ticket)==PT_MIXED_CAUSAL_EARLY);
    if(commit){c->port.model.base.ticks=oracle(1920);assert(pt_mixed_causal_fire(cl_s(c)->causal,ticket)==PT_MIXED_CAUSAL_COMMITTED);
       assert(c->port.model.controls==count&&c->port.control.frame==1920&&c->port.control.observed==oracle(1920)&&
          c->port.control.issued==oracle(1920));
       for(i=0;i<count;++i){struct ct_reader *reader=ct_reader(&c->port.model.base,packet->packet.key+i);
          const struct pt_editor_mixed_causal_lineage_control_action *a=c->change->action+i;assert(reader);
          if(reader->action.route==PT_MIXED_READERS_PAULA)assert(reader->action.geometry.paula.period==a->period&&
             reader->action.geometry.paula.volume==a->volume);
          else assert(reader->action.geometry.amigus.rate==a->rate&&reader->action.geometry.amigus.left==a->left&&
             reader->action.geometry.amigus.right==a->right);
       }
    }
    return ticket;
}
static void cl_detach_root(struct cl_trial *c)
{
    void *former=cl_s(c)->command[c->command[0].slot].handle.address;assert(former);
    c->port.model.hold_first=0;
    assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[0],0,NULL)==PT_MIXED_READERS_OK);
    assert(!cl_s(c)->command[c->command[0].slot].handle.address&&c->control.root_detached&&c->control.root_closed);
    /* former is now only an opaque disposed identity; no read/poison occurs. */
}
static void cl_drop(struct cl_trial *c)
{
    struct cs_trial *f=&c->old;struct pt_editor_mixed_causal_prepare *s=cl_s(c);unsigned i,unpublished_seen=0;int closed;
    c->allocation_reentry=c->allocation_mutation=c->scratch_alias=c->port.task_hook=f->owned_hook=0;
    f->ordinary.alias=f->chip.alias=NULL;c->port.model.hold_first=c->port.hold_control=c->port.hold_third=0;
    c->port.model.quiet_mask=~0U;c->port.quiet_raw=1;
    if(s->pool){
       for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(s->command[i].handle.address&&s->command[i].transferred){
          struct pt_editor_mixed_command_ref ref={i,s->command[i].serial};
          enum pt_mixed_readers_result r=pt_editor_mixed_causal_prepare_service_command(s,ref,1,NULL);
          if(r==PT_MIXED_READERS_INVALID){struct pt_editor_mixed_command_record *record=s->command+i;
             assert(c->unpublished_ticket&&!unpublished_seen&&record->ticket==c->unpublished_ticket&&
                ref.slot==c->unpublished_ref.slot&&ref.serial==c->unpublished_ref.serial&&
                record->transferred&&record->handle.address&&!s->first_error);
             ++unpublished_seen;
          }else assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_BACKEND);
       }
       for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(s->reader[i].handle.address&&s->reader[i].ticket){
          struct pt_editor_mixed_reader_ref ref={i,s->reader[i].serial};
          enum pt_mixed_readers_result r=pt_editor_mixed_causal_prepare_service_reader(s,ref,1,NULL);
          assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_BACKEND||r==PT_MIXED_READERS_INVALID);
       }
    }
    if(c->unpublished_ticket)assert(unpublished_seen==1);
    cs_same(f);closed=pt_editor_mixed_causal_prepare_close(s);
    assert(!s->pool&&!s->causal&&!s->queue);
    if(!closed){unsigned calls=f->ordinary.calls,releases=f->ordinary.releases,shutdowns=c->port.model.base.shutdowns;
       assert(pt_editor_mixed_causal_prepare_close(s));
       assert(calls==f->ordinary.calls&&releases==f->ordinary.releases&&shutdowns==c->port.model.base.shutdowns);
    }
    assert(pt_editor_mixed_causal_prepare_close(s));cs_same(f);assert(pt_editor_mixed_detach(f->binding));
    for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    assert(pt_editor_dispose(f->editor));assert(pt_amigus_wavetable_cache_detach(&f->card->cache));
    assert(pt_amigus_reservation_close(&f->card->reservation));pt_document_release(f->document);
    assert(!cs_live(&f->ordinary)&&!cs_live(&f->chip)&&!cs_live(&f->masters));
    assert(f->ordinary.calls==f->ordinary.releases+f->ordinary.refusals&&f->chip.calls==f->chip.releases+f->chip.refusals);
    assert(!f->ordinary.alias_releases&&!f->chip.alias_releases);
    free(c->change);free(c->stop);free(c->command);free(f->saved);free(f->causal_workspace);free(f->factory_workspace);
    free(f->card);free(f->document);free(f->stop);free(f->request);free(f->command);free(f->reader);free(f->editor);free(f->binding);free(c);
}
static void cl_success(unsigned bits,unsigned cache,unsigned all_card,unsigned targets,unsigned order)
{
    struct cl_trial *c=cl_make(bits,cache,0);struct cs_trial *f=&c->old;
    struct pt_editor_mixed_reader_record readers[32];struct pt_mixed_readers_key slots[20];
    struct pt_editor_mixed_command_ref refused={99,999};struct pt_editor_mixed_reader_ref no_reader={99,999};
    unsigned i,ordinary,chip,masters,writes,pins,selected=0;uint64_t root,control,stop,out=777;
    if(all_card)cs_all_card(f);root=cl_first(c,1);memcpy(readers,cl_s(c)->reader,sizeof(readers));
    memcpy(slots,c->port.model.base.slot,sizeof(slots));ordinary=f->ordinary.calls;chip=f->chip.calls;masters=f->masters.calls;
    writes=f->card->writes;pins=cs_cache_pins(f);control=cl_control(c,6,1);
    assert(root!=control&&c->control.stage==2&&pt_mixed_readers_commands_held(cl_s(c)->queue)==2);
    cl_stop_input(c,targets);
    assert(pt_editor_mixed_causal_lineage_stop_prepare_batch_begin(&c->control,c->stop,&refused)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(refused.slot==99&&refused.serial==999&&f->ordinary.calls==ordinary+1&&!c->control.root_closed);
    assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[0],0,NULL)==PT_MIXED_READERS_PENDING);
    cl_detach_root(c);assert(pt_mixed_readers_commands_held(cl_s(c)->queue)==1);
    cl_later_prepare(c,1);stop=cl_enqueue(c,2);
    assert(c->command[2].slot==c->command[0].slot&&c->command[2].serial!=c->command[0].serial&&stop!=root&&stop!=control);
    assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[0])==PT_MIXED_READERS_INVALID);
    assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[0],0,NULL)==PT_MIXED_READERS_INVALID);
    assert(pt_editor_mixed_causal_prepare_reader_reference(cl_s(c),c->command[1],0,&no_reader)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(pt_editor_mixed_causal_prepare_reader_reference(cl_s(c),c->command[2],0,&no_reader)==PT_EDITOR_MIXED_READERS_INVALID&&no_reader.serial==999);
    assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[2])==PT_MIXED_READERS_OK);
    assert(c->port.root.identity.event==c->port.third.event);
    {struct ct_command *p=ct_command(&c->port.model.base,stop);assert(p&&p->packet.frame==2880&&p->packet.first==oracle(2880)&&p->packet.last==oracle(2881));
       selected=0;for(i=0;i<targets;++i)selected|=1U<<ct_index(p->packet.key[i].route,p->packet.key[i].slot);}
    c->port.model.base.ticks=oracle(2880)-1;assert(pt_mixed_causal_fire(cl_s(c)->causal,stop)==PT_MIXED_CAUSAL_EARLY&&!c->port.model.stops);
    c->port.model.base.ticks++;assert(pt_mixed_causal_fire(cl_s(c)->causal,stop)==PT_MIXED_CAUSAL_COMMITTED&&c->port.model.stops==targets);
    assert(c->port.model.stopped_mask==selected&&!(c->port.model.base.mask&selected));
    for(i=0;i<20;++i)if(!(selected&(1U<<i)))assert(keys_equal(slots+i,c->port.model.base.slot+i));
    assert(!memcmp(readers,cl_s(c)->reader,sizeof(readers))&&f->ordinary.calls==ordinary+2&&chip==f->chip.calls&&
       masters==f->masters.calls&&writes==f->card->writes&&pins==cs_cache_pins(f));
    assert(c->high_water<=3+2+16&&pt_mixed_readers_readers_held(cl_s(c)->queue)==16);
    assert(pt_editor_mixed_causal_lineage_stop_prepare_batch_begin(&c->control,c->stop,&refused)==PT_EDITOR_MIXED_READERS_CAPACITY);
    assert(pt_editor_mixed_causal_prepare_enqueue(cl_s(c),c->command[2],&out)==PT_MIXED_READERS_INVALID&&out==777);
    for(i=0;i<16;++i){struct pt_mixed_readers_reader_receipt observed,before;
       struct pt_mixed_readers_key key;enum pt_mixed_readers_result current;
       memset(&observed,0xa5,sizeof(observed));memcpy(&before,&observed,sizeof(before));
       assert(pt_editor_mixed_causal_prepare_service_reader(cl_s(c),f->reader[i],0,&observed)==PT_MIXED_READERS_PENDING);
       /* PENDING never exports a receipt; preserve the caller beforeimage.
        * Independently query actual admission: stopped original R refuses,
        * untouched ACTIVE R still returns its genuine original root key. */
       assert(!memcmp(&observed,&before,sizeof(observed)));
       current=pt_mixed_causal_reader_key(cl_s(c)->causal,root,i,&key);
       if(i<targets)assert(current==PT_MIXED_READERS_STALE);
       else assert(current==PT_MIXED_READERS_OK&&key.trigger==root&&key.action==i);
    }
    if(!order){assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[1],0,NULL)==PT_MIXED_READERS_OK);
       assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[2],0,NULL)==PT_MIXED_READERS_OK);}
    c->port.model.quiet_mask=selected;
    for(i=0;i<targets;++i){assert(pt_editor_mixed_causal_prepare_service_reader(cl_s(c),f->reader[i],0,NULL)==PT_MIXED_READERS_OK);
       if(order)assert(cl_s(c)->reader[f->reader[i].slot].handle.address);}
    if(order){assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[1],0,NULL)==PT_MIXED_READERS_OK);
       assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[2],0,NULL)==PT_MIXED_READERS_OK);}
    cs_same(f);cl_drop(c);
}
static void cl_shape_refusal(unsigned mode)
{
    struct cl_trial *c=cl_make(24,16,0);struct pt_editor_mixed_command_ref out={99,999};
    unsigned ordinary,chip,writes,masters,pins,stage;unsigned char *unaligned=NULL;
    struct pt_editor_mixed_causal_lineage_control_batch *original=c->change;
    enum pt_editor_mixed_readers_result r;
    (void)cl_first(c,mode!=0);cl_control_input(c,6);
    if(mode==1)c->change->action[1].reader=c->change->action[0].reader;
    if(mode==2)++c->change->action[0].reader.serial;
    if(mode==3)c->change->action[15].reader=c->old.reader[15];
    if(mode==4)c->change->frame=UINT64_MAX;
    if(mode==5)c->change->action[0].period=0;
    if(mode==6)c->change->action[0].volume=65;
    if(mode==7)c->change->action[0].rate=1;
    if(mode==8)c->change->action[5].period=214;
    if(mode==9)c->change->action[5].rate=0;
    if(mode==10){unaligned=malloc(sizeof(*c->change)+1);assert(unaligned);
       memcpy(unaligned+1,c->change,sizeof(*c->change));c->change=(void *)(unaligned+1);}
    if(mode==11)c->change->count=0;
    if(mode==12)c->change->frame=960;
    if(mode==13){assert(pt_editor_mixed_causal_prepare_service_reader(cl_s(c),c->old.reader[0],1,NULL)==PT_MIXED_READERS_OK);}
    ordinary=c->old.ordinary.calls;chip=c->old.chip.calls;writes=c->old.card->writes;
    masters=c->old.masters.calls;pins=cs_cache_pins(&c->old);stage=c->control.stage;
    r=pt_editor_mixed_causal_lineage_control_prepare_batch_begin(&c->control,c->change,&out);
    assert(r==PT_EDITOR_MIXED_READERS_INVALID&&out.slot==99&&out.serial==999&&ordinary==c->old.ordinary.calls&&
       chip==c->old.chip.calls&&masters==c->old.masters.calls&&writes==c->old.card->writes&&
       pins==cs_cache_pins(&c->old)&&stage==c->control.stage);
    c->change=original;free(unaligned);cs_same(&c->old);cl_drop(c);
}
static void cl_constructor_refusal(unsigned mode)
{
    struct cl_trial *c=cl_make(16,8,0);struct pt_editor_mixed_causal_lineage_inputs saved;
    struct pt_editor_mixed_causal_lineage_prepare *w=&c->control;
    const struct pt_editor_mixed_causal_lineage_inputs *in=&c->input;
    unsigned char *unaligned=NULL;
    if(mode==0)c->input.original.contexts=(struct pt_sampler_storage_span){w,sizeof(w->original)};
    if(mode==1)c->input.lineage.context=&c->old.ordinary;
    if(mode==2)--c->input.lineage.context_bytes;
    if(mode==3)++c->input.lineage.flags;
    if(mode==4)++c->input.lineage.version;
    if(mode==5)c->input.lineage.publish_control=NULL;
    if(mode==6)c->input.lineage.publish_stop_after_control=NULL;
    if(mode==7)c->control.self=&c->control;
    if(mode==8){struct pt_editor_mixed_causal_lineage_prepare *copy;
       cl_open(c);copy=malloc(sizeof(*copy));assert(copy);memcpy(copy,w,sizeof(*copy));
       assert(pt_editor_mixed_causal_lineage_prepare_begin(copy,in)==PT_EDITOR_MIXED_READERS_INVALID&&!cl_s(c)->first_error);
       free(copy);cl_drop(c);return;}
    if(mode==9){unaligned=calloc(1,sizeof(*w)+1);assert(unaligned);w=(void *)(unaligned+1);}
    if(mode==10){unaligned=calloc(1,sizeof(*in)+1);assert(unaligned);memcpy(unaligned+1,in,sizeof(*in));in=(void *)(unaligned+1);}
    memcpy(&saved,&c->input,sizeof(saved));
    assert(pt_editor_mixed_causal_lineage_prepare_begin(w,in)==PT_EDITOR_MIXED_READERS_INVALID&&
       !memcmp(&saved,&c->input,sizeof(saved))&&!c->old.ordinary.calls&&!c->old.chip.calls&&
       !c->old.card->writes&&!c->port.binds&&!c->old.binding->preparation_context);
    free(unaligned);memset(&c->control,0,sizeof(c->control));cl_drop(c);
}
static void cl_output_alias(unsigned mode)
{
    struct cl_trial *c=cl_make(24,16,0);void *alias;unsigned char before[sizeof(struct pt_editor_mixed_command_ref)];
    unsigned calls,writes;(void)cl_first(c,1);cl_control_input(c,6);
    if(mode==0)alias=(char *)c->old.causal_workspace+c->old.causal_capacity-sizeof(before);
    else if(mode==1)alias=(char *)c->old.factory_workspace+c->old.factory_capacity-sizeof(before);
    else if(mode==2)alias=&c->control.factory;
    else alias=cl_s(c)->reader[c->old.reader[0].slot].handle.address;
    memcpy(before,alias,sizeof(before));calls=c->old.ordinary.calls;writes=c->old.card->writes;
    assert(pt_editor_mixed_causal_lineage_control_prepare_batch_begin(&c->control,c->change,alias)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(!memcmp(before,alias,sizeof(before))&&calls==c->old.ordinary.calls&&writes==c->old.card->writes&&!cl_s(c)->first_error);
    cl_drop(c);
}
static void cl_allocate_refusal(unsigned mode)
{
    struct cl_trial *c=cl_make(24,16,0);struct cs_trial *f=&c->old;
    struct pt_editor_mixed_command_ref *out=c->command+1;
    struct pt_editor_mixed_reader_record readers[32];struct pt_sampler_storage_span guards[PT_EDITOR_MIXED_READERS_GUARDS];
    struct cs_ledger ledger[80];unsigned calls,releases,refusals,live,gcount,chip,masters,writes,pins;
    enum pt_editor_mixed_readers_result r;(void)cl_first(c,1);cl_control_input(c,6);
    *out=(struct pt_editor_mixed_command_ref){99,999};calls=f->ordinary.calls;releases=f->ordinary.releases;
    refusals=f->ordinary.refusals;live=cs_live(&f->ordinary);gcount=cl_s(c)->guard_count;
    chip=f->chip.calls;masters=f->masters.calls;writes=f->card->writes;pins=cs_cache_pins(f);
    memcpy(readers,cl_s(c)->reader,sizeof(readers));memcpy(guards,cl_s(c)->guards,sizeof(guards));
    memcpy(ledger,f->ordinary.live,sizeof(ledger));
    if(mode==0)f->ordinary.fail=calls+1;
    if(mode==1)f->ordinary.alias=(char *)c->change+sizeof(*c->change)-sizeof(c->change->action[15]);
    if(mode==2)c->allocation_reentry=1;
    if(mode==3)c->allocation_mutation=1;
    if(mode==4)c->scratch_alias=1;
    r=pt_editor_mixed_causal_lineage_control_prepare_batch_begin(&c->control,c->change,out);
    assert(r!=PT_EDITOR_MIXED_READERS_PENDING&&out->slot==99&&out->serial==999&&
       f->ordinary.calls==calls+1&&c->control.stage==2&&!c->control.constructing&&
       !memcmp(readers,cl_s(c)->reader,sizeof(readers))&&
       cl_s(c)->guard_count==gcount&&!memcmp(guards,cl_s(c)->guards,sizeof(guards))&&
       chip==f->chip.calls&&masters==f->masters.calls&&writes==f->card->writes&&pins==cs_cache_pins(f));
    if(mode!=3)assert(!cl_s(c)->command[1].handle.address&&live==cs_live(&f->ordinary)&&
       !memcmp(ledger,f->ordinary.live,sizeof(ledger)));
    else{const struct pt_editor_mixed_command_record *record=cl_s(c)->command+1;
       /* The lower allocator genuinely returned an admitted C2 before the
        * outer original-input comparison failed. Keep that positive ownership
        * registered; caller output refusal is not permission to drop it. */
       assert(record->handle.address&&record->handle.token&&record->serial&&
          !record->transferred&&!record->ticket&&!record->count&&
          c->control.control.slot==1&&c->control.control.serial==record->serial&&
          cs_live(&f->ordinary)==live+1&&f->ordinary.releases==releases);
    }
    if(mode==0)assert(r==PT_EDITOR_MIXED_READERS_CAPACITY&&!cl_s(c)->first_error&&
       f->ordinary.refusals==refusals+1&&releases==f->ordinary.releases);
    else assert(cl_s(c)->first_error);
    if(mode==1||mode==4)assert(f->ordinary.refusals==refusals+1&&releases==f->ordinary.releases&&!f->ordinary.alias_releases);
    if(mode==2)assert(f->ordinary.releases==releases+1);
    if(mode==4)assert(c->scratch_hits==1&&!c->scratch_alias);
    if(mode==3)--c->change->frame;
    /* Lifetime stage is consumed even after NULL/refused alias; no implicit
     * fresh preparation, allocation, upload or reader reconstruction. */
    assert(pt_editor_mixed_causal_lineage_control_prepare_batch_begin(&c->control,c->change,out)!=PT_EDITOR_MIXED_READERS_PENDING&&
       f->ordinary.calls==calls+1);cs_same(f);cl_drop(c);
}
static void cl_publication_outcome(unsigned mode)
{
    struct cl_trial *c=cl_make(24,16,0);struct pt_editor_mixed_reader_record before[32];
    unsigned later=mode>=3,index=later?2:1;uint64_t ticket;enum pt_mixed_readers_result r;
    (void)cl_first(c,1);
    if(later){(void)cl_control(c,6,1);cl_detach_root(c);cl_stop_input(c,2);}
    else cl_control_input(c,6);
    cl_later_prepare(c,later);memcpy(before,cl_s(c)->reader,sizeof(before));ticket=cl_enqueue(c,index);
    if(mode%3==0){if(later)c->port.third_raw=0;else c->port.control_raw=0;}
    if(mode%3==1){if(later)c->port.third_raw=-1;else c->port.control_raw=-1;}
    if(mode%3==2)c->port.task_hook=1;
    r=pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[index]);
    assert(r==(mode%3==0?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_BACKEND)&&
       !memcmp(before,cl_s(c)->reader,sizeof(before))&&pt_mixed_readers_readers_held(cl_s(c)->queue)==16&&
       cl_s(c)->command[c->command[index].slot].transferred&&!c->port.model.stops);
    assert((later?c->port.third_publications:c->port.control_publications)==1);
    if(mode%3==0){assert(!cl_s(c)->first_error&&!ct_command(&c->port.model.base,ticket));
       c->unpublished_ref=c->command[index];c->unpublished_ticket=ticket;}
    else{assert(ct_command(&c->port.model.base,ticket));
       if(mode%3==2)assert(cl_s(c)->first_error);}
    cs_same(&c->old);cl_drop(c);
}
static void cl_issue_failure(unsigned mode)
{
    struct cl_trial *c=cl_make(24,16,0);uint64_t ticket;unsigned calls,chip,masters,writes,pins;
    (void)cl_first(c,1);(void)cl_control(c,6,1);cl_detach_root(c);cl_stop_input(c,2);cl_later_prepare(c,1);
    ticket=cl_enqueue(c,2);assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[2])==PT_MIXED_READERS_OK);
    calls=c->old.ordinary.calls;chip=c->old.chip.calls;masters=c->old.masters.calls;writes=c->old.card->writes;pins=cs_cache_pins(&c->old);
    c->port.model.base.ticks=oracle(2880);
    if(mode==0)c->port.model.base.commit_raw=0;
    if(mode==1)c->port.model.base.commit_raw=-1;
    if(mode==2)c->port.model.base.late=1;
    if(mode==3)c->port.model.base.malformed=1;
    if(mode==4)c->port.model.base.ticks=oracle(2881);
    assert(pt_mixed_causal_fire(cl_s(c)->causal,ticket)!=PT_MIXED_CAUSAL_COMMITTED&&
       pt_mixed_readers_readers_held(cl_s(c)->queue)==16&&calls==c->old.ordinary.calls&&
       chip==c->old.chip.calls&&masters==c->old.masters.calls&&writes==c->old.card->writes&&pins==cs_cache_pins(&c->old));
    cs_same(&c->old);cl_drop(c);
}
static void cl_incomplete_control(void)
{
    struct cl_trial *c=cl_make(16,16,0);struct pt_editor_mixed_command_ref out={99,999};unsigned calls;
    (void)cl_first(c,1);(void)cl_control(c,6,0);cl_detach_root(c);cl_stop_input(c,2);calls=c->old.ordinary.calls;
    assert(c->control.root_detached&&c->control.root_closed&&!c->port.control_completed&&
       pt_editor_mixed_causal_lineage_stop_prepare_batch_begin(&c->control,c->stop,&out)==PT_EDITOR_MIXED_READERS_INVALID&&
       out.slot==99&&out.serial==999&&calls==c->old.ordinary.calls&&c->control.stage==2);
    cl_drop(c);
}
static void cl_positive_enqueue_fault(void)
{
    struct cl_trial *c=cl_make(24,16,0);struct pt_editor_mixed_reader_record readers[32];
    uint64_t ticket=777;struct pt_editor_mixed_command_record *record;
    (void)cl_first(c,1);cl_control_input(c,6);cl_later_prepare(c,0);
    memcpy(readers,cl_s(c)->reader,sizeof(readers));c->old.owned_hook=1;c->old.owned_after=c->change->count+1;
    /* CONTROL first checks every selected actual reader key. Inject outer
     * reentry at the next genuine callback: lower queue holder validation,
     * after those preliminary checks. Actual enqueue OK remains required. */
    assert(pt_editor_mixed_causal_prepare_enqueue(cl_s(c),c->command[1],&ticket)==PT_MIXED_READERS_OK&&ticket!=777);
    record=cl_s(c)->command+c->command[1].slot;
    assert(!c->old.owned_hook&&cl_s(c)->first_error&&record->transferred&&record->ticket==ticket&&
       record->handle.address&&record->handle.token&&c->control.control_ticket==ticket&&
       !memcmp(readers,cl_s(c)->reader,sizeof(readers))&&pt_mixed_readers_commands_held(cl_s(c)->queue)==2);
    assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[1])==PT_MIXED_READERS_INVALID);
    cs_same(&c->old);cl_drop(c);
}
static void cl_source_pending(int raw)
{
    struct cl_trial *c=cl_make(24,16,0);unsigned releases,i;
    (void)cl_first(c,1);(void)cl_control(c,6,1);cl_detach_root(c);cl_stop_input(c,1);cl_later_prepare(c,1);
    {uint64_t ticket=cl_enqueue(c,2);assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[2])==PT_MIXED_READERS_OK);
       c->port.model.base.ticks=oracle(2880);assert(pt_mixed_causal_fire(cl_s(c)->causal,ticket)==PT_MIXED_CAUSAL_COMMITTED);}
    c->port.model.quiet_mask=~0U;
    assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[1],0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[2],0,NULL)==PT_MIXED_READERS_OK);
    /* Explicit shutdown retires every original R, including untouched ACTIVE
     * readers; setting the quiet mask alone is not a retirement receipt. */
    for(i=0;i<16;++i)assert(pt_editor_mixed_causal_prepare_service_reader(cl_s(c),c->old.reader[i],1,NULL)==PT_MIXED_READERS_OK);
    assert(!pt_mixed_readers_commands_held(cl_s(c)->queue)&&!pt_mixed_readers_readers_held(cl_s(c)->queue));
    c->port.model.base.source_raw=raw;c->port.quiet_raw=0;
    assert(!pt_editor_mixed_causal_prepare_close(cl_s(c))&&!cl_s(c)->pool&&!cl_s(c)->queue&&cl_s(c)->causal&&
       cs_live(&c->old.ordinary)==1&&c->old.binding->preparation_context==cl_s(c));
    releases=c->old.ordinary.releases;assert(!pt_editor_dispose(c->old.editor)&&c->old.ordinary.releases==releases);
    c->port.quiet_raw=1;assert(pt_editor_mixed_causal_prepare_close(cl_s(c))&&!cl_s(c)->causal&&
       c->port.model.base.shutdowns==1&&c->old.ordinary.releases==releases+1);cs_same(&c->old);cl_drop(c);
}
int main(void)
{
    unsigned i;
    cl_success(8,8,0,16,0);cl_success(16,16,0,1,1);cl_success(24,16,0,16,1);
    cl_success(8,16,1,16,1);cl_success(16,8,1,1,0);cl_success(24,16,1,16,0);
    for(i=0;i<14;++i)cl_shape_refusal(i);
    for(i=0;i<11;++i)cl_constructor_refusal(i);
    for(i=0;i<4;++i)cl_output_alias(i);
    for(i=0;i<5;++i)cl_allocate_refusal(i);
    for(i=0;i<6;++i)cl_publication_outcome(i);
    for(i=0;i<5;++i)cl_issue_failure(i);
    cl_incomplete_control();cl_positive_enqueue_fault();cl_source_pending(0);cl_source_pending(-1);
    assert(cl_cases==55&&cs_cases==55);
    puts("EDITOR MIXED CAUSAL LINEAGE PREPARE PASS:55 genuine controller instances;6 mixed/card8/16/24 master and8/16cache TRIGGER-CONTROL-STOP schedules;14 shape/ACTIVE/completion refusals;11 whole constructor guards;4 output aliases;5 consumed allocation/reentry/input/scratch refusals;6 typed publication outcomes;5 partial/unknown/late STOP issues;1 actual incomplete CONTROL;1 actual enqueue OK amid outer fault;2 independent SOURCE quiet cases;original960/1920/2880 windows,genuine C1 detach/NULL close,fresh disposed-slot identity,zero new readers/cache/upload,exact master/save custody;SOFTWARE_ONLY");
    return 0;
}
