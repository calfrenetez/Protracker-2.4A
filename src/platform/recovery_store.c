#include "recovery_store.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef __amigaos__
#include <proto/dos.h>
#else
#include <sys/stat.h>
#endif
static int new_directory(const char *path)
{
#ifdef __amigaos__
    BPTR lock=CreateDir((STRPTR)path);
    if(!lock)return 0;
    UnLock(lock);return 1;
#else
    return mkdir(path,0700)==0;
#endif
}
static int empty_directory(const char *path)
{
#ifdef __amigaos__
    return DeleteFile((STRPTR)path)!=0;
#else
    return rmdir(path)==0;
#endif
}
static int remove_owned(struct pt_recovery_store *s,unsigned slot)
{
    if(!(s->owned&(1U<<slot)))return 1;
    if(unlink(s->path[slot]) && errno!=ENOENT)return 0;
    s->owned&=(uint8_t)~(1U<<slot);return 1;
}
enum pt_save_result pt_recovery_store_create(struct pt_recovery_store *s,const char *path,uint64_t id)
{
    struct pt_recovery_store candidate={0};size_t n;
    if(!s || s->opened || s->busy || !path || !id)return PT_SAVE_INVALID;
    n=strlen(path);if(!n || n>PT_RECOVERY_DIRECTORY_MAX)return PT_SAVE_INVALID;
    memcpy(candidate.directory,path,n+1);
    snprintf(candidate.path[0],sizeof(candidate.path[0]),"%s/0.ptg",path);
    snprintf(candidate.path[1],sizeof(candidate.path[1]),"%s/1.ptg",path);
    if(!new_directory(path))return PT_SAVE_BEGIN;
    candidate.document_id=id;candidate.opened=1;*s=candidate;return PT_SAVE_OK;
}
enum pt_save_result pt_recovery_store_save(struct pt_recovery_store *s,
    const struct pt_project *p,const struct pt_recovery_info *m,const struct pt_allocator *a)
{
    unsigned target,old;enum pt_save_result result;
    if(!s || !s->opened || s->busy || !m || m->document_id!=s->document_id ||
       m->timestamp<=s->timestamp)return PT_SAVE_INVALID;
    old=s->current;target=s->owned?1-old:0;s->busy=1;
    if(!remove_owned(s,target)) {s->busy=0;return PT_SAVE_BEGIN;}
    result=pt_recovery_file_save(s->path[target],p,m,a);
    if(result==PT_SAVE_OK) {
        s->owned|=(uint8_t)(1U<<target);s->current=(uint8_t)target;s->timestamp=m->timestamp;
        if(target!=old)remove_owned(s,old);
    }
    s->busy=0;return result;
}
int pt_recovery_store_discard(struct pt_recovery_store *s)
{
    unsigned i;
    if(!s || !s->opened || s->busy)return 0;
    s->busy=1;
    for(i=0;i<2;++i)if(!remove_owned(s,i)) {s->busy=0;return 0;}
    if(!empty_directory(s->directory)) {s->busy=0;return 0;}
    memset(s,0,sizeof(*s));return 1;
}
