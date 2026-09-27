#include "../src/platform/recovery_find.h"
static void recovery_find_cases(const char *directory,const struct pt_project *project,
    struct pt_recovery_info info,const struct pt_allocator *allocator)
{
    char dir[512],path[2][512];struct pt_recovery_candidate found,sentinel;
    struct pt_document restored;struct pt_project previous;FILE *file;
    size_t owned=live,i,count;uint64_t first=info.timestamp;
    snprintf(dir,sizeof(dir),"%s/find",directory);assert(test_mkdir(dir)==0);
    snprintf(path[0],sizeof(path[0]),"%s/find/0.ptg",directory);
    snprintf(path[1],sizeof(path[1]),"%s/find/1.ptg",directory);
    memset(&sentinel,0xa5,sizeof(sentinel));found=sentinel;
#define FIND(id,source,time,limit,budget) pt_recovery_find(dir,id,source,time,limit,budget,allocator,&found)
    assert(FIND(info.document_id,info.source,0,SIZE_MAX,SIZE_MAX)==PT_RECOVERY_NONE);
    assert(!memcmp(&found,&sentinel,sizeof(found)));
    assert(pt_recovery_file_save(path[0],project,&info,allocator)==PT_SAVE_OK);
    ++info.timestamp;info.revision=42;
    assert(pt_recovery_file_save(path[1],project,&info,allocator)==PT_SAVE_OK);
    calls=0;assert(FIND(info.document_id,info.source,0,SIZE_MAX,SIZE_MAX)==PT_RECOVERY_FOUND);count=calls;
    assert(found.info.timestamp==info.timestamp && !strcmp(found.path,path[1]) && live==owned);
    for(i=1;i<=count;++i) {
        calls=0;fail=i;found=sentinel;
        assert(FIND(info.document_id,info.source,0,SIZE_MAX,SIZE_MAX)==PT_RECOVERY_FIND_ERROR);
        assert(!memcmp(&found,&sentinel,sizeof(found)) && live==owned);
    }
    fail=0;found=sentinel;
    assert(FIND(info.document_id,info.source,0,8,SIZE_MAX)==PT_RECOVERY_FIND_ERROR);
    assert(FIND(info.document_id,info.source,0,SIZE_MAX,0)==PT_RECOVERY_FIND_ERROR);
    assert(FIND(info.document_id+1,info.source,0,SIZE_MAX,SIZE_MAX)==PT_RECOVERY_NONE);
    assert(FIND(info.document_id,"different source",0,SIZE_MAX,SIZE_MAX)==PT_RECOVERY_NONE);
    assert(FIND(info.document_id,info.source,info.timestamp,SIZE_MAX,SIZE_MAX)==PT_RECOVERY_NONE);
    assert(!memcmp(&found,&sentinel,sizeof(found)) && live==owned);
    assert(FIND(info.document_id,info.source,first,SIZE_MAX,SIZE_MAX)==PT_RECOVERY_FOUND);
    pt_document_init(&restored,allocator);assert(pt_document_new(&restored,1,SIZE_MAX)==PT_PROJECT_OK);
    previous=restored.project;owned=live;
    /* Selection does not authorize a later different snapshot at the same path. */
    assert(unlink(path[1])==0);++info.timestamp;
    assert(pt_recovery_file_save(path[1],project,&info,allocator)==PT_SAVE_OK);
    assert(pt_recovery_restore_selected(&restored,&found,SIZE_MAX,SIZE_MAX)==PT_PROJECT_INVALID);
    assert(!memcmp(&previous,&restored.project,sizeof(previous)) && live==owned);
    assert(FIND(info.document_id,info.source,0,SIZE_MAX,SIZE_MAX)==PT_RECOVERY_FOUND);
    assert(pt_recovery_restore_selected(&restored,&found,SIZE_MAX,SIZE_MAX)==PT_PROJECT_OK && restored.dirty);
    {size_t length;uint8_t *bytes=encoded(project,&length);exact(&restored.project,bytes,length);free(bytes);}
    pt_document_release(&restored);
    /* Corrupt newer candidate falls back to an independently valid older one. */
    file=fopen(path[1],"wb");assert(file && fputs("broken",file)>=0 && !fclose(file));
    assert(FIND(info.document_id,info.source,0,SIZE_MAX,SIZE_MAX)==PT_RECOVERY_FOUND);
    assert(found.info.timestamp==first && !strcmp(found.path,path[0]));
    assert(unlink(path[0])==0 && unlink(path[1])==0 && test_rmdir(dir)==0);
#undef FIND
}
