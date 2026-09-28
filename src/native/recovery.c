#include "recovery.h"
#include <proto/dos.h>
#include <dos/var.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
/* DOS uses BCPL-compatible data: FileInfoBlock requires longword alignment. */
/* 1: complete nonempty text, 0: absent, -1: unreadable/invalid. Binary reads
 * expose line breaks instead of silently accepting only their first line.
 * Reject a full buffer conservatively: V36 returns the complete size, whereas
 * V37+ can report a successful truncated read. Leave a spare byte in both. */
static int variable(const char *name,char *value,size_t size)
{
    LONG n;size_t i;
    if(size<2)return -1;
    n=GetVar((STRPTR)name,(STRPTR)value,(LONG)size,GVF_GLOBAL_ONLY|GVF_BINARY_VAR);
    if(n<0)return IoErr()==ERROR_OBJECT_NOT_FOUND?0:-1;
    if(!n || n>=(LONG)size-1)return -1;
    for(i=0;i<(size_t)n;++i)if((unsigned char)value[i]<32 || (unsigned char)value[i]==127)return -1;
    value[n]=0;return 1;
}
static uint64_t source_id(const char *source)
{
    const unsigned char *p=(const unsigned char *)source;
    uint64_t id=UINT64_C(14695981039346656037);
    do {id^=*p;id*=UINT64_C(1099511628211);}while(*p++);
    return id?id:1;
}
int pt_native_recovery_configure(struct pt_native_recovery *r,const struct pt_allocator *a)
{
    char text[40],*end;int state;BPTR lock;struct FileInfoBlock info __attribute__((aligned(4)));unsigned long seconds=300;
    struct pt_recovery_policy policy={300,1,0};
    if(!r || r->store.opened || r->schedule.busy || !a || !a->allocate || !a->release)return 0;
    memset(r,0,sizeof(*r));r->allocator=*a;
    if(variable("PT24G_RECOVERY_DIR",r->root,sizeof(r->root))!=1 ||
       variable("PT24G_RECOVERY_MEDIA",text,sizeof(text))!=1)return 0;
    if(!strcmp(text,"removable"))r->removable=1;
    else if(strcmp(text,"fixed"))return 0;
    state=variable("PT24G_RECOVERY_REMOVABLE",text,sizeof(text));
    if(state<0 || (state && strcmp(text,"0") && strcmp(text,"1")))return 0;
    if(state && !strcmp(text,"1"))policy.allow_removable=1;
    if(r->removable && !policy.allow_removable)return 0;
    state=variable("PT24G_RECOVERY_SECONDS",text,sizeof(text));
    if(state<0)return 0;
    if(state) {
        for(end=text;*end;++end)if(*end<'0' || *end>'9')return 0;
        seconds=strtoul(text,&end,10);
        if(!*text || *end || seconds<30 || seconds>86400)return 0;
    }
    lock=Lock((STRPTR)r->root,ACCESS_READ);if(!lock)return 0;
    if(!Examine(lock,&info) || info.fib_DirEntryType<=0 || !NameFromLock(lock,(STRPTR)r->root,sizeof(r->root))) {
        UnLock(lock);return 0;
    }
    UnLock(lock);policy.interval_seconds=(uint32_t)seconds;
    r->configured=(uint8_t)pt_recovery_configure(&r->schedule,&policy);return r->configured;
}
int pt_native_recovery_bind(struct pt_native_recovery *r,const char *source)
{
    BPTR lock;struct stat st;struct pt_recovery_policy policy;
    if(!r || !r->configured)return 0;
    r->bound=0;
    if(r->store.opened && !pt_recovery_store_discard(&r->store)) {r->configured=0;return 0;}
    memset(&r->info,0,sizeof(r->info));r->source_timestamp=0;
    if(source && *source) {
        lock=Lock((STRPTR)source,ACCESS_READ);if(!lock)return 0;
        if(!NameFromLock(lock,(STRPTR)r->info.source,sizeof(r->info.source))) {UnLock(lock);return 0;}
        UnLock(lock);
        if(stat(r->info.source,&st) || st.st_mtime<0)return 0;
        r->source_timestamp=(uint64_t)st.st_mtime;
    }
    r->info.document_id=source_id(r->info.source);policy=r->schedule.policy;
    if(!pt_recovery_configure(&r->schedule,&policy))return 0;
    r->bound=1;return 1;
}
static void prefix(char *out,size_t size,uint64_t id)
{snprintf(out,size,"pg%08lx%08lx-",(unsigned long)(id>>32),(unsigned long)(uint32_t)id);}
static int snapshot(void *context)
{
    struct pt_native_recovery *r=context;unsigned attempt;char path[PT_RECOVERY_DIRECTORY_MAX+1],name[64],tag[24];
    enum pt_save_result result;
    if(!r->store.opened) {
        prefix(tag,sizeof(tag),r->info.document_id);
        for(attempt=0;attempt<32;++attempt) {
            snprintf(name,sizeof(name),"%s%08lx%02u",tag,(unsigned long)r->info.timestamp,attempt);
            strcpy(path,r->root);if(!AddPart((STRPTR)path,(STRPTR)name,sizeof(path)))return 0;
            result=pt_recovery_store_create(&r->store,path,r->info.document_id);
            if(result==PT_SAVE_OK)break;
            if(result!=PT_SAVE_BEGIN)return 0;
        }
        if(!r->store.opened)return 0;
    }
    return pt_recovery_store_save(&r->store,r->project,&r->info,&r->allocator)==PT_SAVE_OK;
}
enum pt_recovery_tick_result pt_native_recovery_poll(struct pt_native_recovery *r,
    const struct pt_project *p,uint32_t revision,uint32_t saved,int safe)
{
    time_t now;enum pt_recovery_tick_result result;
    if(!r || !r->configured || !r->bound)return PT_RECOVERY_SKIPPED;
    if(revision==saved && r->store.opened && safe) {
        struct pt_recovery_policy policy=r->schedule.policy;
        if(!pt_recovery_store_discard(&r->store)) {r->configured=0;return PT_RECOVERY_FAILED;}
        pt_recovery_configure(&r->schedule,&policy);
    }
    now=time(NULL);if(now<=0)return PT_RECOVERY_SKIPPED;
    r->info.revision=revision;r->info.saved_revision=saved;r->info.timestamp=(uint64_t)now;r->project=p;
    result=pt_recovery_tick(&r->schedule,(uint64_t)now,revision,saved,1,r->removable,safe,snapshot,r);
    r->project=NULL;return result;
}
enum pt_recovery_find_result pt_native_recovery_find(struct pt_native_recovery *r,struct pt_recovery_candidate *out)
{
    BPTR lock;struct FileInfoBlock info __attribute__((aligned(4)));char tag[24],path[PT_RECOVERY_DIRECTORY_MAX+1];unsigned count=0;
    struct pt_recovery_candidate best={0},candidate;enum pt_recovery_find_result result=PT_RECOVERY_NONE;
    if(!r || !r->configured || !r->bound || !out)return PT_RECOVERY_NONE;
    lock=Lock((STRPTR)r->root,ACCESS_READ);if(!lock)return PT_RECOVERY_FIND_ERROR;
    if(!Examine(lock,&info)) {UnLock(lock);return PT_RECOVERY_FIND_ERROR;}
    prefix(tag,sizeof(tag),r->info.document_id);
    while(ExNext(lock,&info)) {
        if(++count>128) {result=PT_RECOVERY_FIND_ERROR;break;}
        if(info.fib_DirEntryType<=0 || strncmp((const char *)info.fib_FileName,tag,strlen(tag)))continue;
        strcpy(path,r->root);
        if(!AddPart((STRPTR)path,info.fib_FileName,sizeof(path))) {result=PT_RECOVERY_FIND_ERROR;break;}
        result=pt_recovery_find(path,r->info.document_id,r->info.source,r->source_timestamp,
            64UL*1024*1024,SIZE_MAX,&r->allocator,&candidate);
        if(result==PT_RECOVERY_FIND_ERROR)break;
        if(result==PT_RECOVERY_FOUND && candidate.info.timestamp>best.info.timestamp)best=candidate;
    }
    if(result!=PT_RECOVERY_FIND_ERROR && IoErr()!=ERROR_NO_MORE_ENTRIES)result=PT_RECOVERY_FIND_ERROR;
    UnLock(lock);
    if(result==PT_RECOVERY_FIND_ERROR)return result;
    if(!best.info.timestamp)return PT_RECOVERY_NONE;
    *out=best;return PT_RECOVERY_FOUND;
}
int pt_native_recovery_finish(struct pt_native_recovery *r,int discard)
{
    if(!r)return 0;
    if(discard && r->store.opened && !pt_recovery_store_discard(&r->store))return 0;
    r->bound=0;r->configured=0;return 1;
}
