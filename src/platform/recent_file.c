#include "recent_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Generation and its complement precede the checksummed PTRC record. */
struct workspace {
    struct pt_recent list[2];
    unsigned char bytes[PT_RECENT_BYTES+9];
    char path[2][PT_RECENT_PATH];
    uint32_t generation[2];
    int valid[2];
};
static uint32_t get32(const unsigned char *p)
{return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];}
static void put32(unsigned char *p,uint32_t n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static int read_slot(struct workspace *w,unsigned i)
{
    FILE *f=fopen(w->path[i],"rb");size_t n;int ok;
    if(!f)return 0;
    n=fread(w->bytes,1,sizeof(w->bytes),f);ok=!ferror(f);if(fclose(f))ok=0;
    if(!ok || n<24 || n>PT_RECENT_BYTES+8)return 0;
    w->generation[i]=get32(w->bytes);
    if(!w->generation[i] || get32(w->bytes+4)!=~w->generation[i])return 0;
    return pt_recent_decode(&w->list[i],w->bytes+8,n-8)==PT_RECENT_OK;
}
static struct workspace *open_slots(const char *prefix)
{
    struct workspace *w;size_t n;
    if(!prefix || !(n=strlen(prefix)) || n>PT_RECENT_PATH-3)return NULL;
    w=calloc(1,sizeof(*w));if(!w)return NULL;
    memcpy(w->path[0],prefix,n);memcpy(w->path[1],prefix,n);
    strcpy(w->path[0]+n,".0");strcpy(w->path[1]+n,".1");
    w->valid[0]=read_slot(w,0);w->valid[1]=read_slot(w,1);return w;
}
static int newest(const struct workspace *w)
{
    if(!w->valid[0])return w->valid[1]?1:-1;
    return w->valid[1] && w->generation[1]>w->generation[0]?1:0;
}
int pt_recent_file_load(const char *prefix,struct pt_recent *r)
{
    struct workspace *w;int current;
    if(!r || !(w=open_slots(prefix)))return 0;
    current=newest(w);if(current>=0)*r=w->list[current];free(w);return current>=0;
}
int pt_recent_file_save(const char *prefix,const struct pt_recent *r)
{
    struct workspace *w;FILE *f;size_t n;uint32_t generation;int current,target,ok=0;
    if(!r || !(w=open_slots(prefix)))return 0;
    current=newest(w);target=current<0?0:1-current;
    generation=current<0?1:w->generation[current]+1;
    /* Refuse wrap rather than letting an older slot win at the next startup. */
    if(!generation || pt_recent_encode(r,w->bytes+8,PT_RECENT_BYTES,&n)!=PT_RECENT_OK)goto done;
    put32(w->bytes,generation);put32(w->bytes+4,~generation);
    f=fopen(w->path[target],"wb");if(!f)goto done;
    ok=fwrite(w->bytes,1,n+8,f)==n+8 && !ferror(f);if(fclose(f))ok=0;
    if(ok)ok=read_slot(w,(unsigned)target) && w->generation[target]==generation &&
        w->list[target].count==r->count;
    if(ok) {unsigned i;for(i=0;i<r->count;++i)if(strcmp(w->list[target].path[i],r->path[i]))ok=0;}
done:
    free(w);return ok;
}
