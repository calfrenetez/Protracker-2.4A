#include "studio_queue.h"
#include <string.h>
struct block {struct pt_pcm pcm;int32_t data[512];};
struct pt_studio_queue {
    struct pt_allocator allocator;unsigned capacity,head,count,leased,closed;
    uint64_t ticket;struct block blocks[];
};
struct pt_studio_queue *pt_studio_queue_open(const struct pt_allocator *a,unsigned blocks)
{
    struct pt_studio_queue *q;size_t bytes;
    if(!a || !a->allocate || !a->release || !blocks || blocks>8)return NULL;
    bytes=sizeof(*q)+blocks*sizeof(struct block);q=a->allocate(a->context,bytes);if(!q)return NULL;
    memset(q,0,bytes);q->allocator=*a;q->capacity=blocks;return q;
}
enum pt_queue_result pt_studio_queue_push(struct pt_studio_queue *q,const struct pt_pcm *p)
{
    struct block *b;
    if(!q || !p)return PT_QUEUE_INVALID;
    if(q->closed)return PT_QUEUE_CLOSED;
    if(q->count==q->capacity)return PT_QUEUE_FULL;
    if(!p->frames || p->frames>256 || p->channels!=2 || p->bits!=24 || p->rate!=48000 || pt_pcm_validate(p)!=PT_PCM_OK)return PT_QUEUE_INVALID;
    b=&q->blocks[(q->head+q->count)%q->capacity];
    memmove(b->data,p->data,p->frames*2*sizeof(*p->data));
    b->pcm=(struct pt_pcm){b->data,512,p->frames,48000,2,24};++q->count;return PT_QUEUE_OK;
}
enum pt_queue_result pt_studio_queue_acquire(struct pt_studio_queue *q,const struct pt_pcm **out,uint64_t *ticket)
{
    if(!q || !out || !ticket)return PT_QUEUE_INVALID;
    if(q->leased)return PT_QUEUE_BUSY;
    if(!q->count)return q->closed?PT_QUEUE_DONE:PT_QUEUE_EMPTY;
    if(q->ticket==UINT64_MAX)return PT_QUEUE_INVALID;
    ++q->ticket;q->leased=1;*out=&q->blocks[q->head].pcm;*ticket=q->ticket;return PT_QUEUE_OK;
}
enum pt_queue_result pt_studio_queue_release(struct pt_studio_queue *q,uint64_t ticket)
{
    if(!q || !q->leased || ticket!=q->ticket)return PT_QUEUE_INVALID;
    q->leased=0;q->head=(q->head+1)%q->capacity;--q->count;return PT_QUEUE_OK;
}
void pt_studio_queue_finish(struct pt_studio_queue *q) {if(q)q->closed=1;}
void pt_studio_queue_abort(struct pt_studio_queue *q) {if(q) {q->closed=1;q->count=q->leased?1:0;}}
enum pt_queue_result pt_studio_queue_close(struct pt_studio_queue *q)
{
    if(q) {struct pt_allocator a=q->allocator;if(q->leased)return PT_QUEUE_BUSY;a.release(a.context,q);}
    return PT_QUEUE_OK;
}
