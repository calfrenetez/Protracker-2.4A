#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "safe_save.h"
struct allocation {unsigned calls,fail,live;};
static void *alloc(void *ctx,size_t n)
{struct allocation *a=ctx;void *p;if(++a->calls==a->fail)return NULL;p=malloc(n);if(p)++a->live;return p;}
static void release(void *ctx,void *p)
{struct allocation *a=ctx;if(!a->live)fprintf(stderr,"ALLOC UNDERFLOW calls=%u fail=%u pointer=%p context=%p\n",a->calls,a->fail,p,ctx);assert(a->live);--a->live;free(p);}
struct file {unsigned fail,aborts,finished,verified,published;size_t size;char previous[8],temporary[32];};
static int begin(void *ctx) {struct file *f=ctx;f->size=0;return f->fail!=1;}
static size_t write_part(void *ctx,const void *p,size_t n)
{struct file *f=ctx;if(f->fail==2 && f->size>=3)return 0;if(n>3)n=3;assert(f->size+n<=32);memcpy(f->temporary+f->size,p,n);f->size+=n;return n;}
static int finish(void *ctx) {struct file *f=ctx;f->finished=1;return f->fail!=3;}
static int verify(void *ctx,const void *data,size_t n)
{struct file *f=ctx;assert(f->finished && f->size==n && !memcmp(data,f->temporary,n));f->verified=1;return f->fail!=4;}
static int publish(void *ctx)
{struct file *f=ctx;assert(f->verified && f->finished);if(f->fail==5)return 0;memcpy(f->previous,f->temporary,8);f->published=1;return 1;}
static void abort_save(void *ctx) {struct file *f=ctx;assert(!f->published);++f->aborts;memset(f->temporary,0,32);}
int main(int argc,char **argv)
{
    struct allocation a={0};struct pt_allocator allocator={&a,alloc,release};
    struct pt_document doc,before;FILE *file;long n;uint8_t *data,*project;size_t length,w;unsigned i,live;
    assert(argc==2);file=fopen(argv[1],"rb");assert(file);assert(!fseek(file,0,SEEK_END));n=ftell(file);assert(n>0);rewind(file);
    data=malloc((size_t)n);assert(data);assert(fread(data,1,(size_t)n,file)==(size_t)n);assert(!fclose(file));
    pt_document_init(&doc,&allocator);assert(pt_document_load(&doc,data,(size_t)n,SIZE_MAX)==PT_PROJECT_OK);
    assert(a.live==6);doc.dirty=1;before=doc;live=a.live;printf("ALLOC initial calls=%u live=%u bytes=%lu\n",a.calls,a.live,(unsigned long)doc.allocated_bytes);
    assert(pt_project_size(&doc.project,&length)==PT_PROJECT_OK);project=malloc(length);assert(project);
    assert(pt_project_encode(&doc.project,project,length,&w)==PT_PROJECT_OK);
    /* Six nonempty allocations in this fixture; fail each in turn. */
    for(i=1;i<=6;++i) {
        printf("ALLOC failure-case=%u live=%u\n",i,a.live);a.calls=0;a.fail=i;
        assert(pt_document_load(&doc,project,length,SIZE_MAX)==PT_PROJECT_CAPACITY);
        assert(!memcmp(&doc,&before,sizeof(doc)) && a.live==live);
    }
    printf("ALLOC budget live=%u\n",a.live);a.fail=0;a.calls=0;
    assert(pt_document_load(&doc,project,length,doc.allocated_bytes-1)==PT_PROJECT_CAPACITY);
    assert(!a.calls && !memcmp(&doc,&before,sizeof(doc)));
    project[length-1]^=1;
    assert(pt_document_load(&doc,project,length,SIZE_MAX)!=PT_PROJECT_OK && !a.calls && !memcmp(&doc,&before,sizeof(doc)));
    project[length-1]^=1;
    printf("ALLOC replacement live=%u\n",a.live);
    assert(pt_document_load(&doc,project,length,SIZE_MAX)==PT_PROJECT_OK && !doc.dirty && a.live==live);
    puts("ALLOC reload loop");
    for(i=0;i<200;++i)assert(pt_document_load(&doc,data,(size_t)n,SIZE_MAX)==PT_PROJECT_OK && a.live==live);
    printf("ALLOC final release live=%u\n",a.live);pt_document_release(&doc);assert(!a.live && !doc.loaded);pt_document_release(&doc);assert(!a.live);
    for(i=0;i<=5;++i) {
        struct file f;struct pt_save_ops ops={&f,begin,write_part,finish,verify,publish,abort_save};
        memset(&f,0,sizeof(f));memcpy(f.previous,"OLD GOOD",8);f.fail=i;
        if(i) {assert(pt_safe_save(&ops,"NEW GOOD",8)!=(enum pt_save_result)PT_SAVE_OK);assert(!memcmp(f.previous,"OLD GOOD",8) && f.aborts==1);}
        else {assert(pt_safe_save(&ops,"NEW GOOD",8)==PT_SAVE_OK);assert(!memcmp(f.previous,"NEW GOOD",8) && !f.aborts);}
    }
    free(data);free(project);
    puts("DOCUMENT/SAVE PASS: every staging allocation failure, memory budget, corrupt input, 200 reloads, partial writes and all save failure phases preserve prior state");
    return 0;
}
