#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
static uint32_t state=0x24f;
static uint32_t next(void) {state^=state<<13;state^=state>>17;state^=state<<5;return state;}
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static void put32(uint8_t *p,uint32_t n) {p[0]=(uint8_t)(n>>24);p[1]=(uint8_t)(n>>16);p[2]=(uint8_t)(n>>8);p[3]=(uint8_t)n;}
static void repair(uint8_t *p,size_t n)
{
    size_t i;unsigned j;uint32_t crc=0xffffffffUL;memset(p+20,0,4);put32(p+12,(uint32_t)n);
    for(i=0;i<n;++i) {crc^=p[i];for(j=0;j<8;++j)crc=(crc>>1)^((crc&1)?0xedb88320UL:0);}
    put32(p+20,crc^0xffffffffUL);
}
int main(int argc,char **argv)
{
    FILE *f;long original;uint8_t *source;unsigned iteration,accepted=0;
    struct pt_allocator allocator={NULL,allocate,release};
    assert(argc==2);f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));original=ftell(f);assert(original>32 && original<65536);rewind(f);
    source=malloc((size_t)original);assert(source);assert(fread(source,1,(size_t)original,f)==(size_t)original);assert(!fclose(f));
    for(iteration=0;iteration<5000;++iteration) {
        size_t n=(iteration%3)?(size_t)original:next()%((size_t)original+33),i,w;
        uint8_t *data=malloc(n?n:1);struct pt_project_requirements req,before;enum pt_project_result result;
        assert(data);memset(data,0,n);memcpy(data,source,n<(size_t)original?n:(size_t)original);
        for(i=0;i<1+iteration%7 && n;++i)data[next()%n]^=(uint8_t)(next()|1);
        if((iteration&1) && n>=32)repair(data,n);
        memset(&req,0xa5,sizeof(req));before=req;result=pt_project_probe(data,n,&req);
        if(result==PT_PROJECT_OK) {
            struct pt_document doc;uint8_t *encoded;size_t bytes;
            pt_document_init(&doc,&allocator);assert(pt_document_load(&doc,data,n,32UL*1024*1024)==PT_PROJECT_OK);
            assert(pt_project_validate(&doc.project,NULL)==PT_PROJECT_OK);
            assert(pt_project_size(&doc.project,&bytes)==PT_PROJECT_OK);encoded=malloc(bytes);assert(encoded);
            assert(pt_project_encode(&doc.project,encoded,bytes,&w)==PT_PROJECT_OK && w==bytes);
            assert(pt_project_probe(encoded,bytes,&req)==PT_PROJECT_OK);free(encoded);pt_document_release(&doc);++accepted;
        } else assert(!memcmp(&before,&req,sizeof(req)));
        free(data);
    }
    free(source);printf("PROJECT MUTATIONS PASS seed=0x24f iterations=5000 accepted_roundtrips=%u\n",accepted);return 0;
}
