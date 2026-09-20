#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pp20.h"
#include "document.h"
#include "mod_project.h"
static unsigned calls,fail,live;
static void *allocate(void *ctx,size_t n) {(void)ctx;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *ctx,void *p) {(void)ctx;assert(live);--live;free(p);}
static uint8_t *read_file(const char *name,size_t *n)
{FILE *f=fopen(name,"rb");long size;uint8_t *p;assert(f && !fseek(f,0,SEEK_END));size=ftell(f);assert(size>0);rewind(f);p=malloc((size_t)size);assert(p && fread(p,1,(size_t)size,f)==(size_t)size && !fclose(f));*n=(size_t)size;return p;}
int main(int argc,char **argv)
{
    uint8_t *packed,*plain,*out,*copy,*mod;size_t n,size,needed,w,i,bytes;uint32_t random=1;enum pt_pp20_result result;
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,before;unsigned allocated;
    assert(argc==3);packed=read_file(argv[1],&n);plain=read_file(argv[2],&size);out=malloc(size);copy=malloc(n);mod=malloc(size);assert(out && copy && mod);
    assert(pt_pp20_probe(packed,n,&needed)==PT_PP20_OK && needed==size);
    assert(pt_pp20_decode(packed,n,out,size,&w)==PT_PP20_OK && w==size && !memcmp(out,plain,size));
    memset(out,0x55,size);w=99;assert(pt_pp20_decode(packed,n,out,size-1,&w)==PT_PP20_CAPACITY && w==99);
    assert(pt_pp20_decode(packed,n,packed,size,&w)==PT_PP20_ALIAS);
    for(i=0;i<n && i<64;++i) {needed=99;result=pt_pp20_probe(packed,i,&needed);if(result!=PT_PP20_OK)assert(needed==99);}
    memcpy(copy,packed,n);copy[4]=8;assert(pt_pp20_decode(copy,n,out,size,&w)==PT_PP20_INVALID && w==99);
    memcpy(copy,packed,n);copy[4]=16;assert(pt_pp20_probe(copy,n,&needed)==PT_PP20_INVALID);
    memcpy(copy,packed,n);copy[n-1]=33;assert(pt_pp20_probe(copy,n,&needed)==PT_PP20_INVALID);
    memcpy(copy,packed,n);memset(copy+n-4,0,3);assert(pt_pp20_probe(copy,n,&needed)==PT_PP20_INVALID);
    memcpy(copy,packed,n);copy[1]='X';assert(pt_pp20_probe(copy,n,&needed)==PT_PP20_UNSUPPORTED);
    /* Mutations may remain structurally valid; failures must never write output. */
    for(i=0;i<256;++i) {
        size_t j;memcpy(copy,packed,n);random=random*1664525u+1013904223u;copy[random%n]^=(uint8_t)((random>>24)|1);
        memset(out,0x55,size);w=99;result=pt_pp20_decode(copy,n,out,size,&w);
        if(result!=PT_PP20_OK) {assert(w==99);for(j=0;j<size;++j)assert(out[j]==0x55);}
        else assert(w<=size);
    }
    pt_document_init(&d,&a);assert(pt_document_load(&d,plain,size,SIZE_MAX)==PT_PROJECT_OK);d.dirty=1;before=d;allocated=live;
    /* This classic fixture has six owned candidate allocations plus PP scratch. */
    for(i=1;i<=7;++i) {
        calls=0;fail=(unsigned)i;assert(pt_document_load(&d,packed,n,SIZE_MAX)==PT_PROJECT_CAPACITY);
        assert(live==allocated && !memcmp(&d,&before,sizeof(d)));
    }
    calls=0;fail=0;assert(pt_document_load(&d,packed,n,size-1)==PT_PROJECT_CAPACITY && !calls);
    assert(pt_document_load(&d,packed,n,size+d.allocated_bytes-1)==PT_PROJECT_CAPACITY && live==allocated && !memcmp(&d,&before,sizeof(d)));
    memcpy(copy,packed,n);copy[4]=1;calls=0;assert(pt_document_load(&d,copy,n,SIZE_MAX)==PT_PROJECT_INVALID && !calls && !memcmp(&d,&before,sizeof(d)));
    copy[1]='X';assert(pt_document_load(&d,copy,n,SIZE_MAX)==PT_PROJECT_UNSUPPORTED && !calls && !memcmp(&d,&before,sizeof(d)));
    {
        const uint8_t packed_nonmod[]={0x50,0x50,0x32,0x30,9,10,12,13,0,0,4,16,0,0,1,0};
        assert(pt_document_load(&d,packed_nonmod,sizeof(packed_nonmod),SIZE_MAX)!=PT_PROJECT_OK && live==allocated && !memcmp(&d,&before,sizeof(d)));
    }
    assert(pt_document_load(&d,packed,n,SIZE_MAX)==PT_PROJECT_OK && !d.dirty && live==allocated);
    assert(pt_mod_export_direct(&d.project,mod,size,&bytes)==PT_PROJECT_OK && bytes==size && !memcmp(mod,plain,size));
    for(i=0;i<16;++i)assert(pt_document_load(&d,packed,n,SIZE_MAX)==PT_PROJECT_OK && live==allocated);
    pt_document_release(&d);assert(!live);free(packed);free(plain);free(out);free(copy);free(mod);
    puts("PP20 PASS: exact unpack, malformed bounds, unchanged failed output, atomic MOD import, all allocation failures, peak budget and lossless MOD roundtrip");return 0;
}
