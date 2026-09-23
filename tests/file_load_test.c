#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/platform/file_load.h"
static int live,fail,mutation;
static const char *filename;
static unsigned char source[70001];
static void write_source(size_t n)
{
    FILE *f=fopen(filename,"wb");assert(f);
    assert(fwrite(source,1,n,f)==n);assert(!fclose(f));
}
static void *allocate(void *ctx,size_t n)
{
    void *p;(void)ctx;assert(!live);
    if(fail)return NULL;
    if(mutation)write_source(mutation==1?12:sizeof(source));
    p=malloc(n);assert(p);live=1;return p;
}
static void release(void *ctx,void *p)
{(void)ctx;assert(live && p);free(p);live=0;}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};uint8_t sentinel,*p=&sentinel;
    size_t n=123,i;char path[1024];assert(argc==2);
    assert(snprintf(path,sizeof(path),"%s/input.bin",argv[1])>0);filename=path;
    for(i=0;i<sizeof(source);++i)source[i]=(unsigned char)(i*13);
    write_source(70000);
    assert(pt_file_load(path,69999,&a,&p,&n)==PT_LOAD_LIMIT);
    assert(p==&sentinel && n==123 && !live);
    fail=1;assert(pt_file_load(path,70000,&a,&p,&n)==PT_LOAD_MEMORY);fail=0;
    assert(p==&sentinel && n==123 && !live);
    assert(pt_file_load(path,70000,&a,&p,&n)==PT_LOAD_OK);
    assert(n==70000 && !memcmp(p,source,n));release(NULL,p);p=&sentinel;n=123;
#ifndef __amigaos__
    /* AmigaDOS refuses reopening the input for writing while its descriptor is
       open. Keep concurrent size-change coverage on hosts that permit it. */
    for(mutation=1;mutation<=2;++mutation) {
        write_source(70000);
        assert(pt_file_load(path,70000,&a,&p,&n)==PT_LOAD_IO);
        assert(p==&sentinel && n==123 && !live);
    }
#else
    puts("FILE LOAD: concurrent rewrite cases host-only (AmigaDOS sharing)");
#endif
    mutation=0;write_source(0);
    assert(pt_file_load(path,0,&a,&p,&n)==PT_LOAD_OK && !n);release(NULL,p);
    assert(!remove(path));p=&sentinel;n=123;
    assert(pt_file_load(path,70000,&a,&p,&n)==PT_LOAD_IO);
    assert(p==&sentinel && n==123 && !live);
    assert(pt_file_load(path,70000,NULL,&p,&n)==PT_LOAD_INVALID);
    puts("FILE LOAD PASS");return 0;
}
