#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/platform/project_import.h"
static size_t live,calls,fail;
static void *allocate(void *ctx,size_t n) {(void)ctx;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *ctx,void *p) {(void)ctx;if(p) {assert(live);--live;free(p);}}
struct reader {uint8_t *data;size_t length,calls,fail;int finish_fail;};
static int read_at(void *ctx,size_t pos,uint8_t *out,size_t n)
{struct reader *r=ctx;assert(n<=1092 && pos<=r->length && n<=r->length-pos);if(++r->calls==r->fail)return 0;memcpy(out,r->data+pos,n);return 1;}
static int finish(void *ctx) {return !((struct reader *)ctx)->finish_fail;}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_project old;
    struct reader r={0};size_t w,i,owned,allocation_count,read_count;FILE *f;long n;uint8_t *output;
    assert(argc==2);f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));n=ftell(f);assert(n>0);rewind(f);
    r.length=(size_t)n;r.data=malloc(r.length);output=malloc(r.length);assert(r.data && output);
    assert(fread(r.data,1,r.length,f)==r.length && !fclose(f));
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    calls=0;assert(pt_document_load_project_reader(&d,read_at,&r,r.length,SIZE_MAX,finish)==PT_PROJECT_OK);
    allocation_count=calls;read_count=r.calls;
    assert(pt_project_encode(&d.project,output,r.length,&w)==PT_PROJECT_OK && w==r.length && !memcmp(r.data,output,w));
    pt_document_release(&d);assert(!live);
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);old=d.project;owned=live;
    for(i=1;i<=read_count;++i) {r.calls=0;r.fail=i;assert(pt_document_load_project_reader(&d,read_at,&r,r.length,SIZE_MAX,finish)!=PT_PROJECT_OK);assert(live==owned && !memcmp(&old,&d.project,sizeof(old)));}
    r.fail=0;r.finish_fail=1;assert(pt_document_load_project_reader(&d,read_at,&r,r.length,SIZE_MAX,finish)==PT_PROJECT_INVALID && live==owned && !memcmp(&old,&d.project,sizeof(old)));r.finish_fail=0;
    for(i=1;i<=allocation_count;++i) {fail=calls+i;assert(pt_project_file_load(&d,argv[1],r.length,SIZE_MAX)==PT_PROJECT_CAPACITY);assert(live==owned && !memcmp(&old,&d.project,sizeof(old)));}
    fail=0;assert(pt_project_file_candidate(argv[1]));
    assert(pt_project_file_load(&d,argv[1],r.length-1,SIZE_MAX)==PT_PROJECT_CAPACITY);
    assert(pt_project_file_load(&d,argv[1],r.length,0)==PT_PROJECT_CAPACITY);
    assert(pt_project_file_load(&d,argv[1],r.length,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_project_encode(&d.project,output,r.length,&w)==PT_PROJECT_OK && w==r.length && !memcmp(r.data,output,w));
    pt_document_release(&d);assert(!live);free(r.data);free(output);
    puts("PROJECT IMPORT STREAM PASS: exact master roundtrip, every read/allocation/finish failure preserves old document, budgets and file routing");return 0;
}
