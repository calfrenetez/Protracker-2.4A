/* PowerPacker decrunch, based on the Public Domain algorithm by Stuart Caie,
   Heikki Orsila's amigadepack and libxmp's ppdepack.c (Claudio Matsuoka).
   Pinned reference: libxmp 1e0812137e4f9e15cc026c0c75af82d71d3cb5ad.
   This adaptation uses index bounds and a validation-only pass before writing.
   See the accompanying provenance document and retained original notices. */
#include <string.h>
#include "pp20.h"
struct bits {const uint8_t *data;size_t pos;unsigned left;uint8_t byte;};
static int read_bits(struct bits *b,unsigned n,uint32_t *value)
{
    uint32_t v=0;unsigned i;
    for(i=0;i<n;++i) {
        if(!b->left) {if(b->pos<=8)return 0;b->byte=b->data[--b->pos];b->left=8;}
        v=(v<<1)|(b->byte&1);b->byte>>=1;--b->left;
    }
    *value=v;return 1;
}
static int walk(const uint8_t *data,size_t n,uint8_t *out,size_t size)
{
    struct bits b={data,n-4,0,0};uint32_t value,length,offset,width;size_t remaining=size;
    if(!read_bits(&b,data[n-1],&value))return 0;
    while(remaining) {
        if(!read_bits(&b,1,&value))return 0;
        if(!value) {
            length=1;
            do {
                if(!read_bits(&b,2,&value) || value>remaining-length)return 0;
                length+=value;
            } while(value==3);
            while(length--) {
                if(!read_bits(&b,8,&value))return 0;
                --remaining;if(out)out[remaining]=(uint8_t)value;
            }
            if(!remaining)break;
        }
        if(!read_bits(&b,2,&value))return 0;
        width=data[4+value];length=value+2;
        if(length>remaining)return 0;
        if(value==3) {
            if(!read_bits(&b,1,&value))return 0;
            if(!value)width=7;
            if(!read_bits(&b,width,&offset))return 0;
            do {
                if(!read_bits(&b,3,&value) || value>remaining-length)return 0;
                length+=value;
            } while(value==7);
        } else if(!read_bits(&b,width,&offset))return 0;
        if(offset>=size-remaining)return 0;
        while(length--) {if(out)out[remaining-1]=out[remaining+offset];--remaining;}
    }
    return 1;
}
enum pt_pp20_result pt_pp20_probe(const uint8_t *data,size_t n,size_t *out)
{
    size_t size;unsigned i;
    if(!data || !out)return PT_PP20_INVALID;
    if(n<4)return PT_PP20_INVALID;
    if(memcmp(data,"PP20",4))return PT_PP20_UNSUPPORTED;
    if(n<16 || (n&3))return PT_PP20_INVALID;
    for(i=4;i<8;++i)if(data[i]<9 || data[i]>15)return PT_PP20_INVALID;
    size=((size_t)data[n-4]<<16)|((size_t)data[n-3]<<8)|data[n-2];
    if(!size || data[n-1]>32 || !walk(data,n,NULL,size))return PT_PP20_INVALID;
    *out=size;return PT_PP20_OK;
}
enum pt_pp20_result pt_pp20_decode(const uint8_t *data,size_t n,uint8_t *out,size_t capacity,size_t *written)
{
    size_t size;uintptr_t a=(uintptr_t)data,b=(uintptr_t)out;enum pt_pp20_result result=pt_pp20_probe(data,n,&size);
    if(result!=PT_PP20_OK)return result;
    if(!out || !written)return PT_PP20_INVALID;
    if(capacity<size)return PT_PP20_CAPACITY;
    if(n>UINTPTR_MAX-a || size>UINTPTR_MAX-b || (a<b+size && b<a+n))return PT_PP20_ALIAS;
    if(!walk(data,n,out,size))return PT_PP20_INVALID; /* Same immutable input already validated. */
    *written=size;return PT_PP20_OK;
}

/* Reverse positional input cache; output back-references still require a full
 * unpublished unpacked buffer. The packed file need not be retained in RAM. */
struct reader_bits {
    pt_pp20_read read;void *context;size_t pos,start,end;
    unsigned left;uint8_t byte,block[256];
};
static int reader_bits(struct reader_bits *b,unsigned n,uint32_t *value)
{
    uint32_t v=0;unsigned i;
    for(i=0;i<n;++i) {
        if(!b->left) {
            if(b->pos<=8)return 0;
            --b->pos;
            if(b->pos<b->start || b->pos>=b->end) {
                b->end=b->pos+1;b->start=b->end>264?b->end-256:8;
                if(b->read(b->context,b->start,b->block,b->end-b->start)!=1)return 0;
            }
            b->byte=b->block[b->pos-b->start];b->left=8;
        }
        v=(v<<1)|(b->byte&1);b->byte>>=1;--b->left;
    }
    *value=v;return 1;
}
static enum pt_pp20_result reader_header(pt_pp20_read read,void *context,size_t n,uint8_t header[8],uint8_t tail[4],size_t *size)
{
    unsigned i;
    if(!read || n<4)return PT_PP20_INVALID;
    if(read(context,0,header,4)!=1)return PT_PP20_INVALID;
    if(memcmp(header,"PP20",4))return PT_PP20_UNSUPPORTED;
    if(n<16 || (n&3))return PT_PP20_INVALID;
    if(read(context,4,header+4,4)!=1 || read(context,n-4,tail,4)!=1)return PT_PP20_INVALID;
    for(i=4;i<8;++i)if(header[i]<9 || header[i]>15)return PT_PP20_INVALID;
    *size=((size_t)tail[0]<<16)|((size_t)tail[1]<<8)|tail[2];
    return !*size || tail[3]>32?PT_PP20_INVALID:PT_PP20_OK;
}
static int walk_reader(pt_pp20_read read,void *context,const uint8_t widths[4],unsigned skip,size_t n,uint8_t *out,size_t size)
{
    struct reader_bits b={0};uint32_t value,length,offset,width;size_t remaining=size;
    b.read=read;b.context=context;b.pos=n-4;
    if(!reader_bits(&b,skip,&value))return 0;
    while(remaining) {
        if(!reader_bits(&b,1,&value))return 0;
        if(!value) {
            length=1;
            do {
                if(!reader_bits(&b,2,&value) || value>remaining-length)return 0;
                length+=value;
            } while(value==3);
            while(length--) {
                if(!reader_bits(&b,8,&value))return 0;
                --remaining;if(out)out[remaining]=(uint8_t)value;
            }
            if(!remaining)break;
        }
        if(!reader_bits(&b,2,&value))return 0;
        width=widths[value];length=value+2;
        if(length>remaining)return 0;
        if(value==3) {
            if(!reader_bits(&b,1,&value))return 0;
            if(!value)width=7;
            if(!reader_bits(&b,width,&offset))return 0;
            do {
                if(!reader_bits(&b,3,&value) || value>remaining-length)return 0;
                length+=value;
            } while(value==7);
        } else if(!reader_bits(&b,width,&offset))return 0;
        if(offset>=size-remaining)return 0;
        while(length--) {if(out)out[remaining-1]=out[remaining+offset];--remaining;}
    }
    return 1;
}
enum pt_pp20_result pt_pp20_probe_reader(pt_pp20_read read,void *context,size_t n,size_t *out)
{
    uint8_t header[8],tail[4];size_t size;enum pt_pp20_result r;
    if(!out)return PT_PP20_INVALID;
    r=reader_header(read,context,n,header,tail,&size);if(r!=PT_PP20_OK)return r;
    if(!walk_reader(read,context,header+4,tail[3],n,NULL,size))return PT_PP20_INVALID;
    *out=size;return PT_PP20_OK;
}
enum pt_pp20_result pt_pp20_decode_reader(pt_pp20_read read,void *context,size_t n,uint8_t *out,size_t capacity,size_t *written)
{
    uint8_t header[8],tail[4];size_t size;enum pt_pp20_result r;
    if(!out || !written)return PT_PP20_INVALID;
    r=reader_header(read,context,n,header,tail,&size);if(r!=PT_PP20_OK)return r;
    if(!walk_reader(read,context,header+4,tail[3],n,NULL,size))return PT_PP20_INVALID;
    if(capacity<size)return PT_PP20_CAPACITY;
    if(!walk_reader(read,context,header+4,tail[3],n,out,size))return PT_PP20_INVALID;
    *written=size;return PT_PP20_OK;
}
