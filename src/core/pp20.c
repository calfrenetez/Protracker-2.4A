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
