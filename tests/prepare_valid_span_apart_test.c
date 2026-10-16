/* Numeric golden predicates only: no pointee, owner or device is followed. */
#include "../src/editor/prepare_span_internal.h"
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>

/* Frozen original editor_mixed_causal_prepare.c predicates; names only differ.
 * The production helper may omit the first span check only after the caller's
 * original non-NULL, nonempty and nonoverflowing first-span check succeeds. */
static int original_span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int original_apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return original_span(a,n)&&original_span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}

static uint64_t tuples,valid_first,accepted,refused,invalid_first;
static uint64_t model_special_calls,host_special_calls;
static uint64_t random_state=UINT64_C(0x24a6d070616c17b9);

static uint64_t next_value(void)
{random_state^=random_state<<13;random_state^=random_state>>7;
 random_state^=random_state<<17;return random_state;}
static int model_span(uint64_t address,uint64_t bytes,uint64_t maximum)
{return !bytes||(address&&bytes<=maximum-address);}
static int model_apart(uint64_t first,uint64_t first_bytes,
    uint64_t other,uint64_t other_bytes,uint64_t maximum)
{return model_span(first,first_bytes,maximum)&&model_span(other,other_bytes,maximum)&&
 (!first_bytes||!other_bytes||(first<=other?first_bytes<=other-first:other_bytes<=first-other));}
static int model_valid_apart(uint64_t first,uint64_t first_bytes,
    uint64_t other,uint64_t other_bytes,uint64_t maximum)
{return model_span(other,other_bytes,maximum)&&
 (!other_bytes||(first<=other?first_bytes<=other-first:other_bytes<=first-other));}

static void compare(uint64_t first,uint64_t first_bytes,uint64_t other,
    uint64_t other_bytes,uint64_t maximum,int matching_host_abi)
{
    int valid=first&&first_bytes&&model_span(first,first_bytes,maximum);
    int original_entry=valid&&model_apart(first,first_bytes,other,other_bytes,maximum);
    int specialized_entry=0;
    assert(first<=maximum&&first_bytes<=maximum&&other<=maximum&&other_bytes<=maximum);
    ++tuples;
    if(valid){
        int expected=model_apart(first,first_bytes,other,other_bytes,maximum);
        int actual=model_valid_apart(first,first_bytes,other,other_bytes,maximum);
        ++valid_first;++model_special_calls;
        assert(actual==expected);
        if(actual)++accepted;else ++refused;
        specialized_entry=actual;
    }else ++invalid_first;
    assert(original_entry==specialized_entry);
    if(matching_host_abi){
        const void *p=(const void *)(uintptr_t)first;
        const void *other_pointer=(const void *)(uintptr_t)other;
        size_t n=(size_t)first_bytes,m=(size_t)other_bytes;
        int original_valid=p&&n&&original_span(p,n);
        int expected=original_valid&&original_apart(p,n,other_pointer,m),actual=0;
        assert(original_valid==valid);
        /* Invalid first spans are rejected at the original caller boundary;
         * they are deliberately never passed to the specialized helper. */
        if(original_valid){
            ++host_special_calls;
            actual=pt_prepare_valid_span_apart(p,n,other_pointer,m);
        }
        assert(actual==expected&&actual==original_entry);
    }
}

static unsigned edge_values(uint64_t maximum,uint64_t *values)
{
    const uint64_t raw[]={0,1,2,3,7,8,15,16,31,32,63,64,127,128,255,256,
        4095,4096,65535,65536,maximum/2,maximum/2+1,maximum-16,maximum-8,
        maximum-4,maximum-3,maximum-2,maximum-1,maximum};
    unsigned i,j,count=0;
    for(i=0;i<sizeof(raw)/sizeof(raw[0]);++i){
        if(raw[i]>maximum)continue;
        for(j=0;j<count&&values[j]!=raw[i];++j){}
        if(j==count)values[count++]=raw[i];
    }
    return count;
}
static void boundary_domain(uint64_t maximum,int matching_host_abi)
{
    uint64_t values[40];unsigned count=edge_values(maximum,values),a,b,c,d,k;
    for(a=0;a<count;++a)for(b=0;b<count;++b)
    for(c=0;c<count;++c)for(d=0;d<count;++d)
        compare(values[a],values[b],values[c],values[d],maximum,matching_host_abi);
    for(k=0;k<100000;++k){
        uint64_t first=next_value()&maximum,bytes=next_value()&maximum;
        uint64_t other=next_value()&maximum,other_bytes=next_value()&maximum;
        compare(first,bytes,other,other_bytes,maximum,matching_host_abi);
        if(first&&maximum-first){
            bytes=1+(next_value()%(maximum-first));
            compare(first,bytes,other,other_bytes,maximum,matching_host_abi);
            compare(first,bytes,first,bytes,maximum,matching_host_abi); /* Exact alias. */
            compare(first,bytes,first+bytes,0,maximum,matching_host_abi); /* Empty endpoint. */
            if(first+bytes<maximum)
                compare(first,bytes,first+bytes,1,maximum,matching_host_abi); /* Adjacent. */
            if(bytes>1)
                compare(first,bytes,first+bytes-1,1,maximum,matching_host_abi); /* Overlap. */
        }
    }
}

int main(void)
{
    unsigned first,bytes,other,other_bytes;
    /* The five-bit model exhausts every tuple, including invalid first spans.
     * Wider models are finite boundary/random controls, not native ABI proof. */
    assert(SIZE_MAX==UINTPTR_MAX);
    assert(UINTPTR_MAX==UINT32_MAX||UINTPTR_MAX==UINT64_MAX);
    for(first=0;first<=31;++first)for(bytes=0;bytes<=31;++bytes)
    for(other=0;other<=31;++other)for(other_bytes=0;other_bytes<=31;++other_bytes)
        compare(first,bytes,other,other_bytes,31,0);
    boundary_domain(UINT32_MAX,UINTPTR_MAX==UINT32_MAX);
    boundary_domain(UINT64_MAX,UINTPTR_MAX==UINT64_MAX);
    assert(tuples==UINT64_C(3663138)&&valid_first==UINT64_C(2387186));
    assert(accepted==UINT64_C(837705)&&refused==UINT64_C(1549481));
    assert(invalid_first==UINT64_C(1275952));
    assert(model_special_calls==valid_first&&host_special_calls>0);
    printf("PREPARE VALID SPAN APART PASS:%" PRIu64 " tuples;%" PRIu64
        " valid-first;%" PRIu64 " accept;%" PRIu64 " refuse;%" PRIu64
        " invalid-first;MODELED32/64;HOST predicate only\n",
        tuples,valid_first,accepted,refused,invalid_first);
    return 0;
}
