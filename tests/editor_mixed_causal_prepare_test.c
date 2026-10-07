/* Private SOURCE proposal: Root alone compiles/runs. The focused entry uses
 * genuine project/editor/sampler/factory/causal owner/queue production units.
 * Existing causal/resource test helpers are included; both old entries are
 * renamed and UNCALLED. No activation_test constructor or mirrored core body.
 * ct_port is an ordinary-RAM adapter model, not a deadline/device backend. */
#define PT_MIXED_CAUSAL_TEST_MAIN inherited_causal_pair_suite_not_called
#include "mixed_readers_causal_test.c"
#undef PT_MIXED_CAUSAL_TEST_MAIN
#include "../src/editor/editor_mixed_causal_prepare.h"
#include "../src/editor/editor_mixed_causal_source_internal.h"
#include "../src/core/mixed_readers_causal_factory_internal.h"

struct cp_trial;
struct cp_bus {struct cp_trial *owner;};
struct cp_ledger {void *p;size_t n;};
struct cp_memory {
    struct cp_trial *owner;
    unsigned calls,releases,fail,hook,alias_releases;
    void *alias;
    struct cp_ledger live[80];
};
struct cp_port {
    struct cp_trial *owner;
    struct ct_port model;
    unsigned bind_calls,bind_mode,task_hook,quiet_hook;
    int bind_raw,quiet_raw;
};
struct cp_trial {
    struct pt_editor_mixed_causal_prepare control;
    struct pt_editor_mixed_causal_prepare_inputs input;
    struct cp_memory ordinary,masters,chip;
    struct cp_port port;
    struct cp_bus bus;
    struct fake library;
    struct fixture *card;
    struct pt_document *document;
    struct pt_editor *editor;
    struct pt_editor_mixed *binding;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_pcm pcm[PT_PROJECT_SAMPLES];
    struct pt_editor_mixed_readers_request *request;
    struct pt_editor_mixed_command_ref *command;
    struct pt_editor_mixed_reader_ref *reader;
    int32_t original[2][64];
    void *causal_workspace,*factory_workspace;
    size_t causal_capacity,factory_capacity;
    uint8_t *saved;size_t saved_bytes;
    unsigned bits,cache_bits,little,count,owned_hook,write_hook,reentered;
};
static unsigned cp_cases;
static void cp_reenter(struct cp_trial *f)
{
    uint32_t revision=f->editor->history.revision,generation=f->editor->sampler.generation;
    enum pt_editor_mixed_readers_result before=f->control.first_error;
    assert(pt_editor_mixed_causal_prepare_get(&f->control)==(before?before:PT_EDITOR_MIXED_READERS_FAULT));
    assert(!pt_editor_mixed_causal_prepare_close(&f->control));
    assert(!pt_editor_prepare_change(f->editor)&&!pt_editor_dispose(f->editor));
    assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation);
    ++f->reentered;
}
static void *cp_new(struct cp_memory *m,size_t n)
{
    void *p;unsigned i;++m->calls;if(m->fail==m->calls)return NULL;
    if(m->alias)return m->alias;
    p=malloc(n);assert(p);
    for(i=0;i<80&&m->live[i].p;++i){}assert(i<80);
    m->live[i]=(struct cp_ledger){p,n};return p;
}
static void cp_free(struct cp_memory *m,void *p,size_t n,unsigned exact)
{
    unsigned i;if(p==m->alias){++m->alias_releases;return;}
    for(i=0;i<80&&m->live[i].p!=p;++i){}assert(i<80&&(!exact||m->live[i].n==n));
    m->live[i]=(struct cp_ledger){NULL,0};++m->releases;free(p);
}
static unsigned cp_live(const struct cp_memory *m)
{unsigned i,n=0;for(i=0;i<80;++i)if(m->live[i].p){assert(m->live[i].n);++n;}return n;}
static void *cp_allocate(void *context,size_t n)
{
    struct cp_memory *m=context;struct cp_trial *f=m->owner;void *p;
    if(m->hook==m->calls+1){m->hook=0;cp_reenter(f);}
    assert(f->binding->preparation_context==&f->control&&f->binding->preparation_close);
    p=cp_new(m,n);return p;
}
static void cp_release(void *context,void *p)
{
    struct cp_memory *m=context;struct cp_trial *f=m->owner;unsigned i;
    for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)assert(f->control.ordinary[i].data!=p);
    if(m->hook){m->hook=0;cp_reenter(f);}cp_free(m,p,0,0);
}
static void *cp_master_new(void *context,size_t n){return cp_new(context,n);}
static void cp_master_free(void *context,void *p){cp_free(context,p,0,0);}
static void *cp_chip_new(void *context,size_t n)
{struct cp_memory *m=context;if(m->hook){m->hook=0;cp_reenter(m->owner);}return cp_new(m,n);}
static void cp_chip_free(void *context,void *p,size_t n)
{struct cp_memory *m=context;unsigned i;for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP;++i)assert(m->owner->control.chip[i].data!=p);
 if(m->hook){m->hook=0;cp_reenter(m->owner);}cp_free(m,p,n,1);}
static int cp_owned(void *context)
{
    struct cp_bus *bus=context;struct cp_trial *f=bus->owner;int r=f->card->healthy==1&&f->library.library&&
        f->library.owner==&f->card->reservation&&f->library.acquired==PT_AMIGUS_WAVETABLE&&f->card->reservation.reserved;
    if(f->owned_hook){f->owned_hook=0;cp_reenter(f);}return r;
}
static int cp_write(void *context,unsigned reg,uint32_t value)
{
    struct cp_bus *bus=context;struct cp_trial *f=bus->owner;struct fixture *c=f->card;unsigned i;
    assert(cp_owned(context)&&c->reservation.access);++c->writes;
    if(reg==0x14)c->address=value;
    else{assert(reg==0x10&&!(c->address&3)&&c->address<=sizeof(c->ram)-4);
        for(i=0;i<4;++i)c->ram[c->address+i]=(uint8_t)(value>>(24-8*i));}
    if(f->write_hook){f->write_hook=0;cp_reenter(f);}return 1;
}
static int cp_bind(void *context,const struct pt_mixed_causal_registration *registration)
{
    struct cp_port *p=context;struct cp_trial *f=p->owner;
    struct pt_mixed_causal_owner *owner=f->control.causal;struct pt_mixed_readers_output *queue=NULL;
    assert(++p->bind_calls==1&&f->ordinary.calls==2&&cp_live(&f->ordinary)==2);
    assert(!f->control.pool&&!f->chip.calls&&!f->card->writes&&!p->model.clocks&&!p->model.publications);
    assert(owner&&f->control.queue&&registration->owner==owner&&registration->queue==f->control.queue);
    assert(registration->session==31&&registration->generation==17);
    assert(!pt_mixed_readers_commands_held(f->control.queue)&&!pt_mixed_readers_readers_held(f->control.queue));
    memcpy(&p->model.registration,registration,sizeof(*registration));
    if(p->bind_mode==1)cp_reenter(f);
    if(p->bind_mode==2){assert(!pt_mixed_causal_close(&owner)&&owner==f->control.causal);}
    if(p->bind_mode==3)++f->input.causal.session;
    if(p->bind_mode==4){assert(pt_mixed_causal_borrow_queue(owner,&queue)==PT_MIXED_READERS_BACKEND&&!queue);}
    return p->bind_raw;
}
static int cp_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct cp_port *p=context;return ct_clock(&p->model,ticks,frequency);}
static int cp_publish(void *context,struct pt_mixed_causal_owner *owner,
 const struct pt_mixed_causal_command_identity *identity,const struct pt_mixed_causal_packet *packet)
{struct cp_port *p=context;int r=ct_publish(&p->model,owner,identity,packet);
 if(p->task_hook){p->task_hook=0;cp_reenter(p->owner);}return r;}
static int cp_publish_successor(void *context,struct pt_mixed_causal_owner *owner,const struct pt_mixed_causal_publication *publication)
{struct cp_port *p=context;int r=ct_publish_successor(&p->model,owner,publication);
 if(p->task_hook){p->task_hook=0;cp_reenter(p->owner);}return r;}
static int cp_commit(void *context,const struct pt_mixed_causal_packet *packet,struct pt_mixed_causal_actual *actual)
{struct cp_port *p=context;assert(!p->model.reenter);return ct_commit(&p->model,packet,actual);}
static int cp_command_quiet(void *context,const struct pt_mixed_causal_command_identity *identity,unsigned cancel)
{struct cp_port *p=context;return ct_command_quiet(&p->model,identity,cancel);}
static int cp_reader_quiet(void *context,const struct pt_mixed_causal_reader_identity *identity,unsigned cancel)
{struct cp_port *p=context;return ct_reader_quiet(&p->model,identity,cancel);}
static int cp_source_close(void *context,const struct pt_mixed_causal_registration *registration)
{struct cp_port *p=context;int r=ct_source_close(&p->model,registration);
 if(p->quiet_hook){p->quiet_hook=0;cp_reenter(p->owner);}return r;}
static int cp_source_quiet(void *context,const struct pt_mixed_causal_registration *registration)
{struct cp_port *p=context;
 if(p->quiet_raw!=1){++p->model.probes;assert(ct_registration(&p->model.registration,registration)&&ct_empty(&p->model));return p->quiet_raw;}
 return ct_source_quiet(&p->model,registration);}
static uint8_t *cp_save(struct cp_trial *f,size_t *n)
{
    size_t used;uint8_t *p;
    assert(pt_project_size(f->editor->project,n)==PT_PROJECT_OK);p=malloc(*n);assert(p);
    assert(pt_project_encode(f->editor->project,p,*n,&used)==PT_PROJECT_OK&&used==*n);return p;
}
static void cp_same(struct cp_trial *f)
{
    size_t n;uint8_t *p;unsigned i;
    f->editor->project->channels.selected=0;p=cp_save(f,&n);assert(n==f->saved_bytes&&!memcmp(p,f->saved,n));free(p);
    for(i=0;i<2;++i){assert(f->editor->project->samples[i].pcm.bits==f->bits);
        assert(!memcmp(f->editor->project->samples[i].pcm.data,f->original[i],12*sizeof(int32_t)));}
}
static struct cp_trial *cp_make(unsigned bits,unsigned cache_bits,unsigned little)
{
    struct cp_trial *f=calloc(1,sizeof(*f));struct pt_allocator a;struct pt_amigus_reservation_api api;
    struct pt_project *p;unsigned i,j;assert(f);++cp_cases;f->bits=bits;f->cache_bits=cache_bits;f->little=little;
    f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
    f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
    f->request=calloc(16,sizeof(*f->request));f->command=calloc(2,sizeof(*f->command));f->reader=calloc(32,sizeof(*f->reader));
    assert(f->card&&f->document&&f->editor&&f->binding&&f->request&&f->command&&f->reader);f->bus.owner=f;f->ordinary.owner=f;f->masters.owner=f;f->chip.owner=f;f->port.owner=f;f->port.bind_raw=1;f->port.quiet_raw=1;
    a=(struct pt_allocator){&f->masters,cp_master_new,cp_master_free};
    pt_document_init(f->document,&a);assert(pt_document_new(f->document,16,SIZE_MAX)==PT_PROJECT_OK);
    p=&f->document->project;
    for(i=0;i<16;++i)p->channels.track[i].route=i<4?PT_PAULA:PT_AMIGUS;
    for(i=0;i<2;++i){int32_t values[]={127,-128,1,-1,3,-3,64,-64,0,2,-2,126};
        for(j=0;j<64;++j)f->original[i][j]=values[j%12]*(int32_t)(1U<<(bits-8));
        p->samples[i].pcm=(struct pt_pcm){f->original[i],64,6,8000,2,(uint8_t)bits};p->samples[i].volume=64;}
    assert(pt_editor_init(f->editor,p));pt_sampler_init(&f->editor->sampler,&a,SIZE_MAX);
    /* An unused already-owned master with real spare PCM capacity exercises
     * the entire capacity ledger, independently of played sample geometry. */
    {struct pt_pcm spare;int32_t *values=cp_new(&f->masters,64*sizeof(int32_t));
        for(i=0;i<64;++i)values[i]=(int32_t)i;
        spare=(struct pt_pcm){values,64,1,8000,1,24};
        assert(pt_sampler_append_owned(&f->editor->sampler,p,&f->editor->history,&spare,&a,"unused spare")==PT_EDIT_OK);
        assert(!spare.data);}
    for(i=0;i<p->sample_count;++i)assert(pt_sampler_pin(&f->editor->sampler,p,i,f->editor->sampler.generation,
        &f->pcm[i],&f->pin[i])==PT_EDIT_OK);
    assert(pt_editor_mixed_attach(f->binding,f->editor));
    f->library.available=f->library.supported=f->library.count=1;f->card->healthy=1;
    api=(struct pt_amigus_reservation_api){&f->library,open_library,close_library,find,supported,reserve,release};
    assert(pt_amigus_reservation_open_resource(&f->card->reservation,&api,0,PT_AMIGUS_WAVETABLE)==PT_AMIGUS_RESERVED);
    assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,cp_owned,cp_write));
    f->input.binding=f->binding;f->input.backend=&f->card->cache;
    f->input.causal.allocator=(struct pt_allocator){&f->ordinary,cp_allocate,cp_release};
    f->input.causal.allocator_context=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
    f->input.causal.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};f->input.causal.session=31;
    f->input.causal.control_budget=pt_mixed_causal_control_size();
    f->input.causal.queue_budget=pt_mixed_readers_control_size();
    f->input.causal.port=(struct pt_mixed_causal_port){&f->port,sizeof(f->port),PT_MIXED_CAUSAL_PORT_VERSION,
        PT_MIXED_CAUSAL_PORT_REQUIRED,cp_clock,cp_publish,cp_commit,cp_command_quiet,cp_reader_quiet,
        cp_source_close,cp_source_quiet,cp_publish_successor};
    f->input.bind_original=cp_bind;
    f->port.model.ticks=100;f->port.model.frequency=709379;
    f->port.model.commit_raw=f->port.model.source_raw=1;
    f->input.chip_context=&f->chip;f->input.chip_allocate=cp_chip_new;f->input.chip_release=cp_chip_free;
    f->input.factory_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
    f->input.chip_budget=4096;f->input.contexts=(struct pt_sampler_storage_span){f,sizeof(*f)};
    f->causal_capacity=pt_mixed_causal_workspace_size()+128;
    f->factory_capacity=pt_sampler_mixed_workspace_size()+128;
    f->causal_workspace=calloc(1,f->causal_capacity);f->factory_workspace=calloc(1,f->factory_capacity);
    assert(f->causal_workspace&&f->factory_workspace);
    f->input.causal_workspace=f->causal_workspace;f->input.causal_capacity=f->causal_capacity;
    f->input.factory_workspace=f->factory_workspace;f->input.factory_capacity=f->factory_capacity;
    f->saved=cp_save(f,&f->saved_bytes);return f;
}
static void cp_open(struct cp_trial *f)
{
    enum pt_editor_mixed_readers_result r;unsigned n=0;
    assert(pt_editor_mixed_causal_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(!f->ordinary.calls&&f->binding->preparation_context==&f->control);
    do{r=pt_editor_mixed_causal_prepare_advance_validation(&f->control,7);assert(++n<10000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN&&f->ordinary.calls==3&&!f->chip.calls&&!f->card->writes&&!f->port.model.clocks);
    assert(f->port.bind_calls==1&&f->control.original_binding_called&&f->control.original_binding_confirmed);
    assert(f->control.original_binding_outcome==1&&f->control.pool&&f->control.causal&&f->control.queue);
}
static void cp_requests(struct cp_trial *f,unsigned n)
{
    unsigned i;memset(f->request,0,16*sizeof(*f->request));f->count=n;
    for(i=0;i<n;++i){struct pt_editor_mixed_readers_request *x=f->request+i;
        x->kind=PT_MIXED_READERS_TRIGGER;x->track=i;x->sample=i%2;x->expected=f->pin[x->sample];x->channel=f->editor->project->channels.track[i].route==PT_AMIGUS?1:0;
        if(f->editor->project->channels.track[i].route==PT_PAULA){x->geometry.paula.period=428;x->geometry.paula.volume=64;}
        else{x->geometry.amigus.bits=f->cache_bits;x->geometry.amigus.little_endian=f->little;
            x->geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,64,128};}}
}
static void cp_prepare(struct cp_trial *f,unsigned ci,uint64_t frame)
{
    enum pt_editor_mixed_readers_result r;unsigned n=0,writes;
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,frame,f->request,f->count,f->command+ci)==PT_EDITOR_MIXED_READERS_PENDING);
    do{writes=f->card->writes;r=pt_editor_mixed_causal_prepare_batch_advance(&f->control,f->command[ci]);
        assert(f->card->writes-writes<=128&&++n<2000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN);
}
static void cp_refs(struct cp_trial *f,unsigned ci,unsigned base)
{
    unsigned i;for(i=0;i<f->count;++i)assert(pt_editor_mixed_causal_prepare_reader_reference(&f->control,f->command[ci],i,
        f->reader+base+i)==PT_EDITOR_MIXED_READERS_OPEN);
}
static void cp_barrier(struct cp_trial *f)
{
    uint32_t revision=f->editor->history.revision,generation=f->editor->sampler.generation;
    assert(f->binding->preparation_context==&f->control&&f->binding->preparation_close);
    /* In a continuing success path, inspect hook ownership only. A real edit
     * request intentionally cancels work via finish; separate veto cases below
     * exercise that behavior instead of normalizing live success state. */
    assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation);
}
static void cp_drop(struct cp_trial *f)
{
    unsigned i,calls,releases,chip_releases,shutdowns,probes;int closed;
    f->ordinary.alias=f->chip.alias=NULL;f->ordinary.hook=f->chip.hook=0;
    f->owned_hook=f->write_hook=f->port.task_hook=f->port.quiet_hook=0;
    cp_same(f);closed=pt_editor_mixed_causal_prepare_close(&f->control);
    assert(!f->control.pool&&!f->control.causal&&!f->control.queue);
    if(!closed){
        /* An actual consumed child with close0 is terminal. This later call
         * only completes the retained outer hook; no shutdown/free repetition. */
        cp_barrier(f);calls=f->ordinary.calls;releases=f->ordinary.releases;
        chip_releases=f->chip.releases;shutdowns=f->port.model.shutdowns;probes=f->port.model.probes;
        assert(pt_editor_mixed_causal_prepare_close(&f->control));
        assert(calls==f->ordinary.calls&&releases==f->ordinary.releases&&chip_releases==f->chip.releases&&
            shutdowns==f->port.model.shutdowns&&probes==f->port.model.probes);
    }
    assert(pt_editor_mixed_causal_prepare_close(&f->control));cp_same(f);
    assert(pt_editor_mixed_detach(f->binding));
    for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    assert(pt_editor_dispose(f->editor));
    assert(pt_amigus_wavetable_cache_detach(&f->card->cache));assert(pt_amigus_reservation_close(&f->card->reservation));
    pt_document_release(f->document);
    assert(!cp_live(&f->ordinary)&&!cp_live(&f->chip)&&!cp_live(&f->masters));
    assert(f->ordinary.calls==f->ordinary.releases&&f->chip.calls==f->chip.releases);
    assert(!f->ordinary.alias_releases&&!f->chip.alias_releases);
    free(f->saved);free(f->causal_workspace);free(f->factory_workspace);free(f->card);free(f->document);
    free(f->request);free(f->command);free(f->reader);free(f->editor);free(f->binding);free(f);
}
static uint64_t cp_admit(struct cp_trial *f,unsigned ci)
{
    uint64_t ticket=999;struct pt_editor_mixed_command_record *record;
    assert(pt_editor_mixed_causal_prepare_enqueue(&f->control,f->command[ci],&ticket)==PT_MIXED_READERS_OK&&ticket!=999);
    record=f->control.command+f->command[ci].slot;
    assert(record->ticket==ticket&&record->transferred&&record->handle.address);
    assert(pt_editor_mixed_causal_prepare_publish(&f->control,f->command[ci])==PT_MIXED_READERS_OK);
    return ticket;
}
static void cp_observe(struct cp_trial *f,unsigned base,unsigned count)
{
    unsigned i,releases=f->ordinary.releases,chip=f->chip.releases,commands=f->port.model.command_proofs;
    for(i=0;i<count;++i){struct pt_editor_mixed_reader_record *r=f->control.reader+f->reader[base+i].slot;
        void *original=r->handle.address;uint64_t token=r->handle.token;
        assert(original&&token&&r->ticket);
        assert(pt_editor_mixed_causal_prepare_service_reader(&f->control,f->reader[base+i],0,NULL)==PT_MIXED_READERS_PENDING);
        assert(r->handle.address==original&&r->handle.token==token&&r->ticket);
    }
    assert(releases==f->ordinary.releases&&chip==f->chip.releases&&commands==f->port.model.command_proofs);
    cp_same(f);
}
static void cp_drain_readers(struct cp_trial *f,unsigned base,unsigned count,unsigned failed)
{
    unsigned i;enum pt_mixed_readers_result expected=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
    for(i=0;i<count;++i)assert(pt_editor_mixed_causal_prepare_service_reader(&f->control,f->reader[base+i],1,NULL)==expected);
    /* Positive R proof with retained C references can leave the real factory
     * handle alive; only later actual NULL consumption closes registration. */
}
static void cp_drain_command(struct cp_trial *f,unsigned ci,unsigned failed)
{
    assert(pt_editor_mixed_causal_prepare_service_command(&f->control,f->command[ci],1,NULL)==
        (failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    assert(!f->control.command[f->command[ci].slot].handle.address);
}
static void cp_all_card(struct cp_trial *f)
{
    unsigned i;for(i=0;i<16;++i)f->editor->project->channels.track[i].route=PT_AMIGUS;
    free(f->saved);f->saved=cp_save(f,&f->saved_bytes);
}
static void cp_success(unsigned bits,unsigned cache_bits,unsigned order,unsigned card_only)
{
    struct cp_trial *f=cp_make(bits,cache_bits,order);uint64_t first,second;unsigned i,refs;
    struct pt_mixed_causal_diagnostic diagnostic;
    if(card_only)cp_all_card(f);
    cp_open(f);cp_requests(f,16);cp_prepare(f,0,960);cp_refs(f,0,0);first=cp_admit(f,0);
    cp_prepare(f,1,1920);cp_refs(f,1,16);second=cp_admit(f,1);
    assert(first!=second&&f->port.model.publications==2&&!f->port.model.commits&&!f->port.model.effects);
    assert(pt_mixed_readers_commands_held(f->control.queue)==2&&pt_mixed_readers_readers_held(f->control.queue)==32);
    refs=cp_live(&f->ordinary);assert(refs==37);cp_barrier(f);
    assert(pt_mixed_causal_diagnostic(f->control.causal,&diagnostic)&&diagnostic.admitted&&diagnostic.published&&!diagnostic.completed);
    f->port.model.ticks=oracle(960)-1;
    assert(pt_mixed_causal_fire(f->control.causal,second)==PT_MIXED_CAUSAL_EARLY&&!f->port.model.commits);
    assert(pt_mixed_causal_fire(f->control.causal,first)==PT_MIXED_CAUSAL_EARLY&&!f->port.model.effects);
    f->port.model.ticks=oracle(960);
    assert(pt_mixed_causal_fire(f->control.causal,first)==PT_MIXED_CAUSAL_COMMITTED&&f->port.model.effects==16);
    cp_observe(f,0,16);assert(cp_live(&f->ordinary)==refs&&pt_mixed_readers_readers_held(f->control.queue)==32);
    if(!order){cp_drain_command(f,0,0);assert(pt_mixed_readers_commands_held(f->control.queue)==1);
        assert(pt_mixed_readers_readers_held(f->control.queue)==32);cp_barrier(f);}
    f->port.model.ticks=oracle(1920)-1;
    assert(pt_mixed_causal_fire(f->control.causal,second)==PT_MIXED_CAUSAL_EARLY&&f->port.model.effects==16);
    f->port.model.ticks=oracle(1920);
    assert(pt_mixed_causal_fire(f->control.causal,second)==PT_MIXED_CAUSAL_COMMITTED&&f->port.model.effects==32);
    cp_observe(f,16,16);cp_barrier(f);
    if(!order)cp_drain_command(f,1,0);
    cp_drain_readers(f,0,16,0);cp_drain_readers(f,16,16,0);
    if(order){cp_drain_command(f,0,0);cp_drain_command(f,1,0);}
    assert(!pt_mixed_readers_commands_held(f->control.queue)&&!pt_mixed_readers_readers_held(f->control.queue));
    for(i=0;i<20;++i)assert(!f->port.model.slot[i].serial);
    assert(ct_empty(&f->port.model)&&cp_live(&f->ordinary)==(order?35U:3U));
    cp_drop(f);
}
static void cp_binding_refusal(unsigned mode,int raw)
{
    struct cp_trial *f=cp_make(24,16,0);enum pt_editor_mixed_readers_result r;
    f->port.bind_mode=mode;f->port.bind_raw=raw;
    assert(pt_editor_mixed_causal_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
    r=pt_editor_mixed_causal_prepare_advance_validation(&f->control,7);
    assert(r==PT_EDITOR_MIXED_READERS_FAULT&&f->control.original_binding_called&&!f->control.original_binding_confirmed);
    assert(f->control.original_binding_outcome==raw&&f->port.bind_calls==1&&f->ordinary.calls==2);
    assert(f->control.causal&&f->control.queue&&!f->control.pool&&cp_live(&f->ordinary)==2);
    assert(!f->chip.calls&&!f->card->writes&&!f->port.model.clocks&&!f->port.model.publications);
    cp_barrier(f);
    assert(pt_editor_mixed_causal_prepare_advance_validation(&f->control,7)==PT_EDITOR_MIXED_READERS_FAULT&&f->port.bind_calls==1);
    if(mode==3)--f->input.causal.session;
    cp_drop(f);
}
static void cp_empty(unsigned before_open)
{
    struct cp_trial *f=cp_make(24,16,0);
    if(before_open){assert(pt_editor_mixed_causal_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
        assert(pt_editor_mixed_causal_prepare_cancel(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
        assert(!f->ordinary.calls&&!f->port.bind_calls&&!f->port.model.shutdowns);}
    else{cp_open(f);cp_barrier(f);assert(f->port.bind_calls==1&&!f->port.model.publications);}
    cp_drop(f);
}
static void cp_refuse_kinds_and_third(void)
{
    struct cp_trial *f=cp_make(24,16,0);struct pt_editor_mixed_command_ref refused={99,999};
    unsigned calls,writes;uint64_t first,second;
    cp_open(f);cp_requests(f,16);calls=f->ordinary.calls;writes=f->card->writes;
    f->request[0].kind=PT_MIXED_READERS_CONTROL;
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,960,f->request,16,&refused)==PT_EDITOR_MIXED_READERS_INVALID);
    f->request[0].kind=PT_MIXED_READERS_STOP;
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,960,f->request,16,&refused)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(refused.slot==99&&refused.serial==999&&calls==f->ordinary.calls&&writes==f->card->writes&&!f->control.first_error);
    cp_requests(f,16);cp_prepare(f,0,960);cp_refs(f,0,0);first=cp_admit(f,0);
    cp_prepare(f,1,1920);cp_refs(f,1,16);second=cp_admit(f,1);
    assert(first&&second);calls=f->ordinary.calls;writes=f->card->writes;
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,2880,f->request,16,&refused)==PT_EDITOR_MIXED_READERS_CAPACITY);
    assert(refused.slot==99&&refused.serial==999&&calls==f->ordinary.calls&&writes==f->card->writes);
    cp_drain_command(f,0,0);cp_drain_command(f,1,0);cp_drain_readers(f,0,16,0);cp_drain_readers(f,16,16,0);
    calls=f->ordinary.calls;
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,2880,f->request,16,&refused)==PT_EDITOR_MIXED_READERS_CAPACITY);
    assert(calls==f->ordinary.calls&&f->control.prepared_batches==2&&!f->port.model.effects);
    cp_drop(f);
}
static void cp_enqueue_outer_fault(void)
{
    struct cp_trial *f=cp_make(24,16,0);uint64_t ticket=999;unsigned i;
    cp_open(f);cp_requests(f,5);cp_prepare(f,0,960);cp_refs(f,0,0);f->owned_hook=1;
    assert(pt_editor_mixed_causal_prepare_enqueue(&f->control,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    assert(ticket!=999&&f->control.command[f->command[0].slot].transferred&&f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT);
    for(i=0;i<5;++i)assert(f->control.reader[f->reader[i].slot].ticket==ticket);
    assert(!f->port.model.publications&&!f->port.model.effects&&pt_mixed_readers_commands_held(f->control.queue)==1);
    cp_barrier(f);cp_drop(f);
}
static void cp_publish_outer_fault(unsigned successor)
{
    struct cp_trial *f=cp_make(24,16,0);uint64_t ticket=999;unsigned ci=successor?1:0;
    cp_open(f);cp_requests(f,5);cp_prepare(f,0,960);cp_refs(f,0,0);
    if(successor){assert(cp_admit(f,0));cp_prepare(f,1,1920);cp_refs(f,1,16);}
    assert(pt_editor_mixed_causal_prepare_enqueue(&f->control,f->command[ci],&ticket)==PT_MIXED_READERS_OK&&ticket!=999);
    f->port.task_hook=1;
    assert(pt_editor_mixed_causal_prepare_publish(&f->control,f->command[ci])==PT_MIXED_READERS_BACKEND);
    assert(f->reentered&&f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT&&f->port.model.publications==(successor?2U:1U));
    cp_barrier(f);assert(!pt_editor_mixed_causal_prepare_close(&f->control)&&f->control.pool&&f->control.causal);
    cp_drain_command(f,0,1);if(successor)cp_drain_command(f,1,1);
    cp_drain_readers(f,0,5,1);if(successor)cp_drain_readers(f,16,5,1);
    cp_drop(f);
}
static void cp_source_pending(int raw)
{
    struct cp_trial *f=cp_make(24,16,0);unsigned before;cp_open(f);f->port.model.source_raw=raw;
    assert(!pt_editor_mixed_causal_prepare_close(&f->control)&&!f->control.pool&&!f->control.queue&&f->control.causal);
    assert(cp_live(&f->ordinary)==1&&f->port.model.shutdowns==1&&!f->port.model.probes);
    cp_barrier(f);before=f->ordinary.releases;
    assert(pt_editor_mixed_causal_prepare_close(&f->control)&&!f->control.causal);
    assert(f->port.model.shutdowns==1&&f->port.model.probes==1&&f->ordinary.releases==before+1);
    cp_drop(f);
}
static void cp_full_guards(unsigned kind)
{
    struct cp_trial *f=cp_make(24,16,0);struct pt_editor_mixed_command_ref out={99,999};
    unsigned calls,writes,proofs;void *alias;
    cp_open(f);cp_requests(f,5);calls=f->ordinary.calls;writes=f->card->writes;proofs=f->port.model.publications;
    alias=kind==0?(void *)((char *)f->causal_workspace+f->causal_capacity-1):
        kind==1?(void *)((char *)f->factory_workspace+f->factory_capacity-1):
        (void *)(f->pcm[31].data+63);
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,960,f->request,5,alias)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(calls==f->ordinary.calls&&writes==f->card->writes&&proofs==f->port.model.publications&&!f->control.first_error);
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,960,f->request,5,&out)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(out.serial!=999);cp_drop(f);
}
static void cp_real_veto(unsigned mode,unsigned order)
{
    struct cp_trial *f=cp_make(24,16,0);uint64_t ticket,successor;unsigned revision,generation,i,held;
    struct pt_event event;
    cp_open(f);cp_requests(f,5);cp_prepare(f,0,960);cp_refs(f,0,0);ticket=cp_admit(f,0);
    /* This owner admits exactly a pair. Both genuine publications precede the
     * first in-window fire; a lone first is intentionally not executable. */
    cp_prepare(f,1,1920);cp_refs(f,1,16);successor=cp_admit(f,1);
    assert(ticket&&successor&&ticket!=successor&&f->port.model.publications==2);
    assert(pt_mixed_readers_commands_held(f->control.queue)==2&&pt_mixed_readers_readers_held(f->control.queue)==10);
    held=cp_live(&f->ordinary);assert(held==15&&!f->port.model.commits&&!f->port.model.effects);
    f->port.model.ticks=oracle(960);
    assert(pt_mixed_causal_fire(f->control.causal,ticket)==PT_MIXED_CAUSAL_COMMITTED);
    assert(f->port.model.commits==1&&f->port.model.effects==5&&cp_live(&f->ordinary)==held);
    revision=f->editor->history.revision;generation=f->editor->sampler.generation;event=f->editor->project->events[0];
    f->port.model.source_raw=0;f->port.quiet_raw=0;
    if(mode==0){f->editor->editing=1;f->editor->panel=0;f->editor->row=0;f->editor->project->channels.selected=0;
        pt_editor_key(f->editor,0x46,0);}
    else if(mode==1)pt_editor_key(f->editor,0x31,8);
    else if(mode==2){f->editor->panel=4;f->editor->channel_details=0;f->editor->project->channels.selected=0;
        pt_editor_key(f->editor,0x20,0);assert(f->editor->project->channels.track[0].route==PT_PAULA);}
    else assert(!pt_editor_dispose(f->editor));
    assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation&&
        !memcmp(&event,f->editor->project->events,sizeof(event))&&f->binding->preparation_context==&f->control);
    assert(f->control.pool&&f->control.causal&&f->control.queue&&cp_live(&f->ordinary)==held);
    assert(pt_mixed_readers_commands_held(f->control.queue)==2&&pt_mixed_readers_readers_held(f->control.queue)==10);
    for(i=0;i<5;++i)assert(f->control.reader[f->reader[i].slot].handle.address&&
        f->control.reader[f->reader[16+i].slot].handle.address);
    if(!order){cp_drain_command(f,0,1);cp_drain_command(f,1,1);}
    cp_drain_readers(f,0,5,1);cp_drain_readers(f,16,5,1);
    if(order){cp_drain_command(f,0,1);cp_drain_command(f,1,1);}
    assert(f->port.model.commits==1&&f->port.model.effects==5);
    assert(!pt_editor_prepare_change(f->editor)&&!f->control.pool&&f->control.causal&&!f->control.queue);
    assert(f->port.model.shutdowns==1&&cp_live(&f->ordinary)==1);
    assert(!pt_editor_dispose(f->editor)&&f->port.model.shutdowns==1&&f->port.model.probes);
    f->port.quiet_raw=1;
    assert(pt_editor_prepare_change(f->editor)&&!f->control.causal&&!f->binding->preparation_context);
    assert(f->port.model.shutdowns==1);
    for(i=0;i<5;++i)assert(!f->control.reader[f->reader[i].slot].handle.address&&
        !f->control.reader[f->reader[16+i].slot].handle.address);
    assert(!f->control.command[f->command[0].slot].handle.address&&!f->control.command[f->command[1].slot].handle.address);
    cp_drop(f);
}
static void cp_direct_missing_owner(void)
{
    struct cp_trial *f=cp_make(24,16,0);struct pt_sampler_mixed_config config={0};
    struct pt_sampler_mixed_pool *output=NULL;uint8_t *before;unsigned calls;
    assert(pt_editor_mixed_causal_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(pt_editor_mixed_causal_prepare_advance_validation(&f->control,7)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(f->control.original_binding_confirmed&&f->ordinary.calls==2&&!f->control.pool);
    config.allocator=f->control.allocator;config.sampler=&f->editor->sampler;config.project=f->editor->project;
    config.queue=f->control.queue;config.backend=&f->card->cache;config.chip_context=&f->chip;
    config.chip_allocate=cp_chip_new;config.chip_release=cp_chip_free;config.control_budget=f->input.factory_budget;
    config.chip_budget=4096;config.generation=17;config.maximum_commands=2;config.maximum_readers=32;
    config.context_count=3;config.contexts[0]=(struct pt_mixed_readers_span){f,sizeof(*f)};
    config.contexts[1]=(struct pt_mixed_readers_span){f->card,sizeof(*f->card)};
    config.contexts[2]=(struct pt_mixed_readers_span){f->causal_workspace,f->causal_capacity};
    before=malloc(f->factory_capacity);assert(before);memcpy(before,f->factory_workspace,f->factory_capacity);calls=f->ordinary.calls;
    assert(pt_sampler_mixed_causal_open(&config,f->control.revision,f->factory_workspace,f->factory_capacity,
        &f->control.factory_binding,&output)==PT_SAMPLER_MIXED_INVALID);
    assert(!output&&calls==f->ordinary.calls&&!memcmp(before,f->factory_workspace,f->factory_capacity)&&!f->control.first_error);
    free(before);cp_drop(f);
}
static void cp_source_scope(void)
{
    struct cp_trial *f=cp_make(24,16,0);struct pt_editor_mixed_causal_source_inputs *source=calloc(1,sizeof(*source));
    struct pt_editor_mixed_causal_source_borrow *borrow=calloc(1,sizeof(*borrow)),copy;
    enum pt_editor_mixed_readers_result result;unsigned i,steps=0;struct pt_editor_mixed_source_observation observation;
    assert(source&&borrow);source->binding=f->binding;source->contexts=f->input.contexts;source->preparation=&f->input;
    source->causal_workspace=(struct pt_sampler_storage_span){f->causal_workspace,f->causal_capacity};
    source->factory_workspace=(struct pt_sampler_storage_span){f->factory_workspace,f->factory_capacity};
    source->backend_parent=(struct pt_sampler_storage_span){f->card,sizeof(*f->card)};
    source->immutable_count=3;source->immutable[0]=source->causal_workspace;
    source->immutable[1]=source->factory_workspace;source->immutable[2]=source->backend_parent;
    source->mutable_count=3;source->mutable[0]=(struct pt_sampler_storage_span){f->request,16*sizeof(*f->request)};
    source->mutable[1]=(struct pt_sampler_storage_span){f->command,2*sizeof(*f->command)};
    source->mutable[2]=(struct pt_sampler_storage_span){f->reader,32*sizeof(*f->reader)};
    assert(pt_editor_mixed_causal_source_begin(&f->control,source,borrow)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(!f->ordinary.calls&&!f->port.bind_calls);cp_barrier(f);copy=*borrow;
    assert(!pt_editor_mixed_causal_source_enter(&copy)&&!pt_editor_mixed_causal_source_borrow_close(&copy));
    assert(pt_editor_mixed_causal_source_enter(borrow)&&pt_editor_mixed_causal_source_leave(borrow));
    assert(pt_editor_mixed_causal_source_activate(&f->control,borrow,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
    do{result=pt_editor_mixed_causal_prepare_advance_validation(&f->control,7);assert(++steps<10000);}while(result==PT_EDITOR_MIXED_READERS_PENDING);
    assert(result==PT_EDITOR_MIXED_READERS_OPEN&&f->port.bind_calls==1);
    assert(!pt_editor_mixed_causal_prepare_close(&f->control)&&!f->control.pool&&!f->control.causal&&!f->control.queue);
    assert(pt_editor_mixed_causal_source_children_closed(&f->control,borrow));cp_barrier(f);
    observation=pt_editor_mixed_causal_source_observe(&f->control,borrow);
    assert(observation.scope==PT_EDITOR_MIXED_SOURCE_SCOPE_HELD);
    for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    assert(pt_editor_mixed_causal_source_borrow_close(borrow)&&!borrow->address&&!borrow->serial);
    assert(pt_editor_mixed_causal_prepare_close(&f->control));cp_drop(f);free(source);free(borrow);
}
#ifndef PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN
#define PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN main
#endif
int PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN(void)
{
    static const unsigned bits[]={8,16,24},cache[]={8,16};unsigned i,j,order;
    for(i=0;i<3;++i)for(j=0;j<2;++j)for(order=0;order<2;++order)cp_success(bits[i],cache[j],order,0);
    for(order=0;order<2;++order)cp_success(24,16,order,1);
    cp_binding_refusal(0,0);cp_binding_refusal(0,-1);cp_binding_refusal(0,2);
    for(i=1;i<=4;++i)cp_binding_refusal(i,1);
    cp_empty(0);cp_empty(1);cp_refuse_kinds_and_third();cp_enqueue_outer_fault();
    cp_publish_outer_fault(0);cp_publish_outer_fault(1);
    cp_source_pending(0);cp_source_pending(-1);cp_source_pending(2);
    for(i=0;i<3;++i)cp_full_guards(i);
    cp_direct_missing_owner();cp_source_scope();
    for(i=0;i<4;++i)for(order=0;order<2;++order)cp_real_veto(i,order);
    assert(cp_cases==43);
    puts("EDITOR MIXED CAUSAL PREPARE PASS:43 genuine typed pure-TRIGGER pair cases; original bind before factory, raw/refusal/reentry retention, literal windows, 4Paula+12card/16card and 8/16/24 masters,32 pinned readers and independent C/R/source quiet, complete guards/actual transfer/sticky pair scope/exact saves/SOURCE borrow; SOFTWARE_ONLY");
    return 0;
}
