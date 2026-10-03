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
static int configuration_text(const char *text,size_t capacity,size_t *length)
{
    size_t n;
    for(n=0;n<capacity;++n) {
        unsigned char c=(unsigned char)text[n];
        if(!c) {*length=n;return 1;}
        if(c<32 || c==127)return 0;
    }
    return 0;
}
static int configuration_disjoint(const struct pt_native_recovery *r,const void *p,size_t bytes)
{
    uintptr_t a=(uintptr_t)r,b=(uintptr_t)p;
    if(!r || !p || sizeof(*r)>UINTPTR_MAX-a || bytes>UINTPTR_MAX-b)return 0;
    return a+sizeof(*r)<=b || b+bytes<=a;
}
static int configuration_valid(const struct pt_native_recovery_configuration *c,size_t *length)
{
    if(c->enabled>1 || c->allow_removable>1 || c->media>PT_NATIVE_RECOVERY_MEDIA_REMOVABLE ||
       c->interval_seconds<30 || c->interval_seconds>86400 ||
       !configuration_text(c->directory,sizeof(c->directory),length))return 0;
    if((!*length)!=(c->media==PT_NATIVE_RECOVERY_MEDIA_UNKNOWN))return 0;
    if(c->enabled && (!*length || (c->media==PT_NATIVE_RECOVERY_MEDIA_REMOVABLE && !c->allow_removable)))return 0;
    return 1;
}
int pt_native_recovery_get_configuration(const struct pt_native_recovery *r,
    struct pt_native_recovery_configuration *out)
{
    struct pt_native_recovery_configuration value={0};size_t n;
    if(!configuration_disjoint(r,out,sizeof(*out)) || r->schedule.busy || r->store.busy ||
       r->configured>1 || r->removable>1)return 0;
    value.interval_seconds=300;
    if(r->configured) {
        if(!configuration_text(r->root,sizeof(r->root),&n))return 0;
        memcpy(value.directory,r->root,n+1);value.interval_seconds=r->schedule.policy.interval_seconds;
        value.enabled=r->schedule.policy.enabled;value.allow_removable=r->schedule.policy.allow_removable;
        value.media=n?(r->removable?PT_NATIVE_RECOVERY_MEDIA_REMOVABLE:PT_NATIVE_RECOVERY_MEDIA_FIXED):PT_NATIVE_RECOVERY_MEDIA_UNKNOWN;
        if(!configuration_valid(&value,&n))return 0;
    }
    *out=value;return 1;
}
static int configuration_same(const struct pt_native_recovery *r,
    const struct pt_native_recovery_configuration *c,size_t length)
{
    struct pt_native_recovery_configuration current;size_t n;
    if(!r->configured || !pt_native_recovery_get_configuration(r,&current) ||
       !configuration_text(current.directory,sizeof(current.directory),&n))return 0;
    return n==length && !memcmp(current.directory,c->directory,n+1) &&
        current.interval_seconds==c->interval_seconds && current.enabled==c->enabled &&
        current.media==c->media && current.allow_removable==c->allow_removable;
}
int pt_native_recovery_apply_configuration(struct pt_native_recovery *r,
    const struct pt_native_recovery_configuration *configuration)
{
    struct pt_native_recovery_configuration value;
    struct pt_native_recovery candidate;struct pt_recovery_policy policy={0};
    BPTR lock;struct FileInfoBlock info __attribute__((aligned(4)));size_t n;
    if(!configuration_disjoint(r,configuration,sizeof(*configuration)) ||
       r->schedule.busy || r->store.busy || r->store.opened || r->store.owned ||
       !r->allocator.allocate || !r->allocator.release)return 0;
    value=*configuration;if(!configuration_valid(&value,&n))return 0;
    if(configuration_same(r,&value,n))return 1;
    if(value.enabled) {
        lock=Lock((STRPTR)value.directory,ACCESS_READ);if(!lock)return 0;
        if(!Examine(lock,&info) || info.fib_DirEntryType<=0 ||
           !NameFromLock(lock,(STRPTR)value.directory,sizeof(value.directory))) {UnLock(lock);return 0;}
        UnLock(lock);
        if(!configuration_text(value.directory,sizeof(value.directory),&n) || !n)return 0;
        if(configuration_same(r,&value,n))return 1;
    }
    candidate=*r;memset(candidate.root,0,sizeof(candidate.root));memcpy(candidate.root,value.directory,n+1);
    candidate.removable=(uint8_t)(value.media==PT_NATIVE_RECOVERY_MEDIA_REMOVABLE);candidate.configured=1;
    policy.interval_seconds=value.interval_seconds;policy.enabled=value.enabled;policy.allow_removable=value.allow_removable;
    if(!pt_recovery_configure(&candidate.schedule,&policy))return 0;
    *r=candidate;return 1;
}
int pt_native_recovery_configure(struct pt_native_recovery *r,const struct pt_allocator *a)
{
    char text[40],*end;int state;unsigned long seconds=300;struct pt_allocator allocator;
    struct pt_native_recovery_configuration configuration={0};
    struct pt_recovery_policy policy={300,1,0};
    if(!r || r->store.opened || r->store.owned || r->store.busy || r->schedule.busy ||
       !a || !a->allocate || !a->release)return 0;
    allocator=*a;memset(r,0,sizeof(*r));r->allocator=allocator;
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
    memcpy(configuration.directory,r->root,sizeof(configuration.directory));configuration.interval_seconds=(uint32_t)seconds;
    configuration.enabled=1;configuration.media=r->removable?PT_NATIVE_RECOVERY_MEDIA_REMOVABLE:PT_NATIVE_RECOVERY_MEDIA_FIXED;
    configuration.allow_removable=policy.allow_removable;
    return pt_native_recovery_apply_configuration(r,&configuration);
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
    if(!r || !r->configured || !r->bound || !r->root[0] || !out)return PT_RECOVERY_NONE;
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
