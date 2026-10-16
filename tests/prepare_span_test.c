/* Forged numbers exercise a pure predicate; no topology or pointee is read. */
#include "../src/editor/prepare_span_internal.h"
#ifndef PT_PREPARE_SPAN_NO_MAIN
#include <assert.h>
#endif
#include <stdio.h>
static int bounds_reference_span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int bounds_reference_inside(const void *parent,size_t parent_bytes,const void *p,size_t n)
{return n&&bounds_reference_span(parent,parent_bytes)&&bounds_reference_span(p,n)&&parent_bytes>=n&&
 (uintptr_t)p>=(uintptr_t)parent&&(uintptr_t)p-(uintptr_t)parent<=parent_bytes-n;}
static void pt_prepare_span_controls(void)
{
    const uintptr_t address[]={0,1,2,7,8,15,16,255,256,UINTPTR_MAX/2,
                              UINTPTR_MAX/2+1,UINTPTR_MAX-2,UINTPTR_MAX-1,UINTPTR_MAX};
    const size_t bytes[]={0,1,2,7,8,15,16,255,256,SIZE_MAX/2,SIZE_MAX-1,SIZE_MAX};
    size_t a,b,c,d;unsigned yes=0,no=0;uintptr_t state=UINT32_C(0x13579bdf);
    assert(pt_prepare_span_contains((const void *)(uintptr_t)8,16,(const void *)(uintptr_t)8,16));
    assert(pt_prepare_span_contains((const void *)(uintptr_t)8,16,(const void *)(uintptr_t)23,1));
    assert(!pt_prepare_span_contains((const void *)(uintptr_t)8,16,(const void *)(uintptr_t)24,1));
    assert(!pt_prepare_span_contains((const void *)(uintptr_t)8,16,(const void *)(uintptr_t)24,0));
    for(a=0;a<sizeof(address)/sizeof(address[0]);++a)
    for(b=0;b<sizeof(bytes)/sizeof(bytes[0]);++b)
    for(c=0;c<sizeof(address)/sizeof(address[0]);++c)
    for(d=0;d<sizeof(bytes)/sizeof(bytes[0]);++d){
        int expected=bounds_reference_inside((const void *)address[a],bytes[b],(const void *)address[c],bytes[d]);
        assert(pt_prepare_span_contains((const void *)address[a],bytes[b],(const void *)address[c],bytes[d])==expected);
        if(expected)++yes;else ++no;
    }
    for(a=0;a<20000;++a){
        uintptr_t parent,child;size_t pn,n;int expected;
#define NEXT_VALUE() do {state^=state<<13;state^=state>>7;state^=state<<17;}while(0)
        NEXT_VALUE();parent=state;NEXT_VALUE();pn=(size_t)state;
        NEXT_VALUE();child=state;NEXT_VALUE();n=(size_t)state;
#undef NEXT_VALUE
        expected=bounds_reference_inside((const void *)parent,pn,(const void *)child,n);
        assert(pt_prepare_span_contains((const void *)parent,pn,(const void *)child,n)==expected);
        if(expected)++yes;else ++no;
    }
    assert(yes&&no);
}
#ifndef PT_PREPARE_SPAN_NO_MAIN
int main(void)
{pt_prepare_span_controls();puts("PREPARE SPAN EQUIVALENCE PASS");return 0;}
#endif
