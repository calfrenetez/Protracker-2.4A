#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "document.h"
#include "../src/platform/file_save.h"
static unsigned live,refuse;
static void *allocate(void *ctx,size_t n) {(void)ctx;assert(!live);if(refuse)return NULL;void *p=malloc(n);assert(p);++live;return p;}
static void release(void *ctx,void *p) {(void)ctx;assert(live && p);--live;free(p);}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};uint8_t data[9000],check[9000];char path[1200];FILE *f;unsigned i;
    assert(argc==2 && strlen(argv[1])<1000);for(i=0;i<sizeof(data);++i)data[i]=(uint8_t)(i*37);
    snprintf(path,sizeof(path),"%s/saved.bin",argv[1]);refuse=1;
    assert(pt_file_save_new_allocated(path,data,sizeof(data),&a)==PT_SAVE_MEMORY && !live && access(path,F_OK));refuse=0;
    assert(pt_file_save_new_allocated(path,data,sizeof(data),&a)==PT_SAVE_OK && !live);
    assert(pt_file_save_new_allocated(path,"REPLACE",7,&a)==PT_SAVE_PUBLISH && !live);
    f=fopen(path,"rb");assert(f && fread(check,1,sizeof(check),f)==sizeof(check) && fgetc(f)==EOF && !ferror(f));assert(!fclose(f));
    assert(!memcmp(data,check,sizeof(data)));
    for(i=0;i<sizeof(data);++i)assert(data[i]==(uint8_t)(i*37));
    snprintf(path,sizeof(path),"%s/empty.bin",argv[1]);assert(pt_file_save_new_allocated(path,NULL,0,&a)==PT_SAVE_OK && !live);
    puts("FILE SAVE MEMORY PASS: exact multi-block bytes, allocation refusal, existing destination and zero-length preservation");return 0;
}
