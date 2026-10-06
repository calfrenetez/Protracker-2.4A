#define PT_TEST_EDITOR_MIXED_EXEC
#include "editor_mixed_test.c"
#include "../src/editor/editor_mixed_establish.h"
struct editor_establish_fixture {
    struct pt_editor_mixed_establish control;
    struct pt_editor_mixed binding;
    struct pt_editor *editor;struct pt_document document;
    struct pt_sampler_storage_span parent;
    int32_t *pcm;void *alias;
    unsigned calls,releases,live,alias_releases,allocation_hook,release_hook,reentries;
};
static void reenter_change(struct editor_establish_fixture *g)
{
    struct pt_editor *e=g->editor;unsigned revision=e->history.revision,generation=e->sampler.generation;
    assert(!pt_editor_prepare_change(e));
    e->editing=1;e->row=0;e->panel=0;e->project->channels.selected=4;
    pt_editor_key(e,0x46,0);
    assert(e->history.revision==revision&&e->sampler.generation==generation&&e->project->events[4].kind==PT_NOTE_PERIOD);
    assert(!pt_editor_dispose(e)&&!pt_editor_mixed_detach(&g->binding));
    assert(g->binding.preparation_context==&g->control&&g->control.binding==&g->binding);
    ++g->reentries;
}
static void *editor_establish_allocate(void *context,size_t bytes)
{
    struct editor_establish_fixture *g=context;void *p;++g->calls;
    assert(g->binding.preparation_context==&g->control&&g->binding.preparation_close);
    if(g->calls==g->allocation_hook)reenter_change(g);
    if(g->alias)return g->alias;
    p=malloc(bytes);if(p)++g->live;return p;
}
static void editor_establish_release(void *context,void *p)
{
    struct editor_establish_fixture *g=context;
    if(p==g->alias){++g->alias_releases;return;}
    if(g->release_hook)reenter_change(g);
    assert(p&&g->live);--g->live;++g->releases;free(p);
}
static struct editor_establish_fixture *editor_establish_make(unsigned bits)
{
    struct editor_establish_fixture *g=calloc(1,sizeof(*g));struct pt_allocator a={NULL,fast_alloc,fast_free};unsigned i;
    assert(g);g->editor=malloc(sizeof(*g->editor));g->pcm=malloc(3*4096*sizeof(*g->pcm));assert(g->editor&&g->pcm);
    for(i=0;i<3*4096;++i)g->pcm[i]=((int32_t)(i%120)-60)*(bits==8?1:bits==16?251:65537);
    pt_document_init(&g->document,&a);assert(pt_document_new(&g->document,16,SIZE_MAX)==PT_PROJECT_OK);
    g->document.project.sample_count=3;
    for(i=0;i<3;++i){g->document.project.samples[i].pcm=(struct pt_pcm){g->pcm+i*4096,4096,2048,8000,1,(uint8_t)bits};g->document.project.samples[i].volume=64;}
    g->document.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    assert(pt_editor_init(g->editor,&g->document.project));
    g->editor->sampler.allocator=(struct pt_allocator){g,editor_establish_allocate,editor_establish_release};
    g->editor->sampler.budget=1024*1024;g->parent=(struct pt_sampler_storage_span){g,sizeof(*g)};
    assert(pt_editor_mixed_attach(&g->binding,g->editor));return g;
}
static enum pt_establish_result editor_establish_step(struct editor_establish_fixture *g)
{return pt_editor_mixed_establish_step(&g->control,32);}
static void editor_establish_drop(struct editor_establish_fixture *g)
{
    g->alias=NULL;g->release_hook=0;
    assert(pt_editor_mixed_stop(&g->binding)&&!g->control.binding&&!g->control.job.active);
    assert(!g->binding.preparation_close&&!g->binding.preparation_context&&!g->alias_releases);
    assert(pt_editor_mixed_detach(&g->binding)&&pt_editor_dispose(g->editor)&&!g->live);
    pt_document_release(&g->document);free(g->pcm);free(g->editor);free(g);
}
static void editor_establish_case(unsigned bits,unsigned mode)
{
    struct editor_establish_fixture *g=editor_establish_make(bits);enum pt_establish_result r;unsigned n=0,i,live;
    size_t bytes,done;uint8_t *before,*after;struct pt_sample_version *masters[3];
    assert(pt_project_size(&g->document.project,&bytes)==PT_PROJECT_OK);before=malloc(bytes);after=malloc(bytes);assert(before&&after);
    assert(pt_project_encode(&g->document.project,before,bytes,&done)==PT_PROJECT_OK&&done==bytes);
    if(mode==6)g->allocation_hook=1;
    r=pt_editor_mixed_establish_begin(&g->control,&g->binding,&g->parent,1);
    assert(r==(mode==6?PT_ESTABLISH_FAULT:PT_ESTABLISH_PENDING));
    assert(g->binding.preparation_context==&g->control&&!g->binding.owner&&!g->binding.transport);
    assert(pt_editor_mixed_establish_begin(&g->control,&g->binding,&g->parent,1)==PT_ESTABLISH_INVALID);
    assert(pt_editor_mixed_begin(&g->binding,NULL,NULL,NULL,NULL,NULL)==PT_MIXED_OWNER_INVALID);
    assert(pt_editor_mixed_prepare(&g->binding,NULL)==PT_MIXED_OWNER_INVALID);
    assert(pt_editor_mixed_start(&g->binding,NULL,0,0,NULL)==PT_MIXED_OWNER_INVALID);
    assert(pt_editor_mixed_service(&g->binding)==PT_MIXED_OWNER_INVALID&&!pt_editor_mixed_signal(&g->binding));
    if(mode==0||mode==6)goto cancel;
    assert(editor_establish_step(g)==PT_ESTABLISH_PENDING&&g->control.job.validation.project);
    if(mode==1)goto cancel;
    while(g->control.job.validation.project)assert(editor_establish_step(g)==PT_ESTABLISH_PENDING&&++n<10000);
    assert(g->calls==1&&!g->editor->sampler.bytes);
    if(mode==2)goto cancel;
    if(mode==7){g->allocation_hook=2;assert(editor_establish_step(g)==PT_ESTABLISH_FAULT&&g->calls==2);goto cancel;}
    assert(editor_establish_step(g)==PT_ESTABLISH_PENDING&&g->control.job.job.owner);
    assert(editor_establish_step(g)==PT_ESTABLISH_PENDING&&!g->editor->sampler.current[0]);
    if(mode==3||mode==8)goto cancel;
    while(!g->editor->sampler.current[0])assert(editor_establish_step(g)==PT_ESTABLISH_PENDING&&++n<10000);
    if(mode==4){assert(editor_establish_step(g)==PT_ESTABLISH_PENDING);assert(editor_establish_step(g)==PT_ESTABLISH_PENDING);goto cancel;}
    do{r=editor_establish_step(g);assert(++n<10000);}while(r==PT_ESTABLISH_PENDING);
    assert(r==PT_ESTABLISH_READY&&g->calls==4&&g->live==4);
cancel:
    memcpy(masters,g->editor->sampler.current,sizeof(masters));live=0;
    for(i=0;i<3;++i)if(masters[i])++live;
    if(mode==8)g->release_hook=1;
    assert(pt_editor_prepare_change(g->editor));
    assert(!g->control.binding&&!g->control.job.active&&!g->control.job.validation.project&&!g->control.job.job.owner);
    assert(!g->binding.preparation_context&&!g->binding.preparation_close&&g->live==live);
    assert(!memcmp(masters,g->editor->sampler.current,sizeof(masters)));
    g->document.project.channels.selected=0;
    assert(pt_project_encode(&g->document.project,after,bytes,&done)==PT_PROJECT_OK&&done==bytes&&!memcmp(before,after,bytes));
    assert((mode==6||mode==7||mode==8)?g->reentries>0:g->reentries==0);
    /* The same real barrier is exercised by an actual key mutation/disposal. */
    g->release_hook=0;
    assert(pt_editor_mixed_establish_begin(&g->control,&g->binding,&g->parent,1)==PT_ESTABLISH_PENDING);
    if(mode%2){assert(pt_editor_dispose(g->editor));assert(!g->control.binding&&!g->live);}
    else {unsigned revision=g->editor->history.revision;
        g->editor->editing=1;g->editor->row=0;g->editor->panel=0;g->document.project.channels.selected=4;
        pt_editor_key(g->editor,0x46,0);assert(g->editor->history.revision!=revision&&g->document.project.events[4].kind==PT_NOTE_NONE);
        assert(!g->control.binding&&!g->binding.preparation_context);}
    free(before);free(after);editor_establish_drop(g);
}
static void editor_establish_alias(unsigned bits,unsigned mode)
{
    struct editor_establish_fixture *g=editor_establish_make(bits);unsigned n=0,master=mode>=4;
    struct pt_editor_mixed binding=g->binding;struct pt_editor *copy=malloc(sizeof(*copy));assert(copy);memcpy(copy,g->editor,sizeof(*copy));
    if(master){assert(pt_editor_mixed_establish_begin(&g->control,&g->binding,&g->parent,1)==PT_ESTABLISH_PENDING);
        assert(editor_establish_step(g)==PT_ESTABLISH_PENDING);
        while(g->control.job.validation.project)assert(editor_establish_step(g)==PT_ESTABLISH_PENDING&&++n<10000);}
    switch(mode%4){case 0:g->alias=&g->binding;break;case 1:g->alias=g->editor;break;
        case 2:g->alias=&g->control;break;default:g->alias=&g->parent;break;}
    assert((master?editor_establish_step(g):pt_editor_mixed_establish_begin(&g->control,&g->binding,&g->parent,1))==PT_ESTABLISH_ALIAS);
    assert(!g->alias_releases&&g->live==master&&!g->editor->sampler.bytes);
    /* Only private adoption bookkeeping changed; no editor/source initialization. */
    assert(!memcmp(copy,g->editor,sizeof(*copy)));binding.preparation_close=g->binding.preparation_close;binding.preparation_context=g->binding.preparation_context;
    assert(!memcmp(&binding,&g->binding,sizeof(binding)));free(copy);editor_establish_drop(g);
}
int main(void)
{
    unsigned bits,mode;assert(!editor_mixed_fixture());
    for(bits=8;bits<=24;bits+=8){for(mode=0;mode<9;++mode)editor_establish_case(bits,mode);
        for(mode=0;mode<8;++mode)editor_establish_alias(bits,mode);}
    puts("EDITOR ESTABLISH PASS:27 actual editor cancellation/reentry/exact-save/master-retention cases;24 allocator control-alias cases;15 legacy transport regressions; host only, no checked owner/UI/native output acceptance");return 0;
}
