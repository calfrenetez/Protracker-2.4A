/* Reuse the complete genuine ABI2 model and lowerer/oracle bodies. The included
 * implementations are deliberately not compiled a second time by the recipe. */
#include "paula_readers_song_test.c"
#include "../src/editor/editor_paula_readers_prepare.h"

struct editor_reader_fixture {
    struct pt_editor_paula_readers_prepare control;
    struct pt_editor_paula_readers_prepare_inputs input;
    struct memory fast,chip;
    struct backend backend;
    struct song_fixture *source;
    struct pt_editor *editor;
    struct pt_editor_mixed *binding;
    struct pt_editor_mixed_establish *establish;
    void *workspace,*alias;
    size_t workspace_bytes;
    unsigned alloc_hook,release_hook,clock_hook,submit_hook,command_hook,reader_hook,reentered,alias_releases;
};
static void editor_reader_reentry(struct editor_reader_fixture *f)
{
    unsigned revision=f->editor->history.revision,generation=f->editor->sampler.generation;
    enum pt_editor_readers_result before=f->control.first_error;
    assert(pt_editor_paula_readers_prepare_get(&f->control)==(before?before:PT_EDITOR_READERS_FAULT));
    assert(!pt_editor_paula_readers_prepare_close(&f->control));
    assert(!pt_editor_prepare_change(f->editor)&&!pt_editor_dispose(f->editor));
    assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation);++f->reentered;
}
static void *editor_reader_allocate(void *context,size_t n)
{
    struct editor_reader_fixture *f=(void *)((char *)context- offsetof(struct editor_reader_fixture,fast));
    if(f->alloc_hook==f->fast.calls+1){f->alloc_hook=0;editor_reader_reentry(f);}
    if(f->alias){++f->fast.calls;return f->alias;}
    return allocate(&f->fast,n);
}
static void editor_reader_release(void *context,void *p)
{
    struct editor_reader_fixture *f=(void *)((char *)context- offsetof(struct editor_reader_fixture,fast));
    if(p==f->alias){++f->alias_releases;return;}
    if(f->release_hook){f->release_hook=0;editor_reader_reentry(f);}
    release(&f->fast,p);
}
static int editor_reader_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct editor_reader_fixture *f=(void *)((char *)context- offsetof(struct editor_reader_fixture,backend));
    if(f->clock_hook){f->clock_hook=0;editor_reader_reentry(f);}
    return fixture_song_clock(context,ticks,frequency);
}
static int editor_reader_submit(void *context,const struct pt_readers_event *event)
{
    struct editor_reader_fixture *f=(void *)((char *)context- offsetof(struct editor_reader_fixture,backend));
    int r=fixture_song_fault_submit(context,event);
    if(f->submit_hook){f->submit_hook=0;editor_reader_reentry(f);}
    return r;
}
static enum pt_readers_reply editor_reader_poll_command(void *context,uint64_t ticket,struct pt_readers_command_receipt *out)
{
    struct editor_reader_fixture *f=(void *)((char *)context- offsetof(struct editor_reader_fixture,backend));
    enum pt_readers_reply r=fixture_poll_command(context,ticket,out);
    if(f->command_hook){f->command_hook=0;editor_reader_reentry(f);}return r;
}
static enum pt_readers_reply editor_reader_poll_reader(void *context,const struct pt_readers_domain *domain,struct pt_readers_reader_receipt *out)
{
    struct editor_reader_fixture *f=(void *)((char *)context- offsetof(struct editor_reader_fixture,backend));
    enum pt_readers_reply r=fixture_poll_reader(context,domain,out);
    if(f->reader_hook){f->reader_hook=0;editor_reader_reentry(f);}return r;
}
static struct editor_reader_fixture *editor_reader_make(unsigned bits)
{
    struct editor_reader_fixture *f=calloc(1,sizeof(*f));assert(f);
    f->source=song_fixture_new(bits,0);f->editor=malloc(sizeof(*f->editor));
    f->binding=calloc(1,sizeof(*f->binding));f->establish=calloc(1,sizeof(*f->establish));
    f->workspace_bytes=pt_paula_readers_song_begin_workspace_size()+128;f->workspace=malloc(f->workspace_bytes);
    assert(f->editor&&f->binding&&f->establish&&f->workspace);
    assert(pt_editor_init(f->editor,&f->source->genuine.document.project));
    f->editor->sampler.allocator=(struct pt_allocator){&f->fast,editor_reader_allocate,editor_reader_release};
    f->editor->sampler.budget=1024*1024;assert(pt_editor_mixed_attach(f->binding,f->editor));
    f->input.binding=f->binding;f->input.establish=f->establish;f->input.config=fixture_song_config(f->source);
    f->input.config.readers.chip_context=&f->chip;
    f->input.config.backend.context=&f->backend;f->input.config.backend.context_bytes=sizeof(f->backend);
    f->input.config.backend.read_clock=editor_reader_clock;f->input.config.backend.submit=editor_reader_submit;
    f->input.config.backend.poll_command=editor_reader_poll_command;f->input.config.backend.poll_reader=editor_reader_poll_reader;
    f->input.contexts=(struct pt_sampler_storage_span){f,sizeof(*f)};
    f->input.workspace=f->workspace;f->input.workspace_capacity=f->workspace_bytes;f->backend.now=100;
    memset(f->workspace,0xa5,f->workspace_bytes);return f;
}
static uint8_t *editor_reader_save(struct editor_reader_fixture *f,size_t *bytes)
{
    size_t used;uint8_t *p;assert(pt_project_size(f->editor->project,bytes)==PT_PROJECT_OK);
    p=malloc(*bytes);assert(p&&pt_project_encode(f->editor->project,p,*bytes,&used)==PT_PROJECT_OK&&used==*bytes);return p;
}
static void editor_reader_save_same(struct editor_reader_fixture *f,const uint8_t *before,size_t bytes)
{
    size_t count;uint8_t *after;f->editor->project->channels.selected=0;after=editor_reader_save(f,&count);
    assert(count==bytes&&!memcmp(before,after,bytes));free(after);
}
static void editor_reader_source_promoted(struct editor_reader_fixture *f)
{
    struct song_fixture *s=f->source;unsigned i;
    for(i=0;i<2;++i){assert(f->editor->sampler.current[i]);assert(f->editor->project->samples[i].pcm.bits==s->sample_before[i].pcm.bits);
        assert(f->editor->project->samples[i].pcm.frames==s->sample_before[i].pcm.frames);
        assert(!memcmp(f->editor->project->samples[i].pcm.data,s->original_before,16384*sizeof(int32_t)));}
}
static enum pt_editor_readers_result editor_reader_until(struct editor_reader_fixture *f,unsigned phase)
{
    enum pt_editor_readers_result r=PT_EDITOR_READERS_PENDING;unsigned n=0;
    while(f->control.phase!=phase&&!f->control.first_error){r=pt_editor_paula_readers_prepare_step(&f->control,64,NULL);assert(++n<20000);}
    return r;
}
static void editor_reader_drop(struct editor_reader_fixture *f,unsigned disposed)
{
    unsigned i;f->alias=NULL;f->alloc_hook=f->release_hook=0;f->backend.uncertain=0;
    assert(pt_editor_paula_readers_prepare_close(&f->control));assert(!f->control.producer);
    assert(pt_editor_mixed_detach(f->binding));if(!disposed)assert(pt_editor_dispose(f->editor));
    assert(!f->fast.live&&!f->chip.live&&!f->alias_releases);
    for(i=0;i<2;++i)free(f->source->master_before[i]);
    pt_sampler_release(&f->source->genuine.sampler);pt_document_release(&f->source->genuine.document);
    assert(!f->source->genuine.fast.live);free(f->source);free(f->editor);free(f->binding);free(f->establish);free(f->workspace);free(f);
}
static void editor_reader_lifecycle(unsigned bits,unsigned mode)
{
    struct editor_reader_fixture *f=editor_reader_make(bits);struct pt_sample_version *masters[2]={0};
    enum pt_editor_readers_result r;size_t bytes;uint8_t *before=editor_reader_save(f,&bytes);unsigned disposed=0,calls;
    if(mode==10)f->fast.fail=1;
    if(mode==11)f->alloc_hook=1;
    if(mode==12)f->alias=&f->control;
    if(mode==13)f->alias=f->workspace;
    if(mode==14)f->alias=&f->input;
    r=pt_editor_paula_readers_prepare_begin(&f->control,&f->input);
    if(mode>=10&&mode<=14){assert(r!=PT_EDITOR_READERS_PENDING&&f->binding->preparation_context==f->establish);goto close;}
    assert(r==PT_EDITOR_READERS_PENDING&&!f->backend.submissions&&!fixture_command_polls&&!fixture_reader_polls);
    if(mode==0)goto close;
    if(mode==1){assert(pt_editor_paula_readers_prepare_step(&f->control,64,NULL)==r);goto close;}
    if(mode==2){assert(pt_editor_prepare_change(f->editor));assert(pt_editor_paula_readers_prepare_get(&f->control)==PT_EDITOR_READERS_CANCELLED);goto close;}
    if(mode==3){assert(pt_editor_dispose(f->editor));disposed=1;assert(pt_editor_paula_readers_prepare_get(&f->control)==PT_EDITOR_READERS_CANCELLED);goto close;}
    if(mode==15){f->input.config.absolute_start++;calls=f->fast.calls;
        assert(pt_editor_paula_readers_prepare_step(&f->control,64,NULL)==PT_EDITOR_READERS_CANCELLED&&f->fast.calls==calls);
        f->input.config.absolute_start--;goto close;}
    assert(editor_reader_until(f,PT_EDITOR_READERS_HANDOFF)==PT_EDITOR_READERS_PENDING);editor_reader_source_promoted(f);
    memcpy(masters,f->editor->sampler.current,sizeof(masters));assert(!f->backend.submissions&&!f->chip.live);
    if(mode==4)goto close;
    if(mode==16)f->release_hook=1;
    if(mode==17)f->fast.fail=f->fast.calls+1;
    if(mode==18)f->alloc_hook=f->fast.calls+1;
    if(mode==19)f->alias=&f->control;
    if(mode==20)f->alias=f->workspace;
    if(mode==21)f->alias=f->establish;
    if(mode==22)f->alias=&f->backend;
    if(mode==23)f->alias=&f->input;
    r=pt_editor_paula_readers_prepare_step(&f->control,64,NULL);
    if(mode>=16&&mode<=23){assert(r!=PT_EDITOR_READERS_PENDING);
        if(mode!=16)assert(f->binding->preparation_context==&f->control);goto close;}
    assert(r==PT_EDITOR_READERS_PENDING&&f->control.producer&&f->binding->preparation_context==&f->control);
    if(mode==5)goto close;
    if(mode==6){f->editor->editing=1;f->editor->panel=0;f->editor->row=0;f->editor->project->channels.selected=0;
        calls=f->editor->history.revision;pt_editor_key(f->editor,0x46,0);
        assert(f->editor->history.revision!=calls&&f->editor->project->events[0].kind==PT_NOTE_NONE);
        assert(pt_editor_paula_readers_prepare_get(&f->control)==PT_EDITOR_READERS_CANCELLED);
        pt_editor_key(f->editor,0x31,8);assert(f->editor->project->events[0].kind==PT_NOTE_PERIOD);goto close;}
    if(mode==7){assert(pt_editor_dispose(f->editor));disposed=1;assert(pt_editor_paula_readers_prepare_get(&f->control)==PT_EDITOR_READERS_CANCELLED);goto close;}
    if(mode==8)f->release_hook=1;
    if(mode==9){unsigned n=0;struct pt_paula_readers_song_status status;
        while(!f->control.first_error){r=pt_editor_paula_readers_prepare_step(&f->control,256,&status);assert(++n<20000);
            if(status.phase==PT_PAULA_READERS_SONG_PUBLISH)break;}
        assert(r==PT_EDITOR_READERS_PENDING&&status.phase==PT_PAULA_READERS_SONG_PUBLISH&&!f->backend.submissions);goto close;}
close:
    f->alias=NULL;f->alloc_hook=0;
    if(mode==8){assert(!pt_editor_paula_readers_prepare_close(&f->control)&&f->reentered);
        /* Genuine close can consume the owner yet report failure. Its private
         * hook stays installed until the distinct explicit next close. */
        assert(f->binding->preparation_context==&f->control&&!f->control.producer);}
    assert(pt_editor_paula_readers_prepare_close(&f->control));
    assert(!f->backend.submissions&&!f->chip.live&&!fixture_command_polls&&!fixture_reader_polls);
    if(!disposed){editor_reader_save_same(f,before,bytes);
        if(masters[0])assert(!memcmp(masters,f->editor->sampler.current,sizeof(masters))&&f->fast.live>=2);}
    calls=f->fast.calls;assert(pt_editor_paula_readers_prepare_begin(&f->control,&f->input)==PT_EDITOR_READERS_INVALID&&f->fast.calls==calls);
    free(before);editor_reader_drop(f,disposed);
}
static void editor_reader_admission(unsigned bits,unsigned mode)
{
    struct editor_reader_fixture *f=editor_reader_make(bits),*snapshot=malloc(sizeof(*f));
    struct pt_editor_paula_readers_prepare *control=&f->control;void *workcopy;unsigned calls=f->fast.calls;
    switch(mode){case 0:control=(void *)f->editor;break;case 1:control=(void *)f->binding;break;
    case 2:control=(void *)f->establish;break;case 3:control=(void *)&f->input;break;
    case 4:control=(void *)f->source->genuine.master.values;break;case 5:control=(void *)(UINTPTR_MAX-1);break;
    case 6:f->input.workspace_capacity--;f->input.workspace_capacity=pt_paula_readers_song_begin_workspace_size()-1;break;
    case 7:f->input.workspace=(char *)f->workspace+1;break;case 8:f->input.workspace=(void *)(UINTPTR_MAX-1);break;
    case 9:f->input.workspace=&f->control;break;case 10:f->input.contexts.bytes=sizeof(f->control)-1;break;
    case 11:f->input.config.backend.context=&f->control;break;case 12:f->input.config.readers.chip_context=&f->control;break;
    case 13:f->input.establish=(void *)&f->control;break;case 14:f->input.contexts.data=(void *)(UINTPTR_MAX-1);break;
    case 15:f->input.config.backend.context_bytes=sizeof(*f);break;
    case 16:f->input.workspace_capacity=SIZE_MAX;break;case 17:f->input.config.backend.context=f->editor;break;
    case 18:f->input.contexts=(struct pt_sampler_storage_span){&f->input,sizeof(f->input)};break;
    case 19:f->input.config.readers.chip_allocate=NULL;break;case 20:f->input.config.readers.chip_release=NULL;break;
    case 21:f->input.config.backend.read_clock=NULL;break;case 22:f->input.config.backend.submit=NULL;break;
    case 23:f->input.config.backend.poll_command=NULL;break;case 24:f->input.config.backend.cancel_command=NULL;break;
    case 25:f->input.config.backend.poll_reader=NULL;break;default:f->input.config.backend.cancel_reader=NULL;break;}
    memcpy(snapshot,f,sizeof(*f));workcopy=malloc(f->workspace_bytes);assert(workcopy);memcpy(workcopy,f->workspace,f->workspace_bytes);
    assert(pt_editor_paula_readers_prepare_begin(control,&f->input)==PT_EDITOR_READERS_INVALID);
    assert(!memcmp(snapshot,f,sizeof(*f))&&!memcmp(workcopy,f->workspace,f->workspace_bytes)&&calls==f->fast.calls);
    free(snapshot);free(workcopy);editor_reader_drop(f,0);
}
static void editor_reader_publish_setup(struct editor_reader_fixture *f,struct pt_paula_readers_song_status *status)
{
    unsigned n=0;enum pt_editor_readers_result r;
    assert(pt_editor_paula_readers_prepare_begin(&f->control,&f->input)==PT_EDITOR_READERS_PENDING);
    do{r=pt_editor_paula_readers_prepare_step(&f->control,256,status);assert(++n<20000&&r==PT_EDITOR_READERS_PENDING);}while(status->phase!=PT_PAULA_READERS_SONG_PUBLISH);
    f->backend.pool=f->control.producer->pool;
}
static void editor_reader_output_aliases(unsigned bits)
{
    struct editor_reader_fixture *f=editor_reader_make(bits);struct pt_paula_readers_song_status status={0};
    struct pt_editor_paula_readers_prepare before;unsigned i,count=0;
    const void *extra[6];editor_reader_publish_setup(f,&status);before=f->control;
    extra[0]=&f->control;extra[1]=&f->input;extra[2]=&f->backend;extra[3]=f->establish;
    extra[4]=(char *)f->workspace+f->workspace_bytes-1;
    extra[5]=f->editor->project->samples[0].pcm.data;
    for(i=0;i<PT_EDITOR_READERS_ALLOCATIONS+6;++i){void *p;
        if(i<PT_EDITOR_READERS_ALLOCATIONS){if(!f->control.allocation[i].data)continue;
            p=(void *)f->control.allocation[i].data;}else p=(void *)extra[i-PT_EDITOR_READERS_ALLOCATIONS];
        assert(pt_editor_paula_readers_prepare_step(&f->control,64,p)==PT_EDITOR_READERS_INVALID);
        assert(pt_editor_paula_readers_prepare_service_command(&f->control,0,0,p)==PT_SCHEDULED_INVALID);
        assert(pt_editor_paula_readers_prepare_service_reader(&f->control,0,0,p)==PT_SCHEDULED_INVALID);
        assert(!memcmp(&before,&f->control,sizeof(before))&&!f->backend.submissions);++count;
    }
    assert(count>=12);assert(pt_editor_paula_readers_prepare_close(&f->control));editor_reader_drop(f,0);
}
static void editor_reader_domain_case(unsigned bits,unsigned mode)
{
    struct editor_reader_fixture *f=editor_reader_make(bits);struct pt_paula_readers_song_status status={0};
    struct pt_scheduled_batch expected;struct song_oracle *oracle=song_oracle_open(bits);
    unsigned i,command=0,submissions,polls;uint64_t boundary,ticket;size_t bytes;uint8_t *before=editor_reader_save(f,&bytes);
    editor_reader_publish_setup(f,&status);expected=song_oracle_next_nonempty(oracle);
    boundary=status.boundary_frame;assert(boundary==expected.frame&&boundary>=SONG_START);
    if(mode==2||mode==4){if(mode==2)f->backend.now=UINT64_MAX/2;else f->clock_hook=1;submissions=f->backend.submissions;
        assert(pt_editor_paula_readers_prepare_publish(&f->control)==PT_SCHEDULED_BACKEND&&submissions==f->backend.submissions);
        assert(f->control.first_error&&!f->backend.commands);goto finished;}
    if(mode==1)fixture_submit_mode=2;
    if(mode==5)f->submit_hook=1;
    assert(pt_editor_paula_readers_prepare_publish(&f->control)==(mode==1||mode==5?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK));
    assert(f->backend.commands==1&&f->backend.submissions==1);ticket=f->backend.command[0].event.scheduled.ticket;
    song_batch_same(&f->backend.command[0].event.scheduled.batch,&expected);
    assert(f->backend.command[0].event.session==f->input.config.session);
    assert(f->backend.command[0].event.scheduled.batch.generation==f->input.config.grid.generation);
    {uint64_t first,last;struct pt_elapsed_clock clock={0};
        assert(pt_elapsed_clock_init(&clock,f->input.config.grid.frequency,f->input.config.grid.rate,f->input.config.grid.epoch,0)==PT_ELAPSED_OK);
        assert(pt_elapsed_clock_deadline(&clock,boundary,&first)==PT_ELAPSED_OK&&pt_elapsed_clock_deadline(&clock,boundary+1,&last)==PT_ELAPSED_OK);
        assert(first==f->backend.command[0].event.scheduled.first&&last==f->backend.command[0].event.scheduled.last);}
    for(i=0;i<2;++i)if(f->control.producer->command[i].ticket==ticket)command=i;
    if(mode!=1&&mode!=5){f->backend.now=f->backend.command[0].event.scheduled.first;fire(&f->backend,ticket);
        polls=fixture_command_polls;
        if(mode==6){struct pt_readers_command_receipt out,saved;unsigned reentered=f->reentered;
            memset(&out,0x5a,sizeof(out));saved=out;f->command_hook=1;
            assert(pt_editor_paula_readers_prepare_service_command(&f->control,command,0,&out)==PT_SCHEDULED_BACKEND);
            assert(!f->command_hook&&f->reentered==reentered+1&&f->control.first_error==PT_EDITOR_READERS_FAULT);
            assert(!memcmp(&out,&saved,sizeof(out))&&f->control.producer->command[command].holder);
            /* The actual command proof still reached the genuine queue even
             * though controller failure suppressed the external receipt. */
            for(i=0;i<4;++i){struct pt_readers_key actual;
                assert(pt_paula_readers_reader_key(f->control.producer->reader[i].holder,&actual)==PT_SCHEDULED_OK);
                assert(same_key(&actual,&f->backend.reader[i].key));}
        }else assert(pt_editor_paula_readers_prepare_service_command(&f->control,command,0,NULL)==PT_SCHEDULED_PENDING);
        assert(fixture_command_polls==polls+1);}
    polls=fixture_command_polls+fixture_reader_polls;submissions=f->backend.submissions;
    assert(!pt_editor_prepare_change(f->editor)&&!pt_editor_dispose(f->editor));
    assert(!pt_editor_paula_readers_prepare_close(&f->control));
    assert(polls==fixture_command_polls+fixture_reader_polls&&submissions==f->backend.submissions);
    assert(f->binding->preparation_context==&f->control&&f->control.producer);
    if(mode==1){assert(f->backend.uncertain);f->backend.uncertain=0;}
    if(mode==3){/* Reader retirement alone does not detach referencing command. */
        for(i=0;i<f->backend.readers;++i)retired(&f->backend,&f->backend.reader[i].key);
        for(i=0;i<8;++i)(void)pt_editor_paula_readers_prepare_service_reader(&f->control,i,1,NULL);
        assert(!pt_editor_paula_readers_prepare_close(&f->control)&&f->control.producer);
    }
    detach(&f->backend,ticket);polls=fixture_command_polls;
    (void)pt_editor_paula_readers_prepare_service_command(&f->control,command,1,NULL);assert(fixture_command_polls==polls+1);
    if(mode!=3){assert(!pt_editor_paula_readers_prepare_close(&f->control)&&f->control.producer);
        for(i=0;i<f->backend.readers;++i)retired(&f->backend,&f->backend.reader[i].key);
        for(i=0;i<8;++i){polls=fixture_reader_polls;if(mode==7)f->reader_hook=1;
            (void)pt_editor_paula_readers_prepare_service_reader(&f->control,i,mode==7?0:1,NULL);
            assert(fixture_reader_polls<=polls+1);}}
finished:
    assert(pt_editor_paula_readers_prepare_close(&f->control)&&!f->binding->preparation_context);
    editor_reader_save_same(f,before,bytes);free(before);
    /* The real oracle needs its own remaining boundaries and exact stop. */
    while(!oracle->ended){struct pt_scheduled_batch b;(void)song_oracle_next(oracle,&b);}song_oracle_close(oracle);
    editor_reader_drop(f,0);fixture_command_polls=fixture_reader_polls=0;
}
static unsigned editor_reader_command_index(struct editor_reader_fixture *f,uint64_t ticket)
{
    unsigned i;for(i=0;i<2;++i)if(f->control.producer->command[i].ticket==ticket)return i;
    assert(0);return 0;
}
static void editor_reader_detach(struct editor_reader_fixture *f,uint64_t ticket)
{
    unsigned polls=fixture_command_polls,index=editor_reader_command_index(f,ticket);detach(&f->backend,ticket);
    assert(pt_editor_paula_readers_prepare_service_command(&f->control,index,0,NULL)==PT_SCHEDULED_OK);
    assert(fixture_command_polls==polls+1);
}
static void editor_reader_whole(unsigned bits)
{
    struct editor_reader_fixture *f=editor_reader_make(bits);struct song_oracle *oracle=song_oracle_open(bits);
    struct pt_paula_readers_song_status status={0};struct pt_scheduled_batch expected;
    enum pt_editor_readers_result r;unsigned steps=0,batches=0,i,polls,held=0,pressure=0,waits=0,submissions;
    uint64_t held_ticket=0,second_ticket=0,terminal=0;size_t bytes;uint8_t *before=editor_reader_save(f,&bytes);
    editor_reader_publish_setup(f,&status);
    for(;;){
        if(status.phase==PT_PAULA_READERS_SONG_PUBLISH){
            struct model_command *command;uint64_t ticket,boundary=status.boundary_frame;
            expected=song_oracle_next_nonempty(oracle);assert(boundary==expected.frame);
            submissions=f->backend.commands;assert(pt_editor_paula_readers_prepare_publish(&f->control)==PT_SCHEDULED_OK);
            assert(f->backend.commands==submissions+1);command=f->backend.command+submissions;
            ticket=command->event.scheduled.ticket;song_batch_same(&command->event.scheduled.batch,&expected);++batches;
            if(batches==1){/* Reserved future readers cannot provide ACTIVE keys. */
                do{r=pt_editor_paula_readers_prepare_step(&f->control,256,&status);assert(++steps<30000);}while(r==PT_EDITOR_READERS_PENDING);
                assert(r==PT_EDITOR_READERS_WAIT_ACTIVE);boundary=status.boundary_frame;
                for(i=0;i<3;++i){assert(pt_editor_paula_readers_prepare_step(&f->control,256,&status)==r&&status.boundary_frame==boundary);++waits;}
            }
            f->backend.now=command->event.scheduled.first;fire(&f->backend,ticket);polls=fixture_command_polls;
            assert(pt_editor_paula_readers_prepare_service_command(&f->control,editor_reader_command_index(f,ticket),0,NULL)==PT_SCHEDULED_PENDING);
            assert(fixture_command_polls==polls+1);
            for(i=0;i<8;++i){polls=fixture_reader_polls;(void)pt_editor_paula_readers_prepare_service_reader(&f->control,i,0,NULL);
                assert(fixture_reader_polls<=polls+1);}
            if(batches==1){held=1;held_ticket=ticket;}
            else if(batches==2){second_ticket=ticket;
                do{r=pt_editor_paula_readers_prepare_step(&f->control,256,&status);assert(++steps<30000);}while(r==PT_EDITOR_READERS_PENDING);
                assert(r==PT_EDITOR_READERS_WAIT_PRESSURE&&status.command_mask==3);boundary=status.boundary_frame;
                for(i=0;i<3;++i){assert(pt_editor_paula_readers_prepare_step(&f->control,256,&status)==r&&status.boundary_frame==boundary);++pressure;}
                editor_reader_detach(f,held_ticket);held=0;
            }else{if(second_ticket){editor_reader_detach(f,second_ticket);second_ticket=0;}editor_reader_detach(f,ticket);}
        }
        r=pt_editor_paula_readers_prepare_step(&f->control,256,&status);assert(++steps<30000&&!f->control.first_error);
        if(r==PT_EDITOR_READERS_DONE){terminal=status.terminal_frame;break;}
        assert(r==PT_EDITOR_READERS_PENDING||r==PT_EDITOR_READERS_WAIT_ACTIVE);
    }
    assert(!held&&!second_ticket&&batches>20&&waits==3&&pressure==3);
    assert(terminal==SONG_START+oracle->report.frames&&status.boundary_frame==terminal&&!status.terminal_stop_requested);
    {struct pt_scheduled_batch empty;assert(!song_oracle_next(oracle,&empty)&&oracle->ended&&empty.frame==terminal);}
    submissions=f->backend.submissions;
    for(i=0;i<3;++i)assert(pt_editor_paula_readers_prepare_step(&f->control,256,&status)==PT_EDITOR_READERS_DONE);
    assert(submissions==f->backend.submissions);
    assert(pt_editor_paula_readers_prepare_terminal_stop(&f->control)==PT_EDITOR_READERS_PENDING);
    while(status.phase!=PT_PAULA_READERS_SONG_PUBLISH){assert(pt_editor_paula_readers_prepare_step(&f->control,256,&status)==PT_EDITOR_READERS_PENDING&&++steps<30000);}
    submissions=f->backend.commands;assert(pt_editor_paula_readers_prepare_publish(&f->control)==PT_SCHEDULED_OK);
    {struct model_command *command=f->backend.command+submissions;uint64_t ticket=command->event.scheduled.ticket;
        assert(command->event.scheduled.batch.frame==terminal&&command->event.scheduled.batch.count==4);
        for(i=0;i<4;++i)assert(command->event.scheduled.batch.action[i].kind==PT_SCHEDULED_STOP);
        f->backend.now=command->event.scheduled.first;fire(&f->backend,ticket);
        assert(pt_editor_paula_readers_prepare_service_command(&f->control,editor_reader_command_index(f,ticket),0,NULL)==PT_SCHEDULED_PENDING);
        editor_reader_detach(f,ticket);}
    for(i=0;i<f->backend.readers;++i)retired(&f->backend,&f->backend.reader[i].key);
    for(i=0;i<8;++i){polls=fixture_reader_polls;(void)pt_editor_paula_readers_prepare_service_reader(&f->control,i,0,NULL);assert(fixture_reader_polls<=polls+1);}
    assert(pt_editor_paula_readers_prepare_close(&f->control));editor_reader_save_same(f,before,bytes);free(before);
    song_oracle_close(oracle);editor_reader_drop(f,0);fixture_command_polls=fixture_reader_polls=0;
}
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define POISON(p,n) __asan_poison_memory_region(p,n)
#define UNPOISON(p,n) __asan_unpoison_memory_region(p,n)
#endif
#endif
#ifndef POISON
#define POISON(p,n) ((void)(p),(void)(n))
#define UNPOISON(p,n) ((void)(p),(void)(n))
#endif
static void editor_reader_terminal(unsigned bits)
{
    struct editor_reader_fixture *f=editor_reader_make(bits);unsigned i;
    assert(pt_editor_paula_readers_prepare_begin(&f->control,&f->input)==PT_EDITOR_READERS_PENDING);
    assert(pt_editor_paula_readers_prepare_close(&f->control)&&pt_editor_mixed_detach(f->binding)&&pt_editor_dispose(f->editor));
    for(i=0;i<2;++i)free(f->source->master_before[i]);
    pt_sampler_release(&f->source->genuine.sampler);
    pt_document_release(&f->source->genuine.document);free(f->source);free(f->editor);free(f->binding);free(f->establish);free(f->workspace);
    POISON(&f->input,sizeof(*f)-offsetof(struct editor_reader_fixture,input));
    assert(pt_editor_paula_readers_prepare_get(&f->control)==PT_EDITOR_READERS_CLOSED);
    assert(pt_editor_paula_readers_prepare_step(&f->control,64,NULL)==PT_EDITOR_READERS_CLOSED);
    assert(pt_editor_paula_readers_prepare_close(&f->control));
    assert(pt_editor_paula_readers_prepare_publish(&f->control)==PT_SCHEDULED_INVALID);
    UNPOISON(&f->input,sizeof(*f)-offsetof(struct editor_reader_fixture,input));free(f);
}
static void editor_reader_stale_tables(unsigned bits)
{
    struct editor_reader_fixture *f=editor_reader_make(bits);struct pt_paula_readers_song_status status={0},before;
    struct pt_sample *samples;size_t sample_bytes;uint32_t revision;
    editor_reader_publish_setup(f,&status);samples=f->editor->project->samples;
    sample_bytes=f->editor->project->sample_count*sizeof(*samples);revision=f->editor->history.revision;
    f->editor->project->samples=NULL;++f->editor->history.revision;POISON(samples,sample_bytes);
    memset(&status,0x5a,sizeof(status));before=status;
    assert(pt_editor_paula_readers_prepare_get(&f->control)==PT_EDITOR_READERS_CANCELLED);
    assert(pt_editor_paula_readers_prepare_step(&f->control,64,&status)==PT_EDITOR_READERS_CANCELLED&&!memcmp(&status,&before,sizeof(status)));
    assert(pt_editor_paula_readers_prepare_close(&f->control));
    UNPOISON(samples,sample_bytes);f->editor->project->samples=samples;f->editor->history.revision=revision;
    editor_reader_drop(f,0);
}
/* HOST_ONLY constructor leaves the included baseline main byte-exact and runs
 * all its genuine existing song regressions afterward. No native entrypoint. */
static void __attribute__((constructor)) editor_reader_fixture_main(void)
{
    unsigned bits,mode;fixture_command_polls=fixture_reader_polls=0;
    for(bits=8;bits<=24;bits+=8){for(mode=0;mode<24;++mode)editor_reader_lifecycle(bits,mode);
        for(mode=0;mode<27;++mode)editor_reader_admission(bits,mode);
        for(mode=0;mode<8;++mode)editor_reader_domain_case(bits,mode);
        editor_reader_whole(bits);
        editor_reader_output_aliases(bits);editor_reader_stale_tables(bits);editor_reader_terminal(bits);}
    puts("EDITOR PAULA READERS PREPARE PASS:72 genuine8/16/24 establishment/handoff/edit-undo-dispose/cancel/failure/reentry/exact-save lifetimes;81 full-span control/context/workspace/callback admissions;24 real ABI2 exact-grid late/uncertain/callback-reentry/separate command-reader drain cases;3 full song/oracle WAIT_ACTIVE/pressure/original terminal STOP schedules;3 complete producer/child/representation output-alias groups;3 poisoned former-table and3 expired-terminal cases; no timer or native voice start");
}
