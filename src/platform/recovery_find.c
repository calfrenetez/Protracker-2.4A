#include "recovery_find.h"
#include <stdio.h>
#include <string.h>
static int same_info(const struct pt_recovery_info *a,const struct pt_recovery_info *b)
{
    return a->document_id==b->document_id && a->revision==b->revision &&
        a->saved_revision==b->saved_revision && a->timestamp==b->timestamp &&
        !strcmp(a->source,b->source);
}
enum pt_recovery_find_result pt_recovery_find(const char *directory,uint64_t id,
    const char *source,uint64_t timestamp,size_t limit,size_t budget,
    const struct pt_allocator *a,struct pt_recovery_candidate *out)
{
    struct pt_document d;struct pt_recovery_candidate best={0};
    struct pt_recovery_info info;char path[PT_RECOVERY_DIRECTORY_MAX+16];
    unsigned slot;enum pt_project_result result;
    if(!directory || !*directory || strlen(directory)>PT_RECOVERY_DIRECTORY_MAX || !id ||
       !source || strlen(source)>PT_RECOVERY_SOURCE_MAX || !a || !a->allocate || !a->release || !out)
        return PT_RECOVERY_FIND_ERROR;
    pt_document_init(&d,a);
    for(slot=0;slot<2;++slot) {
        snprintf(path,sizeof(path),"%s/%u.ptg",directory,slot);
        result=pt_project_file_load(&d,path,limit,budget);
        if(result==PT_PROJECT_CAPACITY) {pt_document_release(&d);return PT_RECOVERY_FIND_ERROR;}
        if(result==PT_PROJECT_OK && pt_recovery_project_info(&d.project,&info) &&
           info.document_id==id && !strcmp(info.source,source) && info.timestamp>timestamp &&
           info.timestamp>best.info.timestamp) {
            strcpy(best.path,path);best.info=info;
        }
        pt_document_release(&d);
    }
    if(!best.info.timestamp)return PT_RECOVERY_NONE;
    *out=best;return PT_RECOVERY_FOUND;
}
enum pt_project_result pt_recovery_restore_selected(struct pt_document *d,
    const struct pt_recovery_candidate *c,size_t limit,size_t budget)
{
    struct pt_document staged;struct pt_recovery_info info;enum pt_project_result result;
    if(!d || !c || !memchr(c->path,0,sizeof(c->path)) || !memchr(c->info.source,0,sizeof(c->info.source)))
        return PT_PROJECT_INVALID;
    pt_document_init(&staged,&d->allocator);
    result=pt_recovery_file_load(&staged,c->path,c->info.document_id,limit,budget,&info);
    if(result==PT_PROJECT_OK && !same_info(&c->info,&info))result=PT_PROJECT_INVALID;
    if(result==PT_PROJECT_OK) {pt_document_release(d);*d=staged;return result;}
    pt_document_release(&staged);return result;
}
