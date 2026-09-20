#include "recent.h"
#include <string.h>
static size_t length(const char *s)
{
    size_t i;if(!s)return 0;
    for(i=0;i<PT_RECENT_PATH;++i) {
        unsigned c=(unsigned char)s[i];if(!c)return i;
        if(c<32 || c==127)return 0;
    }
    return 0;
}
static unsigned fold(unsigned c) {return c>='A' && c<='Z'?c+32:c;}
static int equal(const char *a,const char *b)
{
    while(*a && *b)if(fold((unsigned char)*a++)!=fold((unsigned char)*b++))return 0;
    return *a==*b;
}
static int valid(const struct pt_recent *r)
{
    unsigned i,j;if(!r || r->count>PT_RECENT_LIMIT)return 0;
    for(i=0;i<r->count;++i) {
        if(!length(r->path[i]))return 0;
        for(j=0;j<i;++j)if(equal(r->path[i],r->path[j]))return 0;
    }
    return 1;
}
void pt_recent_init(struct pt_recent *r) {if(r)memset(r,0,sizeof(*r));}
enum pt_recent_result pt_recent_remember(struct pt_recent *r,const char *path)
{
    char name[PT_RECENT_PATH];size_t n=length(path);unsigned i;
    if(!n || !valid(r))return PT_RECENT_INVALID;
    memcpy(name,path,n+1); /* Source may be a list member that is about to move. */
    for(i=0;i<r->count;++i)if(equal(name,r->path[i]))break;
    if(i==r->count) {if(r->count<PT_RECENT_LIMIT)++r->count;else --i;}
    if(i)memmove(r->path[1],r->path[0],i*PT_RECENT_PATH);
    memset(r->path[0],0,PT_RECENT_PATH);memcpy(r->path[0],name,n+1);return PT_RECENT_OK;
}
enum pt_recent_result pt_recent_remove(struct pt_recent *r,unsigned index)
{
    if(!valid(r) || index>=r->count)return PT_RECENT_INVALID;
    --r->count;if(index<r->count)memmove(r->path[index],r->path[index+1],(r->count-index)*PT_RECENT_PATH);
    memset(r->path[r->count],0,PT_RECENT_PATH);return PT_RECENT_OK;
}
static void put16(uint8_t *p,unsigned n) {p[0]=(uint8_t)(n>>8);p[1]=(uint8_t)n;}
static unsigned get16(const uint8_t *p) {return (unsigned)p[0]*256+p[1];}
static void put32(uint8_t *p,uint32_t n) {put16(p,n>>16);put16(p+2,n&65535);}
static uint32_t get32(const uint8_t *p) {return (uint32_t)get16(p)*65536+get16(p+2);}
static uint32_t crc(const uint8_t *p,size_t n)
{
    uint32_t c=UINT32_MAX;size_t i;unsigned bit;
    for(i=0;i<n;++i) {c^=(i>=12 && i<16)?0:p[i];for(bit=0;bit<8;++bit)c=(c>>1)^((0U-(c&1))&0xedb88320UL);}
    return ~c;
}
enum pt_recent_result pt_recent_encode(const struct pt_recent *r,void *data,size_t capacity,size_t *written)
{
    uint8_t *out=data;size_t size=16,pos=16,n;unsigned i;
    if(!data || !written || !valid(r))return PT_RECENT_INVALID;
    for(i=0;i<r->count;++i)size+=2+length(r->path[i]);
    if(capacity<size)return PT_RECENT_CAPACITY;
    memset(out,0,16);memcpy(out,"PTRC",4);put16(out+4,1);put16(out+6,r->count);put32(out+8,(uint32_t)size);
    for(i=0;i<r->count;++i) {n=length(r->path[i]);put16(out+pos,(unsigned)n);pos+=2;memcpy(out+pos,r->path[i],n);pos+=n;}
    put32(out+12,crc(out,size));*written=size;return PT_RECENT_OK;
}
enum pt_recent_result pt_recent_decode(struct pt_recent *r,const void *data,size_t size)
{
    const uint8_t *in=data;struct pt_recent next;size_t pos=16,n;unsigned i;
    if(!r || !data)return PT_RECENT_INVALID;
    if(size<16 || size>PT_RECENT_BYTES || memcmp(in,"PTRC",4) || get16(in+4)!=1 ||
       get16(in+6)>PT_RECENT_LIMIT || get32(in+8)!=size || get32(in+12)!=crc(in,size))return PT_RECENT_CORRUPT;
    pt_recent_init(&next);next.count=(uint8_t)get16(in+6);
    for(i=0;i<next.count;++i) {
        if(size-pos<2)return PT_RECENT_CORRUPT;
        n=get16(in+pos);pos+=2;
        if(!n || n>=PT_RECENT_PATH || n>size-pos || memchr(in+pos,0,n))return PT_RECENT_CORRUPT;
        memcpy(next.path[i],in+pos,n);pos+=n;
    }
    if(pos!=size || !valid(&next))return PT_RECENT_CORRUPT;
    *r=next;return PT_RECENT_OK;
}
