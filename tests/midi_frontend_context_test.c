#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../src/native/midi_frontend_context.h"
#define LIMIT (1024UL*1024UL)
#define MAGIC 0x31415926UL
struct entry {void *pointer;size_t bytes;unsigned workspace;};
/* Complete allocator context holds pointers, never the adopted objects. */
struct ledger {
    struct entry entries[128];size_t bytes;
    unsigned live,calls,frees,workspace_frees,record_frees;
    unsigned expect_workspace,fail,alias,reentry_mode,reentries,hooks,veto;
    unsigned long magic;void *workspace;
    struct pt_midi_frontend_context *frontend;
    struct pt_midi_setup_owner *owner;struct pt_editor *editor;
};
struct rig {
    struct ledger *ledger;struct pt_document *document;struct pt_editor *editor;
    struct pt_midi_frontend_context *frontend;struct pt_midi_setup_owner *owner;
    size_t frontend_capacity,owner_capacity;
};
static struct rig *retained[4];static unsigned completed;
static void reentry(struct ledger *l)
{
    enum pt_midi_setup_result a,b,c;
    if(!l->reentry_mode)return;
    a=pt_midi_frontend_context_begin(l->frontend,0);
    b=pt_midi_frontend_context_end(l->frontend);
    c=pt_midi_frontend_context_dispose_close(l->frontend);
    if(l->reentry_mode==2){assert(a==PT_MIDI_SETUP_IN_USE&&b==PT_MIDI_SETUP_BUSY&&c==PT_MIDI_SETUP_IN_USE);}
    else assert(a==PT_MIDI_SETUP_BUSY&&b==PT_MIDI_SETUP_BUSY&&c==PT_MIDI_SETUP_BUSY);
    assert(!pt_midi_frontend_context_model(l->frontend));++l->reentries;
}
static void *allocate(void *context,size_t bytes)
{
    struct ledger *l=context;unsigned i;void *p;
    assert(l&&l->magic==MAGIC&&l->bytes<=LIMIT);++l->calls;reentry(l);
    if(l->alias)return l->editor; /* Known original-source alias, not fresh memory. */
    if(l->fail||bytes>LIMIT-l->bytes)return NULL;
    p=malloc(bytes);assert(p);
    for(i=0;i<128&&l->entries[i].pointer;++i){}
    assert(i<128);l->entries[i]=(struct entry){p,bytes,l->expect_workspace};
    if(l->expect_workspace){assert(!l->workspace);l->workspace=p;l->expect_workspace=0;}
    ++l->live;l->bytes+=bytes;return p;
}
static void release(void *context,void *pointer)
{
    struct ledger *l=context;unsigned i;assert(l&&l->magic==MAGIC);reentry(l);
    for(i=0;i<128&&l->entries[i].pointer!=pointer;++i){}
    assert(i<128&&l->live&&l->bytes>=l->entries[i].bytes);
    if(l->entries[i].workspace){++l->workspace_frees;l->workspace=NULL;}
    else if(l->entries[i].bytes==pt_midi_setup_command_bytes())++l->record_frees;
    l->bytes-=l->entries[i].bytes;--l->live;++l->frees;
    l->entries[i]=(struct entry){0};free(pointer);
}
static int barrier(void *context)
{struct ledger *l=context;assert(l->magic==MAGIC);++l->hooks;return !l->veto;}
static struct rig *create(void)
{
    struct rig *r=calloc(1,sizeof(*r));struct pt_allocator a;assert(r);
    r->ledger=calloc(1,sizeof(*r->ledger));r->document=calloc(1,sizeof(*r->document));
    r->editor=calloc(1,sizeof(*r->editor));
    r->frontend_capacity=pt_midi_frontend_context_size()+16;
    r->owner_capacity=pt_midi_setup_owner_size()+16;
    r->frontend=calloc(1,r->frontend_capacity);r->owner=calloc(1,r->owner_capacity);
    assert(r->ledger&&r->document&&r->editor&&r->frontend&&r->owner);
    r->ledger->magic=MAGIC;r->ledger->frontend=r->frontend;r->ledger->owner=r->owner;r->ledger->editor=r->editor;
    a=(struct pt_allocator){r->ledger,allocate,release};pt_document_init(r->document,&a);
    assert(pt_document_new(r->document,4,LIMIT)==PT_PROJECT_OK&&pt_editor_init(r->editor,&r->document->project));
    /* Rebind only genuinely empty components before any master/work exists. */
    pt_sampler_init(&r->editor->sampler,&a,LIMIT);pt_document_init(&r->editor->sample_source,&a);
    pt_song_init(&r->editor->song,&a,LIMIT);assert(pt_editor_change_barrier(r->editor,barrier,r->ledger));
    memset((char *)r->frontend+pt_midi_frontend_context_size(),0x6d,16);
    memset((char *)r->owner+pt_midi_setup_owner_size(),0x7b,16);return r;
}
static void tails(struct rig *r)
{
    unsigned i;for(i=0;i<16;++i){
        assert(((unsigned char *)r->frontend)[pt_midi_frontend_context_size()+i]==0x6d);
        assert(((unsigned char *)r->owner)[pt_midi_setup_owner_size()+i]==0x7b);
    }
}
static enum pt_midi_setup_result construct(struct rig *r)
{
    enum pt_midi_setup_result x;r->ledger->expect_workspace=1;
    x=pt_midi_frontend_context_construct(r->frontend,r->frontend_capacity,r->owner,r->owner_capacity,r->editor,
        (struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},4*pt_midi_setup_command_bytes());
    r->ledger->expect_workspace=0;tails(r);return x;
}
static struct pt_midi_setup_owner_status status(struct rig *r)
{struct pt_midi_setup_owner_status s;assert(pt_midi_setup_owner_get(r->owner,&s)==PT_MIDI_SETUP_OK);return s;}
static void text(struct pt_midi_requester_model *m,enum pt_midi_setup_target target,const char *s)
{
    const unsigned char *p=(const unsigned char *)s;
    assert(m&&pt_midi_requester_model_select(m,target)==PT_MIDI_SETUP_OK);
    assert(pt_midi_requester_model_key(m,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
    while(*p)assert(pt_midi_requester_model_key(m,PT_MIDI_KEY_CHARACTER,*p++)==PT_MIDI_EVENT_CHANGED);
}
static void apply(struct rig *r,unsigned track,enum pt_midi_setup_target target,const char *s)
{
    struct pt_midi_requester_model *m;
    assert(pt_midi_frontend_context_begin(r->frontend,track)==PT_MIDI_SETUP_OK);
    m=pt_midi_frontend_context_model(r->frontend);assert(m);text(m,target,s);
    assert(pt_midi_requester_model_apply(m)==PT_MIDI_SETUP_OK&&!m->self);
    assert(!pt_midi_frontend_context_model(r->frontend)&&pt_midi_frontend_context_state(r->frontend)==PT_MIDI_FRONTEND_MODAL);
    assert(pt_midi_frontend_context_end(r->frontend)==PT_MIDI_SETUP_OK);
}
static void finish(struct rig *r,int constructed)
{
    unsigned reentries=r->ledger->reentries,expected=0;
    if(constructed&&r->ledger->reentry_mode==3)expected=status(r).commands+1;
    if(constructed){assert(pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_OK);
        assert(pt_midi_frontend_context_state(r->frontend)==PT_MIDI_FRONTEND_CLOSED&&r->ledger->workspace_frees==1);
        if(expected){assert(r->ledger->reentries==reentries+expected);r->ledger->reentry_mode=0;}}
    else assert(pt_editor_dispose(r->editor));
    tails(r);free(r->owner);free(r->frontend);free(r->editor);
    assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);free(r->document);
    assert(!r->ledger->live&&!r->ledger->bytes&&r->ledger->magic==MAGIC);
    free(r->ledger);free(r);++completed;
}
static void preflight_and_null(void)
{
    struct rig *r=create();unsigned calls=r->ledger->calls;size_t i;
    unsigned char *front=malloc(r->frontend_capacity),*owner=malloc(r->owner_capacity);assert(front&&owner);
    memcpy(front,r->frontend,r->frontend_capacity);memcpy(owner,r->owner,r->owner_capacity);
    assert(pt_midi_frontend_context_construct(r->frontend,pt_midi_frontend_context_size()-1,r->owner,r->owner_capacity,r->editor,
        (struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_frontend_context_construct(r->frontend,r->frontend_capacity,r->owner,pt_midi_setup_owner_size()-1,r->editor,
        (struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_frontend_context_construct((struct pt_midi_frontend_context *)r->owner,r->frontend_capacity,r->owner,r->owner_capacity,r->editor,
        (struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_frontend_context_construct(r->frontend,r->frontend_capacity,r->owner,r->owner_capacity,r->editor,
        (struct pt_midi_setup_owner_span){r->frontend,r->frontend_capacity},0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_frontend_context_construct((struct pt_midi_frontend_context *)r->editor,r->frontend_capacity,r->owner,r->owner_capacity,r->editor,
        (struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},0)==PT_MIDI_SETUP_INVALID);
    ((unsigned char *)r->frontend)[0]=1;
    assert(construct(r)==PT_MIDI_SETUP_BUSY&&((unsigned char *)r->frontend)[0]==1);
    ((unsigned char *)r->frontend)[0]=0;
    {
        size_t align=pt_midi_setup_owner_alignment(),offset=pt_midi_frontend_context_size();
        size_t bytes;unsigned char *slab;struct pt_midi_setup_owner *overlap;
        offset+=(align-offset%align)%align;bytes=offset+r->owner_capacity;
        slab=calloc(1,bytes);assert(slab);overlap=(struct pt_midi_setup_owner *)(slab+offset);
        /* Both prefixes fit and do not overlap, but the truthful complete
         * frontend extent includes the owner. Full capacity must refuse. */
        assert(pt_midi_frontend_context_construct((struct pt_midi_frontend_context *)slab,bytes,
            overlap,r->owner_capacity,r->editor,(struct pt_midi_setup_owner_span){r->ledger,sizeof(*r->ledger)},0)==PT_MIDI_SETUP_INVALID);
        for(i=0;i<bytes;++i)assert(!slab[i]);free(slab);
    }
    assert(!memcmp(front,r->frontend,r->frontend_capacity)&&!memcmp(owner,r->owner,r->owner_capacity)&&r->ledger->calls==calls);
    r->ledger->fail=1;assert(construct(r)==PT_MIDI_SETUP_CAPACITY);r->ledger->fail=0;
    for(i=0;i<pt_midi_frontend_context_size();++i)assert(!((unsigned char *)r->frontend)[i]);
    for(i=0;i<pt_midi_setup_owner_size();++i)assert(!((unsigned char *)r->owner)[i]);
    assert(!r->ledger->workspace&&!r->ledger->workspace_frees);free(front);free(owner);finish(r,0);
}
static void failed_open_and_once_return(void)
{
    struct rig *r=create();struct pt_midi_setup_owner_borrow a,b;unsigned calls,hooks;
    assert(construct(r)==PT_MIDI_SETUP_OK);calls=r->ledger->calls;hooks=r->ledger->hooks;
    assert(pt_midi_setup_owner_borrow(r->owner,&a)==PT_MIDI_SETUP_OK&&pt_midi_setup_owner_return(r->owner,&a)==PT_MIDI_SETUP_OK);
    assert(pt_midi_frontend_context_begin(r->frontend,99)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_frontend_context_state(r->frontend)==PT_MIDI_FRONTEND_LIVE&&!status(r).modal);
    assert(pt_midi_frontend_context_end(r->frontend)==PT_MIDI_SETUP_IN_USE);
    assert(pt_midi_setup_owner_borrow(r->owner,&b)==PT_MIDI_SETUP_OK&&b.serial==a.serial+2);
    assert(pt_midi_setup_owner_return(r->owner,&b)==PT_MIDI_SETUP_OK);
    assert(pt_midi_frontend_context_begin(r->frontend,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_IN_USE);
    assert(pt_midi_frontend_context_end(r->frontend)==PT_MIDI_SETUP_OK&&pt_midi_frontend_context_end(r->frontend)==PT_MIDI_SETUP_IN_USE);
    assert(r->ledger->calls==calls&&r->ledger->hooks==hooks);finish(r,1);
}
static void records_redo_saved_roundtrip_and_disposal(void)
{
    struct rig *r=create();struct pt_project *p=&r->document->project;struct pt_channels channels;
    struct pt_event_update note={0,{428,0,PT_NOTE_PERIOD,1,0,0,0,0}};struct pt_document decoded;
    struct pt_allocator a={r->ledger,allocate,release};char inactive[12][PT_MIDI_ENDPOINT];
    uint8_t *encoded;size_t bytes,written;uint32_t revision;unsigned frees,record_frees;
    assert(pt_channels_route(&p->channels,1,PT_AMIGUS)==PT_CHANNEL_OK&&pt_channels_route(&p->channels,2,PT_MIDI)==PT_CHANNEL_OK);
    memset(p->midi_output+4,0x7e,sizeof(inactive));memcpy(inactive,p->midi_output+4,sizeof(inactive));channels=p->channels;
    assert(construct(r)==PT_MIDI_SETUP_OK);apply(r,2,PT_MIDI_SETUP_OUTPUT,"synth");
    assert(pt_pattern_apply(p,&r->editor->history,&note,1)==PT_EDIT_OK);apply(r,0,PT_MIDI_SETUP_INPUT,"keys");
    assert(!memcmp(&p->channels,&channels,sizeof(channels))&&!memcmp(p->midi_output+4,inactive,sizeof(inactive)));
    pt_editor_saved(r->editor);revision=r->editor->history.revision;
    assert(pt_pattern_undo(p,&r->editor->history,-1)==PT_EDIT_OK&&!p->midi_input[0]);
    assert(status(r).commands==2&&status(r).metadata_bytes==2*pt_midi_setup_command_bytes());
    assert(pt_midi_frontend_context_begin(r->frontend,0)==PT_MIDI_SETUP_OK);
    text(pt_midi_frontend_context_model(r->frontend),PT_MIDI_SETUP_INPUT,"canceled");
    assert(pt_midi_frontend_context_end(r->frontend)==PT_MIDI_SETUP_OK&&r->editor->history.count==3&&r->editor->history.cursor==2);
    assert(status(r).commands==2&&pt_pattern_undo(p,&r->editor->history,1)==PT_EDIT_OK);
    assert(r->editor->history.revision==revision&&!pt_editor_dirty(r->editor));
    assert(pt_project_size(p,&bytes)==PT_PROJECT_OK);encoded=malloc(bytes);assert(encoded);
    assert(pt_project_encode(p,encoded,bytes,&written)==PT_PROJECT_OK&&written==bytes);
    pt_document_init(&decoded,&a);assert(pt_document_load(&decoded,encoded,bytes,LIMIT)==PT_PROJECT_OK);
    assert(!strcmp(decoded.project.midi_input,"keys")&&!strcmp(decoded.project.midi_output[2],"synth")&&
        !memcmp(&decoded.project.channels,&p->channels,sizeof(p->channels))&&decoded.project.events[0].pitch==428);
    assert(pt_document_release_checked(&decoded)==PT_DOCUMENT_RELEASED);free(encoded);
    frees=r->ledger->frees;record_frees=r->ledger->record_frees;
    assert(pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_OK&&r->ledger->frees==frees+3);
    assert(r->ledger->record_frees==record_frees+2&&r->ledger->workspace_frees==1);
    tails(r);free(r->owner);free(r->frontend);free(r->editor);
    assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);free(r->document);
    assert(!r->ledger->live&&!r->ledger->bytes);free(r->ledger);free(r);++completed;
}
static void callback_busy(void)
{
    struct rig *r=create();r->ledger->reentry_mode=1;
    assert(construct(r)==PT_MIDI_SETUP_OK&&r->ledger->reentries==1);
    r->ledger->reentry_mode=2;apply(r,0,PT_MIDI_SETUP_INPUT,"keys");assert(r->ledger->reentries==2);
    r->ledger->reentry_mode=3;finish(r,1);
}
static void constructor_known_alias_held(void)
{
    struct rig *r=create();struct pt_editor *before=malloc(sizeof(*before));unsigned calls,frees;assert(before);
    memcpy(before,r->editor,sizeof(*before));r->ledger->alias=1;assert(construct(r)==PT_MIDI_SETUP_HELD);
    assert(!memcmp(before,r->editor,sizeof(*before)));free(before);calls=r->ledger->calls;frees=r->ledger->frees;
    assert(pt_midi_frontend_context_state(r->frontend)==PT_MIDI_FRONTEND_HELD);
    assert(pt_midi_frontend_context_begin(r->frontend,0)==PT_MIDI_SETUP_HELD&&pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_HELD);
    assert(r->ledger->calls==calls&&r->ledger->frees==frees&&!r->ledger->workspace_frees);retained[0]=r;
}
static void actual_apply_held(void)
{
    struct rig *r=create();struct pt_midi_requester_model *m;unsigned calls,frees;
    assert(construct(r)==PT_MIDI_SETUP_OK&&pt_midi_frontend_context_begin(r->frontend,0)==PT_MIDI_SETUP_OK);
    m=pt_midi_frontend_context_model(r->frontend);text(m,PT_MIDI_SETUP_INPUT,"keys");r->ledger->alias=1;
    assert(pt_midi_requester_model_apply(m)==PT_MIDI_SETUP_HELD&&pt_midi_frontend_context_end(r->frontend)==PT_MIDI_SETUP_HELD);
    calls=r->ledger->calls;frees=r->ledger->frees;
    assert(pt_midi_frontend_context_end(r->frontend)==PT_MIDI_SETUP_HELD&&pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_HELD);
    assert(r->ledger->calls==calls&&r->ledger->frees==frees&&!r->ledger->workspace_frees);retained[1]=r;
}
static void genuine_dispose_veto_held(void)
{
    struct rig *r=create();unsigned hooks,frees;assert(construct(r)==PT_MIDI_SETUP_OK);apply(r,0,PT_MIDI_SETUP_INPUT,"keys");
    r->ledger->veto=1;hooks=r->ledger->hooks;frees=r->ledger->frees;
    assert(pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_HELD);
    assert(r->ledger->hooks==hooks+1&&r->ledger->frees==frees&&status(r).commands==1);
    assert(pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_HELD&&r->ledger->hooks==hooks+1);
    assert(pt_midi_frontend_context_state(r->frontend)==PT_MIDI_FRONTEND_HELD&&!r->ledger->workspace_frees);retained[2]=r;
}
static void external_genuine_owner_held_latches(void)
{
    struct rig *r=create();struct pt_midi_setup_owner_borrow b;unsigned calls,frees;
    assert(construct(r)==PT_MIDI_SETUP_OK&&pt_midi_setup_owner_borrow(r->owner,&b)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_begin(b.setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK&&pt_midi_setup_name(b.setup,"keys",5)==PT_MIDI_SETUP_OK);
    r->ledger->alias=1;assert(pt_midi_setup_commit(b.setup)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_setup_owner_return(r->owner,&b)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_frontend_context_state(r->frontend)==PT_MIDI_FRONTEND_LIVE);calls=r->ledger->calls;frees=r->ledger->frees;
    assert(pt_midi_frontend_context_begin(r->frontend,0)==PT_MIDI_SETUP_HELD&&pt_midi_frontend_context_state(r->frontend)==PT_MIDI_FRONTEND_HELD);
    assert(pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_HELD&&r->ledger->calls==calls&&r->ledger->frees==frees);retained[3]=r;
}
static void closed_reads_no_former_sources(void)
{
    struct rig *r=create();unsigned frees;assert(construct(r)==PT_MIDI_SETUP_OK);
    assert(pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_OK);tails(r);
    free(r->editor);r->editor=NULL;assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);free(r->document);r->document=NULL;
    free(r->owner);r->owner=NULL;frees=r->ledger->frees;
    assert(pt_midi_frontend_context_state(r->frontend)==PT_MIDI_FRONTEND_CLOSED);
    assert(pt_midi_frontend_context_begin(r->frontend,0)==PT_MIDI_SETUP_IN_USE&&pt_midi_frontend_context_end(r->frontend)==PT_MIDI_SETUP_IN_USE);
    assert(pt_midi_frontend_context_dispose_close(r->frontend)==PT_MIDI_SETUP_IN_USE&&!pt_midi_frontend_context_model(r->frontend));
    assert(r->ledger->frees==frees&&!r->ledger->live&&!r->ledger->bytes);free(r->frontend);free(r->ledger);free(r);++completed;
}
int main(void)
{
    preflight_and_null();failed_open_and_once_return();records_redo_saved_roundtrip_and_disposal();callback_busy();
    constructor_known_alias_held();actual_apply_held();genuine_dispose_veto_held();external_genuine_owner_held_latches();closed_reads_no_former_sources();
    assert(completed==5&&retained[0]&&retained[1]&&retained[2]&&retained[3]);
    puts("MIDI FRONTEND CONTEXT HOST PASS:9 groups;5 complete/4 retained;real constructor/modal/journal/dispose;NO NATIVE FRONTEND/TASK/OUTPUT AUTHORITY");return 0;
}
