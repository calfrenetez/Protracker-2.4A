#include "recent_file.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
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
    int fd=open(w->path[i],O_RDONLY),ok=1;size_t n=0;
    if(fd<0)return 0;
    while(n<sizeof(w->bytes)) {
        long got=(long)read(fd,w->bytes+n,sizeof(w->bytes)-n);
        if(got<0 && errno==EINTR)continue;
        if(got<0) {ok=0;break;}
        if(!got)break;
        n+=(size_t)got;
    }
    if(close(fd))ok=0;
    if(!ok || n<24 || n>PT_RECENT_BYTES+8)return 0;
    w->generation[i]=get32(w->bytes);
    if(!w->generation[i] || get32(w->bytes+4)!=~w->generation[i])return 0;
    return pt_recent_decode(&w->list[i],w->bytes+8,n-8)==PT_RECENT_OK;
}
static struct workspace *open_slots(const char *prefix,const struct pt_allocator *a)
{
    struct workspace *w;size_t n;
    if(!prefix || !(n=strlen(prefix)) || n>PT_RECENT_PATH-3)return NULL;
    if(!a || !a->allocate || !a->release)return NULL;
    w=a->allocate(a->context,sizeof(*w));if(!w)return NULL;
    memset(w,0,sizeof(*w));
    memcpy(w->path[0],prefix,n);memcpy(w->path[1],prefix,n);
    strcpy(w->path[0]+n,".0");strcpy(w->path[1]+n,".1");
    w->valid[0]=read_slot(w,0);w->valid[1]=read_slot(w,1);return w;
}
static int newest(const struct workspace *w)
{
    if(!w->valid[0])return w->valid[1]?1:-1;
    return w->valid[1] && w->generation[1]>w->generation[0]?1:0;
}
int pt_recent_file_load_allocated(const char *prefix,struct pt_recent *r,const struct pt_allocator *a)
{
    struct workspace *w;int current;
    if(!r || !(w=open_slots(prefix,a)))return 0;
    current=newest(w);if(current>=0)*r=w->list[current];a->release(a->context,w);return current>=0;
}
int pt_recent_file_save_allocated(const char *prefix,const struct pt_recent *r,const struct pt_allocator *a)
{
    struct workspace *w;int fd;size_t n,pos;uint32_t generation;int current,target,ok=0;
    if(!r || !(w=open_slots(prefix,a)))return 0;
    current=newest(w);target=current<0?0:1-current;
    generation=current<0?1:w->generation[current]+1;
    /* Refuse wrap rather than letting an older slot win at the next startup. */
    if(!generation || pt_recent_encode(r,w->bytes+8,PT_RECENT_BYTES,&n)!=PT_RECENT_OK)goto done;
    put32(w->bytes,generation);put32(w->bytes+4,~generation);
    fd=open(w->path[target],O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)goto done;
    ok=1;pos=0;
    while(pos<n+8) {
        long wrote=(long)write(fd,w->bytes+pos,n+8-pos);
        if(wrote<0 && errno==EINTR)continue;
        if(wrote<=0) {ok=0;break;}
        pos+=(size_t)wrote;
    }
    if(close(fd))ok=0;
    if(ok)ok=read_slot(w,(unsigned)target) && w->generation[target]==generation &&
        w->list[target].count==r->count;
    if(ok) {unsigned i;for(i=0;i<r->count;++i)if(strcmp(w->list[target].path[i],r->path[i]))ok=0;}
done:
    a->release(a->context,w);return ok;
}

static void *heap_allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void heap_release(void *ctx,void *p) {(void)ctx;free(p);}
static const struct pt_allocator heap={NULL,heap_allocate,heap_release};
int pt_recent_file_load(const char *prefix,struct pt_recent *r)
{return pt_recent_file_load_allocated(prefix,r,&heap);}
int pt_recent_file_save(const char *prefix,const struct pt_recent *r)
{return pt_recent_file_save_allocated(prefix,r,&heap);}
