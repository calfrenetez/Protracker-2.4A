#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/platform/recent_file.h"
static struct pt_recent a,b,r,before;
static unsigned char bytes[PT_RECENT_BYTES+8];
static void write_bytes(const char *path,size_t n)
{FILE *f=fopen(path,"wb");assert(f && fwrite(bytes,1,n,f)==n && !fclose(f));}
int main(int argc,char **argv)
{
    char slot[PT_RECENT_PATH];FILE *f;size_t n,i;unsigned bit;
    assert(argc==2 && strlen(argv[1])<PT_RECENT_PATH-3);
    assert(pt_recent_remember(&r,"Work:preserved.ptg")==PT_RECENT_OK);before=r;
    assert(!pt_recent_file_load(argv[1],&r) && !memcmp(&r,&before,sizeof(r)));
    assert(pt_recent_remember(&a,"Work:first.mod")==PT_RECENT_OK);
    assert(pt_recent_file_save(argv[1],&a));
    assert(pt_recent_file_load(argv[1],&r) && !memcmp(&r,&a,sizeof(r)));
    b=a;assert(pt_recent_remember(&b,"Work:second.pp20")==PT_RECENT_OK);
    assert(pt_recent_file_save(argv[1],&b));
    assert(pt_recent_file_load(argv[1],&r) && !memcmp(&r,&b,sizeof(r)));
    snprintf(slot,sizeof(slot),"%s.1",argv[1]);f=fopen(slot,"rb");assert(f);
    n=fread(bytes,1,sizeof(bytes),f);assert(n>24 && n<sizeof(bytes) && !fclose(f));
    /* Every interrupted-write length and every single-bit corruption of the
       latest record must recover the complete previous list. */
    for(i=0;i<n;++i) {
        write_bytes(slot,i);assert(pt_recent_file_load(argv[1],&r) && !memcmp(&r,&a,sizeof(r)));
        for(bit=0;bit<8;++bit) {
            bytes[i]^=(unsigned char)(1U<<bit);write_bytes(slot,n);
            assert(pt_recent_file_load(argv[1],&r) && !memcmp(&r,&a,sizeof(r)));
            bytes[i]^=(unsigned char)(1U<<bit);
        }
    }
    write_bytes(slot,n);assert(pt_recent_file_load(argv[1],&r) && !memcmp(&r,&b,sizeof(r)));
    /* Clear is itself a new valid generation, not removal of the good backup. */
    pt_recent_init(&r);assert(pt_recent_file_save(argv[1],&r));
    assert(pt_recent_file_load(argv[1],&r) && !r.count);
    r.count=11;assert(!pt_recent_file_save(argv[1],&r));
    assert(pt_recent_file_load(argv[1],&r) && !r.count);
    puts("RECENT FILE PASS: alternating generations, interrupted/corrupt write recovery, persisted clear, invalid-save refusal");
    return 0;
}
