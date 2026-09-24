#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/platform/mod_import.h"
#include "mod_project.h"
static size_t live,calls,fail;
static void *allocate(void *ctx,size_t n) {(void)ctx;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *ctx,void *p) {(void)ctx;if(p) {assert(live);--live;free(p);}}
static uint8_t input[1084+2048+4096],output[sizeof(input)];
struct reader {unsigned calls,fail,finish_fail;};
static int read_at(void *ctx,size_t pos,uint8_t *out,size_t n)
{struct reader *r=ctx;assert(n<=1084 && pos<=sizeof(input) && n<=sizeof(input)-pos);if(++r->calls==r->fail)return 0;memcpy(out,input+pos,n);return 1;}
static int finish(void *ctx) {struct reader *r=ctx;return !r->finish_fail;}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_project old;
    struct reader r={0};size_t w,i,owned,allocation_count,read_count;FILE *f;
    assert(argc==2);input[950]=1;input[951]=127;input[1079]=1;memcpy(input+1080,"M.K.",4);
    input[42]=8;input[45]=64;input[48]=0;input[49]=4;
    for(i=1;i<31;++i)input[20+i*30+29]=1;
    input[1084]=1;input[1085]=172;input[1086]=16;
    for(i=3132;i<sizeof(input);++i)input[i]=(uint8_t)(i*37);
    f=fopen(argv[1],"wb");assert(f && fwrite(input,1,sizeof(input),f)==sizeof(input) && !fclose(f));
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);old=d.project;owned=live;
    calls=0;assert(pt_document_load_mod_reader(&d,read_at,&r,sizeof(input),SIZE_MAX,finish)==PT_PROJECT_OK);
    allocation_count=calls;read_count=r.calls;
    assert(pt_mod_export_direct(&d.project,output,sizeof(output),&w)==PT_PROJECT_OK && w==sizeof(input) && !memcmp(input,output,w));
    pt_document_release(&d);assert(!live);
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);old=d.project;owned=live;
    for(i=1;i<=read_count;++i) {r=(struct reader){0,(unsigned)i,0};assert(pt_document_load_mod_reader(&d,read_at,&r,sizeof(input),SIZE_MAX,finish)!=PT_PROJECT_OK);assert(live==owned && !memcmp(&old,&d.project,sizeof(old)));}
    r=(struct reader){0,0,1};assert(pt_document_load_mod_reader(&d,read_at,&r,sizeof(input),SIZE_MAX,finish)==PT_PROJECT_INVALID && live==owned && !memcmp(&old,&d.project,sizeof(old)));
    for(i=1;i<=allocation_count;++i) {fail=calls+i;assert(pt_mod_file_load(&d,argv[1],sizeof(input),SIZE_MAX)==PT_PROJECT_CAPACITY);assert(live==owned && !memcmp(&old,&d.project,sizeof(old)));}
    fail=0;assert(pt_mod_file_candidate(argv[1]));
    assert(pt_mod_file_load(&d,argv[1],sizeof(input)-1,SIZE_MAX)==PT_PROJECT_CAPACITY);
    assert(pt_mod_file_load(&d,argv[1],sizeof(input),0)==PT_PROJECT_CAPACITY);
    assert(pt_mod_file_load(&d,argv[1],sizeof(input),SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_mod_export_direct(&d.project,output,sizeof(output),&w)==PT_PROJECT_OK && w==sizeof(input) && !memcmp(input,output,w));
    pt_document_release(&d);assert(!live);
    memcpy(input,"PT24G\r\n\032",8);f=fopen(argv[1],"wb");assert(f && fwrite(input,1,sizeof(input),f)==sizeof(input) && !fclose(f));assert(!pt_mod_file_candidate(argv[1]));
    assert(!unlink(argv[1]));puts("MOD IMPORT STREAM PASS: exact roundtrip, inactive orders, loops, every read/allocation/finish failure preserves old document, budgets and format routing");return 0;
}
