/* Exercise production DOS traversal and snapshots without changing guest ENV or
 * its clock. Only configuration reads and time are private fixture inputs. */
#include <proto/dos.h>
#include <dos/var.h>
#include <time.h>
#include <sys/stat.h>
static const char *recovery_fixture_root,*recovery_fixture_media="fixed",*recovery_fixture_seconds="30",*recovery_fixture_removable="0";
static time_t recovery_fixture_now;
static LONG recovery_fixture_error;
static LONG recovery_fixture_getvar(STRPTR name,STRPTR out,LONG capacity,ULONG flags)
{
    const char *value=NULL;size_t n,copied;
    assert(flags==(GVF_GLOBAL_ONLY|GVF_BINARY_VAR));
    if(recovery_fixture_error && !strcmp((const char *)name,"PT24G_RECOVERY_SECONDS")) {SetIoErr(recovery_fixture_error);return -1;}
    if(!strcmp((const char *)name,"PT24G_RECOVERY_DIR"))value=recovery_fixture_root;
    if(!strcmp((const char *)name,"PT24G_RECOVERY_MEDIA"))value=recovery_fixture_media;
    if(!strcmp((const char *)name,"PT24G_RECOVERY_SECONDS"))value=recovery_fixture_seconds;
    if(!strcmp((const char *)name,"PT24G_RECOVERY_REMOVABLE"))value=recovery_fixture_removable;
    if(!value) {SetIoErr(ERROR_OBJECT_NOT_FOUND);return -1;}
    if(capacity<=0) {SetIoErr(ERROR_BAD_NUMBER);return -1;}
    n=strlen(value);copied=n<(size_t)capacity?n:(size_t)capacity-1;
    memcpy(out,value,copied);out[copied]=0;SetIoErr((LONG)n);return (LONG)copied;
}
static time_t recovery_fixture_time(time_t *out)
{if(out)*out=recovery_fixture_now;return recovery_fixture_now;}
#undef GetVar
#define GetVar recovery_fixture_getvar
#define time recovery_fixture_time
#include "../src/native/recovery.c"
#undef time
#undef GetVar
static void native_recovery_cases(const char *directory,const char *source,
    const struct pt_project *project,const struct pt_allocator *allocator)
{
    struct pt_native_recovery r={0};struct pt_recovery_candidate candidate;
    struct pt_document restored;struct stat st;size_t owned=live,n;uint8_t *bytes;
    recovery_fixture_root=directory;
    recovery_fixture_media="unknown";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_media="removable";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_media="fixed";
    recovery_fixture_error=ERROR_READ_PROTECTED;assert(!pt_native_recovery_configure(&r,allocator));recovery_fixture_error=0;
    recovery_fixture_seconds=NULL;assert(pt_native_recovery_configure(&r,allocator) && r.schedule.policy.interval_seconds==300);
    recovery_fixture_seconds="";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_seconds="30\ninvalid";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_seconds="000000000000000000000000000000000000030invalid";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_seconds="+30";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_seconds=" 30";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_seconds="86401";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_seconds="86400";assert(pt_native_recovery_configure(&r,allocator) && r.schedule.policy.interval_seconds==86400);
    recovery_fixture_removable="yes";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_removable=NULL;assert(pt_native_recovery_configure(&r,allocator));
    recovery_fixture_media="removable";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_removable="1";assert(pt_native_recovery_configure(&r,allocator));
    recovery_fixture_media="fixed";recovery_fixture_removable="0";
    recovery_fixture_seconds="0";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_seconds="30x";assert(!pt_native_recovery_configure(&r,allocator));
    recovery_fixture_seconds="30";assert(pt_native_recovery_configure(&r,allocator));
    assert(pt_native_recovery_bind(&r,source) && !r.store.opened);
    assert(pt_native_recovery_find(&r,&candidate)==PT_RECOVERY_NONE);
    assert(!stat(source,&st));recovery_fixture_now=st.st_mtime+10;
    assert(pt_native_recovery_poll(&r,project,1,0,1)==PT_RECOVERY_SKIPPED && !r.store.opened);
    recovery_fixture_now+=31;
    assert(pt_native_recovery_poll(&r,project,1,0,0)==PT_RECOVERY_SKIPPED && !r.store.opened);
    assert(pt_native_recovery_poll(&r,project,1,0,1)==PT_RECOVERY_SAVED && r.store.opened);
    assert(!pt_native_recovery_configure(&r,allocator)); /* Never lose a live owner. */
    assert(pt_native_recovery_find(&r,&candidate)==PT_RECOVERY_FOUND);
    assert(candidate.info.timestamp==(uint64_t)recovery_fixture_now);
    pt_document_init(&restored,allocator);
    assert(pt_recovery_restore_selected(&restored,&candidate,SIZE_MAX,SIZE_MAX)==PT_PROJECT_OK && restored.dirty);
    bytes=encoded(project,&n);exact(&restored.project,bytes,n);free(bytes);pt_document_release(&restored);
    assert(pt_native_recovery_poll(&r,project,0,0,1)==PT_RECOVERY_SKIPPED && !r.store.opened);
    assert(pt_native_recovery_find(&r,&candidate)==PT_RECOVERY_NONE);
    /* A new dirty state equal to a discarded snapshot must actually write again. */
    assert(pt_native_recovery_poll(&r,project,1,0,1)==PT_RECOVERY_SKIPPED);
    recovery_fixture_now+=30;
    assert(pt_native_recovery_poll(&r,project,1,0,1)==PT_RECOVERY_SAVED);
    assert(pt_native_recovery_finish(&r,0) && r.store.opened); /* Keep on abnormal exit. */
    assert(pt_recovery_store_discard(&r.store)); /* Explicit fixture-owned cleanup. */
    assert(live==owned);
    puts("NATIVE RECOVERY PASS: explicit configuration, canonical identity, deferred writes, exact snapshot discovery, clean-state discard and subsequent undo snapshot");
}

/* Seed a recoverable session without crashing or leaking the editor. The clock
 * override belongs only to this fixture; real guest time and ENV are untouched. */
static int native_recovery_seed(const char *source,const char *directory,const char *expected)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;
    struct pt_native_recovery r={0};struct stat st;
    pt_document_init(&d,&a);
    assert(pt_project_file_load(&d,source,SIZE_MAX,SIZE_MAX)==PT_PROJECT_OK);
    strcpy(d.project.title,"Recovered full precision");
    recovery_fixture_root=directory;
    assert(pt_native_recovery_configure(&r,&a) && pt_native_recovery_bind(&r,source));
    assert(!stat(source,&st));recovery_fixture_now=st.st_mtime+10;
    assert(pt_native_recovery_poll(&r,&d.project,1,0,1)==PT_RECOVERY_SKIPPED);
    recovery_fixture_now+=31;
    assert(pt_native_recovery_poll(&r,&d.project,1,0,1)==PT_RECOVERY_SAVED);
    assert(pt_project_file_save(expected,&d.project,&a)==PT_SAVE_OK);
    assert(pt_native_recovery_finish(&r,0));
    pt_document_release(&d);assert(!live);
    puts("NATIVE RECOVERY SEED PASS: snapshot retained, exact expected project written, source preserved");
    return 0;
}
