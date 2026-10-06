/* Actual editor barrier + established masters + genuine paired factory/owner.
 * Include the committed complete factory/activation/caches/oracle bodies; their
 * independent suites remain observable rather than substituted with mocks. */
#define PT_MIXED_READERS_TEST_MAIN inherited_paired_main
#include "sampler_mixed_activation_test.c"
#undef PT_MIXED_READERS_TEST_MAIN
#include "../src/editor/editor_mixed_readers_prepare.h"

struct emp_memory {
    unsigned calls,releases,fail,hook,alias_releases;
    void *alias;
    struct ledger_test live[80];
};
struct emp_trial {
    struct pt_editor_mixed_readers_prepare control;
    struct pt_editor_mixed_readers_prepare_inputs input;
    struct emp_memory ordinary,masters,chip;
    struct sma_port port;
    struct {struct emp_trial *owner;} bus;
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
    void *activation_workspace,*factory_workspace;
    size_t activation_capacity,factory_capacity;
    uint8_t *saved;size_t saved_bytes;
    unsigned bits,cache_bits,little,count,port_hook,owned_hook,write_hook,reentered;
};
static void emp_reenter(struct emp_trial *f)
{
    uint32_t revision=f->editor->history.revision,generation=f->editor->sampler.generation;
    enum pt_editor_mixed_readers_result before=f->control.first_error;
    assert(pt_editor_mixed_readers_prepare_get(&f->control)==(before?before:PT_EDITOR_MIXED_READERS_FAULT));
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    assert(!pt_editor_prepare_change(f->editor)&&!pt_editor_dispose(f->editor));
    assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation);
    ++f->reentered;
}
static void *emp_new(struct emp_memory *m,size_t n)
{
    void *p;unsigned i;++m->calls;if(m->fail==m->calls)return NULL;
    if(m->alias)return m->alias;
    p=malloc(n);assert(p);
    for(i=0;i<80&&m->live[i].p;++i){}assert(i<80);
    m->live[i]=(struct ledger_test){p,n};return p;
}
static void emp_free(struct emp_memory *m,void *p,size_t n,unsigned exact)
{
    unsigned i;if(p==m->alias){++m->alias_releases;return;}
    for(i=0;i<80&&m->live[i].p!=p;++i){}assert(i<80&&(!exact||m->live[i].n==n));
    m->live[i]=(struct ledger_test){NULL,0};++m->releases;free(p);
}
static void *emp_allocate(void *context,size_t n)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,ordinary));void *p;
    if(f->ordinary.hook==f->ordinary.calls+1){f->ordinary.hook=0;emp_reenter(f);}
    assert(f->binding->preparation_context==&f->control&&f->binding->preparation_close);
    p=emp_new(&f->ordinary,n);return p;
}
static void emp_release(void *context,void *p)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,ordinary));
    unsigned i;for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)assert(f->control.ordinary[i].data!=p);
    if(f->ordinary.hook){f->ordinary.hook=0;emp_reenter(f);}
    emp_free(&f->ordinary,p,0,0);
}
static void *emp_master_new(void *context,size_t n){return emp_new(context,n);}
static void emp_master_free(void *context,void *p){emp_free(context,p,0,0);}
static void *emp_chip_new(void *context,size_t n)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,chip));
    if(f->chip.hook){f->chip.hook=0;emp_reenter(f);}return emp_new(&f->chip,n);
}
static void emp_chip_free(void *context,void *p,size_t n)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,chip));unsigned i;
    for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP;++i)assert(f->control.chip[i].data!=p);
    if(f->chip.hook){f->chip.hook=0;emp_reenter(f);}emp_free(&f->chip,p,n,1);
}
static int emp_owned(void *context)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,bus));int r=f->card->healthy==1&&f->library.library&&
        f->library.owner==&f->card->reservation&&f->library.acquired==PT_AMIGUS_WAVETABLE&&f->card->reservation.reserved;
    if(f->owned_hook){f->owned_hook=0;emp_reenter(f);}return r;
}
static int emp_write(void *context,unsigned reg,uint32_t value)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,bus));struct fixture *c=f->card;unsigned i;
    assert(emp_owned(context)&&c->reservation.access);++c->writes;
    if(reg==0x14)c->address=value;
    else{assert(reg==0x10&&!(c->address&3)&&c->address<=sizeof(c->ram)-4);
        for(i=0;i<4;++i)c->ram[c->address+i]=(uint8_t)(value>>(24-8*i));}
    if(f->write_hook){f->write_hook=0;emp_reenter(f);}return 1;
}
static struct emp_trial *emp_from_port(void *context)
{return (void *)((char *)context- offsetof(struct emp_trial,port));}
static int emp_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct emp_trial *f=emp_from_port(context);int r=sma_clock(context,ticks,frequency);
    if(f->port_hook==1){f->port_hook=0;emp_reenter(f);}
    /* A failed task clock need not reenter or publish to change fixed tags. */
    if(f->port_hook==7||f->port_hook==10){unsigned hook=f->port_hook;
        f->port_hook=0;++f->editor->history.revision;return hook==10?1:0;}return r;
}
static int emp_publish(void *context,struct pt_mixed_readers_activation *owner,const struct pt_mixed_activation_packet *packet)
{
    struct emp_trial *f=emp_from_port(context);int r=sma_publish(context,owner,packet);
    if(f->port_hook==2){f->port_hook=0;emp_reenter(f);}
    if(f->port_hook==9){unsigned before=f->control.reentries;
        f->port_hook=0;++f->editor->history.revision;
        f->editor->project->samples=(void *)(UINTPTR_MAX-7);
        f->editor->project->events=(void *)(UINTPTR_MAX-7);
        f->editor->project->extensions=(void *)(UINTPTR_MAX-7);
        /* Aliased incoming spans cannot make begin write a busy fault. */
        assert(pt_editor_mixed_readers_prepare_begin(&f->control,(void *)(f->pcm[31].data+63))==PT_EDITOR_MIXED_READERS_INVALID);
        assert(f->control.reentries==before&&!f->control.first_error);
        assert(pt_editor_mixed_readers_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_FAULT);
        assert(f->control.reentries==before+1);++f->reentered;
    }
    if(f->port_hook==8){f->port_hook=0;++f->editor->history.revision;}return r;
}
static int emp_command_quiet(void *context,const struct pt_mixed_activation_command_identity *identity,unsigned cancel)
{
    struct emp_trial *f=emp_from_port(context);int r=sma_command_quiet(context,identity,cancel);
    if(f->port_hook==3){f->port_hook=0;emp_reenter(f);}return r;
}
static int emp_reader_quiet(void *context,const struct pt_mixed_activation_reader_identity *identity,unsigned cancel)
{
    struct emp_trial *f=emp_from_port(context);int r=sma_reader_quiet(context,identity,cancel);
    if(f->port_hook==4){f->port_hook=0;emp_reenter(f);}return r;
}
static int emp_source_close(void *context,const struct pt_mixed_activation_registration *registration)
{
    struct emp_trial *f=emp_from_port(context);int r=sma_source_close(context,registration);
    if(f->port_hook==5){f->port_hook=0;emp_reenter(f);}return r;
}
static int emp_source_quiet(void *context,const struct pt_mixed_activation_registration *registration)
{
    struct emp_trial *f=emp_from_port(context);int r=sma_source_quiet(context,registration);
    if(f->port_hook==6){f->port_hook=0;emp_reenter(f);}return r;
}
static uint8_t *emp_save(struct emp_trial *f,size_t *n)
{
    size_t used;uint8_t *p;
    assert(pt_project_size(f->editor->project,n)==PT_PROJECT_OK);p=malloc(*n);assert(p);
    assert(pt_project_encode(f->editor->project,p,*n,&used)==PT_PROJECT_OK&&used==*n);return p;
}
static void emp_same(struct emp_trial *f)
{
    size_t n;uint8_t *p;unsigned i;
    f->editor->project->channels.selected=0;p=emp_save(f,&n);assert(n==f->saved_bytes&&!memcmp(p,f->saved,n));free(p);
    for(i=0;i<2;++i){assert(f->editor->project->samples[i].pcm.bits==f->bits);
        assert(!memcmp(f->editor->project->samples[i].pcm.data,f->original[i],12*sizeof(int32_t)));}
}
static struct emp_trial *emp_make(unsigned bits,unsigned cache_bits,unsigned little)
{
    struct emp_trial *f=calloc(1,sizeof(*f));struct pt_allocator a;struct pt_amigus_reservation_api api;
    struct pt_project *p;unsigned i,j;assert(f);f->bits=bits;f->cache_bits=cache_bits;f->little=little;
    f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
    f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
    f->request=calloc(16,sizeof(*f->request));f->command=calloc(2,sizeof(*f->command));f->reader=calloc(32,sizeof(*f->reader));
    assert(f->card&&f->document&&f->editor&&f->binding&&f->request&&f->command&&f->reader);f->bus.owner=f;
    a=(struct pt_allocator){&f->masters,emp_master_new,emp_master_free};
    pt_document_init(f->document,&a);assert(pt_document_new(f->document,16,SIZE_MAX)==PT_PROJECT_OK);
    p=&f->document->project;
    for(i=0;i<16;++i)p->channels.track[i].route=i<4?PT_PAULA:PT_AMIGUS;
    for(i=0;i<2;++i){int32_t values[]={127,-128,1,-1,3,-3,64,-64,0,2,-2,126};
        for(j=0;j<64;++j)f->original[i][j]=values[j%12]*(int32_t)(1U<<(bits-8));
        p->samples[i].pcm=(struct pt_pcm){f->original[i],64,6,8000,2,(uint8_t)bits};p->samples[i].volume=64;}
    assert(pt_editor_init(f->editor,p));pt_sampler_init(&f->editor->sampler,&a,SIZE_MAX);
    /* An unused already-owned master with real spare PCM capacity exercises
     * the entire capacity ledger, independently of played sample geometry. */
    {struct pt_pcm spare;int32_t *values=emp_new(&f->masters,64*sizeof(int32_t));
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
    assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,emp_owned,emp_write));
    f->input.binding=f->binding;f->input.backend=&f->card->cache;
    f->input.activation.allocator=(struct pt_allocator){&f->ordinary,emp_allocate,emp_release};
    f->input.activation.allocator_context=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
    f->input.activation.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};f->input.activation.session=31;
    f->input.activation.control_budget=pt_mixed_activation_control_size();
    f->input.activation.queue_budget=pt_mixed_readers_control_size();
    f->input.activation.port=(struct pt_mixed_activation_port){&f->port,sizeof(f->port),PT_MIXED_ACTIVATION_PORT_VERSION,
        PT_MIXED_ACTIVATION_PORT_REQUIRED,emp_clock,emp_publish,sma_commit,emp_command_quiet,emp_reader_quiet,emp_source_close,emp_source_quiet};
    f->port.ticks=100;f->port.reader_allow=1;f->port.close_result=f->port.quiet_result=1;
    f->input.chip_context=&f->chip;f->input.chip_allocate=emp_chip_new;f->input.chip_release=emp_chip_free;
    f->input.factory_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
    f->input.chip_budget=4096;f->input.contexts=(struct pt_sampler_storage_span){f,sizeof(*f)};
    f->activation_capacity=pt_mixed_activation_workspace_size()+128;
    f->factory_capacity=pt_sampler_mixed_workspace_size()+128;
    f->activation_workspace=calloc(1,f->activation_capacity);f->factory_workspace=calloc(1,f->factory_capacity);
    assert(f->activation_workspace&&f->factory_workspace);
    f->input.activation_workspace=f->activation_workspace;f->input.activation_capacity=f->activation_capacity;
    f->input.factory_workspace=f->factory_workspace;f->input.factory_capacity=f->factory_capacity;
    f->saved=emp_save(f,&f->saved_bytes);return f;
}
static void emp_open(struct emp_trial *f)
{
    enum pt_editor_mixed_readers_result r;unsigned n=0;
    assert(pt_editor_mixed_readers_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(!f->ordinary.calls&&f->binding->preparation_context==&f->control);
    do{r=pt_editor_mixed_readers_prepare_advance_validation(&f->control,7);assert(++n<10000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN&&f->ordinary.calls==3&&!f->chip.calls&&!f->card->writes&&!f->port.reads);
}
static void emp_requests(struct emp_trial *f,unsigned n)
{
    unsigned i;memset(f->request,0,16*sizeof(*f->request));f->count=n;
    for(i=0;i<n;++i){struct pt_editor_mixed_readers_request *x=f->request+i;
        x->kind=PT_MIXED_READERS_TRIGGER;x->track=i;x->sample=i%2;x->expected=f->pin[x->sample];x->channel=i>=4?1:0;
        if(i<4){x->geometry.paula.period=428;x->geometry.paula.volume=64;}
        else{x->geometry.amigus.bits=f->cache_bits;x->geometry.amigus.little_endian=f->little;
            x->geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,64,128};}}
}
static void emp_prepare(struct emp_trial *f,unsigned ci,uint64_t frame)
{
    enum pt_editor_mixed_readers_result r;unsigned n=0,writes;
    assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,frame,f->request,f->count,f->command+ci)==PT_EDITOR_MIXED_READERS_PENDING);
    do{writes=f->card->writes;r=pt_editor_mixed_readers_prepare_batch_advance(&f->control,f->command[ci]);
        assert(f->card->writes-writes<=128&&++n<2000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN);
}
static void emp_refs(struct emp_trial *f,unsigned ci,unsigned base)
{
    unsigned i;for(i=0;i<f->count;++i)assert(pt_editor_mixed_readers_prepare_reader_reference(&f->control,f->command[ci],i,
        f->reader+base+i)==PT_EDITOR_MIXED_READERS_OPEN);
}
static uint64_t emp_issue(struct emp_trial *f,unsigned ci,uint64_t frame)
{
    uint64_t t=999;
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[ci],&t)==PT_MIXED_READERS_OK&&t!=999);
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[ci])==PT_MIXED_READERS_OK);
    f->port.ticks=oracle(frame);
    assert(pt_mixed_activation_fire(f->control.activation,t)==PT_MIXED_ACTIVATION_COMMITTED);return t;
}
static void emp_drain(struct emp_trial *f,unsigned ci,unsigned base,unsigned count,unsigned reader_first)
{
    unsigned i;if(!reader_first)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[ci],0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<count;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[base+i],1,NULL)==PT_MIXED_READERS_OK);
    if(reader_first)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[ci],0,NULL)==PT_MIXED_READERS_OK);
}
static void emp_drop(struct emp_trial *f,unsigned disposed)
{
    unsigned i,calls,releases,chip_releases,close_calls,quiet_calls;int closed;
    f->ordinary.alias=f->chip.alias=NULL;f->ordinary.hook=f->chip.hook=0;
    f->port_hook=f->owned_hook=f->write_hook=0;
    closed=pt_editor_mixed_readers_prepare_close(&f->control);
    assert(!f->control.pool&&!f->control.activation&&!f->control.queue);
    if(!closed){
        /* close0 + actual NULL consumption is not a retryable child. Retain
         * that first observation, then separately drain the fixed editor hook. */
        assert(f->binding->preparation_context==&f->control&&f->binding->preparation_close);
        calls=f->ordinary.calls;releases=f->ordinary.releases;chip_releases=f->chip.releases;
        close_calls=f->port.close_calls;quiet_calls=f->port.quiet_calls;
        assert(pt_editor_mixed_readers_prepare_close(&f->control));
        assert(calls==f->ordinary.calls&&releases==f->ordinary.releases&&chip_releases==f->chip.releases&&
            close_calls==f->port.close_calls&&quiet_calls==f->port.quiet_calls);
    }
    assert(pt_editor_mixed_detach(f->binding));
    for(i=0;i<PT_PROJECT_SAMPLES;++i)pt_sampler_unpin(f->pin[i]);
    if(!disposed)assert(pt_editor_dispose(f->editor));
    assert(pt_amigus_wavetable_cache_detach(&f->card->cache));assert(pt_amigus_reservation_close(&f->card->reservation));
    pt_document_release(f->document);
    for(i=0;i<80;++i)assert(!f->ordinary.live[i].p&&!f->chip.live[i].p&&!f->masters.live[i].p);
    assert(!f->ordinary.alias_releases&&!f->chip.alias_releases);
    free(f->saved);free(f->activation_workspace);free(f->factory_workspace);free(f->card);free(f->document);
    free(f->request);free(f->command);free(f->reader);
    free(f->editor);free(f->binding);free(f);
}
static void emp_geometry(struct emp_trial *f,uint64_t ticket)
{
    struct sma_command *c=sma_command(&f->port,ticket);unsigned i,j;assert(c&&c->packet.count==16);
    assert(c->packet.first==oracle(960)&&c->packet.last==oracle(961));
    for(i=0;i<16;++i){unsigned sample=i%2;const struct pt_mixed_readers_action *a=c->packet.action+i;
        assert(a->kind==PT_MIXED_READERS_TRIGGER);
        if(i<4){assert(a->geometry.paula.words==3&&a->geometry.paula.period==428&&a->geometry.paula.volume==64);
            for(j=0;j<6;++j)assert(a->geometry.paula.data[j]==smf_convert8(f->pcm[sample].data[j*2],f->bits));}
        else{const struct pt_mixed_readers_card *card=c->packet.card+i;
            assert(card->bits==f->cache_bits&&card->little_endian==f->little&&card->source_channel==1);
            assert(card->logical_bytes==6*f->cache_bits/8&&card->full_capacity>=card->logical_bytes);
            for(j=0;j<6;++j){int32_t v=f->pcm[sample].data[j*2+1];
                if(f->cache_bits==8)assert(f->card->ram[card->address+j]==smf_convert8(v,f->bits));
                else{uint16_t w=smf_convert16(v,f->bits);
                    assert(f->card->ram[card->address+2*j]==(uint8_t)(f->little?w:w>>8));
                    assert(f->card->ram[card->address+2*j+1]==(uint8_t)(f->little?w>>8:w));}}}
    }
}
static void emp_lifetime(unsigned bits,unsigned cache_bits,unsigned little,unsigned order)
{
    struct emp_trial *f=emp_make(bits,cache_bits,little);uint64_t ticket;unsigned i;
    struct pt_editor_mixed_readers_request controlled[2];struct pt_editor_mixed_command_ref next;
    emp_open(f);f->editor->project->channels.selected=15;
    assert(pt_editor_mixed_readers_prepare_get(&f->control)==PT_EDITOR_MIXED_READERS_OPEN);
    emp_requests(f,16);emp_prepare(f,0,960);emp_refs(f,0,0);
    /* Issued ownership alone is never authority for CONTROL/STOP. */
    memset(controlled,0,sizeof(controlled));controlled[0]=f->request[0];
    controlled[0].kind=PT_MIXED_READERS_CONTROL;controlled[0].expected=NULL;controlled[0].reader=f->reader[0];
    assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,1440,controlled,1,&next)==PT_EDITOR_MIXED_READERS_INVALID);
    ticket=emp_issue(f,0,960);emp_geometry(f,ticket);
    for(i=0;i<2;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    /* A detached C is not an expired R identity. Fresh positive ACTIVE getter
     * yields exactly the original key and remains independent of C lifetime. */
    controlled[0].geometry.paula.period=400;controlled[0].geometry.paula.volume=32;
    controlled[1]=f->request[4];controlled[1].kind=PT_MIXED_READERS_CONTROL;
    controlled[1].expected=NULL;controlled[1].reader=f->reader[4];memset(&controlled[1].geometry,0,sizeof(controlled[1].geometry));
    controlled[1].geometry.amigus.rate=0x18000;controlled[1].geometry.amigus.left=111;controlled[1].geometry.amigus.right=222;
    assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,1440,controlled,2,&next)==PT_EDITOR_MIXED_READERS_PENDING);
    f->command[1]=next;do{ i=(unsigned)pt_editor_mixed_readers_prepare_batch_advance(&f->control,next);}while(i==PT_EDITOR_MIXED_READERS_PENDING);
    assert(i==PT_EDITOR_MIXED_READERS_OPEN);emp_issue(f,1,1440);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,next,0,NULL)==PT_MIXED_READERS_OK);
    controlled[0].kind=controlled[1].kind=PT_MIXED_READERS_STOP;
    memset(&controlled[0].geometry,0,sizeof(controlled[0].geometry));memset(&controlled[1].geometry,0,sizeof(controlled[1].geometry));
    assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,1920,controlled,2,&next)==PT_EDITOR_MIXED_READERS_PENDING);
    f->command[1]=next;do{i=(unsigned)pt_editor_mixed_readers_prepare_batch_advance(&f->control,next);}while(i==PT_EDITOR_MIXED_READERS_PENDING);
    assert(i==PT_EDITOR_MIXED_READERS_OPEN);emp_issue(f,1,1920);
    if(!order)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,next,0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<16;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_OK);
    if(order)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,next,0,NULL)==PT_MIXED_READERS_OK);
    assert(!f->port.mask);emp_same(f);emp_drop(f,0);
}
static void emp_cancel_construction(unsigned phase)
{
    struct emp_trial *f=emp_make(24,16,0);unsigned n=0,calls;
    assert(pt_editor_mixed_readers_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
    while(f->control.phase!=phase){assert(pt_editor_mixed_readers_prepare_advance_validation(&f->control,1)==PT_EDITOR_MIXED_READERS_PENDING);assert(++n<10000);}
    calls=f->ordinary.calls;
    assert(pt_editor_mixed_readers_prepare_cancel(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
    assert(pt_editor_mixed_readers_prepare_close(&f->control));
    assert(f->ordinary.calls==calls&&!f->port.reads&&!f->port.publishes&&!f->card->writes);
    emp_same(f);emp_drop(f,0);
}
static void emp_unpublished_cancel(unsigned steps)
{
    struct emp_trial *f=emp_make(24,16,0);unsigned i,calls;enum pt_editor_mixed_readers_result r;
    emp_open(f);emp_requests(f,5);
    assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,960,f->request,5,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emp_refs(f,0,0);
    for(i=0;i<steps;++i){r=pt_editor_mixed_readers_prepare_batch_advance(&f->control,f->command[0]);assert(r==PT_EDITOR_MIXED_READERS_PENDING||r==PT_EDITOR_MIXED_READERS_OPEN);}
    calls=f->ordinary.calls;assert(pt_editor_mixed_readers_prepare_cancel(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
    assert(pt_editor_mixed_readers_prepare_close(&f->control));
    assert(calls==f->ordinary.calls&&!f->port.reads&&!f->port.publishes&&!f->port.command_calls&&!f->port.reader_calls);
    emp_same(f);emp_drop(f,0);
}
static void emp_barrier(unsigned mode,unsigned order)
{
    struct emp_trial *f=emp_make(16,16,0);unsigned revision,generation,i;struct pt_event event;
    emp_open(f);emp_requests(f,5);emp_prepare(f,0,960);emp_refs(f,0,0);emp_issue(f,0,960);
    revision=f->editor->history.revision;generation=f->editor->sampler.generation;event=f->editor->project->events[0];
    f->port.close_result=0;f->port.quiet_result=0;
    /* Exercise real edit/undo/route/dispose calls, not just hook presence. */
    if(mode==0){f->editor->editing=1;f->editor->panel=0;f->editor->row=0;f->editor->project->channels.selected=0;
        pt_editor_key(f->editor,0x46,0);}
    else if(mode==1)pt_editor_key(f->editor,0x31,8);
    else if(mode==2){f->editor->panel=4;f->editor->channel_details=0;f->editor->project->channels.selected=0;
        pt_editor_key(f->editor,0x20,0);assert(f->editor->project->channels.track[0].route==PT_PAULA);}
    else assert(!pt_editor_dispose(f->editor));
    assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation&&
        !memcmp(&event,f->editor->project->events,sizeof(event))&&f->binding->preparation_context==&f->control);
    if(!order)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i){assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_BACKEND);
        assert(!pt_editor_prepare_change(f->editor));}
    if(order)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_BACKEND);
    assert(!pt_editor_prepare_change(f->editor)&&!f->control.pool&&f->control.activation&&f->port.close_calls==1);
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&f->port.close_calls==1&&f->port.quiet_calls);
    f->port.callback_owner=NULL;f->port.quiet_result=1;
    assert(pt_editor_prepare_change(f->editor));assert(!f->binding->preparation_context&&!f->control.activation);
    assert(pt_editor_mixed_readers_prepare_get(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
    emp_same(f);emp_drop(f,0);
}
static void emp_revision_poison(unsigned mode)
{
    struct emp_trial *f=emp_make(24,16,0);struct pt_project header;unsigned calls,reads;
    emp_open(f);emp_requests(f,5);emp_prepare(f,0,960);emp_refs(f,0,0);emp_issue(f,0,960);
    memcpy(&header,f->editor->project,sizeof(header));calls=f->ordinary.calls;reads=f->port.reads;
    if(mode==0)++f->editor->history.revision;
    else{++f->editor->history.revision;f->editor->project->samples=(void *)(UINTPTR_MAX-7);
        f->editor->project->events=(void *)(UINTPTR_MAX-7);f->editor->project->extensions=(void *)(UINTPTR_MAX-7);}
    assert(pt_editor_mixed_readers_prepare_get(&f->control)==PT_EDITOR_MIXED_READERS_STALE);
    assert(pt_editor_mixed_readers_prepare_batch_advance(&f->control,f->command[0])==PT_EDITOR_MIXED_READERS_STALE);
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&calls==f->ordinary.calls&&reads==f->port.reads);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_BACKEND);
    {unsigned i;for(i=0;i<5;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_BACKEND);}
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    assert(!f->control.pool&&!f->control.activation&&!f->control.queue&&f->binding->preparation_context==&f->control);
    memcpy(f->editor->project,&header,sizeof(header));--f->editor->history.revision;
    emp_same(f);emp_drop(f,0);
}
static void emp_aliases(void)
{
    struct emp_trial *f=emp_make(24,16,0);struct pt_sampler_storage_span alias[12];unsigned i,calls,reads;
    struct pt_editor_mixed_command_ref out={1,999};struct pt_mixed_readers_command_receipt receipt;
    emp_open(f);emp_requests(f,5);emp_prepare(f,0,960);emp_refs(f,0,0);
    alias[0]=(struct pt_sampler_storage_span){&f->control,sizeof(f->control)};
    alias[1]=(struct pt_sampler_storage_span){f->control.activation,pt_mixed_activation_control_size()};
    alias[2]=(struct pt_sampler_storage_span){f->control.queue,pt_mixed_readers_control_size()};
    alias[3]=(struct pt_sampler_storage_span){f->control.pool,pt_sampler_mixed_pool_size()};
    alias[4]=(struct pt_sampler_storage_span){f->control.command[0].handle.address,pt_sampler_mixed_command_size()};
    alias[5]=(struct pt_sampler_storage_span){f->control.reader[0].handle.address,pt_sampler_mixed_reader_size()};
    alias[6]=(struct pt_sampler_storage_span){f->pcm[0].data,f->pcm[0].capacity*sizeof(int32_t)};
    alias[7]=(struct pt_sampler_storage_span){&f->port,sizeof(f->port)};
    alias[8]=(struct pt_sampler_storage_span){f->factory_workspace,f->factory_capacity};
    alias[9]=(struct pt_sampler_storage_span){&f->card->cache,sizeof(f->card->cache)};
    for(i=0;i<PT_EDITOR_MIXED_READERS_CHIP&& !f->control.chip[i].data;++i){}assert(i<PT_EDITOR_MIXED_READERS_CHIP);
    alias[10]=f->control.chip[i];alias[11]=(struct pt_sampler_storage_span){f->editor,sizeof(*f->editor)};
    calls=f->ordinary.calls;reads=f->port.reads;
    for(i=0;i<12;++i){void *copy=malloc(alias[i].bytes);assert(copy);memcpy(copy,alias[i].data,alias[i].bytes);
        assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],(void *)alias[i].data)==PT_MIXED_READERS_INVALID);
        assert(pt_editor_mixed_readers_prepare_reader_reference(&f->control,f->command[0],0,(void *)alias[i].data)==PT_EDITOR_MIXED_READERS_INVALID);
        assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,1920,(void *)alias[i].data,1,&out)==PT_EDITOR_MIXED_READERS_INVALID);
        assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,(void *)alias[i].data)==PT_MIXED_READERS_INVALID);
        assert(!memcmp(copy,alias[i].data,alias[i].bytes));free(copy);}
    /* Complete unused source capacity and declared workspace tail are guarded. */
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],(void *)(f->pcm[0].data+f->pcm[0].capacity-1))==PT_MIXED_READERS_INVALID);
    assert(f->pcm[31].capacity==64&&f->pcm[31].frames==1);
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],(void *)(f->pcm[31].data+63))==PT_MIXED_READERS_INVALID);
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],(void *)((char *)f->factory_workspace+f->factory_capacity-1))==PT_MIXED_READERS_INVALID);
    assert(out.serial==999&&calls==f->ordinary.calls&&reads==f->port.reads&&!f->control.first_error);
    memset(&receipt,0xa5,sizeof(receipt));emp_same(f);emp_drop(f,0);
}
static void emp_clock_revision(unsigned succeeds)
{
    struct emp_trial *f=emp_make(24,16,0);uint64_t first=999,second=999;unsigned i,calls,reads,revision;
    emp_open(f);emp_requests(f,5);emp_prepare(f,0,960);emp_refs(f,0,0);
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],&first)==PT_MIXED_READERS_OK&&first!=999);
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_OK);
    assert(sma_command(&f->port,first)&&!sma_command(&f->port,first)->committed);
    emp_requests(f,6);f->request[0]=f->request[5];f->count=1;
    emp_prepare(f,1,1440);emp_refs(f,1,5);
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[1],&second)==PT_MIXED_READERS_OK&&second!=999&&second!=first);
    calls=f->ordinary.calls;reads=f->port.reads;revision=f->editor->history.revision;
    f->port_hook=succeeds?10:7;
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[1])==PT_MIXED_READERS_BACKEND);
    assert(f->editor->history.revision==revision+1&&!f->reentered&&f->control.first_error==PT_EDITOR_MIXED_READERS_STALE);
    assert(f->port.reads==reads+1&&f->port.publishes==1&&!f->port.commits&&!f->port.mask&&calls==f->ordinary.calls);
    assert(f->control.command[f->command[0].slot].ticket==first&&f->control.command[f->command[1].slot].ticket==second&&
        f->control.command[f->command[1].slot].transferred);
    /* Invoke the existing copied-only fire directly: it cannot consult editor
     * tags and must already be fail-closed by task publication/post checks. */
    f->port.ticks=oracle(960);
    assert(pt_mixed_activation_fire(f->control.activation,first)==PT_MIXED_ACTIVATION_FAILED);
    assert(!f->port.commits&&!f->port.mask&&f->binding->preparation_context==&f->control);
    for(i=0;i<5;++i)assert(f->control.reader[f->reader[i].slot].handle.address);
    assert(!pt_editor_prepare_change(f->editor));
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_BACKEND);
    --f->editor->history.revision;emp_same(f);emp_drop(f,0);
}
static void emp_adopted_begin_poison(void)
{
    struct emp_trial *f=emp_make(24,16,0);struct pt_project header;uint64_t ticket=999;
    unsigned i,calls,reads,reentries;enum pt_editor_mixed_readers_result error;
    emp_open(f);emp_requests(f,5);emp_prepare(f,0,960);emp_refs(f,0,0);
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    memcpy(&header,f->editor->project,sizeof(header));f->port_hook=9;
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_BACKEND);
    assert(f->reentered==1&&f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT);
    calls=f->ordinary.calls;reads=f->port.reads;reentries=f->control.reentries;error=f->control.first_error;
    /* Already-adopted nonbusy reuse refuses read-only, even after former
     * source arrays were poisoned by the callback. No fresh admission walks. */
    assert(pt_editor_mixed_readers_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(calls==f->ordinary.calls&&reads==f->port.reads&&reentries==f->control.reentries&&error==f->control.first_error);
    f->port.ticks=oracle(960);
    assert(pt_mixed_activation_fire(f->control.activation,ticket)==PT_MIXED_ACTIVATION_FAILED&&!f->port.commits&&!f->port.mask);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_BACKEND);
    memcpy(f->editor->project,&header,sizeof(header));--f->editor->history.revision;
    emp_same(f);emp_drop(f,0);
}
static void emp_fault(unsigned kind)
{
    struct emp_trial *f=emp_make(16,16,0);enum pt_editor_mixed_readers_result r;uint64_t ticket=999;unsigned i;
    if(kind<=3){assert(pt_editor_mixed_readers_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
        f->ordinary.hook=kind;
        do{r=pt_editor_mixed_readers_prepare_advance_validation(&f->control,64);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
        assert(r==PT_EDITOR_MIXED_READERS_FAULT&&f->reentered);emp_drop(f,0);return;}
    emp_open(f);emp_requests(f,5);
    if(kind==4){f->ordinary.hook=f->ordinary.calls+1;
        assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,960,f->request,5,f->command)==PT_EDITOR_MIXED_READERS_FAULT);
        emp_drop(f,0);return;}
    if(kind==5||kind==6){assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,960,f->request,5,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
        if(kind==5)f->chip.hook=1;else f->write_hook=1;
        do{r=pt_editor_mixed_readers_prepare_batch_advance(&f->control,f->command[0]);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
        assert(r==PT_EDITOR_MIXED_READERS_FAULT&&f->reentered);emp_drop(f,0);return;}
    emp_prepare(f,0,960);emp_refs(f,0,0);
    if(kind==7){f->owned_hook=1;
        assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],&ticket)==PT_MIXED_READERS_OK);
        assert(ticket!=999&&f->control.command[f->command[0].slot].transferred&&f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT);
        assert(!f->port.publishes);emp_drop(f,0);return;}
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    if(kind==8||kind==9||kind==10){f->port_hook=kind==8?1:kind==9?2:8;
        assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_BACKEND);
        assert(!pt_editor_mixed_readers_prepare_close(&f->control));
        if(f->port.publishes){assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],1,NULL)==PT_MIXED_READERS_BACKEND);
            for(i=0;i<5;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_BACKEND);}
        if(kind==10)--f->editor->history.revision;
        emp_drop(f,0);return;}
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_OK);
    f->port.partial=1;f->port.ticks=oracle(960);
    assert(pt_mixed_activation_fire(f->control.activation,ticket)==PT_MIXED_ACTIVATION_FAILED);
    assert(!pt_editor_prepare_change(f->editor)&&f->port.mask==1);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_BACKEND);
    emp_drop(f,0);
}
static void emp_slot_reuse(void)
{
    struct emp_trial *f=emp_make(8,8,1);struct pt_editor_mixed_reader_ref old[16];struct pt_editor_mixed_command_ref oldc;
    struct pt_editor_mixed_readers_request x;struct pt_editor_mixed_command_ref out={1,999};unsigned i;
    emp_open(f);emp_requests(f,16);emp_prepare(f,0,960);emp_refs(f,0,0);oldc=f->command[0];memcpy(old,f->reader,sizeof(old));
    emp_issue(f,0,960);emp_drain(f,0,0,16,0);
    emp_requests(f,16);emp_prepare(f,0,1920);emp_refs(f,0,0);emp_issue(f,0,1920);
    assert(oldc.slot==f->command[0].slot&&oldc.serial!=f->command[0].serial);
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,oldc)==PT_MIXED_READERS_INVALID);
    for(i=0;i<16;++i){assert(old[i].slot==f->reader[i].slot&&old[i].serial!=f->reader[i].serial);
        x=f->request[i];x.kind=PT_MIXED_READERS_STOP;x.expected=NULL;x.reader=old[i];memset(&x.geometry,0,sizeof(x.geometry));
        assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,2880,&x,1,&out)==PT_EDITOR_MIXED_READERS_INVALID);}
    assert(out.serial==999);emp_drain(f,0,0,16,1);emp_same(f);emp_drop(f,0);
}
#ifndef PT_EDITOR_MIXED_READERS_TEST_MAIN
#define PT_EDITOR_MIXED_READERS_TEST_MAIN main
#endif
int PT_EDITOR_MIXED_READERS_TEST_MAIN(void)
{
    unsigned bits,cache,little,phase,mode,order,i;
    for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(little=0;little<2;++little)
        emp_lifetime(bits,cache,little,little);
    for(phase=PT_EDITOR_MIXED_READERS_ACTIVATION;phase<=PT_EDITOR_MIXED_READERS_VALIDATION;++phase)emp_cancel_construction(phase);
    for(i=0;i<23;++i)emp_unpublished_cancel(i);
    for(mode=0;mode<4;++mode)for(order=0;order<2;++order)emp_barrier(mode,order);
    emp_revision_poison(0);emp_revision_poison(1);emp_aliases();
    emp_clock_revision(0);emp_clock_revision(1);emp_adopted_begin_poison();
    for(i=1;i<=11;++i)emp_fault(i);
    emp_slot_reuse();
    puts("EDITOR MIXED READERS PREPARE PASS:12 genuine paired editor 8/16/24 master/Chip8/card8+16 precision/channel/endian lifetimes; original-window CONTROL/STOP only from positive ACTIVE keys; actual edit/undo/route/dispose veto until independent C/R and source quiet; construction/action cancellation, revision-only stale and poisoned former tables, complete unpublished/spare output/input guards, authoritative enqueue transfer, callback faults/partial effects, slot serial reuse and exact master saves; SOFTWARE_ONLY");
    return inherited_paired_main();
}
