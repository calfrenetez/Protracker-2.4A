#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "src/core/document.h"
#include "src/editor/editor.h"
#include "src/editor/sampler_lifetime_internal.h"
#include "src/editor/sampler_workflow.h"
#include "src/native/recovery.h"

/* HOST allocation boundary only. It models the native prefix/charge contract;
 * it is not an Exec allocator, native geometry or Task-lifetime qualification.
 * Real editor/document/sampler/song/history code uses these callbacks. */
struct host_master {size_t used,limit,allocations,releases,live;};
static struct host_master master_memory;
static struct pt_document *release_reentry_document;
static unsigned release_reentry_once,release_reentry_observed;
static size_t pt_master_memory_available(const struct host_master *m)
{return m->used<m->limit?m->limit-m->used:0;}
static void *pt_master_allocate(void *context,size_t bytes)
{
    struct host_master *m=context;size_t total,*p;
    if(!m || !bytes || bytes>SIZE_MAX-sizeof(size_t))return NULL;
    total=bytes+sizeof(size_t);
    if(total>pt_master_memory_available(m))return NULL;
    p=malloc(total);if(!p)return NULL;
    *p=total;m->used+=total;++m->allocations;++m->live;return p+1;
}
static void pt_master_release(void *context,void *pointer)
{
    struct host_master *m=context;size_t *p,total;
    if(!pointer)return;
    p=(size_t *)pointer-1;total=*p;
    assert(m && total<=m->used && m->live);
    m->used-=total;--m->live;++m->releases;free(p);
    if(release_reentry_once) {
        release_reentry_once=0;
        assert(release_reentry_document);
        assert(pt_document_release_checked(release_reentry_document)==PT_DOCUMENT_RELEASE_REFUSED);
        ++release_reentry_observed;
    }
}

/* These are exact selected definitions, not a second cleanup implementation.
 * Native recovery finish and its used recovery_store definitions are selected;
 * no DOS/Intuition/Wait function is simulated or invoked by this fixture. */
#include "recovery-store.selected.h"
#include "recovery-finish.selected.h"
#include "main-cleanup.selected.h"

struct rig {
    struct pt_document document;
    struct pt_editor *editor;
    struct pt_native_recovery recovery;
    struct pt_sample_usage_scan *scan;
    struct pt_sample_usage_preview *preview;
    struct pt_sample_version *pin;
    struct pt_pcm pinned;
    void *extra;
};
/* Deliberately refused owners stay rooted until their fresh fixture process
 * ends. This is not native HOLD, leak-sanitizer success or cleanup authority. */
static struct rig *retained;
static const uint8_t raw24[12]={1,0,0,255,255,127,0,0,128,254,255,255};
static const int32_t pcm24[4]={1,8388607,-8388608,-2};

static struct rig *create(void)
{
    struct rig *r=calloc(1,sizeof(*r));
    struct pt_allocator allocator={&master_memory,pt_master_allocate,pt_master_release};
    assert(r);memset(&master_memory,0,sizeof(master_memory));master_memory.limit=32U*1024U*1024U;
    pt_document_init(&r->document,&allocator);
    assert(pt_document_new(&r->document,4,SIZE_MAX)==PT_PROJECT_OK);
    r->editor=pt_master_allocate(&master_memory,sizeof(*r->editor));assert(r->editor);
    assert(editor_init_memory(r->editor,&r->document.project));
    assert(r->editor->sampler.allocator.context==&master_memory);
    return r;
}
static void sample(struct rig *r)
{
    struct pt_raw_format format={48000,24,1,1,0};
    assert(pt_sampler_import_raw(&r->editor->sampler,&r->document.project,
        &r->editor->history,0,raw24,sizeof(raw24),"cleanup24",&format)==PT_EDIT_OK);
    assert(r->document.project.samples[0].pcm.bits==24);
    assert(r->document.project.samples[0].pcm.frames==4);
    assert(!memcmp(r->document.project.samples[0].pcm.data,pcm24,sizeof(pcm24)));
}
static void finish(struct rig *r,int normal_exit,int recovery_clean,int expected)
{
    assert(cleanup_editor_owners(r->editor,&r->document));
    assert(!r->document.loaded && !r->document.allocated_bytes);
    assert(pt_sampler_lifetime_storage_retired(&r->editor->sampler));
    assert(!r->editor->song.current && !r->editor->song.bytes);
    assert(master_memory.used==sizeof(*r->editor)+sizeof(size_t));
    /* Actual main reaches its final RC helper only after native teardown and
     * this descriptor release. This fixture qualifies the owner/RC decisions;
     * native teardown/order is separately source-bound, not faked here. */
    assert(cleanup_return_code(normal_exit,recovery_clean)==-1);
    pt_master_release(&master_memory,r->editor);r->editor=NULL;
    assert(!master_memory.used && !master_memory.live);
    assert(master_memory.allocations==master_memory.releases);
    assert(cleanup_return_code(normal_exit,recovery_clean)==expected);
    free(r->scan);free(r->preview);free(r);
}
static void startup_empty(void)
{
    struct pt_document document;
    struct pt_allocator allocator={&master_memory,pt_master_allocate,pt_master_release};
    memset(&master_memory,0,sizeof(master_memory));pt_document_init(&document,&allocator);
    assert(cleanup_editor_owners(NULL,&document));
    assert(cleanup_return_code(0,1)==20 && cleanup_return_code(1,0)==20);
    assert(cleanup_return_code(1,-1)==-1);
    assert(!master_memory.used && !master_memory.releases);
}
static void success_history(void)
{
    struct rig *r=create();size_t live;sample(r);
    assert(pt_pattern_title_apply(&r->document.project,&r->editor->history,"cleanup journal")==PT_EDIT_OK);
    assert(pt_sampler_attributes(&r->editor->sampler,&r->document.project,&r->editor->history,
        0,"kept24",43,-2)==PT_EDIT_OK);
    assert(pt_song_append(&r->editor->song,&r->document.project,&r->editor->history,0,0)==PT_EDIT_OK);
    assert(r->editor->song.current && r->editor->song.bytes);
    assert(pt_pattern_undo(&r->document.project,&r->editor->history,-1)==PT_EDIT_OK);
    assert(pt_pattern_undo(&r->document.project,&r->editor->history,1)==PT_EDIT_OK);
    assert(!memcmp(r->document.project.samples[0].pcm.data,pcm24,sizeof(pcm24)));
    assert(r->editor->history.count>=4 && r->editor->history.cursor==r->editor->history.count);
    assert(pt_document_new(&r->editor->sample_source,4,SIZE_MAX)==PT_PROJECT_OK);
    live=master_memory.live;assert(live>4);
    finish(r,1,cleanup_recovery_result(&r->recovery,1),0);
}
static int veto(void *context)
{unsigned *calls=context;++*calls;return 0;}
static void editor_veto(void)
{
    struct rig *r=create();static unsigned calls;size_t used=master_memory.used;
    assert(pt_editor_change_barrier(r->editor,veto,&calls));
    assert(!cleanup_editor_owners(r->editor,&r->document));
    assert(calls==1 && r->document.loaded && r->editor->project==&r->document.project);
    assert(master_memory.used==used && !master_memory.releases);
    retained=r;
}
static void sampler_pin(void)
{
    struct rig *r=create();sample(r);
    assert(pt_sampler_pin(&r->editor->sampler,&r->document.project,0,
        r->editor->sampler.generation,&r->pinned,&r->pin)==PT_EDIT_OK);
    assert(pt_sampler_lifetime_idle(&r->editor->sampler));
    assert(!cleanup_editor_owners(r->editor,&r->document));
    assert(r->pin && r->document.loaded && r->pinned.bits==24);
    assert(!memcmp(r->pinned.data,pcm24,sizeof(pcm24)));
    assert(!pt_sampler_lifetime_storage_retired(&r->editor->sampler));
    retained=r;
}
static int progress(void *context,uint32_t done,uint32_t total)
{(void)context;(void)done;(void)total;return 1;}
static void progress_retained(void)
{
    struct rig *r=create();size_t used=master_memory.used;
    r->editor->sampler.progress=progress;r->editor->sampler.progress_context=r;
    assert(!cleanup_editor_owners(r->editor,&r->document));
    assert(master_memory.used==used && !master_memory.releases && r->document.loaded);
    retained=r;
}
static void workflow_cancel(void)
{
    struct rig *r=create();struct pt_sample_usage_options options={0};unsigned ready=0,n=0;
    sample(r);r->scan=calloc(1,sizeof(*r->scan));r->preview=calloc(1,sizeof(*r->preview));
    assert(r->scan && r->preview);
    options.revision=r->editor->history.revision;options.generation=r->editor->sampler.generation;
    assert(pt_sample_usage_begin(r->scan,&r->document.project,&options)==PT_USAGE_OK);
    do {
        assert(pt_sample_usage_step(r->scan,&r->document.project,options.revision,
            options.generation,r->preview,&ready)==PT_USAGE_OK);assert(++n<=8);
    }while(!ready);
    assert(pt_sampler_copy_begin(&r->editor->sampler,&r->document.project,&r->editor->history,
        r->preview,0,0,4,1,&r->editor->workflow.transaction)==PT_EDIT_OK);
    assert(r->editor->workflow.transaction && !pt_sampler_lifetime_idle(&r->editor->sampler));
    assert(!cleanup_editor_owners(r->editor,&r->document));
    /* Ordinary genuine cancellation, not forced cleanup or predicate reset. */
    pt_editor_workflow_cancel(r->editor);
    assert(!r->editor->workflow.transaction && pt_sampler_lifetime_idle(&r->editor->sampler));
    finish(r,1,cleanup_recovery_result(&r->recovery,1),0);
}
static void recovery_admission(void)
{
    struct rig *r=create();struct pt_native_recovery before;
    r->recovery.store.busy=1;before=r->recovery;
    assert(cleanup_recovery_result(&r->recovery,1)==-1 && !memcmp(&before,&r->recovery,sizeof(before)));
    r->recovery.store.busy=0;r->recovery.schedule.busy=1;before=r->recovery;
    assert(cleanup_recovery_result(&r->recovery,1)==-1 && !memcmp(&before,&r->recovery,sizeof(before)));
    r->recovery.schedule.busy=0;r->recovery.project=&r->document.project;before=r->recovery;
    assert(cleanup_recovery_result(&r->recovery,1)==-1 && !memcmp(&before,&r->recovery,sizeof(before)));
    r->recovery.project=NULL;finish(r,0,cleanup_recovery_result(&r->recovery,0),20);
}
static void recovery_residue(const char *temporary)
{
    struct rig *r=create();char root[1024],directory[1100],blocker[1200];FILE *file;int clean;
    assert(temporary && strlen(temporary)<900);
    assert(snprintf(root,sizeof(root),"%s/cleanup-recovery-XXXXXX",temporary)>0 && mkdtemp(root));
    assert(snprintf(directory,sizeof(directory),"%s/snapshot",root)>0);
    assert(pt_recovery_store_create(&r->recovery.store,directory,1)==PT_SAVE_OK);
    assert(snprintf(blocker,sizeof(blocker),"%s/foreign",directory)>0);
    file=fopen(blocker,"wb");assert(file && fwrite("x",1,1,file)==1 && fclose(file)==0);
    clean=cleanup_recovery_result(&r->recovery,1);
    assert(clean==0 && r->recovery.store.opened && !r->recovery.store.busy);
    finish(r,1,clean,20);
    /* Test-temp hygiene after the observed RC20 decision, never a product
     * retry of finish/discard or an acceptance of recovery cleanup success. */
    assert(!unlink(blocker) && !rmdir(directory) && !rmdir(root));
}
static void ledger_mismatch(void)
{
    struct rig *r=create();r->extra=pt_master_allocate(&master_memory,17);assert(r->extra);
    assert(!cleanup_editor_owners(r->editor,&r->document));
    assert(!r->document.loaded && master_memory.used>sizeof(*r->editor)+sizeof(size_t));
    assert(cleanup_return_code(1,1)==-1 && r->editor && r->extra);
    retained=r;
}
static void quarantine(unsigned source)
{
    struct rig *r=create();size_t prior;
    if(source)assert(pt_document_new(&r->editor->sample_source,4,SIZE_MAX)==PT_PROJECT_OK);
    release_reentry_document=source?&r->editor->sample_source:&r->document;
    prior=master_memory.releases;release_reentry_once=1;
    assert(!cleanup_editor_owners(r->editor,&r->document));
    assert(release_reentry_observed==1 && master_memory.releases==prior+1);
    assert(pt_document_release_checked(release_reentry_document)==PT_DOCUMENT_RELEASE_UNCERTAIN);
    assert(master_memory.releases==prior+1); /* quarantine query performs no retry */
    if(source)assert(r->document.loaded);
    assert(r->editor && master_memory.used>sizeof(*r->editor)+sizeof(size_t));
    retained=r;
}
int main(int argc,char **argv)
{
    assert(argc==3);
    if(!strcmp(argv[1],"startup_empty"))startup_empty();
    else if(!strcmp(argv[1],"success_history"))success_history();
    else if(!strcmp(argv[1],"editor_veto"))editor_veto();
    else if(!strcmp(argv[1],"sampler_pin"))sampler_pin();
    else if(!strcmp(argv[1],"progress_retained"))progress_retained();
    else if(!strcmp(argv[1],"workflow_cancel"))workflow_cancel();
    else if(!strcmp(argv[1],"recovery_admission"))recovery_admission();
    else if(!strcmp(argv[1],"recovery_residue"))recovery_residue(argv[2]);
    else if(!strcmp(argv[1],"ledger_mismatch"))ledger_mismatch();
    else if(!strcmp(argv[1],"source_quarantine"))quarantine(1);
    else if(!strcmp(argv[1],"root_quarantine"))quarantine(0);
    else assert(!"unknown finite case");
    if(retained)assert(retained->editor && master_memory.used && master_memory.live);
    else assert(!master_memory.used && !master_memory.live);
    printf("MAIN CLEANUP HOST %s PASS %s\n",argv[1],retained?"RETAINED_DECISION":"COMPLETE_DECISION");
    return 0;
}
