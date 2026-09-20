#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "recent.h"
static struct pt_recent r,copy,before;
static unsigned char bytes[PT_RECENT_BYTES],damaged[PT_RECENT_BYTES];
int main(int argc,char **argv)
{
    char name[PT_RECENT_PATH];size_t n=123,unchanged;unsigned i;
    pt_recent_init(&r);
    for(i=0;i<12;++i) {snprintf(name,sizeof(name),"Work:Songs/Module%02u.mod",i);assert(pt_recent_remember(&r,name)==PT_RECENT_OK);}
    assert(r.count==10 && !strcmp(r.path[0],"Work:Songs/Module11.mod") && !strcmp(r.path[9],"Work:Songs/Module02.mod"));
    assert(pt_recent_remember(&r,"work:songs/MODULE05.mod")==PT_RECENT_OK && r.count==10 && !strcmp(r.path[0],"work:songs/MODULE05.mod"));
    strcpy(name,r.path[7]);assert(pt_recent_remember(&r,r.path[7])==PT_RECENT_OK && !strcmp(name,r.path[0]));
    before=r;memset(name,'X',sizeof(name));assert(pt_recent_remember(&r,name)==PT_RECENT_INVALID && !memcmp(&r,&before,sizeof(r)));
    assert(pt_recent_remember(&r,"bad\npath")==PT_RECENT_INVALID && !memcmp(&r,&before,sizeof(r)));
    assert(pt_recent_remove(&r,10)==PT_RECENT_INVALID && !memcmp(&r,&before,sizeof(r)));
    memset(bytes,0xa5,sizeof(bytes));unchanged=n;assert(pt_recent_encode(&r,bytes,1,&n)==PT_RECENT_CAPACITY && n==unchanged && bytes[0]==0xa5);
    assert(pt_recent_encode(&r,bytes,sizeof(bytes),&n)==PT_RECENT_OK);
    assert(pt_recent_decode(&copy,bytes,n)==PT_RECENT_OK && !memcmp(&r,&copy,sizeof(r)));
    for(i=0;i<n;++i) {memcpy(damaged,bytes,n);damaged[i]^=1;assert(pt_recent_decode(&copy,damaged,n)==PT_RECENT_CORRUPT && !memcmp(&copy,&r,sizeof(r)));}
    for(i=0;i<n;++i)assert(pt_recent_decode(&copy,bytes,i)==PT_RECENT_CORRUPT && !memcmp(&copy,&r,sizeof(r)));
    if(argc==2) {FILE *f=fopen(argv[1],"wb");assert(f && fwrite(bytes,1,n,f)==n && !fclose(f));}
    assert(pt_recent_remove(&r,4)==PT_RECENT_OK && r.count==9 && !strcmp(r.path[4],before.path[5]));
    while(r.count)assert(pt_recent_remove(&r,r.count-1)==PT_RECENT_OK);
    assert(pt_recent_encode(&r,bytes,sizeof(bytes),&n)==PT_RECENT_OK && n==16 && pt_recent_decode(&copy,bytes,n)==PT_RECENT_OK && !copy.count);
    memset(name,'x',sizeof(name)-1);name[sizeof(name)-1]=0;assert(pt_recent_remember(&r,name)==PT_RECENT_OK);
    assert(pt_recent_encode(&r,bytes,sizeof(bytes),&n)==PT_RECENT_OK && pt_recent_decode(&copy,bytes,n)==PT_RECENT_OK);
    puts("RECENT PASS: ten-item MRU, case-folded dedup, alias promotion, bounds, removal, CRC/truncation refusal and atomic decode");return 0;
}
