#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/platform/recovery_file.h"
#include "recovery_schedule_cases.h"
static size_t live,calls,fail;
static void *allocate(void *ctx,size_t n)
{void *p;(void)ctx;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *ctx,void *p)
{(void)ctx;if(p){assert(live);--live;free(p);}}
static uint8_t *encoded(const struct pt_project *p,size_t *n)
{
    uint8_t *b;size_t wrote;
    assert(pt_project_size(p,n)==PT_PROJECT_OK);b=malloc(*n);assert(b);
    assert(pt_project_encode(p,b,*n,&wrote)==PT_PROJECT_OK && wrote==*n);return b;
}
static void exact(const struct pt_project *p,const uint8_t *original,size_t n)
{size_t size;uint8_t *b=encoded(p,&size);assert(size==n && !memcmp(b,original,n));free(b);}
#include "recovery_store_cases.h"
#include "recovery_find_cases.h"
static int recovery_fixture(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,recovered,raw;
    struct pt_recovery_info info={0},got,sentinel,bad;
    struct pt_project previous;uint8_t *original,*snapshot;size_t n,sn,owned,count,i;
    char path[512],malformed[512];
    recovery_schedule_cases();
    assert(argc==3);assert(strlen(argv[2])+20<sizeof(path));
    snprintf(path,sizeof(path),"%s/recovery.ptg",argv[2]);
    snprintf(malformed,sizeof(malformed),"%s/malformed.ptg",argv[2]);
    pt_document_init(&d,&a);pt_document_init(&recovered,&a);pt_document_init(&raw,&a);
    assert(pt_project_file_load(&d,argv[1],SIZE_MAX,SIZE_MAX)==PT_PROJECT_OK);
    original=encoded(&d.project,&n);owned=live;
    info.document_id=UINT64_C(0x123456789abcdef0);info.revision=17;info.saved_revision=9;
    info.timestamp=1790552000;strcpy(info.source,"Work:Songs/original.ptg");
    memset(&sentinel,0xa5,sizeof(sentinel));got=sentinel;
    assert(!pt_recovery_project_info(&d.project,&got) && !memcmp(&got,&sentinel,sizeof(got)));
    bad=info;bad.revision=bad.saved_revision;
    assert(pt_recovery_file_save(path,&d.project,&bad,&a)==PT_SAVE_INVALID);
    bad=info;bad.document_id=0;assert(pt_recovery_file_save(path,&d.project,&bad,&a)==PT_SAVE_INVALID);
    bad=info;memset(bad.source,'a',sizeof(bad.source));assert(pt_recovery_file_save(path,&d.project,&bad,&a)==PT_SAVE_INVALID);
    bad=info;bad.source[4]='\n';assert(pt_recovery_file_save(path,&d.project,&bad,&a)==PT_SAVE_INVALID);
    calls=0;assert(pt_recovery_file_save(path,&d.project,&info,&a)==PT_SAVE_OK);count=calls;
    assert(live==owned);exact(&d.project,original,n);
    assert(pt_recovery_file_save(argv[1],&d.project,&info,&a)!=PT_SAVE_OK); /* Original also protected. */
    assert(pt_project_file_load(&raw,path,SIZE_MAX,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_recovery_project_info(&raw.project,&got));
    assert(got.document_id==info.document_id && got.revision==17 && got.saved_revision==9 &&
        got.timestamp==info.timestamp && !strcmp(got.source,info.source));
    snapshot=encoded(&raw.project,&sn);pt_document_release(&raw);assert(live==owned);
    assert(pt_recovery_file_save(path,&d.project,&info,&a)!=PT_SAVE_OK); /* No replace. */
    assert(pt_project_file_load(&raw,path,SIZE_MAX,SIZE_MAX)==PT_PROJECT_OK);exact(&raw.project,snapshot,sn);
    assert(pt_recovery_file_save(malformed,&raw.project,&info,&a)==PT_SAVE_INVALID); /* No nesting. */
    pt_document_release(&raw);assert(unlink(path)==0);
    for(i=1;i<=count;++i) {
        calls=0;fail=i;assert(pt_recovery_file_save(path,&d.project,&info,&a)!=PT_SAVE_OK);
        assert(access(path,F_OK)!=0 && live==owned);exact(&d.project,original,n);
    }
    fail=0;assert(pt_recovery_file_save(path,&d.project,&info,&a)==PT_SAVE_OK);
    assert(pt_document_new(&recovered,1,SIZE_MAX)==PT_PROJECT_OK);previous=recovered.project;owned=live;
    got=sentinel;
    assert(pt_recovery_file_load(&recovered,path,info.document_id+1,SIZE_MAX,SIZE_MAX,&got)==PT_PROJECT_INVALID);
    assert(!memcmp(&previous,&recovered.project,sizeof(previous)) && !memcmp(&got,&sentinel,sizeof(got)) && live==owned);
    assert(pt_recovery_file_load(&recovered,path,info.document_id,sn-1,SIZE_MAX,&got)==PT_PROJECT_CAPACITY);
    assert(pt_recovery_file_load(&recovered,path,info.document_id,SIZE_MAX,0,&got)==PT_PROJECT_CAPACITY);
    assert(!memcmp(&previous,&recovered.project,sizeof(previous)) && live==owned);
    /* Discover every staging allocation; repeat refusals against an open song. */
    calls=0;assert(pt_recovery_file_load(&raw,path,info.document_id,SIZE_MAX,SIZE_MAX,&got)==PT_PROJECT_OK);count=calls;
    pt_document_release(&raw);
    for(i=1;i<=count;++i) {
        calls=0;fail=i;got=sentinel;
        assert(pt_recovery_file_load(&recovered,path,info.document_id,SIZE_MAX,SIZE_MAX,&got)!=PT_PROJECT_OK);
        assert(!memcmp(&previous,&recovered.project,sizeof(previous)) && !memcmp(&got,&sentinel,sizeof(got)) && live==owned);
    }
    fail=0;assert(pt_project_file_load(&raw,path,SIZE_MAX,SIZE_MAX)==PT_PROJECT_OK);
    /* The outer project can be valid while recovery metadata is invalid. */
    {
        struct pt_extension *e=NULL;unsigned j;uint8_t saved_byte;
        for(j=0;j<raw.project.extension_count;++j)if(raw.project.extensions[j].id==PT_RECOVERY_EXTENSION)e=raw.project.extensions+j;
        assert(e);got=sentinel;
        e->version=2;assert(!pt_recovery_project_info(&raw.project,&got));e->version=1;
        assert(!memcmp(&got,&sentinel,sizeof(got)));
        saved_byte=e->data[0];((uint8_t *)e->data)[0]=0;
        assert(!pt_recovery_project_info(&raw.project,&got));((uint8_t *)e->data)[0]=saved_byte;
        saved_byte=e->data[44];((uint8_t *)e->data)[44]=0;
        assert(!pt_recovery_project_info(&raw.project,&got));((uint8_t *)e->data)[44]=saved_byte;
        {
            struct pt_project duplicate=raw.project;struct pt_extension copies[8];
            assert(duplicate.extension_count<8);
            memcpy(copies,duplicate.extensions,duplicate.extension_count*sizeof(*copies));
            copies[duplicate.extension_count++]=*e;duplicate.extensions=copies;
            assert(!pt_recovery_project_info(&duplicate,&got));
        }
        assert(!memcmp(&got,&sentinel,sizeof(got)));
        ((uint8_t *)e->data)[42]=1;
        assert(pt_project_file_save(malformed,&raw.project,&a)==PT_SAVE_OK);
    }
    pt_document_release(&raw);got=sentinel;
    assert(pt_recovery_file_load(&recovered,malformed,info.document_id,SIZE_MAX,SIZE_MAX,&got)==PT_PROJECT_INVALID);
    assert(!memcmp(&previous,&recovered.project,sizeof(previous)) && !memcmp(&got,&sentinel,sizeof(got)) && live==owned);
    assert(unlink(malformed)==0);
    assert(pt_recovery_file_load(&recovered,path,info.document_id,SIZE_MAX,SIZE_MAX,&got)==PT_PROJECT_OK && recovered.dirty);
    exact(&recovered.project,original,n);exact(&d.project,original,n);
    assert(!pt_recovery_project_info(&recovered.project,&got));
    assert(unlink(path)==0);
    /* An undo to the initial state after saving is still recoverable work. */
    info.revision=0;
    assert(pt_recovery_file_save(path,&d.project,&info,&a)==PT_SAVE_OK);
    assert(pt_recovery_file_load(&recovered,path,info.document_id,SIZE_MAX,SIZE_MAX,&got)==PT_PROJECT_OK);
    assert(got.revision==0 && got.saved_revision==9 && recovered.dirty);
    exact(&recovered.project,original,n);assert(unlink(path)==0);
    recovery_store_cases(argv[2],&d.project,info,&a);
    recovery_find_cases(argv[2],&d.project,info,&a);
    pt_document_release(&d);pt_document_release(&recovered);
    free(original);free(snapshot);assert(!live);
    puts("RECOVERY FILE PASS: full-precision project identity, new-file-only snapshot, bounded allocation failures, corrupt/mismatched metadata preserves open song, explicit dirty restoration");
    return 0;
}
#ifndef PT_RECOVERY_NATIVE
int main(int argc,char **argv) {return recovery_fixture(argc,argv);}
#endif
