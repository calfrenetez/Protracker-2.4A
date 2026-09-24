#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "studio_queue.h"
static unsigned live,refuse;
static void *alloc(void *c,size_t n) {(void)c;if(refuse)return NULL;++live;return malloc(n);}
static void drop(void *c,void *p) {(void)c;--live;free(p);}
int main(void)
{
    struct pt_allocator a={NULL,alloc,drop};struct pt_studio_queue *q;
    int32_t data[]={1,257,-513,8388607,-8388608,1025};struct pt_pcm source={data,6,3,48000,2,24};
    const struct pt_pcm *out=NULL;uint64_t ticket=0,old;unsigned i;
    assert(!pt_studio_queue_open(&a,0) && !pt_studio_queue_open(&a,9));refuse=1;assert(!pt_studio_queue_open(&a,2));refuse=0;
    q=pt_studio_queue_open(&a,2);assert(q && live==1);
    assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_EMPTY && !out && !ticket);
    for(i=0;i<32;++i) {
        data[0]=(int32_t)i;assert(pt_studio_queue_push(q,&source)==PT_QUEUE_OK);
        data[0]=999;assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK && out->data[0]==(int32_t)i && out->data[1]==257);
        old=ticket;assert(pt_studio_queue_push(q,&source)==PT_QUEUE_OK);
        assert(pt_studio_queue_push(q,&source)==PT_QUEUE_FULL && out->data[0]==(int32_t)i);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_BUSY && ticket==old);
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY && live==1);
        assert(pt_studio_queue_release(q,ticket+1)==PT_QUEUE_INVALID);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK && out->data[0]==999 && ticket>old);
        assert(pt_studio_queue_release(q,old)==PT_QUEUE_INVALID);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
    }
    assert(pt_studio_queue_push(q,&source)==PT_QUEUE_OK);pt_studio_queue_finish(q);
    assert(pt_studio_queue_push(q,&source)==PT_QUEUE_CLOSED);
    assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK);
    assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
    assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_DONE);
    assert(pt_studio_queue_close(q)==PT_QUEUE_OK && !live);
    q=pt_studio_queue_open(&a,2);assert(q);
    assert(pt_studio_queue_push(q,&source)==PT_QUEUE_OK);assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK);
    assert(pt_studio_queue_push(q,&source)==PT_QUEUE_OK);pt_studio_queue_abort(q);
    assert(out->data[1]==257 && out->data[4]==-8388608 && pt_studio_queue_close(q)==PT_QUEUE_BUSY);
    assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
    assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_DONE);
    assert(pt_studio_queue_close(q)==PT_QUEUE_OK && !live);
    puts("STUDIO QUEUE PASS: true24 copies, bounded pressure, lease retention, stale refusal, drain and abort cleanup");return 0;
}
