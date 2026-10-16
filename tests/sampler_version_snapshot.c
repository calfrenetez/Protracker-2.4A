/* Test-only TU substitutes for sampler.c in the boundary fixture. Compile the
 * genuine source once, so the opaque layout is never copied or guessed here.
 * Normal application/library units do not export these test observations. */
#include "../src/editor/sampler.c"
#include "sampler_version_snapshot.h"
#include <assert.h>
#include <stddef.h>
int pt_test_version_header_equal(const void *actual,const void *saved,size_t bytes)
{
    struct pt_sample_version a,b;
    if(!actual || !saved || bytes!=sizeof(a))return 0;
    memcpy(&a,actual,bytes);memcpy(&b,saved,bytes);
    if(!a.references || !b.references)return 0;
    a.references=b.references;
    a.live.previous=b.live.previous;a.live.next=b.live.next;
    return !memcmp(&a,&b,sizeof(a));
}
void pt_test_version_header_fixture(void)
{
    struct pt_sample_version a,b;size_t i;
    const size_t reference=offsetof(struct pt_sample_version,references);
    const size_t previous=offsetof(struct pt_sample_version,live)+offsetof(struct pt_sampler_live,previous);
    const size_t next=offsetof(struct pt_sample_version,live)+offsetof(struct pt_sampler_live,next);
    memset(&a,0,sizeof(a));a.references=1;memcpy(&b,&a,sizeof(a));
    assert(!pt_test_version_header_equal(NULL,&a,sizeof(a)));
    assert(!pt_test_version_header_equal(&a,NULL,sizeof(a)));
    assert(!pt_test_version_header_equal(&a,&a,sizeof(a)-1));
    b.references=2;b.live.previous=&b.live;b.live.next=&a.live;
    assert(pt_test_version_header_equal(&b,&a,sizeof(a)));
    b.references=0;assert(!pt_test_version_header_equal(&b,&a,sizeof(a)));
    /* Every other byte, including all immutable metadata and padding, remains
     * covered. The comparison does not silently omit an opaque whole header. */
    for(i=0;i<sizeof(a);++i){
        if((i>=reference && i-reference<sizeof(a.references)) ||
           (i>=previous && i-previous<sizeof(a.live.previous)) ||
           (i>=next && i-next<sizeof(a.live.next)))continue;
        memcpy(&b,&a,sizeof(a));((unsigned char *)&b)[i]^=1;
        assert(!pt_test_version_header_equal(&b,&a,sizeof(a)));
    }
}
