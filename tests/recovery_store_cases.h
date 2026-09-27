#include "../src/platform/recovery_store.h"
#ifdef __amigaos__
#include <proto/dos.h>
static int test_mkdir(const char *p) {BPTR l=CreateDir((STRPTR)p);if(!l)return -1;UnLock(l);return 0;}
static int test_rmdir(const char *p) {return DeleteFile((STRPTR)p)?0:-1;}
#else
#include <sys/stat.h>
static int test_mkdir(const char *p) {return mkdir(p,0700);}
static int test_rmdir(const char *p) {return rmdir(p);}
#endif
static void recovery_store_cases(const char *directory,const struct pt_project *project,
    struct pt_recovery_info info,const struct pt_allocator *allocator)
{
    struct pt_recovery_store store={0},other={0},before;
    struct pt_document check;struct pt_recovery_info got;
    char path[512],blocked[512],foreign[512],refusal[512];FILE *file;
    size_t count,i,owned=live;unsigned old;
    snprintf(path,sizeof(path),"%s/session",directory);
    snprintf(blocked,sizeof(blocked),"%s/held.ptg",directory);
    snprintf(foreign,sizeof(foreign),"%s/session/foreign",directory);
    pt_document_init(&check,allocator);
    assert(pt_recovery_store_create(&store,path,info.document_id)==PT_SAVE_OK);
    assert(pt_recovery_store_create(&other,path,info.document_id)==PT_SAVE_BEGIN && !other.opened);
    assert(pt_recovery_store_create(&store,path,info.document_id)==PT_SAVE_INVALID);
    calls=0;assert(pt_recovery_store_save(&store,project,&info,allocator)==PT_SAVE_OK);count=calls;
    assert(live==owned && store.owned==1);
    before=store;
    assert(pt_recovery_store_save(&store,project,&info,allocator)==PT_SAVE_INVALID); /* Same timestamp. */
    ++info.timestamp;++info.document_id;
    assert(pt_recovery_store_save(&store,project,&info,allocator)==PT_SAVE_INVALID);--info.document_id;
    assert(!memcmp(&before,&store,sizeof(store)));
    for(i=1;i<=count;++i) {
        calls=0;fail=i;
        assert(pt_recovery_store_save(&store,project,&info,allocator)!=PT_SAVE_OK);
        fail=0;assert(live==owned && !memcmp(&before,&store,sizeof(store)));
        assert(pt_recovery_file_load(&check,store.path[store.current],info.document_id,SIZE_MAX,SIZE_MAX,&got)==PT_PROJECT_OK);
        assert(got.timestamp==before.timestamp);pt_document_release(&check);
        assert(access(store.path[1-store.current],F_OK)!=0);
    }
    /* A foreign target is never removed just to allow publication. */
    file=fopen(store.path[1-store.current],"wb");assert(file);assert(fputs("foreign",file)>=0 && !fclose(file));
    assert(pt_recovery_store_save(&store,project,&info,allocator)!=PT_SAVE_OK);
    file=fopen(store.path[1-store.current],"rb");assert(file && fgetc(file)=='f' && !fclose(file));
    assert(unlink(store.path[1-store.current])==0);
    /* Simulate filesystem refusing to unlink the old entry after publication. */
    old=store.current;assert(rename(store.path[old],blocked)==0 && test_mkdir(store.path[old])==0);
    /* AmigaDOS DeleteFile/unlink can remove an empty directory; make the
     * injected directory nonempty so both host and native refuse removal. */
    snprintf(refusal,sizeof(refusal),"%s/guard",store.path[old]);
    file=fopen(refusal,"wb");assert(file && !fclose(file));
    assert(pt_recovery_store_save(&store,project,&info,allocator)==PT_SAVE_OK && store.owned==3);
    assert(store.current!=old);before=store;++info.timestamp;
    assert(pt_recovery_store_save(&store,project,&info,allocator)==PT_SAVE_BEGIN);
    assert(!memcmp(&before,&store,sizeof(store)));
    assert(pt_recovery_file_load(&check,store.path[store.current],info.document_id,SIZE_MAX,SIZE_MAX,&got)==PT_PROJECT_OK);
    assert(got.timestamp==before.timestamp);pt_document_release(&check);
    assert(unlink(refusal)==0 && test_rmdir(store.path[old])==0 && rename(blocked,store.path[old])==0);
    assert(pt_recovery_store_save(&store,project,&info,allocator)==PT_SAVE_OK);
    assert(store.owned==(1U<<store.current) && access(store.path[1-store.current],F_OK)!=0);
    file=fopen(foreign,"wb");assert(file && !fclose(file));
    assert(!pt_recovery_store_discard(&store) && store.opened && !store.owned);
    assert(access(foreign,F_OK)==0);assert(unlink(foreign)==0);
    assert(pt_recovery_store_discard(&store) && !store.opened && access(path,F_OK)!=0);
    assert(live==owned);
}
