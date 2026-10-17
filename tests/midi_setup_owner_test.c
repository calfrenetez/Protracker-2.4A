#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../src/editor/midi_setup_owner.h"
#define LIMIT (1024UL*1024UL)
#define MAGIC 0x31415926UL
struct entry {void *pointer;size_t bytes;unsigned workspace;};
/* This exact allocator/barrier context contains POINTERS only to the separately
 * allocated owner/editor/doc. No sidecar, workspace, modal/ticket is embedded. */
struct context {
    struct entry entries[128];size_t bytes;unsigned live,calls,frees,workspace_frees;
    unsigned fail,alias,expect_workspace,reenter,hooks,reentries;
    unsigned long magic;
    struct pt_midi_setup_owner *owner;size_t owner_capacity;
    struct pt_editor *editor;void *workspace;
};
struct rig {
    struct context *context;struct pt_document *document;struct pt_editor *editor;
    struct pt_midi_setup_owner *owner;size_t owner_capacity;
};
static struct rig *retained[2];
static unsigned completed;
static void busy_witness(struct context *c)
{
    struct pt_midi_setup_owner_borrow b;struct pt_midi_setup_owner_status s;
    memset(&b,0xa5,sizeof(b));memset(&s,0x5a,sizeof(s));
    assert(pt_midi_setup_owner_borrow(c->owner,&b)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_setup_owner_get(c->owner,&s)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_setup_owner_cancel_draft(c->owner)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_setup_owner_close_controller(c->owner)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_setup_owner_release_workspace(c->owner)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_setup_owner_rebind(c->owner,c->editor)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_setup_owner_init(c->owner,c->owner_capacity,c->editor,
        (struct pt_midi_setup_owner_span){c,sizeof(*c)},4*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_BUSY);
    ++c->reentries;
}
static void *allocate(void *context,size_t bytes)
{
    struct context *c=context;unsigned i;void *p;
    assert(c&&c->magic==MAGIC&&c->bytes<=LIMIT);++c->calls;
    if(c->reenter)busy_witness(c);
    if(c->alias)return c->editor; /* Deliberately violates fresh allocator contract. */
    if(c->fail||bytes>LIMIT-c->bytes)return NULL;
    p=malloc(bytes);assert(p);
    for(i=0;i<128&&c->entries[i].pointer;++i){}
    assert(i<128);c->entries[i]=(struct entry){p,bytes,c->expect_workspace};
    if(c->expect_workspace){assert(!c->workspace);c->workspace=p;c->expect_workspace=0;}
    c->bytes+=bytes;++c->live;return p;
}
static void release(void *context,void *pointer)
{
    struct context *c=context;unsigned i;
    assert(c&&c->magic==MAGIC);
    if(c->reenter)busy_witness(c);
    for(i=0;i<128&&c->entries[i].pointer!=pointer;++i){}
    assert(i<128&&c->live&&c->bytes>=c->entries[i].bytes);
    if(c->entries[i].workspace)++c->workspace_frees;
    c->bytes-=c->entries[i].bytes;--c->live;++c->frees;
    c->entries[i]=(struct entry){0};free(pointer);
}
static int barrier(void *context)
{struct context *c=context;assert(c->magic==MAGIC);++c->hooks;return 1;}
static struct rig *create(void)
{
    struct rig *r=calloc(1,sizeof(*r));struct pt_allocator a;unsigned char *tail;
    assert(r);r->context=calloc(1,sizeof(*r->context));r->document=calloc(1,sizeof(*r->document));
    r->editor=calloc(1,sizeof(*r->editor));r->owner_capacity=pt_midi_setup_owner_size()+16;
    r->owner=calloc(1,r->owner_capacity);assert(r->context&&r->document&&r->editor&&r->owner);
    r->context->magic=MAGIC;r->context->owner=r->owner;r->context->owner_capacity=r->owner_capacity;r->context->editor=r->editor;
    a=(struct pt_allocator){r->context,allocate,release};pt_document_init(r->document,&a);
    assert(pt_document_new(r->document,4,LIMIT)==PT_PROJECT_OK);
    assert(pt_editor_init(r->editor,&r->document->project));
    /* Bind only genuinely empty default components, before any retained work. */
    pt_sampler_init(&r->editor->sampler,&a,LIMIT);pt_document_init(&r->editor->sample_source,&a);
    pt_song_init(&r->editor->song,&a,LIMIT);
    assert(pt_editor_change_barrier(r->editor,barrier,r->context));
    tail=(unsigned char *)r->owner+pt_midi_setup_owner_size();memset(tail,0x7b,16);
    return r;
}
static void tail_unchanged(struct rig *r)
{unsigned i;const unsigned char *p=(const unsigned char *)r->owner+pt_midi_setup_owner_size();for(i=0;i<16;++i)assert(p[i]==0x7b);}
static enum pt_midi_setup_result initialize(struct rig *r,size_t budget)
{
    enum pt_midi_setup_result result;r->context->expect_workspace=1;
    result=pt_midi_setup_owner_init(r->owner,r->owner_capacity,r->editor,
        (struct pt_midi_setup_owner_span){r->context,sizeof(*r->context)},budget);
    r->context->expect_workspace=0;tail_unchanged(r);return result;
}
static struct pt_midi_setup_owner_status status(struct rig *r)
{
    struct pt_midi_setup_owner_status s;
    assert(pt_midi_setup_owner_get(r->owner,&s)==PT_MIDI_SETUP_OK);return s;
}
static struct pt_midi_setup_owner_borrow borrow(struct rig *r)
{
    struct pt_midi_setup_owner_borrow b;
    assert(pt_midi_setup_owner_borrow(r->owner,&b)==PT_MIDI_SETUP_OK);
    assert(b.owner==r->owner&&(void *)b.setup==r->context->workspace);return b;
}
static void put(struct rig *r,enum pt_midi_setup_target kind,const char *name)
{
    struct pt_midi_setup_owner_borrow b=borrow(r);
    assert(pt_midi_setup_begin(b.setup,kind,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_name(b.setup,name,strlen(name)+1)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_commit(b.setup)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_owner_return(r->owner,&b)==PT_MIDI_SETUP_OK);
}
static void dispose_editor(struct rig *r)
{assert(r->editor&&pt_editor_dispose(r->editor));}
static void finish(struct rig *r,int owner_released,int editor_disposed)
{
    if(!editor_disposed)dispose_editor(r);
    if(!owner_released){
        assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_OK);
        assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_OK);
    }
    tail_unchanged(r);free(r->owner);free(r->editor);
    assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);free(r->document);
    assert(!r->context->live&&!r->context->bytes&&r->context->magic==MAGIC);
    free(r->context);free(r);++completed;
}
static void constructor_refusal_and_null(void)
{
    struct rig *r=create();unsigned calls=r->context->calls,i;
    assert(pt_midi_setup_owner_init(r->owner,pt_midi_setup_owner_size()-1,r->editor,
        (struct pt_midi_setup_owner_span){r->context,sizeof(*r->context)},0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_owner_init(r->owner,r->owner_capacity,r->editor,
        (struct pt_midi_setup_owner_span){r->owner,r->owner_capacity},0)==PT_MIDI_SETUP_INVALID);
    assert(r->context->calls==calls);
    r->context->fail=1;assert(initialize(r,0)==PT_MIDI_SETUP_CAPACITY);r->context->fail=0;
    for(i=0;i<pt_midi_setup_owner_size();++i)assert(!((unsigned char *)r->owner)[i]);
    assert(!r->context->workspace&&!r->context->workspace_frees);
    dispose_editor(r);tail_unchanged(r);free(r->owner);free(r->editor);
    assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);free(r->document);
    assert(!r->context->live&&!r->context->bytes);free(r->context);free(r);++completed;
}
static void modal_identity_and_draft(void)
{
    struct rig *r=create();struct pt_midi_setup_owner_borrow first,second,bad;
    unsigned calls;
    assert(initialize(r,4*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK);first=borrow(r);bad=first;
    bad.setup=(struct pt_midi_setup *)r->editor;
    assert(pt_midi_setup_owner_return(r->owner,&bad)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_setup_owner_cancel_draft(r->owner)==PT_MIDI_SETUP_BUSY);
    calls=r->context->calls;
    assert(pt_midi_setup_owner_init(r->owner,r->owner_capacity,r->editor,
        (struct pt_midi_setup_owner_span){r->context,sizeof(*r->context)},0)==PT_MIDI_SETUP_BUSY);
    assert(r->context->calls==calls&&status(r).modal);
    assert(pt_midi_setup_begin(first.setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_name(first.setup,"uncommitted",12)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_owner_return(r->owner,&first)==PT_MIDI_SETUP_OK);
    assert(!r->document->project.midi_input[0]&&!status(r).commands&&!status(r).modal);
    second=borrow(r);assert(second.setup==first.setup&&second.serial>first.serial);
    assert(pt_midi_setup_owner_return(r->owner,&first)==PT_MIDI_SETUP_INVALID&&status(r).modal);
    assert(pt_midi_setup_owner_return(r->owner,&second)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_owner_cancel_draft(r->owner)==PT_MIDI_SETUP_OK);
    finish(r,0,0);
}
static void actual_records_redo_dispose_and_rebind(void)
{
    struct rig *r=create();struct pt_allocator a={r->context,allocate,release};
    struct pt_event_update note={0,{428,0,PT_NOTE_PERIOD,1,0,0,0,0}};
    struct pt_midi_setup_owner_borrow b,old;void *workspace;unsigned calls,frees;
    assert(initialize(r,4*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK);
    r->context->reenter=1;put(r,PT_MIDI_SETUP_INPUT,"keys");r->context->reenter=0;
    assert(r->context->reentries==1);
    assert(pt_pattern_apply(&r->document->project,&r->editor->history,&note,1)==PT_EDIT_OK);
    put(r,PT_MIDI_SETUP_OUTPUT,"synth");
    assert(r->editor->history.count==3&&status(r).commands==2);
    assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
    assert(!r->document->project.midi_output[0][0]&&status(r).commands==2);
    assert(status(r).metadata_bytes==2*pt_midi_setup_command_bytes());
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_IN_USE);
    assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_IN_USE);
    assert(!r->context->workspace_frees&&status(r).commands==2);
    old=borrow(r);assert(pt_midi_setup_owner_return(r->owner,&old)==PT_MIDI_SETUP_OK);
    workspace=r->context->workspace;frees=r->context->frees;dispose_editor(r);
    assert(r->context->frees==frees+2&&!r->context->workspace_frees);
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_OK);
    assert(status(r).state==PT_MIDI_OWNER_CLOSED&&!status(r).commands);
    /* Same original document remains live. Genuine init at the final address,
     * after actual disposal, is NOT a simulated document publication/transfer. */
    assert(pt_editor_init(r->editor,&r->document->project));pt_sampler_init(&r->editor->sampler,&a,LIMIT);
    pt_document_init(&r->editor->sample_source,&a);pt_song_init(&r->editor->song,&a,LIMIT);
    assert(pt_editor_change_barrier(r->editor,barrier,r->context));calls=r->context->calls;
    assert(pt_midi_setup_owner_rebind(r->owner,r->editor)==PT_MIDI_SETUP_OK);
    assert(r->context->calls==calls&&r->context->workspace==workspace);
    b=borrow(r);assert(b.serial>old.serial&&b.setup==old.setup);
    assert(pt_midi_setup_owner_return(r->owner,&old)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_begin(b.setup,PT_MIDI_SETUP_CLOCK,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_flags(b.setup,3)==PT_MIDI_SETUP_OK&&pt_midi_setup_commit(b.setup)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_owner_return(r->owner,&b)==PT_MIDI_SETUP_OK);
    assert(r->document->project.midi_flags==3&&status(r).commands==1);
    dispose_editor(r);assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_OK);
    frees=r->context->frees;assert(r->context->workspace_frees==1);
    assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_INVALID&&r->context->frees==frees);
    finish(r,1,1);
}
static void no_former_editor_or_project_access(void)
{
    struct rig *r=create();struct pt_midi_setup_owner_status s;
    assert(initialize(r,0)==PT_MIDI_SETUP_OK);dispose_editor(r);
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_OK);
    free(r->editor);r->editor=NULL;
    assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);free(r->document);r->document=NULL;
    /* ASan would reject any dereference of either genuinely ended source. */
    assert(pt_midi_setup_owner_get(r->owner,&s)==PT_MIDI_SETUP_OK&&s.state==PT_MIDI_OWNER_CLOSED);
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_OK);
    assert(!r->context->live&&!r->context->bytes&&r->context->workspace_frees==1);
    tail_unchanged(r);free(r->owner);free(r->context);free(r);++completed;
}
static void constructor_and_release_reentry(void)
{
    struct rig *r=create();unsigned count;
    r->context->reenter=1;assert(initialize(r,0)==PT_MIDI_SETUP_OK);r->context->reenter=0;
    assert(r->context->reentries==1);dispose_editor(r);
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_OK);
    r->context->reenter=1;count=r->context->reentries;
    assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_OK);
    r->context->reenter=0;assert(r->context->reentries==count+1&&r->context->workspace_frees==1);
    finish(r,1,1);
}
static void workspace_budget_and_known_outputs(void)
{
    struct rig *r=create();struct pt_midi_setup_owner_borrow b;unsigned calls;
    assert(initialize(r,0)==PT_MIDI_SETUP_OK);
    assert(status(r).workspace_bytes==pt_midi_setup_size()&&!status(r).metadata_budget);
    assert(pt_midi_setup_owner_get(r->owner,(struct pt_midi_setup_owner_status *)r->owner)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_owner_borrow(r->owner,(struct pt_midi_setup_owner_borrow *)r->context)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_owner_borrow(r->owner,(struct pt_midi_setup_owner_borrow *)r->context->workspace)==PT_MIDI_SETUP_INVALID);
    b=borrow(r);calls=r->context->calls;
    assert(pt_midi_setup_begin(b.setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_name(b.setup,"no budget",10)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_commit(b.setup)==PT_MIDI_SETUP_CAPACITY);
    assert(r->context->calls==calls&&!r->document->project.midi_input[0]);
    assert(pt_midi_setup_owner_return(r->owner,&b)==PT_MIDI_SETUP_OK);
    assert(!status(r).commands&&!status(r).metadata_bytes);finish(r,0,0);
}
static void retain_workspace_alias(void)
{
    struct rig *r=create();unsigned char *before=malloc(sizeof(*r->editor));unsigned frees=r->context->frees;
    struct pt_midi_setup_owner_borrow b;assert(before);memcpy(before,r->editor,sizeof(*r->editor));
    r->context->alias=1;assert(initialize(r,0)==PT_MIDI_SETUP_HELD);
    assert(!memcmp(before,r->editor,sizeof(*r->editor))&&r->context->frees==frees&&!r->context->workspace);
    free(before);assert(pt_midi_setup_owner_borrow(r->owner,&b)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_HELD);
    assert(r->context->frees==frees);tail_unchanged(r);retained[0]=r;
}
static void retain_genuine_controller_alias(void)
{
    struct rig *r=create();struct pt_midi_setup_owner_borrow b;struct pt_midi_setup_owner_status out,saved;
    unsigned frees;assert(initialize(r,4*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK);b=borrow(r);
    assert(pt_midi_setup_begin(b.setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_name(b.setup,"held",5)==PT_MIDI_SETUP_OK);frees=r->context->frees;r->context->alias=1;
    assert(pt_midi_setup_commit(b.setup)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_setup_owner_return(r->owner,&b)==PT_MIDI_SETUP_HELD);
    memset(&out,0x5a,sizeof(out));saved=out;
    assert(pt_midi_setup_owner_get(r->owner,&out)==PT_MIDI_SETUP_HELD&&!memcmp(&out,&saved,sizeof(out)));
    assert(pt_midi_setup_owner_cancel_draft(r->owner)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_HELD);
    assert(r->context->frees==frees&&!r->context->workspace_frees&&r->context->workspace);
    assert(!r->document->project.midi_input[0]&&!r->editor->history.count);tail_unchanged(r);retained[1]=r;
}
int main(void)
{
    constructor_refusal_and_null();modal_identity_and_draft();actual_records_redo_dispose_and_rebind();
    no_former_editor_or_project_access();constructor_and_release_reentry();workspace_budget_and_known_outputs();
    retain_workspace_alias();retain_genuine_controller_alias();
    assert(completed==6&&retained[0]&&retained[1]);
    puts("MIDI SETUP OWNER HOST PASS:8 groups;6 complete/2 retained;original allocator/workspace;genuine modal/journal/redo/dispose/rebind;NO DOCUMENT PUBLICATION/NATIVE AUTHORITY");
    return 0;
}
